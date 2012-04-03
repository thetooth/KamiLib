#pragma comment(lib,"kami.lib")

#include "kami.h"
#include "objload.h"
#include <wchar.h>
//#include <vld.h>

using namespace klib;

int main(){
	KLGL *gc;
	KLGLTexture *testTexture, *bgTexture;
	KLGLFont *print;
	Obj::File* obj;
	try{
		gc = new KLGL("Test02", 800, 640, 60, false, 1, 1);
		print = new KLGLFont();
		gc->InitShaders(1, 0, "common/default.vert", "common/cloud.frag");
		gc->InitShaders(2, 0, "common/model.vert", "common/model.frag");
		gc->InitShaders(3, 0, "common/diffuse.vert", "common/diffuse.frag");
		gc->InitShaders(4, 0, "common/default.vert", "common/blur.frag");
		bgTexture = new KLGLTexture("common/wallpaper-1370530.png");
		//testTexture = new KLGLTexture("common/logo.png");
		obj = new Obj::File();
		obj->Load("common/sea.obj");
	}catch(KLGLException e){
		MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
	}
	//FreeConsole();

	int status = 1;
	int selected = 0;
	float rotation = 0.0f, scale, blurBias;
	wchar_t* consoleBuffer = new wchar_t[4096];
	wchar_t* tmpclBuffer = new wchar_t[4096];
	POINT mouseXY;

	while(status){
		if(PeekMessage(&gc->msg, NULL, NULL, NULL, PM_REMOVE)){
			switch(gc->msg.message){
			case WM_QUIT:
				PostQuitMessage(0);
				status = 0;
				break;
			case WM_KEYDOWN:
				switch(gc->msg.wParam){
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
				TranslateMessage(&gc->msg);
				DispatchMessage(&gc->msg);
			}
		}

		if (selected > 16){
			selected = 16;
		}else if(selected < 1){
			selected = 1;
		}

		GetCursorPos(&mouseXY);
		ScreenToClient(gc->hWnd, &mouseXY);
		gc->OpenFBO(50, 0.0, 0.0, 80.0f);

		// Values
		rotation += 0.25f;
		scale = min(1.0f, mouseXY.y/1000.0f);
		blurBias = max(0.0f, mouseXY.x/1000.0f/25.0f);

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
		gc->BindShaders(3);
		glPushMatrix();
		{
			glScaled(3.0, 3.0, 3.0);
			glRotatef( rotation, 1.0f, 1.0f, 1.0f );
			glBindTexture(GL_TEXTURE_2D, testTexture->gltexture);
			obj->Draw();
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		glPopMatrix();
		gc->UnbindShaders();*/

		gc->OrthogonalStart();
		{

			/*gc->BindShaders(1);
			glUniform1f(glGetUniformLocation(gc->GetShaderID(1), "time"), rotation/100.0f);
			glUniform2f(glGetUniformLocation(gc->GetShaderID(1), "resolution"), gc->window.width*gc->overSampleFactor, gc->window.height*gc->overSampleFactor);
			glUniform2f(glGetUniformLocation(gc->GetShaderID(1), "mouse"), -mouseXY.x/1000.0f, -mouseXY.y/1000.0f);
			gc->BindMultiPassShader(1, 1, false);
			gc->UnbindShaders();*/

			// Sprite
			gc->Blit2D(bgTexture, (gc->window.width/2)-(bgTexture->width/2), (gc->window.height/2)-(bgTexture->height/2), rotation, 0.65f);

			// Aero
			int x = 8;
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
					print->color = new KLGLColor(0, 0, 0);
				}else{
					print->color = new KLGLColor(255, 255, 255);
				}
				print->Draw(x+6, y+7+((i-1)*16), consoleBuffer);
			}

			mbstowcs(tmpclBuffer, clBuffer, 4096);
			swprintf(consoleBuffer, L"@D@CFFFFFF%ls\n\nx: %d y: %d r: %f s: %f b: %f\n%lc", tmpclBuffer, mouseXY.x, mouseXY.y, rotation, scale, blurBias, gc->shaderClock);
			print->Draw(0, 8, consoleBuffer);
			
		}
		gc->OrthogonalEnd();
		gc->Swap();
	}
	delete gc;
	return 0;
}
