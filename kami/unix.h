#pragma once

#define GL_GLEXT_PROTOTYPES
#define X11_EVENT_MASK ExposureMask | StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask | KeyPressMask | KeyReleaseMask
#define stricmp strcasecmp

#include <vector>
#include "pure.h"
#include "input.h"

namespace klib_unix { // Precious little bitch why don't you declare everything fucking globally
	#include <GL/glx.h>
}

namespace klib {
	class X11WM {
	public:
		// X11 access
		klib_unix::Display              *dpy;
		klib_unix::Window                xWin;
		klib_unix::XEvent                event;
		klib_unix::XVisualInfo          *vInfo;
		klib_unix::XSetWindowAttributes  swa;
		klib_unix::GLXFBConfig          *fbConfigs;
		klib_unix::GLXContext            context;
		klib_unix::GLXWindow             glxWin;
		int                   swaMask;
		int                   numReturned;
		int                   swapFlag;
		
		Rect<int> *window;
		
		X11WM(const char* _title, Rect<int> *_window, int _scaleFactor, bool _fullscreen, bool _vsync);
		~X11WM();

		void _swap();
		void clientResize(int nWidth, int nHeight);
		int _event(std::vector<KLGLKeyEvent> *keyQueue, int *mouseX, int *mouseY);
		
	private:
		inline static int WaitForNotify( klib_unix::Display *dpy, klib_unix::XEvent *event, klib_unix::XPointer arg ) {
			return (event->type == MapNotify) && (event->xmap.window == (klib_unix::Window) arg);
		}
	};
}
