#include "win32.h"

namespace klib {
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message){
		case WM_CREATE:
			break;
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
		/*case WM_SIZE:
			//windowResize(hWnd);
			break;
*/
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
		}
		return 0;
	}

	void KLGL::ProcessEvent(int *status){
		if(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)){
			if(msg.message == WM_QUIT){
				PostQuitMessage(0);
				*status = 0;
			}else{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
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