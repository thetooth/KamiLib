#include "masternoodles.h"
#include "platformer.h"
#include "logicalObjects.h"
#include <list>

using namespace std;
using namespace klib;
namespace NeoPlatformer{

	Environment *envCallbackPtr = NULL;
	map<const char*, void*, cmp_cstring> neoVarList;

	/*int l_get( lua_State *L ){
		const char *key = luaL_checkstring(L, 2);

		if (neoVarList.count(key) > 0 && neoVarList[key] != NULL){
			float* val = (float*)neoVarList[key];
			if (KLGLDebug){
				printf("-- GET %s[0x%X] with value of %f\n", key, val, *val);
			}
			lua_pushnumber(L, *val);
			
			return 1;
		}else{
			lua_pushnil(L);
			return LUA_ERRERR;
		}
	}

	int l_set( lua_State *L ){
		const char *key = luaL_checkstring(L, 2);
		float val = luaL_checknumber(L, 3);		

		if (neoVarList.count(key) > 0 && neoVarList[key] != NULL){
			float *srcPtr = (float*)neoVarList[key];
			if (KLGLDebug){
				printf("-- SET %s[0x%X] with value of %f\n", key, srcPtr, val);
			}
			*srcPtr = val;

			return 1;
		}else{
			return LUA_ERRERR;
		}
	}

	int luaopen_neo(lua_State *L) {
		lua_newuserdata(L, 1);
		luaL_newmetatable(L, "neolib");
		luaL_setfuncs(L, neolib, NULL);
		neo_register_info(L);
		lua_setmetatable(L, -2);
		lua_setglobal(L, "neo");
		
		return 1;
	}*/

	Environment::Environment(){
		mode = 0;
		dt = 0.0;
		dtMulti = 1.0;
		scroll.x = scroll.y = 0;
		scroll_real.x = scroll_real.y = 0;
		target.x = target.y = 0;
		scrollspeed = 20;
		scrollspeedMulti = 80;
		character = NULL;
		platforms = NULL;
		mapProg = new char[4096];
		mapDispListState = 0;
		mapDispList = glGenLists(1);
		envCallbackPtr = this;

		neoVarList["scrollX"] = &scroll.x;
		neoVarList["scrollspeed"] = &scrollspeed;
		neoVarList["scrollspeedMulti"] = &scrollspeedMulti;
	}

	Environment::~Environment(){
		glDeleteLists(mapDispList, 1);
		delete character;
		delete platforms;
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
		dt = min(dt, 0.5);

		// Multiply the timer by the dtMulti modifier, this should produce a slow motion effect for numbers < 1.0
		dt = max(dt*dtMulti, 0.00005);

		// Update the character based on velocity, gravity and FPU errors.
		character->comp(&*this);

		// Check for collisions and set character state.
		for(list<Platform>::iterator e = platforms->begin(); e != platforms->end(); e++){
			CollisionType state = e->checkCollision(*this, e);
			if (state != C_NONE)
			{
				if (e->mapMaskPtr[e->tileId] == TileType::T_LAVA)
				{
					character->vel.y = character->jumpAccel/2.0f;
					character->health -= 10;
					audioProxy->system->playSound(FMOD_CHANNEL_FREE, audioProxy->sound[3], 0, &audioProxy->channel[2]);
				}
			}
		}

		// Do lua events
		/*int luaStat = luaL_loadstring(gc->lua, mapProg);
		if (luaStat == 0)
		{
			luaStat = lua_pcall(gc->lua, 0, LUA_MULTRET, 0);
		}
		gc->LuaCheckError(gc->lua, luaStat);*/
		
		debugflags.x = character->health;
		debugflags.y = gc->shaderClock;
	}

	void Environment::drawHUD(KLGL* gc){
		if (character->health <= 0)
		{
			// Blur shader data
			float relHealth = min(-character->health, 1000);
			/*gc->BindShaders(3);
			glUniform1f(glGetUniformLocation(gc->GetShaderID(3), "time"), gc->shaderClock);
			glUniform2f(glGetUniformLocation(gc->GetShaderID(3), "BUFFER_EXTENSITY"), gc->window.width*gc->overSampleFactor, gc->window.height*gc->overSampleFactor);
			glUniform1f(glGetUniformLocation(gc->GetShaderID(3), "BLUR_BIAS"), relHealth/500000.0f);
			gc->BindMultiPassShader(3, 4);*/
			gc->Rectangle2D(0, 0, gc->window.width, gc->window.height, KLGLColor(200, 0, 0, (relHealth/1000.0f)*255));
			gc->Blit2D(gameoverTexture, 0, 0);
		}
		gc->OrthogonalStart(gc->overSampleFactor);
		if (character->health <= 0 && gc->shaderClock >= 500 && gc->shaderClock <= 1000){

		}else{
			int offset = gc->buffer.width-152;
			for (int i = 0; i <= 16; i++){
				gc->BlitSprite2D(hudSpriteSheet, offset+(i*8), 16, 1);
				if (i == 16){
					gc->BlitSprite2D(hudSpriteSheet, offset+((i+1)*8), 16, 2);
				}else if (i == 0){
					gc->BlitSprite2D(hudSpriteSheet, offset+((i-1)*8), 16, 0);
				}

				if (i <= (character->health * 16) / 100){
					gc->BlitSprite2D(hudSpriteSheet, offset+(i*8), 16, 3);
				}
			}
			hudFont->Draw(offset-(16*8), 16, "@DSCORE: 00000");
		}
		gc->OrthogonalEnd();
	}

