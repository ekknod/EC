/*
 * originally written in C (2019), ported to C++ (17/09/2022)
 * i think code is easier to understand when it's written with C++.
 * 
 * what EC-CSGO open-source version lacks of(?)
 * - security features
 *
 */


#ifndef CSGOGAME_H
#define CSGOGAME_H

#ifdef _KERNEL_MODE
extern "C" int _fltused;
#endif


#include "csgo.h"



//
// features.cpp
//
namespace csgo
{
	namespace features
	{
		void run(void);
		void reset(void);
	}
}


//
// implemented by application/driver
//
namespace client
{
	extern void mouse_move(int x, int y);
	extern void mouse1_down(void);
	extern void mouse1_up(void);
	extern void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b);
	extern void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b);
}

namespace csgo
{
	inline void run(void)
	{
		if (csgo::running())
		{
			features::run();
		} else {
			features::reset();
		}
	}
}


#endif // CSGOGAME_H

