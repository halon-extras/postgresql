#include "configuration.hpp"
#include <syslog.h>
#include <sstream>

bool parse_config(HalonConfig* cfg, ParsedConfig* parsed_cfg)
{
	const char* default_profile = HalonMTA_config_string_get(HalonMTA_config_object_get(cfg, "default_profile"), nullptr);
	parsed_cfg->default_profile = default_profile ? default_profile : "__default";

	auto profiles = HalonMTA_config_object_get(cfg, "profiles");
	if (profiles)
	{
		size_t x = 0;
		HalonConfig* profile;
		while ((profile = HalonMTA_config_array_get(profiles, x++)))
		{
			auto postgresql_profile = std::make_shared<PostgreSQLProfile>();

			const char* id = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "id"), nullptr);
			if (!id)
			{
				syslog(LOG_CRIT, "postgresql: Missing id for profile");
				return false;
			}

			std::stringstream conn_str;

			const char* host = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "host"), nullptr);
			if (host)
				conn_str << "host=" << host;
			else
				conn_str << "host=" << "127.0.0.1";

			const char* port = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "port"), nullptr);
			if (port)
				conn_str << " port=" << port;
			else
				conn_str << " port=" << 5432;

			const char* database = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "database"), nullptr);
			if (database)
				conn_str << " dbname=" << database;

			const char* username = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "username"), nullptr);
			if (username)
				conn_str << " user=" << username;

			const char* password = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "password"), nullptr);
			if (password)
				conn_str << " password=" << password;

			postgresql_profile->conn_str = conn_str.str();

			const char* connect_timeout = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "connect_timeout"), nullptr);
			if (connect_timeout)
				conn_str << " connect_timeout=" << connect_timeout;
			else
				conn_str << " connect_timeout=" << 5;

			const char* pool_size = HalonMTA_config_string_get(HalonMTA_config_object_get(profile, "pool_size"), nullptr);
			if (pool_size)
				postgresql_profile->pool_size = (unsigned int)strtoul(pool_size, nullptr, 10);

			parsed_cfg->profiles[id] = postgresql_profile;
		}
	}
	if (parsed_cfg->profiles.empty())
	{
		auto postgresql_profile = std::make_shared<PostgreSQLProfile>();
		parsed_cfg->profiles[parsed_cfg->default_profile] = postgresql_profile;
	}

	return true;
}
