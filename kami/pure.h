/*
	PURE.H - Cross-platform utility header.
	Copyright (C) 2005-2011, Ameoto Systems Inc. All rights reserved.
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sstream>
#include <string.h>
#include <wchar.h>
#include <unordered_map>

namespace klib {

#pragma region Global_Defines

#ifndef APP_MOTD
#define APP_MOTD "(c) 2005-2012 Ameoto Systems Inc. All Rights Reserved.\n\n"
#endif
#define APP_BUFFER_SIZE 4096
#define APP_ENABLE_MIPMAP 0
#define APP_ANISOTROPY 4.0
#define APP_CONSOLE_LINES 32

	// Determine compiler
#if		defined __ECC || defined __ICC || defined __INTEL_COMPILER
#	define APP_COMPILER_STRING "Intel C/C++"
#elif	defined __GNUC__
#	define APP_COMPILER_STRING "GNU/GCC"
#elif	defined _MSC_VER
#	define APP_COMPILER_STRING "Microsoft Visual C++"
#elif  !defined APP_COMPILER_STRING
#	define APP_COMPILER_STRING "Unknown compiler"
#endif

	// Macro to print out actual def names
#	define DEFASSTR(x)	#x

	// Check arch
#if _WIN64 || __x86_64__ || __ppc64__
#	define APP_ARCH_STRING "AMD64"
#else
#	define APP_ARCH_STRING "x86"
#endif

#if defined(_WIN32)
	#define APP_WINDOWMANAGER_CLASS Win32WM
#else
	#define APP_WINDOWMANAGER_CLASS X11WM
#endif

#pragma endregion

#pragma region Global_Data

	static int cursor_x = 0, cursor_y = 0, clBufferAllocLen = APP_BUFFER_SIZE;
	static bool cursor_L_down = false;
	extern bool KLGLDebug;
	extern bool resizeEvent;
	extern char *clBuffer;
	//extern std::unordered_map<std::string, void*> clHashTable;

#pragma endregion

#pragma region Inline_System_Utilitys

	inline int substr(char *dest, const char *src, int start, int len){
		if (src == 0 || strlen(src) == 0 || strlen(src) < start || strlen(src) < (start+len)){
			return -1;
		}

		strncpy(dest, src + start, len);
		return 0;
	}

	inline int cl(const char* format, ...){

		// File handle
		static FILE *fp;
		if (fp == NULL){
			fp = fopen("cl.log", "w+");
		}

		// Exit situation
		if (format == NULL && fp != NULL){
			fclose(fp);
			return -1;
		}

		if (strlen(clBuffer)+strlen(format) >= clBufferAllocLen){
			std::fill_n(clBuffer, 4, '\0');
			/*clBufferAllocLen = clBufferAllocLen*4;
			char* tclBuffer = (char*)realloc(clBuffer, clBufferAllocLen);
			fill_n(tclBuffer+clBufferAllocLen, clBufferAllocLen, '\0');
			if (tclBuffer != NULL){
				clBuffer = tclBuffer;
			}else{
				exit(1);
			}*/
		}
		
		// Temporary buffer
		char *tBuffer = new char[clBufferAllocLen];
		std::fill_n(tBuffer, clBufferAllocLen, '\0');
		
		int ret = 0, numLn = 0, numLnP = 0;

		// Do normal output
		va_list arg;
		va_start(arg, format);
		vsprintf(tBuffer, format, arg);
		ret = printf(tBuffer);
		va_end(arg);

		// Append the new string to the buffer
		size_t bufferLen = strlen(clBuffer);
		memcpy(clBuffer+bufferLen, tBuffer, clBufferAllocLen);

		// Flush to log file
		if (fp != NULL){
			fwrite(tBuffer, 1, strlen(tBuffer), fp);
			fflush(fp);
		}

		// Tail buffer
		numLn = APP_CONSOLE_LINES+1;
		while (numLn > APP_CONSOLE_LINES)
		{
			numLn = 0;
			for (char* c = clBuffer; *c != '\0'; c++)
			{
				if (*c == '\n'){ numLn++;	}
				if (numLn == 0){ numLnP++;	}
			}
			if (numLn > APP_CONSOLE_LINES){
				memcpy(tBuffer, clBuffer+numLnP+1, clBufferAllocLen);
				memcpy(clBuffer, tBuffer, clBufferAllocLen);
			}
		}

		delete [] tBuffer;
		return ret;
	}

	inline float ubtof(int color){
		return (float)color/255.0f;
	}

	

#pragma endregion

#pragma region Misc

	struct cmp_cstring : public std::binary_function<const char*, const char*, bool> {
	public:
		bool operator()(const char* str1, const char* str2) const {
			if (str1 == NULL || str2 == NULL){
				return false;
			}
			return strcmp(str1, str2) < 0;
		}
	};

	template<typename T> inline std::string stringify(T const& x)
	{
		std::ostringstream out;
		out << x;
		return out.str();
	}

	template<typename T> inline std::wstring wstringify(T const& x)
	{
		std::wstringstream out;
		out << x;
		return out.str();
	}

#pragma endregion

}
