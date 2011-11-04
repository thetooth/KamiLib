#include "game.h"
#include "kami.h"
#include "field.h"

using namespace klib;

void Game::tick()
{
	field_.score++;
	if (!snake_.tick(field_))
	{
		snake_ = Snake();
		field_ = Field();
	}
}

void Game::draw(KLGL &p) const
{
	field_.draw(p);
}

void Game::keyEvent(Snake::Direction d)
{
	snake_.keyEvent(d);
}
