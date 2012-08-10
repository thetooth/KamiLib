// Copyright 2005-2011 The Department of Redundancy Department.

#include "masternoodles.h"
#include "platformer.h"
#include "console.h"

#ifdef _WIN32
#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#pragma comment(lib,"fmodex64_vc.lib")
#pragma comment(lib,"UI64.lib")
#else
#pragma comment(lib,"kami.lib")
#pragma comment(lib,"fmodex_vc.lib")
#pragma comment(lib,"python3.lib")
#pragma comment(lib,"UI.lib")
#endif
#endif

using namespace klib;
using namespace NeoPlatformer;

void Loading(KLGL *gc, KLGLTexture *loading);
void Warning(KLGL *gc, char *errorMessage, KLGLTexture *warning = nullptr);

int main(int argc, wchar_t **argv){
	int consoleInput = 0;
	char inputBuffer[256] = {};
	char textBuffer[4096] = {};
	bool quit = false, internalTimer = false, mapScroll = false;
	int frame = 0,fps = 0,cycle = 0, th_id = 0, nthreads = 0, qualityPreset = 0, shaderAlliterations = 0;
	float tweenX = 0, tweenY = 0, titleFade = 0;
	POINT mouseXY;
	clock_t t0 = clock(),t1 = 0,t2 = 0;
	char wTitle[256] = {};

	enum GameMode {_MENU, _MAPLOAD, _MAPDESTROY, _INGAME};
	int mode = 0, paused = 0;
	Environment *gameEnv;
	EnvLoaderThread* mapLoader;
	Point<int> *stars[1000] = {};

	Console *console;
	UIDialog *dialog;
	LiquidParticleThread* particleThread[4];
	LiquidParticles particleTest = LiquidParticles(1000, &mouseXY, APP_SCREEN_W, APP_SCREEN_H);

	tween::Tweener tweener;
	tween::TweenerParam titleFaderTween(2000, tween::EXPO, tween::EASE_OUT);

	// Audio
	Audio *audio = NULL;

	/*
	* Init
	*/
	KLGL *gc;
	KLGLTexture *charmapTexture, *titleTexture, *loadingTexture, *testTexture, *startupTexture, *warningTexture;
	KLGLSprite *loaderSprite;
	KLGLFont *font;

	t0 = clock();

	try {

		// Init display
		gc = new KLGL("Neo", APP_SCREEN_W, APP_SCREEN_H, 60, false, 2, 2);

		// Python
		PyImport_AppendInittab("neo", PyInit_neo);
		//Py_SetProgramName(argv[0]);
		Py_Initialize();
		PyRun_SimpleString("import platform; import neo; neo.cl(\"Initialized Python {0}\\n\".format(platform.python_version()))");

		// Loading screen and menu buffer
		warningTexture = new KLGLTexture("common/warning.png");
		startupTexture = new KLGLTexture("common/startup.png");

		// Init sound
		audio = new Audio();
		audio->loadSound("common/startup.wav", 10, FMOD_LOOP_OFF);
		audio->loadSound("common/title-outoflimits.mp3", 0, FMOD_LOOP_NORMAL);

		// :3
		Loading(gc, startupTexture);
		audio->system->playSound(FMOD_CHANNEL_FREE, audio->sound[10], false, &audio->channel[0]);

		// Configuration values
		internalTimer		= gc->config->GetBoolean("neo", "useInternalTimer", true);
		qualityPreset		= gc->config->GetInteger("neo", "qualityPreset", 1);
		shaderAlliterations	= gc->config->GetInteger("neo", "blurAlliterations", 4);

		// Textures
		charmapTexture  = new KLGLTexture("common/internalfont.png");
		testTexture = new KLGLTexture("common/out[composite].png");
		loadingTexture = new KLGLTexture("common/loading.png");
		titleTexture = new KLGLTexture("common/neo-logo.png");

		// Sprites
		loaderSprite = new KLGLSprite(testTexture, 64, 64);

		// UI
		dialog = new UIDialog(gc, "common/UI_Interface.png");
		dialog->bgColor = new KLGLColor(51, 51, 115, 255);
		dialog->pos.width = 512;
		dialog->pos.height = 256;
		dialog->pos.x = (gc->buffer.width/2)-(dialog->pos.width/2)-8;
		dialog->pos.y = (gc->buffer.height/2)-(dialog->pos.height/2)-8;

		// Fonts
		font = new KLGLFont(charmapTexture->gltexture, charmapTexture->width, charmapTexture->height, 8, 8, -1);

		// Game Console
		console = new Console(4096);

		// Default shaders
		gc->GenFrameBuffer(gc->fbo[2], gc->fbo_texture[2], gc->fbo_depth[2]);
		gc->GenFrameBuffer(gc->fbo[3], gc->fbo_texture[3], gc->fbo_depth[3]);
		gc->InitShaders(0, 0, 
			"common/passthrough.vert",
			"common/passthrough.frag"
			);
		gc->InitShaders(4, 0, 
			"common/model.vert",
			"common/model.frag"
			);
		gc->InitShaders(5, 0, 
			"common/postDefaultV.glsl",
			"common/waves.frag"
			);

		// Particles
		particleTest.numThreads = 4;
		for (int i = 0; i < particleTest.numThreads; i++){
			particleThread[i] = new LiquidParticleThread(&particleTest);
			particleThread[i]->status = false;
		}

	}catch(KLGLException e){
		quit = 1;
		Warning(gc, e.getMessage(), warningTexture);
	}

	cl("\nNeo %s R%d", NEO_VERSION, APP_BUILD_VERSION);

	if (quit){
		MessageBox(NULL, "An important resource is missing or failed to load, check the console window _now_ for more details.", "Error", MB_OK | MB_ICONERROR);
#ifdef KLGLDebug
		quit = 0;
#endif
	}

	while (clock()-t0 < 2500 && !KLGLDebug){
		// Ain't no thing
		Sleep(10);
	}

	audio->system->playSound(FMOD_CHANNEL_FREE, audio->sound[0], false, &audio->channel[1]);
	audio->channel[1]->setVolume(0.5f);

	tweener.step(clock());
	titleFaderTween.addProperty(&titleFade, 255);
	tweener.addTween(&titleFaderTween);

	while (!quit)
	{
		t0 = clock();

#pragma region Input events

		if(PeekMessage( &gc->windowManager->wm->msg, NULL, 0, 0, PM_REMOVE)){
			if(gc->windowManager->wm->msg.message == WM_QUIT){
				quit = true;
			}else if (gc->windowManager->wm->msg.message == WM_KEYUP){
				switch(mode){
				case GameMode::_INGAME:
					switch (gc->windowManager->wm->msg.wParam){
					case VK_LEFT:
						gameEnv->character->leftUp();
						break;
					case VK_RIGHT:
						gameEnv->character->rightUp();
						break;
					case VK_UP:
						gameEnv->character->jumpUp();
						break;
					}
					break;
				}
			}else if (gc->windowManager->wm->msg.message == WM_KEYDOWN){
				// Application wide events
				switch (gc->windowManager->wm->msg.wParam){
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;
				case VK_D:
					KLGLDebug = !KLGLDebug;
					break;
				}

				// Context sensitive
				switch(mode){
				case GameMode::_MENU:
					if(consoleInput){
						if ((gc->windowManager->wm->msg.wParam>=32) && (gc->windowManager->wm->msg.wParam<=255) && (gc->windowManager->wm->msg.wParam != '|')) {
							if (strlen(inputBuffer) < 255) {
								//cl("key: 0x%x\n", graphicsContext->msg.wParam);
								char buf[3];
								sprintf(buf, "%c\0", vktochar(gc->windowManager->wm->msg.wParam));
								strcat(inputBuffer, buf);

							}
						} else if (gc->windowManager->wm->msg.wParam == VK_BACK) {
							if(strlen(inputBuffer) > 0){
								inputBuffer[strlen(inputBuffer)-1]='\0';
							}
						} else if (gc->windowManager->wm->msg.wParam == VK_TAB) {
							inputBuffer[strlen(inputBuffer)]='\t';
						} else if (gc->windowManager->wm->msg.wParam == VK_RETURN) {
							if(strlen(inputBuffer) > 0){
								if (strcmp(inputBuffer, "exit") == 0){
									PostQuitMessage(0);
									quit = true;
								}else{
									console->input(inputBuffer);
								}
							}else{
								cl("-- RETURN\n", 13);
								mode = GameMode::_MAPLOAD;
							}
							fill_n(inputBuffer, 256, '\0');
						}
					} else if (gc->windowManager->wm->msg.wParam == VK_RETURN) {
						particleTest.threadingState = 2; // This pauses the simulation
						mode = GameMode::_MAPLOAD;
					}
					break;
				case GameMode::_INGAME:
					switch (gc->windowManager->wm->msg.wParam){
					case VK_RETURN:
						particleTest.threadingState = 1;
						mode = GameMode::_MAPDESTROY;
						break;
					case VK_T:
						internalTimer = !internalTimer;
						break;
					case VK_V:
						gc->vsync = !gc->vsync;
						SetVSync(gc->vsync);
						break;
					case VK_G:
						if (qualityPreset < 3)
						{
							qualityPreset++;
						}else{
							qualityPreset = 0;
						}
						break;
					case VK_R:
						mode = GameMode::_MAPLOAD;
						break;
					case VK_M:
						mapScroll = !mapScroll;
						break;
					case VK_P:
						if (!paused){
							titleFade = 0.0f;
							titleFaderTween.addProperty(&titleFade, 255);
							tweener.addTween(&titleFaderTween);
						}else{
							titleFade = 0.0f;
							tweener.removeTween(&titleFaderTween);
						}
						paused = !paused;
						break;
					case VK_LEFT:
						gameEnv->character->left();
						break;
					case VK_RIGHT:
						gameEnv->character->right();
						break;
					case VK_UP:
						gameEnv->character->jump();
						break;
					}
					break;
				}
			}else{
				TranslateMessage(&gc->windowManager->wm->msg);
				DispatchMessage(&gc->windowManager->wm->msg);
			}

#pragma endregion

		}else if (t0-t2 >= CLOCKS_PER_SEC){
			fps = frame;
			frame = 0;
			t2 = clock();

			// Title
			sprintf(wTitle, "Neo - FPS: %d", fps);
			SetWindowText(gc->windowManager->wm->hWnd, wTitle);

			// Update graphics quality
			gc->BindShaders(1);
			glUniform1i(glGetUniformLocation(gc->GetShaderID(1), "preset"), qualityPreset);
			gc->UnbindShaders();
		}else if (t0-t1 >= CLOCKS_PER_SEC/gc->fps || !internalTimer){

			GetCursorPos(&mouseXY);
			ScreenToClient(gc->windowManager->wm->hWnd, &mouseXY);

			audio->system->update();

			switch(mode){
			case GameMode::_MENU:											// ! Start menu

				sprintf(textBuffer, "@CFFFFFF@D%s\n\n%s\n> %s%c", clBuffer, console->buffer, inputBuffer, (frame%16 <= 4 ? '_' : '\0'));

				// Update clock
				tweener.step(t0);
				t1 = clock();

				gc->OpenFBO(45, 0.0, 0.0, 0.0);
				gc->OrthogonalStart();
				{
					// Reset pallet
					KLGLColor(255, 255, 255, 255).Set();

					gc->OrthogonalStart();

					gc->OrthogonalStart(gc->overSampleFactor/1.0f);
					gc->BindShaders(5);
					glUniform1f(glGetUniformLocation(gc->GetShaderID(5), "time"), cycle/40.0f);
					glUniform2f(glGetUniformLocation(gc->GetShaderID(5), "resolution"), gc->window.width*(gc->overSampleFactor/1.0f), gc->window.height*(gc->overSampleFactor/1.0f));
					gc->BindMultiPassShader(5, 1, false);
					gc->UnbindShaders();

					//drawSinWave(mouseXY.x/10.0f);

					gc->Blit2D(titleTexture, 0, 0);
					gc->BlitSprite2D(loaderSprite, 0, 0, cycle);

					// Draw console
					//gc->OrthogonalStart(gc->overSampleFactor);
					//font->Draw(0, 8, textBuffer);

					// Fader
					if (titleFade < 254){
						gc->Rectangle2D(0, 0, gc->buffer.width, gc->buffer.height, KLGLColor(0, 0, 0, int(255-titleFade)));
					}
				}
				gc->OrthogonalEnd();
				gc->Swap();
				break;
			case GameMode::_MAPLOAD:										// ! Threaded map loader :D

				cl("\nNew game...\n");
				try{
					// Shutdown any sound the menu mite be playing
					audio->channel[0]->stop();
					audio->channel[1]->stop();
					// Initialize the game engine
					gameEnv = new Environment("common/map01");
					gameEnv->gcProxy = gc;
					gameEnv->audioProxy = audio;
					// Pass-through our font
					gameEnv->hudFont = font;
					// Setup a thread to load and parse our map(if the map is large we want to be able to check its progress).
					mapLoader = new EnvLoaderThread(gameEnv);

					cl("Loading thread %d started.\n", GetThreadId(mapLoader->getHandle()));

					// Enter a tight loop of no return :D
					while(mapLoader->status){
						// Read latest console buffer
						sprintf(textBuffer, "@CFFFFFF%s", clBuffer);

						gc->OrthogonalStart();
						glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
						gc->Blit2D(loadingTexture, 0, 0);
						gameEnv->drawLoader(gc, gc->buffer.height-(24*gc->overSampleFactor), 1*gc->overSampleFactor, 100*gc->overSampleFactor, 16*gc->overSampleFactor);
						if (KLGLDebug){
							font->Draw(8, 14, textBuffer);
						}
						gc->OrthogonalEnd();
						SwapBuffers(gc->windowManager->wm->hDC);
					}

					// Setup initial post quality
					gc->BindShaders(1);
					glUniform1i(glGetUniformLocation(gc->GetShaderID(1), "preset"), qualityPreset);
					gc->UnbindShaders();

					delete mapLoader;

					// Misc
					for (int i = 0; i < 100; i++)
					{
						stars[i] = new Point<int>;
						stars[i]->x = rand()%(gameEnv->map.width*16+1);
						stars[i]->y = rand()%(gameEnv->map.height*16+1);
					}

					//audio->system->playSound(FMOD_CHANNEL_FREE, audio->sound[1], false, &audio->channel[0]);
					audio->system->playSound(FMOD_CHANNEL_FREE, audio->sound[4], false, &audio->channel[0]);
					audio->channel[0]->setVolume(0.15f);

					mode = GameMode::_INGAME;
				}catch(KLGLException e){
					MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
					mode = GameMode::_MENU;
				}

				cl("\nGame created successfully.\n");

				break;
			case GameMode::_MAPDESTROY:

				cl("Unloading game...");
				try{
					// Audio
					audio->channel[0]->stop();
					// Destroy the game
					delete gameEnv;
					gc->UnloadShaders(1);
					gc->UnloadShaders(2);
					gc->UnloadShaders(3);

					audio->system->playSound(FMOD_CHANNEL_FREE, audio->sound[0], false, &audio->channel[0]);
					audio->channel[0]->setVolume(0.5f);
				}catch(KLGLException e){
					MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
					mode = GameMode::_MENU;
				}

				mode = GameMode::_MENU;
				cl(" [OK]\n");

				break;
			case GameMode::_INGAME:											// ! Main game test environment

				// Update clock
				gameEnv->dt = (t0 - t1)/float(CLOCKS_PER_SEC);
				tweener.step(t0);
				t1 = clock();

				// BEGIN DRAWING!
				gc->OpenFBO(50, 0.0, 0.0, 80.0);
				gc->OrthogonalStart();
				{
					if (paused){
						if (titleFade < 255){
							glBindTexture(GL_TEXTURE_2D, gc->fbo_texture[1]);
							glBegin(GL_QUADS);
							glTexCoord2d(0.0,0.0); glVertex2i(0,				0);
							glTexCoord2d(1.0,0.0); glVertex2i(gc->buffer.width,	0);
							glTexCoord2d(1.0,1.0); glVertex2i(gc->buffer.width,	gc->buffer.height);
							glTexCoord2d(0.0,1.0); glVertex2i(0,				gc->buffer.height);
							glEnd();
							glBindTexture(GL_TEXTURE_2D, 0);
						}
						gc->OrthogonalStart(gc->overSampleFactor/2.0f);
						static int scrollOffset;
						if (scrollOffset == NULL || scrollOffset > 16){
							scrollOffset = 0;
						}
						glColor4ub(255, 255, 255, titleFade);
						for (int y = 0; y-1 <= gc->buffer.height/32; y++)
						{
							for (int x = 0; x-1 <= gc->buffer.width/32; x++)
							{
								gc->BlitSprite2D(gameEnv->hudSpriteSheet16, (x*16)-scrollOffset, (y*16)-scrollOffset, 0);
							}
						}
						scrollOffset++;
						glColor4ub(255, 255, 255, 255);

						gc->OrthogonalStart();

						// Pause menu
						dialog->comp();
						dialog->draw();


					}else{
						// Compute game physics and update events
						gameEnv->comp(gc, (mapScroll ? mouseXY.x - gc->buffer.width / 2 : 0), (mapScroll ? mouseXY.y - gc->buffer.height / 2 : 0));

						gameEnv->debugflags.x = mouseXY.x;
						gameEnv->debugflags.y = mouseXY.y;

						PyRun_SimpleString("neo.set(\"gameEnv.scrollX\", 10)");

						// Debug info
						if (KLGLDebug)
						{
							sprintf(textBuffer, 
								"@CF8F8F8@DNeo\n-------------------------\n"\
								"Render FPS: %d\n"\
								"       Syn: V%dT%d\n"\
								"       Buf: %dx%d\n"\
								"       Acu: %f (less is more)\n\n"\
								"Player Pos: %f,%f\n"\
								"       Vel: %f,%f\n\n"\
								"Map    Pos: %d,%d\n"\
								"       Rel: %d,%d >> %d\n"\
								"       Tar: %d,%d >> %d\n\n"\
								"Debug  Act: %d\n"\
								"       DB0: %d\n"\
								"       DB1: %d\n", 
								fps,
								gc->vsync,
								internalTimer,
								APP_SCREEN_W,
								APP_SCREEN_H,
								gameEnv->dt,
								gameEnv->character->pos.x, 
								gameEnv->character->pos.y, 
								gameEnv->character->vel.x,
								gameEnv->character->vel.y,
								gameEnv->scroll.x, 
								gameEnv->scroll.y,
								gameEnv->target.x,
								gameEnv->target.y, FPM,
								gameEnv->scroll_real.x,
								gameEnv->scroll_real.y, FPM,
								KLGLDebug,
								gameEnv->debugflags.x,
								gameEnv->debugflags.y
								);
						}

						// Scaling
						//glScalef(float(gc->window.width)/float(APP_SCREEN_W), float(gc->window.width)/float(APP_SCREEN_W), 0.0);

						// Background
						int horizon = max(gc->buffer.width, gc->buffer.height+(-gameEnv->scroll.y));
						int stratosphere = min(0, -gameEnv->scroll.y);
						glBegin(GL_QUADS);
						glColor3ub(71,84,93);
						glVertex2i(0, stratosphere); glVertex2i(gc->buffer.width, stratosphere);
						glColor3ub(214,220,214);
						glVertex2i(gc->buffer.width, horizon); glVertex2i(0, horizon);
						glEnd();

						// Barell rotation
						//glTranslatef(gc->window.width/gc->scaleFactor, gc->window.height/gc->scaleFactor, 0.0f);
						//glRotatef(mouseXY.x/10.0f, 0.0f, 0.0f, 1.0f );
						//glTranslatef(-gc->window.width/gc->scaleFactor, -gc->window.height/gc->scaleFactor, 0.0f);

						// Stars
						for (int i = 0; i < 100; i++)
						{
							int x = (stars[i]->x-240)-(gameEnv->scroll.x/6);
							int y = (stars[i]->y-240)-(gameEnv->scroll.y/6);
							if(x < 0 || x > APP_SCREEN_W || y < 0 || y > APP_SCREEN_H){
								continue;
							}
							gc->Rectangle2D(x, y, 1, 1, KLGLColor(255, 255, 255, 80+rand()%47));
						}

						// Reset pallet
						KLGLColor(255, 255, 255, 255).Set();

						// Cloud
						gc->Blit2D(gameEnv->backdropTexture, (APP_SCREEN_W/2)-(gameEnv->scroll.x/4), (APP_SCREEN_H/3)-(gameEnv->scroll.y/4));

						// Draw Map
						gameEnv->drawMap(gc);

						// Draw Player & Enemys
						gameEnv->character->draw(gameEnv, gc, gameEnv->mapSpriteSheet, frame);
						for (auto enemy = gameEnv->enemys.begin(); enemy != gameEnv->enemys.end(); enemy++){
							(*enemy)->draw(gc, gameEnv->mapSpriteSheet, frame);
						}

						// Lighting effects
						gc->BindShaders(4);
						glUniform1f(glGetUniformLocation(gc->GetShaderID(4), "time"), gc->shaderClock);
						glUniform2f(glGetUniformLocation(gc->GetShaderID(4), "resolution"), gc->buffer.width/gc->overSampleFactor, gc->buffer.height/gc->overSampleFactor);
						glUniform2f(glGetUniformLocation(gc->GetShaderID(4), "position"), (gameEnv->scroll.x-(56*16))/1.0f, (gameEnv->scroll.y-(27*16))/1.0f);
						glUniform1f(glGetUniformLocation(gc->GetShaderID(4), "radius"), 140.0f);
						glUniform1f(glGetUniformLocation(gc->GetShaderID(4), "blendingDivision"), 5.0f+sin(cycle/16.0f));
						gc->BindMultiPassShader(4, 1, false);
						gc->UnbindShaders();

						// Adv Bloom
						gc->BindShaders(3);
						glUniform1i(glGetUniformLocation(gc->GetShaderID(3), "image"), 0);
						glUniform1i(glGetUniformLocation(gc->GetShaderID(3), "slave"), 1);
						glUniform1f(glGetUniformLocation(gc->GetShaderID(3), "time"), gc->shaderClock);
						glUniform2f(glGetUniformLocation(gc->GetShaderID(3), "resolution"), gc->buffer.width/gc->overSampleFactor, gc->buffer.height/gc->overSampleFactor);
						gc->BindMultiPassShader(3, 1, false);
						gc->UnbindShaders();

						/*gc->BindShaders(2);
						glUniform1i(glGetUniformLocation(gc->GetShaderID(2), "image"), 0);
						glUniform1i(glGetUniformLocation(gc->GetShaderID(2), "slave"), 1);
						glUniform1f(glGetUniformLocation(gc->GetShaderID(2), "time"), gc->shaderClock);
						glUniform2f(glGetUniformLocation(gc->GetShaderID(2), "resolution"), gc->buffer.width/gc->overSampleFactor, gc->buffer.height/gc->overSampleFactor);
						gc->BindMultiPassShader(2, 1, false);
						gc->UnbindShaders();*/

						// Master post shader data
						gc->BindShaders(1);
						glUniform1f(glGetUniformLocation(gc->GetShaderID(1), "time"), gc->shaderClock);
						glUniform2f(glGetUniformLocation(gc->GetShaderID(1), "resolution"), gc->buffer.width/gc->overSampleFactor, gc->buffer.height/gc->overSampleFactor);
						gc->BindMultiPassShader(1, 1, false);
						gc->UnbindShaders();

						// HUD, Score, Health, etc
						gameEnv->drawHUD(gc);

						if (KLGLDebug){
							gc->OrthogonalStart(gc->overSampleFactor);
							font->Draw(0, 8, textBuffer);
						}
					}
				}
				gc->OrthogonalEnd();
				gc->Swap();
				break;
			}
			frame++;
			cycle++;
		}

	}
	particleTest.threadingState = 0;
	for (int i = 0; i < particleTest.numThreads; i++){
		particleThread[i]->stop();
		delete particleThread[i];
	}
	delete gc;
	return 0;
}

void Loading(KLGL *gc, KLGLTexture *loading)
{
	gc->OpenFBO();
	gc->OrthogonalStart();
	{
		// Draw loading screen
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gc->fbo[0]);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		gc->Blit2D(loading, 0, 0);
	}
	gc->OrthogonalEnd();
	gc->Swap();
}

void Warning(KLGL *gc, char *errorMessage, KLGLTexture *warning){
	KLGLFont *font = new KLGLFont();
	gc->OpenFBO();
	gc->OrthogonalStart();
	{
		if (warning != nullptr){
			gc->Blit2D(warning, 0, 0);
		}
		font->Draw(0, 42, errorMessage);
	}
	gc->OrthogonalEnd();
	gc->Swap();
}
