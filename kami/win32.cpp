#include "win32.h"

namespace klib {
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		HKEY hKey;
		WINDOWPLACEMENT pos;
		LONG res;
		switch (message){
		case WM_CREATE:
			res = RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Ameoto\\Neo"), &hKey);

			if(res == ERROR_SUCCESS){
				pos.length = sizeof(WINDOWPLACEMENT);
				res = RegQueryValueEx(hKey, TEXT("windowPlacement"), NULL, NULL, (LPBYTE)&pos, NULL);
				SetWindowPlacement(hWnd, &pos);
				RegCloseKey(hKey);
			}
			break;
		case WM_CLOSE:
		case WM_DESTROY:
			pos.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hWnd, &pos);
			RegCreateKey(HKEY_CURRENT_USER, TEXT("Software\\Ameoto\\Neo"), &hKey);
			RegSetValueEx(hKey, TEXT("windowPlacement"), NULL, REG_BINARY, (BYTE*)&pos, sizeof(WINDOWPLACEMENT));
			RegCloseKey(hKey);
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

	Win32WM::Win32WM(const char* title, Rect<int> window, int scaleFactor, bool fullscreen){
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
			dmScreenSettings.dmPelsWidth	= window.width*scaleFactor;
			dmScreenSettings.dmPelsHeight	= window.height*scaleFactor;
			dmScreenSettings.dmBitsPerPel	= 32;
			dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

			if(ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL){
				cl("The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Will Use Windowed Mode Instead.\n\n");
				fullscreen = false;
			}else{
				hWnd = CreateWindowEx(WS_EX_APPWINDOW, "KamiGLWnd32", title, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, NULL, window.width*scaleFactor, window.height*scaleFactor, NULL, NULL, wc.hInstance, NULL );
			}
		}

		if(!fullscreen){
			hWnd = CreateWindowEx(WS_EX_APPWINDOW, "KamiGLWnd32", title, WS_CAPTION | WS_OVERLAPPEDWINDOW, window.x, window.y, window.width*scaleFactor, window.height*scaleFactor, NULL, NULL, wc.hInstance, NULL );
			clientResize(window.width*scaleFactor, window.height*scaleFactor);
		}

		SetWindowText(hWnd, "Loading...");

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
		hRCAUX = wglCreateContext(hDC);
		if(wglShareLists(hRC, hRCAUX) == FALSE){
			DWORD errorCode=GetLastError();
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &lpMsgBuf, 0, NULL);
			MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
			//throw KLGLException((LPCTSTR)lpMsgBuf);
			LocalFree(lpMsgBuf);
			//Destroy the GL context and just use 1 GL context
			wglDeleteContext(hRCAUX);
		}
		wglMakeCurrent(hDC, hRC);

		ShowWindow(hWnd,SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);

		glewExperimental = GL_TRUE;
		GLenum glewIinitHandle = glewInit();
		if(glewIinitHandle != GLEW_OK)
		{
			cl("Error: %s\n", glewGetErrorString(glewIinitHandle));
			exit(EXIT_FAILURE);
		}
		glewIinitHandle = NULL;
	}

	void Win32WM::_swap(){
		SwapBuffers(hDC);
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
}