#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sstream>
#include <string.h>
#include <wchar.h>
#include <unordered_map>
#include <math.h>
#include <future>
#include <regex>

//#define APP_USE_GLEW

#if defined(APP_USE_GLEW)
#include "glew.h"
#else
#include "gl_core_3_3.h"
#endif

namespace klib {

#ifndef APP_MOTD
#define APP_MOTD "(c) 2005-2014 Ameoto Systems Inc. All Rights Reserved.\n\n"
#endif
#define APP_BUFFER_SIZE 4096
#define APP_ENABLE_MIPMAP 0
#define APP_ANISOTROPY 4.0
#define APP_CONSOLE_LINES 32
#define APP_PI 3.14159265359

#define uchar2vec(x) vector<unsigned char>(reinterpret_cast<unsigned char*>(x), reinterpret_cast<unsigned char*>(x)+sizeof(x))

	// Determine compiler
#if		defined __ECC || defined __ICC || defined __INTEL_COMPILER
#	define APP_COMPILER_STRING "Intel C/C++"
#elif	defined __GNUC__
#	if defined __clang__
#		define APP_COMPILER_STRING "LLVM " __clang_version__
#	else
#		define APP_COMPILER_STRING "GCC"
#	endif
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

#if _MSC_VER
#define snprintf _snprintf
#endif

	static int clBufferAllocLen = APP_BUFFER_SIZE;
	static int clBufferLines = APP_CONSOLE_LINES;
	extern bool KLGLDebug;
	extern bool resizeEvent;
	extern char *clBuffer;
	//extern std::unordered_map<std::string, void*> clHashTable;

	class KLGLException {
	private:
		char* msg;
	public:
		KLGLException(const char* m, ...);
		char* getMessage();
	};

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
		ret = printf("%s", tBuffer);
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
		numLn = clBufferLines+1;
		while (numLn > clBufferLines)
		{
			numLn = 0;
			for (char* c = clBuffer; *c != '\0'; c++)
			{
				if (*c == '\n'){ numLn++;	}
				if (numLn == 0){ numLnP++;	}
			}
			if (numLn > clBufferLines){
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

	inline std::string REGEXParseCode(std::regex_constants::error_type etype) {
	    switch (etype) {
	    case std::regex_constants::error_collate:
	        return "error_collate: invalid collating element request";
	    case std::regex_constants::error_ctype:
	        return "error_ctype: invalid character class";
	    case std::regex_constants::error_escape:
	        return "error_escape: invalid escape character or trailing escape";
	    case std::regex_constants::error_backref:
	        return "error_backref: invalid back reference";
	    case std::regex_constants::error_brack:
	        return "error_brack: mismatched bracket([ or ])";
	    case std::regex_constants::error_paren:
	        return "error_paren: mismatched parentheses(( or ))";
	    case std::regex_constants::error_brace:
	        return "error_brace: mismatched brace({ or })";
	    case std::regex_constants::error_badbrace:
	        return "error_badbrace: invalid range inside a { }";
	    case std::regex_constants::error_range:
	        return "erro_range: invalid character range(e.g., [z-a])";
	    case std::regex_constants::error_space:
	        return "error_space: insufficient memory to handle this regular expression";
	    case std::regex_constants::error_badrepeat:
	        return "error_badrepeat: a repetition character (*, ?, +, or {) was not preceded by a valid regular expression";
	    case std::regex_constants::error_complexity:
	        return "error_complexity: the requested match is too complex";
	    case std::regex_constants::error_stack:
	        return "error_stack: insufficient memory to evaluate a match";
	    default:
	        return "";
	    }
	}

#define LOType template <class LOValue>

	// Logical Object template
	LOType class LObject {
	public:
		LOValue x;
		LOValue y;
		LObject(){ moveTo(0, 0); };
		LObject(LOValue newx, LOValue newy){ moveTo(newx, newy); };
		inline LOValue getX(){ return x; };
		inline LOValue getY(){ return y; };
		inline void setX(LOValue newx){ x = newx; };
		inline void setY(LOValue newy){ y = newy; };
		inline void moveTo(LOValue newx, LOValue newy){ setX(newx); setY(newy); };
		inline void rMoveTo(LOValue deltax, LOValue deltay){ moveTo(getX() + deltax, getY() + deltay); };
		inline void moveAngle(LOValue angle, LOValue speed = 1, LOValue time = 1){
			float rad = angle * (APP_PI / 180);
			x += cos(rad) * (time * speed);
			y -= sin(rad) * (time * speed);
		}
	};

	// Point
	LOType class Point: public LObject<LOValue> {
	public:
		Point(): LObject<LOValue>::LObject(){};
		Point(LOValue newx, LOValue newy): LObject<LOValue>::LObject(newx, newy){};
	};

	// Rectangle
	LOType class Rect: public LObject<LOValue> {
	public:
		LOValue width;
		LOValue height;
		Rect(): LObject<LOValue>::LObject(){
			setWidth(0);
			setHeight(0);
		};
		Rect(LOValue newx, LOValue newy, LOValue newwidth, LOValue newheight): LObject<LOValue>::LObject(newx, newy){
			setWidth(newwidth);
			setHeight(newheight);
		};
		inline LOValue getWidth(){
			return width;
		};
		inline LOValue getHeight(){
			return height;
		};
		inline void setWidth(LOValue newwidth){
			width = newwidth;
		};
		inline void setHeight(LOValue newheight){
			height = newheight;
		};
	};

	// Vec4
	LOType class Vec4: public LObject<LOValue> {
	public:
		LOValue x;
		LOValue y;
		LOValue z;
		LOValue w;

		Vec4(): LObject<LOValue>::LObject(){
			x = 0;
			y = 0;
			z = 0;
			w = 0;
		}
		Vec4(LOValue newx, LOValue newy, LOValue newz, LOValue neww): LObject<LOValue>::LObject(newx, newy){
			x = newx;
			y = newy;
			z = newz;
			w = neww;
		}
		inline LOValue* Data(){
			rawTypePtr[0] = x;
			rawTypePtr[1] = y;
			rawTypePtr[2] = z;
			rawTypePtr[3] = w;
			return rawTypePtr;
		}
	private:
		LOValue rawTypePtr[4];
	};

	template<typename T> struct Point2D
	{
		T x, y;
		Point2D(T X, T Y) : x(X), y(Y) {};
	};
	template<typename T> struct Rect2D
	{
		T x, y, z, w;
		Rect2D(T X, T Y, T Z, T W) : x(X), y(Y), z(Z), w(W) {};
	};
}
