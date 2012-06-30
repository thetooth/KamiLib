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
		gc->GenFrameBuffer(gc->fbo[10], gc->fbo_texture[10], gc->fbo_depth[10]);
		cached = 0;
		uiSprite = new KLGLSprite(spriteSheet, 16, 16);
		bgColor = new KLGLColor(255, 255, 255, 255);
		pos.x = 24;
		pos.y = 188;
		pos.width = 400;
		pos.height = 320;
	}

	void UIDialog::draw(){
		if(pos.width != prevPos.width || pos.height != prevPos.height){
			prevPos = pos;
			cached = 0;
		}
		if (!cached){
			//gc->GenFrameBuffer(gc->fbo[10], gc->fbo_texture[10], gc->fbo_depth[10], pos.width+16);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gc->fbo[10]);
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			gc->Rectangle2D(0+16, 0+16, pos.width-16, pos.height-16, *bgColor);
			KLGLColor(255, 255, 255, 255).Set();
			gc->BlitSprite2D(uiSprite, 0,			0,				0, 0);
			gc->BlitSprite2D(uiSprite, 0+pos.width,	0,				0, 2);
			gc->BlitSprite2D(uiSprite, 0,			0+pos.height,	2, 0);
			gc->BlitSprite2D(uiSprite, 0+pos.width,	0+pos.height,	2, 2);
			for (int x = 1; x <= pos.width/16; x++)
			{
				gc->BlitSprite2D(uiSprite, 0+(x == pos.width/16 ? pos.width-16 : 16*x),	0,
					0, 1, 1, (x == pos.width/16 ? Rect<int>((x*16-(pos.width-16)), 0, 0, 0) : Rect<int>(0,0,0,0))
					);
				gc->BlitSprite2D(uiSprite, 0+(x == pos.width/16 ? pos.width-16 : 16*x),	0+pos.height,
					2, 1, 1, (x == pos.width/16 ? Rect<int>((x*16-(pos.width-16)), 0, 0, 0) : Rect<int>(0,0,0,0))
					);
			}
			for (int y = 1; y <= pos.height/16; y++)
			{
				gc->BlitSprite2D(uiSprite, 0,			0+(y == pos.height/16 ? pos.height-16 : 16*y),
					1, 0, 1, (y == pos.height/16 ? Rect<int>(0, (y*16-(pos.height-16)), 0, 0) : Rect<int>(0,0,0,0))
					);
				gc->BlitSprite2D(uiSprite, 0+pos.width,	0+(y == pos.height/16 ? pos.height-16 : 16*y),
					1, 2, 1, (y == pos.height/16 ? Rect<int>(0, (y*16-(pos.height-16)), 0, 0) : Rect<int>(0,0,0,0))
					);
			}
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gc->fbo[0]);
			cached = 1;
			if (KLGLDebug)
			{
				//gc->Rectangle2D(pos.x, pos.y, gc->window.width, gc->window.height, KLGLColor(255,0,0,100));
			}
		}

		glBindTexture(GL_TEXTURE_2D, gc->fbo_texture[10]);
		glBegin(GL_QUADS);
		glTexCoord2d(0.0,1.0); glVertex2i(pos.x,					pos.y);
		glTexCoord2d(1.0,1.0); glVertex2i(pos.x+gc->window.width,	pos.y);
		glTexCoord2d(1.0,0.0); glVertex2i(pos.x+gc->window.width,	pos.y+gc->window.height);
		glTexCoord2d(0.0,0.0); glVertex2i(pos.x,					pos.y+gc->window.height);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);

		for (std::vector<UIButton*>::iterator button = buttonList.begin(); button != buttonList.end(); button++)
		{
			(*button)->comp();
			(*button)->draw();
		}
	}

	void UIDialog::comp(){
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