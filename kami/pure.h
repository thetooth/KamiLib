/*
	PURE.H - Cross-platform utility header.
	Copyright (C) 2005-2011, Ameoto Systems Inc. All rights reserved.
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "kami.h"
#include "threads.h"

namespace klib {

#pragma region Global_Defines

#if		defined __ECC || defined __ICC || defined __INTEL_COMPILER
#	define APP_COMPILER_STRING "Intel C/C++"
#elif	defined __GNUC__
#	define APP_COMPILER_STRING "Gnu GCC"
#elif	defined _MSC_VER
#	define APP_COMPILER_STRING "Microsoft Visual C++"
#elif  !defined APP_COMPILER_STRING
#	define APP_COMPILER_STRING "Unknown compiler"
#endif
#	define DEFASSTR(x)	#x

#pragma endregion

#pragma region Global_Data

	static int cursor_x = 0, cursor_y = 0, clBufferAllocLen = APP_BUFFER_SIZE;
	static bool cursor_L_down = false;
	extern bool KLGLDebug;
	extern char *clBuffer;

#pragma endregion

#pragma region Inline_System_Utilitys

	inline char *substr(const char *pstr, int start, int len)
	{
		if (pstr == 0 || strlen(pstr) == 0 || strlen(pstr) < start || strlen(pstr) < (start+len)){
			return const_cast<char*>(pstr);
		}

		static size_t prev_allocsize;
		static char *pnew;
		if (prev_allocsize == NULL){
			prev_allocsize = 0;
		}
		if (pnew == NULL || prev_allocsize < len){
			prev_allocsize = len;
			pnew = new char[len];
		}
		strncpy(pnew, pstr + start, len);
		pnew[len] = '\0';
		return pnew;
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
			fill_n(clBuffer, 4, '\0');
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
		fill_n(tBuffer, clBufferAllocLen, '\0');
		
		int ret = 0, numLn = 0, numLnP = 0;

		// Do normal output
		va_list arg;
		va_start(arg, format);
		vsprintf(tBuffer, format, arg);
		ret = vprintf(format, arg);
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
		numLn = 17;
		while (numLn > 16)
		{
			numLn = 0;
			for (char* c = clBuffer; *c != '\0'; c++)
			{
				if (*c == '\n'){ numLn++;	}
				if (numLn == 0){ numLnP++;	}
			}
			if (numLn > 16){
				memcpy(tBuffer, clBuffer+numLnP+1, clBufferAllocLen);
				memcpy(clBuffer, tBuffer, clBufferAllocLen);
			}
		}

		delete tBuffer;
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
			return std::strcmp(str1, str2) < 0;
		}
	};

#pragma endregion

}