	void Environment::drawMap(KLGL* gc){
		glTranslatef(-scroll.x, -scroll.y, 0);
		if (mapDispListState == 1){
			glCallList(mapDispList);
		}else{
			glNewList(mapDispList, GL_COMPILE_AND_EXECUTE);
			// Draw each platform
			for(list<Platform>::iterator e = platforms->begin(); e != platforms->end(); e++){
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
			platforms->push_back(Platform(x0*tileWidth, y0*tileHeight, x1*tileWidth, y1*tileHeight, type, flags, mapData, mapMask));
		}
	}

	void Environment::load_map(char* mapPath){
		cl("Loading Map: %s+bmd ", mapPath);

		FILE* mapFile = fopen(mapPath, "rb");
		FILE* mapInfoFile = fopen(strcat(substr(mapPath, 0, strlen(mapPath)-3), "bmd"), "rb");

		if (mapFile == NULL || mapInfoFile == NULL){
			cl("[ERROR]\n");
			throw KLGLException("Error opening map file!");
			return;
		}

		platforms = new list<Platform>;
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

		// Parse map data into are geometry vectors
		for(int x = 0; x < map.width; x++){
			for(int y = 0; y < map.height; y++){
				char tile = mapData[lvLookup];
				char mask = mapMask[lvLookup];
				if(tile > 0){
					int xl = x;
					int yl = y;
					while(mapData[yl*map.width+x] != 0 && mapData[yl*map.width+x] == tile && mapMask[yl*map.width+x] == mask){
						yl++;
					}
					map_span(y*map.width+x, x, y, 1, yl-y);
					//platforms->push_back(Platform(x*tileWidth, y*tileHeight, 1*tileWidth, (yl-y)*tileHeight, lvLookup, mapData, mapMask));
					y = yl-1;
				}
			}
		}

		/*Rect<int> boundingBox = Rect<int>();
		int primaryTile = -1, primaryMask = -1;
		for(int x = 0; x < map.width; x++){
			for(int y = 0; y < map.height; y++){
				char tile = mapData[lvLookup];
				char mask = mapMask[lvLookup];
				if (tile > 0 && tile == primaryTile && mask == primaryMask){
					boundingBox.width = 1;
					boundingBox.height += 1;
				}else{
					map_span(lvLookup, boundingBox.x, boundingBox.y+1, boundingBox.width+1, boundingBox.height);
					primaryTile = tile;
					primaryMask = mask;
					boundingBox.x = x;
					boundingBox.y = y;
					boundingBox.width = 0;
					boundingBox.height = 0;
				}
			}
		}*/

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
			gameEnv->load_map("common/map01.smp");
			// Create our character
			gameEnv->character = new Character(128, 0, 8, 16);
			//gameEnv->enemys
			// Map sprites
			gameEnv->mapSpriteSheet = new KLGLSprite("common/tilemap.png", 16, 16);
			gameEnv->hudSpriteSheet = new KLGLSprite("common/hud.png", 8, 8);
			// Textures
			gameEnv->backdropTexture = new KLGLTexture("common/clouds.png");
			gameEnv->gameoverTexture = new KLGLTexture("common/gameover.png");
			// Load shader programs
			gameEnv->gcProxy->InitShaders(1, 0, 
				"common/postDefaultV.glsl",
				"common/production.frag"
				);
			gameEnv->gcProxy->InitShaders(2, 0, 
				"common/postDefaultV.glsl",
				"common/tile.frag"
				);
			gameEnv->gcProxy->InitShaders(3, 0, 
				"common/postDefaultV.glsl",
				"common/gaussianBlur.frag"
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

		neoVarList["characterMaxWalkSpeed"] = &maxWalkSpeed;
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

	void Character::drag(Environment &env)
	{
		// Regular slowdown
		if(vel.x > 0) // Moving right
		{
			if(!falling)
			{
				vel.x -= groundDrag*env.dt;
			}
			else  // Mid-air slowdown
				vel.x -= airDrag*env.dt;

			// Came to a stop
			if(vel.x < 0)
				vel.x = 0;
		}
		else if(vel.x < 0)  // Moving left
		{
			if(!falling)
			{
				vel.x += groundDrag*env.dt;
			}
			else  // Mid-air slowdown
				vel.x += airDrag*env.dt;

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
			drag(*env);
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
		vel.x = 0.0f;
		vel.y = 0.0f;
		collisionDetect = 0;
	}

	void Enemy::patrol(){
		// See CollisionType Platform::checkCollision(Environment &) for a better explanation of how this works.
		float cTop = envPtr->character->pos.y - envPtr->character->collisionRect.height;
		float cBottom = envPtr->character->pos.y;
		float cLeft = envPtr->character->pos.x - envPtr->character->collisionRect.width/2;
		float cRight = envPtr->character->pos.x + envPtr->character->collisionRect.width/2;
		float pTop = pos.y;
		float pBottom = pos.y + collisionRect.height;
		float pLeft = pos.x;
		float pRight = pos.x + collisionRect.width;

		if (collisionDetect != 1){
			vel.x = 1;
			pos.x += vel.x*envPtr->dt;
			if((cLeft - envPtr->character->vel.x*envPtr->dt + 1 >= pRight && cLeft <= pRight)	// Crossed right X and...
				&& (((pTop <= cTop && cTop <= pBottom)									// Corner within platform Y or...
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y or...
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				collisionDetect = 1;
			}
		}else if (collisionDetect != -1){
			vel.x = -1;
			pos.x -= vel.x*envPtr->dt;
			if((cRight - envPtr->character->vel.x*envPtr->dt - 1 <= pLeft && cRight >= pLeft)	// Crossed left X and...
				&& (((pTop <= cTop && cTop <= pBottom)									// Corner within platform Y or...
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				collisionDetect = -1;
			}
		}
	}

	void Enemy::comp(Environment *env){
		if (env != envPtr){
			envPtr = env;
		}
		patrol();
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
				glBegin(GL_LINE);
				glVertex2i(scrX, scrY);
				glVertex2i(scrX+16, scrY+16);
				glEnd();
			}
		}
	}

	int Platform::checkOverheadSlope(list<Platform>::iterator platformData){
		int offset = ((platformData->pos.y/16)-1)*(256)+(platformData->pos.x/16);
		//cl("%d\n", offset);
		return ((platformData->mapMaskPtr[offset] == TileFlag::F_LEFTSLOPE || platformData->mapMaskPtr[offset] == TileFlag::F_RIGHTSLOPE) ? 0 : 1);
	}

	CollisionType Platform::checkCollision(Environment &env, list<Platform>::iterator platformData)
	{
		if(env.character == NULL){
			throw KLGLException("Character does not exist!");
			return C_NONE;
		}else if (env.character->health < 0){
			return C_NONE;
		}

		// For algorithmic ease, here are the edges of the collision rects.
		/*  Here's the position (P) relative to the character.
		\O/
		|
		/ \
		P
		*/
		float cTop = env.character->pos.y - env.character->collisionRect.height;
		float cBottom = env.character->pos.y;
		float cLeft = env.character->pos.x - env.character->collisionRect.width/2;
		float cRight = env.character->pos.x + env.character->collisionRect.width/2;

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
				env.character->slopeRight(pRight-cRight, pTop);
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
				env.character->slopeLeft(cLeft-pLeft, pTop);
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
			
			if(slopeDetect && env.character->vel.y >= 0
				&& (cBottom - env.character->vel.y*env.dt - padding <= pTop && cBottom + padding >= pTop)  // Crossed top Y and...
				&& (((pLeft <= cLeft && cLeft <= pRight)  // Corner within platform X or...
				|| (pLeft <= cRight && cRight <= pRight))  // Other corner within platform X
				|| (cLeft <= pLeft && pRight <= cRight)))  // Platform is narrow and at the middle of the character
			{
				env.character->land(pTop);
				return C_DOWN;
			}
		}

		// Can I hit it on the way up?
		if(ceiling)
		{   
			if((cTop - env.character->vel.y*env.dt + 1 >= pBottom && cTop <= pBottom)  // Crossed bottom Y and...
				&& (((pLeft <= cLeft && cLeft <= pRight)  // Corner within platform X or...
				|| (pLeft <= cRight && cRight <= pRight))  // Other corner within platform X
				|| (cLeft <= pLeft && pRight <= cRight)))  // Ceiling is narrow and at the middle of the character
			{
				env.character->hitCeiling(pBottom);
				return C_UP;
			}
		}

		// Wall?
		if(wallLeft)
		{   
			if(slopeDetect && (cRight - env.character->vel.x*env.dt - 1 <= pLeft && cRight >= pLeft)	// Crossed left X and...
				&& (((pTop <= cTop && cTop <= pBottom)									// Corner within platform Y or...
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				env.character->wallRight(pLeft);
				return C_RIGHT;
			}
		}

		if(wallRight)
		{   
			if(slopeDetect && (cLeft - env.character->vel.x*env.dt + 1 >= pRight && cLeft <= pRight)	// Crossed right X and...
				&& (((pTop <= cTop && cTop <= pBottom)									// Corner within platform Y or...
				|| (pTop <= cBottom && cBottom <= pBottom))								// Other corner within platform Y or...
				|| (cTop <= pTop && pBottom <= cBottom)))								// Wall is short and at the middle of the character
			{
				env.character->wallLeft(pRight);
				return C_LEFT;
			}
		}

		// No collisions
		return C_NONE;
	}
}