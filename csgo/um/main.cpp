#include "../shared/shared.h"

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
	BOOL  rcs;
	DWORD aimbot_button;
	float aimbot_fov;
	float aimbot_smooth;
	BOOL  aimbot_visibility_check;
	DWORD triggerbot_button;
}

int main(void)
{
	config::rcs = TRUE;
	config::aimbot_button = 110;
	config::aimbot_fov = 2.0f;
	config::aimbot_smooth = 100.0f;
	config::aimbot_visibility_check = 0;
	config::triggerbot_button = 111;

	while (1)
	{
		Sleep(1);
		csgo::run();
	}
	return 0;
}

