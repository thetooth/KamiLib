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

		swa.border_pixel = 10;
		swa.event_mask = StructureNotifyMask;
		swa.colormap = XCreateColormap(dpy, RootWindow(dpy, vInfo->screen), vInfo->visual, AllocNone);

		swaMask = CWBorderPixel | CWColormap | CWEventMask;

		xWin = XCreateWindow(dpy, RootWindow(dpy, vInfo->screen), 0, 0, window->width, window->height, 0, vInfo->depth, InputOutput, vInfo->visual, swaMask, &swa);

		Atom wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(dpy, xWin, &wmDeleteMessage, 1);

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

		/* Setup the input event mask so we capture key presses */
		XSelectInput(dpy, xWin, X11_EVENT_MASK);
		initKeyTable();
	}

	void X11WM::_swap(){
		glXSwapBuffers(dpy, glxWin);
	}

	int X11WM::_event(std::vector<KLGLKeyEvent> *keyQueue){
		int status = 1;
		while (XPending(dpy) > 0)
		{
			XNextEvent(dpy, &event);
			KeySym nativeKeyCode;
			char keyCode;
			Atom wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
			switch (event.type){
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
			case KeyRelease:
				nativeKeyCode = XLookupKeysym(&event.xkey, event.xkey.state & ShiftMask);
				if (nativeKeyCode < 32 || nativeKeyCode > 255){
					keyCode = '\0';
				}else{
					keyCode = nativeKeyCode;
				}
				(*keyQueue).push_back(
					KLGLKeyEvent(
						KLGLKeyEvent::translateNativeKeyCode(nativeKeyCode), 
						keyCode, 
						event.xkey.state, 
						nativeKeyCode, 
						(event.type == KeyPress ? 1 : 0)
					)
				);
				break;
			case ClientMessage:
				if(event.xclient.data.l[0] == wmDeleteMessage){
					XCloseDisplay(dpy);
					return 0;
				}
				break;
			default:
				break;
			}
		}
		return status;
	}
	
	void X11WM::clientResize(int nWidth, int nHeight){
		
	}

	unsigned int sKeyTable[MAX_KEYCODE];

	// Much of this keyTable is courtesy of SDL's keyboard handling code
	static void initKeyTable()
	{
		for( int c = 0; c < MAX_KEYCODE; ++c ){
			sKeyTable[c] = KLGLKeyEvent::KEY_UNKNOWN;
		}

		sKeyTable[XK_BackSpace] = KLGLKeyEvent::KEY_BACKSPACE;
		sKeyTable[XK_Tab] = KLGLKeyEvent::KEY_TAB;
		sKeyTable[XK_Clear] = KLGLKeyEvent::KEY_CLEAR;
		sKeyTable[XK_Return] = KLGLKeyEvent::KEY_RETURN;
		sKeyTable[XK_Pause] = KLGLKeyEvent::KEY_PAUSE;
		sKeyTable[XK_Escape] = KLGLKeyEvent::KEY_ESCAPE;
		sKeyTable[' '] = KLGLKeyEvent::KEY_SPACE;
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
		sKeyTable[XK_semicolon] = KLGLKeyEvent::KEY_SEMICOLON;
		sKeyTable[XK_equal] = KLGLKeyEvent::KEY_EQUALS;
		sKeyTable[XK_bracketleft] = KLGLKeyEvent::KEY_LEFTBRACKET;
		sKeyTable[XK_backslash] = KLGLKeyEvent::KEY_BACKSLASH;
		sKeyTable[XK_less] = KLGLKeyEvent::KEY_LESS;
		sKeyTable[XK_bracketright] = KLGLKeyEvent::KEY_RIGHTBRACKET;
		sKeyTable[0xC0] = KLGLKeyEvent::KEY_BACKQUOTE;
		sKeyTable[0xDF] = KLGLKeyEvent::KEY_BACKQUOTE;
		sKeyTable['a'] = KLGLKeyEvent::KEY_a;
		sKeyTable['b'] = KLGLKeyEvent::KEY_b;
		sKeyTable['c'] = KLGLKeyEvent::KEY_c;
		sKeyTable['d'] = KLGLKeyEvent::KEY_d;
		sKeyTable['e'] = KLGLKeyEvent::KEY_e;
		sKeyTable['f'] = KLGLKeyEvent::KEY_f;
		sKeyTable['g'] = KLGLKeyEvent::KEY_g;
		sKeyTable['h'] = KLGLKeyEvent::KEY_h;
		sKeyTable['i'] = KLGLKeyEvent::KEY_i;
		sKeyTable['j'] = KLGLKeyEvent::KEY_j;
		sKeyTable['k'] = KLGLKeyEvent::KEY_k;
		sKeyTable['l'] = KLGLKeyEvent::KEY_l;
		sKeyTable['m'] = KLGLKeyEvent::KEY_m;
		sKeyTable['n'] = KLGLKeyEvent::KEY_n;
		sKeyTable['o'] = KLGLKeyEvent::KEY_o;
		sKeyTable['p'] = KLGLKeyEvent::KEY_p;
		sKeyTable['q'] = KLGLKeyEvent::KEY_q;
		sKeyTable['r'] = KLGLKeyEvent::KEY_r;
		sKeyTable['s'] = KLGLKeyEvent::KEY_s;
		sKeyTable['t'] = KLGLKeyEvent::KEY_t;
		sKeyTable['u'] = KLGLKeyEvent::KEY_u;
		sKeyTable['v'] = KLGLKeyEvent::KEY_v;
		sKeyTable['w'] = KLGLKeyEvent::KEY_w;
		sKeyTable['x'] = KLGLKeyEvent::KEY_x;
		sKeyTable['y'] = KLGLKeyEvent::KEY_y;
		sKeyTable['z'] = KLGLKeyEvent::KEY_z;
		sKeyTable[XK_Delete] = KLGLKeyEvent::KEY_DELETE;

		sKeyTable[XK_KP_0] = KLGLKeyEvent::KEY_KP0;
		sKeyTable[XK_KP_1] = KLGLKeyEvent::KEY_KP1;
		sKeyTable[XK_KP_2] = KLGLKeyEvent::KEY_KP2;
		sKeyTable[XK_KP_3] = KLGLKeyEvent::KEY_KP3;
		sKeyTable[XK_KP_4] = KLGLKeyEvent::KEY_KP4;
		sKeyTable[XK_KP_5] = KLGLKeyEvent::KEY_KP5;
		sKeyTable[XK_KP_6] = KLGLKeyEvent::KEY_KP6;
		sKeyTable[XK_KP_7] = KLGLKeyEvent::KEY_KP7;
		sKeyTable[XK_KP_8] = KLGLKeyEvent::KEY_KP8;
		sKeyTable[XK_KP_9] = KLGLKeyEvent::KEY_KP9;
		sKeyTable[XK_KP_Decimal] = KLGLKeyEvent::KEY_KP_PERIOD;
		sKeyTable[XK_KP_Divide] = KLGLKeyEvent::KEY_KP_DIVIDE;
		sKeyTable[XK_KP_Multiply] = KLGLKeyEvent::KEY_KP_MULTIPLY;
		sKeyTable[XK_KP_Subtract] = KLGLKeyEvent::KEY_KP_MINUS;
		sKeyTable[XK_KP_Add] = KLGLKeyEvent::KEY_KP_PLUS;

		sKeyTable[XK_Up] = KLGLKeyEvent::KEY_UP;
		sKeyTable[XK_Down] = KLGLKeyEvent::KEY_DOWN;
		sKeyTable[XK_Right] = KLGLKeyEvent::KEY_RIGHT;
		sKeyTable[XK_Left] = KLGLKeyEvent::KEY_LEFT;
		sKeyTable[XK_Insert] = KLGLKeyEvent::KEY_INSERT;
		sKeyTable[XK_Home] = KLGLKeyEvent::KEY_HOME;
		sKeyTable[XK_End] = KLGLKeyEvent::KEY_END;
		sKeyTable[XK_Page_Up] = KLGLKeyEvent::KEY_PAGEUP;
		sKeyTable[XK_Page_Down] = KLGLKeyEvent::KEY_PAGEDOWN;

		sKeyTable[XK_F1] = KLGLKeyEvent::KEY_F1;
		sKeyTable[XK_F2] = KLGLKeyEvent::KEY_F2;
		sKeyTable[XK_F3] = KLGLKeyEvent::KEY_F3;
		sKeyTable[XK_F4] = KLGLKeyEvent::KEY_F4;
		sKeyTable[XK_F5] = KLGLKeyEvent::KEY_F5;
		sKeyTable[XK_F6] = KLGLKeyEvent::KEY_F6;
		sKeyTable[XK_F7] = KLGLKeyEvent::KEY_F7;
		sKeyTable[XK_F8] = KLGLKeyEvent::KEY_F8;
		sKeyTable[XK_F9] = KLGLKeyEvent::KEY_F9;
		sKeyTable[XK_F10] = KLGLKeyEvent::KEY_F10;
		sKeyTable[XK_F11] = KLGLKeyEvent::KEY_F11;
		sKeyTable[XK_F12] = KLGLKeyEvent::KEY_F12;
		sKeyTable[XK_F13] = KLGLKeyEvent::KEY_F13;
		sKeyTable[XK_F14] = KLGLKeyEvent::KEY_F14;
		sKeyTable[XK_F15] = KLGLKeyEvent::KEY_F15;

		sKeyTable[XK_Num_Lock] = KLGLKeyEvent::KEY_NUMLOCK;
		sKeyTable[XK_Caps_Lock] = KLGLKeyEvent::KEY_CAPSLOCK;
		sKeyTable[XK_Scroll_Lock] = KLGLKeyEvent::KEY_SCROLLOCK;
		sKeyTable[XK_Shift_R] = KLGLKeyEvent::KEY_RSHIFT;
		sKeyTable[XK_Shift_L] = KLGLKeyEvent::KEY_LSHIFT;
		sKeyTable[XK_Control_R] = KLGLKeyEvent::KEY_RCTRL;
		sKeyTable[XK_Control_L] = KLGLKeyEvent::KEY_LCTRL;
		sKeyTable[XK_Alt_R] = KLGLKeyEvent::KEY_RALT;
		sKeyTable[XK_Alt_L] = KLGLKeyEvent::KEY_LALT;
		sKeyTable[XK_Meta_R] = KLGLKeyEvent::KEY_RSUPER;
		sKeyTable[XK_Meta_L] = KLGLKeyEvent::KEY_LSUPER;

		sKeyTable[XK_Help] = KLGLKeyEvent::KEY_HELP;
		sKeyTable[XK_Print] = KLGLKeyEvent::KEY_PRINT;
		sKeyTable[XK_Cancel] = KLGLKeyEvent::KEY_BREAK;
		sKeyTable[XK_Menu] = KLGLKeyEvent::KEY_MENU;

		sTableInited = true;
	}
}
