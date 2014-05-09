#include "win32.h"

namespace klib {
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message){
		case WM_QUIT:
		case WM_CLOSE:
		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;
		case WM_KEYDOWN:
			switch (wParam){
			case VK_ESCAPE:
				PostQuitMessage(0);
			}
			break;
		case WM_SIZE:
			resizeEvent = true;
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
		}
		return 0;
	}

	Win32WM::Win32WM(const char* title, Rect<int> *window, int scaleFactor, bool fullscreen, bool vsync){
		// Register win32 class
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
		wc.lpszMenuName = NULL;
		wc.lpszClassName = "KamiGLWnd32";
		RegisterClass(&wc);

		// create main window
		if(fullscreen){
			// Device Mode
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
			dmScreenSettings.dmSize=sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth	= window->width*scaleFactor;
			dmScreenSettings.dmPelsHeight	= window->height*scaleFactor;
			dmScreenSettings.dmBitsPerPel	= 32;
			dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

			if(ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL){
				cl("The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Will Use Windowed Mode Instead.\n\n");
				fullscreen = false;
			}else{
				hWnd = CreateWindowEx(WS_EX_APPWINDOW, "KamiGLWnd32", title, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, NULL, window->width*scaleFactor, window->height*scaleFactor, NULL, NULL, wc.hInstance, NULL );
			}
		}

		if(!fullscreen){
			hWnd = CreateWindowEx(WS_EX_APPWINDOW, "KamiGLWnd32", title, WS_CAPTION | WS_OVERLAPPEDWINDOW, window->x, window->y, window->width*scaleFactor, window->height*scaleFactor, NULL, NULL, wc.hInstance, NULL );
			clientResize(window->width*scaleFactor, window->height*scaleFactor);
		}

		SetWindowText(hWnd, title);

		PIXELFORMATDESCRIPTOR pfd;
		int format;

		// get the device context (DC)
		hDC = GetDC( hWnd );

		// set the pixel format for the DC
		ZeroMemory( &pfd, sizeof( pfd ) );
		pfd.nSize = sizeof( pfd );
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 16;
		pfd.iLayerType = PFD_MAIN_PLANE;
		format = ChoosePixelFormat( hDC, &pfd );
		SetPixelFormat( hDC, format, &pfd );

		// create and enable the render context (RC)
		hRC = wglCreateContext(hDC);
		wglMakeCurrent(hDC, hRC);

		ShowWindow(hWnd,SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);
		SetVSync(vsync);

		// Initialize reference table for key codes
		initKeyTable();
	}

	void Win32WM::_swap(){
		SwapBuffers(hDC);
	}

	int Win32WM::_event(std::vector<KLGLKeyEvent> *keyQueue, int *mouseX, int *mouseY){
		int status = 1;
		if(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)){
			switch (msg.message){
			case WM_QUIT:
			case WM_CLOSE:
			case WM_DESTROY:
				PostQuitMessage(0);
				status = 0;
				break;
			case WM_KEYDOWN:
			case WM_KEYUP:
				(*keyQueue).push_back(KLGLKeyEvent(
					KLGLKeyEvent::translateNativeKeyCode(msg.wParam), 
					vktochar(msg.wParam), 
					(msg.wParam == VK_SHIFT ? KLGLKeyEvent::SHIFT_DOWN : 0), 
					msg.wParam, 
					(msg.message == WM_KEYDOWN ? true : false)
					));
				break;
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				unsigned int key;
				switch (msg.wParam){
				case MK_LBUTTON:
					key = KLGLKeyEvent::KEY_MLBUTTON;
					break;
				case MK_RBUTTON:
					key = KLGLKeyEvent::KEY_MRBUTTON;
					break;
				case MK_MBUTTON:
					key = KLGLKeyEvent::KEY_MMBUTTON;
					break;
				default:
					key = KLGLKeyEvent::KEY_MLBUTTON; // ! WIN32 - WM_LBUTTONUP does not contain a KEY_MLBUTTON, fucking retards
				}
				{
					bool stroke = false;
					if (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN || msg.message == WM_MBUTTONDOWN){
						stroke = true;
					}
					(*keyQueue).push_back(KLGLKeyEvent(
						key,
						0,
						0,
						msg.wParam,
						stroke
						));
				}
				break;
			case WM_MOUSEMOVE:
				POINT p;
				GetCursorPos(&p);
				ScreenToClient(hWnd, &p);
				*mouseX = p.x;
				*mouseY = p.y;
				break;
			default:
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return status;
	}

	void Win32WM::clientResize(int nWidth, int nHeight){
		RECT rcClient, rcWind;
		POINT ptDiff;
		GetClientRect(hWnd, &rcClient);
		GetWindowRect(hWnd, &rcWind);
		ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
		ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;
		MoveWindow(hWnd,rcWind.left, rcWind.top, nWidth + ptDiff.x, nHeight + ptDiff.y, TRUE);
	}

	Win32WM::~Win32WM(){
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hRC);
		ReleaseDC(hWnd, hDC);
		DeleteDC(hDC);

		// destroy the window explicitly
		DestroyWindow(hWnd);
	}

	void Win32Resource::ExtractBinResource(char* strCustomResName, int nResourceId){
		HGLOBAL hResourceLoaded;		// handle to loaded resource 
		HRSRC hRes;						// handle/ptr. to res. info. 
		// find location of the resource and get handle to it
		hRes = FindResource(NULL, MAKEINTRESOURCE(nResourceId), strCustomResName);
		// loads the specified resource into global memory. 
		hResourceLoaded = LoadResource(NULL, hRes); 
		// get a pointer to the loaded resource!
		pointer = (void*)LockResource(hResourceLoaded); 
		if (pointer == NULL)
		{
			cl("[!] Could not acquire Win32 resource: %s:%d\n", strCustomResName, nResourceId);
		}
		// determine the size of the resource, so we know how much to write out to file!  
		size = SizeofResource(NULL, hRes);
	}

	unsigned int sKeyTable[MAX_KEYCODE];

	// Much of this keyTable is courtesy of SDL's keyboard handling code
	static void initKeyTable(){
		for( int c = 0; c < MAX_KEYCODE; ++c ){
			sKeyTable[c] = KLGLKeyEvent::KEY_UNKNOWN;
		}

		sKeyTable[VK_BACK] = KLGLKeyEvent::KEY_BACKSPACE;
		sKeyTable[VK_TAB] = KLGLKeyEvent::KEY_TAB;
		sKeyTable[VK_CLEAR] = KLGLKeyEvent::KEY_CLEAR;
		sKeyTable[VK_RETURN] = KLGLKeyEvent::KEY_RETURN;
		sKeyTable[VK_PAUSE] = KLGLKeyEvent::KEY_PAUSE;
		sKeyTable[VK_ESCAPE] = KLGLKeyEvent::KEY_ESCAPE;
		sKeyTable[VK_SPACE] = KLGLKeyEvent::KEY_SPACE;
		sKeyTable[0xDE] = KLGLKeyEvent::KEY_QUOTE;
		sKeyTable[0xBC] = KLGLKeyEvent::KEY_COMMA;
		sKeyTable[0xDD] = KLGLKeyEvent::KEY_MINUS;
		sKeyTable[0xBE] = KLGLKeyEvent::KEY_PERIOD;
		sKeyTable[0xBF] = KLGLKeyEvent::KEY_SLASH;
		sKeyTable['0'] = KLGLKeyEvent::KEY_0;
		sKeyTable['1'] = KLGLKeyEvent::KEY_1;
		sKeyTable['2'] = KLGLKeyEvent::KEY_2;
		sKeyTable['3'] = KLGLKeyEvent::KEY_3;
		sKeyTable['4'] = KLGLKeyEvent::KEY_4;
		sKeyTable['5'] = KLGLKeyEvent::KEY_5;
		sKeyTable['6'] = KLGLKeyEvent::KEY_6;
		sKeyTable['7'] = KLGLKeyEvent::KEY_7;
		sKeyTable['8'] = KLGLKeyEvent::KEY_8;
		sKeyTable['9'] = KLGLKeyEvent::KEY_9;
		sKeyTable[0xBA] = KLGLKeyEvent::KEY_SEMICOLON;
		sKeyTable[0xBB] = KLGLKeyEvent::KEY_EQUALS;
		sKeyTable[0xDB] = KLGLKeyEvent::KEY_LEFTBRACKET;
		sKeyTable[0xDC] = KLGLKeyEvent::KEY_BACKSLASH;
		sKeyTable[VK_OEM_102] = KLGLKeyEvent::KEY_LESS;
		sKeyTable[0xDD] = KLGLKeyEvent::KEY_RIGHTBRACKET;
		sKeyTable[0xC0] = KLGLKeyEvent::KEY_BACKQUOTE;
		sKeyTable[0xDF] = KLGLKeyEvent::KEY_BACKQUOTE;
		sKeyTable['A'] = KLGLKeyEvent::KEY_a;
		sKeyTable['B'] = KLGLKeyEvent::KEY_b;
		sKeyTable['C'] = KLGLKeyEvent::KEY_c;
		sKeyTable['D'] = KLGLKeyEvent::KEY_d;
		sKeyTable['E'] = KLGLKeyEvent::KEY_e;
		sKeyTable['F'] = KLGLKeyEvent::KEY_f;
		sKeyTable['G'] = KLGLKeyEvent::KEY_g;
		sKeyTable['H'] = KLGLKeyEvent::KEY_h;
		sKeyTable['I'] = KLGLKeyEvent::KEY_i;
		sKeyTable['J'] = KLGLKeyEvent::KEY_j;
		sKeyTable['K'] = KLGLKeyEvent::KEY_k;
		sKeyTable['L'] = KLGLKeyEvent::KEY_l;
		sKeyTable['M'] = KLGLKeyEvent::KEY_m;
		sKeyTable['N'] = KLGLKeyEvent::KEY_n;
		sKeyTable['O'] = KLGLKeyEvent::KEY_o;
		sKeyTable['P'] = KLGLKeyEvent::KEY_p;
		sKeyTable['Q'] = KLGLKeyEvent::KEY_q;
		sKeyTable['R'] = KLGLKeyEvent::KEY_r;
		sKeyTable['S'] = KLGLKeyEvent::KEY_s;
		sKeyTable['T'] = KLGLKeyEvent::KEY_t;
		sKeyTable['U'] = KLGLKeyEvent::KEY_u;
		sKeyTable['V'] = KLGLKeyEvent::KEY_v;
		sKeyTable['W'] = KLGLKeyEvent::KEY_w;
		sKeyTable['X'] = KLGLKeyEvent::KEY_x;
		sKeyTable['Y'] = KLGLKeyEvent::KEY_y;
		sKeyTable['Z'] = KLGLKeyEvent::KEY_z;
		sKeyTable[VK_DELETE] = KLGLKeyEvent::KEY_DELETE;

		sKeyTable[VK_NUMPAD0] = KLGLKeyEvent::KEY_KP0;
		sKeyTable[VK_NUMPAD1] = KLGLKeyEvent::KEY_KP1;
		sKeyTable[VK_NUMPAD2] = KLGLKeyEvent::KEY_KP2;
		sKeyTable[VK_NUMPAD3] = KLGLKeyEvent::KEY_KP3;
		sKeyTable[VK_NUMPAD4] = KLGLKeyEvent::KEY_KP4;
		sKeyTable[VK_NUMPAD5] = KLGLKeyEvent::KEY_KP5;
		sKeyTable[VK_NUMPAD6] = KLGLKeyEvent::KEY_KP6;
		sKeyTable[VK_NUMPAD7] = KLGLKeyEvent::KEY_KP7;
		sKeyTable[VK_NUMPAD8] = KLGLKeyEvent::KEY_KP8;
		sKeyTable[VK_NUMPAD9] = KLGLKeyEvent::KEY_KP9;
		sKeyTable[VK_DECIMAL] = KLGLKeyEvent::KEY_KP_PERIOD;
		sKeyTable[VK_DIVIDE] = KLGLKeyEvent::KEY_KP_DIVIDE;
		sKeyTable[VK_MULTIPLY] = KLGLKeyEvent::KEY_KP_MULTIPLY;
		sKeyTable[VK_SUBTRACT] = KLGLKeyEvent::KEY_KP_MINUS;
		sKeyTable[VK_ADD] = KLGLKeyEvent::KEY_KP_PLUS;

		sKeyTable[VK_UP] = KLGLKeyEvent::KEY_UP;
		sKeyTable[VK_DOWN] = KLGLKeyEvent::KEY_DOWN;
		sKeyTable[VK_RIGHT] = KLGLKeyEvent::KEY_RIGHT;
		sKeyTable[VK_LEFT] = KLGLKeyEvent::KEY_LEFT;
		sKeyTable[VK_INSERT] = KLGLKeyEvent::KEY_INSERT;
		sKeyTable[VK_HOME] = KLGLKeyEvent::KEY_HOME;
		sKeyTable[VK_END] = KLGLKeyEvent::KEY_END;
		sKeyTable[VK_PRIOR] = KLGLKeyEvent::KEY_PAGEUP;
		sKeyTable[VK_NEXT] = KLGLKeyEvent::KEY_PAGEDOWN;

		sKeyTable[VK_F1] = KLGLKeyEvent::KEY_F1;
		sKeyTable[VK_F2] = KLGLKeyEvent::KEY_F2;
		sKeyTable[VK_F3] = KLGLKeyEvent::KEY_F3;
		sKeyTable[VK_F4] = KLGLKeyEvent::KEY_F4;
		sKeyTable[VK_F5] = KLGLKeyEvent::KEY_F5;
		sKeyTable[VK_F6] = KLGLKeyEvent::KEY_F6;
		sKeyTable[VK_F7] = KLGLKeyEvent::KEY_F7;
		sKeyTable[VK_F8] = KLGLKeyEvent::KEY_F8;
		sKeyTable[VK_F9] = KLGLKeyEvent::KEY_F9;
		sKeyTable[VK_F10] = KLGLKeyEvent::KEY_F10;
		sKeyTable[VK_F11] = KLGLKeyEvent::KEY_F11;
		sKeyTable[VK_F12] = KLGLKeyEvent::KEY_F12;
		sKeyTable[VK_F13] = KLGLKeyEvent::KEY_F13;
		sKeyTable[VK_F14] = KLGLKeyEvent::KEY_F14;
		sKeyTable[VK_F15] = KLGLKeyEvent::KEY_F15;

		sKeyTable[VK_NUMLOCK] = KLGLKeyEvent::KEY_NUMLOCK;
		sKeyTable[VK_CAPITAL] = KLGLKeyEvent::KEY_CAPSLOCK;
		sKeyTable[VK_SCROLL] = KLGLKeyEvent::KEY_SCROLLOCK;
		sKeyTable[VK_RSHIFT] = KLGLKeyEvent::KEY_RSHIFT;
		sKeyTable[VK_LSHIFT] = KLGLKeyEvent::KEY_LSHIFT;
		sKeyTable[VK_RCONTROL] = KLGLKeyEvent::KEY_RCTRL;
		sKeyTable[VK_LCONTROL] = KLGLKeyEvent::KEY_LCTRL;
		sKeyTable[VK_RMENU] = KLGLKeyEvent::KEY_RALT;
		sKeyTable[VK_LMENU] = KLGLKeyEvent::KEY_LALT;
		sKeyTable[VK_RWIN] = KLGLKeyEvent::KEY_RSUPER;
		sKeyTable[VK_LWIN] = KLGLKeyEvent::KEY_LSUPER;

		sKeyTable[VK_HELP] = KLGLKeyEvent::KEY_HELP;
		sKeyTable[VK_PRINT] = KLGLKeyEvent::KEY_PRINT;
		sKeyTable[VK_SNAPSHOT] = KLGLKeyEvent::KEY_PRINT;
		sKeyTable[VK_CANCEL] = KLGLKeyEvent::KEY_BREAK;
		sKeyTable[VK_APPS] = KLGLKeyEvent::KEY_MENU;

		sTableInited = true;
	}
}