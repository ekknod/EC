#include "cs2game.h"

namespace cs2
{
	namespace bhop
	{
		BOOL onshot;

		DWORD currentms;
		DWORD delay;
	}
}
namespace client
{
	extern void key_down(void);
	extern void key_up(void);
}



void cs2::bhop::run(void)
{
	if (!bhop_enabled)
	{
		return;
	}

	if (cs2::input::is_button_down(66))
	{

		//get current time
		currentms = cs2::engine::get_current_ms();

		//check if player is on ground
		if (cs2::player::get_flags(local_player_gb) & 1)
		{
			if (onshot == 0 && (delay < currentms))
			{
				client::key_down();
				onshot = 1;
				delay = currentms + 17;
			}
			if (onshot == 1 && (delay < currentms))
			{
				client::key_up();
				delay = currentms + 17;
				onshot = 0;	
			}
		}
		//reset while in air
		else
		{
			if (onshot == 1 && (delay < currentms))
			{
				client::key_up();
				delay = 0;
				onshot = 0;
			}
			else if (onshot == 0)
			{
				delay = 0;
			}
		}
	}
	else if (onshot)
	{
		reset();
	}
}

void cs2::bhop::reset(void)
{
	if (onshot == 1)
	{
		client::key_up();
		delay = 0;
		onshot = 0;
	}
	else if (onshot == 0)
	{
		delay = 0;
	}
}