#include "cs2game.h"

namespace cs2
{
	namespace bhop
	{
		//
		//write memory for +jump/-jump
		//
		//QWORD jump_offset = 0x172F570;	//Jump  OLD dwForceJump = 0x1730530

		//DWORD jump = 65537;				//onground
		//DWORD dontjump = 256;				//in air

		BOOL onshot = 1;					//do something once untill being reset
	}
}
namespace client
{
	extern void key_down(BYTE virtual_key_code);
	extern void key_up(BYTE virtual_key_code);
}
//UM version (uses windows api calls)
void cs2::bhop::run(void)
{
	if (!bhop_enabled)
	{
		return;
	}

	if (cs2::input::is_button_down(66))
	{
		//check if player is on ground
		if (cs2::player::get_flags(local_player_gb) & 1)
		{
			if (onshot)
			{
				client::key_down(VK_NUMLOCK);			// vm::write(game_handle, (clientdll_base + jump_offset), &jump, sizeof(jump)); // write +jump
				onshot = 0;
			}
			else if (!onshot)
			{
				client::key_up(VK_NUMLOCK);				// vm::write(game_handle, (clientdll_base + jump_offset), &dontjump, sizeof(dontjump)); // write -jump
				onshot = 1;
			}
			Sleep(17);
		}
		else
		{
			if (!onshot)
			{
				client::key_up(VK_NUMLOCK);				// vm::write(game_handle, (clientdll_base + jump_offset), &dontjump, sizeof(dontjump)); // write -jump
				onshot = 1;
			}
		}
	}
}

void cs2::bhop::reset(void)
{
	onshot = 1;
}
