#include "shared.h"

//
// private data. only available for features.cpp
//

namespace features
{
	//
	// triggerbot
	//
	static DWORD mouse_down_tick;
	static DWORD mouse_up_tick;

	//
	// rcs
	//
	static vec3  rcs_old_punch;

	//
	// aimbot
	//
	static BOOL  aimbot_active;


	void reset(void)
	{
		mouse_down_tick  = 0;
		mouse_up_tick    = 0;
		rcs_old_punch    = {};
		aimbot_active    = 0;
	}

	static void standalone_rcs(DWORD shots_fired, vec3 vec_punch, float sensitivity);
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

	DWORD num_shots   = cs::player::get_shots_fired(local_player);
	vec3  aim_punch   = cs::player::get_vec_punch(local_player);
	float sensitivity = cs::mouse::get_sensitivity() * cs::player::get_fov_multipler(local_player);

	if (config::rcs)
	{
		standalone_rcs(num_shots, aim_punch, sensitivity);
	}

	if (input::is_button_down(config::triggerbot_button))
	{
		//
		// accurate shots only
		//
		if (aim_punch.x > -0.04f)
		{
			triggerbot(local_player);
		}
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

static void features::standalone_rcs(DWORD num_shots, vec3 vec_punch, float sensitivity)
{
	if (num_shots > 1)
	{
		float x = (vec_punch.x - rcs_old_punch.x) * -1.0f;
		float y = (vec_punch.y - rcs_old_punch.y) * -1.0f;
		
		int mouse_angle_x = (int)(((y * 2.0f) / sensitivity) / -0.022f);
		int mouse_angle_y = (int)(((x * 2.0f) / sensitivity) / 0.022f);

		if (!aimbot_active)
		{
			input::mouse_move(mouse_angle_x, mouse_angle_y);
		}
	}
	rcs_old_punch = vec_punch;
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

