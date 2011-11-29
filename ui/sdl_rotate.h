/*

SDL_rotozoom - rotozoomer

LGPL (c) A. Schiffler

*/

#ifndef _SDL_rotozoom_h
#define _SDL_rotozoom_h

#include <SDL.h>
#include <math.h>

SDL_Surface* rotateSurface90Degrees(SDL_Surface* src, int numClockwiseTurns);

#endif	/* _SDL_rotozoom_h */
