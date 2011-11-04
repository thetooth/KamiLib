#include "painter.h"
#include "kami.h"

void Painter::bar(int x1, int y1, int x2, int y2)
{
	glColor3ub(48,72,72);
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
}

void Painter::extra(int x1, int y1, int x2, int y2)
{
	glColor3f(72,120,96);
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
}
