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

int main(void)
{
	while (1)
	{
		cs2::run();
	}
	return 0;
}

