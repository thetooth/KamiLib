// Copyright 2005-2011 The Department of Redundancy Department, Dept.

#include "masternoodles.h"
#include "platformer.h"

#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#else
#pragma comment(lib,"kami.lib")
#endif

using namespace klib;
using namespace NeoPlatformer;

void Loading(KLGL *gc, KLGLTexture *texture);

int main(){
	char textBuffer[4096] = {};
	bool quit = false, internalTimer = false, mapScroll = false;
	int frame = 0,fps = 0,cycle = 0, th_id = 0, nthreads = 0, qualityPreset = 0;
	POINT mouseXY;
	clock_t t0 = 0,t1 = 0,t2 = 0;
	char wTitle[256] = {};

	enum GameMode {_MENU, _MAPLOAD, _MAPDESTROY, _DEV};
	int mode = 0;
	Environment* gameEnv;
	EnvLoaderThread* mapLoader;
	Point<int> *stars[1000] = {};

	/*
	* Init
	*/
	KLGL* gc;
	KLGLTexture* charmapTexture, *titleTexture, *loadingTexture, *backdropTexture, *gameoverTexture;
	KLGLSprite* mapSpriteSheet;
	KLGLFont* font;
	KLGLSound* soundTest;

	try {

		// Init display
		gc = new KLGL("Neo", APP_SCREEN_W, APP_SCREEN_H, 60, false, 2, 2);

		// Loading screen
		loadingTexture = new KLGLTexture("common/loading.png");
		Loading(gc, loadingTexture);

		// Configuration values
		internalTimer = gc->config->GetBoolean("neo", "useInternalTimer", false);
		qualityPreset = gc->config->GetInteger("neo", "qualityPreset", 1);

		// Textures
		charmapTexture = new KLGLTexture("common/charmap.png");		
		titleTexture = new KLGLTexture("common/title.png");
		backdropTexture = new KLGLTexture("common/clouds.png");
		gameoverTexture = new KLGLTexture("common/gameover.png");

		// Audio
		//soundTest = new KLGLSound;
		//soundTest->open(NeoSoundServer);

		// Fonts
		font = new KLGLFont(charmapTexture->gltexture, charmapTexture->width, charmapTexture->height, 6, 8, -1);

	} catch(KLGLException e){
		quit = 1;
		MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
	}

	luaopen_neo(gc->lua);
	int s = luaL_loadfile(gc->lua, "common/init.lua");
	if ( s==0 ) {
		// execute Lua program
		s = lua_pcall(gc->lua, 0, LUA_MULTRET, 0);
	}
	gc->LuaCheckError(gc->lua, s);

	if (quit)
	{
		MessageBox(NULL, "An important resource is missing or failed to load, check the console window _now_ for more details.", "Error", MB_OK | MB_ICONERROR);
	}

	while (!quit)
	{
		t0 = clock();
		if(PeekMessage( &gc->msg, NULL, 0, 0, PM_REMOVE)){
			if(gc->msg.message == WM_QUIT){
				quit = true;
			}else if (gc->msg.message == WM_KEYUP){
				switch (gc->msg.wParam){
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
			}else if (gc->msg.message == WM_KEYDOWN){
				// Application wide events
				switch (gc->msg.wParam){
				case VK_ESCAPE:
					PostQuitMessage(0);
					quit = true;
					break;
				case VK_D:
					KLGLDebug = !KLGLDebug;
					break;
				}

				// Context sensitive
				switch(mode){
				case GameMode::_MENU:
					switch (gc->msg.wParam){
					case VK_RETURN:
						mode = GameMode::_MAPLOAD;
						break;
					}
					break;
				case GameMode::_DEV:
					switch (gc->msg.wParam){
					case VK_RETURN:
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
						delete gameEnv->character;
						gameEnv->character = new Character(128, 0, 8, 16);
						break;
					case VK_M:
						mapScroll = !mapScroll;
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
				TranslateMessage(&gc->msg);
				DispatchMessage(&gc->msg);
			}
		}else if (t0-t2 >= CLOCKS_PER_SEC){
			fps = frame;
			frame = 0;
			t2 = clock();

			// Title
			sprintf(wTitle, "Neo - FPS: %d", fps);
			SetWindowText(gc->hWnd, wTitle);

			// Update graphics quality
			gc->BindShaders(1);
			glUniform1i(glGetUniformLocation(gc->GetShaderID(1), "preset"), qualityPreset);
			gc->UnbindShaders();
		}else if (t0-t1 >= CLOCKS_PER_SEC/50.0f || !internalTimer){

			GetCursorPos(&mouseXY);
			ScreenToClient(gc->hWnd, &mouseXY);

			switch(mode){
			case GameMode::_MENU:											// ! Start menu

				sprintf(textBuffer, 
					"@CFFFFFFNeo %s R%d, Press return to continue.", 
					APP_VERSION, APP_BUILD_VERSION
					);

				// Update clock
				t1 = clock();

				gc->OpenFBO(50, 0.0, 0.0, 80.0);
				gc->OrthogonalStart();
				{
					gc->Blit2D(titleTexture, 0, 0);
					font->Draw(8, APP_SCREEN_H-22, textBuffer);
				}
				gc->OrthogonalEnd();

				break;
			case GameMode::_MAPLOAD:										// ! Threaded map loader :D

				cl("\nNew game...\n");
				try{
					// Shutdown any sound the menu mite be playing
					//soundTest->close();
					//soundTest->open(NeoSoundServer);
					// Initialize the game engine
					gameEnv = new Environment();
					gameEnv->platforms = new list<Platform>;
					// Setup a thread to load and parse our map(if the map is large we want to be able to check its progress).
					mapLoader = new EnvLoaderThread(gameEnv);
					//soundTest->close();

					// Enter a tight loop of no return :D
					while(mapLoader->status){
						// Read latest console buffer
						sprintf(textBuffer, "@CFFFFFF%s", clBuffer);

						gc->OrthogonalStart();
						{
							glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
							gc->Blit2D(loadingTexture, 0, 0);
							font->Draw(8, 14, textBuffer);
						}
						gc->OrthogonalEnd();
						SwapBuffers(gc->hDC);
						// No need to rush
						Sleep(100);
					}
					gameEnv->character = new Character(128, 0, 8, 16);
					for (int i = 0; i < 100; i++)
					{
						stars[i] = new Point<int>;
						stars[i]->x = rand()%(gameEnv->map.width*16+1);
						stars[i]->y = rand()%(gameEnv->map.height*16+1);
					}
					// Map sprites
					mapSpriteSheet = new KLGLSprite("common/tilemap.png", 16, 16);
					delete mapLoader;
					gc->InitShaders(1, 
						"common/postDefaultV.glsl",
						"common/productionfrag.glsl"
						);
					gc->InitShaders(2, 
						"common/tile.vert",
						"common/tile.frag"
						);
					mode = GameMode::_DEV;
				}catch(KLGLException e){
					MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
					mode = GameMode::_MENU;
				}

				cl("\nGame created successfully.\n");

				break;
			case GameMode::_MAPDESTROY:

				cl("Unloading game...");
				try{
					delete gameEnv;
					delete mapSpriteSheet;
					gc->UnloadShader(1);
				}catch(KLGLException e){
					MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
					mode = GameMode::_MENU;
				}

				mode = GameMode::_MENU;
				cl("[OK]\n");

				break;
			case GameMode::_DEV:											// ! Main game test environment

				// Compute game physics and update events
				gameEnv->dt = double(t0 - t1)/1000.0;
				gameEnv->comp((mapScroll ? mouseXY.x - gc->buffer_width / 2 : 0), (mapScroll ? mouseXY.y - gc->buffer_height / 2 : 0));

				// Debug info
				if (KLGLDebug)
				{
					sprintf(textBuffer, "@CFFFFFF%s \n\nNeo\n-------------------------\n"\
						"Render FPS: %d\n"\
						"       Syn: V%dT%d\n"\
						"       Buf: %dx%d\n"\
						"       Acu: %f (less is more)\n\n"\
						"Player Pos: %f,%f\n"\
						"       Vel: %f,%f\n\n"\
						"Map    Pos: %d,%d\n"\
						"       Rel: %d,%d >> %d\n"\
						"       Tar: %d,%d >> %d\n\n"\
						"Debug  act: %d\n"\
						"       db0: %d\n"\
						"       db1: %d\n", 
						clBuffer,
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
				// Update clock
				t1 = clock();

				// BEGIN DRAWING!
				gc->OpenFBO(50, 0.0, 0.0, 80.0);
				gc->OrthogonalStart();
				{
					// Background
					glBegin(GL_QUADS);
					glColor3ub(71,84,93);
					glVertex2i(0, 0); glVertex2i(APP_SCREEN_W, 0);
					glColor3ub(214,220,214);
					glVertex2i(APP_SCREEN_W, APP_SCREEN_H); glVertex2i(0, APP_SCREEN_H);
					glEnd();

					// Barell rotation
					//glTranslatef(APP_SCREEN_W/2.0f, APP_SCREEN_H/2.0f, 0.0f);
					//glRotatef(t1/10.0f, 0.0f, 0.0f, 1.0f );
					//glTranslatef(-APP_SCREEN_W/2.0f, -APP_SCREEN_H/2.0f, 0.0f);
					
					// Stars
					for (int i = 0; i < 100; i++)
					{
						int x = (stars[i]->x-240)-(gameEnv->scroll.x/6);
						int y = (stars[i]->y-240)-(gameEnv->scroll.y/6);
						if(x < 0 || x > APP_SCREEN_W || y < 0 || y > APP_SCREEN_H){
							continue;
						}
						gc->Rectangle2D(x, y, 1, 1, KLGLColor(255, 255, 255, 127));
					}

					// Reset pallet
					KLGLColor(255, 255, 255, 255).Set();

					// Cloud
					gc->Blit2D(backdropTexture, (APP_SCREEN_W/2)-(gameEnv->scroll.x/4), (APP_SCREEN_H/3)-(gameEnv->scroll.y/4));

					gameEnv->drawMap(gc, mapSpriteSheet);

					// Player and HUD
					gameEnv->character->draw(gameEnv, gc, mapSpriteSheet, frame);
					gameEnv->drawHUD(gc, gameoverTexture);

					if (KLGLDebug)
					{
						glViewport(0, gc->buffer_height-APP_SCREEN_H, APP_SCREEN_W, APP_SCREEN_H);
						font->Draw(8, 14, textBuffer);
					}
				}
				gc->OrthogonalEnd();
				break;
			}
			gc->Swap();
			frame++;
			cycle++;
		}

	}
	delete gc;
	return 0;
}

void Loading(KLGL *gc, KLGLTexture *texture){
	gc->OrthogonalStart();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	gc->Blit2D(texture, 0, 0);
	gc->OrthogonalEnd();
	SwapBuffers(gc->hDC);
}