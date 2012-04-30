#pragma once

#include <string>
#include <wchar.h>
#include "kami.h"

namespace klib {

	class UI
	{
	public:
		UI(void);
		~UI(void);

	};

	enum UIButtonState{NONE,HOVER,DOWN,BAIL,UP};
	class UIButton : public UI {
	public:
		int state,lastState,mouseState;
		Rect<int> pos;
		POINT mousePosition;
		std::wstring text;
		UIButton(KLGL *gcPtr, int initialState = 0);
		~UIButton();
		void draw();
		void comp();
	private:
		KLGL *gc;
		KLGLFont *font;
		KLGLSprite *sprite;
		int _getState(POINT mouse, int mouseState);
	};
}