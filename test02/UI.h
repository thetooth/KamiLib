#pragma once

#include <string>
#include <wchar.h>
#include "kami.h"

namespace klib {

	class UI
	{
	public:
		KLGL *gc;
		KLGLSprite *uiSprite;

		UI(void);
		~UI(void);

	};

	class UIButton;
	class UIDialog;

	enum UIState{NONE,HOVER,DOWN,BAIL,UP};

	class UIDialog : public UI {
	public:
		int state;
		Rect<int> pos;
		std::vector<UIButton*> buttonList;

		UIDialog(KLGL *gcPtr, const char* spriteSheet);

		void draw();
		void comp();
	private:
		GLuint fbo;
		GLuint fboTexture;
		GLuint fboDepth;
	};

	class UIButton : public UI {
	public:
		int state,lastState,mouseState,absolutePos;
		void (*callbackPtr)(void* parma);
		void *callbackParam;
		Rect<int*> offset;
		Rect<int> pos;
		POINT mousePosition;
		std::wstring text;
		KLGLFont *font;

		UIButton(KLGL *gcPtr, int initialState = 0);
		~UIButton();
		void draw();
		void comp();
	private:
		int _getState(POINT mouse, int mouseState);
	};
}