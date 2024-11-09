#include "argument.hpp"

bool parse_hsl_argument_as_bool(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, bool& value)
{
	HalonHSLValue* arg = HalonMTA_hsl_argument_get(args, index);
	if (arg)
	{
		if (HalonMTA_hsl_value_type(arg) != HALONMTA_HSL_TYPE_BOOLEAN)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "argument is not a boolean", 0);
			return false;
		}
		HalonMTA_hsl_value_get(arg, HALONMTA_HSL_TYPE_BOOLEAN, &value, nullptr);
	}
	else
	{
		if (required)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "missing required argument", 0);
			return false;
		}
	}
	return true;
}

bool parse_hsl_argument_as_double(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, double& value)
{
	HalonHSLValue* arg = HalonMTA_hsl_argument_get(args, index);
	if (arg)
	{
		if (HalonMTA_hsl_value_type(arg) != HALONMTA_HSL_TYPE_NUMBER)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "argument is not a number", 0);
			return false;
		}
		HalonMTA_hsl_value_get(arg, HALONMTA_HSL_TYPE_NUMBER, &value, nullptr);
	}
	else
	{
		if (required)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "missing required argument", 0);
			return false;
		}
	}
	return true;
}

bool parse_hsl_argument_as_string(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, std::string& value)
{
	const char* _value;
	HalonHSLValue* arg = HalonMTA_hsl_argument_get(args, index);
	if (arg)
	{
		if (HalonMTA_hsl_value_type(arg) != HALONMTA_HSL_TYPE_STRING)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "argument is not a string", 0);
			return false;
		}
		HalonMTA_hsl_value_get(arg, HALONMTA_HSL_TYPE_STRING, &_value, nullptr);
		value = _value;
	}
	else
	{
		if (required)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "missing required argument", 0);
			return false;
		}
	}
	return true;
}

bool parse_hsl_argument_as_params(HalonHSLContext* hhc, HalonHSLArguments* args, size_t index, bool required, pqxx::params& value)
{
	HalonHSLValue* arg = HalonMTA_hsl_argument_get(args, index);
	if (arg)
	{
		if (HalonMTA_hsl_value_type(arg) == HALONMTA_HSL_TYPE_ARRAY)
		{
			size_t _index = 0;
			HalonHSLValue *k, *v;
			while ((v = HalonMTA_hsl_value_array_get(arg, _index++, &k)))
			{
				if (HalonMTA_hsl_value_type(v) == HALONMTA_HSL_TYPE_STRING)
				{
					const char* _value;
					HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_STRING, &_value, nullptr);
					try
					{
						value.append(_value);
					}
					catch (std::exception const& err)
					{
						HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
						HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
						return false;
					}
				}
				else if (HalonMTA_hsl_value_type(v) == HALONMTA_HSL_TYPE_NUMBER)
				{
					double _value;
					HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_NUMBER, &_value, nullptr);
					try
					{
						value.append(_value);
					}
					catch (std::exception const& err)
					{
						HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
						HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
						return false;
					}
				}
				else if (HalonMTA_hsl_value_type(v) == HALONMTA_HSL_TYPE_BOOLEAN)
				{
					bool _value;
					HalonMTA_hsl_value_get(v, HALONMTA_HSL_TYPE_BOOLEAN, &_value, nullptr);
					try
					{
						value.append(_value);
					}
					catch (std::exception const& err)
					{
						HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
						HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, err.what(), 0);
						return false;
					}
				}
				else
				{
					HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
					HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "array item is not a valid type", 0);
					return false;
				}
			}
		}
		else
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "argument is not an array", 0);
			return false;
		}
	}
	else
	{
		if (required)
		{
			HalonHSLValue* e = HalonMTA_hsl_throw(hhc);
			HalonMTA_hsl_value_set(e, HALONMTA_HSL_TYPE_EXCEPTION, "missing required argument", 0);
			return false;
		}
	}
	return true;
}