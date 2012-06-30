#pragma once

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include <Windows.h>
#include "GL/glew.h"
#include "GL/wglew.h"
#include "pure.h"
#include "logicalObjects.h"

namespace klib {

#define VK_1	0x31
#define VK_2	0x32
#define VK_3	0x33
#define VK_4	0x34
#define VK_5	0x35
#define VK_6	0x36
#define VK_7	0x37
#define VK_8	0x38
#define VK_9	0x39
#define VK_0	0x30
#define VK_A	0x041
#define VK_B	0x042
#define VK_C	0x043
#define VK_D	0x044
#define VK_E	0x045
#define VK_F	0x046
#define VK_G	0x047
#define VK_H	0x048
#define VK_I	0x049
#define VK_J	0x04A
#define VK_K	0x04B
#define VK_L	0x04C
#define VK_M	0x04D
#define VK_N	0x04E
#define VK_O	0x04F
#define VK_P	0x050
#define VK_Q	0x051
#define VK_R    0x052
#define VK_S	0x053
#define VK_T	0x054
#define VK_U	0x055
#define VK_V	0x056
#define VK_W	0x057
#define VK_X	0x058
#define VK_Y	0x059
#define VK_Z	0x05A

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	inline void windowResize(HWND hWnd){
		RECT windowRect;
		GetWindowRect(hWnd, &windowRect);
		
		int width = windowRect.left-windowRect.right;
		int height = windowRect.top-windowRect.bottom;

		if (height <= 0)
			height = 1;

		int aspectratio = width / height;
		glViewport(0, 0, width, height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, aspectratio, 0.2f, 255.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	inline void virtualPageCommit(SYSTEM_INFO sysinfo){
		LPVOID lpvAddr = VirtualAlloc(NULL, sysinfo.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READONLY | PAGE_GUARD);
		if(lpvAddr == NULL) {
			cl("VirtualAlloc failed. Error: %ld\n", GetLastError());
		}else{
			while(VirtualLock(lpvAddr, sysinfo.dwPageSize)){
				cl("Cannot lock at 0x%lp, error = %lx\n", lpvAddr, GetLastError());
			}
			cl("Committed %lu bytes at address 0x%lp\n", sysinfo.dwPageSize, lpvAddr);
		}
	}

	inline void SetVSync(bool sync){
		typedef bool(APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
		PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if(wglSwapIntervalEXT ){
			wglSwapIntervalEXT(sync);
		}
	}

	inline char vktochar(WORD vk)
	{
		if(vk == VK_SPACE)
		{ // Space character.
			return ' ';
		}

		bool shift = (GetAsyncKeyState(VK_SHIFT));

		switch(vk)
		{
			// Non-shifted input
		case VK_ADD:		return '+';
		case VK_SUBTRACT:	return '-';
		case VK_DIVIDE:		return '/';
		case VK_MULTIPLY:	return '*';
			// Modifiable
		case VK_0:			return shift ? ')' : VK_0;
		case VK_1:			return shift ? '!' : VK_1;
		case VK_2: 			return shift ? '@' : VK_2;
		case VK_3: 			return shift ? '#' : VK_3;
		case VK_4: 			return shift ? '$' : VK_4;
		case VK_5: 			return shift ? '%' : VK_5;
		case VK_6: 			return shift ? '^' : VK_6;
		case VK_7:			return shift ? '&' : VK_7;
		case VK_8: 			return shift ? '*' : VK_8;
		case VK_9: 			return shift ? '(' : VK_9;
		case VK_OEM_1:      return shift ? ':' : ';';
		case VK_OEM_PLUS:   return shift ? '+' : '=';
		case VK_OEM_COMMA:  return shift ? '<' : ',';
		case VK_OEM_MINUS:  return shift ? '_' : '-';
		case VK_OEM_PERIOD: return shift ? '>' : '.';
		case VK_OEM_2:      return shift ? '?' : '/';
		case VK_OEM_3:      return shift ? '~' : '`';
		case VK_OEM_4:      return shift ? '{' : '[';
		case VK_OEM_5:      return shift ? '|' : '\\';
		case VK_OEM_6:      return shift ? '}' : ']';
		case VK_OEM_7:      return shift ? '"' : '\'';
		}

		char ascii = MapVirtualKey(vk, MAPVK_VK_TO_CHAR);
		if(ascii >= 'A' && ascii <= 'Z')
		{ // Simple conversion.
			return shift ? ascii : tolower(ascii);
		}

		if (ascii >= '0' && ascii <= '9')
		{
			return ascii;
		}

		return '?';
	}
	
	class Win32WM {
	public:
		// Win32 access
		WNDCLASS wc;
		HWND hWnd;
		HDC hDC;
		HGLRC hRC,hRCAUX;
		MSG msg;
		
		Win32WM(const char* title, Rect<int> window, int scaleFactor, bool fullscreen);
		~Win32WM();

		void _swap();
		void clientResize(int nWidth, int nHeight);
	};

	class Win32Resource {
	public:
		Win32Resource(){};
		Win32Resource(char* strCustomResName, int nResourceId){
			ExtractBinResource(strCustomResName, nResourceId);
		};
		void ExtractBinResource(char* strCustomResName, int nResourceId);
		void* pointer;
		DWORD size;
	};
}
