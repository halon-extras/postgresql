#include "configuration.hpp"
#include "argument.hpp"
#include "postgresql/postgresql.hpp"
#include <HalonMTA.h>

ParsedConfig parsed_cfg;
std::mutex mutex_cfg;

HALON_EXPORT
int Halon_version()
{
	return HALONMTA_PLUGIN_VERSION;
}

HALON_EXPORT
bool Halon_init(HalonInitContext* hic)
{
	HalonConfig* cfg = nullptr;
	HalonMTA_init_getinfo(hic, HALONMTA_INIT_APPCONFIG, nullptr, 0, &cfg, nullptr);
	if (!cfg)
		return false;

	if (!parse_config(cfg, &parsed_cfg))
		return false;

	return true;
}

HALON_EXPORT
void Halon_config_reload(HalonConfig* cfg)
{
	ParsedConfig new_parsed_cfg;
	if (!parse_config(cfg, &new_parsed_cfg))
		return;

	mutex_cfg.lock();
	parsed_cfg = new_parsed_cfg;
	mutex_cfg.unlock();
}

HALON_EXPORT
bool Halon_hsl_register(HalonHSLRegisterContext* hhrc)
{
	HalonMTA_hsl_module_register_function(hhrc, "PostgreSQL", PostgreSQL_constructor);
	return true;
}

HALON_EXPORT
void Halon_cleanup()
{
	// Remove the last references to the shared pointers
	parsed_cfg.profiles.clear();
}
