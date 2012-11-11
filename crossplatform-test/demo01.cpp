#include "kami.h"
#include <math.h>

#ifdef KLGLENV64 && _WIN32
#pragma comment(lib,"kami64.lib")
#elif defined(_WIN32)
#pragma comment(lib,"kami.lib")
#endif

using namespace klib;

int main(){
	KLGL *gc = nullptr;
	KLGLFont *font = nullptr;
	int status = 1;
	unsigned int cycle = 0;
	char text[4096] = {'\0'};

	gc = new KLGL("hiii~", 640, 480, 60, false, 1, 1);
	gc->bufferAutoSize = true;
	font = new KLGLFont();

	while(status){
		gc->windowManager->ProcessEvent(&status);
		gc->OpenFBO();
		gc->OrthogonalStart();
		{
			double value = (sin(cycle/100.0f)+1.0)/2.0;
			gc->Rectangle2D(0, 0, gc->buffer.width, gc->buffer.height, KLGLColor(value*255, value*255, value*255, 255));
			glTranslated(gc->buffer.width/2, gc->buffer.height/2, 0.0);
			glRotated(cycle, 0.0, 0.0, 1.0);
			glTranslated(-(gc->buffer.width/2), -(gc->buffer.height/2), 0.0);
			time_t buildTime = (time_t)APP_BUILD_TIME;
			char* buildTimeString = asctime(gmtime(&buildTime));
			memset(buildTimeString+strlen(buildTimeString)-1, 0, 1); // Remove the \n
			sprintf(text, "@DKamiLib v0.0.2 R%d %s, %s %s,\n%d", APP_BUILD_VERSION, buildTimeString, APP_ARCH_STRING, APP_COMPILER_STRING, cycle);
			font->Draw(16, gc->buffer.height/2, text);
		}
		gc->OrthogonalEnd();
		gc->Swap();
		cycle++;
	}

	delete gc;
	return 0;
}
