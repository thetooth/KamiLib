#pragma once

#include "pure.h"
#if defined _WIN32
#include "win32.h"
#else
#include "unix.h"
#endif

#include <iostream>
#include <string>
#include <fstream>
#include <initializer_list>

#define GLSL(src) "#version 150 core\n" #src

namespace klib {
	template<int BufferType, typename StorageClass> class glBuffer
	{
	public:
		GLuint buffer;

		glBuffer() = default;
		~glBuffer(){ glDeleteBuffers(1, &buffer); };

		void Create(std::vector<StorageClass> v){
			glGenBuffers(1, &buffer);
			glBindBuffer(BufferType, buffer);
			glBufferData(BufferType, v.size()*sizeof(StorageClass), &v.front(), GL_STATIC_DRAW);
		};

		void Update(std::vector<StorageClass> v){
			glBindBuffer(BufferType, buffer);
			glBufferData(BufferType, v.size()*sizeof(StorageClass), &v.front(), GL_DYNAMIC_DRAW);
		};
	};

	template<typename StorageClass> class glObj
	{
	public:
		GLuint vao;
		glBuffer<GL_ARRAY_BUFFER, StorageClass> vbo;
		glBuffer<GL_ELEMENT_ARRAY_BUFFER, GLuint> ebo;

		int elementSize;

		glObj() = default;
		glObj(std::vector<StorageClass> v, std::vector<GLuint> e) { Create(v, e); };
		~glObj(){ glDeleteVertexArrays(1, &vao); };

		void Create(std::vector<StorageClass> v, std::vector<GLuint> e){
			// Create Vertex Array Object
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			// Retain raw element index length for drawing call
			elementSize = e.size();

			// Create two buffers, first stores the vertex data, second stores element index for draw order
			vbo.Create(std::move(v));
			ebo.Create(std::move(e));
		};

		void Update(std::vector<StorageClass> v, std::vector<GLuint> e){
			// Rebind existing buffer
			glBindVertexArray(vao);

			// Update buffer element size
			elementSize = e.size();

			// Send new arrays
			vbo.Update(std::move(v));
			ebo.Update(std::move(e));
		};

		void Draw(int mode = GL_TRIANGLE_STRIP){
			glBindVertexArray(vao);
			glDrawElements(mode, elementSize, GL_UNSIGNED_INT, 0);
		};

		void DrawInstanced(int instances, int mode = GL_TRIANGLE_STRIP){
			glBindVertexArray(vao);
			glDrawElementsInstanced(mode, elementSize, GL_UNSIGNED_INT, 0, instances);
		};
	};

	inline void PrintShaderInfoLog(GLuint obj, int isShader = 0){
		int status;
		int infoLogLength = 0;
		int charsWritten = 0;

		glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);

		if ((status != GL_TRUE || KLGLDebug) && infoLogLength > 0){
			std::vector<char> infoLog(infoLogLength);
			if (isShader){
				glGetShaderInfoLog(obj, infoLogLength, &charsWritten, infoLog.data());
			}else{
				glGetProgramInfoLog(obj, infoLogLength, &charsWritten, infoLog.data());
			}
			cl("%s", infoLog.data());
		}
	}

	class Shader {
	public:
		GLuint shader;

		Shader() = default;
		Shader(GLuint shaderType, std::string shaderSource){
			Compile(shaderType, shaderSource);
		};
		~Shader(){};

		void Compile(GLuint shaderType, std::string shaderSource){
			shader = glCreateShader(shaderType);
			const char *c_str = shaderSource.c_str();
			glShaderSource(shader, 1, &c_str, 0);
			glCompileShader(shader);
			if (KLGLDebug){
				PrintShaderInfoLog(shader, 1);
			}
		}
	};

	class ShaderProgram {
	public:
		GLuint program;

		std::vector<Shader> shaders;

		ShaderProgram() = default;
		ShaderProgram(std::initializer_list<std::string> s){ CreateList(s); };
		~ShaderProgram(){ glDeleteProgram(program); };

		void Create(GLuint type, const char* shader){
			std::ifstream shaderFile(shader, std::ios::in);
			if (shaderFile.bad()){
				throw KLGLException("[KLGLShaderProgram::create][%d:%s] \nResource \"%s\" could not be opened.", __LINE__, __FILE__, shader);
				return;
			}

			std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)),
			std::istreambuf_iterator<char>());

			shaders.push_back(Shader(type, shaderSource));
		}

		void CreateSRC(GLuint type, std::string source){
			shaders.push_back(Shader(type, source));
		}

		void CreateList(std::initializer_list<std::string> s){
			cl("Loading Program: ");
			if (s.size() > 3){
				throw KLGLException("[KLGLShaderProgram::initializer_list] Wrong number of arguments.");
				return;
			}

			int i = 0;
			std::vector<int> type = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER };

			for (auto shader : s){
				cl("%s ", shader.c_str());
				Create(type[i], shader.c_str()); i++;
			}

			Link();
			cl("[OK]\n");
		}
		
		GLuint Link(){
			program = glCreateProgram();
			for (auto s : shaders){
				glAttachShader(program, s.shader);
			}
			glLinkProgram(program);
			if (KLGLDebug){
				PrintShaderInfoLog(program, 0);
			}
			return program;
		}

		void Bind(){
			glUseProgram(program);
		}
	private:
		bool m_firstuse;
	};

	class FrameBuffer {
	public:
		GLuint fbo;
		GLuint depthStencil;
		GLuint texture;

		FrameBuffer() = default;
		FrameBuffer(int width, int height){
			Create(width, height);
		};
		~FrameBuffer(){
			glDeleteTextures(1, &texture);
			glDeleteRenderbuffers(1, &depthStencil);
			glDeleteFramebuffers(1, &fbo);
		};

		void Create(int width, int height){
			// Create frame buffer
			GLuint frameBuffer;
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			// Create texture to hold color buffer
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

			// Create Renderbuffer Object to hold depth and stencil buffers
			/*glGenRenderbuffers(1, &depthStencil);
			glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);*/
			glGenRenderbuffers(1, &depthStencil);
			glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthStencil);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

			switch (status) {
			case GL_FRAMEBUFFER_COMPLETE:
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				throw KLGLException("Error: unsupported framebuffer format!\n");
				break;
			default:
				throw KLGLException("Error: invalid framebuffer config!\n");
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void Bind(){
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		}

		void Unbind(){
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	};
}