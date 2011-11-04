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
	extern char clBuffer[];
	extern KLGLMutex clBufferSpinLock;

#pragma endregion

#pragma region Inline_System_Utilitys

	inline int cl(const char* format, ...){
		static FILE *fp = fopen("stdout.txt", "a+");
		if (fp == NULL)
		{
			printf("\nFATAL ERROR: COULD NOT OPEN LOG FILE, CHECK YOUR FILE PERMISSIONS. WILL NOW TERMINATE!\n");
			assert(fp != NULL);
			exit(EXIT_FAILURE);
		}
		static char tBuffer[1024] = {};
		int ret = -1;
		va_list arg;
		va_start (arg, format);
		int numLn = 0, numLnP = 0;
		size_t bufferLen = strlen(clBuffer);

		// Append the new string to the buffer and temporary buffer
		vsprintf(clBuffer+bufferLen, format, arg);
		vsprintf(tBuffer, format, arg);

		// Flush to log file
		fwrite(tBuffer, 1, strlen(tBuffer), fp);
		fflush(fp);

		// Count newlines
		for (char* c = clBuffer; *c != '\0'; c++)
		{
			// Number of newlines before first termination
			if (*c == '\n') numLn++;
			// Char position of the first newline
			if (numLn == 0) numLnP++;
		}

		if (numLn > 16)
		{
			// Remove the first line from our string yo
			sprintf(clBuffer, "%s", clBuffer+numLnP+1);
		}

		// Do normal output
		ret = vprintf (format, arg);
		va_end (arg);

		return ret;
	}

	inline char *substr(const char *pstr, int start, int len)
	{
		if (pstr == 0 || strlen(pstr) == 0 || strlen(pstr) < start || strlen(pstr) < (start+len)){
			return 0;
		}
		static char *pnew = new char[len];
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

}