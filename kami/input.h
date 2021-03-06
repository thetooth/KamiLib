#pragma once

#include "event.h"
#include <limits.h>

namespace klib {
	static bool sTableInited = false;
	static const int MAX_KEYCODE = 65536;
	extern unsigned int sKeyTable[MAX_KEYCODE];

	static void initKeyTable();

	class KLGLKeyEvent : public KLGLEvent {
	public:	
		KLGLKeyEvent( unsigned int aCode, char aChar, unsigned int aModifiers, unsigned int aNativeKeyCode, bool aStroke )
			: KLGLEvent(), mCode( aCode ), mChar( aChar ), mModifiers( aModifiers ), mNativeKeyCode( aNativeKeyCode ), mStroke(aStroke), mFinished(false) {}

		//! Returns the key code associated with the event, which maps into the enum listed below
		int		getCode() const { return mCode; }
		//! Returns the ASCII character associated with the event.
		char	getChar() const { return mChar; }
		//! Returns the modifier character associated with the event.
		char	getModifier() const { return mModifiers; }
		//! Returns the platform-native key-code. Advanced users only.
		int		getNativeKeyCode() const { return mNativeKeyCode; }
		//! Returns whether the Shift key was pressed during the event.
		bool	isShiftDown() const { return (mModifiers & SHIFT_DOWN) ? true : false; }
		//! Returns whether the Alt (or Option) key was pressed during the event.
		bool	isAltDown() const { return (mModifiers & ALT_DOWN) ? true : false; }
		//! Returns whether the Control key was pressed during the event.
		bool	isControlDown() const { return (mModifiers & CTRL_DOWN) ? true : false; }
		//! Returns whether the meta key was pressed during the event. Maps to the Windows key on Windows and the Command key on Mac OS X.
		bool	isMetaDown() const { return (mModifiers & META_DOWN) ? true : false; }	
		//! Returns whether the accelerator key was pressed during the event. Maps to the Control key on Windows and the Command key on Mac OS X.
		bool	isAccelDown() const { return (mModifiers & ACCEL_DOWN) ? true : false; }
		//! Returns true if the key was traveling down when the event was created.
		bool	isDown() { return mStroke; }
		//! Returns whether the event is ready for deletion
		bool	isfinished() { return mFinished; }
		//! Marks the event for deletion
		void	finished(bool state = true) { mFinished = true; }

		//! Maps a platform-native key-code to the key code enum
		static unsigned int translateNativeKeyCode( int nativeKeyCode ){
			if( ! sTableInited || nativeKeyCode >= MAX_KEYCODE ){
				return KLGLKeyEvent::KEY_UNKNOWN;
			}else{
				return sKeyTable[nativeKeyCode];
			}
		};

		enum {
			SHIFT_DOWN	= 0x0008,
			ALT_DOWN	= 0x0010,
			CTRL_DOWN	= 0x0020,
			META_DOWN	= 0x0040,
#if defined( _WIN32 )
			ACCEL_DOWN	= CTRL_DOWN
#else
			ACCEL_DOWN	= META_DOWN
#endif
		};

		// Key codes
		enum {
			KEY_UNKNOWN		= 0,
			KEY_FIRST		= 0,
			KEY_BACKSPACE	= 8,
			KEY_TAB			= 9,
			KEY_CLEAR		= 12,
			KEY_RETURN		= 13,
			KEY_PAUSE		= 19,
			KEY_ESCAPE		= 27,
			KEY_SPACE		= 32,
			KEY_EXCLAIM		= 33,
			KEY_QUOTEDBL	= 34,
			KEY_HASH		= 35,
			KEY_DOLLAR		= 36,
			KEY_AMPERSAND	= 38,
			KEY_QUOTE		= 39,
			KEY_LEFTPAREN	= 40,
			KEY_RIGHTPAREN	= 41,
			KEY_ASTERISK	= 42,
			KEY_PLUS		= 43,
			KEY_COMMA		= 44,
			KEY_MINUS		= 45,
			KEY_PERIOD		= 46,
			KEY_SLASH		= 47,
			KEY_0			= 48,
			KEY_1			= 49,
			KEY_2			= 50,
			KEY_3			= 51,
			KEY_4			= 52,
			KEY_5			= 53,
			KEY_6			= 54,
			KEY_7			= 55,
			KEY_8			= 56,
			KEY_9			= 57,
			KEY_COLON		= 58,
			KEY_SEMICOLON	= 59,
			KEY_LESS		= 60,
			KEY_EQUALS		= 61,
			KEY_GREATER		= 62,
			KEY_QUESTION	= 63,
			KEY_AT			= 64,

