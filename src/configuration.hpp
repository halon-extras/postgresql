#ifndef CONFIGURATION_HPP_
#define CONFIGURATION_HPP_

#include "postgresql/postgresql.hpp"
#include <string>
#include <HalonMTA.h>

struct ParsedConfig
{
	std::string default_profile;
	std::map<std::string, std::shared_ptr<PostgreSQLProfile>> profiles;
};

bool parse_config(HalonConfig* cfg, ParsedConfig* parsed_cfg);

#endif
