#pragma once

#ifdef _M_X64
#define KLGLENV64
#endif

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#pragma region Constants

#ifndef APP_MOTD
#define APP_MOTD "(c) 2005-2012 Ameoto Systems Inc. All Rights Reserved.\n\n"
#endif
#define APP_BUFFER_SIZE 4096
#define APP_ENABLE_MIPMAP 0
#define APP_ANISOTROPY 4.0
//#define APP_ENABLE_LUA 1

#pragma endregion

#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"Winmm.lib")
#ifdef APP_ENABLE_LUA
#pragma comment(lib,"SLB.lib")
#endif
#ifdef APP_ENABLE_SQUIRREL
#pragma comment(lib,"squirrel.lib")
#pragma comment(lib,"sqstdlib.lib")
#endif

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string.h>
#include <time.h>
#ifdef _DEBUG
//#include <vld.h>
#endif

#include "pure.h"
#include "version.h"
#ifdef _WIN32
#include "win32.h"
#endif
#define SLB_STATIC_LIBRARY
#include "../../SLB/SLB/SLB.hpp"
#ifdef APP_ENABLE_LUA
#include "lualoader.h"
#endif
#ifdef APP_ENABLE_SQUIRREL
#include "SQUIRREL/squirrel.h"
#include "SQUIRREL/sqstdio.h"
#include "SQUIRREL/sqstdaux.h"
#ifdef SQUNICODE
#define scvprintf vswprintf
#else
#define scvprintf vsprintf
#endif
#endif
#include "GL/glew.h"
#include "GL/wglew.h"
#include "inireader.h"
#include "threads.h"
#include "logicalObjects.h"
#include "sound.h"

namespace klib{

#pragma region KLGL_Interop

#ifdef APP_ENABLE_LUA
	// Lua interop
	static void kami_register_info(lua_State *L) {
		lua_pushliteral (L, "_COPYRIGHT");
		lua_pushliteral (L, "Copyright (C) 2005-2011 Ameoto Systems Inc. All Rights Reserved.");
		lua_settable (L, -3);
		lua_pushliteral (L, "_DESCRIPTION");
		lua_pushliteral (L, "test");
		lua_settable (L, -3);
		lua_pushliteral (L, "_VERSION");
		lua_pushliteral (L, "Kami Interop v1");
		lua_settable (L, -3);
	}

	static int clL(lua_State *L) {
		const char *str = luaL_checkstring(L, 1);
		lua_pushinteger(L, cl("%s", str));
		return 1;
	}

	static const struct luaL_Reg kamiL[] = {
		{"cl", clL},
		{NULL, NULL},
	};

	inline int luaL_openkami(lua_State *L) {
		//luaL_newlib(L, kamiL);
		//luaL_setfuncs(L, kamiL, 0);
		luaL_openlib(L, "kami", kamiL, 0);
		kami_register_info(L);
		return 1;
	}
#endif
#ifdef APP_ENABLE_SQUIRREL
	inline void SQcl(HSQUIRRELVM v, const SQChar *s, ...){
		static SQChar *tmpBuffer;
		if (tmpBuffer == NULL){
			tmpBuffer = new SQChar[strlen(s)+4096];
		}
		va_list arglist;
		va_start(arglist, s);
		scvprintf(tmpBuffer, s, arglist);
		cl(tmpBuffer);
		va_end(arglist);
	}
#endif

#pragma endregion

#pragma region KLGL_Classes

	// Pre define
	class KLGL;

	class KLGLException {
	private:
		char* msg;
	public:
		KLGLException(const char* m, ...);
		char* getMessage();
	};

	inline char *file_contents(char *filename)
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

		char *buffer = (char*)malloc(length+1);
		fread(buffer, 1, length, f);
		fclose(f);
		buffer[length] = '\0';

