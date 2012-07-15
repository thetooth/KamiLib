#include "kami.h"
#include "picopng.h"
#include "databin.h"
#include "resource.h"
#include <iostream>
#include <string>

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
		FILE *fin = fopen(fname, "rb");

		if(fin == NULL){
			cl("[FAILED]\n");
			throw KLGLException("[KLGLTexture::LoadTexture][%d:%s] \nResource \"%s\" could not be opened.", __LINE__, __FILE__, fname);
			return 1;
		}

		// Pass a big file and we could be sitting here for a long time
		fseek(fin, 0, SEEK_END);
		size_t size = ftell(fin);
		rewind(fin);

		unsigned char *data = new unsigned char[size+8];
		fread((unsigned char*)data, 1, size, fin);
		fclose(fin);

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
		if (APP_ENABLE_MIPMAP)
		{
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
			cl("KamiLib v0.0.2 R%d %s, %s %s,\n", APP_BUILD_VERSION, substr(buildTimeString, 0, strlen(buildTimeString)-1), APP_ARCH_STRING, APP_COMPILER_STRING);
			cl(APP_MOTD);

			// Initialize the window geometry
			window.width		= _width;
			window.height		= _height;

			vsync				= true;
			overSampleFactor	= _OSAA;
			scaleFactor			= _scale;
			fps					= _framerate;
			shaderClock			= 0.0f;

			// Attempt to load runtime configuration
			config = new KLGLINIReader("configuration.ini");
			if (config->ParseError() == -1){
				cl("Failed to load configuration file overrides.\n");
			}else if (config->ParseError()){
				cl("KLGLINIReader, error on line %s\n", config->ParseError());
			}else{
				KLGLDebug		= config->GetBoolean("system", "debug",			KLGLDebug);
				window.x		= config->GetInteger("system", "lastPosX",		0);
				window.y		= config->GetInteger("system", "lastPosY",		0);
				window.width	= config->GetInteger("system", "windowWidth",	_width);
				window.height	= config->GetInteger("system", "windowHeight",	_height);
				vsync			= config->GetBoolean("system", "vsync",			vsync);
				scaleFactor		= config->GetInteger("system", "windowScale",	_scale);
				fullscreen		= config->GetBoolean("system", "fullscreen",	_fullscreen);
				overSampleFactor= config->GetInteger("system", "bufferScale",	_OSAA);
				bufferAutoSize	= config->GetBoolean("system", "bufferAutoSize",	false);
			}

			// Get buffer size from window and sampling parameters
			buffer.width	= window.width*overSampleFactor;
			buffer.height	= window.height*overSampleFactor;

			// Init window
			windowManager = new KLGLWindowManager(_title, window, scaleFactor, fullscreen);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			// Continue setup of OpenGL and framebuffer
			GenFrameBuffer(fbo[0], fbo_texture[0], fbo_depth[0]);
			GenFrameBuffer(fbo[1], fbo_texture[1], fbo_depth[1]);

			// Initialize gl parmas
			glViewport(0, 0, window.width*scaleFactor, window.height*scaleFactor);	// Create initial 2D view
			glEnable(GL_TEXTURE_2D);												// Enable Texture Mapping ( NEW )
			glShadeModel(GL_SMOOTH);												// Enable Smooth Shading
			glClearColor(ubtof(153), ubtof(51), ubtof(51), ubtof(255));				// Background Color
			glClearDepth(1.0f);														// Depth Buffer Setup
			glEnable(GL_DEPTH_TEST);												// Enables Depth Testing
			glDepthFunc(GL_LEQUAL);													// The Type Of Depth Testing To Do
			glAlphaFunc(GL_GREATER, 0.0f);
			glEnable(GL_ALPHA_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);						// Really Nice Perspective Calculations

			light_diffuse = Vec4<GLfloat>(1.0, 1.0, 1.0, 1.0);
			light_position = Vec4<GLfloat>(1.0, 1.0, 1.0, 0.0);						// Default Infinite light location.
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse.Data());
			glLightfv(GL_LIGHT0, GL_POSITION, light_position.Data());
			glEnable(GL_LIGHT0);

			// Init OpenGL OK!
			cl("Initialized %s OpenGL %s @%dx%d\n", glGetString(GL_VENDOR), glGetString(GL_VERSION), buffer.width, buffer.height);

			// Initialize the Lua API
