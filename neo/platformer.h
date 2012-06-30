#pragma once

#define APP_ENABLE_LUA 1
#include "kami.h"
#include "logicalObjects.h"
#include <list>
#include <map>
#include <unordered_map>

using namespace std;
using namespace klib;

#define FPM 9
#define MAP_PHASE_ADJ_SPEED			64

namespace NeoPlatformer{

	// Classes in this file
	class Environment;
	class ObjectInterface;
	class Character;
	class Platform;

	// Global pointer to our environment(used for lua)
	extern Environment *envCallbackPtr;
	extern map<const char*, void*, cmp_cstring> neoVarList;

	/*static void neo_register_info (lua_State *L) {
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

	int l_get( lua_State *L );
	int l_set( lua_State *L );

	const struct luaL_Reg neolib[] =
	{
		{ "__index",	l_get	},
		{ "__newindex",	l_set	},
		{"cl",			clL		},
		{ NULL,			NULL	}
	};

	int luaopen_neo(lua_State *L);*/

	// The direction of the obstacle that I collided with.  See Character::checkCollision()
	enum CollisionType {C_NONE, C_UP, C_DOWN, C_LEFT, C_RIGHT, C_SLOPELEFT, C_SLOPERIGHT};
	enum TileType {T_NULL, T_NONSOLID, T_UNDER, T_LEFTSLOPE, T_RIGHTSLOPE, T_LAVA};
	enum TileFlag {
		F_NONSOLID		= 0x00,
		F_CEIL			= 0x01,
		F_FLOOR			= 0x02,
		F_LEFT			= 0x04,
		F_RIGHT			= 0x08,
		F_LEFTSLOPE		= 0x10,
		F_RIGHTSLOPE	= 0x20,
	};

	class Audio {
	public:
		FMOD::System     *system;
		FMOD::Sound      *sound[128];
		FMOD::Channel    *channel[128];
		FMOD_RESULT       result;
		int               soundKey;
		int               channelKey;
		unsigned int      version;

		Audio(){
			result = FMOD::System_Create(&system);
			FMODCHECK(result);

			result = system->getVersion(&version);
			FMODCHECK(result);

			if (version < FMOD_VERSION){
				throw KLGLException("You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
				return;
			}

			result = system->setSoftwareChannels(100);		// Allow 100 software mixed voices to be audible at once.
			FMODCHECK(result);

			result = system->setHardwareChannels(32);	// Require the soundcard to have at least 32 2D and 3D hardware voices, and clamp it to using 64 if it has more than this.
			FMODCHECK(result);

			result = system->init(200, FMOD_INIT_NORMAL, 0);
			FMODCHECK(result);

			soundKey = 0;
			channelKey = 0;

			cl("Initialized FMOD %08x\n", FMOD_VERSION);
		};

		inline int loadSound(char* file, int key = -1, unsigned int flags = FMOD_DEFAULT){
			cl("Loading Audio: %s ", file);
			if (file == NULL || strlen(file) < 1 || system == NULL){
				throw KLGLException("");
			}
			system->createSound(file, flags, 0, &sound[(key = -1 ? soundKey : key)]);
			soundKey++;
			cl("[OK]\n");
			return soundKey-1;
		}

	private:
		inline void FMODCHECK(FMOD_RESULT result)
		{
			if (result != FMOD_OK){
				printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
				exit(-1);
			}
		}
	};

	class Environment
	{
	public:

		//
		KLGL *gcProxy;
		Audio *audioProxy;

		// Lets keep our main() clean by declearing all state logic here :D
		int mode;
		Point<int> debugflags;

		// Env geometrys
		float dt;
		float dtMulti;
		int tileWidth, tileHeight;
		Rect<int> map;
		Point<int> scroll;
		Point<int> scroll_real;
		Point<int> target;
		int scrollspeed;
		int scrollspeedMulti;

		// Map data
		Character* character;
		//Enemy* enemys[64];
		list<Platform>* platforms;
		char *mapData;
		char *mapMask;
		char *mapProg;

		// Textures
		KLGLFont *hudFont;
		KLGLSprite *mapSpriteSheet;
		KLGLSprite *hudSpriteSheet;
		KLGLTexture *backdropTexture;
		KLGLTexture *gameoverTexture;

		// Display list
		int mapDispListState;
		unsigned int mapDispList;

		Environment();
		~Environment();
		void comp(KLGL* gc, int offsetX = 0, int offsetY = 0);
		void drawHUD(KLGL* gc);
		void drawMap(KLGL* gc);
		void drawLoader(KLGL* gc, int y, int height, int length, int speed);
		void load_map(char* mapfile);
		void map_span(int type, int x0, int y0, int x1, int y1);
	};

	class EnvLoaderThread : public KLGLThread {
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
		Environment *envPtr;

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
		void comp(Environment *env);
	};

	class Enemy : public ObjectInterface {
	public:
		// Enviroment pointer
		Environment *envPtr;

		int  collisionDetect; // -1 Left 0 None +1 Right

		Enemy(float x, float y, int w, int h);
		~Enemy();

		void comp(Environment *env);
		void patrol();
	};

	class Platform
	{
	public:
		int tileId;
		int tileFlags;
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

		Platform(float x, float y, int w, int h, int sprId, int flags, char* mapDataPtrI, char* mapMaskPtrI);
		~Platform(){};
		void scroll_normal();
		void draw(KLGL* gc, Environment* env, KLGLSprite* sprite);
		CollisionType checkCollision(Environment &env, list<Platform>::iterator platformData);
		int checkOverheadSlope(list<Platform>::iterator platformData);
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
