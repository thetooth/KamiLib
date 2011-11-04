#include "kami.h"
#include "logicalObjects.h"
#include "calc.h"

#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#else
#pragma comment(lib,"kami.lib")
#endif

using namespace klib;
using namespace calc;

#define APP_NAME "Calc"
#define APP_BUCKET_SIZE 65792
#define APP_SCREEN_W 620
#define APP_SCREEN_H 208
#define APP_FRAMERATE 60
#define APP_SCALE_FACTOR 1

int main(){

	clock_t t0 = clock(), t1 = clock();
	int cycle = 0;
	bool quit = false, vsync = true, internalTimer = false;

	int textInput = 1;
	char inputBuffer[256];
	char* textBuffer = new char[4096];

	fill_n(inputBuffer, 256, '\0');
	fill_n(textBuffer, 4096, '\0');

	char err[255];
	double res = 0;

	// create a parser object
	Parser prs;
	char* parserRes;

	// kami
	KLGL* graphicsContext;
	KLGLTexture *charmap;
	KLGLFont* screen;

	try{
		// KamiLib ^D^
		graphicsContext = new KLGL(APP_NAME, APP_SCREEN_W, APP_SCREEN_H, APP_FRAMERATE, false, 1, APP_SCALE_FACTOR);
		screen = new KLGLFont();
	} catch(KLGLException e){
		quit = 1;
		MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
	}

	if (quit)
	{
		MessageBox(NULL, "Quit flag was set before the application begun, check the console for more info.", "Error", MB_OK | MB_ICONERROR);
	}
	t1 = (clock()-t0)/CLOCKS_PER_SEC;
	SetVSync(true);

	// Ready, hide teh console
	char path[260];
	GetModuleFileName(NULL,path,260);
	HWND console = FindWindow("ConsoleWindowClass", path);

	if(IsWindow(console)){
		ShowWindow(console,SW_HIDE); // hides the window
	}

	fill_n(clBuffer, 256, '\0');

	cl("Calc. (c) 2005-2011 Ameoto Systems Inc. All Rights Reserved.\n\n");
	cl("- Enter an expression and press Enter to calculate the result.\n");
	cl("- Enter an empty expression to quit.\n\n");

	while (!quit)
	{
		t0 = clock();
		// check for messages
		if(PeekMessage( &graphicsContext->msg, NULL, 0, 0, PM_REMOVE)){
			// handle or dispatch messages
			if(graphicsContext->msg.message == WM_QUIT){
				quit = true;
			}else if (graphicsContext->msg.message == WM_KEYDOWN){
				switch (graphicsContext->msg.wParam){
				case VK_ESCAPE:
					PostQuitMessage(0);
					quit = true;
					break;
				case VK_V:
					vsync = !vsync;
					SetVSync(vsync);
					break;
				case VK_T:
					internalTimer = !internalTimer;
					break;
				}

				//}else if(graphicsContext->msg.message = WM_CHAR){
				if(textInput){
					if ((graphicsContext->msg.wParam>=32) && (graphicsContext->msg.wParam<=255) && (graphicsContext->msg.wParam != '|')) {
						if (strlen(inputBuffer) < 255) {
							//cl("key: 0x%x\n", graphicsContext->msg.wParam);
							char buf[3];
							sprintf(buf, "%c\0", vktochar(graphicsContext->msg.wParam));
							strcat(inputBuffer, buf);

						}
					} else if (graphicsContext->msg.wParam == VK_BACK) {
						if(strlen(inputBuffer) > 0){
							inputBuffer[strlen(inputBuffer)-1]='\0';
						}
					} else if (graphicsContext->msg.wParam == VK_TAB) {
						inputBuffer[strlen(inputBuffer)]='\t';
					} else if (graphicsContext->msg.wParam == VK_RETURN) {
						if(strlen(inputBuffer) > 0){
							try{
								parserRes = prs.parse(inputBuffer);
								cl("%s\n", parserRes);
							}
							catch (...)
							{
								cl("Don't do that. You corrupt the stack.\n");
							}
							fill_n(inputBuffer, 256, '\0');
						}else{
							PostQuitMessage(0);
							quit = true;
						}
					}
				}
			}else{
				TranslateMessage(&graphicsContext->msg);
				DispatchMessage(&graphicsContext->msg);
			}
		}else if ((t0-t1 >= CLOCKS_PER_SEC/60.0f || !internalTimer) && graphicsContext->hWnd == GetActiveWindow()){
			t1 = clock();
			sprintf(textBuffer, "@CFFFFFF%s\n> %s%c", clBuffer, inputBuffer, (cycle%16 <= 4 ? '_' : '\0'));
			graphicsContext->OpenFBO(50, 0.0, 0.0, 80.0f);

			graphicsContext->OrthogonalStart();
			{
				// Background
				glBegin(GL_QUADS);
				glColor3ub(71,84,93);
				glVertex2i(0, 0); glVertex2i(APP_SCREEN_W, 0);
				glColor3ub(20,20,25);
				glVertex2f(APP_SCREEN_W, APP_SCREEN_H); glVertex2f(0, APP_SCREEN_H);
				glEnd();
				screen->Draw(8, 14, textBuffer);
			}
			graphicsContext->OrthogonalEnd();
			graphicsContext->Swap();
			cycle++;
		}else{
			//Sleep(500);
		}
	}
	delete graphicsContext;
	return 0;
}
