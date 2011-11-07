#include "kami.h"
#include "picopng.h"
#include "databin.h"
#include "resource.h"
#include <iostream>
#include <string>

namespace klib{

	// Make extern local to klib
	char clBuffer[1024] = {};
	bool KLGLDebug = false;
	KLGLMutex clBufferSpinLock("KLGL_Console");

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
			cl("[FAILED] Resource not found.\n");
			return 1;
		}

		// Pass a big file and we could be sitting here for a long time
		fseek(fin, 0, SEEK_END);
		unsigned int size = (unsigned)ftell(fin);
		unsigned char *data = new unsigned char[size+1];
		rewind(fin);
		fread((char*)data, sizeof(char), size, fin);
		data[size] = '\0';
		fclose(fin);
		InitTexture(data, size);
		delete[] data;
		cl("[OK]\n");
		return 0;
	}

	int KLGLTexture::InitTexture(unsigned char *data, int size){
		std::vector<unsigned char> swap;
		// PNG DECODER OMG OMG OMG
		decodePNG(swap, width, height, data, size);
		//delete data;

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

	KLGL::KLGL(const char* title, int width, int height, int framerate, bool fullscreen, int OSAA, int scale, float anisotropy){

		try{
			// State
#if defined(_DEBUG)
			KLGLDebug = true;
#else
			KLGLDebug = false;
#endif

			// Clear the console buffer(VERY IMPORTANT)
			fill_n(clBuffer, 1024, '\0');

			//MOTD
			cl("KamiLib, (c) 2005-2011 Ameoto Systems Inc. All Rights Reserved.\n\n");

			// Initialize the window geometry
			window.width		= width;
			window.height		= height;

			vsync				= true;
			overSampleFactor	= OSAA;
			scaleFactor			= scale;
			fps					= framerate;
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
				window.width	= config->GetInteger("system", "windowWidth",	width);
				window.height	= config->GetInteger("system", "windowHeight",	height);
				vsync			= config->GetBoolean("system", "vsync",			vsync);
				scaleFactor		= config->GetInteger("system", "windowScale",	scale);
				fullscreen		= config->GetBoolean("system", "fullscreen",	fullscreen);
				overSampleFactor= config->GetInteger("system", "bufferScale",	OSAA);
			}

			// Get buffer size from window and sampling parameters
			buffer_width	= window.width*overSampleFactor;
			buffer_height	= window.height*overSampleFactor;

			// Register win32 class
			wc.style = CS_OWNDC;
			wc.lpfnWndProc = WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = GetModuleHandle(NULL);
			wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
			wc.hCursor = LoadCursor( NULL, IDC_ARROW );
			wc.hbrBackground = (HBRUSH)GetStockObject( NULL_BRUSH );
			wc.lpszMenuName = NULL;
			wc.lpszClassName = "KamiGLWnd32";
			RegisterClass(&wc);

			// create main window
			if(fullscreen){
				// Device Mode
				DEVMODE dmScreenSettings;
				memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
				dmScreenSettings.dmSize=sizeof(dmScreenSettings);
				dmScreenSettings.dmPelsWidth	= window.width*scaleFactor;
				dmScreenSettings.dmPelsHeight	= window.height*scaleFactor;
				dmScreenSettings.dmBitsPerPel	= 32;
				dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

				if(ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL){
					cl("The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Will Use Windowed Mode Instead.\n\n");
					fullscreen = false;
				}else{
					hWnd = CreateWindowEx(WS_EX_APPWINDOW, "KamiGLWnd32", title, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, NULL, NULL, window.width*scaleFactor, window.height*scaleFactor, NULL, NULL, wc.hInstance, NULL );
				}
			}

			if(!fullscreen){
				hWnd = CreateWindowEx(WS_EX_APPWINDOW, "KamiGLWnd32", title, WS_CAPTION | WS_POPUPWINDOW, window.x, window.y, window.width*scaleFactor+6, window.height*scaleFactor+GetSystemMetrics(SM_CYCAPTION)+6, NULL, NULL, wc.hInstance, NULL );
			}

			SetWindowText(hWnd, "Loading...");

			PIXELFORMATDESCRIPTOR pfd;
			int format;

			// get the device context (DC)
			hDC = GetDC( hWnd );

			// set the pixel format for the DC
			ZeroMemory( &pfd, sizeof( pfd ) );
			pfd.nSize = sizeof( pfd );
			pfd.nVersion = 1;
			pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.cColorBits = 24;
			pfd.cDepthBits = 16;
			pfd.iLayerType = PFD_MAIN_PLANE;
			format = ChoosePixelFormat( hDC, &pfd );
			SetPixelFormat( hDC, format, &pfd );

			// create and enable the render context (RC)
			hRC = wglCreateContext( hDC );
			wglMakeCurrent( hDC, hRC );

			ShowWindow(hWnd,SW_SHOW);
			SetForegroundWindow(hWnd);
			SetFocus(hWnd);

			glewExperimental = GL_TRUE;
			GLenum glewIinitHandle = glewInit();
			if(glewIinitHandle != GLEW_OK)
			{
				cl("Error: %s\n", glewGetErrorString(glewIinitHandle));
				exit(EXIT_FAILURE);
			}
			glewIinitHandle = NULL;

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			// Continue setup of OpenGL and framebuffer
			GenFrameBuffer(fbo[0], fbo_texture[0], fbo_depth[0]);
			GenFrameBuffer(fbo[1], fbo_texture[1], fbo_depth[1]);

			// Initialize gl parmas
			glViewport(0, 0, window.width*scaleFactor, window.height*scaleFactor);				// Create initial 2D view
			glEnable(GL_TEXTURE_2D);												// Enable Texture Mapping ( NEW )
			glShadeModel(GL_SMOOTH);												// Enable Smooth Shading
			glClearColor(ubtof(153), ubtof(51), ubtof(51), ubtof(255));									// Black Background
			glClearDepth(1.0f);														// Depth Buffer Setup
			glEnable(GL_DEPTH_TEST);												// Enables Depth Testing
			glDepthFunc(GL_LEQUAL);													// The Type Of Depth Testing To Do
			glAlphaFunc(GL_GREATER, 0.0f);
			glEnable(GL_ALPHA_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);						// Really Nice Perspective Calculations

			GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};  /* Diffuse light. */
			GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};  /* Infinite light location. */
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			glEnable(GL_LIGHT0);

			// Vertical synchronization
			typedef bool(APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
			PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
			wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
			if(wglSwapIntervalEXT ){
				wglSwapIntervalEXT(vsync);
			}

			// Init OpenGL OK!
			cl("Initialized GL %s @%dx%d 32bits\n", glGetString(GL_VERSION), buffer_width, buffer_height);

			// Initialize the Lua API
			lua = lua_open();
			if (lua != NULL){
				cl("Initialized %s\n", LUA_VERSION);
			}else{
				throw KLGLException("luaInit: %s", lua_tostring(lua, -1));
			}
			luaL_openlibs(lua);
			luaL_openkami(lua);

			// Draw logo and load internal resources
			InfoBlue = new KLGLTexture(KLGLInfoBluePNG, 205);
			CheepCheepDebug = new KLGLTexture(KLGLCheepCheepDebugPNG, 296);
			klibLogo = new KLGLTexture(KLGLStartupLogoPNG, 370);

			framebuffer = new KLGLTexture();
			framebuffer->width = window.width;
			framebuffer->height = window.height;

//#if defined(_DEBUG)
			OrthogonalStart();
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			//Tile2D(InfoBlue, 0, 0, buffer_width/32, buffer_height/32);
			Blit2D(klibLogo, 18, window.height-32);
			OrthogonalEnd();
			SwapBuffers(hDC);
//#endif

			// Setup audio API
			//audio = new KLGLSound();

			internalStatus = 0;

		}catch(KLGLException e){
			MessageBox(NULL, e.getMessage(), "KLGLException", MB_OK | MB_ICONERROR);
			exit(EXIT_FAILURE);
		}
	}

	KLGL::~KLGL(){
		for (int i = 0; i < 10; i++)
		{
			UnloadShader(i);
		}

		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hRC);
		ReleaseDC(hWnd, hDC);
		DeleteDC(hDC);

		// destroy the window explicitly
		DestroyWindow(hWnd);

		lua_close(lua);

		clBufferSpinLock.release();

		cl("\n0x%x :3\n", internalStatus);
	}

	void KLGL::OpenFBO(float fov, float eyex, float eyey, float eyez){
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo[0]); // Bind our frame buffer for rendering
		glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT); // Push our glEnable and glViewport states

		// Set the size of the frame buffer view port
		glViewport(0, 0, buffer_width, buffer_height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(fov, 1.0f*buffer_width/buffer_height, 0.1, 1000.0);
		gluLookAt(eyex, eyey, eyez, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);

		// Set MODELVIEW matrix mode
		glMatrixMode(GL_MODELVIEW);
		//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_LIGHTING);
	}

	void KLGL::Swap(){
		// Restore our glEnable and glViewport states and unbind our texture
		glPopAttrib();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); //Clear the colour buffer (more buffers later on)
		glLoadIdentity(); // Load the Identity Matrix to reset our drawing locations
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glDisable(GL_LIGHTING);

		BindShaders(1);
		glUniform1f(glGetUniformLocation(GetShaderID(1), "time"), shaderClock);
		shaderClock += 0.0001f;
		if (shaderClock > fps/100.0f)
		{
			shaderClock = 0.0f;
		}

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
		SwapBuffers(hDC);
	}

	void KLGL::GenFrameBuffer(GLuint& fbo, GLuint &fbo_texture, GLuint &fbo_depth) {
		// Depth Buffer
		glGenRenderbuffersEXT(1, &fbo_depth); // Generate one render buffer and store the ID in fbo_depth  
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo_depth); // Bind the fbo_depth render buffer  
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, buffer_width, buffer_height); // Set the render buffer storage to be a depth component, with a width and height of the window  
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbo_depth); // Set the render buffer of this buffer to the depth buffer 
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0); // Unbind the render buffer

		// Texture Buffer
		glGenTextures(1, &fbo_texture);
		glBindTexture(GL_TEXTURE_2D, fbo_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, buffer_width, buffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbo_depth);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

		switch (status) {
		case GL_FRAMEBUFFER_COMPLETE:
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			throw KLGLException("Error: unsupported framebuffer format!\n");
			break;
		default:
			throw KLGLException("Error: invalid framebuffer config!\n");
		}
	}

	void KLGL::Blit2D(KLGLTexture* texture, int x, int y, float rotation, KLGLColor vcolor){
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

			glBegin(GL_QUADS);
			{
				vcolor.Set();
				glTexCoord2d(0.0,0.0); glVertex2i(x,				y);
				glTexCoord2d(1.0,0.0); glVertex2i(x+texture->width,	y);
				glTexCoord2d(1.0,1.0); glVertex2i(x+texture->width,	y+texture->height);
				glTexCoord2d(0.0,1.0); glVertex2i(x,				y+texture->height);
			}
			glEnd();
		}
		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void KLGL::BlitSprite2D(KLGLSprite* sprite, int x, int y, int row, int col, bool managed){
		if (managed)
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, sprite->texturePtr->gltexture);
			glBegin(GL_QUADS);
		}
		{
			//calculate how wide each character is in term of texture coords
			GLfloat dtx = (float)sprite->swidth/(float)sprite->texturePtr->width;
			GLfloat dty = (float)sprite->sheight/(float)sprite->texturePtr->height;

			// find the texture coords
			GLfloat tx = (float)(col * sprite->swidth)/(float)sprite->width;
			GLfloat ty = (float)(row * sprite->sheight)/(float)sprite->height;

			glTexCoord2f(tx,		ty+dty);	glVertex2i(x,				y+sprite->sheight);
			glTexCoord2f(tx+dtx,	ty+dty);	glVertex2i(x+sprite->swidth,	y+sprite->sheight);
			glTexCoord2f(tx+dtx,	ty);		glVertex2i(x+sprite->swidth,	y);
			glTexCoord2f(tx,		ty);		glVertex2i(x,				y);
		}
		if (managed)
		{
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
				if(scrX < 0-texture->width || scrX > buffer_width || scrY < 0-texture->height || scrY > buffer_height){
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

	void KLGL::OrthogonalStart(){
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, window.width, 0, window.height, -1, 1);
		glScalef(1.0f, -1.0f, 1.0f);
		glTranslatef(0.0f, -window.height*1.0f, 0.0f);
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
	}

	void KLGL::OrthogonalEnd(){
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glEnable(GL_LIGHTING);
	}

	int KLGL::InitShaders(int shaderProgId, const char *vsFile, const char *fsFile, int isString){
		cl("Computing Shaders...");

		shader_vp[shaderProgId] = glCreateShader(GL_VERTEX_SHADER);
		shader_fp[shaderProgId] = glCreateShader(GL_FRAGMENT_SHADER);

		const char* vsText;
		const char* fsText;

		if (isString){
			vsText = vsFile;
			fsText = fsFile;
		}else{
			vsText = static_cast<char*>(LoadShaderFile(vsFile));
			fsText = static_cast<char*>(LoadShaderFile(fsFile));
		}

		if (vsText == NULL || fsText == NULL) {
			cl("Vertex or fragment shader error.\n");
			return -1;
		}

		glShaderSource(shader_vp[shaderProgId], 1, &vsText, 0);
		glShaderSource(shader_fp[shaderProgId], 1, &fsText, 0);

		glCompileShader(shader_vp[shaderProgId]);
		glCompileShader(shader_fp[shaderProgId]);

		if(KLGLDebug){
			PrintShaderInfoLog(shader_vp[shaderProgId]);
			PrintShaderInfoLog(shader_fp[shaderProgId]);
		}

		shader_id[shaderProgId] = glCreateProgram();
		glAttachShader(shader_id[shaderProgId], shader_fp[shaderProgId]);
		glAttachShader(shader_id[shaderProgId], shader_vp[shaderProgId]);

		glLinkProgram(shader_id[shaderProgId]);
		if(KLGLDebug){
			PrintShaderInfoLog(shader_id[shaderProgId], 0);
		}
		if (!isString){
			free(const_cast<char*>(vsText));
			free(const_cast<char*>(fsText));
		}

		cl("[OK]\n");
		return 0;
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
		int infologLength = 0;
		int charsWritten = 0;
		char *infoLog;

		glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
		glGetShaderiv(obj, GL_COMPILE_STATUS, &status);

		if (status != 1 && infologLength > 0){
			infoLog = (char *)malloc(infologLength);
			if(isShader){
				glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
			}else{
				glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
			}
			cl(" ! 0x%X\n%s\n", status, infoLog);
			free(infoLog);
		}
	}

	KLGLFont::KLGLFont(){
		auto *tmtex = new KLGLTexture(KLGLFontDefault, 760);
		c_texture = tmtex->gltexture;
		m_width = 1097;
		m_height= 10;
		c_width = 8;
		c_height= 10;
		extended = 0;
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

	void KLGLFont::Draw(int x, int y, char* text)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, c_texture);
		glBegin(GL_QUADS);
		{
			//character location and dimensions
			int cp = 0;
			int cx = 0;
			int cy = 0;
			int cw = c_width;
			int ch = c_height;
			int twidth = 4;

			//calculate how wide each character is in term of texture coords
			GLfloat dtx = (float)c_width/(float)m_width;
			GLfloat dty = (float)c_height/(float)m_height;

			for (char* c = text; *c != 0; c++,cp++) {
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
						static char *token = (char*)malloc(8);
						strcpy(token, substr(text, cp+2, 6));

						color->r = strtoul(substr(token, 0, 2), NULL, 16);
						color->g = strtoul(substr(token, 2, 2), NULL, 16);
						color->b = strtoul(substr(token, 4, 2), NULL, 16);

						c += 8;
					}
					break;
				}

				color->Set();

				// Increment current cursor posision
				cx += cw;

				// If not in extended mode subtract the value of the first
				// char in the character map to get the index in our map
				int index = *c;
				if(extended == 0){
					index -= ' ';
				}else if(extended > 0){
					index -= extended;
				}

				int col = index%c_per_row;
				int row = (index-col)/c_per_row;

				// find the texture coords
				GLfloat tx = (float)(col * c_width)/(float)m_width;
				GLfloat ty = (float)(row * c_height)/(float)m_height;

				glTexCoord2f(tx,		ty+dty);	glVertex2i(x+cx,	y+cy+ch);
				glTexCoord2f(tx+dtx,	ty+dty);	glVertex2i(x+cx+cw,	y+cy+ch);
				glTexCoord2f(tx+dtx,	ty);		glVertex2i(x+cx+cw,	y+cy);
				glTexCoord2f(tx,		ty);		glVertex2i(x+cx,	y+cy);
			}
		}
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	KLGLFont::~KLGLFont(){
		//delete color;
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