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
#define FONT_LARGE 6
#define FONT_SMALL 5

typedef DWORD C_Player;
typedef DWORD C_TeamList;
typedef DWORD C_Team;
typedef DWORD C_PlayerList;

namespace cs
{
	BOOL running(void);
}

#endif /* cs.h */

