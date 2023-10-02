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
namespace input
{
	extern void mouse_move(int x, int y);
	extern void mouse1_down(void);
	extern void mouse1_up(void);
	extern BOOL is_button_down(DWORD button);

	typedef struct {
		float x;
		float y;
		float w;
		float h;
	} WINDOW_INFO ;

	WINDOW_INFO get_window_info(void);
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
	extern BOOL  visuals_enabled;
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

