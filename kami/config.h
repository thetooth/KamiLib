#pragma once

#include "json.h"
#include <string>
#include <fstream>

namespace klib {
	class Config
	{
	public:
		int status;
		json::Value data;
		Config(){};
		Config(const char *configFile){
			status = 0;
			std::ifstream configStream(configFile);
			if (configStream.bad()){
				status = 1;
			}else{
				try {
					json::read(configStream, data);
				}
				catch (json::JsonException e){
					status = 1;
					throw KLGLException("JSON::%s:%d::%s", __FILE__, __LINE__, e.what());
				}
			}
		};
		~Config(){
			
		};

		inline int GetInteger(const char *section, const char *key, int defaultValue = 0){
			if (!data.HasMember(section)){
				return defaultValue;
			}
			json::Value const& sec = data.GetObjectMember(section);
			if (sec.HasMember(key)){
				return sec.GetIntMember(key);
			}else{
				return defaultValue;
			}
		}

		inline bool GetBoolean(const char *section, const char *key, bool defaultValue = false){
			if (!data.HasMember(section)){
				return defaultValue;
			}
			json::Value const& sec = data.GetObjectMember(section);
			if (sec.HasMember(key)){
				return sec.GetBoolMember(key);
			}else{
				return defaultValue;
			}
		}

		inline std::string GetString(const char *section, const char *key, std::string defaultValue = ""){
			if (!data.HasMember(section)){
				return defaultValue;
			}
			json::Value const& sec = data.GetObjectMember(section);
			if (sec.HasMember(key)){
				return sec.GetStringMember(key);
			}else{
				return defaultValue;
			}
		}

		inline int ParseError(){
			switch (status)
			{
			default:
				break;
			}
			return status;
		}
	};
}