#pragma once

#include "kami.h"
#include "version.h"
#include <omp.h>

#define PI 3.14159265
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define chop(x)  ((x)>=0?(long)((x)+0.9):(long)((x)-0.9))
#define frand() (float)rand()/(float)RAND_MAX

#define APP_VERSION "v0.1.0"
const int APP_SCREEN_W = 640;
const int APP_SCREEN_H = 360;

_inline int floorMpl(float x,int m){
	x = floor(x);
	if((int)x%m != 0){
		return (int)x;
	}else{
		return (int)x-((int)x%m);
	}
}
