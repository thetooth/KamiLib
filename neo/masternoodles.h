#pragma once

#include "kami.h"
#include "version.h"
#include "tween.h"
#include "../UI/UI.h"
#include <fmod.hpp>
#include <fmod_errors.h>

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define chop(x)  ((x)>=0?(long)((x)+0.9):(long)((x)-0.9))
#define frand() (float)rand()/(float)RAND_MAX

#define NEO_VERSION "v0.2.0"
#define NEO_NAMESPACE "neo"
const int APP_SCREEN_W = 640;
const int APP_SCREEN_H = 360;

inline int floorMpl(float x,int m){
	x = floor(x);
	if((int)x%m != 0){
		return (int)x;
	}else{
		return (int)x-((int)x%m);
	}
}
