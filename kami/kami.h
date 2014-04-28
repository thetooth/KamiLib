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

#if defined _WIN32
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"opengl32.lib")
#include "win32.h"
#else
#include "unix.h"
#endif

// Internal modules
#include "config.h"
#include "input.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gl4.h"

namespace klib{

#include "predef.h"

	// Storage Type for loading textures in a format opengl can use(unsigned RGBA8)
	class Texture {
	public:
		GLuint gltexture;
		unsigned int width;
		unsigned int height;
		Texture();
		Texture(unsigned char *data, int size){
			InitTexture(data, size);
		}
		Texture(const char *fname){
			LoadTexture(fname);
		}
		~Texture();
		void Bind();

		int LoadTexture(const char *fname);
		int InitTexture(unsigned char *data, size_t size);
		int InitTexture(std::vector<unsigned char> &raw);
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
			//glColor4ub(r, g, b, a);
		}

		int r, g, b, a;
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

	class WindowManager {
	public:
		APP_WINDOWMANAGER_CLASS* wm;
		WindowManager(const char* title, Rect<int> *window, int scaleFactor, bool fullscreen, bool vsync){
			wm = new APP_WINDOWMANAGER_CLASS(title, window, scaleFactor, fullscreen, vsync);
		}
		~WindowManager(){
			delete wm;
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

	class Rectangle2D {
	public:
		Rectangle2D() = default;
		~Rectangle2D() = default;

		void Create(){
			// Compile basic color passthrough shader
			std::string vert2d = GLSL(
				in vec2 position;

				uniform mat4 MVP;

				void main() {
					gl_Position = MVP*vec4(position, 0.0, 1.0);
				}
			);
			std::string frag2d = GLSL(
				out vec4 outColor;

				uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

				void main() {
					outColor = color;
				}
			);

			shader.CreateSRC(gl::VERTEX_SHADER, vert2d);
			shader.CreateSRC(gl::FRAGMENT_SHADER, frag2d);
			shader.Link();

			MVP = gl::GetUniformLocation(shader.program, "MVP");
			colorPtr = gl::GetUniformLocation(shader.program, "color");

			// Generate null geometry
			std::vector<Point2D<GLfloat>> v = {
				{ 0, 0 }
			};

			std::vector<GLuint> e = { 0 };
			obj.Create(v, e);

			// Specify the layout of the vertex data
			GLint posAttrib = gl::GetAttribLocation(shader.program, "position");
			gl::EnableVertexAttribArray(posAttrib);
			gl::VertexAttribPointer(posAttrib, 2, gl::FLOAT, false, 2 * sizeof(GLfloat), 0);
		}

		void Draw(glm::mat4 projection, float x, float y, float width, float height, Color vcolor = Color(255, 0, 0, 255)){

			std::vector<Point2D<GLfloat>> v = {
				{ x, y },
				{ x + width, y },
				{ x + width, y + height },
				{ x, y + height }
			};

			std::vector<GLuint> e = { 0, 1, 2, 2, 3, 0 };
			obj.Update(v, e);

			shader.Bind();
			gl::UniformMatrix4fv(MVP, 1, false, glm::value_ptr(projection));
			gl::Uniform4f(colorPtr, ubtof(vcolor.r), ubtof(vcolor.g), ubtof(vcolor.b), ubtof(vcolor.a));
			obj.Draw();
		}
	private:
		GLuint MVP, colorPtr;
		glObj<Point2D<GLfloat>> obj;
		ShaderProgram shader;
	};

	// General drawing helper and view mode manager
	class GC {
	public:
		int internalStatus;

		// Window regions
		std::unique_ptr<WindowManager> windowManager;
		Rect<int> window;
		Rect<int> buffer;
		int overSampleFactor, scaleFactor;
		bool vsync;
		bool fullscreen;
		bool bufferAutoSize;

		// OpenGL 3 utility pipeline
		GLuint defaultMVP;
		glObj<Rect2D<GLfloat>> defaultRect;
		ShaderProgram defaultShader;
		std::vector<FrameBuffer> fbo;
		Rectangle2D dynamicRect;

		std::unique_ptr<Config> config;
		double fps;

		GC(const char* title = "Application", int width = 640, int height = 480, int framerate = 60, bool fullscreen = false, int OSAA = 1, int scale = 1, float anisotropy = 4.0);
		~GC();

		// Prepare current frame for drawing
		void OpenFBO();
		// Render FBO to the display and reset
		void Swap();
		// Blit -- These all need to be rewritten for GL3+
		void Blit2D(Texture* texture, int x, int y, float rotation = 0.0f, float scale = 1.0f, Color vcolor = Color(255, 255, 255, 255), Rect<float> mask = Rect<float>(0,0,0,0));
		void BlitSprite2D(Sprite* sprite, int x, int y, int row, int col, bool managed = 1, Rect<int> mask = Rect<int>(0,0,0,0));
		void BlitSprite2D(Sprite* sprite, int x, int y, int index = 0, bool managed = 1, Rect<int> mask = Rect<int>(0,0,0,0)){
			int length = sprite->width/sprite->swidth;
			int col = index%length;
			int row = floor(index/length);

			BlitSprite2D(sprite, x, y, row, col, managed, mask);
		};
		void Rectangle(int x, int y, int width, int height, Color vcolor = Color(255, 0, 0, 255));
		void BindMultiPassShader(int shaderProgId = 0, int alliterations = 1, bool flipOddBuffer = true, float x = 0.0f, float y = 0.0f, float width = -1.0f, float height = -1.0f, int textureSlot = 0);

		GLuint FastQuad(glObj<Rect2D<GLfloat>> &vao, GLuint &shaderProgram, int width, int height);

		int ProcessEvent(int *status);
		std::function<void(std::vector<KLGLKeyEvent>::iterator)> HandleEvent(std::function<void(std::vector<KLGLKeyEvent>::iterator)> func){
			std::vector<KLGLKeyEvent>::iterator it = windowManager->keyQueue.begin();
			for(;it != windowManager->keyQueue.end(); ++it){
				func(it);
			}
			return func;
		}

	private:
		Rect<int> ASPRatio(Rect<int> &rcScreen, Rect<int> &sizePicture, bool bCenter = true);
	};

	// Simple text drawing
	class Font {
	public:
		bool created, dropshadow;
		GLuint MVP, color;
		ShaderProgram shader;
		glObj<Rect2D<GLfloat>> buffer;
		std::vector<std::unique_ptr<Texture>> m_texture;
		unsigned int cache;

		struct Descriptor {
			short x, y;
			short Width, Height;
			short XOffset, YOffset;
			short XAdvance;
			short Page;

			Descriptor() : x( 0 ), y( 0 ), Width( 0 ), Height( 0 ), XOffset( 0 ), YOffset( 0 ),
				XAdvance( 0 ), Page( 0 )
			{ }
		};
		struct Charset {
			short LineHeight;
			short Base;
			short Width, Height;
			short Pages;
			std::string FileName[8];
			std::map<int, Descriptor> Chars;
		} charsetDesc;

		Font(){};
		Font(const std::string font){ Load(font); };
		~Font();

		void Load(const std::string font);
		void Draw(glm::mat4 projection, int x, int y, const char* text);
		void Draw(glm::mat4 projection, int x, int y, const wchar_t* text);
		bool ParseFnt(std::istream& Stream);
	};
}
