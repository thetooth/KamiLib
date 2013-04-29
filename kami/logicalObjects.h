#pragma once

#include <math.h>

#define LOType template <class LOValue>
namespace klib{

	// Logical Object template
	LOType class LObject {
	public:
		LOValue x;
		LOValue y;
		LObject(){ moveTo(0, 0); };
		LObject(LOValue newx, LOValue newy){ moveTo(newx, newy); };
		inline LOValue getX(){ return x; };
		inline LOValue getY(){ return y; };
		inline void setX(LOValue newx){ x = newx; };
		inline void setY(LOValue newy){ y = newy; };
		inline void moveTo(LOValue newx, LOValue newy){ setX(newx); setY(newy); };
		inline void rMoveTo(LOValue deltax, LOValue deltay){ moveTo(getX() + deltax, getY() + deltay); };
		inline void moveAngle(LOValue angle, LOValue speed = 1, LOValue time = 1){
			float rad = angle * (APP_PI / 180);
			x += cos(rad) * (time * speed);
			y -= sin(rad) * (time * speed);
		}
	};

	// Point
	LOType class Point: public LObject<LOValue> {
	public:
		Point(): LObject<LOValue>::LObject(){};
		Point(LOValue newx, LOValue newy): LObject<LOValue>::LObject(newx, newy){};
	};

	// Rectangle
	LOType class Rect: public LObject<LOValue> {
	public:
		LOValue width;
		LOValue height;
		Rect(): LObject<LOValue>::LObject(){
			setWidth(0);
			setHeight(0);
		};
		Rect(LOValue newx, LOValue newy, LOValue newwidth, LOValue newheight): LObject<LOValue>::LObject(newx, newy){
			setWidth(newwidth);
			setHeight(newheight);
		};
		inline LOValue getWidth(){
			return width;
		};
		inline LOValue getHeight(){
			return height;
		};
		inline void setWidth(LOValue newwidth){
			width = newwidth;
		};
		inline void setHeight(LOValue newheight){
			height = newheight;
		};
	};

	// Vec4
	LOType class Vec4: public LObject<LOValue> {
	public:
		LOValue x;
		LOValue y;
		LOValue z;
		LOValue w;
		
		Vec4(): LObject<LOValue>::LObject(){
			x = 0;
			y = 0;
			z = 0;
			w = 0;
		}
		Vec4(LOValue newx, LOValue newy, LOValue newz, LOValue neww): LObject<LOValue>::LObject(newx, newy){
			x = newx;
			y = newy;
			z = newz;
			w = neww;
		}
		inline LOValue* Data(){
			rawTypePtr[0] = x;
			rawTypePtr[1] = y;
			rawTypePtr[2] = z;
			rawTypePtr[3] = w;
			return rawTypePtr;
		}
	private:
		LOValue rawTypePtr[4];
	};
}
#undef LOType
