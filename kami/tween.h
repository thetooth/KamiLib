/**************************************************************************/

/*
	tween.cpp - CppTweener

	Copyright	(C) 2005-2012 Ameoto Systems Inc.
				(C) 2009-2011 Wesley Marques.
				(C) 2001-2001 Robert Penner.
	All rights reserved.

	Written by Wesley Ferreira Marques - wesley[]codevein.com

	This port was based in a initial code from Jesus Gollonet, port of 
	Penners easing equations to C/C++:
	- http://www.jesusgollonet.com/blog/2007/09/24/penner-easing-cpp/
	- http://robertpenner.com/easing/

	Redistribution and use in source and binary forms, with or without 
	modification, are permitted provided that the following conditions are 
	met:

	Redistributions of source code must retain the above copyright notice, 
	this list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright 
	notice, this list of conditions and the following disclaimer in the 
	documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
	OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
	TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
	USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
	DAMAGE.
*/

/**************************************************************************/

#ifndef _CPP_TWEEEN_
#define _CPP_TWEEEN_

#include<math.h>
#include<list>
#include<vector>
#include<iostream>

#include "kami.h"

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

namespace tween {

	class Easing {
	public :
		Easing() {}
		//pure virtual
		virtual float easeIn(float t,float b , float c, float d)=0;
		virtual float easeOut(float t,float b , float c, float d)=0;
		virtual float easeInOut(float t,float b , float c, float d)=0;
	};

