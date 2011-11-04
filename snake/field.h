#pragma once
#include "kami.h"

using namespace klib;

class Field
{
public:
	enum { WIDTH = 80, HEIGHT = 60 };
	enum { BLOCK_WIDTH = 4, BLOCK_HEIGHT = 4 };
	enum Type { EMPTY, SNAKE_BLOCK, FRUIT };
	Field();
	void setBlock(Type type, int x, int y);
	Type block(int x, int y) const;
	void draw(KLGL &) const;
	void newFruit();
	int score;
private:
	Type m_[HEIGHT][WIDTH];
};
