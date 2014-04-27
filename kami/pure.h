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
#include <locale>
#include <codecvt>
#include <fstream>

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
#define APP_CONSOLE_LINES 16
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

	class Console;

	static int clBufferAllocLen = APP_BUFFER_SIZE;
	static int clBufferLines = APP_CONSOLE_LINES;
	extern bool KLGLDebug;
	extern bool resizeEvent;
	extern std::shared_ptr<Console> clConsole;

	class KLGLException {
	private:
		char* msg;
	public:
		KLGLException(const char* m, ...);
		char* getMessage();
	};

	template <class T> class RingBuffer {
	public:
		std::vector<T> ring;
		const size_t   bufferSize;
		std::atomic<uint64_t> head;
		std::atomic<uint64_t> tail;

		RingBuffer(size_t buffer_size) : ring(buffer_size), bufferSize(buffer_size), head(0), tail(0){ };

		T* back(){
			bool received = false;

			if (available(head, tail)){
				return &(ring[head % bufferSize]);
			}

			return nullptr;
		}

		void push(){
			++head;
		}

		T* front(){
			if (tail < head){
				return &ring[tail % bufferSize];
			}

			return nullptr;
		}

		void pop(){
			++tail;
		}

		size_t size() const {
			if (tail < head){
				return bufferSize - ((tail + bufferSize) - head);
			}
			else if (tail > head){
				return bufferSize - (tail - head);
			}

			return 0;
		}

		bool available(){
			return available(head, tail);
		}

		bool available(uint64_t h, uint64_t t) const {
			if (h == t){
				return true;
			}
			else if (t > h){
				return (t - h) > bufferSize;
			}
			else/* if(h > t)*/{
				return (t + bufferSize) - h > 0;
			}
		}
	};

	class Console {
	public:
		std::wofstream file;
		std::unique_ptr<RingBuffer<std::wstring>> buffer;
		bool following;

		Console(){
			file.open("cl.log", std::ios::binary);
			if (file.bad()){
				exit(EXIT_FAILURE);
			}
			buffer = std::make_unique<RingBuffer<std::wstring>>(APP_CONSOLE_LINES);
			following = false;
		};
		~Console() = default;

		void push_back(std::wstring str){
			file << str;
			if (buffer->back() == nullptr){	// If we exceed the buffer boundaries discard oldest entry
				buffer->pop();
			}

			auto val = buffer->back();
			
			if (str.back() == L'\n'){		// Check if return
				if (following){				// Append if we're following a non returning line
					*val += std::move(str);
				}else{						// Otherwise its a new entry
					*val = std::move(str);
				}
				following = false;
				buffer->push();
			}else{							// Non-returning line, do not increment pointer
				if (following){				// Append if we're following a non returning line
					*val += std::move(str);
				}else{						// Otherwise its a new entry
					*val = std::move(str);
				}
				following = true;
			}
		}

		void str(std::wstring &tmpstr){
			for (int i = 0; i < buffer->size(); i++){
				tmpstr += buffer->ring[(buffer->tail + i) % buffer->bufferSize];
			}
		}
	};

	inline int cl(const char* format, ...){
		// Temporary buffer
		char tBuffer[4096];

		// Do normal output
		va_list arg;
		va_start(arg, format);
		vsprintf_s(tBuffer, 4096, format, arg);
		int ret = printf("%s", tBuffer);
		va_end(arg);

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(tBuffer);
		clConsole->push_back(wide);

		return ret;
	}

	inline int cl(const wchar_t* format, ...){
		// Temporary buffer
		wchar_t tBuffer[4096];

		// Do normal output
		va_list arg;
		va_start(arg, format);
		vswprintf_s(tBuffer, 4096, format, arg);
		int ret = wprintf(L"%s", tBuffer);
		va_end(arg);

		clConsole->push_back(tBuffer);

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
