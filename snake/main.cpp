#include "kami.h"

using namespace klib;

#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#else
#pragma comment(lib,"kami.lib")
#endif

#include "painter.h"
#include "game.h"
#include "resource.h"

#include <time.h>

int main(int argc, char **argv)
{
	clock_t t0 = clock(), t1 = clock();
	bool quit = false, paused = false, vsync = true;
	KLGL* gc = new KLGL("Snake", Field::WIDTH * Field::BLOCK_WIDTH, Field::HEIGHT * Field::BLOCK_WIDTH, 60, false, 1, 2);

	Win32Resource vertShaderSrc("GLSL", IDR_GLSL1);
	Win32Resource fragShaderSrc("GLSL", IDR_GLSL2);
	Win32Resource chaTextureSrc("PNG",  IDB_8PXFONT);

	KLGLTexture *charmap = new KLGLTexture((unsigned char*)chaTextureSrc.pointer, chaTextureSrc.size);

	gc->InitShaders(1, (const char*)vertShaderSrc.pointer, (const char*)fragShaderSrc.pointer, 1);
	KLGLFont* text = new KLGLFont(charmap->gltexture, charmap->width, charmap->height, 8, 10);

	Game game;
	Painter p;

	if (quit){
		MessageBox(NULL, "An error was encountered while initializing!\nApplication will now terminate >:D", "Fatal Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

#ifndef _DEBUG
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	while (!quit)
	{
		t0 = clock();
		// check for messages
		if(PeekMessage( &gc->msg, NULL, 0, 0, PM_REMOVE)){
			// handle or dispatch messages
			if(gc->msg.message == WM_QUIT){
				quit = true;
			}else if (gc->msg.message == WM_KEYDOWN){
				switch (gc->msg.wParam){
				case VK_ESCAPE:
					PostQuitMessage(0);
					quit = true;
					break;
				case VK_V:
					vsync = !vsync;
					break;
				case VK_P:
					paused = !paused;
				case VK_UP:
					game.keyEvent(Snake::UP);
					break;
				case VK_DOWN:
					game.keyEvent(Snake::DOWN);
					break;
				case VK_LEFT:
					game.keyEvent(Snake::LEFT);
					break;
				case VK_RIGHT:
					game.keyEvent(Snake::RIGHT);
					break;
				}
			}else{
				TranslateMessage(&gc->msg);
				DispatchMessage(&gc->msg);
			}
		}else if (t0-t1 >= CLOCKS_PER_SEC/10 && !paused){
			t1 = clock();
			SetVSync(vsync);
			game.tick();
		}else{
			sprintf(clBuffer, "@C304848%d\0", game.field_.score);
			gc->OpenFBO(50, 0.0, 0.0, 40.0);
			gc->OrthogonalStart();
			{
				gc->Rectangle2D(0, 0, Field::WIDTH * Field::BLOCK_WIDTH, Field::HEIGHT * Field::BLOCK_WIDTH, KLGLColor(216,240,192));
				game.draw(*gc);
				text->Draw(0, 4, clBuffer);
			}
			gc->OrthogonalEnd();
			gc->Swap();
		}
	}
}
