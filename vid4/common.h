#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>

struct Point
{
	Point(int x = 0, int y = 0) : x(x), y(y) { }
	
	int x;
	int y;
};


unsigned int __NOW; // TODO: something better than a global hehe
class Time
{
public:
	static inline unsigned int GetNow() { return __NOW; }
	
	static void UpdateNow()
	{
		__NOW = SDL_GetTicks();
	}
};

#endif
