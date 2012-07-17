#include <algorithm>
#include <cctype>
#include <cstdlib>
#include "ini.h"
#include "INIReader.h"

#ifndef _WIN32
#include "unix.h"
#endif

KLGLINIReader::KLGLINIReader(char* filename)
{
	_error = ini_parse(filename, ValueHandler, this);
}

int KLGLINIReader::ParseError()
{
	return _error;
}

const char* KLGLINIReader::Get(char* section, char* name, char* default_value)
{
	std::string key = MakeKey(section, name);
	return _values.count(key) ? _values[key].c_str() : default_value;
}

long KLGLINIReader::GetInteger(char* section, char* name, long default_value)
{
	const char* valstr = Get(section, name, "");
	char* end;
	// This parses "1234" (decimal) and also "0x4D2" (hex)
	long n = strtol(valstr, &end, 0);
	return end > valstr ? n : default_value;
}

bool KLGLINIReader::GetBoolean(char* section, char* name, bool default_value)
{
	const char* valstr = Get(section, name, "");
	if (stricmp(valstr, "true") == 0 || stricmp(valstr, "yes") == 0 || stricmp(valstr, "on") == 0 || stricmp(valstr, "1") == 0){
		return true;
	}else if(stricmp(valstr, "false") == 0 || stricmp(valstr, "no") == 0 || stricmp(valstr, "off") == 0 || stricmp(valstr, "0") == 0){
		return false;
	}else{
		return default_value;
	}
}

std::string KLGLINIReader::MakeKey(const char* section, const char* name)
{
	char* key = new char[strlen(section)+strlen(name)+1];
	sprintf(key, "%s.%s", section, name);
	for (int i = 0; i < strlen(key); i++){
		key[i] = tolower(key[i]);
	}
	return key;
}

int KLGLINIReader::ValueHandler(void* user, const char* section, const char* name, const char* val)
{
	KLGLINIReader* reader = (KLGLINIReader*)user;
	reader->_values[MakeKey(section, name)] = const_cast<char*>(val);
	return 1;
}
