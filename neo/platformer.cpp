#include "masternoodles.h"
#include "platformer.h"
#include "logicalObjects.h"
#include <list>

using namespace std;
using namespace klib;
namespace emb{
	PyObject* Stdout_write(PyObject* self, PyObject* args)
	{
		std::size_t written(0);
		Stdout* selfimpl = reinterpret_cast<Stdout*>(self);
		if (selfimpl->write)
		{
			char* data;
			if (!PyArg_ParseTuple(args, "s", &data))
				return 0;

			std::string str(data);
			selfimpl->write(str);
			written = str.size();
		}
		return PyLong_FromSize_t(written);
	}

	PyObject* Stdout_flush(PyObject* self, PyObject* args)
	{
		// no-op
		return Py_BuildValue("");
	}

	PyMODINIT_FUNC PyInit_emb(void) 
	{
		g_stdout = 0;
		g_stdout_saved = 0;

		StdoutType.tp_new = PyType_GenericNew;
		if (PyType_Ready(&StdoutType) < 0)
			return 0;

		PyObject* m = PyModule_Create(&embmodule);
		if (m)
		{
			Py_INCREF(&StdoutType);
			PyModule_AddObject(m, "Stdout", reinterpret_cast<PyObject*>(&StdoutType));
		}
		return m;
	}

	void set_stdout(stdout_write_type write)
	{
		if (!g_stdout)
		{
			g_stdout_saved = PySys_GetObject("stdout"); // borrowed
			g_stdout = StdoutType.tp_new(&StdoutType, 0, 0);
		}

		Stdout* impl = reinterpret_cast<Stdout*>(g_stdout);
		impl->write = write;
		PySys_SetObject("stdout", g_stdout);
		PySys_SetObject("stderr", g_stdout);
	}

	void reset_stdout()
	{
		if (g_stdout_saved)
			PySys_SetObject("stdout", g_stdout_saved);

		Py_XDECREF(g_stdout);
		g_stdout = 0;
	}
}
namespace NeoPlatformer{

	map<const char*, void*, cmp_cstring> neoCallbackList;

	Environment::Environment(char *map){
		gcProxy = nullptr;
		audioProxy = nullptr;
		mode = 0;
		dt = 0.0;
		dtMulti = 1.0;
		scroll.x = scroll.y = 0;
		scroll_real.x = scroll_real.y = 0;
		target.x = target.y = 0;
		scrollspeed = 20;
		scrollspeedMulti = 80;
		character = NULL;
		mapName = map;
		mapProg = new char[4096];
		mapDispListState = 0;
		mapDispList = glGenLists(1);

		neoCallbackList["gameEnv"] = this;
		neoCallbackList["gameEnv.scrollX"] = &scroll.x;
		neoCallbackList["gameEnv.scrollY"] = &scroll.y;
		neoCallbackList["gameEnv.scrollspeed"] = &scrollspeed;
		neoCallbackList["gameEnv.scrollspeedMulti"] = &scrollspeedMulti;
	}

	Environment::~Environment(){
		delete mapData,mapMask,mapProg;
		glDeleteLists(mapDispList, 1);
		delete character;
		platforms.clear();
		//delete platforms;
	}

