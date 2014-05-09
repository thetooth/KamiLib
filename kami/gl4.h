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
		~glBuffer(){ gl::DeleteBuffers(1, &buffer); };

		void Create(std::vector<StorageClass> v){
			gl::GenBuffers(1, &buffer);
			gl::BindBuffer(BufferType, buffer);
			gl::BufferData(BufferType, v.size()*sizeof(StorageClass), &v.front(), gl::STATIC_DRAW);
		};

		void Update(std::vector<StorageClass> v){
			gl::BindBuffer(BufferType, buffer);
			gl::BufferData(BufferType, v.size()*sizeof(StorageClass), &v.front(), gl::DYNAMIC_DRAW);
		};
	};

	template<typename StorageClass> class glObj
	{
	public:
		GLuint vao;
		glBuffer<gl::ARRAY_BUFFER, StorageClass> vbo;
		glBuffer<gl::ELEMENT_ARRAY_BUFFER, GLuint> ebo;

		size_t elementSize;

		glObj() = default;
		glObj(std::vector<StorageClass> v, std::vector<GLuint> e) { Create(v, e); };
		~glObj(){ gl::DeleteVertexArrays(1, &vao); };

		void Create(std::vector<StorageClass> v, std::vector<GLuint> e){
			// Create Vertex Array Object
			gl::GenVertexArrays(1, &vao);
			gl::BindVertexArray(vao);

			// Retain raw element index length for drawing call
			elementSize = e.size();

			// Create two buffers, first stores the vertex data, second stores element index for draw order
			vbo.Create(std::move(v));
			ebo.Create(std::move(e));
		};

		void Update(std::vector<StorageClass> v, std::vector<GLuint> e){
			// Rebind existing buffer
			gl::BindVertexArray(vao);

			// Update buffer element size
			elementSize = e.size();

			// Send new arrays
			vbo.Update(std::move(v));
			ebo.Update(std::move(e));
		};

		void Draw(int mode = gl::TRIANGLE_STRIP){
			gl::BindVertexArray(vao);
			gl::DrawElements(mode, elementSize, gl::UNSIGNED_INT, 0);
		};

		void DrawInstanced(int instances, int mode = gl::TRIANGLE_STRIP){
			gl::BindVertexArray(vao);
			gl::DrawElementsInstanced(mode, elementSize, gl::UNSIGNED_INT, 0, instances);
		};
	};

	inline void PrintShaderInfoLog(GLuint obj, int isShader = 0){
		int status;
		int infoLogLength = 0;
		int charsWritten = 0;

		gl::GetShaderiv(obj, gl::COMPILE_STATUS, &status);
		gl::GetShaderiv(obj, gl::INFO_LOG_LENGTH, &infoLogLength);

		if ((status != gl::TRUE_ || KLGLDebug) && infoLogLength > 0){
			std::vector<char> infoLog(infoLogLength);
			if (isShader){
				gl::GetShaderInfoLog(obj, infoLogLength, &charsWritten, infoLog.data());
			}else{
				gl::GetProgramInfoLog(obj, infoLogLength, &charsWritten, infoLog.data());
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
			shader = gl::CreateShader(shaderType);
			const char *c_str = shaderSource.c_str();
			gl::ShaderSource(shader, 1, &c_str, 0);
			gl::CompileShader(shader);
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
		~ShaderProgram(){ gl::DeleteProgram(program); };

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
			std::vector<int> type = { gl::VERTEX_SHADER, gl::FRAGMENT_SHADER, gl::GEOMETRY_SHADER };

			for (auto shader : s){
				cl("%s ", shader.c_str());
				Create(type[i], shader.c_str()); i++;
			}

			Link();
			cl("[OK]\n");
		}
		
		GLuint Link(){
			program = gl::CreateProgram();
			for (auto s : shaders){
				gl::AttachShader(program, s.shader);
			}
			gl::LinkProgram(program);
			if (KLGLDebug){
				PrintShaderInfoLog(program, 0);
			}
			return program;
		}

		void Bind(){
			gl::UseProgram(program);
		}
	private:
		bool m_firstuse;
	};

	class FrameBuffer {
	public:
		GLuint fbo;
		GLuint depth;
		GLuint texture;

		FrameBuffer() = default;
		FrameBuffer(int width, int height){
			Create(width, height);
		};
		~FrameBuffer(){
			gl::DeleteTextures(1, &texture);
			gl::DeleteRenderbuffers(1, &depth);
			gl::DeleteFramebuffers(1, &fbo);
		};

		void Create(int width, int height){
			// Create frame buffer
			gl::GenFramebuffers(1, &fbo);
			gl::BindFramebuffer(gl::FRAMEBUFFER, fbo);

			// Create texture to hold color buffer
			gl::GenTextures(1, &texture);
			gl::BindTexture(gl::TEXTURE_2D, texture);

			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, width, height, 0, gl::RGBA, gl::UNSIGNED_BYTE, NULL);

			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

			gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, texture, 0);

			// Create Render Buffer Object to hold depth buffer
			gl::GenRenderbuffers(1, &depth);
			gl::BindRenderbuffer(gl::RENDERBUFFER, depth);
			gl::RenderbufferStorage(gl::RENDERBUFFER, gl::DEPTH_COMPONENT, width, height);
			gl::FramebufferRenderbuffer(gl::FRAMEBUFFER, gl::DEPTH_ATTACHMENT, gl::RENDERBUFFER, depth);

			GLenum status = gl::CheckFramebufferStatus(gl::FRAMEBUFFER);

			switch (status) {
			case gl::FRAMEBUFFER_COMPLETE:
				break;
			case gl::FRAMEBUFFER_UNSUPPORTED:
				throw KLGLException("Error: unsupported framebuffer format!\n");
				break;
			default:
				throw KLGLException("Error: invalid framebuffer config!\n");
			}

			gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
		}

		void Bind(){
			gl::BindFramebuffer(gl::FRAMEBUFFER, fbo);
		}

		void Unbind(){
			gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
		}
	};
}