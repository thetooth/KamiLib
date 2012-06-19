#include "UI.h"

namespace klib{
	UI::UI(void)
	{
	}


	UI::~UI(void)
	{
	}

	UIDialog::UIDialog(KLGL *gcPtr, const char* spriteSheet){
		gc = gcPtr;
		gc->GenFrameBuffer(fbo, fboTexture, fboDepth);
		uiSprite = new KLGLSprite(spriteSheet, 16, 16);
		pos.x = 24;
		pos.y = 188;
		pos.width = 400;
		pos.height = 320;
	}

	void UIDialog::draw(){
		gc->Rectangle2D(pos.x+16, pos.y+16, pos.width-16, pos.height-16, KLGLColor(250,250,251));
		KLGLColor(255, 255, 255, 255).Set();
		gc->BlitSprite2D(uiSprite, pos.x,			pos.y,		0, 0);
		gc->BlitSprite2D(uiSprite, pos.x+pos.width,		pos.y,		0, 2);
		gc->BlitSprite2D(uiSprite, pos.x,			pos.y+pos.height,	2, 0);
		gc->BlitSprite2D(uiSprite, pos.x+pos.width,		pos.y+pos.height,	2, 2);
		for (int x = 1; x <= pos.width/16; x++)
		{
			gc->BlitSprite2D(uiSprite, pos.x+(x == pos.width/16 ? pos.width-16 : 16*x),	pos.y,				0, 1);
			gc->BlitSprite2D(uiSprite, pos.x+(x == pos.width/16 ? pos.width-16 : 16*x),	pos.y+pos.height,	2, 1);
		}
		for (int y = 1; y <= pos.height/16; y++)
		{
			gc->BlitSprite2D(uiSprite, pos.x,			pos.y+(y == pos.height/16 ? pos.height-16 : 16*y),	1, 0);
			gc->BlitSprite2D(uiSprite, pos.x+pos.width,	pos.y+(y == pos.height/16 ? pos.height-16 : 16*y),	1, 2);
		}

		for (std::vector<UIButton*>::iterator button = buttonList.begin(); button != buttonList.end(); button++)
		{
			(*button)->comp();
			(*button)->draw();
		}
	}

	UIButton::UIButton(KLGL *gcPtr, int initialState){
		gc = gcPtr;
		//font = new KLGLFont();
		//sprite = new KLGLSprite("common/button.png", 256, 48);
		callbackPtr = NULL;
		callbackParam = NULL;
		offset = Rect<int*>();
		pos = Rect<int>();
		state = initialState;
		lastState = state;
		absolutePos = 0;
	}

	int UIButton::_getState(POINT mouse, int mouseState){
		int x,y;
		if (absolutePos){
			x = pos.x;
			y = pos.y;
		}else{
			x = *offset.x+pos.x;
			y = *offset.y+pos.y;
		}
		if (mouse.x > x && mouse.y > y && mouse.x < x+pos.width && mouse.y < y+pos.height){
			if (mouseState && lastState){
				return UIState::DOWN;
			}else if(!mouseState && (state == UIState::DOWN || lastState == 0)){
				if (!lastState){
					lastState = 1;
					return UIState::NONE;
				}else{
					return UIState::UP;
				}
			}
			return UIState::HOVER;
		}else if (mouseState && (state == UIState::DOWN || state == UIState::BAIL)){
			return UIState::BAIL;
		}else if (mouseState){
			lastState = 0;
			return UIState::NONE;
		}
		lastState = 1;
		return UIState::NONE;
	}

	void UIButton::comp(){
		state = _getState(mousePosition, mouseState);
		if(state == UIState::UP && callbackPtr != NULL){
			return (void)callbackPtr(callbackParam);
		}
	}

	void UIButton::draw(){
		int x,y;
		if (absolutePos){
			x = pos.x;
			y = pos.y;
		}else{
			x = *offset.x+pos.x;
			y = *offset.y+pos.y;
		}
		font->color->Assign(255, 255, 255);
		switch(state){
		case UIState::HOVER:
			gc->Rectangle2D(x, y, pos.width, pos.height, KLGLColor(255, 0, 0, 127));
			//gc->BlitSprite2D(sprite, offsetX+x, y, UIButtonState::HOVER);
			break;
		case UIState::DOWN:
			gc->Rectangle2D(x, y, pos.width, pos.height, KLGLColor(0, 255, 0, 127));
			//gc->BlitSprite2D(sprite, offsetX+x, y, UIButtonState::DOWN);
			break;
		case UIState::BAIL:
			gc->Rectangle2D(x, y, pos.width, pos.height, KLGLColor(255, 255, 0, 127));
			//gc->BlitSprite2D(sprite, offsetX+x, y, UIButtonState::BAIL);
			break;
		case UIState::UP:
			gc->Rectangle2D(x, y, pos.width, pos.height, KLGLColor(255, 255, 255, 127));
			//gc->BlitSprite2D(sprite, x, y, UIButtonState::UP);
			break;
		default:
			gc->Rectangle2D(x, y, pos.width, pos.height, KLGLColor(0, 0, 0, 127));
			font->color->Assign(255, 255, 255);
			//gc->BlitSprite2D(sprite, x, y, UIButtonState::NONE);
		}
		// Draw button text
		font->Draw(
			x+((pos.width/2)-((font->c_width*text.length())/1.5)),
			y+((pos.height/2)-(font->c_height/2)),
			const_cast<wchar_t*>(text.c_str())
		);
		// Reset pallet
		KLGLColor(255, 255, 255, 255).Set();
	}
}