	void Environment::comp(KLGL* gc, int offsetX, int offsetY){

		// Compute map position based on character pos and scroll delta
		target.x = int((character->pos.x+offsetX) - APP_SCREEN_W / 2) << FPM;
		target.y = int((character->pos.y+offsetY) - APP_SCREEN_H / 2) << FPM;

		scroll_real.x += ((target.x - scroll_real.x) / scrollspeed)*dt*scrollspeedMulti;
		scroll_real.y += ((target.y - scroll_real.y) / scrollspeed)*dt*scrollspeedMulti;

		scroll.x = scroll_real.x >> FPM;
		scroll.y = scroll_real.y >> FPM;

		// Sanitize timer, if were delayed any more then 500ms theres something wrong!
		dt = min(dt, 0.5f);

		// Multiply the timer by the dtMulti modifier, this should produce a slow motion effect for numbers < 1.0
		dt = max(dt*dtMulti, 0.00005f);

		// Update the character based on velocity, gravity and FPU errors.
		character->comp(this);

		// Update enermys
		for (auto e = enemys.begin(); e != enemys.end(); e++){
			(*e)->comp(this);
		}

		// Check for collisions and set character state.
		for(auto e = platforms.begin(); e != platforms.end(); e++){
			CollisionType characterState;
			if(character->health <= 0){
				characterState = C_NONE;
			}else{
				characterState = (*e).checkCollision(character, dt, e);
			}
			if(characterState != C_NONE){
				if((*e).mapMaskPtr[(*e).tileId] == TileType::T_LAVA){
					character->vel.y = character->jumpAccel/2.0f;
					character->health -= 10;
					audioProxy->system->playSound(FMOD_CHANNEL_FREE, audioProxy->sound[3], 0, &audioProxy->channel[2]);
				}
			}
			for (auto a = enemys.begin(); a != enemys.end(); a++){
				characterState = (*e).checkCollision((*a), dt, e);
			}
		}

		// Run python script
		PyRun_SimpleString(mapProg);

		debugflags.x = character->health;
		debugflags.y = gc->shaderClock;
	}

	void Environment::drawHUD(KLGL* gc){
		if (character->health <= 0)
		{
			// Blur shader data
			float relHealth = min(-character->health, 1000);
			gc->Rectangle2D(0, 0, gc->window.width, gc->window.height, KLGLColor(200, 0, 0, (relHealth/1000.0f)*255));
			gc->Blit2D(gameoverTexture, 0, 0);
		}

		gc->OrthogonalStart(gc->overSampleFactor);
		if (character->health <= 0 && gc->shaderClock >= 500 && gc->shaderClock <= 1000){

		}else{
			Point<int> hudPos = Point<int>();
			hudPos.x = 8;
			hudPos.y = gc->buffer.height-40;
			gc->BlitSprite2D(hudSpriteSheet16, hudPos.x, hudPos.y+0, 0, 1);
			gc->BlitSprite2D(hudSpriteSheet16, hudPos.x, hudPos.y+16,1, 1);
			gc->BlitSprite2D(hudSpriteSheet16, hudPos.x+16, hudPos.y+0, 0, 2);
			gc->BlitSprite2D(hudSpriteSheet16, hudPos.x+16, hudPos.y+16,1, 2);
			gc->BlitSprite2D(hudSpriteSheet16, hudPos.x+32, hudPos.y+0, 0, 3);
			gc->BlitSprite2D(hudSpriteSheet16, hudPos.x+32, hudPos.y+16,1, 3);

			int offset = gc->buffer.width-152;
			for (int i = 0; i <= 16; i++){
				gc->BlitSprite2D(hudSpriteSheet8, offset+(i*8), 16, 1);
				if (i == 16){
					gc->BlitSprite2D(hudSpriteSheet8, offset+((i+1)*8), 16, 2);
				}else if (i == 0){
					gc->BlitSprite2D(hudSpriteSheet8, offset+((i-1)*8), 16, 0);
				}

				if (i <= (character->health * 16) / 100){
					gc->BlitSprite2D(hudSpriteSheet8, offset+(i*8), 16, 3);
				}
			}
			hudFont->Draw(offset-(16*8), 16, "@DSCORE: 00000");
		}
	}

	void Environment::drawMap(KLGL* gc){
		glTranslatef(-scroll.x, -scroll.y, 0);
		if (mapDispListState == 1){
			glCallList(mapDispList);
		}else{
			glNewList(mapDispList, GL_COMPILE_AND_EXECUTE);
			// Draw each platform
			for(auto e = platforms.begin(); e != platforms.end(); e++){
				e->draw(gc, this, mapSpriteSheet);
			}
			glEndList();
			mapDispListState = 1;
		}
		glTranslatef(scroll.x, scroll.y, 0);
	}

