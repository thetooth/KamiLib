#pragma once

#include "kami.h"
#include "logicalObjects.h"
#include <list>

using namespace std;
using namespace klib;

#define FPM 9
#define MAP_PHASE_ADJ_SPEED			64

namespace NeoPlatformer{
	
	class ObjectInterface;
	class Character;
	class Platform;

	// Lua interop
	static void set_info (lua_State *L) {
		lua_pushliteral (L, "_COPYRIGHT");
		lua_pushliteral (L, "Copyright (C) 2005-2011 Ameoto Systems Inc. All Rights Reserved.");
		lua_settable (L, -3);
		lua_pushliteral (L, "_DESCRIPTION");
		lua_pushliteral (L, "test");
		lua_settable (L, -3);
		lua_pushliteral (L, "_VERSION");
		lua_pushliteral (L, "Neo Interop v1");
		lua_settable (L, -3);
	}

	static int test(lua_State *L) {
		const char *str = luaL_checkstring(L, 1);
		cl("- %s", str);
		lua_pushfstring (L,"%s", str);
		return 1;
	}

	static const struct luaL_Reg neolib[] = {
		{"test", test},
		{NULL, NULL},
	};

	inline int luaopen_neo(lua_State *L) {
		luaL_register(L, "neo", neolib);
		set_info(L);
		return 1;
	}

	// Sometimes an environment class makes things cleaner.  Mine will be
	// globally instanced, but you could make a local one and pass it around with 
	// a pointer.
	class Environment
	{
	public:

		// Lets keep our main() clean by declearing all state logic here :D
		int mode;
		Point<int> debugflags;

		double dt;
		int tileWidth, tileHeight;
		Rect<int> map;
		Point<int> scroll;
		Point<int> scroll_real;
		Point<int> target;
		int scrollspeed;
		int scrollspeedMulti;

		Character* character;
		list<Platform>* platforms;
		char *mapData;
		char *mapMask;

		Environment(){
			mode = 0;
			dt = 0.0;
			scroll.x = scroll.y = 0;
			scroll_real.x = scroll_real.y = 0;
			target.x = target.y = 0;
			scrollspeed = 20;
			scrollspeedMulti = 80;
			character = NULL;
			platforms = NULL;
		}
		~Environment();
		void comp(int offsetX = 0, int offsetY = 0);
		void drawHUD(KLGL* gc, KLGLTexture* tex);
		void drawMap(KLGL* gc, KLGLSprite* spriteSheet);
		void load_map(char* mapfile);
		void map_span(int type, int x0, int y0, int x1, int y1);
	};

	class EnvLoaderThread : KLGLThread {
	public:
		int status;
		void *loaderPtr;
		EnvLoaderThread(){};
		EnvLoaderThread(void *parma){
			status = true;
			loaderPtr = parma;
			start();
		};
		~EnvLoaderThread() {
			status = false;
			if(m_hThread != NULL) {
				stop();
			}
		}
		void run();
	};

	// Interface for objects including the player
	class ObjectInterface {
	public:
		Point<float> pos;  // Position
		Point<float> vel;  // Velocity

		Rect<int> collisionRect;
		Rect<int> standingRect;
	};

	class Character : public ObjectInterface
	{
	public:
		// Enviroment pointer
		Environment envPtr;

		// Players profile(score, lvl, ammo, etc)
		int score;
		int health;

		// Physical constants
		// Adjust these in the constructor (or set them later) to make the
		// gameplay feel good.
		float gravity;
		float jumpAccel;
		float walkAccel;
		float maxWalkSpeed;
		float brakeAccel;
		float groundDrag;
		float slideDrag;
		float airDrag;
		float airAccel;

		// States of the character
		int  collisionDetect; // -1 Left 0 None +1 Right
		bool falling;
		bool fallLock;
		bool walking;
		bool facingRight;
		bool rightPressed;
		bool leftPressed;
		bool landedLastFrame;

		// Animation
		int walkAnime;

		Character(float x, float y, int w, int h);
		~Character();
		void draw(Environment* env, KLGL* gc, KLGLSprite *sprite, int frame);
		void jump();
		void jumpUp();
		void wallJump();
		void land(int platformY);
		void hitCeiling(int platformY);
		void wallLeft(int wallX);
		void wallRight(int wallX);
		void slopeLeft(int charX, int wallY);
		void slopeRight(int charX, int wallY);
		void left();
		void leftUp();
		void right();
		void rightUp();
		void drag(Environment &env);
		void comp(Environment &env);
	};

	// The direction of the obstacle that I collided with.  See Character::checkCollision()
	enum CollisionType {C_NONE, C_UP, C_DOWN, C_LEFT, C_RIGHT, C_SLOPELEFT, C_SLOPERIGHT};
	enum TileType{t_null, t_nonsolid, t_under, t_slopeleft, t_sloperight};

	class Platform
	{
	public:
		int tileId;
		char* mapDataPtr;
		char* mapMaskPtr;
		bool ceiling; // No pass through : Character collides with bottom while moving upward
		bool floor;  // Collide with top
		bool wallLeft;  // Collide with left
		bool wallRight;  // Collide with right
		bool slopeLeft;
		bool slopeRight;
		Rect<int> collisionRect;
		Point<float> pos;

		Platform(float x, float y, int w, int h, int sprId, bool* flags, char* mapDataPtrI, char* mapMaskPtrI);
		~Platform(){};
		void checkCollisionType();
		void scroll_normal();
		void draw(KLGL* gc, Environment* env, KLGLSprite* sprite);
		CollisionType checkCollision(Environment &env);
	private:
	};

	inline void NeoSoundServer(void *pSoundBuffer, long bufferLen){
		float pie2 = (3.1415926f * 2.f);
		float sinStep = ((pie2 * 1640.f) / 44100.f);
		float sinPos = 0.f;

		// Convert params, assuming we create a 16bits, mono waveform.
		signed short *pSample = (signed short*)pSoundBuffer;
		long nbSample = bufferLen / sizeof(signed short);

		for (int i=0;i<nbSample;i++)
		{
			*pSample++ = (signed short)(16384.0f * sin(sinPos));
			sinPos += sinStep;
		}
		if (sinPos >= pie2) sinPos -= pie2;
	}
}