		return buffer;
	}

	// Storage Type for loading textures in a format opengl can use(unsigned RGBA8)
	class KLGLTexture {
	public:
		GLuint gltexture;
		unsigned long width;
		unsigned long height;
		KLGLTexture();
		KLGLTexture(unsigned char *data, int size){
			InitTexture(data, size);
		}
		KLGLTexture(const char *fname){
			LoadTexture(fname);
		}
		~KLGLTexture();
	private:
		int LoadTexture(const char *fname);
		int InitTexture(unsigned char *data, size_t size);
	};

	// Colors
	class KLGLColor {
	public:
		KLGLColor(){
			Assign(0, 0, 0, 255);
		}
		KLGLColor(int tred, int tgreen, int tblue, int talpha = 255){
			Assign(tred, tgreen, tblue, talpha);
		}
		KLGLColor(KLGLColor &tVcolor){
			Assign(tVcolor.r, tVcolor.g, tVcolor.b, tVcolor.a);
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
	class KLGLSprite {
	public:
		KLGLTexture* texturePtr;
		int swidth, sheight, width, height, localTexture;
		KLGLSprite(){
			localTexture = 0;
		}
		KLGLSprite(KLGLTexture* texture, int sWidth, int sHeight, int Internal = 0){
			localTexture = Internal;
			texturePtr = texture;
			swidth = sWidth;
			sheight = sHeight;
			width = texturePtr->width;
			height = texturePtr->height;
		}
		KLGLSprite(const char *fname, int sWidth, int sHeight);
		~KLGLSprite();
	};

	// General drawing helper and view mode manager
	class KLGL {
	public:
		int internalStatus;

		// Win32 access
		WNDCLASS wc;
		HWND hWnd;
		HDC hDC;
		HGLRC hRC,hRCAUX;
		MSG msg;

		// Lua
#ifdef APP_ENABLE_LUA
		lua_State *lua;
#endif
#ifdef APP_ENABLE_SQUIRREL
		HSQUIRRELVM squirrel;
#endif

		// Window regions
		Rect<int> window;
		Rect<int> buffer;
		int overSampleFactor, scaleFactor;
		bool vsync;

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

		int shaderClock;
		KLGLINIReader *config;
		KLGLTexture *framebuffer, *klibLogo, *InfoBlue, *CheepCheepDebug;
		KLGLSound *audio;
		double fps;

		KLGL(const char* title = "Application", int width = 640, int height = 480, int framerate = 60, bool fullscreen = false, int OSAA = 1, int scale = 1, float anisotropy = 4.0);
		~KLGL();

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
		void GenFrameBuffer(GLuint& fbo, GLuint &fbtex, GLuint &fbo_depth);
		void Resize(){
			//windowResize(hWnd);
		}
		// Blit
		void Blit2D(KLGLTexture* texture, int x, int y, float rotation = 0.0f, float scale = 1.0f, KLGLColor vcolor = KLGLColor(255, 255, 255, 255));
		void BlitSprite2D(KLGLSprite* sprite, int x, int y, int row, int col, bool managed = 1);
		void BlitSprite2D(KLGLSprite* sprite, int x, int y, int index = 0, bool managed = 1){
			int length = sprite->width/sprite->swidth;
			int col = index%length;
			int row = (index-col)/length;

			BlitSprite2D(sprite, x, y, row, col, managed);
		};
		void Tile2D(KLGLTexture* texture, int x, int y, int w, int h);
		void Rectangle2D(int x, int y, int width, int height, KLGLColor vcolor = KLGLColor(255, 0, 0, 255));
		void OrthogonalStart(float scale = 1.0f);
		void OrthogonalEnd();
		void BindMultiPassShader(int shaderProgId = 0, int alliterations = 1, bool flipOddBuffer = true, float x = 0.0f, float y = 0.0f, float width = -1.0f, float height = -1.0f);

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
		void ProcessEvent(int *status);

	private:
		float getElapsedTimeInMs(){
			return clock()/(CLOCKS_PER_SEC/1000);
		}
		static char* LoadShaderFile(const char* fname);
		static void PrintShaderInfoLog(GLuint obj, int isShader = 1);
		KLGLTexture nullTexture;

	};

	// Simple text drawing
	class KLGLFont
	{
	public:
		KLGLFont();
		KLGLFont(GLuint init_texture, GLuint init_m_width, GLuint init_m_height, GLuint init_c_width, GLuint init_c_height, int init_extended = 0);
		void Draw(int x, int y, char* text, KLGLColor* vcolor = NULL);
		void Draw(int x, int y, wchar_t* text, KLGLColor* vcolor = NULL);
		~KLGLFont();
		//private:
		GLuint c_texture;
		GLint c_per_row;

		//bitmap setting
		GLuint m_width;
		GLuint m_height;

		//character settings
		KLGLColor *color;
		GLuint c_width;
		GLuint c_height;
		int extended;
	};

#pragma endregion

}