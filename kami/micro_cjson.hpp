/*
MicroCJson - C++11 Json parser and stack machine

Copyright (c) 2012 Ameoto Systems Inc. All rights reserved.
Original code by cristianob

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <cstdlib>
#include <map>
#include <vector>
#include <string>
#include <cstdio>
#include <iostream>

using namespace std;

namespace MicroCJson {

	typedef enum {
		VDT_NULL, VDT_BOOL, VDT_INT, VDT_ULONG, VDT_STRING, VDT_LIST, VDT_MAP
	} JSONDataType;

	typedef enum {
		UNKNOWN, IN_KEY, IN_VALUE
	} JsonParserState;

	typedef enum {
		MAP, LIST
	} JsonParserContainer;

	class JSONData {
	public:
		void *pointer;
		JSONDataType type;

		JSONData* operator[](const char *s)
		{
			if(this->type != VDT_MAP)
				return nullptr;

			return ((map<string, struct JSONData*> *) this->pointer)->find(string(s))->second;
		}

		JSONData* operator[](unsigned int s)
		{
			if(this->type != VDT_LIST)
				return nullptr;

			return ((vector<struct JSONData*> *) this->pointer)->at(s);
		}
	};

	const std::string JSONData_listDump(JSONData* d_list, int i);

	inline JSONData *vdgi(JSONData *p, int k){
		return ((* ((JSONData *) p))[k]);
	}

	inline JSONData *vdgc(JSONData *p, const char *k){
		return ((* ((JSONData *) p))[k]);
	}

	inline string JSONData_toString(JSONData* data)
	{
		if(data->type == VDT_NULL)
			return "(null value)";
		else if(data->type == VDT_BOOL){
			if(*(bool *) data->pointer == true)
				return "true";
			else
				return "false";
		}
		else if(data->type == VDT_INT){
			char rtr[32];
			sprintf(rtr,"%d", *(int *) data->pointer);
			return string(rtr);
		}
		else if(data->type == VDT_ULONG){
			char rtr[64];
			sprintf(rtr,"%lu", *(unsigned long *) data->pointer);
			return string(rtr);
		}
		else if(data->type == VDT_STRING)
			return *((string *) data->pointer);
		else if(data->type == VDT_LIST)
			return "LIST";
		else if(data->type == VDT_MAP)
			return "MAP";
		else
			return "";
	}

	inline JSONData *JSONData_mapCreate()
	{
		JSONData *vd = new JSONData;

		vd->pointer	= (void *) new map<string, JSONData*>();
		vd->type	= VDT_MAP;

		return vd;
	}

	inline bool JSONData_mapAdd(JSONData* d_map, string key, JSONData *value)
	{
		if(d_map->type != VDT_MAP)
			return false;

		((map<string, JSONData*> *) d_map->pointer)->insert( pair<string,JSONData*>(key, value) );
		return true;
	}

	inline JSONData *JSONData_mapGet(JSONData* d_map, string key)
	{
		if(d_map->type != VDT_MAP)
			return nullptr;

		auto tmp = ((map<string, JSONData*> *) d_map->pointer)->find(key);
		if (tmp == ((map<string, JSONData*> *) d_map->pointer)->end()){
			return nullptr;
		}else{
			return tmp->second;
		}
	}

	inline const std::string JSONData_mapDump(JSONData* d_map, int i)
	{	
		std::stringstream buffer(stringstream::in | stringstream::out);
		map<string, JSONData*> *p_map = (map<string, JSONData*> *) d_map->pointer;
		map<string, JSONData*>::iterator it;

		for(int j=0; j<i; j++){
			buffer << '\t';
		}
		buffer << "{" << endl;
		for ( it=p_map->begin() ; it != p_map->end(); it++ ){
			if(it->second->type == VDT_MAP){
				for(int j=0; j<i; j++)
					buffer << '\t';
				buffer << it->first << " => ";

				buffer << JSONData_mapDump(it->second, i+1);
			}

			else if(it->second->type == VDT_LIST){
				for(int j=0; j<i; j++)
					buffer << '\t';
				buffer << it->first << " => ";

				buffer << JSONData_listDump(it->second, i+1);
			}

			else if(it->second->type == VDT_STRING){
				for(int j=0; j<i; j++)
					buffer << '\t';
				buffer << it->first << " => \"" << JSONData_toString(it->second) << '\"' << endl;
			}

			else {
				for(int j=0; j<i; j++)
					buffer << '\t';
				buffer << it->first << " => " << JSONData_toString(it->second) << endl;
			}
		}
		for(int j=0; j<i; j++){
			buffer << '\t';
		}
		buffer << "}" << endl;

		return buffer.str();
	}

	inline JSONData *JSONData_listCreate()
	{
		JSONData *vd = new JSONData;

		vd->pointer	= (void *) new vector<JSONData*>();
		vd->type	= VDT_LIST;

		return vd;
	}

	inline bool JSONData_listAdd(JSONData* d_map, JSONData *value)
	{
		if(d_map->type != VDT_LIST)
			return false;

		((vector<JSONData*> *) d_map->pointer)->push_back( value );
		return true;
	}

	inline const std::string JSONData_listDump(JSONData* d_list, int i)
	{
		std::stringstream buffer(stringstream::in|stringstream::out);
		vector<JSONData*> *p_list = (vector<JSONData*> *) d_list->pointer;

		buffer << "[" << endl;
		for ( unsigned int z=0; z<p_list->size(); z++ ){
			if((*p_list)[z]->type == VDT_MAP){
				for(int j=0; j<i; j++)
					buffer << '\t';

				buffer << JSONData_mapDump((JSONData *) (*p_list)[z], i+1);
			}

			else if((*p_list)[z]->type == VDT_LIST){
				for(int j=0; j<i; j++)
					buffer << '\t';

				buffer << JSONData_listDump((JSONData *) (*p_list)[z]->pointer, i+1);

				for(int j=0; j<i; j++)
					buffer << '\t';
				buffer << "]" << endl;
			}		

			else if((*p_list)[z]->type == VDT_STRING){
				for(int j=0; j<i+1; j++)
					buffer << '\t';
				buffer << JSONData_toString((*p_list)[z]) << endl;
			}

			else {
				for(int j=0; j<i; j++)
					buffer << '\t';
				buffer << JSONData_toString((*p_list)[z]) << ", ";
			}
		}
		for(int j=0; j<i; j++)
			buffer << '\t';
		buffer << "]" << endl;

		return buffer.str();
	}

	inline JSONData *JSONData_createNull()
	{
		JSONData *vd = new JSONData;

		vd->pointer	= NULL;
		vd->type	= VDT_NULL;

		return vd;
	}

	inline JSONData *JSONData_createByBool(bool d)
	{
		JSONData *vd = new JSONData;

		vd->pointer	= (void *) new bool(d);
		vd->type	= VDT_BOOL;

		return vd;
	}

	inline JSONData *JSONData_createByInt(int d)
	{
		JSONData *vd = new JSONData;

		vd->pointer	= (void *) new int(d);
		vd->type	= VDT_INT;

		return vd;
	}

	inline JSONData *JSONData_createByULong(unsigned long d)
	{
		JSONData *vd = new JSONData;

		vd->pointer	= (void *) new unsigned long(d);
		vd->type	= VDT_ULONG;

		return vd;
	}

	inline JSONData *JSONData_createByString(string d)
	{
		JSONData *vd = new JSONData;

		vd->pointer	= (void *) new string(d);
		vd->type	= VDT_STRING;

		return vd;
	}

	inline bool *JSONData_readBool(JSONData *data)
	{
		if(data != nullptr && data->type == VDT_BOOL)
			return (bool *) data->pointer;
		else
			return nullptr;
	}

	inline int *JSONData_readInt(JSONData *data)
	{
		if(data != nullptr && (data->type == VDT_INT || data->type == VDT_ULONG))
			return (int *) data->pointer;
		else
			return nullptr;
	}

	inline unsigned long *JSONData_readULong(JSONData *data)
	{
		if(data != nullptr && (data->type == VDT_INT || data->type == VDT_ULONG))
			return (unsigned long *) data->pointer;
		else
			return nullptr;
	}

	inline string *JSONData_readString(JSONData *data)
	{
		if(data != nullptr && data->type == VDT_STRING)
			return (string *) data->pointer;
		else
			return nullptr;
	}

	inline void JSONData_delete(JSONData *data)
	{
		if(data->type == VDT_INT)
			delete (int *) data->pointer;
		else if(data->type == VDT_STRING)
			delete (string *) data->pointer;

		delete data;
	}

	inline const std::string JSONData_dump(JSONData *data)
	{
		if(data == nullptr){
			return "!nullptr";
		}else if(data->type == VDT_MAP){
			return JSONData_mapDump(data, 0);
		}else if(data->type == VDT_LIST){
			return JSONData_listDump(data, 0);
		}else{
			return "!unknown";
		}
	}

	inline bool checknum(char s)
	{
		return (s >= 48 && s <= 57);
	}

	extern "C" inline JSONData *parseJSON(string data)
	{
		const char *dc = data.c_str();

		int p = -1;
		JsonParserState state = UNKNOWN;
		string key = "";
		vector<string> key_level;
		vector<JSONData*> stack;
		vector<JsonParserContainer> stack_type;

		for(int i=0; dc[i] != '\0'; i++){
			if(dc[i] == '{'){
				state = UNKNOWN;
				key_level.push_back(key);
				stack.push_back(JSONData_mapCreate());
				stack_type.push_back(MAP);
				p++;
			}
			else if(dc[i] == '['){
				state = UNKNOWN;
				key_level.push_back(key);
				stack.push_back(JSONData_listCreate());
				stack_type.push_back(LIST);
				p++;
			}

			else if((dc[i] == '"' && (state == IN_VALUE || state == UNKNOWN)) && stack_type[stack_type.size() - 1] == MAP){
				key = "";

				state = IN_KEY;

				i++;
				while(dc[i] != '"'){
					key += dc[i];
					i++;
				}
			}

			else if((dc[i] == ']' || dc[i] == '}') && p>0){
				if(stack_type[stack_type.size() - 2] == LIST)
					JSONData_listAdd(stack[stack.size() - 2], stack[stack.size() - 1]);
				else if(stack_type[stack_type.size() - 2] == MAP)
					JSONData_mapAdd(stack[stack.size() - 2], key_level[key_level.size() - 1], stack[stack.size() - 1]);

				stack.pop_back();
				stack_type.pop_back();
				key_level.pop_back();
				p--;
			}

			else if(state == IN_KEY || (stack_type[stack_type.size() - 1] == MAP) || (stack_type[stack_type.size() - 1] == LIST)){
				state = IN_VALUE;

				if(dc[i] == ':'){
					state = IN_KEY;
					continue;
				}

				if(dc[i] == '"'){
					string value = "";

					i++;
					while(dc[i] != '"'){
						if(dc[i] == '\\' && dc[i+1] != 'u'){
							i++;
						}

						value += dc[i];
						i++;
					}

					if(stack_type[stack_type.size() - 1] == MAP)
						JSONData_mapAdd(stack[stack.size() - 1], key, JSONData_createByString(value));
					else
						JSONData_listAdd(stack[stack.size() - 1], JSONData_createByString(value));
				}
				else if(dc[i] == 'f'){
					i += 4;

					if(stack_type[stack_type.size() - 1] == MAP)
						JSONData_mapAdd(stack[stack.size() - 1], key, JSONData_createByBool(false));
					else
						JSONData_listAdd(stack[stack.size() - 1], JSONData_createByBool(false));
				}
				else if(dc[i] == 't'){
					i += 3;

					if(stack_type[stack_type.size() - 1] == MAP)
						JSONData_mapAdd(stack[stack.size() - 1], key, JSONData_createByBool(true));
					else
						JSONData_listAdd(stack[stack.size() - 1], JSONData_createByBool(true));
				}
				else if(dc[i] == 'n'){
					i += 3;

					if(stack_type[stack_type.size() - 1] == MAP)
						JSONData_mapAdd(stack[stack.size() - 1], key, JSONData_createNull());
					else
						JSONData_listAdd(stack[stack.size() - 1], JSONData_createNull());
				}
				else if( checknum(dc[i]) ){
					string value = "";

					while( checknum(dc[i]) ){
						value += dc[i];
						i++;
					}

					if(stack_type[stack_type.size() - 1] == MAP)
						JSONData_mapAdd(stack[stack.size() - 1], key, JSONData_createByULong( atoi(value.c_str()) ));
					else
						JSONData_listAdd(stack[stack.size() - 1], JSONData_createByULong( atoi(value.c_str()) ));
				}
			}
		}

		return stack[stack.size() - 1];
	}
}