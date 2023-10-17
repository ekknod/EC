#ifndef SHARED_H
#define SHARED_H

#ifdef _KERNEL_MODE
extern "C" int _fltused;
#endif

#include "cs.h"

//
// features.cpp
//
namespace features
{
	void run(void);
	void reset(void);
}

//
// implemented by application/driver
//
namespace client
{
	extern void mouse_move(int x, int y);
	extern void mouse1_down(void);
	extern void mouse1_up(void);
	extern void DrawRect(void *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b);
	extern void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b);
}

namespace cs2
{
	inline void run(void)
	{
		if (cs::running())
		{
			features::run();
		}
		else
		{
			features::reset();
		}
	}
}


#endif // SHARED_H