	void Environment::map_span(int type, int x0, int y0, int x1, int y1)
	{
		if (!(x0 == x1 && y0 == y1)){

			if(KLGLDebug){
				//cl("[%d,%d->%d,%d]\n", x0, y0, x1, y1);
			}

			int flags = 0;
			//memset(flags, 0, 6);

			switch (mapMask[type])
			{
			case TileType::T_NONSOLID:
				flags = TileFlag::F_NONSOLID;
				break;
			case TileType::T_UNDER:
				flags = TileFlag::F_FLOOR;
				break;
			case TileType::T_LEFTSLOPE:
				flags = TileFlag::F_LEFTSLOPE;
				break;
			case TileType::T_RIGHTSLOPE:
				flags = TileFlag::F_RIGHTSLOPE;
				break;
			case TileType::T_LAVA:
			default:
				flags = TileFlag::F_CEIL | TileFlag::F_FLOOR | TileFlag::F_LEFT | TileFlag::F_RIGHT;
			}
			platforms.push_back(Platform(x0*tileWidth, y0*tileHeight, x1*tileWidth, y1*tileHeight, type, flags, mapData, mapMask));
		}
	}

	void Environment::load_map(char* mapPath){
		cl("Loading Map: %s.smp&bmd&py ", mapPath);

		char* tmpPath = new char[strlen(mapPath)+4];
		strcpy(tmpPath, mapPath);
		FILE* mapFile = fopen(strcat(tmpPath, ".smp"), "rb");
		strcpy(tmpPath, mapPath);
		FILE* mapInfoFile = fopen(strcat(tmpPath, ".bmd"), "rb");
		strcpy(tmpPath, mapPath);
		FILE* mapScriptFile = fopen(strcat(tmpPath, ".py"), "r");

		if (mapFile == NULL || mapInfoFile == NULL || mapScriptFile == NULL){
			cl("[ERROR]\n");
			throw KLGLException("Error opening map file!");
			return;
		}

		size_t wordSize;
		char tBuffer[8];
		map.width = 0;
		map.height = 0;
		tileWidth = 0;
		tileHeight = 0;

		fill_n(tBuffer, 8, '\0');
		fread(&tBuffer, 1, 7, mapFile);
		if (strcmp(substr(tBuffer, 0, 7), "STME1.0")){
			cl("[BADHEADER=\"%s\"]", tBuffer);
		}

		fill_n(tBuffer, 8, '\0');
		fread(&tBuffer, 1, 1, mapFile);
		if (!strcmp(substr(tBuffer, 0, 1), "B")){
			wordSize = sizeof(char);
		}else{
			wordSize = sizeof(unsigned short);
		}

		fseek(mapFile, 8, SEEK_SET);
		fread(&map.width, wordSize, 1, mapFile);
		fread(&map.height, wordSize, 1, mapFile);
		fread(&tileWidth, 1, 1, mapFile);
		fread(&tileHeight, 1, 1, mapFile);

#define lvLookup y*map.width+x

		mapData = (char*)malloc((map.width*map.height)+1);
		mapMask = (char*)malloc((map.width*map.height)+1);

		fill_n(mapData, (map.width*map.height)+1, '\0');
		fill_n(mapMask, (map.width*map.height)+1, '\0');

		if(KLGLDebug){
			cl("\n- %dx%d %dbyte(s)\n", map.width, map.height, (map.width*map.height)+8);
		}

		// Read files into memory... back to front and inside out...
		for(int y = 0; y < map.height; y++){
			for(int x = 0; x < map.width; x++){
				if (&mapData[lvLookup] == NULL)
				{
					throw KLGLException("Null item at address: %d", lvLookup);
				}
				fread(&mapData[lvLookup], 1, 1, mapFile);
				fread(&mapMask[lvLookup], 1, 1, mapInfoFile);
			}
		}

		fclose(mapFile);
		fclose(mapInfoFile);

		if(KLGLDebug){
			cl("- Parsing...\n");
		}

		// Parse map data into geometry vectors
		for(int x = 0; x < map.width; x++){
			for(int y = 0; y < map.height; y++){
				char tile = mapData[lvLookup];
				char mask = mapMask[lvLookup];
				if(tile > 0){
					int xl = x;
					int yl = y;
					while(mapData[yl*map.width+xl] != 0 && mapData[yl*map.width+xl] == tile && mapMask[yl*map.width+xl] == mask){
						yl++;
						/*while(mapData[yl*map.width+xl] != 0 && mapData[yl*map.width+xl] == tile && mapMask[yl*map.width+xl] == mask){
							xl++;
						}*/
					}
					map_span(y*map.width+x, x, y, 1, yl-y);
					y = yl-1;
					//x = xl-1;
				}
			}
		}

		// Load maps script
		fseek(mapScriptFile, 0, SEEK_END);
		int scriptLen = ftell(mapScriptFile);
		rewind(mapScriptFile);
		mapProg = (char*)malloc(scriptLen);
		fill_n(mapProg, scriptLen, '\0');
		fread(mapProg, 1, scriptLen, mapScriptFile);

		cl("[OK]\n");
	}

