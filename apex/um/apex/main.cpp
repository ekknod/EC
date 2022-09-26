#include "../../shared/shared.h"
#include <time.h>

//
// https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
//
int Sleep(long ms)
{
	struct timespec ts;
	int res;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	return nanosleep(&ts, &ts);
}

namespace config
{
	DWORD aimbot_button = 111;
	float aimbot_fov = 2.0f;
	float aimbot_smooth = 5.0f;
	BOOL  aimbot_visibility_check = 1;
	BOOL  visuals_enabled = 1;
}

int main(void)
{
	while (1)
	{
		Sleep(1);
		apex_legends::run();
	}
}

