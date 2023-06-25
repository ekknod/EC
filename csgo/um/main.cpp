#include "../shared/shared.h"

namespace input
{
	void mouse_move(int x, int y)
	{
		// mouse_event(MOUSEEVENTF_MOVE, (DWORD)x, (DWORD)y, 0, 0);

		cs::input::mouse_move(x, y);
	}

	void mouse1_down(void)
	{
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	}

	void mouse1_up(void)
	{
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	}
}

namespace config
{
	BOOL  rcs = 1;
	DWORD aimbot_button = 110;
	float aimbot_fov = 2.0f;
	float aimbot_smooth = 10.0f;
	BOOL  aimbot_visibility_check = 0;
	DWORD triggerbot_button = 111;
	BOOL  visuals_enabled = 1;

	DWORD time_begin      = 0;
	DWORD time_end        = 0;
	DWORD licence_type    = 0;
	DWORD invalid_hwid    = 0;
}

int main(void)
{
	cs::reset_globals();
	while (1)
	{
		Sleep(1);
		csgo::run();
	}
	return 0;
}

