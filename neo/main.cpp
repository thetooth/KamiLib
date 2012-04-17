// Copyright 2005-2011 The Department of Redundancy Department.

#include "masternoodles.h"
#include "platformer.h"

#ifdef KLGLENV64
#pragma comment(lib,"kami64.lib")
#else
#pragma comment(lib,"kami.lib")
#endif

using namespace klib;
using namespace NeoPlatformer;

void Loading(KLGL *gc, KLGLTexture *loading, KLGLTexture *splash);

/*
void call_foo(HSQUIRRELVM v, int n,float f,const SQChar *s)
{
	int top = sq_gettop(v); //saves the stack size before the call
	sq_pushroottable(v); //pushes the global table
	sq_pushstring(v,_SC("foo"),-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) { //gets the field 'foo' from the global table
		sq_pushroottable(v); //push the 'this' (in this case is the global table)
		sq_pushinteger(v,n); 
		sq_pushfloat(v,f);
		sq_pushstring(v,s,-1);
		sq_call(v,4,0,0); //calls the function 
	}
	sq_settop(v,top); //restores the original stack size
}*/

int main(){
	int consoleInput = 1;
	char inputBuffer[256] = {};
	char textBuffer[4096] = {};
	bool quit = false, internalTimer = false, mapScroll = false;
	int frame = 0,fps = 0,cycle = 0, th_id = 0, nthreads = 0, qualityPreset = 0, shaderAlliterations = 0;
	POINT mouseXY;
	clock_t t0 = 0,t1 = 0,t2 = 0;
	char wTitle[256] = {};

	enum GameMode {_MENU, _MAPLOAD, _MAPDESTROY, _INGAME};
	int mode = 0;
	Environment *gameEnv;
	EnvLoaderThread* mapLoader;
	Point<int> *stars[1000] = {};

	/*
	* Init
	*/
	KLGL *gc;
	KLGLTexture *charmapTexture, *titleTexture, *loadingTexture, *backdropTexture, *gameoverTexture, *userInterfaceTexture;
	KLGLSprite *userInterface;
	KLGLFont *font;
	KLGLSound *soundTest;

	try {

		// Init display
		gc = new KLGL("Neo", APP_SCREEN_W, APP_SCREEN_H, 60, false, 2, 2);

		// Loading screen and menu buffer
		loadingTexture = new KLGLTexture("common/loading.png");
		titleTexture = new KLGLTexture("common/title.png");
		Loading(gc, loadingTexture, titleTexture);

		// Configuration values
		internalTimer		= gc->config->GetBoolean("neo", "useInternalTimer", false);
		qualityPreset		= gc->config->GetInteger("neo", "qualityPreset", 1);
		shaderAlliterations	= gc->config->GetInteger("neo", "blurAlliterations", 4);

		// Textures
		charmapTexture  = new KLGLTexture("common/internalfont.png");
		userInterfaceTexture = new KLGLTexture("common/ui.png");

		userInterface = new KLGLSprite(userInterfaceTexture, 32, 32);

		// Audio
		soundTest = new KLGLSound;
		//soundTest->open(NeoSoundServer);

		// Fonts
		font = new KLGLFont(charmapTexture->gltexture, charmapTexture->width, charmapTexture->height, 8, 8, -1);

		// Default shaders
		gc->InitShaders(0, 0, 
			"common/passthrough.vert",
			"common/passthrough.frag"
		);

	}catch(KLGLException e){
		quit = 1;
		MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
	}

	cl("\nNeo %s R%d", APP_VERSION, APP_BUILD_VERSION); 
	luaopen_neo(gc->lua);
	int luaStat = luaL_loadfile(gc->lua, "common/init.lua");
	if ( luaStat==0 ) {
		// execute Lua program
		luaStat = lua_pcall(gc->lua, 0, LUA_MULTRET, 0);
	}
	LuaCheckError(gc->lua, luaStat);
	/*sq_pushroottable(gc->squirrel); //push the root table(were the globals of the script will be stored)
	if (SQ_SUCCEEDED(sqstd_dofile(gc->squirrel, _SC("test.nut"), 0, 1))){
		call_foo(gc->squirrel, 1, 2.5, _SC("teststring"));
	}
	sq_pop(gc->squirrel, 1); //pops the root table*/

	if (quit){
		MessageBox(NULL, "An important resource is missing or failed to load, check the console window _now_ for more details.", "Error", MB_OK | MB_ICONERROR);
	}

	while (!quit)
	{
		t0 = clock();
		
		if(PeekMessage( &gc->msg, NULL, 0, 0, PM_REMOVE)){
			if(gc->msg.message == WM_QUIT){
				quit = true;
			}else if (gc->msg.message == WM_KEYUP){
				switch(mode){
				case GameMode::_INGAME:
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
					break;
				}
			}else if (gc->msg.message == WM_KEYDOWN){
				// Application wide events
				switch (gc->msg.wParam){
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
						if ((gc->msg.wParam>=32) && (gc->msg.wParam<=255) && (gc->msg.wParam != '|')) {
							if (strlen(inputBuffer) < 255) {
								//cl("key: 0x%x\n", graphicsContext->msg.wParam);
								char buf[3];
								sprintf(buf, "%c\0", vktochar(gc->msg.wParam));
								strcat(inputBuffer, buf);

							}
						} else if (gc->msg.wParam == VK_BACK) {
							if(strlen(inputBuffer) > 0){
								inputBuffer[strlen(inputBuffer)-1]='\0';
							}
						} else if (gc->msg.wParam == VK_TAB) {
							inputBuffer[strlen(inputBuffer)]='\t';
						} else if (gc->msg.wParam == VK_RETURN) {
							if(strlen(inputBuffer) > 0){
								if (strcmp(inputBuffer, "exit") == 0){
									PostQuitMessage(0);
									quit = true;
								}else{
									int s = luaL_loadstring(gc->lua, inputBuffer);
									if ( s==0 ) {
										// execute Lua program
										s = lua_pcall(gc->lua, 0, LUA_MULTRET, 0);
									}
									LuaCheckError(gc->lua, s);
								}
							}else{
								cl("-- RETURN\n", 13);
								mode = GameMode::_MAPLOAD;
							}
							fill_n(inputBuffer, 256, '\0');
						}
					}
					break;
				case GameMode::_INGAME:
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
						mode = GameMode::_MAPLOAD;
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
		}else if (t0-t1 >= CLOCKS_PER_SEC/gc->fps || !internalTimer){

			GetCursorPos(&mouseXY);
			ScreenToClient(gc->hWnd, &mouseXY);

			switch(mode){
			case GameMode::_MENU:											// ! Start menu

				sprintf(textBuffer, "@CFFFFFF@D%s\n> %s%c", clBuffer, inputBuffer, (frame%16 <= 4 ? '_' : '\0'));

				// Update clock
				t1 = clock();

				gc->OpenFBO(50, 0.0, 0.0, 80.0);
				gc->OrthogonalStart();
				{
					// Reset pallet
					KLGLColor(255, 255, 255, 255).Set();
					// Draw BG
					gc->OrthogonalStart(gc->overSampleFactor);
					static int scrollOffset;
					if (scrollOffset == NULL || scrollOffset > 32){
						scrollOffset = 0;
					}
					for (int y = 0; y-1 <= gc->buffer.height/32; y++)
					{
						for (int x = 0; x-1 <= gc->buffer.width/32; x++)
						{
							gc->BlitSprite2D(userInterface, (x*32)-scrollOffset, (y*32)-scrollOffset, 0);
						}
					}
					scrollOffset++;
					gc->OrthogonalStart();
					gc->Blit2D(titleTexture, 0, 0);
					gc->OrthogonalStart(gc->overSampleFactor);

					// Draw console
					font->Draw(8, 8, textBuffer);
				}
				gc->OrthogonalEnd();
				gc->Swap();
				break;
			case GameMode::_MAPLOAD:										// ! Threaded map loader :D

				cl("\nNew game...\n");
				try{
					// Shutdown any sound the menu mite be playing
					//soundTest->close();
					//soundTest->open(NeoSoundServer);
					// Initialize the game engine
					gameEnv = new Environment();
					// Setup a thread to load and parse our map(if the map is large we want to be able to check its progress).
					mapLoader = new EnvLoaderThread(gameEnv);
					//soundTest->close();

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
						SwapBuffers(gc->hDC);
					}
					
					// Pass-through our font
					gameEnv->hudFont = font;
					// Map sprites
					gameEnv->mapSpriteSheet = new KLGLSprite("common/tilemap.png", 16, 16);
					gameEnv->hudSpriteSheet = new KLGLSprite("common/hud.png", 8, 8);
					backdropTexture = new KLGLTexture("common/clouds.png");
					gameoverTexture = new KLGLTexture("common/gameover.png");

					delete mapLoader;

					// Load shader programs
					gc->InitShaders(1, 0, 
						"common/postDefaultV.glsl",
						"common/production.frag"
						);
					gc->InitShaders(2, 0, 
						"common/postDefaultV.glsl",
						"common/tile.frag"
						);
					gc->InitShaders(3, 0, 
						"common/postDefaultV.glsl",
						"common/gaussianBlur.frag"
						);
					// Setup initial post quality
					gc->BindShaders(1);
					glUniform1i(glGetUniformLocation(gc->GetShaderID(1), "preset"), qualityPreset);
					gc->UnbindShaders();

					// Misc
					for (int i = 0; i < 100; i++)
					{
						stars[i] = new Point<int>;
						stars[i]->x = rand()%(gameEnv->map.width*16+1);
						stars[i]->y = rand()%(gameEnv->map.height*16+1);
					}

					mode = GameMode::_INGAME;
				}catch(KLGLException e){
					MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
					mode = GameMode::_MENU;
				}

				//envCallbackPtr = gameEnv;

				//cl("Running user script common/map01.lua...\n");
				//gameEnv->mapProg = file_contents("common/map01.lua");
				/*luaStat = luaL_loadstring(gc->lua, gameEnv->mapProg);
				if (luaStat == 0)
				{
					luaStat = lua_pcall(gc->lua, 0, LUA_MULTRET, 0);
				}
				gc->LuaCheckError(gc->lua, luaStat);*/

				cl("\nGame created successfully.\n");

				break;
			case GameMode::_MAPDESTROY:

				cl("Unloading game...");
				try{
					delete gameEnv;
					gc->UnloadShaders(1);
					gc->UnloadShaders(2);
					gc->UnloadShaders(3);
				}catch(KLGLException e){
					MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
					mode = GameMode::_MENU;
				}

				mode = GameMode::_MENU;
				cl(" [OK]\n");

				break;
			case GameMode::_INGAME:											// ! Main game test environment

				// Compute game physics and update events
				gameEnv->dt = (t0 - t1)/double(CLOCKS_PER_SEC);
				//gameEnv->dtMulti = mouseXY.x/1000.0;
				gameEnv->comp(gc, (mapScroll ? mouseXY.x - gc->buffer.width / 2 : 0), (mapScroll ? mouseXY.y - gc->buffer.height / 2 : 0));

				// Debug info
				if (KLGLDebug)
				{
					sprintf(textBuffer, "@CF8F8F8@D%s \n\nNeo\n-------------------------\n"\
						"Render FPS: %d\n"\
						"       Syn: V%dT%d\n"\
						"       Buf: %dx%d\n"\
						"       Acu: %Lf (less is more)\n\n"\
						"Player Pos: %f,%f\n"\
						"       Vel: %f,%f\n\n"\
						"Map    Pos: %d,%d\n"\
						"       Rel: %d,%d >> %d\n"\
						"       Tar: %d,%d >> %d\n\n"\
						"Debug  Act: %d\n"\
						"       DB0: %d\n"\
						"       DB1: %d\n", 
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
					int horizon = max(gc->window.width, gc->window.height+(-gameEnv->scroll.y));
					int stratosphere = min(0, -gameEnv->scroll.y);
					glBegin(GL_QUADS);
					glColor3ub(71,84,93);
					glVertex2i(0, stratosphere); glVertex2i(gc->window.width, stratosphere);
					glColor3ub(214,220,214);
					glVertex2i(gc->window.width, horizon); glVertex2i(0, horizon);
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
						gc->Rectangle2D(x, y, 1, 1, KLGLColor(255, 255, 255, 127));
					}

					// Reset pallet
					KLGLColor(255, 255, 255, 255).Set();

					// Cloud
					gc->Blit2D(backdropTexture, (APP_SCREEN_W/2)-(gameEnv->scroll.x/4), (APP_SCREEN_H/3)-(gameEnv->scroll.y/4));

					/*glTranslatef(gc->window.width/2.0f, gc->window.height/2.0f, 0.0f);
					glScalef(mouseXY.y/100.0f, mouseXY.y/100.0f, 1.0f);
					glTranslatef(-gc->window.width/2.0f, -gc->window.height/2.0f, 0.0f);*/

					// Draw Map
					gameEnv->drawMap(gc);

					// Draw Player
					gameEnv->character->draw(gameEnv, gc, gameEnv->mapSpriteSheet, frame);

					/*glTranslatef(gc->window.width/2.0f, gc->window.height/2.0f, 0.0f);
					glScalef(-mouseXY.y/100.0f, -mouseXY.y/100.0f, 1.0f);
					glTranslatef(-gc->window.width/2.0f, -gc->window.height/2.0f, 0.0f);*/

					// Master post shader data
					gc->BindShaders(1);
					glUniform1f(glGetUniformLocation(gc->GetShaderID(1), "time"), gc->shaderClock);
					glUniform2f(glGetUniformLocation(gc->GetShaderID(1), "BUFFER_EXTENSITY"), gc->window.width*gc->scaleFactor, gc->window.height*gc->scaleFactor);
					gc->BindMultiPassShader(1, 1, false);
					gc->UnbindShaders();

					// HUD, Score, Health, etc
					gameEnv->drawHUD(gc, gameoverTexture);

					if (KLGLDebug)
					{
						gc->OrthogonalStart(gc->overSampleFactor);
						font->Draw(8, 14, textBuffer);
						sprintf(textBuffer, "@CFFFFFF@D%c", 16);
						font->Draw(mouseXY.x, mouseXY.y, textBuffer);
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
	delete gc;
	return 0;
}

void Loading(KLGL *gc, KLGLTexture *loading, KLGLTexture *splash)
{
	gc->OpenFBO();
	gc->OrthogonalStart();
	{
		// Initialize auxiliary buffer(pretty bg)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gc->fbo[1]);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		gc->Blit2D(splash, 0, 0);

		// Draw loading screen
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, gc->fbo[0]);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		gc->Blit2D(loading, 0, 0);
	}
	gc->OrthogonalEnd();
	gc->Swap();
}