	void Environment::drawLoader(KLGL* gc, int y, int height, int length, int speed){
		static int i;
		if (i == NULL || i < 0 || i >= gc->buffer.width+length){
			i = 0;
		}
		int x = gc->buffer.width-i;
		gc->OrthogonalStart(gc->overSampleFactor);
		glBegin(GL_QUADS);
		glColor4ub(255, 255, 255, 127);
		glVertex2i(x, y);
		glColor4ub(255, 255, 255, 60);
		glVertex2i(x+(length*((x+(length/2))/(float)gc->buffer.width)), y);
		glVertex2i(x+(length*((x+(length/2))/(float)gc->buffer.width)), y+height);
		glColor4ub(255, 255, 255, 127);
		glVertex2i(x, y+height);
		glEnd();
		i += speed;
	}

	void EnvLoaderThread::run(){
		// Cast it cunt
		Environment *gameEnv = (Environment*)loaderPtr;
		// Switch to auxiliary context to access texture data
		wglMakeCurrent(gameEnv->gcProxy->windowManager->wm->hDC, gameEnv->gcProxy->windowManager->wm->hRCAUX);

		try{
			// Load the map
			gameEnv->load_map(gameEnv->mapName);
			// Create our character
			gameEnv->character = new Character(128, 0, 8, 16);
			gameEnv->enemys.push_back(new Enemy(128, 415, 16, 16));
			// Map sprites
			gameEnv->mapSpriteSheet = new KLGLSprite("common/tilemap.png", 16, 16);
			gameEnv->hudSpriteSheet8 = new KLGLSprite("common/hud8.png", 8, 8);
			gameEnv->hudSpriteSheet16 = new KLGLSprite("common/hud16.png", 16, 16);
			// Textures
			gameEnv->backdropTexture = new KLGLTexture("common/clouds.png");
			gameEnv->gameoverTexture = new KLGLTexture("common/gameover.png");
			// Sounds
			// Audio
			gameEnv->audioProxy->loadSound("common/music.ogg", 1, FMOD_LOOP_NORMAL);
			gameEnv->audioProxy->loadSound("common/jump.wav", 2, FMOD_LOOP_OFF);
			gameEnv->audioProxy->loadSound("common/hurt.wav", 3, FMOD_LOOP_OFF);
			gameEnv->audioProxy->loadSound("common/ambient2.mp3", 4, FMOD_LOOP_NORMAL);
			// Load shader programs
			gameEnv->gcProxy->InitShaders(1, 0, 
				"common/postDefaultV.glsl",
				"common/production.frag"
				);
			gameEnv->gcProxy->InitShaders(2, 0, 
				"common/postDefaultV.glsl",
				"common/crt.frag"
				);
			gameEnv->gcProxy->InitShaders(3, 0, 
				"common/postDefaultV.glsl",
				"common/fastBloom.frag"
				);
			gameEnv->gcProxy->InitShaders(4, 0, 
				"common/postDefaultV.glsl",
				"common/lights.frag"
				);
		}catch(KLGLException e){
			MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
			status = false;
		}

		// Rebind the primary context(we have to do it in this thread since exiting with 
		// the auxiliary bound causes both contexts to be reset.
		wglMakeCurrent(gameEnv->gcProxy->windowManager->wm->hDC, gameEnv->gcProxy->windowManager->wm->hRC);
		status = false;
	}

