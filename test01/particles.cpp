// Liquid particles canvas experiment
// 2010 spielzeugz.de, C++ port by thetooth

#include "particles.h"

namespace klib{
	LiquidParticles::LiquidParticles(int count, POINT *mouseRef, int canvaswidth, int canvasheight){
		canvasW = canvaswidth;
		canvasH = canvasheight;
		friction = 0.96f;
		vacAccel = 0.002f;
		numMovers = count;
		numThreads = 1;
		threadID = 0;
		threadingState = 1;
		isMouseDown = 0;
		mouseX = mouseRef->x;
		mouseY = mouseRef->y;

		initialize();
	}

	LiquidParticles::~LiquidParticles(){
		delete [] movers;
		movers = NULL;
	}

	float LiquidParticles::Mrnd(){
		srand(clock()*(rand()%100));
		return (rand()%100)/100.0f;
	}

	void LiquidParticles::initialize(){
		int i = numMovers;
		movers = new LiquidParticleType[10000000];
		while ( i-- ){
			movers[i] = LiquidParticleType(canvasW * 0.5f, canvasH * 0.5f);
		}

		mouseX = prevMouseX = canvasW * 0.5f;
		mouseY = prevMouseY = canvasH * 0.5f;
		mouseVX = mouseVY = 0.0f;
	}

	void LiquidParticles::audit(int addremove){
		if (numMovers+addremove < 0 || numMovers > 10000000)
		{
			return;
		}
		if (addremove > 0)
		{
			for(int i = 0; i < addremove; i++){
				movers[numMovers+i] = LiquidParticleType(canvasW * 0.5f, canvasH * 0.5f);
			}
			numMovers += addremove;
		}else if (addremove < 0)
		{
			for(int i = 0; i < addremove; i--){
				delete &movers[numMovers-i];
			}
			numMovers += addremove;
		}
	}

	void LiquidParticles::simulate(){
		mouseVX    = mouseX - prevMouseX;
		mouseVY    = mouseY - prevMouseY;
		prevMouseX = mouseX;
		prevMouseY = mouseY;

		float toDist   = canvasW * 0.86f;
		float stirDist = canvasW * 0.125f;
		float blowDist = canvasW * 0.125f;

		int threadedParts = numMovers/numThreads;
		int i = numMovers-(threadedParts*threadID);
		int tpos = i;
		threadID++;
		while(threadingState && i != NULL && i > 0 && i--){
			if (i < tpos-threadedParts)
			{
				i = 0;
				continue;
			}
			if (&movers[i] == NULL)
			{
				throw KLGLThreadException("General Error, CORRUPT ITEM IN JOB!");
				continue;
			}
			int x  = movers[i].x;
			int y  = movers[i].y;
			float vX = movers[i].vX;
			float vY = movers[i].vY;

			int dX = x - mouseX;
			int dY = y - mouseY;
			float d  = sqrtf( dX * dX + dY * dY );
			float a = atan2f(dY, dX);
			float cosA = cosf(a);
			float sinA = sinf(a);

			if ( !isMouseDown ){
				if ( d < blowDist ){
					float blowAcc = ( 1 - ( d / blowDist ) ) * 8;
					vX += cosA * blowAcc + 0.5f - Mrnd();
					vY += sinA * blowAcc + 0.5f - Mrnd();
				}
			}else{

				if ( d < toDist ){
					float toAcc = ( 1 - ( d / toDist ) ) * canvasW * vacAccel;
					vX -= cosA * toAcc;
					vY -= sinA * toAcc;
				}

				if ( d < stirDist ){
					float mAcc = ( 1 - ( d / stirDist ) ) * canvasW * 0.00013f;
					vX += mouseVX * mAcc;
					vY += mouseVY * mAcc;
				}}

			vX *= friction;
			vY *= friction;

			float avgVX = abs(vX);
			float avgVY = abs(vY);
			float avgV  = ( avgVX + avgVY ) * 0.5f;

			if( avgVX < .1f ){
				vX *= Mrnd() * 3;
			}
			if( avgVY < .1f ){
				vY *= Mrnd() * 3;
			}

			float sc = avgV * 0.45f;
			sc = max( min( sc , 2.0f ) , 1.0f );

			// Gravity
			vY += Mrnd();

			float nextX = x + vX;
			float nextY = y + vY;

			if ( nextX > canvasW ){
				nextX = canvasW;
				vX *= -1.0f*Mrnd();
			}
			else if ( nextX < 0 ){
				nextX = 0;
				vX *= -1.0f*Mrnd();
			}

			if ( nextY > canvasH ){
				nextY = canvasH;
				vY *= -1.0f*Mrnd();
			}
			else if ( nextY < 0 ){
				nextY = 0;
				vY *= -1.0f*Mrnd();
			}

			movers[i].vX = vX;
			movers[i].vY = vY;
			movers[i].pX = x;
			movers[i].pY = y;
			movers[i].x  = nextX;
			movers[i].y  = nextY;
			movers[i].size = sc;
		}

		if (threadID >= numThreads)
		{
			threadID = 0;
		}
	}

	void LiquidParticles::draw(KLGL *gc){
		//glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		glBegin(GL_LINES);
		for(int i = numMovers; i > 0; i--){
			if (&movers[i] == NULL)
			{
				continue;
			}
			//glColor4f(1.0f, 1.0f, 1.0f, movers[i].size/2.0f);
			//gc->Blit2D(gc->CheepCheepDebug, movers[i].x, movers[i].y, movers[i].vX*10.0f);
			glVertex2f(movers[i].pX-1, movers[i].pY);
			glVertex2i(movers[i].x, movers[i].y);
		}
		glEnd();
	}

	void LiquidParticleThread::run()
	{
		// Setup our local clock
		clock_t t;
		clock_t t_prev;
		t = t_prev = clock();

		// Create a pointer to the reference of our particle pointer...
		LiquidParticles &particleTestPtr = *((LiquidParticles*)particleCompPtr);

		// Run for as long as status = 1, sleeping when non zero, otherwise we terminate the thread!
		while(status != 0){
			while(status == 1){
				t = clock();
				if(t - t_prev > 1000.0f/90.0f){
					status = particleTestPtr.threadingState;
					particleTestPtr.simulate();
					t_prev = clock();
				}else{
					Sleep(t - t_prev);
				}
			}
			status = particleTestPtr.threadingState;
			Sleep(100);
		}
	}
}