			KEY_LEFTBRACKET	= 91,
			KEY_BACKSLASH	= 92,
			KEY_RIGHTBRACKET= 93,
			KEY_CARET		= 94,
			KEY_UNDERSCORE	= 95,
			KEY_BACKQUOTE	= 96,
			KEY_a			= 97,
			KEY_b			= 98,
			KEY_c			= 99,
			KEY_d			= 100,
			KEY_e			= 101,
			KEY_f			= 102,
			KEY_g			= 103,
			KEY_h			= 104,
			KEY_i			= 105,
			KEY_j			= 106,
			KEY_k			= 107,
			KEY_l			= 108,
			KEY_m			= 109,
			KEY_n			= 110,
			KEY_o			= 111,
			KEY_p			= 112,
			KEY_q			= 113,
			KEY_r			= 114,
			KEY_s			= 115,
			KEY_t			= 116,
			KEY_u			= 117,
			KEY_v			= 118,
			KEY_w			= 119,
			KEY_x			= 120,
			KEY_y			= 121,
			KEY_z			= 122,
			KEY_DELETE		= 127,

			KEY_KP0			= 256,
			KEY_KP1			= 257,
			KEY_KP2			= 258,
			KEY_KP3			= 259,
			KEY_KP4			= 260,
			KEY_KP5			= 261,
			KEY_KP6			= 262,
			KEY_KP7			= 263,
			KEY_KP8			= 264,
			KEY_KP9			= 265,
			KEY_KP_PERIOD	= 266,
			KEY_KP_DIVIDE	= 267,
			KEY_KP_MULTIPLY	= 268,
			KEY_KP_MINUS	= 269,
			KEY_KP_PLUS		= 270,
			KEY_KP_ENTER	= 271,
			KEY_KP_EQUALS	= 272,

			KEY_UP			= 273,
			KEY_DOWN		= 274,
			KEY_RIGHT		= 275,
			KEY_LEFT		= 276,
			KEY_INSERT		= 277,
			KEY_HOME		= 278,
			KEY_END			= 279,
			KEY_PAGEUP		= 280,
			KEY_PAGEDOWN	= 281,

			KEY_F1			= 282,
			KEY_F2			= 283,
			KEY_F3			= 284,
			KEY_F4			= 285,
			KEY_F5			= 286,
			KEY_F6			= 287,
			KEY_F7			= 288,
			KEY_F8			= 289,
			KEY_F9			= 290,
			KEY_F10			= 291,
			KEY_F11			= 292,
			KEY_F12			= 293,
			KEY_F13			= 294,
			KEY_F14			= 295,
			KEY_F15			= 296,

			KEY_NUMLOCK		= 300,
			KEY_CAPSLOCK	= 301,
			KEY_SCROLLOCK	= 302,
			KEY_RSHIFT		= 303,
			KEY_LSHIFT		= 304,
			KEY_RCTRL		= 305,
			KEY_LCTRL		= 306,
			KEY_RALT		= 307,
			KEY_LALT		= 308,
			KEY_RMETA		= 309,
			KEY_LMETA		= 310,
			KEY_LSUPER		= 311,		/* Left "Windows" key */
			KEY_RSUPER		= 312,		/* Right "Windows" key */
			KEY_MODE		= 313,		/* "Alt Gr" key */
			KEY_COMPOSE		= 314,		/* Multi-key compose key */

			KEY_HELP		= 315,
			KEY_PRINT		= 316,
			KEY_SYSREQ		= 317,
			KEY_BREAK		= 318,
			KEY_MENU		= 319,
			KEY_POWER		= 320,		/* Power Macintosh power key */
			KEY_EURO		= 321,		/* Some european keyboards */
			KEY_UNDO		= 322,		/* Atari keyboard has Undo */

			KEY_MLBUTTON	= 323,
			KEY_MRBUTTON	= 324,
			KEY_MMBUTTON	= 325,

			KEY_LAST
		};

	protected:
		unsigned int	mCode;
		char			mChar;
		unsigned int	mModifiers;
		unsigned int	mNativeKeyCode;
		bool			mFinished;
		bool			mStroke;
	};
}