	Character::Character(float x, float y, int w, int h)
	{
		standingRect = Rect<int>(0, 0, w, h);
		collisionRect = standingRect;

		score = 0;
		health = 100;
		walkAnime = 0;

		pos.x = x;
		pos.y = y;
		vel.x = 0.0f;
		vel.y = 0.0f;

		collisionDetect = 0;
		falling = true;  // Start in the air (this will reset when you hit the ground)
		fallLock = false;
		walking = false;
		facingRight = true;
		rightPressed = false;
		leftPressed = false;
		landedLastFrame = false;

		gravity = 800.0f;
		jumpAccel = -350.0f;
		walkAccel = 650.0f;
		maxWalkSpeed = 250.0f;
		brakeAccel = 900.0f;
		groundDrag = 600.0f;
		slideDrag = 200.0f;
		airDrag = 30.0f;
		airAccel = 100.0f;

		neoCallbackList["characterMaxWalkSpeed"] = &maxWalkSpeed;
	}

	Character::~Character(){

	}

	void Character::draw(Environment* env, KLGL* gc, KLGLSprite *sprite, int frame){
		int x = pos.x - env->scroll.x;
		int y = pos.y - env->scroll.y;

		if(facingRight){
			gc->BlitSprite2D(sprite, x-8, y-16, 1+walkAnime);
		}else{
			gc->BlitSprite2D(sprite, x-8, y-16, 4+walkAnime);
		}

		walkAnime += (frame%2 ? 0 : 1);
		if (walkAnime > 2 || !walking)
		{
			walkAnime = 0;
		}
	}

	void Character::jump(){
		if(!falling && !fallLock && vel.y <= 0)
		{
			vel.y += (fabs((float)vel.x)/maxWalkSpeed)*0.1f*jumpAccel + 1.0f*jumpAccel;
			falling = 1;
			fallLock = 1;
			envPtr->audioProxy->system->playSound(FMOD_CHANNEL_FREE, envPtr->audioProxy->sound[2], 0, &envPtr->audioProxy->channel[1]);
		}
		wallJump();
	}

	void Character::jumpUp(){
		fallLock = 0;
	}

	void Character::wallJump(){
		if (!fallLock && collisionDetect != 0)
		{
			vel.y += (vel.x/maxWalkSpeed)*jumpAccel + 0.85f*jumpAccel;
			if(vel.x >= 0 && rightPressed){
				vel.x -= walkAccel*1000.0f;
			}else if (vel.x <= 0 && leftPressed){
				vel.x += walkAccel*1000.0f;
			}
			envPtr->audioProxy->system->playSound(FMOD_CHANNEL_FREE, envPtr->audioProxy->sound[2], 0, &envPtr->audioProxy->channel[1]);
		}
	}

	void Character::land(int platformY)
	{
		pos.y = (float)platformY;
		vel.y = 0;
		if(!landedLastFrame && (rightPressed || leftPressed))
		{
			walking = 1;
		}
		falling = 0;
		landedLastFrame = 1;  // !falling
	}

	void Character::hitCeiling(int platformY)
	{
		pos.y = platformY + collisionRect.getHeight()+1;
		vel.y = 0;
	}

	void Character::wallLeft(int wallX)
	{
		collisionDetect = -1;
		pos.x = wallX + collisionRect.getWidth()/2;
		vel.x = 0;
	}

	void Character::wallRight(int wallX)
	{
		collisionDetect = 1;
		pos.x = wallX - collisionRect.getWidth()/2;
		vel.x = 0;
	}

	void Character::slopeLeft(int charX, int wallY){
		collisionDetect = -1;
		pos.y = wallY + (charX % 16);
		vel.y = 0;
		if(!landedLastFrame && (rightPressed || leftPressed))
		{
			walking = 1;
		}
		falling = 0;
		landedLastFrame = 1;  // !falling
	}

	void Character::slopeRight(int charX, int wallY){
		collisionDetect = 1;
		pos.y = wallY + (charX % 16);
		vel.y = 0;
		if(!landedLastFrame && (rightPressed || leftPressed))
		{
			walking = 1;
		}
		falling = 0;
		landedLastFrame = 1;  // !falling
	}

	void Character::left()
	{
		leftPressed = 1;
		if(!rightPressed)
			facingRight = 0;
		walking = 1;
	}

