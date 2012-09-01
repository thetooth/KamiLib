#include "console.h"

namespace NeoPlatformer{
	Console::Console(int bufferSize){
		bufferLen = bufferSize;
		buffer = new char[bufferLen];
		fill_n(buffer, bufferLen, '\0');
		font = new KLGLFont();
	}

	Console::~Console(void){
		delete font;
		delete buffer;
	}

	void Console::draw(){
		font->Draw(4, 8, buffer);
	}

	void Console::input(char* string){
		if (string == NULL || strlen(string) < 1){
			throw KLGLException("Console string error");
		}
		// TODO: Write fancy parser
		int location;
		char* varName;
		switch(string[0]){
		case '$':						// Process var
			location = strchr(string, (int)'=')-string+1;
			varName = substr(string, 1, location);
			if (strlen(varName) > 0){
				auto p = callbackPtrMap.find(varName);
				auto c = substr(string, location+1, strlen(string)-location);
				if (p == callbackPtrMap.end()){
					callbackPtrMap[varName] = c;
					cl("Wrote new var %s with value %s\n", varName, c);
				}else{
					cl("Wrote existing var %s with value %s\n", p->first.c_str(), (char*)p->second);
					callbackPtrMap[varName] = c;
				}
				//memcpy(p->second, c, strlen(string)-location);
			}
			break;
		case '!':						// Macro

			break;
		default:						// Append to buffer
			/*if (strlen(buffer)+strlen(string)+1 >= bufferLen){
				throw std::exception("Attempted to append text to a full buffer!");
				return;
			}
			strcat(buffer, string);
			strcat(buffer, "\n");*/
			PyRun_SimpleString(string);
		}
	}
}