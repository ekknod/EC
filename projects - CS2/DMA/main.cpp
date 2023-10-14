#include "../../cs2/shared/shared.h"

namespace input
{
	void mouse_move(int x, int y)
	{
		cs::input::move(x, y);
		// kmbox_mouse_move();
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
	BOOL  rcs = 0;
	DWORD aimbot_button = 314; // mouse left key
	float aimbot_fov = 2.0f;
	float aimbot_smooth = 30.0f;
	BOOL  aimbot_visibility_check = 0;
	DWORD triggerbot_button = 318; // mouse forward key
	BOOL  visuals_enabled = 1;
}

int main(void)
{
	while (1)
	{
		cs2::run();
	}
	return 0;
}

