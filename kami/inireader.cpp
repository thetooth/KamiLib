#include <algorithm>
#include <cctype>
#include <cstdlib>
#include "../ini.h"
#include "INIReader.h"

using std::string;

KLGLINIReader::KLGLINIReader(string filename)
{
	_error = ini_parse(filename.c_str(), ValueHandler, this);
}

int KLGLINIReader::ParseError()
{
	return _error;
}

string KLGLINIReader::Get(string section, string name, string default_value)
{
	string key = MakeKey(section, name);
	return _values.count(key) ? _values[key] : default_value;
}

long KLGLINIReader::GetInteger(string section, string name, long default_value)
{
	string valstr = Get(section, name, "");
	const char* value = valstr.c_str();
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	long n = strtol(value, &end, 0);
	return end > value ? n : default_value;
}

bool KLGLINIReader::GetBoolean(string section, string name, bool default_value)
{
	string valstr = Get(section, name, "");
	// Convert to lower case to make string comparisons case-insensitive
	std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
	if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
		return true;
	else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
		return false;
	else
		return default_value;
}

string KLGLINIReader::MakeKey(string section, string name)
{
	string key = section + "." + name;
	// Convert to lower case to make section/name lookups case-insensitive
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	return key;
}

int KLGLINIReader::ValueHandler(void* user, const char* section, const char* name,
	const char* value)
{
	KLGLINIReader* reader = (KLGLINIReader*)user;
	reader->_values[MakeKey(section, name)] = value;
	return 1;
}