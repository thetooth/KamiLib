/*
	PURE.H - Cross-platform utility header.
	Copyright (C) 2005-2011, Ameoto Systems Inc. All rights reserved.
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "threads.h"

namespace klib {

#pragma region Global_Data

	static int cursor_x = 0, cursor_y = 0;
	static bool cursor_L_down = false;
	extern bool KLGLDebug;
	extern char *clBuffer;

#pragma endregion

#pragma region Inline_System_Utilitys

#define DEFASSTR(x)	#x
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
		
		// Temporary buffer
		char *tBuffer = new char[1024];
		
		int ret = 0, numLn = 0, numLnP = 0;
		size_t bufferLen = strlen(clBuffer);

		va_list arg;
		va_start(arg, format);

		// Append the new string to the buffer
		vsprintf(tBuffer, format, arg);
		strcpy_s(clBuffer+bufferLen, 1024, tBuffer);

		// Do normal output
		ret = vprintf(format, arg);
		va_end(arg);

		// Flush to log file
		if (fp != NULL){
			fwrite(tBuffer, 1, strlen(tBuffer), fp);
			fflush(fp);
		}

		// Count newlines
		for (char* c = clBuffer; *c != '\0'; c++){
			// Number of newlines before first termination
			if (*c == '\n') numLn++;
			// Char position of the first newline
			if (numLn == 0) numLnP++;
		}

		// Remove first line if we have to many lines.
		if (numLn > 16){
			strcpy_s(tBuffer, 1024, clBuffer+numLnP+1);
			strcpy_s(clBuffer, 1024, tBuffer);
		}

		delete tBuffer;
		
		return ret;
	}

	inline char *substr(const char *pstr, int start, int len)
	{
		if (pstr == 0 || strlen(pstr) == 0 || strlen(pstr) < start || strlen(pstr) < (start+len)){
			return 0;
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

	inline float ubtof(int color){
		return (float)color/255.0f;
	}

	inline char *file_contents(char *filename)
	{
		FILE *f = fopen(filename, "rb");
		char *buffer;

		if (!f) {
			cl("Unable to open %s for reading\n", filename);
			return NULL;
		}

		fseek(f, 0, SEEK_END);
		int length = ftell(f);
		fseek(f, 0, SEEK_SET);

		buffer = (char*)malloc(length+1);
		fread(buffer, 1, length, f);
		fclose(f);
		buffer[length] = '\0';

		return buffer;
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