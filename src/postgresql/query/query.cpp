#include "query.hpp"
#include <string>
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

	try
	{
		// Attempt query
		pqxx::nontransaction tx{ *connection };
		connection->prepare("prepared_query", query);
		result = tx.exec_prepared("prepared_query", params);
		connection->unprepare("prepared_query");
		tx.commit();
	}
	catch (pqxx::broken_connection&)
	{
		// Reconnect and attempt query again for broken connections
		connection = std::make_shared<pqxx::connection>(ptr->profile->conn_str);
		pqxx::nontransaction tx{ *connection };
		connection->prepare("prepared_query", query);
		result = tx.exec_prepared("prepared_query", params);
		connection->unprepare("prepared_query");
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
			const char* column_name = result.column_name(col);
			pqxx::oid oid = result.column_type(column_name);
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
