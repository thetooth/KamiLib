#pragma once

#define GL_GLEXT_PROTOTYPES
#define stricmp strcasecmp

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include "pure.h"
#include "logicalObjects.h"

namespace klib {
	class X11WM {
	public:
		// X11 access
		Display              *dpy;
		Window                xWin;
		XEvent                event;
		XVisualInfo          *vInfo;
		XSetWindowAttributes  swa;
		GLXFBConfig          *fbConfigs;
		GLXContext            context;
		GLXWindow             glxWin;
		int                   swaMask;
		int                   numReturned;
		int                   swapFlag;
		
		Rect<int> *window;
		
		X11WM(const char* _title, Rect<int> *_window, int _scaleFactor, bool _fullscreen);
		~X11WM();

		void _swap();
		void clientResize(int nWidth, int nHeight);
		int _event();
		
	private:
		inline static int WaitForNotify( Display *dpy, XEvent *event, XPointer arg ) {
			return (event->type == MapNotify) && (event->xmap.window == (Window) arg);
		}
	};
}
