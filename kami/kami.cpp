#include "kami.h"
#include "lodepng.h"
#include "xxhash.h"
#include "version.h"

#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#include <regex>

namespace klib{

	// Make extern local to klib
	std::shared_ptr<Console> clConsole;
	bool KLGLDebug = false;
	bool resizeEvent = false;

	Texture::Texture(){
		gltexture = 1;
		width = 64;
		height = 64;
	}

	Texture::~Texture(){
		// Drop the texture from VRAM
		gl::DeleteTextures( 1, &gltexture );
	}

	int Texture::LoadTexture(const char *fname){
		cl("Loading Texture: %s ", fname);
		std::ifstream file(fname, std::ios::in | std::ios::binary);

		if(file.bad()){
			cl("[FAILED]\n");
			throw KLGLException("[KLGLTexture::LoadTexture][%d:%s] \nResource \"%s\" could not be opened.", __LINE__, __FILE__, fname);
			return 1;
		}

		std::vector<unsigned char> buffer;
		std::vector<unsigned char> raw;

		std::copy( 
			std::istreambuf_iterator<char>(file), 
			std::istreambuf_iterator<char>( ),
			std::back_inserter(buffer));

		file.close();

		unsigned error = lodepng::decode(raw, width, height, buffer);

		if (error){
			cl("[FAILED]\n");
			throw KLGLException("[KLGLTexture::LoadTexture][%d:%s] \nPNG decode failed with \"%s\".", __LINE__, __FILE__, lodepng_error_text(error));
			return 2;
		}

		InitTexture(raw);

		cl("[OK]\n");
		return 0;
	}

	int Texture::InitTexture(unsigned char *data, size_t size){
		std::vector<unsigned char> raw;
		unsigned int error = lodepng::decode(raw, width, height, data, size);

		InitTexture(raw);
		return 0;
	}

