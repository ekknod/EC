#ifndef CS_H
#define CS_H

#include "../../library/vm.h"
#include "../../library/math.h"

#ifndef _KERNEL_MODE
#define DEBUG
#include <stdio.h>
#define DEBUG
#define LOG(...) printf("[EC] " __VA_ARGS__)
#else

// #define DEBUG
#define LOG(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)

#endif

namespace cs
{
	namespace engine
	{
		QWORD get_convar(const char *name);
		DWORD get_current_tick(void);
	}

	namespace entity
	{
		QWORD get_local_player_controller(void);
		QWORD get_client_entity(int index);
		QWORD get_player(QWORD controller);
	}

	namespace mouse
	{
		float get_sensitivity(void);
	}

	namespace player
	{
		DWORD get_health(QWORD player);
		DWORD get_team_num(QWORD player);
		DWORD get_crosshair_id(QWORD player);
	}

	BOOL running(void);
}

#endif /* cs.h */

