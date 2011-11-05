#pragma once

#include "kami.h"
namespace klib{

	#define FPM 9
	class LiquidParticleThread : KLGLThread{
	public:
		int status;
		void *particleCompPtr;
		LiquidParticleThread(){};
		LiquidParticleThread(void *parma){
			status = true;
			particleCompPtr = parma;
			start();
		};
		~LiquidParticleThread() {
			status = false;
			if(m_hThread != NULL) {
				stop();
			}
		}
		void run();
	};

	class LiquidParticleType {
	public:
		int x, y, pX, pY, size;
		float vX, vY;
		LiquidParticleType(){
			LiquidParticleType(rand()%100, rand()%100);
		}
		LiquidParticleType(int vx, int vy){
			x     = vx;
			y     = vy;
			pX    = 0;
			pY    = 0;
			vX    = 0;
			vY    = 0;
			size  = 1;
		}
	};

	class LiquidParticles{
	public:
		int canvasW;
		int canvasH;
		int numMovers;
		int numThreads;
		int mouseX;
		int mouseY;
		int isMouseDown;
		float friction;
		float vacAccel;
		LiquidParticleType* movers;
		int threadingState;
		int threadID;

		LiquidParticles(){

		}
		LiquidParticles(int count, POINT *mouseRef, int canvaswidth = 640, int canvasheight = 480);
		~LiquidParticles();
		void initialize();
		void audit(int addremove);
		void simulate();
		void draw(KLGL* gc);
	private:

		int mouseVX;
		int mouseVY;
		int prevMouseX;
		int prevMouseY;

		float Mrnd();
	};
}