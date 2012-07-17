#include "kami.h"

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
	char text[4096] = {'\0'};

	gc = new KLGL("hiii~", 800, 640, 60, false, 1, 1);
	font = new KLGLFont();

	while(status){
		gc->OpenFBO();
		gc->OrthogonalStart();
		{
			sprintf(text, "Konichiwa~ %d", clock());
			font->Draw(8, 8, text);
		}
		gc->OrthogonalEnd();
		gc->Swap();
	}

	delete gc;
	return 0;
}
