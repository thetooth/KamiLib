#include "UI.h"

namespace klib{
	UI::UI(void)
	{
	}


	UI::~UI(void)
	{
	}

	UIButton::UIButton(KLGL *gcPtr, int initialState){
		gc = gcPtr;
		font = new KLGLFont();
		//sprite = new KLGLSprite("common/button.png", 256, 48);
		pos = Rect<int>();
		state = initialState;
		lastState = state;
	}

	int UIButton::_getState(POINT mouse, int mouseState){
		if (mouse.x > pos.x && mouse.y > pos.y && mouse.x < pos.x+pos.width && mouse.y < pos.y+pos.height){
			if (mouseState && lastState){
				return UIButtonState::DOWN;
			}else if(!mouseState && (state == UIButtonState::DOWN || lastState == 0)){
				if (!lastState){
					lastState = 1;
					return UIButtonState::NONE;
				}else{
					return UIButtonState::UP;
				}
			}
			return UIButtonState::HOVER;
		}else if (mouseState && (state == UIButtonState::DOWN || state == UIButtonState::BAIL)){
			return UIButtonState::BAIL;
		}else if (mouseState){
			lastState = 0;
			return UIButtonState::NONE;
		}
		lastState = 1;
		return UIButtonState::NONE;
	}

	void UIButton::comp(){
		state = _getState(mousePosition, mouseState);
	}

	void UIButton::draw(){
		font->color->Assign(255, 255, 255);
		switch(state){
		case UIButtonState::HOVER:
			gc->Rectangle2D(pos.x, pos.y, pos.width, pos.height, KLGLColor(255, 0, 0, 127));
			//gc->BlitSprite2D(sprite, pos.x, pos.y, UIButtonState::HOVER);
			break;
		case UIButtonState::DOWN:
			gc->Rectangle2D(pos.x, pos.y, pos.width, pos.height, KLGLColor(0, 255, 0, 127));
			//gc->BlitSprite2D(sprite, pos.x, pos.y, UIButtonState::DOWN);
			break;
		case UIButtonState::BAIL:
			gc->Rectangle2D(pos.x, pos.y, pos.width, pos.height, KLGLColor(255, 255, 0, 127));
			//gc->BlitSprite2D(sprite, pos.x, pos.y, UIButtonState::BAIL);
			break;
		case UIButtonState::UP:
			gc->Rectangle2D(pos.x, pos.y, pos.width, pos.height, KLGLColor(255, 255, 255, 127));
			//gc->BlitSprite2D(sprite, pos.x, pos.y, UIButtonState::UP);
			break;
		default:
			gc->Rectangle2D(pos.x, pos.y, pos.width, pos.height, KLGLColor(0, 0, 0, 127));
			font->color->Assign(255, 255, 255);
			//gc->BlitSprite2D(sprite, pos.x, pos.y, UIButtonState::NONE);
		}
		// Draw button text
		font->Draw(
			pos.x+((pos.width/2)-((font->c_width*text.length())/1.5)),
			pos.y+((pos.height/2)-(font->c_height/2)),
			const_cast<wchar_t*>(text.c_str())
		);
		// Reset pallet
		KLGLColor(255, 255, 255, 255).Set();
	}
}