#include "../shared/shared.h"

namespace input
{
	void mouse_move(int x, int y)
	{
		// kmbox_mouse_move();

		cs::input::mouse_move(x, y);
	}

	void mouse1_down(void)
	{
		// kmbox_mouse_down();
	}

	void mouse1_up(void)
	{
		// kmbox_mouse_up();
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