	void Character::leftUp()
	{
		leftPressed = 0;
		if(!rightPressed)
			walking = 0;
		else
			facingRight = 1;
	}

	void Character::right()
	{
		rightPressed = 1;
		if(!leftPressed)
			facingRight = 1;
		walking = 1;
	}

	void Character::rightUp()
	{
		rightPressed = 0;
		if(!leftPressed)
			walking = 0;
		else
			facingRight = 0;
	}

	void Character::drag()
	{
		// Regular slowdown
		if(vel.x > 0) // Moving right
		{
			if(!falling)
			{
				vel.x -= groundDrag*envPtr->dt;
			}
			else  // Mid-air slowdown
				vel.x -= airDrag*envPtr->dt;

			// Came to a stop
			if(vel.x < 0)
				vel.x = 0;
		}
		else if(vel.x < 0)  // Moving left
		{
			if(!falling)
			{
				vel.x += groundDrag*envPtr->dt;
			}
			else  // Mid-air slowdown
				vel.x += airDrag*envPtr->dt;

			// Came to a stop
			if(vel.x > 0)
				vel.x = 0;
		}
	}

	// Update the character based on movement and gravity and such.
	void Character::comp(Environment *env)
	{
		if (env != envPtr){
			envPtr = env;
		}
		if(walking && facingRight)  // Speed up or turn around
		{
			if(vel.x >= 0)
			{
				if(!falling)  // Walking speed up
				{
					vel.x += walkAccel*envPtr->dt;
				}
				else  // Mid-air speed up
					vel.x += airAccel*envPtr->dt;
			}
			else
			{
				if(!falling)  // Walking turn around
				{
					vel.x += brakeAccel*envPtr->dt;
				}
				else  // Mid-air slowdown
					vel.x += airAccel*envPtr->dt;
			}
		}
		else if(walking && !facingRight)  // Speed up or turn around
		{
			if(vel.x <= 0)
			{
				if(!falling)  // Walking speed up
				{
					vel.x -= walkAccel*envPtr->dt;
				}
				else  // Mid-air speed up
					vel.x -= airAccel*envPtr->dt;
			}
			else
			{
				if(!falling)  // Walking turn around
				{
					vel.x -= brakeAccel*envPtr->dt;
				}
				else  // Mid-air slowdown
					vel.x -= airAccel*envPtr->dt;
			}
		}else{
			drag();
		}

		// Speed limit
		if(walking)
		{
			if(vel.x > maxWalkSpeed)
			{
				vel.x = maxWalkSpeed;
			}
			else if(vel.x < -maxWalkSpeed)
			{
				vel.x = -maxWalkSpeed;
			}
		}

		// Apply gravitational acceleration
		vel.y += gravity*envPtr->dt;

		// Update positions
		pos.x += vel.x*envPtr->dt;
		pos.y += vel.y*envPtr->dt;

		// Check if falling to death
		if(pos.y > envPtr->map.height*envPtr->tileHeight){
			health -= 3;
		}

		collisionDetect = 0;
		landedLastFrame = !falling;
		// This disables the falling air jump:
		// We are falling all the time, but if we collide with a surface,
		// then we can jump during that frame.
		//falling = 1;
	};

	Enemy::Enemy(float x, float y, int w, int h){
		pos.x = x;
		pos.y = y;
		vel.x = 100.0f;
		vel.y = 0.0f;
		gravity = 800.0f;
		collisionDetect = 0;
		standingRect = Rect<int>(0, 0, w, h);
		collisionRect = standingRect;
	}

	void Enemy::draw(KLGL* gc, KLGLSprite *sprite, int frame){
		int x = (pos.x - envPtr->scroll.x) - 8;
		int y = (pos.y - envPtr->scroll.y) - 16;

		gc->BlitSprite2D(sprite, x, y, 3, (vel.x > 0.0f ? 4 : 5));
	}

	void Enemy::patrol(){
		/*if (collisionDetect == 1){
		vel.x = -1;
		}else if (collisionDetect == -1){
		vel.x = 1;
		}
		pos.x += vel.x*envPtr->dt;*/
	}

