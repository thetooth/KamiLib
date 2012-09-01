#pragma once

#define APP_ENABLE_LUA 1
#include "kami.h"
#include "logicalObjects.h"
#include "particles.h"
#include <list>
#include <map>
#include <unordered_map>

using namespace std;
using namespace klib;

#define FPM 9
#define MAP_PHASE_ADJ_SPEED			64

namespace emb
{

	typedef std::function<void(std::string)> stdout_write_type;

	struct Stdout
	{
		PyObject_HEAD
			stdout_write_type write;
	};

	PyObject* Stdout_write(PyObject* self, PyObject* args);
	PyObject* Stdout_flush(PyObject* self, PyObject* args);

	static PyMethodDef Stdout_methods[] =
	{
		{"write", Stdout_write, METH_VARARGS, "sys.stdout.write"},
		{"flush", Stdout_flush, METH_VARARGS, "sys.stdout.write"},
		{0, 0, 0, 0} // sentinel
	};

	static PyTypeObject StdoutType =
	{
		PyVarObject_HEAD_INIT(0, 0)
		"emb.StdoutType",     /* tp_name */
		sizeof(Stdout),       /* tp_basicsize */
		0,                    /* tp_itemsize */
		0,                    /* tp_dealloc */
		0,                    /* tp_print */
		0,                    /* tp_getattr */
		0,                    /* tp_setattr */
		0,                    /* tp_reserved */
		0,                    /* tp_repr */
		0,                    /* tp_as_number */
		0,                    /* tp_as_sequence */
		0,                    /* tp_as_mapping */
		0,                    /* tp_hash  */
		0,                    /* tp_call */
		0,                    /* tp_str */
		0,                    /* tp_getattro */
		0,                    /* tp_setattro */
		0,                    /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT,   /* tp_flags */
		"emb.Stdout objects", /* tp_doc */
		0,                    /* tp_traverse */
		0,                    /* tp_clear */
		0,                    /* tp_richcompare */
		0,                    /* tp_weaklistoffset */
		0,                    /* tp_iter */
		0,                    /* tp_iternext */
		Stdout_methods,       /* tp_methods */
		0,                    /* tp_members */
		0,                    /* tp_getset */
		0,                    /* tp_base */
		0,                    /* tp_dict */
		0,                    /* tp_descr_get */
		0,                    /* tp_descr_set */
		0,                    /* tp_dictoffset */
		0,                    /* tp_init */
		0,                    /* tp_alloc */
		0,                    /* tp_new */
	};

	static PyModuleDef embmodule =
	{
		PyModuleDef_HEAD_INIT,
		"emb", 0, -1, 0,
	};

	// Internal state
	static PyObject* g_stdout;
	static PyObject* g_stdout_saved;

	PyMODINIT_FUNC PyInit_emb(void);
	void set_stdout(stdout_write_type write);
	void reset_stdout();

}

namespace NeoPlatformer{

	// Classes in this file
	class Environment;
	class ObjectInterface;
	class Character;
	class Enemy;
	class Platform;

	// Global pointer to our environment(used for python)
	extern map<const char*, void*, cmp_cstring> neoCallbackList;

