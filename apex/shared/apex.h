#ifndef APEX_H
#define APEX_H

#include "../../library/vm.h"
#include "../../library/math.h"

#ifndef _KERNEL_MODE
#include <stdio.h>
#define DEBUG
#define LOG printf
#endif

typedef QWORD C_Entity;
typedef QWORD C_Player;
typedef QWORD C_Weapon;

namespace apex
{
	BOOL running(void);

	namespace entity
	{
		C_Entity get_entity(int index);
	}

	namespace teams
	{
		C_Player get_local_player(void);
	}

	namespace engine
	{
		float get_sensitivity(void);
		DWORD get_current_tick(void);
	}

	namespace input
	{
		BOOL  get_button_state(DWORD button);
		void  mouse_move(int x, int y);
	}

	//
	// use it with teams
	//
	namespace player
	{

		BOOL         is_valid(C_Player player_address);
		float        get_visible_time(C_Player player_address);
		int          get_team_id(C_Player player_address);
		vec3         get_muzzle(C_Player player_address);
		vec3         get_bone_position(C_Player player_address, int index);
		vec3         get_velocity(C_Player player_address);
		vec2         get_viewangles(C_Player player_address);
		void         enable_glow(C_Player);
		C_Weapon     get_weapon(C_Player player_address);
	}

	namespace weapon
	{
		float        get_bullet_speed(C_Weapon weapon_address);
		float        get_bullet_gravity(C_Weapon weapon_address);
		float        get_zoom_fov(C_Weapon weapon_address);
	}
}

#endif /* apex.h */

