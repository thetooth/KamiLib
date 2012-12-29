#include "kami.h"
#include "picopng.h"
#include "databin.h"
//#include "font_unicode.h"
#include <iostream>
#include <string>
#include <regex>

namespace klib{

	// Make extern local to klib
	char *clBuffer;
	bool KLGLDebug = false;
	bool resizeEvent = false;

	KLGLTexture::KLGLTexture(){
		gltexture = 1;
		width = 64;
		height = 64;
	}

	KLGLTexture::~KLGLTexture(){
		// Drop the texture from VRAM
		glDeleteTextures( 1, &gltexture );
	}

	int KLGLTexture::LoadTexture(const char *fname){
		cl("Loading Texture: %s ", fname);
		unique_ptr<FILE, int(*)(FILE*)> filePtr(fopen(fname, "rb"), fclose);

		if(filePtr == NULL){
			cl("[FAILED]\n");
			throw KLGLException("[KLGLTexture::LoadTexture][%d:%s] \nResource \"%s\" could not be opened.", __LINE__, __FILE__, fname);
			return 1;
		}

		// Pass a big file and we could be sitting here for a long time
		fseek(filePtr.get(), 0, SEEK_END);
		size_t size = ftell(filePtr.get());
		rewind(filePtr.get());

		unsigned char *data = new unsigned char[size+8];
		fread((unsigned char*)data, 1, size, filePtr.get());
		filePtr.release();

		InitTexture(data, size);
		data[size+1] = '\0';
		delete [] data;

		cl("[OK]\n");
		return 0;
	}

