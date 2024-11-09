#ifndef POSTGRESQL_HPP_
#define POSTGRESQL_HPP_

#include <HalonMTA.h>
#include <pqxx/pqxx>
#include <mutex>
#include <memory>
#include <condition_variable>

struct PostgreSQLProfile
{
	std::string conn_str;
	std::vector<std::shared_ptr<pqxx::connection>> pool;
	unsigned int pool_size = 1;
	unsigned int pool_added = 0;
	std::map<pqxx::oid, std::string> column_types;
	std::mutex mtx;
	std::condition_variable cv;
};

class PostgreSQL
{
  public:
	std::shared_ptr<PostgreSQLProfile> profile;
};

void PostgreSQL_constructor(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret);
void PostgreSQL_free(void* ptr);

#endif
