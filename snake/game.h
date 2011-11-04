#pragma once
#include "snake.h"
#include "field.h"
#include "kami.h"

using namespace klib;

class Game
{
public:
  void tick();
  void draw(KLGL &) const;
  void keyEvent(Snake::Direction);
  Field field_;
  Snake snake_;
};