	class Back : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Bounce : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Circ : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Cubic : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Elastic : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Expo : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Quad : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};
	
	class Quart : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Quint : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};

	class Sine : public Easing {
	public :
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};
	
	class Linear : public Easing {
	public :
		float easeNone(float t,float b , float c, float d);
		float easeIn(float t,float b , float c, float d);
		float easeOut(float t,float b , float c, float d);
		float easeInOut(float t,float b , float c, float d);
	};
	
	
	enum {
		LINEAR,
		SINE,
		QUINT,
		QUART,
		QUAD,
		EXPO,
		ELASTIC,
		CUBIC,
		CIRC,
		BOUNCE,
		BACK
	};
	
	enum {
		EASE_IN,
		EASE_OUT,
		EASE_IN_OUT
	};
	
	static Linear fLinear;
	static Sine fSine;
	static Quint fQuint;
	static Quart fQuart;
	static Quad  fQuad;
	static Expo fExpo;
	static Elastic fElastic;
	static Cubic fCubic;
	static Circ fCirc;
	static Bounce fBounce;
	static Back fBack;
	
	//callback temp
	typedef void (*TweenerCallBack)();	
	
	struct TweenerProperty {
		float *ptrValue;
		float finalValue;
		float initialValue;
	};
	class TweenerParam {
	public :
		std::vector<TweenerProperty>  properties;
		float time;
		short transition;
		short equation;
		float delay;
		float timeCount;
		int total_properties;
		bool useMilliSeconds;
		bool started;
		bool delayFinished;
		bool reverse;
		int repeat;
		int reverseCount;
		TweenerCallBack onCompleteCallBack;
		TweenerCallBack onStepCallBack;
		
		TweenerParam() {
			useMilliSeconds = true;
			timeCount = 0;
			started = false;
			delayFinished = true;
			delay = 0;
			total_properties = 0;
			onCompleteCallBack = 0;
			onStepCallBack = 0;
			repeat = -1;
			reverse = false;
			reverseCount = -1;
		}
		~TweenerParam(){
			properties.clear();
		}
		TweenerParam(float ptime,short ptransition = EXPO, short pequation = EASE_OUT, float pdelay = 0) {
			time = ptime;
			transition = ptransition;
			equation = pequation;
			useMilliSeconds = true;
			timeCount = 0;
			started = false;
			delayFinished = (pdelay > 0)?false:true;
			delay = pdelay;
			total_properties = 0;
			onCompleteCallBack = 0;
			onStepCallBack = 0;
			repeat = -1;
			reverse = false;
			reverseCount = -1;
		}
		
		void setRepeatWithReverse(int _n,bool _reverse=false) {
			repeat = _n-1;
			if (_reverse) {
				reverse = _reverse;
				reverseCount = repeat + 1;
			}
		}
		
		int decreaseRepeat() {
			float bkpValue = 0;
			if (repeat > -1 || reverseCount > -1) {
				timeCount = 0;
				started = false;
				if (total_properties > 0) {
					for (int i =0 ; i < total_properties; i++ ) {
						TweenerProperty prop = properties[i];
						if (reverseCount <= repeat && reverse) {
							//destroca
							bkpValue = properties[i].finalValue;
							properties[i].finalValue = properties[i].initialValue;
							properties[i].initialValue = bkpValue;
						} else if (reverse) {
							//troca
							bkpValue = properties[i].initialValue;
							properties[i].initialValue = properties[i].finalValue;
							properties[i].finalValue = bkpValue;
						}
						*(prop.ptrValue) = properties[i].initialValue;
					}
				}
				
				if (reverseCount <= repeat) {
					repeat--;		
				} else {
					reverseCount--;
				}	
				

			}	
			
			return repeat;
		}	
		void addProperty(float *valor, float valorFinal) {
			TweenerProperty prop = {valor, valorFinal, *valor};
			properties.push_back(prop);
			total_properties = properties.size();
		}
		
		void setUseMilliSeconds(bool use ){
			useMilliSeconds = use;
		}
		
		void cleanProperties(){
			properties.clear();
			//std::cout<<"\n-Parameter removed";
		}
		
		bool operator==( const TweenerParam& p ) {
			bool equal = false;
			if ((time == p.time) && (transition == p.transition) && (equation == p.equation) ){
				equal = true;
			}
			if (equal) {
				for (unsigned int i =0 ; i < p.total_properties; i++ ) {
					if( properties[i].initialValue != p.properties[i].initialValue ||
					   properties[i].finalValue != p.properties[i].finalValue) {
						equal = false;
						break;
					}
				}
			}
			return equal;
		}
	};
	
	//listener geral
	class TweenerListener {
	public :
		TweenerListener(){};
		~TweenerListener(){};
		virtual void onStart(TweenerParam& param) = 0;
		virtual void onStep(TweenerParam& param) = 0 ;
		virtual void onComplete(TweenerParam& param) = 0 ;
		bool operator==( const TweenerListener& l ) {
			return (this == &l) ;
		};
	};
	
	
	class Tweener {
	protected :
	public :
		enum {ON_START, ON_STEP, ON_COMPLETE};
		short currentFunction ;
		Easing *funcs[11];
		long lastTime;
		
		std::vector<TweenerParam*>::iterator tweensIT;
		std::vector<TweenerListener*> listeners;
		std::vector<TweenerListener*>::iterator listenerIT;
		int total_tweens ;
		
		float runEquation(int transition,int equation, float t,float b , float c, float d);
		void dispatchEvent(TweenerParam *param, short eventType);
		
		std::vector<TweenerParam*> tweens;
		Tweener() {
			this->funcs[LINEAR] = &fLinear;
			this->funcs[SINE]  = &fSine;
			this->funcs[QUINT] = &fQuint;
			this->funcs[QUART] = &fQuart;
			this->funcs[QUAD] = &fQuad;
			this->funcs[EXPO] = &fExpo;
			this->funcs[ELASTIC] = &fElastic;
			this->funcs[CUBIC] = &fCubic;
			this->funcs[CIRC] =  &fCirc;
			this->funcs[BOUNCE] =  &fBounce;
			this->funcs[BACK] =  &fBack;
			lastTime = 0;
		}
		void addTween(TweenerParam *param);
		void removeTween(TweenerParam  *param);
		void addListener(TweenerListener *listener);
		void removeListener(TweenerListener *listener);
		void setFunction(short funcEnum);
		void step(long currentMillis);
	};
}
#endif
