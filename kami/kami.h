#pragma once

#ifdef _M_X64
#define KLGLENV64
#endif

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

#include "pure.h"
#include "version.h"

#if defined _WIN32
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"Winmm.lib")
#include "resource.h"
#include "win32.h"
#else
#include "unix.h"
#endif

// Internal modules
#include "config.h"
#include "objload.h"
#include "input.h"

namespace klib{

#pragma region KLGL_Classes

	// Pre define
	class GC;
	class WindowManager;
	class KLGLObj;

	class KLGLException {
	private:
		char* msg;
	public:
		KLGLException(const char* m, ...);
		char* getMessage();
	};

	inline char *file_contents(const char *filename)
	{
		if (filename == NULL) {
			return NULL;
		}
		FILE *f = fopen(filename, "rb");

		if (!f) {
			throw KLGLException("Unable to open %s for reading\n", filename);
			return NULL;
		}

		fseek(f, 0, SEEK_END);
		int length = ftell(f);
		fseek(f, 0, SEEK_SET);

		static char *buffer;
		if (buffer == NULL || (buffer != NULL && strlen(buffer) < length+1)){
			buffer = (char*)malloc(length+1);
		}

		fread(buffer, 1, length, f);
		fclose(f);
		buffer[length] = '\0';

		return buffer;
	}

	// Storage Type for loading textures in a format opengl can use(unsigned RGBA8)
	class Texture {
	public:
		GLuint gltexture;
		unsigned long width;
		unsigned long height;
		Texture();
		Texture(unsigned char *data, int size){
			InitTexture(data, size);
		}
		Texture(const char *fname){
			LoadTexture(fname);
		}
		~Texture();
	private:
		int LoadTexture(const char *fname);
		int InitTexture(unsigned char *data, size_t size);
	};

	// Colors
	class Color {
	public:
		Color(){
			Assign(0, 0, 0, 255);
		}
		Color(int tred, int tgreen, int tblue, int talpha = 255){
			Assign(tred, tgreen, tblue, talpha);
		}
		Color(Color *tVcolor){
			Assign(tVcolor->r, tVcolor->g, tVcolor->b, tVcolor->a);
		}
		void Set(){
			glColor4ub(r, g, b, a);
		}

		int r,g,b,a;
		void Assign(int tred, int tgreen, int tblue, int talpha = 255){
			r = tred;
			b = tblue;
			g = tgreen;
			a = talpha;
		}
	};

	// Sprites
	class Sprite {
	public:
		Texture* texturePtr;
		int swidth, sheight, width, height, localTexture;
		Sprite(){
			localTexture = 0;
		}
		Sprite(Texture* texture, int sWidth, int sHeight, int Internal = 0){
			localTexture = Internal;
			texturePtr = texture;
			swidth = sWidth;
			sheight = sHeight;
			width = texturePtr->width;
			height = texturePtr->height;
		}
		Sprite(const char *fname, int sWidth, int sHeight);
		~Sprite();
	};

	class KLGLObj : public Obj::File {
	public:
		Texture *texture;
		KLGLObj(){

		};
	protected:
		inline unsigned int OnLoadTexture(const char filename[]){
			texture = new Texture(filename);
			return texture->gltexture;
		}
	};

	class WindowManager {
	public:
		APP_WINDOWMANAGER_CLASS* wm;
		WindowManager(const char* title, Rect<int> *window, int scaleFactor, bool fullscreen){
			wm = new APP_WINDOWMANAGER_CLASS(title, window, scaleFactor, fullscreen);
		}
		void Swap(){
			wm->_swap();
		}
		// Events
		std::vector<KLGLKeyEvent> keyQueue;
		Point<int> mousePos;
		void ProcessEvent(int *status){
			if (*status == 0){
				return;
			}
			keyQueue.erase(std::remove_if(keyQueue.begin(), keyQueue.end(), [](KLGLKeyEvent e) {
				return e.isfinished();
			}), keyQueue.end());
			*status = wm->_event(&keyQueue, &mousePos.x, &mousePos.y);
		}
	};

	// General drawing helper and view mode manager
	class GC {
	public:
		int internalStatus;

		// Lua
#ifdef APP_ENABLE_LUA
		lua_State *lua;
#endif

		// Window regions
		WindowManager *windowManager;
		Rect<int> window;
		Rect<int> buffer;
		int overSampleFactor, scaleFactor;
		bool vsync;
		bool fullscreen;
		bool bufferAutoSize;

		// OpenGL constants
		unsigned int fbo[128]; // The frame buffer object  
		unsigned int fbo_depth[128]; // The depth buffer for the frame buffer object  
		unsigned int fbo_texture[128]; // The texture object to write our frame buffer object to
		unsigned int shader_id[128];
		unsigned int shader_vp[128];
		unsigned int shader_tp[128]; // 128 shaders should be enough!
		unsigned int shader_gp[128];
		unsigned int shader_fp[128];

		Vec4<GLfloat> light_diffuse;		// Default Diffuse light
		Vec4<GLfloat> light_position;		// Default Infinite light location.

		unsigned int rect_VBO;

		double shaderClock;
		Config *config;
		Texture *framebuffer, *klibLogo, *InfoBlue, *CheepCheepDebug;
		//KLGLSound *audio;
		double fps;

		GC(const char* title = "Application", int width = 640, int height = 480, int framerate = 60, bool fullscreen = false, int OSAA = 1, int scale = 1, float anisotropy = 4.0);
		~GC();

		// Sleep
		void sleep(int ms){
			float goal;
			goal = ms + getElapsedTimeInMs();
			while(goal >= getElapsedTimeInMs());
		}

