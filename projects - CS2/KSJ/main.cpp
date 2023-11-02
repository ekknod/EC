#include "../../cs2/shared/shared.h"
#include <stdio.h>

namespace client
{
	void mouse_move(int x, int y)
	{
		cs::input::move(x, y);
	}

	void mouse1_down(void)
	{
	}

	void mouse1_up(void)
	{
	}

	void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
	}
}


int main(int argc, char **argv)
{
	while (1)
	{
		cs2::run();
	}
	return 0;
}