	int KLGLTexture::InitTexture(unsigned char *data, size_t size){
		static std::vector<unsigned char> swap;
		// PNG DECODER OMG OMG OMG
		decodePNG(swap, width, height, data, size);

		// Throw them thar pixels at the VRAM
		glGenTextures(1, &gltexture);
		glBindTexture(GL_TEXTURE_2D, gltexture);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, APP_ANISOTROPY);
		if (APP_ENABLE_MIPMAP){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, swap.data());
		}else{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, swap.data());
		}
		swap.clear();
		return 0;
	}

	KLGL::KLGL(const char* _title, int _width, int _height, int _framerate, bool _fullscreen, int _OSAA, int _scale, float _anisotropy){

		try{
			// State
#if defined(_DEBUG)
			KLGLDebug = true;
#else
			KLGLDebug = false;
#endif

			// Clear the console buffer(VERY IMPORTANT)
			clBuffer = new char[APP_BUFFER_SIZE*2];
			fill_n(clBuffer, APP_BUFFER_SIZE*2, '\0');

			//MOTD
			time_t buildTime = (time_t)APP_BUILD_TIME;
			char* buildTimeString = asctime(gmtime(&buildTime));
			memset(buildTimeString+strlen(buildTimeString)-1, 0, 1); // Remove the \n
			cl("KamiLib v0.0.2 R%d %s, %s %s,\n", APP_BUILD_VERSION, buildTimeString, APP_ARCH_STRING, APP_COMPILER_STRING);
			cl(APP_MOTD);

			vsync				= true;
			overSampleFactor	= _OSAA;
			scaleFactor			= _scale;
			fps					= _framerate;
			shaderClock			= 0.0f;
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
			config = new KLGLConfig("configuration.json");
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
			}

			// Init window
			windowManager = new KLGLWindowManager(_title, &window, scaleFactor, fullscreen);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			// Continue setup of OpenGL and framebuffer
			GenFrameBuffer(fbo[0], fbo_texture[0], fbo_depth[0]);
			GenFrameBuffer(fbo[1], fbo_texture[1], fbo_depth[1]);

			// Initialize gl parmas
			glViewport(0, 0, window.width*scaleFactor, window.height*scaleFactor);	// Create initial 2D view
			glEnable(GL_TEXTURE_2D);												// Enable Texture Mapping ( NEW )
			glShadeModel(GL_SMOOTH);												// Enable Smooth Shading
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);						// Really Nice Perspective Calculations
			glClearColor(ubtof(153), ubtof(51), ubtof(51), ubtof(255));				// Background Color
			glClearDepth(1.0f);														// Depth Buffer Setup

			//glDepthFunc(GL_LESS);													// The Type Of Depth Testing To Do
			//glAlphaFunc(GL_GREATER, 0.0f);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_BLEND);

			light_diffuse = Vec4<GLfloat>(1.0, 1.0, 1.0, 1.0);
			light_position = Vec4<GLfloat>(1.0, 1.0, 1.0, 0.0);						// Default Infinite light location.
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse.Data());
			glLightfv(GL_LIGHT0, GL_POSITION, light_position.Data());
			glEnable(GL_LIGHT0);

			// Init OpenGL OK!
			cl("Initialized %s OpenGL %s @%dx%d\n", glGetString(GL_VENDOR), glGetString(GL_VERSION), buffer.width, buffer.height);

			// Draw logo and load internal resources
			InfoBlue = new KLGLTexture(KLGLInfoBluePNG, 205);
			CheepCheepDebug = new KLGLTexture(KLGLCheepCheepDebugPNG, 296);
			klibLogo = new KLGLTexture(KLGLStartupLogoPNG, 248);

			framebuffer = new KLGLTexture();
			framebuffer->width = window.width;
			framebuffer->height = window.height;

			OrthogonalStart();
			glClearColor(ubtof(24), ubtof(20), ubtof(26), 1.0f);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			Blit2D(klibLogo, window.width/2-16, window.height/2-16);
			OrthogonalEnd();
			windowManager->Swap();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			internalStatus = 0;

		}catch(KLGLException e){
			cl("KLGLException::%s\n", e.getMessage());
			exit(EXIT_FAILURE);
		}
	}

	KLGL::~KLGL(){
		//Bind 0, which means render to back buffer, as a result, fb is unbound
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		// Destroy shaders and framebuffers
		for (int i = 0; i < 128; i++)
		{
			UnloadShaders(i);
			glDeleteTextures(1, &fbo_texture[i]);
			glDeleteTextures(1, &fbo_depth[i]);
			glDeleteFramebuffersEXT(1, &fbo[0]);
		}

		delete config;

#ifdef APP_ENABLE_LUA
		lua_close(lua);
#endif

		cl("\n0x%x :3\n", internalStatus);
		cl(NULL);
		delete clBuffer;
	}

	void KLGL::ProcessEvent(int *status){
		windowManager->ProcessEvent(status);
	}

	void KLGL::OpenFBO(float fov, float eyex, float eyey, float eyez){
#if defined _WIN32
		if (resizeEvent && !fullscreen){
			RECT win;
			GetClientRect(windowManager->wm->hWnd, &win);
			window.width = max(win.right-win.left, 0);
			window.height = max(win.bottom-win.top, 0);
			windowManager->wm->clientResize(window.width, window.height);			
			if (bufferAutoSize){
				buffer.width = window.width*overSampleFactor;
				buffer.height = window.height*overSampleFactor;
				GenFrameBuffer(fbo[0], fbo_texture[0], fbo_depth[0], buffer.width, buffer.height);
				GenFrameBuffer(fbo[1], fbo_texture[1], fbo_depth[1], buffer.width, buffer.height);
			}
			resizeEvent = false;
		}
#endif
		glLoadIdentity();
		glViewport(0, 0, window.width, window.height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); //Clear the colour buffer (more buffers later on)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[0]); // Bind our frame buffer for rendering
		glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT); // Push our glEnable and glViewport states

		// Set the size of the frame buffer view port
		glViewport(0, 0, buffer.width, buffer.height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho( -2.0, 2.0, -2.0, 2.0, -100.0, 100.0 );
		glViewport(0, 0, buffer.width, buffer.height);
		gluPerspective(fov, 1.0f*buffer.width/buffer.height, -100.0, 100.0);
		gluLookAt(eyex, eyey, eyez, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// Set MODELVIEW matrix mode
		glMatrixMode(GL_MODELVIEW);
		glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}

	void KLGL::Swap(){
		// Restore our glEnable and glViewport states and unbind our texture
		glPopAttrib();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		glLoadIdentity(); // Load the Identity Matrix to reset our drawing locations
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glDisable(GL_LIGHTING);

		BindShaders(0);
		glUniform1f(glGetUniformLocation(GetShaderID(0), "time"), shaderClock);
		glUniform2f(glGetUniformLocation(GetShaderID(0), "resolution"), buffer.width*scaleFactor, buffer.height*scaleFactor);
		glUniform2f(glGetUniformLocation(GetShaderID(0), "outputSize"), window.width, window.height);
		shaderClock = clock();

		glBindTexture(GL_TEXTURE_2D, fbo_texture[0]); // Bind our frame buffer texture
		glBegin(GL_QUADS);
		{
			Rect<int> ratio = ASPRatio(window, buffer, false);
			glTexCoord2d(0.0,0.0); glVertex3d(-(ratio.width/double(window.width)),	-(ratio.height/double(window.height)), 0);
			glTexCoord2d(1.0,0.0); glVertex3d(+(ratio.width/double(window.width)),	-(ratio.height/double(window.height)), 0);
			glTexCoord2d(1.0,1.0); glVertex3d(+(ratio.width/double(window.width)),	+(ratio.height/double(window.height)), 0);
			glTexCoord2d(0.0,1.0); glVertex3d(-(ratio.width/double(window.width)),	+(ratio.height/double(window.height)), 0);
		}
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures
		UnbindShaders();

		// Swap!
		windowManager->Swap();
	}

	Rect<int> KLGL::ASPRatio(Rect<int> &rcScreen, Rect<int> &sizePicture, bool bCenter){
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

	void KLGL::GenFrameBuffer(GLuint &fbo, GLuint &fbo_texture, GLuint &fbo_depth, int bufferWidth, int bufferHeight) {

		if (bufferWidth == 0 || bufferHeight == 0){
			bufferWidth = buffer.width;
			bufferHeight = buffer.height;
		}
		// Depth Buffer
		/*glGenTextures(1, &fbo_depth);
		glBindTexture(GL_TEXTURE_2D, fbo_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, bufferWidth, bufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);*/

		glGenRenderbuffers(1, &fbo_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, bufferWidth, bufferHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// Texture Buffer
		glGenTextures(1, &fbo_texture);
		glBindTexture(GL_TEXTURE_2D, fbo_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, fbo_depth, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo_depth);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbo_texture, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

		switch (status) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			throw KLGLException("Error: unsupported framebuffer format!\n");
			break;
		default:
			throw KLGLException("Error: invalid framebuffer config!\n");
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	void KLGL::Blit2D(KLGLTexture* texture, int x, int y, float rotation, float scale, KLGLColor vcolor, Rect<float> mask){
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture->gltexture);

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

			glBegin(GL_QUADS);
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

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void KLGL::BlitSprite2D(KLGLSprite* sprite, int x, int y, int row, int col, bool managed, Rect<int> mask){
		if (managed){
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, sprite->texturePtr->gltexture);
			glBegin(GL_QUADS);
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
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void KLGL::Tile2D(KLGLTexture* texture, int x, int y, int w, int h){
		int scrX = 0;
		int scrY = 0;

		for(int iY = 0; iY < h; iY++){
			for(int iX = 0; iX < w; iX++){
				scrX = x+(iX*texture->width);
				scrY = y+(iY*texture->height);
				if(scrX < 0-texture->width || scrX > buffer.width || scrY < 0-texture->height || scrY > buffer.height){
					//continue;
				}
				Blit2D(texture, scrX, scrY);
			}
		}
	}

	void KLGL::Rectangle2D(int x, int y, int width, int height, KLGLColor vcolor){
		glBegin(GL_QUADS);
		{
			vcolor.Set();
			glTexCoord2d(0.0,0.0); glVertex2i(x,		y);
			glTexCoord2d(1.0,0.0); glVertex2i(x+width,	y);
			glTexCoord2d(1.0,1.0); glVertex2i(x+width,	y+height);
			glTexCoord2d(0.0,1.0); glVertex2i(x,		y+height);
		}
		glEnd();
	}

	void KLGL::OrthogonalStart(float scale){
		glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, (buffer.width/overSampleFactor)*scale, 0, (buffer.height/overSampleFactor)*scale, -1, 1);
		glScalef(1.0f, -1.0f, 1.0f);
		glTranslatef(0.0f, -(buffer.height/overSampleFactor)*scale, 0.0f);
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
	}

	void KLGL::OrthogonalEnd(){
		glPopAttrib();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
		glShadeModel(GL_SMOOTH);
	}

	int KLGL::InitShaders(int shaderProgId, int isString, const char *vsFile, const char *fsFile, const char *gsFile, const char *tsFile){
		cl("Initializing Shader ID %d\n", shaderProgId);

		char* sData = nullptr;
		shader_id[shaderProgId] = glCreateProgram();

#define sLoad(dest, src) LoadShaderFile(&dest, src)

		sLoad(sData, vsFile);
		if (vsFile != NULL && sData != nullptr){
			cl("Compiling %s\n", vsFile);
			shader_vp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_VERTEX_SHADER,	 sData);
		}
		sLoad(sData, fsFile);
		if (fsFile != NULL && sData != nullptr){
			cl("Compiling %s\n", fsFile);
			shader_fp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_FRAGMENT_SHADER, sData);
		}
		sLoad(sData, gsFile);
		if (gsFile != NULL && sData != nullptr){
			cl("Compiling %s\n", gsFile);
			shader_gp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_GEOMETRY_SHADER, sData);
		}
		sLoad(sData, tsFile);
		if (tsFile != NULL && sData != nullptr){
			cl("Compiling %s\n", tsFile);
			shader_tp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_ARB_tessellation_shader, sData);
		}
#undef sLoad

		cl("Linking...", vsFile);
		glLinkProgram(shader_id[shaderProgId]);
		if(KLGLDebug){
			PrintShaderInfoLog(shader_id[shaderProgId], 0);
		}

		cl(" [OK]\n");
		return 0;
	}

	unsigned int KLGL::ComputeShader(unsigned int &shaderProgram, unsigned int shaderType, const char* shaderString){
		if (shaderString == NULL){
			throw KLGLException("NULL ptr given instead of shader string.");
			return NULL;
		}
		unsigned int tmpLinker = glCreateShader(shaderType);
		glShaderSource(tmpLinker, 1, &shaderString, 0);
		glCompileShader(tmpLinker);
		if(KLGLDebug){
			PrintShaderInfoLog(tmpLinker);
		}
		glAttachShader(shaderProgram, tmpLinker);
		//free(const_cast<char *>(shaderString));
		return tmpLinker;
	}

	void KLGL::LoadShaderFile(char **dest, const char *fname){
		if (fname == NULL || strlen(fname) == 0){
			*dest = nullptr;
			return;
			//throw KLGLException("File name NULL or emty.");
		}

		std::string shaderString(file_contents(fname));

		std::basic_regex<char> includeMatch("#include([\\s]+|[\\t]+)\"([^\"]+)\"");
		std::match_results<std::string::const_iterator> matches;

		while(std::regex_search(shaderString, matches, includeMatch)){
			std::string includePath(matches[2]);
			std::string content(file_contents(includePath.c_str()));
			shaderString = regex_replace(shaderString, includeMatch, content);
		}

		*dest = (char*)malloc(shaderString.length());
		strcpy(*dest, shaderString.c_str());
		shaderString.clear();
	}

	void KLGL::PrintShaderInfoLog(GLuint obj, int isShader){
		int status;
		int infoLogLength = 0;
		int infoLogRemainLength = 0;
		int charsWritten = 0;
		char *infoLog;

		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);
		glGetShaderiv(obj, GL_COMPILE_STATUS, &status);

		if ((status != 1 || KLGLDebug) && infoLogLength > 0){
			infoLog = (char *)malloc(infoLogLength);
			if(isShader){
				glGetShaderInfoLog(obj, infoLogLength, &charsWritten, infoLog);
			}else{
				glGetProgramInfoLog(obj, infoLogLength, &charsWritten, infoLog);
			}
			cl("%s", infoLog);
			free(infoLog);
		}
	}

	void KLGL::BindMultiPassShader(int shaderProgId, int alliterations, bool flipOddBuffer, float x, float y, float width, float height, int textureSlot){
		if (width < 0.0f || height < 0.0f){
			width = buffer.width;
			height = buffer.height;
		}

		glPushMatrix();
		glLoadIdentity();

		int primary = (textureSlot != 0 ? textureSlot*2 : 0);
		int secondary = (textureSlot != 0 ? (textureSlot*2)+1 : 1);

		for (int i = 0; i < alliterations; i++)
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[secondary]);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fbo_texture[0]);
			/*glActiveTexture(GL_TEXTURE0+textureSlot);
			glBindTexture(GL_TEXTURE_2D, fbo_texture[2]);*/


			BindShaders(shaderProgId);
			glBegin(GL_QUADS);
			glTexCoord2d(0.0,0.0); glVertex2i(x,			y);
			glTexCoord2d(1.0,0.0); glVertex2i(width,		y);
			glTexCoord2d(1.0,1.0); glVertex2i(width,		height);
			glTexCoord2d(0.0,1.0); glVertex2i(x,			height);
			glEnd();
			UnbindShaders();
			glBindTexture(GL_TEXTURE_2D, 0);

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[primary]);
			//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fbo_texture[1]);
			/*glActiveTexture(GL_TEXTURE0+textureSlot);
			glBindTexture(GL_TEXTURE_2D, fbo_texture[3]);*/

			glBegin(GL_QUADS);
			// Fix for odd flipping when using gl_FragCoord
			if (flipOddBuffer && (i%2 == 0 )){
				glTexCoord2d(0.0,1.0); glVertex2i(x,			y);
				glTexCoord2d(1.0,1.0); glVertex2i(width,		y);
				glTexCoord2d(1.0,0.0); glVertex2i(width,		height);
				glTexCoord2d(0.0,0.0); glVertex2i(x,			height);
			}else{
				glTexCoord2d(0.0,0.0); glVertex2i(x,			y);
				glTexCoord2d(1.0,0.0); glVertex2i(width,		y);
				glTexCoord2d(1.0,1.0); glVertex2i(width,		height);
				glTexCoord2d(0.0,1.0); glVertex2i(x,			height);
			}
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if (textureSlot != 0){
			glActiveTexture(GL_TEXTURE0);
		}

		glPopMatrix();
	}

	KLGLFont::KLGLFont(){
		auto *tmtex = new KLGLTexture(KLGLFontDefault, 2576);
		c_texture = tmtex->gltexture;
		m_width = 128;
		m_height= 192;
		c_width = 8;
		c_height= 8;
		extended = -1;
		color = new KLGLColor(255, 255, 255);
		c_per_row = m_width/c_width;
	}

	KLGLFont::KLGLFont(GLuint init_texture, GLuint init_m_width, GLuint init_m_height, GLuint init_c_width, GLuint init_c_height, int init_extended) : 
		c_texture(init_texture), m_width(init_m_width), m_height(init_m_height), c_width(init_c_width), c_height(init_c_height), extended(init_extended)
	{
		//set the color to render the font
		color = new KLGLColor(255, 255, 255);
		c_per_row = m_width/c_width;
	}

	void KLGLFont::Draw(int x, int y, char* text, KLGLColor* vcolor)
	{
		int len = strlen(text);
		wchar_t* tmpStr = new wchar_t[len+8];
		tmpStr[len] = L'\0';
		mbstowcs(tmpStr, text, len);
		Draw(x, y, tmpStr, vcolor);
		delete [] tmpStr;
	}

	void KLGLFont::Draw(int x, int y, wchar_t* text, KLGLColor* vcolor)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, c_texture);
		glBegin(GL_QUADS);

		//character location and dimensions
		int cp = 0;
		int cx = 0;
		int cy = 0;
		int cw = c_width;
		int ch = c_height;
		int twidth = 4;
		int dropshadow = 0;
		size_t stringLen = wcslen(text);

		//calculate how wide each character is in term of texture coords
		GLfloat dtx = (float)c_width/(float)m_width;
		GLfloat dty = (float)c_height/(float)m_height;

		char* sbtext = new char[stringLen+8];
		memset(sbtext, 0, stringLen+8);

		wcstombs(sbtext, text, stringLen);
		for (char* c = sbtext; *c != 0; c++,cp++) {
			// Per-character logic
			switch(*c){
			case '\n':
				cx = 0;
				cy += ch;
				continue;
			case '\t':
				cx += cw*(twidth-cp%twidth);
				continue;
			case '@':
				if (*(c+1) == 'C'){ // Change color
					if (vcolor != NULL){ // Ignore color codes if global parameter is set
						color = vcolor;
						break;
					}
					char *token = new char[8];
					char *colorSubStr = new char[4];

					memset(token, 0, 8);
					memset(colorSubStr, 0, 4);
					substr(token, sbtext, cp+2, 6);

					c += 7;
					cp+= 7;

					substr(colorSubStr, token, 0, 2);
					color->r = strtoul(colorSubStr, NULL, 16);
					substr(colorSubStr, token, 2, 2);
					color->g = strtoul(colorSubStr, NULL, 16);
					substr(colorSubStr, token, 4, 2);
					color->b = strtoul(colorSubStr, NULL, 16);
					color->a = 255;

					delete [] token;
					delete [] colorSubStr;
					continue;
				}else if (*(c+1) == 'D'){ // Drop shadow
					c += 1;
					cp+= 1;
					dropshadow = 1;
					continue;
				}
				break;
			}

			if(true){
				wchar_t index = text[cp];
				GLfloat tx = (float)charsetDesc.Chars[index].x/(float)charsetDesc.Width;
				GLfloat ty = (float)charsetDesc.Chars[index].y/(float)charsetDesc.Height;
				GLfloat tw = (float)charsetDesc.Chars[index].Width/(float)charsetDesc.Width;
				GLfloat th = (float)charsetDesc.Chars[index].Height/(float)charsetDesc.Height;
				GLfloat tcx = (float)(cx+charsetDesc.Chars[index].XOffset);
				GLfloat tcy = (float)(cy+charsetDesc.Chars[index].YOffset);
				GLfloat tcw = (float)(charsetDesc.Chars[index].Width);
				GLfloat tch = (float)(charsetDesc.Chars[index].Height);

				// Increment current cursor position
				cx += charsetDesc.Chars[index].XAdvance;

				color->Set();

				glTexCoord2f(tx,	ty+th);	glVertex2i(x+tcx,		y+tcy+tch);
				glTexCoord2f(tx+tw,	ty+th);	glVertex2i(x+tcx+tcw,		y+tcy+tch);
				glTexCoord2f(tx+tw,	ty);	glVertex2i(x+tcx+tcw,		y+tcy);
				glTexCoord2f(tx,	ty);	glVertex2i(x+tcx,		y+cy);
			}else{
				// If not in extended mode subtract the value of the first
				// char in the character map to get the index in our map
				wchar_t index = text[cp];
				if(extended == 0){
					index -= L' ';
				}else if(extended > 0){
					index -= extended;
				}

				// Increment current cursor position
				cx += cw;

				int col = index%c_per_row;
				int row = (index-col)/c_per_row;

				// find the texture coords
				GLfloat tx = (float)(col * c_width)/(float)m_width;
				GLfloat ty = (float)(row * c_height)/(float)m_height;

				if (dropshadow == 1){
					glColor4ub(0, 0, 0, 127);

					glTexCoord2f(tx,		ty+dty);	glVertex2i(x+cx-1,		y+cy+ch+1);
					glTexCoord2f(tx+dtx,	ty+dty);	glVertex2i(x+cx+cw-1,	y+cy+ch+1);
					glTexCoord2f(tx+dtx,	ty);		glVertex2i(x+cx+cw-1,	y+cy+1);
					glTexCoord2f(tx,		ty);		glVertex2i(x+cx-1,		y+cy+1);
				}
				{
					color->Set();

					glTexCoord2f(tx,		ty+dty);	glVertex2i(x+cx,		y+cy+ch);
					glTexCoord2f(tx+dtx,	ty+dty);	glVertex2i(x+cx+cw,		y+cy+ch);
					glTexCoord2f(tx+dtx,	ty);		glVertex2i(x+cx+cw,		y+cy);
					glTexCoord2f(tx,		ty);		glVertex2i(x+cx,		y+cy);

				}
			}
		}
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		delete [] sbtext;
	}

	KLGLFont::~KLGLFont(){
		delete color;
	}

	bool KLGLFont::ParseFnt(std::istream& stream){
		string line;
		string Read, Key, Value;
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

	KLGLSprite::KLGLSprite(const char *fname, int sWidth, int sHeight){
		(*this) = KLGLSprite(new KLGLTexture(fname), sWidth, sHeight);
	}

	KLGLSprite::~KLGLSprite(){

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
