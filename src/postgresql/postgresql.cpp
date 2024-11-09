#include "postgresql.hpp"
#include "query/query.hpp"
#include "configuration.hpp"
#include "argument.hpp"
#include <mutex>
#include <memory>

extern ParsedConfig parsed_cfg;
extern std::mutex mutex_cfg;

void PostgreSQL_constructor(HalonHSLContext* hhc, HalonHSLArguments* args, HalonHSLValue* ret)
{
	std::string profile;
	if (!parse_hsl_argument_as_string(hhc, args, 0, false, profile))
		return;

	mutex_cfg.lock();

	if (profile.empty())
		profile = parsed_cfg.default_profile;

	std::shared_ptr<PostgreSQLProfile> postgresql_profile;

	auto profile_iterator = parsed_cfg.profiles.find(profile);
	if (profile_iterator == parsed_cfg.profiles.end())
	{
		mutex_cfg.unlock();
		HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
		HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "invalid profile", 0);
		return;
	}

	postgresql_profile = profile_iterator->second;

	mutex_cfg.unlock();

	PostgreSQL* ptr = new PostgreSQL();
	ptr->profile = postgresql_profile;

	HalonHSLObject* object = HalonMTA_hsl_object_new();
	HalonMTA_hsl_object_type_set(object, "PostgreSQL");
	HalonMTA_hsl_object_register_function(object, "query", PostgreSQL_query);
	HalonMTA_hsl_object_ptr_set(object, ptr, PostgreSQL_free);
	HalonMTA_hsl_value_set(ret, HALONMTA_HSL_TYPE_OBJECT, object, 0);
	HalonMTA_hsl_object_delete(object);
}

void PostgreSQL_free(void* ptr)
{
	PostgreSQL* x = (PostgreSQL*)ptr;
	delete x;
}
