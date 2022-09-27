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

#include "apex.h"

//
// features.cpp
//
namespace features
{
	void run(void);
}


//
// implemented by application/driver
//

namespace input
{
	extern void mouse_move(int x, int y);
}

//
// implemented by application/driver
//
namespace config
{
	extern DWORD aimbot_button;
	extern float aimbot_fov;
	extern float aimbot_smooth;
	extern BOOL  aimbot_visibility_check;
	extern BOOL  visuals_enabled;
}

namespace apex_legends
{
	inline void run(void)
	{
		if (apex::running())
		{
			features::run();
		}
	}
}


#endif // SHARED_H

