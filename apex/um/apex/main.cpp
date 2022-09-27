#include "../../shared/shared.h"
#include <time.h>

//
// https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
//
int Sleep(long ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	return nanosleep(&ts, &ts);
}

namespace config
{
	DWORD aimbot_button = 111;
	float aimbot_fov = 5.0f;
	float aimbot_smooth = 5.0f;
	BOOL  aimbot_visibility_check = 1;
	BOOL  visuals_enabled = 1;
}

namespace input
{
	void mouse_move(int x, int y)
	{
		apex::input::mouse_move(x, y);
	}
}

int main(void)
{
	apex::reset_globals();
	while (1)
	{
		Sleep(1);
		apex_legends::run();
	}
}