	static PyObject *neoErrorHandler;
	static PyObject *neoClHandler(PyObject *self, PyObject *args){
		const char *arg1;
		int sts = 0;

		if (!PyArg_ParseTuple(args, "s", &arg1)){
			return NULL;
		}

		sts = cl(arg1);

		if (sts < 0) {
			PyErr_SetString(neoErrorHandler, "System command failed");
			return NULL;
		}
		return PyLong_FromLong(sts);
	}
	static PyObject *neoSetHandler(PyObject *self, PyObject *args){
		const char *key;
		int val;
		int sts = 0;

		if (!PyArg_ParseTuple(args, "si", &key, &val)){
			return NULL;
		}

		Environment *gameEnv = (Environment*)neoCallbackList["gameEnv"];
		if (gameEnv == NULL || key == NULL){
			sts = -1;
		}else{
			*((int*)neoCallbackList[key]) = val;
		}

		if (sts < 0) {
			PyErr_SetString(neoErrorHandler, "Failed to set value!");
			return NULL;
		}
		return PyLong_FromLong(sts);
	}
	static PyObject *neoGetHandler(PyObject *self, PyObject *args){
		const char *key;
		int val;
		int sts = 0;

		if (!PyArg_ParseTuple(args, "s", &key)){
			return NULL;
		}

		if (key == NULL){
			sts = -1;
		}else{
			val = *((int*)neoCallbackList[key]);
		}

		if (sts < 0) {
			PyErr_SetString(neoErrorHandler, "Failed to set value!");
			return NULL;
		}
		return PyLong_FromLong(val);
	}
	
	static PyMethodDef neoMethods[] = {
		{"cl", neoClHandler, METH_VARARGS, "Print to console."},
		{"set", neoSetHandler, METH_VARARGS, "Set values exposed by game engine."},
		{"get", neoGetHandler, METH_VARARGS, "Get values exposed by game engine."},
		{NULL, NULL, 0, NULL}
	};

	static struct PyModuleDef neoModule = {
		PyModuleDef_HEAD_INIT, "neo", NULL, -1, neoMethods
	};

	inline PyObject* PyInit_neo(void){
		return PyModule_Create(&neoModule);
	}

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

			cl("Initialized FMOD %d.%d.%d\n", HIWORD(FMOD_VERSION), HIBYTE(LOWORD(FMOD_VERSION)), LOBYTE(LOWORD(FMOD_VERSION)));
		};

		inline int loadSound(char* file, int key = -1, unsigned int flags = FMOD_DEFAULT){
			cl("Loading Audio: %s ", file);
			if (file == NULL || strlen(file) < 1 || system == NULL){
				throw KLGLException("SHEEIT");
			}
			result = system->createSound(file, flags, 0, &sound[(key == -1 ? soundKey : key)]);
			FMODCHECK(result);
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
		vector<Enemy*> enemys;
		vector<Platform> platforms;
		char *mapName;
		char *mapData;
		char *mapMask;
		char *mapProg;

		// Textures
		KLGLFont *hudFont;
		KLGLSprite *mapSpriteSheet;
		KLGLSprite *hudSpriteSheet8;
		KLGLSprite *hudSpriteSheet16;
		KLGLTexture *backdropTexture;
		KLGLTexture *gameoverTexture;

		// Display list
		int mapDispListState;
		unsigned int mapDispList;

		Environment(char *map);
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
		// Environment pointer
		Environment *envPtr;

		Point<float> pos;  // Position
		Point<float> vel;  // Velocity

		Rect<int> collisionRect;
		Rect<int> standingRect;

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

		virtual void comp(Environment *env){};
		virtual void land(int platformY){};
		virtual void hitCeiling(int platformY){};
		virtual void wallLeft(int wallX){};
		virtual void wallRight(int wallX){};
		virtual void slopeLeft(int charX, int wallY){};
		virtual void slopeRight(int charX, int wallY){};
	};

	class Character : public ObjectInterface
	{
	public:
		// Players profile(score, lvl, ammo, etc)
		int score;
		int health;

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
		void drag();
		void comp(Environment *env);
	};

	class Enemy : public ObjectInterface {
	public:
		Enemy(float x, float y, int w, int h);
		~Enemy();

		void draw(KLGL* gc, KLGLSprite *sprite, int frame);
		void patrol();
		void land(int platformY);
		void wallLeft(int wallX);
		void wallRight(int wallX);
		void comp(Environment *env);
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
		CollisionType checkCollision(ObjectInterface *object, double dt, vector<Platform>::iterator platformData);
		int checkOverheadSlope(vector<Platform>::iterator platformData);
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
