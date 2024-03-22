#ifndef CSGO_H
#define CSGO_H

#include "../../library/vm/vm.h"
#include "../../library/math.h"


//
// enables global debug messages
//
#ifndef LOGGER
#define LOGGER


#ifndef _KERNEL_MODE
#define DEBUG
#include <stdio.h>
#ifdef DEBUG
#define LOG(...) printf("[EC] " __VA_ARGS__)
#else
#define LOG(...) // __VA_ARGS__
#endif

#else

// #define DEBUG
#ifdef DEBUG
#define LOG(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[EC] " __VA_ARGS__)
#else
#define LOG(...) // __VA_ARGS__
#endif

#endif


#endif


#define FONT_LARGE 6
#define FONT_SMALL 5

typedef DWORD C_Player;
typedef DWORD C_TeamList;
typedef DWORD C_Team;
typedef DWORD C_PlayerList;

namespace csgo
{
	extern vm_handle game_handle ;

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

	namespace entity
	{
		DWORD get_client_entity(int index);
	}

	namespace engine
	{
		DWORD get_viewangles_address(void);
		vec2  get_viewangles(void);
		float get_sensitivity(void);
		int   get_crosshairalpha(void);

		BOOL  is_gamemode_ffa(void);
		DWORD get_current_tick(void);
		void  net_graphcolor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
		void  net_graphfont(DWORD font);

		vec2_i  get_screen_size(void);
		vec2_i  get_screen_pos(void);
		view_matrix_t get_viewmatrix(void);
	}

	namespace input
	{
		DWORD get_hwnd(void);
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
		int          get_team_num(C_Player player_address);
		int          get_crosshair_id(C_Player player_address);
		BOOL         get_dormant(C_Player player_address);
		int          get_life_state(C_Player player_address);
		int          get_health(C_Player player_address);
		int          get_shots_fired(C_Player player_address);
		vec3         get_origin(C_Player player_address);
		vec2         get_vec_punch(C_Player player_address);
		vec2         get_viewangles(C_Player player_address);
		int          get_fov(C_Player player_address);
		DWORD        get_weapon_handle(C_Player player_address);
		WEAPON_CLASS get_weapon_class(C_Player player_address);
		vec3         get_eye_position(C_Player player_address);
		BOOL         get_bone_position(C_Player player_address, int bone_index, matrix3x4_t *matrix);
	}
}

#endif /* csgo.h */

