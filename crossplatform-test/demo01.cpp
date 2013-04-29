#include "kami.h"
#include <math.h>
#include <algorithm>
#include <wchar.h>

#include "lua.hpp"

#ifdef KLGLENV64 && _WIN32
#pragma comment(lib,"kami64.lib")
#elif defined(_WIN32)
#pragma comment(lib,"kami.lib")
#pragma comment(lib,"lua51.lib")
#endif

using namespace klib;

struct System
{
	KLGL gc;
	KLGLFont* msg;
	KLGLTexture* test[32];
};

void report_errors(lua_State *L, int status){
	if ( status!=0 ) {
		cl("-- %s", lua_tostring(L, -1));
		lua_pop(L, 1); // remove error message
	}
}

static int l_my_print(lua_State* L){
	int nargs = lua_gettop(L);

	for (int i=1; i <= nargs; i++) {
		if (lua_isstring(L, i)) {
			/* Pop the next arg using lua_tostring(L, i) and do your print */
			cl(">%s", lua_tostring(L, i));
		}else{
			/* Do something with non-strings if you like */
		}
	}

	return 0;
}

static const struct luaL_reg printlib [] = {
	{"print", l_my_print},
	{NULL, NULL} /* end of array */
};

extern int luaopen_luamylib(lua_State *L){
	luaL_register(L, "demo", printlib);
	return 1;
}

int main(){
	int status = 1, cycle = 0;
	clock_t t0 = clock(),t1 = 0,t2 = 0;
	System sys = {KLGL("Demo 01", 160, 120)};
	wchar_t* buffer = new wchar_t[4096];
	wchar_t* inputBuffer = new wchar_t[256];
	buffer[0] = L'\0';
	inputBuffer[0] = L'\0';
	//fill_n(inputBuffer, 4096, '\0');

	sys.test[0] = new KLGLTexture("res/unicode_0.png");
	sys.test[1] = new KLGLTexture("res/test3.png");
	ifstream fontdescFile("res/unicode.fnt");
	sys.msg = new KLGLFont(sys.test[0]->gltexture, sys.test[0]->width, sys.test[0]->height, 17, 19, 0);
	sys.msg->ParseFnt(fontdescFile);

	sys.gc.InitShaders(0, 0, "res/CRT-geom.vsh", "res/CRT-geom_rgb32_lut.fsh");
	sys.gc.InitShaders(1, 0, "res/crt.vert", "res/particles.frag");

	lua_State *L = lua_open();
	luaopen_luamylib(L);
	luaL_openlibs(L);

	const char* bootScript = "print = demo.print";
	int Ls = luaL_loadbuffer(L, bootScript, strlen(bootScript), "bootScript");
	lua_pcall(L, 0, 0, 0);
	report_errors(L, Ls);

	while(sys.gc.ProcessEvent(&status)){
		sys.gc.HandleEvent([&] (std::vector<KLGLKeyEvent>::iterator key) {
			//cl("Key: %d, '%c', 0x%X, 0x%X, %s\n", key->getCode(), key->getChar(), key->getModifier(), key->getNativeKeyCode(), (key->isDown() ? "DOWN" : "UP"));
			if (key->isDown()){
				if(key->getCode() == KLGLKeyEvent::KEY_RETURN){
					char tempBuffer[256] = {0};
					wcstombs(tempBuffer, inputBuffer, wcslen(inputBuffer));
					int s = luaL_loadbuffer(L, tempBuffer, strlen(tempBuffer), "InputBuffer");

					if ( s==0 ) {
						// execute Lua program
						s = lua_pcall(L, 0, 0, 0);
					}

					report_errors(L, s);
					inputBuffer[0] = L'\0';
				}else if(key->getCode() == KLGLKeyEvent::KEY_BACKSPACE && wcslen(inputBuffer) > 0){
					inputBuffer[wcslen(inputBuffer)-1]='\0';
				}else if(key->getChar() >= 32 && key->getChar() < 128 && key->getModifier() == 0){
					wchar_t temp[2] = { key->getChar(), 0 };
					wcscat(inputBuffer, temp);
				}
			}else{
				if (key->getCode() == KLGLKeyEvent::KEY_ESCAPE){
					status = 0;
				}
			}
			key->finished();
		});
		sys.gc.OpenFBO();
		sys.gc.OrthogonalStart();
		{
			sys.gc.Rectangle2D(0,0,sys.gc.buffer.width,sys.gc.buffer.height,KLGLColor(20, 20, 20, 255));
			KLGLColor(255, 255, 255, 255).Set();

			/*sys.gc.BindShaders(1);
			glUniform1f(glGetUniformLocation(sys.gc.GetShaderID(1), "time"), sys.gc.shaderClock/100.0f);
			glUniform2f(glGetUniformLocation(sys.gc.GetShaderID(1), "resolution"), sys.gc.buffer.width, sys.gc.buffer.height);
			sys.gc.UnbindShaders();
			sys.gc.BindMultiPassShader(1, 1, 0);*/

			sys.gc.Blit2D(sys.test[1], (sys.gc.buffer.width/2)-(sys.test[1]->width/2), (sys.gc.buffer.height/2)-(sys.test[1]->height/2), cycle/10.0f);
			swprintf(buffer, 4096, L"%d:%ls%c::\n", cycle, inputBuffer, (cycle%30 <= 15 ? '_' : ' '));
			sys.msg->Draw(8, 8, buffer);
		}
		sys.gc.OrthogonalEnd();
		sys.gc.Swap();
		cycle++;
	}

	lua_close(L);
	return 0;
}
