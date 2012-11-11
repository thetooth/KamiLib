#pragma once

#include "micro_cjson.hpp"

using namespace MicroCJson;

namespace klib {
	class KLGLConfig
	{
	public:
		int status;
		JSONData *data;
		KLGLConfig(const char *configFile){
			status = 0;
			data = nullptr;
			FILE *fp = fopen(configFile, "r");
			if(fp == NULL){
				status = 1;
				return;
			}
			fseek (fp , 0 , SEEK_END);
			int size = ftell(fp);
			rewind (fp);
			char *config = new char[size+1];
			fread(config, 1, size, fp);
			data = parseJSON(config);
			if(data == nullptr){
				status = 2;
			}
			fclose(fp);
			delete [] config;
		};
		~KLGLConfig(){
			delete [] data;
		};

		inline JSONData* GetPtr(const char *section, const char *key){
			JSONData *sectionPtr = JSONData_mapGet(data, section);
			return JSONData_mapGet(sectionPtr, key);
		}

		inline int GetInteger(const char *section, const char *key, int defaultValue = 0){
			int *ptr = JSONData_readInt(GetPtr(section, key));
			if (ptr != nullptr){
				return *ptr;
			}else{
				return defaultValue;
			}
		}

		inline bool GetBoolean(const char *section, const char *key, bool defaultValue = false){
			bool *ptr = JSONData_readBool(GetPtr(section, key));
			if (ptr != nullptr){
				return *ptr;
			}else{
				return defaultValue;
			}
		}

		inline string GetString(const char *section, const char *key, string defaultValue = ""){
			string *ptr = JSONData_readString(GetPtr(section, key));
			if (ptr != nullptr){
				return *ptr;
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