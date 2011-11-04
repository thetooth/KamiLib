#pragma once
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
	};

	// Point
	LOType class Point: public LObject<LOValue> {};

	// Rectangle
	LOType class Rect: public LObject<LOValue> {
	public:
		LOValue width;
		LOValue height;
		Rect(): LObject(){
			setWidth(0);
			setHeight(0);
		};
		Rect(LOValue newx, LOValue newy, LOValue newwidth, LOValue newheight): LObject(newx, newy) {
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
}
#undef LOType