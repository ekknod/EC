#include "../../cs2/shared/shared.h"

namespace client
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

