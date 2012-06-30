#pragma once

#include "masternoodles.h"

using namespace klib;

namespace NeoPlatformer{
	typedef std::unordered_map<std::string, void*> VarList;
	class Console
	{
	public:
		char* buffer;
		int bufferLen;
		VarList callbackPtrMap;

		Console(int bufferSize = 4096);
		~Console(void);

		void draw();
		void input(char* string);
		void parseCommandQueue();
	private:
		KLGLFont* font;
	};
}
