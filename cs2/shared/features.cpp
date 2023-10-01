#include "shared.h"

//
// private data. only available for features.cpp
//

namespace features
{
	static DWORD mouse_down_tick;
	static DWORD mouse_up_tick;

	void reset(void)
	{
		mouse_down_tick  = 0;
		mouse_up_tick    = 0;
	}

	static void triggerbot(QWORD local_player);
}

inline DWORD random_number(DWORD min, DWORD max)
{
	return min + cs::engine::get_current_tick() % (max + 1 - min);
}

void features::run(void)
{
	//
	// reset triggerbot input
	//
	if (mouse_down_tick)
	{
		DWORD current_tick = cs::engine::get_current_tick();
		if (current_tick > mouse_down_tick)
		{
			input::mouse1_up();
			mouse_down_tick = 0;
			mouse_up_tick   = random_number(30, 50) + current_tick;
		}
	}

	QWORD local_player_controller = cs::entity::get_local_player_controller();
	QWORD local_player = cs::entity::get_player(local_player_controller);

	if (local_player == 0)
		return;
	
	if (input::is_button_down(config::triggerbot_button))
	{
		triggerbot(local_player);
	}

	/*
	for (int i = 1; i < 32; i++)
	{
		QWORD ent = cs::entity::get_client_entity(i);
		if (ent == 0 || (ent == local_player_controller))
		{
			continue;
		}

		QWORD player = cs::entity::get_player(ent);
		if (player == 0)
		{
			continue;
		}


	}
	*/
}

static void features::triggerbot(QWORD local_player)
{
	if (mouse_down_tick)
	{
		return;
	}

	DWORD crosshair_id = cs::player::get_crosshair_id(local_player);

	if (crosshair_id == (DWORD)-1)
		return;

	QWORD crosshair_target = cs::entity::get_client_entity(crosshair_id);
	if (crosshair_target == 0)
		return;

	if (cs::player::get_health(crosshair_target) < 1)
		return;

	if (cs::player::get_team_num(local_player) == cs::player::get_team_num(crosshair_target))
		return;

	DWORD current_tick = cs::engine::get_current_tick();
	if (current_tick > mouse_up_tick)
	{
		input::mouse1_down();
		mouse_up_tick   = 0;
		mouse_down_tick = random_number(30, 50) + current_tick;
	}
}