	int Texture::InitTexture(std::vector<unsigned char> &data){
		gl::GenTextures(1, &gltexture);
		gl::BindTexture(gl::TEXTURE_2D, gltexture);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
		gl::TexParameterf(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::REPEAT);
		gl::TexParameterf(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::REPEAT);
		gl::TexParameterf(gl::TEXTURE_2D, gl::TEXTURE_MAX_ANISOTROPY_EXT, APP_ANISOTROPY);
		if (APP_ENABLE_MIPMAP){
			// ! Upgrade
			/*gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::LINEAR_MIPMAP_LINEAR);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::LINEAR_MIPMAP_LINEAR);
			gl::uBuild2DMipmaps(gl::TEXTURE_2D, gl::RGBA, width, height, gl::RGBA, gl::UNSIGNED_BYTE, data.data());*/
		}else{
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, width, height, 0, gl::RGBA, gl::UNSIGNED_BYTE, data.data());
		}
		return 0;
	}

	void Texture::Bind(){
		gl::BindTexture(gl::TEXTURE_2D, gltexture);
	}

	GC::GC(const char* _title, int _width, int _height, int _framerate, bool _fullscreen, int _OSAA, int _scale, float _anisotropy){

		try{
			// State
#if defined(_DEBUG)
			KLGLDebug = true;
#else
			KLGLDebug = false;
#endif

			// Create console buffer
			clConsole = std::make_shared<Console>();

			// MOTD
			time_t buildTime = (time_t)APP_BUILD_TIME;
			char* buildTimeString = asctime(gmtime(&buildTime));
			memset(buildTimeString+strlen(buildTimeString)-1, 0, 1); // Remove the \n
			cl("KamiLib v0.1.0 R%d %s, %s %s,\n", APP_BUILD_VERSION, buildTimeString, APP_ARCH_STRING, APP_COMPILER_STRING);
			cl(APP_MOTD);

			std::string title	= "Application";
			vsync				= true;
			overSampleFactor	= _OSAA;
			scaleFactor			= _scale;
			fps					= _framerate;
			fullscreen			= _fullscreen;
			bufferAutoSize		= false;

			// Initialize the window geometry
			window.x			= 0;
			window.y			= 0;
			window.width		= _width;
			window.height		= _height;
			buffer.width		= window.width*overSampleFactor;
			buffer.height		= window.height*overSampleFactor;

			// Attempt to load runtime configuration
			config = std::make_unique<Config>("configuration.json");
			if (config->ParseError() == 1){
				cl("Failed to load configuration file overrides.\n");
			}else{
				KLGLDebug		= config->GetBoolean("system", "debug",				KLGLDebug						);
				window.x		= config->GetInteger("system", "lastPosX",			0								);
				window.y		= config->GetInteger("system", "lastPosY",			0								);
				window.width	= config->GetInteger("system", "windowWidth",		_width							);
				window.height	= config->GetInteger("system", "windowHeight",		_height							);
				overSampleFactor= config->GetInteger("system", "bufferScale",		_OSAA							);
				bufferAutoSize	= config->GetBoolean("system", "bufferAutoSize",	false							);
				buffer.width	= config->GetInteger("system", "bufferWidth",		window.width*overSampleFactor	);
				buffer.height	= config->GetInteger("system", "bufferHeight",		window.height*overSampleFactor	);
				vsync			= config->GetBoolean("system", "vsync",				vsync							);
				scaleFactor		= config->GetInteger("system", "windowScale",		_scale							);
				fullscreen		= config->GetBoolean("system", "fullscreen",		_fullscreen						);
				title			= config->GetString("system", "title",				_title							);
			}

			// Init window
			windowManager = std::make_unique<WindowManager>(title.c_str(), &window, scaleFactor, fullscreen, vsync);

			// Load OpenGL
#if defined APP_USE_GLEW
			glewInit();
#else
			gl::exts::LoadTest didLoad = gl::sys::LoadFunctions();
			if (!didLoad){
				cl("Catastrophic Error: Minimum OpenGL version 3 not supported, please upgrade your graphics hardware.\n");
				cl("Number of functions that failed to load: %i.\n", didLoad.GetNumMissing());
				exit(EXIT_FAILURE);
			}
#endif

			// Setup frame buffer
			fbo.emplace_back(buffer.width, buffer.height);

			// Create basic pass-through shaders
			std::string vert2d = GLSL(
				in vec2 position;
				in vec2 texcoord;
				out vec2 coord;

				uniform mat4 MVP;

				void main() {
					coord = texcoord;
					gl_Position = MVP*vec4(position, 0.0, 1.0);
				}
			);
			std::string frag2d = GLSL(
				uniform sampler2D image;

				in vec2 coord;
				out vec4 outColor;

				void main() {
					outColor = texture(image, coord);
				}
			);

			defaultShader.CreateSRC(gl::VERTEX_SHADER, vert2d);
			defaultShader.CreateSRC(gl::FRAGMENT_SHADER, frag2d);
			defaultShader.Link();

			// Create default quad for rendering frame buffer
			defaultMVP = FastQuad(defaultRect, defaultShader.program, buffer.width, buffer.height);

			// Create dynamic quad for utility functions
			dynamicRect.Create();

			// Initialize gl parameters
			gl::Enable(gl::DEPTH_TEST);
			gl::DepthMask(true);
			gl::DepthFunc(gl::LEQUAL);
			gl::DepthRange(0.0f, 1.0f);

			gl::Enable(gl::BLEND);
			gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);

			// Hello World
			gl::ClearColor(ubtof(0), ubtof(122), ubtof(204), 1.0f);
			gl::ClearDepth(1.0f);
			gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
			windowManager->Swap();

			// Init OpenGL OK!
			cl("Initialized %s OpenGL %s %dx%d\n", gl::GetString(gl::VENDOR), gl::GetString(gl::VERSION), buffer.width, buffer.height);
			
			gl::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			internalStatus = 0;

		}catch(KLGLException e){
			cl("KLGLException::%s\n", e.getMessage());
			exit(EXIT_FAILURE);
		}
	}

	GC::~GC(){
		//Bind 0, which means render to back buffer, as a result, fb is unbound
		gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
		fbo.clear();

		cl("\n0x%x :3\n", internalStatus);
	}

	int GC::ProcessEvent(int *status){
		windowManager->ProcessEvent(status);
		return *status;
	}

	void GC::OpenFBO(){
		if (resizeEvent && !fullscreen){
#if defined _WIN32 // ! BUG - Redundant functions
			RECT win;
			GetClientRect(windowManager->wm->hWnd, &win);
			window.width = std::max(int(win.right-win.left), 16);
			window.height = std::max(int(win.bottom-win.top), 16);
			windowManager->wm->clientResize(window.width, window.height);		
#endif	
			if (bufferAutoSize){
				buffer.width = std::max(window.width*overSampleFactor, 16);
				buffer.height = std::max(window.height*overSampleFactor, 16);
				fbo[0] = FrameBuffer(buffer.width, buffer.height);
			}
			resizeEvent = false;
		}

		gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT); //Clear the colour buffer (more buffers later on)
		fbo[0].Bind(); // Bind our frame buffer for rendering
		gl::Viewport(0, 0, buffer.width, buffer.height);
		gl::Clear (gl::COLOR_BUFFER_BIT|gl::DEPTH_BUFFER_BIT);
	}

	void GC::Swap(){
		gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
		gl::Viewport(0, 0, window.width, window.height);

		defaultShader.Bind();
		gl::BindTexture(gl::TEXTURE_2D, fbo[0].texture);
		auto projection = glm::ortho(0.0f, (float)window.width, 0.0f, (float)window.height);
		auto ratio = ASPRatio(window, buffer, false);
		auto view = glm::translate(
			glm::mat4(1.0f),
			glm::vec3((window.width - ratio.width) / 2.0f, (window.height - ratio.height) / 2.0f, 0.0f)
			);
		auto model = glm::scale(
			glm::mat4(1.0f),
			glm::vec3(ratio.width / float(buffer.width), ratio.height / float(buffer.height), 0.0f)
			);
		auto MVP = projection*view*model;
		gl::UniformMatrix4fv(defaultMVP, 1, false, glm::value_ptr(MVP));
		defaultRect.Draw();

		gl::Finish();

		// Swap!
		windowManager->Swap();
	}

	Rect<int> GC::ASPRatio(Rect<int> &rcScreen, Rect<int> &sizePicture, bool bCenter){
		Rect<int> rect(rcScreen);
		double dWidth = rcScreen.width;
		double dHeight = rcScreen.height;
		double dAspectRatio = dWidth/dHeight;

		double dPictureWidth = sizePicture.width;
		double dPictureHeight = sizePicture.height;
		double dPictureAspectRatio = dPictureWidth/dPictureHeight;

		if (dPictureAspectRatio < dAspectRatio){
			int nNewWidth =  (int)(dHeight/dPictureHeight*dPictureWidth);
			int nCenteringFactor = (bCenter ? (rcScreen.width - nNewWidth) / 2 : 0 );
			rect = Rect<int>(nCenteringFactor, 0, nNewWidth + nCenteringFactor, (int)dHeight);
		}else if (dPictureAspectRatio > dAspectRatio){
			int nNewHeight = (int)(dWidth/dPictureWidth*dPictureHeight);
			int nCenteringFactor = (bCenter ? (rcScreen.height - nNewHeight) / 2 : 0);
			rect = Rect<int>(0, nCenteringFactor, (int)dWidth, nNewHeight + nCenteringFactor);
		}
		return rect;
	}

	GLuint GC::FastQuad(glObj<Rect2D<GLfloat>> &vao, GLuint &shaderProgram, int width, int height){

		// This sequence draws a box from two triangles
		std::vector<Rect2D<GLfloat>> v;

		v.emplace_back(0, 0, 0, 0);
		v.emplace_back(0 + width, 0, 1, 0);
		v.emplace_back(0 + width, 0 + height, 1, 1);
		v.emplace_back(0, 0 + height, 0, 1);

		std::vector<GLuint> e{ 0, 1, 2, 2, 3, 0 };

		// Push to GPU
		vao.Create(v, e);

		// Specify the layout of the vertex data
		GLint posAttrib = gl::GetAttribLocation(shaderProgram, "position");
		gl::EnableVertexAttribArray(posAttrib);
		gl::VertexAttribPointer(posAttrib, 2, gl::FLOAT, false, 4 * sizeof(GLfloat), 0);

		GLint texAttrib = gl::GetAttribLocation(shaderProgram, "texcoord");
		gl::EnableVertexAttribArray(texAttrib);
		gl::VertexAttribPointer(texAttrib, 2, gl::FLOAT, false, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

		return gl::GetUniformLocation(shaderProgram, "MVP");
	}

	void GC::Blit2D(Texture* texture, int x, int y, float rotation, float scale, Color vcolor, Rect<float> mask){
		/*glEnable(gl::TEXTURE_2D);
		glBindTexture(gl::TEXTURE_2D, texture->gltexture);

		glPushMatrix();
		{
			if (rotation != 0.0f)
			{
				glLoadIdentity();
				int resetW = (texture->width/2);
				int resetH = (texture->height/2);
				glTranslatef(x+resetW, y+resetH, 0);
				glRotatef( rotation, 0.0f, 0.0f, 1.0f );
				glTranslatef(-x-resetW, -y-resetH, 0);
			}

			if (scale != 1.0f)
			{
				//glLoadIdentity();
				int resetW = (texture->width/2);
				int resetH = (texture->height/2);
				glTranslatef(x+resetW, y+resetH, 0);
				glScalef(scale, scale, 1.0f);
				glTranslatef(-x-resetW, -y-resetH, 0);
			}

			glBegin(gl::QUADS);
			{
				vcolor.Set();
				glTexCoord2d(0.0+mask.x,	0.0+mask.y);		glVertex2i(x+mask.x,						y+mask.y);
				glTexCoord2d(1.0+mask.width,0.0+mask.y);		glVertex2i(x+(texture->width+mask.width),	y+mask.y);
				glTexCoord2d(1.0+mask.width,1.0+mask.height);	glVertex2i(x+(texture->width+mask.width),	y+(texture->height+mask.height));
				glTexCoord2d(0.0+mask.x,	1.0+mask.height);	glVertex2i(x+mask.x,						y+(texture->height+mask.height));
			}
			glEnd();
		}
		glPopMatrix();

		glBindTexture(gl::TEXTURE_2D, 0);*/
	}

	void GC::BlitSprite2D(Sprite* sprite, int x, int y, int row, int col, bool managed, Rect<int> mask){
		/*if (managed){
			glEnable(gl::TEXTURE_2D);
			glBindTexture(gl::TEXTURE_2D, sprite->texturePtr->gltexture);
			glBegin(gl::QUADS);
		}
		{
			//calculate how wide each character is in term of texture coords
			GLfloat dtx = float(sprite->swidth+mask.width)/float(sprite->texturePtr->width);
			GLfloat dty = float(sprite->sheight+mask.height)/float(sprite->texturePtr->height);

			// find the texture coords
			GLfloat tx = float((col * sprite->swidth)+mask.width)/float(sprite->width);
			GLfloat ty = float((row * sprite->sheight)+mask.height)/float(sprite->height);

			glTexCoord2f(tx,		ty);		glVertex2i(x+mask.x,					y+mask.y);
			glTexCoord2f(tx+dtx,	ty);		glVertex2i(x+sprite->swidth+mask.width,	y+mask.y);
			glTexCoord2f(tx+dtx,	ty+dty);	glVertex2i(x+sprite->swidth+mask.width,	y+sprite->sheight+mask.height);
			glTexCoord2f(tx,		ty+dty);	glVertex2i(x+mask.x,					y+sprite->sheight+mask.height);

		}
		if (managed){
			glEnd();
			glBindTexture(gl::TEXTURE_2D, 0);
		}*/
	}

	void GC::Rectangle(int x, int y, int width, int height, Color vcolor){
		auto projection = glm::ortho(0.0f, (float)buffer.width, (float)buffer.height, 0.0f);
		dynamicRect.Draw(projection, x, y, width, height, vcolor);
	}

	void Font::Load(const std::string font){
		std::ifstream fontStream(font);
		auto folder = font.find_last_of("/\\");

		if (fontStream.bad()){
			throw KLGLException("[Font][%s:%d] Error loading font file \"%s\", no stream.", __FILE__, __LINE__, font.c_str());
		}

		ParseFnt(fontStream);
		created = false;
		dropshadow = false;

		for (int i = 0; i < charsetDesc.Pages; i++){
			m_texture.emplace_back(new Texture((font.substr(0, folder + 1) + charsetDesc.FileName[i]).c_str()));
		}

		// Create renderer
		std::string vert2d = GLSL(
			in vec2 position;
			in vec2 texcoord;
			out vec2 coord;

			uniform mat4 MVP;

			void main() {
				coord = texcoord;
				gl_Position = MVP*vec4(position, 0.0, 1.0);
			}
		);
		std::string frag2d = GLSL(
			uniform sampler2D image;
			uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

			in vec2 coord;
			out vec4 outColor;

			void main() {
				outColor = color*texture(image, coord).a;
			}
		);

		shader.CreateSRC(gl::VERTEX_SHADER, vert2d);
		shader.CreateSRC(gl::FRAGMENT_SHADER, frag2d);
		shader.Link();

		MVP = gl::GetUniformLocation(shader.program, "MVP");
		color = gl::GetUniformLocation(shader.program, "color");
	}

	void Font::Draw(glm::mat4 projection, int x, int y, const char* text){
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring narrow = converter.from_bytes(text);
		Draw(projection, x, y, narrow.c_str());
	}

	void Font::Draw(glm::mat4 projection, int x, int y, const wchar_t* text){
		auto stringLen = wcslen(text);

		if (stringLen < 1){
			return;
		}

		auto hash = XXH32(text, stringLen*sizeof(wchar_t), 4352334);

		if (hash != cache){
			cache = hash;

			// Character geometry
			std::vector<Rect2D<GLfloat>> v; v.reserve(stringLen * 4);
			std::vector<GLuint> e; e.reserve(stringLen * 6);

			// Character location and dimensions
			int cp = 0;
			int cx = 0;
			int cy = 0;
			int twidth = 4;
			int lastPage = 0;
			int elementIndex = 0;

			for (const wchar_t* c = text; *c != 0; c++, cp++) {
				// Per-character logic
				switch (*c){
				case L'\n':
					cx = 0;
					cy += charsetDesc.LineHeight;
					continue;
				case L'\t':
					cx += charsetDesc.Base*(twidth - cp%twidth);
					continue;
				}

				wchar_t index = text[cp];
				GLfloat tx = (float)charsetDesc.Chars[index].x / (float)charsetDesc.Width;
				GLfloat ty = (float)charsetDesc.Chars[index].y / (float)charsetDesc.Height;
				GLfloat tw = (float)charsetDesc.Chars[index].Width / (float)charsetDesc.Width;
				GLfloat th = (float)charsetDesc.Chars[index].Height / (float)charsetDesc.Height;
				GLfloat tcx = (float)(cx + charsetDesc.Chars[index].XOffset);
				GLfloat tcy = (float)(cy + charsetDesc.Chars[index].YOffset);
				GLfloat tcw = (float)(charsetDesc.Chars[index].Width);
				GLfloat tch = (float)(charsetDesc.Chars[index].Height);

				// Increment current cursor position
				cx += charsetDesc.Chars[index].XAdvance;

				if (charsetDesc.Chars[index].Page != lastPage){
					//gl::BindTexture(gl::TEXTURE_2D, m_texture[charsetDesc.Chars[index].Page]->gltexture);
					lastPage = charsetDesc.Chars[index].Page;
				}

				v.emplace_back(tcx, tcy, tx, ty);
				v.emplace_back(tcx + tcw, tcy, tx + tw, ty);
				v.emplace_back(tcx + tcw, tcy + tch, tx + tw, ty + th);
				v.emplace_back(tcx, tcy + tch, tx, ty + th);

				e.push_back(elementIndex + 0);
				e.push_back(elementIndex + 1);
				e.push_back(elementIndex + 2);
				e.push_back(elementIndex + 2);
				e.push_back(elementIndex + 3);
				e.push_back(elementIndex + 0);

				elementIndex += 4;
			}

			if (created){
				buffer.Update(v, e);
			}else{
				buffer.Create(v, e);

				GLint posAttrib = gl::GetAttribLocation(shader.program, "position");
				gl::EnableVertexAttribArray(posAttrib);
				gl::VertexAttribPointer(posAttrib, 2, gl::FLOAT, false, 4 * sizeof(GLfloat), 0);

				GLint texAttrib = gl::GetAttribLocation(shader.program, "texcoord");
				gl::EnableVertexAttribArray(texAttrib);
				gl::VertexAttribPointer(texAttrib, 2, gl::FLOAT, false, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

				created = true;
			}
		}

		shader.Bind();
		m_texture[0]->Bind(); // ! We need to support multiple textures per character or we can't do UTF-8

		if (dropshadow){
			auto view = glm::translate(
				glm::mat4(1.0f),
				glm::vec3(x + 1, y + 1, 0.0f)
				);

			gl::UniformMatrix4fv(MVP, 1, false, glm::value_ptr(projection*view));
			gl::Uniform4f(color, 0, 0, 0, 0.5);
			buffer.Draw(gl::TRIANGLES);
		}

		auto view = glm::translate(
			glm::mat4(1.0f),
			glm::vec3(x, y, 0.0f)
			);

		gl::UniformMatrix4fv(MVP, 1, false, glm::value_ptr(projection*view));
		gl::Uniform4f(color, 1, 1, 1, 1);
		buffer.Draw(gl::TRIANGLES);
	}

	Font::~Font(){
	}

	bool Font::ParseFnt(std::istream& stream){
		std::string line;
		std::string Read, Key, Value;
		std::size_t i;
		std::size_t end;
		while( !stream.eof() )
		{
			std::stringstream LineStream;
			std::getline( stream, line );
			LineStream << line;

			//read the line's type
			LineStream >> Read;
			if( Read == "common" )
			{
				//this holds common data
				while( !LineStream.eof() )
				{
					std::stringstream Converter;
					LineStream >> Read;
					i = Read.find('=');
					end = Read.length()-i-1;
					Key = Read.substr( 0, i );
					Value = Read.substr(i+1, end);

					//assign the correct value
					Converter << Value;
					if( Key == "lineHeight" )
						Converter >> charsetDesc.LineHeight;
					else if( Key == "base" )
						Converter >> charsetDesc.Base;
					else if( Key == "scaleW" )
						Converter >> charsetDesc.Width;
					else if( Key == "scaleH" )
						Converter >> charsetDesc.Height;
					else if( Key == "pages" )
						Converter >> charsetDesc.Pages;
				}
			}
			else if( Read == "page" )
			{
				unsigned short PageID = 0;
				while( !LineStream.eof() )
				{
					std::stringstream Converter;
					LineStream >> Read;
					i = Read.find('=');
					end = Read.length()-i-1;
					Key = Read.substr( 0, i );
					Value = Read.substr(i+1, end);

					// Strip quotes
					Value.erase(remove( Value.begin(), Value.end(), '\"' ), Value.end());

					//assign the correct value
					Converter << Value;
					if( Key == "id")
						Converter >> PageID;
					if( Key == "file" )
						Converter >> charsetDesc.FileName[PageID];
				}
			}
			else if( Read == "char" )
			{
				//this is data for a specific char
				unsigned short CharID = 0;

				while( !LineStream.eof() )
				{
					std::stringstream Converter;
					LineStream >> Read;
					i = Read.find('=');
					end = Read.length()-i-1;
					Key = Read.substr( 0, i );
					Value = Read.substr(i+1, end);

					//assign the correct value
					Converter << Value;
					if( Key == "id" ){
						Converter >> CharID;
					}else if( Key == "x" ){
						Converter >> charsetDesc.Chars[CharID].x;
					}else if( Key == "y" ){
						Converter >> charsetDesc.Chars[CharID].y;
					}else if( Key == "width" ){
						Converter >> charsetDesc.Chars[CharID].Width;
					}else if( Key == "height" ){
						Converter >> charsetDesc.Chars[CharID].Height;
					}else if( Key == "xoffset" ){
						Converter >> charsetDesc.Chars[CharID].XOffset;
					}else if( Key == "yoffset" ){
						Converter >> charsetDesc.Chars[CharID].YOffset;
					}else if( Key == "xadvance" ){
						Converter >> charsetDesc.Chars[CharID].XAdvance;
					}else if( Key == "page" ){
						Converter >> charsetDesc.Chars[CharID].Page;
					}else if( Key == "chnl" ){

					}
				}
			}
		}

		return true;
	}

	Sprite::Sprite(const char *fname, int sWidth, int sHeight){
		(*this) = Sprite(new Texture(fname), sWidth, sHeight);
	}

	Sprite::~Sprite(){

	}

	KLGLException::KLGLException(const char* message, ...) {
		va_list arg;
		va_start (arg, message);
		// BUG: Message limited to 1024 chars
		msg = (char*)malloc(1024);
		if (vsprintf(msg, message, arg) < 0)
		{
			printf("\nException handler message out of bounds, this means your triggered an error with more than 1024 chars as the message or there is a corruption of the heap.\n");
			exit(EXIT_FAILURE);
		}
		va_end (arg);
		cl("KLGLException:: %s\n", msg);
	}

	char* KLGLException::getMessage() {
		return msg;
	}
}
