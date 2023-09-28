#include "../../cs2/shared/shared.h"

namespace input
{
	void mouse_move(int x, int y)
	{
		mouse_event(MOUSEEVENTF_MOVE, (DWORD)x, (DWORD)y, 0, 0);
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
	BOOL  rcs = 0;
	DWORD aimbot_button = 107;
	float aimbot_fov = 2.0f;
	float aimbot_smooth = 20.0f;
	BOOL  aimbot_visibility_check = 0;
	DWORD triggerbot_button = 111;
	BOOL  visuals_enabled = 1;
}

int main(void)
{
	while (1)
	{
		Sleep(1);
		cs2::run();
	}
	return 0;
}

