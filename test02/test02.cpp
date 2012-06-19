#include "kami.h"
#include "objload.h"
#include "UI.h"
#include "tween.h"
#include <wchar.h>
#include <fstream>
//#include <vld.h>

#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#else
#pragma comment(lib,"kami.lib")
#endif

using namespace klib;

#include <fmod.hpp>
#include <fmod_errors.h>

#pragma comment(lib,"fmodex64_vc.lib")

struct callbackParamType 
{
	tween::Tweener *tweener;
	int transition;
	int equation;
};
float tweenX = 0, tweenY = 0;
void queueAnimCallback(void* parma){
	callbackParamType *data = (callbackParamType *)parma;
	auto *tweener = data->tweener;
	static auto anim = tween::TweenerParam(2000, data->transition, data->equation);
	anim.addProperty(&tweenX, 350);
	anim.addProperty(&tweenY, 100);
	anim.setRepeatWithReverse(1, true);
	tweener->addTween(&anim);
}

void FMODCHECK(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		exit(-1);
	}
}

int main(){
	KLGL *gc = NULL;
	KLGLTexture *testTexture = NULL, *bgTexture = NULL, *fontTexture = NULL;
	KLGLSprite *uiSprite = NULL;
	KLGLFont *print = NULL, *buttonFont = NULL;
	Obj::File* obj = NULL;
	UIButton *button[10] = {NULL};
	UIDialog *dialog = NULL;

	int status = 1;
	clock_t t0 = 0,t1 = 0,t2 = 0;
	int selected = 0;
	float rotation = 0.0f, scale, blurBias;
	wchar_t* consoleBuffer = new wchar_t[4096];
	wchar_t* tmpclBuffer = new wchar_t[4096];
	POINT mouseXY;
	short mouseState;

	tween::Tweener tweener;
	tween::TweenerParam *anim[12] = {NULL};
	callbackParamType animationPack[10] = {NULL};

	FMOD::System     *system;
	FMOD::Sound      *sound;
	FMOD::Channel    *channel = 0;
	FMOD_RESULT       result;
	int               key;
	unsigned int      version;

	try{
		gc = new KLGL("Test02", 800, 640, 60, false, 1, 1);
		fontTexture = new KLGLTexture("common/UI_font.png");
		print = new KLGLFont();
		buttonFont = new KLGLFont(fontTexture->gltexture, fontTexture->width, fontTexture->height, 20, 20, -1);
		dialog = new UIDialog(gc, "common/UI_Interface.png");
		Rect<int> buttonSize = Rect<int>(16, 16, 200, 32);

		/*
		Create a System object and initialize.
		*/
		result = FMOD::System_Create(&system);
		FMODCHECK(result);

		result = system->getVersion(&version);
		FMODCHECK(result);

		if (version < FMOD_VERSION)
		{
			printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
			return 0;
		}

		result = system->init(1, FMOD_INIT_NORMAL, 0);
		FMODCHECK(result);

		result = system->createSound("common/MF-PANTS.MOD", FMOD_DEFAULT|FMOD_LOOP_NORMAL, 0, &sound);
		FMODCHECK(result);

		result = system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
		FMODCHECK(result);

		#pragma parallel
		for (int i = 0; i < 5; i++)
		{
			button[i] = new UIButton(gc);
			button[i]->pos = Rect<int>(buttonSize.x+(i >= 5 ? buttonSize.width+2 : 0), buttonSize.y+((i >= 5 ? i-5 : i)*(buttonSize.height+2)), buttonSize.width, buttonSize.height);
			button[i]->font = buttonFont;
			button[i]->text = L"No." + wstringify(i);

			animationPack[i].tweener = &tweener;
			animationPack[i].transition = i;
			animationPack[i].equation = tween::EASE_IN_OUT;
			//button[i]->callbackPtr = queueAnimCallback;
			button[i]->callbackParam = (void*)&animationPack[i];
			button[i]->offset.x = &dialog->pos.x;
			button[i]->offset.y = &dialog->pos.y;
			dialog->buttonList.push_back(button[i]);
		}

		#pragma parallel
		for (int i = 0; i < 1; i++)
		{
			anim[i] = new tween::TweenerParam(1000, tween::SINE, tween::EASE_IN_OUT);
			anim[i]->addProperty(&tweenX, 350);
			anim[i]->addProperty(&tweenY, 100);
			anim[i]->setRepeatWithReverse(100, true);
			anim[i]->delay = 4000*i;
			tweener.addTween(anim[i]);
		}

		gc->InitShaders(0, 0, "common/default.vert", "common/hipstervision.frag");
		gc->InitShaders(1, 0, "common/default.vert", "common/demo2.frag");
		//gc->InitShaders(2, 0, "common/default.vert", "common/ssao.frag");
		//gc->InitShaders(3, 0, "common/diffuse.vert", "common/diffuse.frag");
		//gc->InitShaders(4, 0, "common/default.vert", "common/blur.frag");

		//bgTexture = new KLGLTexture("common/wallpaper-1370530.png");
		testTexture = new KLGLTexture("common/tmp.png");
		
		//obj = new Obj::File();
		//obj->Load("common/sea.obj");
		
		uiSprite = new KLGLSprite("common/UI_Interface.png", 16, 16);
	}catch(KLGLException e){
		MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
		exit(-1);
	}
	//FreeConsole();

	while(status){
		t0 = clock();
		if(PeekMessage(&gc->windowManager->wm->msg, NULL, NULL, NULL, PM_REMOVE)){
			switch(gc->windowManager->wm->msg.message){
			case WM_QUIT:
				PostQuitMessage(0);
				status = 0;
				break;
			case WM_KEYDOWN:
				switch(gc->windowManager->wm->msg.wParam){
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;
				case VK_UP:
					selected--;
					if (selected < 1){
						selected = 16;
					}
					break;
				case VK_DOWN:
					selected++;
					if (selected > 16){
						selected = 1;
					}
					break;
				}
				break;
			default:
				TranslateMessage(&gc->windowManager->wm->msg);
				DispatchMessage(&gc->windowManager->wm->msg);
			}
		}else if (t0-t1 >= CLOCKS_PER_SEC/gc->fps){
			t1 = clock();
			if (selected > 16){
				selected = 16;
			}else if(selected < 1){
				selected = 1;
			}

			GetCursorPos(&mouseXY);
			ScreenToClient(gc->windowManager->wm->hWnd, &mouseXY);
			mouseState = (GetAsyncKeyState(VK_LBUTTON) != 0 ? 1 : 0);
			gc->OpenFBO(50, 0.0, 0.0, 80.0f);

			// Values
			rotation += 0.5f;
			scale = min(1.0f, mouseXY.y/1000.0f);
			blurBias = max(0.0f, mouseXY.x/1000.0f/25.0f);

			system->update();
			//channel->setFrequency(tweenY*1000.0f);

			gc->OrthogonalStart();
			{
				// Background
				glBegin(GL_QUADS);
				glColor3ub(71,84,93);
				glVertex2i(0, 0); glVertex2i(1024, 0);
				glColor3ub(214,220,214);
				glVertex2i(1024, 640); glVertex2i(0, 640);
				glEnd();

				// Reset pallet
				KLGLColor(255, 255, 255, 255).Set();
			}
			gc->OrthogonalEnd();

			/*glEnable(GL_LIGHTING);
			gc->light_position.z = mouseXY.x/100.0f;
			glLightfv(GL_LIGHT0, GL_POSITION, gc->light_position.Data());

			glPushMatrix();
			{
			//glScaled(3.0, 3.0, 3.0);*/

			/*glBindTexture(GL_TEXTURE_2D, gc->fbo_texture[1]);
			//gc->BindShaders(3);

			glBegin(GL_QUADS);
			glTexCoord2d(0.0,0.0);
			glVertex3f(0.0, 0.0, 0.0);
			glTexCoord2d(1.0,0.0);
			glVertex3f(1.0, 0.0, 0.0);
			glTexCoord2d(1.0,1.0);
			glVertex3f(1.0, 1.0, 0.0);
			glTexCoord2d(0.0,1.0);
			glVertex3f(0.0, 1.0, 0.0);
			glEnd();

			//gc->UnbindShaders();
			glBindTexture(GL_TEXTURE_2D, 0);
			}
			glPopMatrix();*/

			gc->OrthogonalStart();
			{

				/*gc->BindShaders(1);
				glUniform1f(glGetUniformLocation(gc->GetShaderID(1), "time"), rotation/1.0f);
				glUniform2f(glGetUniformLocation(gc->GetShaderID(1), "resolution"), gc->window.width*gc->overSampleFactor, gc->window.height*gc->overSampleFactor);
				glUniform2f(glGetUniformLocation(gc->GetShaderID(1), "mouse"), -mouseXY.x/1.0f, -mouseXY.y/1.0f);
				gc->BindMultiPassShader(1, 1, false);
				gc->UnbindShaders();*/

				/*
				// Sprite
				gc->Blit2D(bgTexture, (gc->window.width/2)-(bgTexture->width/2), (gc->window.height/2)-(bgTexture->height/2), rotation, 0.65f);*/

				// Aero
				/*int x = 8;
				int y = gc->window.height-(256+8);
				gc->BindShaders(4);
				glUniform1f(glGetUniformLocation(gc->GetShaderID(4), "time"), rotation/100.0f);
				glUniform2f(glGetUniformLocation(gc->GetShaderID(4), "resolution"), gc->window.width*gc->overSampleFactor, gc->window.height*gc->overSampleFactor);
				glUniform4f(glGetUniformLocation(gc->GetShaderID(4), "region"), x*gc->overSampleFactor, 8*gc->overSampleFactor, 256*gc->overSampleFactor, 256*gc->overSampleFactor);
				// Since glsl keeps the bottom to top drawing origin we will run the shader multiple times rather than multipass to keep our region in the same place.
				gc->BindMultiPassShader(4, 1, true);
				gc->BindMultiPassShader(4, 1, true);
				gc->UnbindShaders();
				gc->Rectangle2D(x, y, 256, 256, KLGLColor(255, 255, 255, 50));
				for (int i = 1; i <= 16; i++)
				{
				int rectX = x+4;
				int rectY = y+4+((i-1)*16);
				int rectW = 248;
				int rectH = 14;
				if (mouseXY.x > rectX && mouseXY.y > rectY && mouseXY.x < rectX+rectW && mouseXY.y < rectY+rectH){
				selected = i;
				}
				swprintf(consoleBuffer, L"%d POTATO", i);
				if (i == selected){
				gc->Rectangle2D(rectX, rectY, rectW, rectH, KLGLColor(255, 255, 255));
				print->color->Assign(0, 0, 0);
				}else{
				print->color->Assign(255, 255, 255);
				}
				print->Draw(x+6, y+7+((i-1)*16), consoleBuffer);
				}*/

				mbstowcs(tmpclBuffer, clBuffer, 4096);
				swprintf(consoleBuffer, L"@D@CFFFFFF%ls\n\nx: %d y: %d C: %d r: %f s: %f b: %f\nTweener: %x", 
					tmpclBuffer, mouseXY.x, mouseXY.y, mouseState, rotation, scale, blurBias, 
					tweener.lastTime
				);
				print->Draw(0, 8, consoleBuffer);

				/*glTranslatef(gc->window.width/2, gc->window.height/2, 0);
				glRotatef( rotation, 0.0f, 0.0f, 1.0f );
				glTranslatef(-(gc->window.width/2), -(gc->window.height/2), 0);*/

				//dialog->pos.x = 40+(tweenX/2);
				//dialog->pos.y = 340+(tweenY/2);
				//dialog->pos.width = 32+tweenX;
				//dialog->pos.height = 32+tweenY*2;
				dialog->draw();

				for (int i = 0; i < 5; i++)
				{
					button[i]->mousePosition = mouseXY;
					button[i]->mouseState = mouseState;
					//button[i]->comp();
					if (button[i]->state == UIState::UP){
						cl("Button %d Pressed.\n", i);
					}else if (button[i]->state == UIState::DOWN && GetAsyncKeyState(VK_SHIFT)){
						button[i]->pos.x = mouseXY.x-(button[i]->pos.width/2);
						button[i]->pos.y = mouseXY.y-(button[i]->pos.height/2);
					}
					/*gc->BindShaders(4);
					glUniform1f(glGetUniformLocation(gc->GetShaderID(4), "time"), rotation/100.0f);
					glUniform2f(glGetUniformLocation(gc->GetShaderID(4), "resolution"), gc->window.width*gc->overSampleFactor, gc->window.height*gc->overSampleFactor);
					glUniform4f(glGetUniformLocation(gc->GetShaderID(4), "region"), 
						button[i]->pos.x*gc->overSampleFactor, 
						(button[i]->pos.y+button[i]->pos.height)*gc->overSampleFactor, 
						button[i]->pos.width*gc->overSampleFactor, 
						button[i]->pos.height*gc->overSampleFactor
						);
					// Since glsl keeps the bottom to top drawing origin we will run the shader multiple times rather than multipass to keep our region in the same place.
					gc->BindMultiPassShader(4, 1, true);
					//gc->BindMultiPassShader(4, 1, true);
					gc->UnbindShaders();
					button[i]->draw();*/
				}

				buttonFont->Draw(dialog->pos.x+200, dialog->pos.y+200-dialog->pos.height,L"@C333333oi cunt");
				
				tweener.step(clock());
				//gc->Blit2D(testTexture, 40+tweenX, 120+int(cos((tweenY-50.0f)/25.0f)*120), tweenX, 1+(tweenY/100.0f));
			}
			gc->OrthogonalEnd();
			gc->Swap();
		}
	}

	result = sound->release();
	FMODCHECK(result);
	result = system->close();
	FMODCHECK(result);
	result = system->release();
	FMODCHECK(result);

	delete [] consoleBuffer,tmpclBuffer;
	delete print,testTexture,bgTexture,uiSprite;
	delete gc;
	return 0;
}
