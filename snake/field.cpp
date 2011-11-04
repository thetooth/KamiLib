#include "field.h"
#include "kami.h"

#include <math.h>
#include <cstdlib>

using namespace klib;

Field::Field()
{
	for (int y = 0; y < HEIGHT; ++y)
		for (int x = 0; x < WIDTH; ++x)
			m_[y][x] = EMPTY;
	newFruit();
	score = 0;
}

void Field::setBlock(Type type, int x, int y)
{
	m_[y][x] = type;
}

Field::Type Field::block(int x, int y) const
{
	return m_[y][x];
}
void Field::draw(KLGL &p) const
{
	for (int y = 0; y < HEIGHT; ++y){
		for (int x = 0; x < WIDTH; ++x)
		{
			switch (m_[y][x])
			{
			case EMPTY:
				break;
			case SNAKE_BLOCK:
				p.Rectangle2D(x * BLOCK_WIDTH, y * BLOCK_HEIGHT, BLOCK_WIDTH - 1, BLOCK_HEIGHT - 1, KLGLColor(48,72,72));
				break;
			case FRUIT:
				p.Rectangle2D(x * BLOCK_WIDTH, y * BLOCK_HEIGHT, BLOCK_WIDTH - 1, BLOCK_HEIGHT - 1, KLGLColor(72,120,96));
				break;
			}
		}
	}
}

void Field::newFruit()
{
	score += 250;
	int x;
	int y;
	do
	{
		x = rand() % WIDTH;
		y = rand() % HEIGHT;
	} while (block(x, y) != EMPTY);
	setBlock(FRUIT, x, y);
}