	void Enemy::land(int platformY)
	{
		pos.y = (float)platformY;
		vel.y = 0;
		if(!landedLastFrame && (rightPressed || leftPressed))
		{
			walking = 1;
		}
		falling = 0;
		landedLastFrame = 1;  // !falling
	}

	void Enemy::wallLeft(int wallX)
	{
		collisionDetect = -1;
		pos.x = wallX + collisionRect.getWidth()/2.0f;
		vel.x = 200.0f;
	}

	void Enemy::wallRight(int wallX)
	{
		collisionDetect = 1;
		pos.x = wallX - collisionRect.getWidth()/2.0f;
		vel.x = -200.0f;
	}

	void Enemy::comp(Environment *env){
		if (env != envPtr){
			envPtr = env;
		}

		// Do damage to player
		float cTop = envPtr->character->pos.y - envPtr->character->collisionRect.height;
		float cBottom = envPtr->character->pos.y;
		float cLeft = envPtr->character->pos.x - envPtr->character->collisionRect.width/2;
		float cRight = envPtr->character->pos.x + envPtr->character->collisionRect.width/2;
		float eTop = pos.y;
		float eBottom = pos.y + collisionRect.height;
		float eLeft = pos.x;
		float eRight = pos.x + collisionRect.width;
		if((cLeft <= eRight && cRight >= eLeft)	// Crossed left X and...
			&& (((eTop <= cTop && cTop <= eBottom)									// Corner within platform Y or...
			|| (eTop <= cBottom && cBottom <= eBottom))								// Other corner within platform Y
			|| (cTop <= eTop && eBottom <= cBottom)))								// Wall is short and at the middle of the character
		{
			envPtr->character->vel.y = envPtr->character->jumpAccel/2.0f;
			envPtr->character->health -= 10;
			envPtr->audioProxy->system->playSound(FMOD_CHANNEL_FREE, envPtr->audioProxy->sound[3], 0, &envPtr->audioProxy->channel[2]);
		}

		patrol();

		// Apply gravitational acceleration
		vel.y += gravity*envPtr->dt;

		// Update positions
		pos.x += vel.x*envPtr->dt;
		pos.y += vel.y*envPtr->dt;

		collisionDetect = 0;
	}

	// Platform management
	Platform::Platform(float x, float y, int w, int h, int sprId, int flags, char* mapDataPtrI, char* mapMaskPtrI){
		tileId = sprId;
		tileFlags = flags;
		mapDataPtr = mapDataPtrI;
		mapMaskPtr = mapMaskPtrI;

		// Assume non-solid
		if (tileFlags == 0){
			ceiling = floor = wallLeft = wallRight = slopeLeft = slopeRight = 0;
		}else{
			ceiling =	(tileFlags & TileFlag::F_CEIL);
			floor =		(tileFlags & TileFlag::F_FLOOR);
			wallLeft =	(tileFlags & TileFlag::F_LEFT);
			wallRight =	(tileFlags & TileFlag::F_RIGHT);
			slopeLeft = (tileFlags & TileFlag::F_LEFTSLOPE);
			slopeRight= (tileFlags & TileFlag::F_RIGHTSLOPE);
		}
		pos.x = x;
		pos.y = y;
		collisionRect.x = x;
		collisionRect.y = y;
		collisionRect.setWidth(w);
		collisionRect.setHeight(h);
	}

	void Platform::draw(KLGL* gc, Environment* env, KLGLSprite* sprite){
		// Get number of rows/columns
		int w = chop(collisionRect.width/sprite->swidth);
		int h = chop(collisionRect.height/sprite->sheight);

		for(int iY = 0; iY < h; iY++){
			for(int iX = 0; iX < w; iX++){
				int scrX = floorMpl(pos.x, 16)+(iX*sprite->swidth);
				int scrY = floorMpl(pos.y, 16)+(iY*sprite->sheight);
				gc->BlitSprite2D(sprite, scrX, scrY, mapDataPtr[tileId+(iY*env->map.width+iX)]);
			}
		}

		/*glBegin(GL_LINES);
		glVertex2i(pos.x, pos.y);
		glVertex2i(pos.x+collisionRect.width, pos.y+collisionRect.height);
		glEnd();*/
	}

