#include "unix.h"

namespace klib {
	int singleBufferAttributess[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_RED_SIZE, 1, /* Request a single buffered color buffer */
		GLX_GREEN_SIZE, 1, /* with the maximum number of color bits */
		GLX_BLUE_SIZE, 1, /* for each component */
		None
	};

	int doubleBufferAttributes[] = {
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DOUBLEBUFFER, True, /* Request a double-buffered color buffer with */
		GLX_RED_SIZE, 1, /* the maximum number of bits per component */
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		None
	};

	X11WM::X11WM(const char* _title, Rect<int> *_window, int _scaleFactor, bool _fullscreen){
		window = _window;
		/* Open a connection to the X server */
		dpy = XOpenDisplay(NULL);
		if(dpy == NULL){
			cl("Error: Unable to open a connection to the X server.\n");
			exit(EXIT_FAILURE);
		}

		/* Request a suitable framebuffer configuration - try for a double
		** buffered configuration first */
		fbConfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), doubleBufferAttributes, &numReturned);

		if(fbConfigs == NULL){ // no double buffered configs available
			fbConfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), singleBufferAttributess, &numReturned);
			swapFlag = False;
		}

		/* Create an X colormap and window with a visual matching the first
		** returned framebuffer config */
		vInfo = glXGetVisualFromFBConfig(dpy, fbConfigs[0]);

		swa.border_pixel = 0;
		swa.event_mask = StructureNotifyMask;
		swa.colormap = XCreateColormap(dpy, RootWindow(dpy, vInfo->screen), vInfo->visual, AllocNone);

		swaMask = CWBorderPixel | CWColormap | CWEventMask;

		xWin = XCreateWindow(dpy, RootWindow(dpy, vInfo->screen), 0, 0, window->width, window->height, 0, vInfo->depth, InputOutput, vInfo->visual, swaMask, &swa);

		/* Create a GLX context for OpenGL rendering */
		context = glXCreateNewContext(dpy, fbConfigs[0], GLX_RGBA_TYPE, NULL, True);

		/* Create a GLX window to associate the frame buffer configuration
		** with the created X window */
		glxWin = glXCreateWindow(dpy, fbConfigs[0], xWin, NULL);

		/* Map the window to the screen, and wait for it to appear */
		XMapWindow(dpy, xWin);
		XIfEvent(dpy, &event, WaitForNotify, (XPointer)xWin);

		/* Bind the GLX context to the Window */
		glXMakeContextCurrent(dpy, glxWin, glxWin, context);
	}

	void X11WM::_swap(){
		glXSwapBuffers(dpy, glxWin);
	}

	int X11WM::_event(){
		int status = 1;
		while (XPending(dpy) > 0)
		{
			XNextEvent(dpy, &event);
			switch (event.type){
			/*case Expose:
				if(event.xexpose.count != 0){
					break;
				}
				renderGL();
				break;*/
			case ConfigureNotify:
				if((event.xconfigure.width != window->width) || (event.xconfigure.height != window->height)){
					resizeEvent = true;
					window->width = event.xconfigure.width;
					window->height = event.xconfigure.height;
					clientResize(window->width, window->height);
				}
				break;
			case ButtonPress:
				break;
			case KeyPress:
				if(XLookupKeysym(&event.xkey, 0) == XK_Escape){
					status = 0;
				}
				break;
			case ClientMessage:
				if(strcmp(XGetAtomName(dpy, event.xclient.message_type), "WM_PROTOCOLS") == 0){
					status = 0;
				}
				break;
			default:
				break;
			}
		}
		if(status != 1){
			return status;
		}else{
			return 1;
		}
	}
	
	void X11WM::clientResize(int nWidth, int nHeight){
		
	}
}
