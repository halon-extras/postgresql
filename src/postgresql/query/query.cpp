#include "query.hpp"
#include <string>
#include <set>
#include <vector>
#include "postgresql/postgresql.hpp"
#include <pqxx/pqxx>
#include "argument.hpp"

std::shared_ptr<pqxx::connection> get_connection(PostgreSQL* ptr)
{
	std::unique_lock<std::mutex> ul(ptr->profile->mtx);
	ptr->profile->cv.wait(ul, [&]() { return !ptr->profile->pool.empty() || ptr->profile->pool_added < ptr->profile->pool_size; });

	std::shared_ptr<pqxx::connection> connection;

	if (!ptr->profile->pool.empty())
	{
		// Reuse existing connection
		connection = ptr->profile->pool.front();
		ptr->profile->pool.erase(ptr->profile->pool.begin());
	}
	else
	{
		// Open new connection
		connection = std::make_shared<pqxx::connection>(ptr->profile->conn_str);
		++ptr->profile->pool_added;
	}

	return connection;
}

pqxx::result exec_prepared_retry(PostgreSQL* ptr, std::shared_ptr<pqxx::connection>& connection, const std::string& query, const pqxx::params& params)
{
	pqxx::result result;
	bool sent = false;

	try
	{
		// Attempt query (unnamed statement, replaced by the next prepare, so no cleanup is needed)
		pqxx::nontransaction tx{ *connection };
		connection->prepare(query);
		sent = true;
		result = tx.exec_prepared("", params);
		tx.commit();
	}
	catch (pqxx::broken_connection&)
	{
		// Never retry once the query has been sent, it may already have been executed
		if (sent)
			throw;

		// Reconnect and attempt query again for broken connections
		connection = std::make_shared<pqxx::connection>(ptr->profile->conn_str);
		pqxx::nontransaction tx{ *connection };
		connection->prepare(query);
		result = tx.exec_prepared("", params);
		tx.commit();
	}

	return result;
}

std::string get_column_type(PostgreSQL* ptr, std::shared_ptr<pqxx::connection> connection, const pqxx::oid oid)
{
	std::lock_guard<std::mutex> ul(ptr->profile->mtx);

	std::string column_type;

	auto it = ptr->profile->column_types.find(oid);
	if (it != ptr->profile->column_types.end())
	{
		// Get column type from cache
		column_type = it->second;
	}
	else
	{
		// Get column type from database
		pqxx::params params;
		params.append(oid);
		pqxx::result res = exec_prepared_retry(ptr, connection, "SELECT typname FROM pg_type WHERE oid = $1", params);
		column_type = res[0][0].as<std::string>();
		ptr->profile->column_types[oid] = column_type;
	}

	return column_type;
}

// Make column names unique so no value is shadowed in the HSL result array.
// Repeated names get an occurrence suffix ("?column?", "?column?_2", ...)
// that never collides with a name present in the result.
std::vector<std::string> get_column_names(const pqxx::result& result)
{
	std::set<std::string> originals;
	for (pqxx::row_size_type col = 0; col < result.columns(); ++col)
		originals.insert(result.column_name(col));

	std::vector<std::string> names;
	std::set<std::string> used;
	for (pqxx::row_size_type col = 0; col < result.columns(); ++col)
	{
		std::string name = result.column_name(col);
		if (used.count(name))
		{
			std::string candidate;
			size_t n = 2;
			do
				candidate = name + "_" + std::to_string(n++);
			while (used.count(candidate) || originals.count(candidate));
			name = candidate;
		}
		used.insert(name);
		names.push_back(name);
	}
	return names;
}

void PostgreSQL_query(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	PostgreSQL* ptr = (PostgreSQL*)HalonMTA_hsl_object_ptr_get(hhc);

	// Parse arguments
	std::string query;
	if (!parse_hsl_argument_as_string(hhc, args, 0, true, query))
		return;

	pqxx::params params;
	if (!parse_hsl_argument_as_params(hhc, args, 1, false, params))
		return;

	// Get connection
	std::shared_ptr<pqxx::connection> connection;
	try
	{
		connection = get_connection(ptr);
	}
	catch (std::exception const& err)
	{
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
		return;
	}

	// Execute prepared statement (with retry for broken connections)
	pqxx::result result;
	try
	{
		result = exec_prepared_retry(ptr, connection, query, params);
	}
	catch (std::exception const& err)
	{
		// Put the connection back into the pool
		ptr->profile->mtx.lock();
		ptr->profile->pool.push_back(connection);
		ptr->profile->mtx.unlock();
		ptr->profile->cv.notify_one();

		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
		return;
	}

	const std::vector<std::string> column_names = get_column_names(result);

	// Set HSL response
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);
	int idx = 0;
	for (auto const& row : result)
	{
		HalonHSLValue *k1, *v1;
		HalonMTA_hsl_value_array_add(ret, &k1, &v1);

		double i = idx;
		HalonMTA_hsl_value_set(k1, HALONMTA_HSL_TYPE_NUMBER, &i, 0);
		HalonMTA_hsl_value_set(v1, HALONMTA_HSL_TYPE_ARRAY, nullptr, 0);

		for (pqxx::row_size_type col = 0; col < result.columns(); ++col)
		{
			const char* column_name = column_names[col].c_str();
			pqxx::oid oid = result.column_type(col);
			std::string column_type;

			try
			{
				column_type = get_column_type(ptr, connection, oid);
			}
			catch (std::exception const& err)
			{
				// Put the connection back into the pool
				ptr->profile->mtx.lock();
				ptr->profile->pool.push_back(connection);
				ptr->profile->mtx.unlock();
				ptr->profile->cv.notify_one();

				HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
				HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
				return;
			}

			HalonHSLValue *k2, *v2;
			HalonMTA_hsl_value_array_add(v1, &k2, &v2);
			HalonMTA_hsl_value_set(k2, HALONMTA_HSL_TYPE_STRING, column_name, 0);

			if (column_type == "bool")
			{
				std::optional<bool> column_value = row.at(col).get<bool>();
				if (column_value.has_value())
				{
					bool value = column_value.value();
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_BOOLEAN, &value, 0);
				}
				else
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NONE, nullptr, 0);
			}
			else if (column_type == "int" || column_type == "int2" || column_type == "int4" || column_type == "serial2" || column_type == "serial4")
			{
				std::optional<int> column_value = row.at(col).get<int>();
				if (column_value.has_value())
				{
					double value = (double)column_value.value();
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NUMBER, &value, 0);
				}
				else
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NONE, nullptr, 0);
			}
			else if (column_type == "float4" || column_type == "float8")
			{
				std::optional<double> column_value = row.at(col).get<double>();
				if (column_value.has_value())
				{
					double value = column_value.value();
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NUMBER, &value, 0);
				}
				else
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NONE, nullptr, 0);
			}
			else
			{
				std::optional<const char*> column_value = row.at(col).get<const char*>();
				if (column_value.has_value())
				{
					const char* value = column_value.value();
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_STRING, value, 0);
				}
				else
					HalonMTA_hsl_value_set(v2, HALONMTA_HSL_TYPE_NONE, nullptr, 0);
			}
		}

		++idx;
	}

	// Put the connection back into the pool
	ptr->profile->mtx.lock();
	ptr->profile->pool.push_back(connection);
	ptr->profile->mtx.unlock();
	ptr->profile->cv.notify_one();
}
