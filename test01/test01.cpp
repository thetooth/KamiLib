#include "kami.h"
#include "model.h"
//#include "cl.h"
#include "logicalObjects.h"
#include "particles.h"
#include "resource.h"

#include <algorithm>
#include <regex>
#include <unordered_map>
#include <ppl.h>

#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#else
#pragma comment(lib,"kami.lib")
#endif

using namespace std::tr1;
using namespace klib;

#define APP_NAME "Kami Multi-Threading Demo"
#define APP_BUCKET_SIZE 65792
#define APP_SCREEN_W 1280
#define APP_SCREEN_H 720
#define APP_FRAMERATE 60
#define APP_OVERSAMPLE_FACTOR 1

int main(){
	// Status's
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	virtualPageCommit(sysinfo);

	clock_t t0 = clock(), t1 = clock();
	bool quit = false, pPixelLight = true, vsync = true, internalTimer = false;
	int th_id, demoMode = 1;
	int nthreads = sysinfo.dwNumberOfProcessors-1;

	float theta = -360.0f, thetap = -360.0f;
	int textInput = 0;
	char textBuffer[256];
	POINT mouseXY;
	POINT mouseXYp;
	int mouseStat;

	fill_n(textBuffer, 256, '\0');

	// kami
	KLGL* gc;
	KLGLTexture *charmap, *testTexture;
	KLGLFont* font;
	MilkshapeModel *pModel;

	// Partitcle system
	LiquidParticleThread* particleThread = new LiquidParticleThread[nthreads];
	LiquidParticles particleTest = LiquidParticles(1000, &mouseXY, APP_SCREEN_W, APP_SCREEN_H);

	try{
		// KamiLib ^D^
		gc = new KLGL(APP_NAME, APP_SCREEN_W, APP_SCREEN_H, APP_FRAMERATE, false, APP_OVERSAMPLE_FACTOR);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		// Icon
		const HANDLE hsicon = LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		SendMessage(gc->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)const_cast<HANDLE>(hsicon));

		// Load Shaders
		quit = gc->InitShaders(1, 0, "common/postDefaultV.glsl", "common/postDefaultF.glsl");
		quit = gc->InitShaders(2, 0, "common/modelDefaultV.glsl", "common/modelDefaultF.glsl");
		quit = gc->InitShaders(3, 0, "common/postDefaultV.glsl", "common/gaussianBlur.frag");

		// Setup textures
		testTexture = new KLGLTexture("common/glory.png");

		font = new KLGLFont();

		// Load our model
		pModel = new MilkshapeModel();
		quit = pModel->loadModelData(*gc, "tree.ms3d");

		// Particles!
		if (nthreads <= 0)
		{
			throw KLGLException("YOU NEED AT LEAST 2 LOGICAL PROCCESSORS TO RUN THIS APPLICATION!");
		}
		particleTest.numThreads = nthreads;
		for (int i = 0; i < nthreads; i++){
			particleThread[i] = new LiquidParticleThread(&particleTest);
			particleThread[i].status = true;
		}
	} catch(KLGLException e){
		quit = 1;
		MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
	}

	if (quit)
	{
		MessageBox(NULL, "Quit flag was non-zero before entering the main loop.\nCheck the console for more info.", "Startup Error", MB_OK | MB_ICONERROR);
	}
	t1 = (clock()-t0)/CLOCKS_PER_SEC;
	SetVSync(true);

	// Ready, hide teh console
	char path[260];
	GetModuleFileName(NULL,path,260);
	HWND console = FindWindow("ConsoleWindowClass", path);

	if(IsWindow(console)){
		//ShowWindow(console,SW_HIDE); // hides the window
	}

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
				case VK_S:
					pPixelLight = !pPixelLight;
					break;
				case VK_V:
					vsync = !vsync;
					SetVSync(vsync);
					break;
				case VK_T:
					internalTimer = !internalTimer;
					break;
				case VK_P:
					particleTest.threadingState = !particleTest.threadingState;
					break;
				case VK_SPACE:
					if (demoMode < 2)
					{
						demoMode += 1;
					}else{
						demoMode = -1;
					}
					break;
				case VK_ADD:
					particleTest.audit(1000);
					break;
				case VK_SUBTRACT:
					particleTest.audit(-1000);
					break;
				}
				if(textInput){
					if ((gc->msg.wParam>=32) && (gc->msg.wParam<=126) && (gc->msg.wParam != '|')) {
						if (strlen(textBuffer) < 255) {
							char buf[3];
							sprintf(buf, "%c\0", gc->msg.wParam);
							strcat(textBuffer, buf);
						}
					} else if (gc->msg.wParam == VK_BACK) {
						if(strlen(textBuffer) > 0){
							textBuffer[strlen(textBuffer)-1]='\0';
						}
					} else if (gc->msg.wParam == VK_TAB) {
						textBuffer[strlen(textBuffer)]='\t';
					} else if (gc->msg.wParam == VK_RETURN) {
						if(strlen(textBuffer) > 0){
							textBuffer[strlen(textBuffer)]	='\n';
						}
					}
				}
			}else{
				TranslateMessage(&gc->msg);
				DispatchMessage(&gc->msg);
			}
		}else if (t0-t1 >= CLOCKS_PER_SEC/60.0f || !internalTimer){
			t1 = clock();
			if (gc->hWnd != GetActiveWindow())
			{
				Sleep(500);
				continue;
			}
			mouseXYp = mouseXY;
			GetCursorPos(&mouseXY);
			mouseStat = GetAsyncKeyState(VK_LBUTTON);
			ScreenToClient(gc->hWnd, &mouseXY);

			thetap = theta;
			theta += (cos(thetap-theta)/2.0f)-((theta*10.0f)/360.0f);
			theta += mouseXY.x/180.0f;

			

			glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
			gc->OpenFBO(50, 0.0, 0.0, 80.0f);

			switch(demoMode){
			case -1:
				sprintf(clBuffer, "@CFF0000== SWITCHING DEMO MODE... ================\n");
				demoMode = 0;
				particleTest.threadingState = 2;
				break;
			case 0:
				sprintf(clBuffer, "@CFFFFFF"\
					"== Model loading demo. ===========================\n"\
					"Rotate the model using your mouse, remember you   \n"\
					"can edit the model using MilkShape 3D!");

				glEnable(GL_LIGHTING);
				gc->BindShaders(2);

				glPushMatrix();
				{
					glTranslatef(0.0f, -220.0f, -400.0f);
					glRotatef( theta, 0.0f, 1.0f, 0.0f );
					pModel->draw();
				}
				glPopMatrix();
				gc->UnbindShaders();
				break;
			case 1:
				sprintf(clBuffer, "@CFFFFFF"\
					"== Multithreading Particles ======================\n"\
					"Move your mouse to influence particles. You can   \n"\
					"add more or less using the + and - keys, also try \n"\
					"clicking :P\n\n"\
					"Threads: %d (1 video, %d dedicated)\n"\
					"Particles: %d\n"\
					"Per Thread: %d", 
					nthreads+1, 
					nthreads, 
					particleTest.numMovers, 
					particleTest.numMovers/particleTest.numThreads
					);
				particleTest.threadingState = 1;
				gc->OrthogonalStart();
				{
					particleTest.isMouseDown = mouseStat;
					particleTest.mouseX = mouseXY.x;
					particleTest.mouseY = mouseXY.y;
					particleTest.draw(gc);
					gc->BindShaders(3);
					glUniform1f(glGetUniformLocation(gc->GetShaderID(3), "time"), gc->shaderClock);
					glUniform1f(glGetUniformLocation(gc->GetShaderID(3), "BLUR_BIAS"), 0.0001f);
					glUniform2f(glGetUniformLocation(gc->GetShaderID(3), "BUFFER_EXTENSITY"), gc->buffer_width, gc->buffer_height);
					gc->UnbindShaders();
					gc->BindMultiPassShader(3, 4);
				}
				gc->OrthogonalEnd();
				break;
			case 2:
				sprintf(clBuffer, "@CFFFFFF"\
					"== GUI Test ======================================\n"\
					""
					);
				gc->OrthogonalStart();
				{
					
				}
				gc->OrthogonalEnd();
				break;
			default:
				quit = 1;
			}
			gc->OrthogonalStart();
			{
				font->Draw(8, 14, clBuffer);
			}
			gc->OrthogonalEnd();
			gc->Swap();
		}
	}
	particleTest.threadingState = 0;
	delete gc;
	return 0;
}