		// Prepare current frame for drawing
		void OpenFBO(float fov = 0.0f, float eyex = 0.0f, float eyey = 0.0f, float eyez = 0.0f);
		// Render FBO to the display and reset
		void Swap();
		void GenFrameBuffer(GLuint& fbo, GLuint &fbtex, GLuint &fbo_depth, int bufferWidth = 0, int bufferHeight = 0);
		// Blit
		void Blit2D(Texture* texture, int x, int y, float rotation = 0.0f, float scale = 1.0f, Color vcolor = Color(255, 255, 255, 255), Rect<float> mask = Rect<float>(0,0,0,0));
		void BlitSprite2D(Sprite* sprite, int x, int y, int row, int col, bool managed = 1, Rect<int> mask = Rect<int>(0,0,0,0));
		void BlitSprite2D(Sprite* sprite, int x, int y, int index = 0, bool managed = 1, Rect<int> mask = Rect<int>(0,0,0,0)){
			int length = sprite->width/sprite->swidth;
			int col = index%length;
			int row = floor(index/length);

			BlitSprite2D(sprite, x, y, row, col, managed, mask);
		};
		void Tile2D(Texture* texture, int x, int y, int w, int h);
		void Rectangle2D(int x, int y, int width, int height, Color vcolor = Color(255, 0, 0, 255));
		void OrthogonalStart(float scale = 1.0f);
		void OrthogonalEnd();
		void BindMultiPassShader(int shaderProgId = 0, int alliterations = 1, bool flipOddBuffer = true, float x = 0.0f, float y = 0.0f, float width = -1.0f, float height = -1.0f, int textureSlot = 0);

		// Automatically load and setup a shader program for given vertex and frag shader scripts
		int InitShaders(int shaderProgId, int isString, const char *vsFile = NULL, const char *fsFile = NULL, const char *gpFile = NULL, const char *tsFile = NULL);
		unsigned int ComputeShader(unsigned int &shaderProgram, unsigned int shaderType, const char* shaderString);
		unsigned int GetShaderID(int shaderProgId = 0){
			return shader_id[shaderProgId];
		}
		void BindShaders(int shaderProgId = 0) {
			glUseProgram(shader_id[shaderProgId]);
		}
		void UnbindShaders() {
			glUseProgram(0);
		}
		void UnloadShaders(int id){
			glDetachShader(shader_id[id], shader_vp[id]);
			glDetachShader(shader_id[id], shader_tp[id]);
			glDetachShader(shader_id[id], shader_gp[id]);
			glDetachShader(shader_id[id], shader_fp[id]);

			glDeleteShader(shader_vp[id]);
			glDeleteShader(shader_tp[id]);
			glDeleteShader(shader_gp[id]);
			glDeleteShader(shader_fp[id]);

			glDeleteProgram(shader_id[id]);
		}
		void GenVertVBO(GLuint *vbo, void* vertexPositions, int vertexSize){
			glGenBuffers(1, vbo);

			glBindBuffer(GL_ARRAY_BUFFER, *vbo);
			glBufferData(GL_ARRAY_BUFFER, vertexSize*3*sizeof(float), vertexPositions, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		void GenTexCoordVBO(GLuint *vbo, void* vertexPositions, int vertexSize){
			glGenBuffers(1, vbo);

			glBindBuffer(GL_TEXTURE_COORD_ARRAY, *vbo);
			glBufferData(GL_TEXTURE_COORD_ARRAY, vertexSize*2*sizeof(float), vertexPositions, GL_STATIC_DRAW);
			glBindBuffer(GL_TEXTURE_COORD_ARRAY, 0);
		}

		int ProcessEvent(int *status);
		std::function<void(std::vector<KLGLKeyEvent>::iterator)> HandleEvent(std::function<void(std::vector<KLGLKeyEvent>::iterator)> func){
			std::vector<KLGLKeyEvent>::iterator it = windowManager->keyQueue.begin();
			for(;it != windowManager->keyQueue.end(); ++it){
				func(it);
			}
			return func;
		}

	private:
		float getElapsedTimeInMs(){
			return clock()/(CLOCKS_PER_SEC/1000);
		}
		void LoadShaderFile(char **dest, const char* fname);
		static void PrintShaderInfoLog(GLuint obj, int isShader = 1);
		Texture nullTexture;
		Rect<int> ASPRatio(Rect<int> &rcScreen, Rect<int> &sizePicture, bool bCenter = true);
	};

	// Simple text drawing
	class Font
	{
	public:
		struct Descriptor
		{
			//clean 16 bytes
			short x, y;
			short Width, Height;
			short XOffset, YOffset;
			short XAdvance;
			short Page;

			Descriptor() : x( 0 ), y( 0 ), Width( 0 ), Height( 0 ), XOffset( 0 ), YOffset( 0 ),
				XAdvance( 0 ), Page( 0 )
			{ }
		};
		struct Charset
		{
			short LineHeight;
			short Base;
			short Width, Height;
			short Pages;
			Descriptor Chars[100000];
		} charsetDesc;

		int bmfont;

		Font();
		Font(GLuint init_texture, GLuint init_m_width, GLuint init_m_height, GLuint init_c_width, GLuint init_c_height, int init_extended = 0);
		void Draw(int x, int y, char* text, Color* vcolor = NULL);
		void Draw(int x, int y, wchar_t* text, Color* vcolor = NULL);
		bool ParseFnt(std::istream& Stream);
		~Font();
		//private:
		GLuint c_texture;
		GLint c_per_row;

		//bitmap setting
		GLuint m_width;
		GLuint m_height;

		//character settings
		Color *color;
		GLuint c_width;
		GLuint c_height;
		int extended;
	};

#pragma endregion

}
