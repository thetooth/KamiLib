#pragma once

#ifdef _M_X64
#define KLGLENV64
#endif

#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"opengl32.lib")
//#pragma comment(lib,"opencl.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"kami-lua.lib")
//#pragma comment(lib,"kami-openal.lib")

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string.h>
#include <time.h>
#ifdef _DEBUG
#include <vld.h>
#endif

#include "pure.h"
#ifdef _WIN32
#include "win32.h"
#endif
#include "lualoader.h"
#include "GL/glew.h"
#include "GL/wglew.h"
#include "inireader.h"
#include "threads.h"
#include "logicalObjects.h"
#include "sound.h"

#pragma region Constants

#define APP_ENABLE_MIPMAP 0
#define APP_ANISOTROPY 4.0

#pragma endregion

namespace klib{

#pragma region KLGL_Interop

	// Lua interop
	static void KLGLLuaInfo(lua_State *L) {
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
		luaL_register(L, "kami", kamiL);
		KLGLLuaInfo(L);
		return 1;
	}

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
		int InitTexture(unsigned char *data, int size);
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
	private:
		void Assign(int tred, int tgreen, int tblue, int talpha){
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
		HGLRC hRC;
		MSG msg;

		// Lua
		lua_State *lua;

		// Window regions
		Rect<int> window;
		int buffer_width, buffer_height;
		int overSampleFactor, scaleFactor;
		bool vsync;

		unsigned int fbo; // The frame buffer object  
		unsigned int fbo_depth; // The depth buffer for the frame buffer object  
		unsigned int fbo_texture; // The texture object to write our frame buffer object to
		unsigned int shader_id[128];
		unsigned int shader_vp[128]; // 128 shaders should be enough!
		unsigned int shader_fp[128];

		float shaderClock;
		KLGLINIReader *config;
		KLGLTexture *klibLogo, *InfoBlue, *CheepCheepDebug;
		KLGLSound *audio;
		int fps;

		KLGL(const char* title = "Application", int width = 640, int height = 480, int framerate = 60, bool fullscreen = false, int OSAA = 1, int scale = 1, float anisotropy = 4.0);
		~KLGL();

		// Sleep
		void sleep(int ms){
			float goal;
			goal = ms + getElapsedTimeInMs();
			while(goal >= getElapsedTimeInMs());
		}

		// Prepare current frame for drawing
		void OpenFBO(float fov, float eyex, float eyey, float eyez);
		// Render FBO to the display and reset
		void Swap();
		void Resize(){
			windowResize(hWnd);
		}
		// Blit
		void Blit2D(KLGLTexture* texture, int x, int y, float rotation = 0.0f, KLGLColor vcolor = KLGLColor(255, 255, 255, 255));
		void BlitSprite2D(KLGLSprite* sprite, int x, int y, int row, int col, bool managed = 1);
		void BlitSprite2D(KLGLSprite* sprite, int x, int y, int index = 0, bool managed = 1){
			int length = sprite->width/sprite->swidth;
			int col = index%length;
			int row = (index-col)/length;

			BlitSprite2D(sprite, x, y, row, col, managed);
		};
		void Tile2D(KLGLTexture* texture, int x, int y, int w, int h);
		void Rectangle2D(int x, int y, int width, int height, KLGLColor vcolor = KLGLColor(255, 0, 0, 255));
		void OrthogonalStart();
		void OrthogonalEnd();

		// Automatically load and setup a shader program for given vertex and frag shader scripts
		int InitShaders(int shaderProgId, const char *vsFile, const char *fsFile, int internalString = 0);
		unsigned int GetShaderID(int shaderProgId = 0){
			return shader_id[shaderProgId];
		}
		void BindShaders(int shaderProgId = 0) {
			glColor3f(1.0f, 1.0f, 1.0f);
			glUseProgram(shader_id[shaderProgId]);
		}
		void UnbindShaders() {
			glUseProgram(0);
		}
		void UnloadShader(int id){
			glDetachShader(shader_id[id], shader_fp[id]);
			glDetachShader(shader_id[id], shader_vp[id]);

			glDeleteShader(shader_fp[id]);
			glDeleteShader(shader_vp[id]);
			glDeleteProgram(shader_id[id]);
		}
		void LuaCheckError(lua_State *L, int status){
			if ( status!=0 ) {
				cl("-- %s\n", lua_tostring(L, -1));
				lua_pop(L, 1); // remove error message
			}
		}

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
		void Draw(int x, int y, char * text);
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