#ifdef APP_ENABLE_LUA
			lua = luaL_newstate();
			if (lua != NULL){
				cl("Initialized %s\n", LUA_VERSION);
			}else{
				throw KLGLException("luaInit: %s", lua_tostring(lua, -1));
			}
			luaL_openlibs(lua);
			luaL_openkami(lua);
#endif

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


#ifdef APP_ENABLE_LUA
		lua_close(lua);
#endif

		cl("\n0x%x :3\n", internalStatus);
		cl(NULL);
	}

	void KLGL::ProcessEvent(int *status){
		if(PeekMessage(&windowManager->wm->msg, NULL, NULL, NULL, PM_REMOVE)){
			if(windowManager->wm->msg.message == WM_QUIT){
				PostQuitMessage(0);
				*status = 0;
			}else{
				TranslateMessage(&windowManager->wm->msg);
				DispatchMessage(&windowManager->wm->msg);
			}
		}
	}

	void KLGL::OpenFBO(float fov, float eyex, float eyey, float eyez){
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
		glLoadIdentity();
		glViewport(0, 0, buffer.width, buffer.height);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); //Clear the colour buffer (more buffers later on)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[0]); // Bind our frame buffer for rendering
		glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT); // Push our glEnable and glViewport states

		// Set the size of the frame buffer view port
		glViewport(0, 0, buffer.width, buffer.height);
		glMatrixMode(GL_PROJECTION);
		//glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		gluPerspective(fov, 1.0f*buffer.width/buffer.height, 0.1, 100.0);
		gluLookAt(eyex, eyey, eyez, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);

		// Set MODELVIEW matrix mode
		glMatrixMode(GL_MODELVIEW);
		//glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_LIGHTING);
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
		glUniform2f(glGetUniformLocation(GetShaderID(0), "BUFFER_EXTENSITY"), window.width*scaleFactor, window.height*scaleFactor);
		shaderClock = (clock()%1000)+fps;

		glBindTexture(GL_TEXTURE_2D, fbo_texture[0]); // Bind our frame buffer texture
		glBegin(GL_QUADS);
		{
			glTexCoord2d(0.0,0.0); glVertex3d(-1.0,-1.0, 0);
			glTexCoord2d(1.0,0.0); glVertex3d(+1.0,-1.0, 0);
			glTexCoord2d(1.0,1.0); glVertex3d(+1.0,+1.0, 0);
			glTexCoord2d(0.0,1.0); glVertex3d(-1.0,+1.0, 0);
		}
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind any textures
		UnbindShaders();

		// Swap!
		windowManager->Swap();
	}

	void KLGL::GenFrameBuffer(GLuint &fbo, GLuint &fbo_texture, GLuint &fbo_depth, int bufferWidth, int bufferHeight) {

		if (bufferWidth == 0 || bufferHeight == 0){
			bufferWidth = buffer.width;
			bufferHeight = buffer.height;
		}
		// Depth Buffer
		glGenTextures(1, &fbo_depth);
		glBindTexture(GL_TEXTURE_2D, fbo_depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, bufferWidth, bufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

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

		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, fbo_depth);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fbo_texture, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

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
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, (buffer.width/overSampleFactor)*scale, 0, (buffer.height/overSampleFactor)*scale, -1, 1);
		glScalef(1.0f, -1.0f, 1.0f);
		glTranslatef(0.0f, -(buffer.height/overSampleFactor)*scale, 0.0f);
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
	}

	void KLGL::OrthogonalEnd(){
		glEnable(GL_LIGHTING);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}

	int KLGL::InitShaders(int shaderProgId, int isString, const char *vsFile, const char *fsFile, const char *gsFile, const char *tsFile){
		cl("Initializing Shader ID %d\n", shaderProgId);

		const char* sData;
		shader_id[shaderProgId] = glCreateProgram();

#define sLoad(x) (isString ? x : file_contents(const_cast<char*>(x)))

		sData = sLoad(vsFile);
		if (vsFile != NULL && sData != NULL){
			cl("Compiling %s\n", vsFile);
			shader_vp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_VERTEX_SHADER,	 sData);
		}
		sData = sLoad(fsFile);
		if (fsFile != NULL && sData != NULL){
			cl("Compiling %s\n", fsFile);
			shader_fp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_FRAGMENT_SHADER, sData);
		}
		sData = sLoad(gsFile);
		if (gsFile != NULL && sData != NULL){
			cl("Compiling %s\n", gsFile);
			shader_gp[shaderProgId] = ComputeShader(shader_id[shaderProgId], GL_GEOMETRY_SHADER, sData);
		}
		sData = sLoad(tsFile);
		if (tsFile != NULL && sData != NULL){
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

	char* KLGL::LoadShaderFile(const char *fname){
		char* shaderString = NULL;
		FILE *file = fopen(fname, "rt");
		if(fname != NULL && file != NULL){
			fseek(file, 0, SEEK_END);
			int count = ftell(file);
			rewind(file);

			if (count > 0) {
				shaderString = (char*)malloc(count + 1);
				count = fread(shaderString, sizeof(char), count, file);
				shaderString[count] = '\0';
			}
			fclose(file);
		}
		if (shaderString == NULL)
		{
			throw KLGLException("Error loading shader file: %s", fname);
		}
		return shaderString;
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
			//cl("[ERROR]\n");
			infoLog = (char *)malloc(infoLogLength);
			if(isShader){
				glGetShaderInfoLog(obj, infoLogLength, &charsWritten, infoLog);
			}else{
				glGetProgramInfoLog(obj, infoLogLength, &charsWritten, infoLog);
			}
			infoLogRemainLength = infoLogLength;
			while(infoLogRemainLength > 0){
				cl("%s", substr(infoLog, infoLogLength-infoLogRemainLength, min(512, infoLogLength)));
				infoLogRemainLength -= 512;
			}
			free(infoLog);
		}
	}

	void KLGL::BindMultiPassShader(int shaderProgId, int alliterations, bool flipOddBuffer, float x, float y, float width, float height){
		if (width < 0.0f || height < 0.0f)
		{
			width = window.width;
			height = window.height;
		}

		glPushMatrix();
		glLoadIdentity();

		for (int i = 0; i < alliterations; i++)
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[1]);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glBindTexture(GL_TEXTURE_2D, fbo_texture[0]);

			BindShaders(shaderProgId);
			glBegin(GL_QUADS);
			glTexCoord2d(0.0,0.0); glVertex2i(x,			y);
			glTexCoord2d(1.0,0.0); glVertex2i(width,		y);
			glTexCoord2d(1.0,1.0); glVertex2i(width,		height);
			glTexCoord2d(0.0,1.0); glVertex2i(x,			height);
			glEnd();
			UnbindShaders();
			glBindTexture(GL_TEXTURE_2D, 0);

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[0]);
			//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			glBindTexture(GL_TEXTURE_2D, fbo_texture[1]);

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
		static wchar_t* tmpStr;
		if (tmpStr == NULL || (tmpStr != NULL && wcslen(tmpStr) < len)){
			tmpStr = (wchar_t*)malloc(len*sizeof(wchar_t));
		}
		tmpStr[len] = L'\0';
		mbstowcs(tmpStr, text, len);
		Draw(x, y, tmpStr, vcolor);
		//delete tmpStr;
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
		static size_t lastStringLen;

		//calculate how wide each character is in term of texture coords
		GLfloat dtx = (float)c_width/(float)m_width;
		GLfloat dty = (float)c_height/(float)m_height;

		static char* sbtext;
		if (sbtext == NULL || lastStringLen < stringLen){
			sbtext = (char*)malloc(stringLen+1);
			lastStringLen = stringLen;
		}
		sbtext[stringLen] = '\0';

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
					static char *token;
					if (token == NULL){
						token = (char*)malloc(6);
					}

					memset(token, 0, 6);
					strcpy(token, substr(sbtext, cp+2, 6));
					c += 7;
					cp+= 7;

					if (vcolor != NULL){ // Ignore color codes if global parameter is set
						color = vcolor;
						break;
					}else{
						color->r = strtoul(substr(token, 0, 2), NULL, 16);
						color->g = strtoul(substr(token, 2, 2), NULL, 16);
						color->b = strtoul(substr(token, 4, 2), NULL, 16);
						color->a = 255;
					}
					continue;
				}else if (*(c+1) == 'D'){ // Drop shadow
					c += 1;
					cp+= 1;
					dropshadow = 1;
					continue;
				}
				break;
			}

			// Increment current cursor position
			cx += cw;

			// If not in extended mode subtract the value of the first
			// char in the character map to get the index in our map
			int index = text[cp];
			if(extended == 0){
				index -= L' ';
			}else if(extended > 0){
				index -= extended;
			}

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
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	KLGLFont::~KLGLFont(){
		delete color;
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