/*
 * originally written in C (2019), ported to C++ (17/09/2022)
 * i think code is easier to understand when it's written with C++.
 * 
 * what EC-CSGO open-source version lacks of(?)
 * - security features
 *
 */


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

	//
	// just in case
	//
	void reset_mouse(void);
}

//
// implemented by application/driver
//
namespace input
{
	extern void mouse_move(int x, int y);
	extern void mouse1_down(void);
	extern void mouse1_up(void);
}


//
// implemented by application/driver
//
namespace config
{
	extern BOOL  rcs;
	extern DWORD aimbot_button;
	extern float aimbot_fov;
	extern float aimbot_smooth;
	extern BOOL  aimbot_visibility_check;
	extern DWORD triggerbot_button;
}

namespace csgo
{
	inline void run(void)
	{
		if (cs::running())
		{
			features::run();
		} else {
			features::reset_mouse();
		}
	}
}


#endif // SHARED_H

