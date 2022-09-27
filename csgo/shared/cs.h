#ifndef CS_H
#define CS_H

#include "../../library/vm.h"
#include "../../library/math.h"

#ifndef _KERNEL_MODE
#define DEBUG
#include <stdio.h>
#define DEBUG
#define LOG printf
#endif

typedef DWORD C_Player;
typedef DWORD C_TeamList;
typedef DWORD C_Team;
typedef DWORD C_PlayerList;

namespace cs
{
	enum class WEAPON_CLASS {
		Invalid = 0,
		Knife = 1,
		Grenade = 2,
		Pistol = 3,
		Sniper = 4,
		Rifle = 5,
	} ;

	BOOL  running(void);
	void  reset_globals(void);

	namespace teams
	{
		C_Player     get_local_player(void);
		C_TeamList   get_team_list(void);
		int          get_team_count(void);
		C_Team       get_team(C_TeamList team_list, int index);
		int          get_team_num(C_Team team);
		int          get_player_count(C_Team team);
		C_PlayerList get_player_list(C_Team team);
		C_Player     get_player(C_PlayerList player_list, int index);
		BOOL         contains_player(C_PlayerList player_list, int player_count, int index);
	}

	namespace engine
	{
		vec2  get_viewangles(void);
		float get_sensitivity(void);
		BOOL  is_gamemode_ffa(void);
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
		BOOL         is_visible(C_Player local_player, C_Player player);
		BOOL         is_defusing(C_Player player_address);
		BOOL         has_defuser(C_Player player_address);
		int          get_player_id(C_Player player_address);
		int          get_crosshair_id(C_Player player_address);
		BOOL         get_dormant(C_Player player_address);
		int          get_life_state(C_Player player_address);
		int          get_health(C_Player player_address);
		int          get_shots_fired(C_Player player_address);
		vec2         get_vec_punch(C_Player player_address);
		int          get_fov(C_Player player_address);
		DWORD        get_weapon_handle(C_Player player_address);
		WEAPON_CLASS get_weapon_class(C_Player player_address);
		vec3         get_eye_position(C_Player player_address);
		BOOL         get_bone_position(C_Player player_address, int bone_index, matrix3x4_t *matrix);
	}
}

#endif /* cs.h */