	int Platform::checkOverheadSlope(vector<Platform>::iterator platformData){
		int offset = ((platformData->pos.y/16)-1)*(256)+(platformData->pos.x/16);
		//cl("%d\n", offset);
		return ((platformData->mapMaskPtr[offset] == TileFlag::F_LEFTSLOPE || platformData->mapMaskPtr[offset] == TileFlag::F_RIGHTSLOPE) ? 0 : 1);
	}

	CollisionType Platform::checkCollision(ObjectInterface *object, double dt, vector<Platform>::iterator platformData)
	{
		if(object == NULL){
			throw KLGLException("Character does not exist!");
			return C_NONE;
		}

		// For algorithmic ease, here are the edges of the collision rects.
		/*  Here's the position (P) relative to the character.
		\O/
		|
		/ \
		P
		*/
		float cTop = object->pos.y - object->collisionRect.height;
		float cBottom = object->pos.y;
		float cLeft = object->pos.x - object->collisionRect.width/2;
		float cRight = object->pos.x + object->collisionRect.width/2;

		/*  Here's the position (P) relative to the platform.
		P-----
		|    |
		------
		*/
		float pTop = pos.y;
		float pBottom = pos.y + collisionRect.height;
		float pLeft = pos.x;
		float pRight = pos.x + collisionRect.width;

		int slopeDetect = checkOverheadSlope(platformData);

		if (slopeLeft)
		{
			if((cRight >= pLeft && cLeft <= pRight)										// Crossed X and...
				&& (((pTop <= cBottom && cTop <= pBottom)								// Corner within platform Y
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				object->slopeRight(pRight-cRight, pTop);
				return C_SLOPERIGHT;
			}
		}

		if (slopeRight)
		{
			if((cRight >= pLeft && cLeft <= pRight)										// Crossed right X and...
				&& (((pTop <= cBottom && cTop <= pBottom)								// Corner within platform Y
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y or...
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				object->slopeLeft(cLeft-pLeft, pTop);
				return C_SLOPELEFT;
			}
		}

		if(floor)
		{
			// Here's the collision test.
			// The 10pt here are to prevent slipping through a platform and
			// making sure that 'falling' is set to 0 each frame we're on a platform.
			// y*map.width+x
			float padding = 10.0f;

			if(slopeDetect && object->vel.y >= 0
				&& (cBottom - object->vel.y*dt - padding <= pTop && cBottom + padding >= pTop)  // Crossed top Y and...
				&& (((pLeft <= cLeft && cLeft <= pRight)  // Corner within platform X or...
				|| (pLeft <= cRight && cRight <= pRight))  // Other corner within platform X
				|| (cLeft <= pLeft && pRight <= cRight)))  // Platform is narrow and at the middle of the character
			{
				object->land(pTop);
				return C_DOWN;
			}
		}

		// Can I hit it on the way up?
		if(ceiling)
		{   
			if((cTop - object->vel.y*dt + 1 >= pBottom && cTop <= pBottom)  // Crossed bottom Y and...
				&& (((pLeft <= cLeft && cLeft <= pRight)  // Corner within platform X or...
				|| (pLeft <= cRight && cRight <= pRight))  // Other corner within platform X
				|| (cLeft <= pLeft && pRight <= cRight)))  // Ceiling is narrow and at the middle of the character
			{
				object->hitCeiling(pBottom);
				return C_UP;
			}
		}

		// Wall?
		if(wallLeft)
		{   
			if(slopeDetect && (cRight - object->vel.x*dt - 1 <= pLeft && cRight >= pLeft)	// Crossed left X and...
				&& (((pTop <= cTop && cTop <= pBottom)									// Corner within platform Y or...
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				object->wallRight(pLeft);
				return C_RIGHT;
			}
		}

		if(wallRight)
		{   
			if(slopeDetect && (cLeft - object->vel.x*dt + 1 >= pRight && cLeft <= pRight)	// Crossed right X and...
				&& (((pTop <= cTop && cTop <= pBottom)									// Corner within platform Y or...
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y or...
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				object->wallLeft(pRight);
				return C_LEFT;
			}
		}

		// No collisions
		return C_NONE;
	}
}
