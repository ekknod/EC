#include "../../cs2/shared/shared.h"

namespace input
{
	void mouse_move(int x, int y)
	{
		// kmbox_mouse_move();


		cs::input::move(x, y);
	}

	void mouse1_down(void)
	{
		// kmbox_mouse_down();
	}

	void mouse1_up(void)
	{
		// kmbox_mouse_up();
	}

	void DrawRect(void *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b)
	{
	
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(w);
		UNREFERENCED_PARAMETER(h);
		UNREFERENCED_PARAMETER(r);
		UNREFERENCED_PARAMETER(g);
		UNREFERENCED_PARAMETER(b);
	}

	void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(w);
		UNREFERENCED_PARAMETER(h);
		UNREFERENCED_PARAMETER(r);
		UNREFERENCED_PARAMETER(g);
		UNREFERENCED_PARAMETER(b);
	}
}

int main(void)
{
	while (1)
	{
		cs2::run();
	}
	return 0;
}

