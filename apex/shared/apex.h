#ifndef APEX_H
#define APEX_H

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

namespace apex
{
	extern vm_handle game_handle ;
	BOOL running(void);

	typedef struct
	{
		float x, y, w, h;
	} WINDOW_INFO ;

	namespace engine
	{
		QWORD         get_convar(PCSTR name);
		int           get_current_tick(void);
		view_matrix_t get_viewmatrix(void);
		QWORD         get_hwnd(void);
		BOOL          get_window_info(WINDOW_INFO *info);
	}

	namespace entity
	{
		QWORD get_local_player(void);
		QWORD get_client_entity(int index);
	}

	namespace mouse
	{
		float get_sensitivity(void);
	}

	int get_controller_sensitivity();

	namespace input
	{
		BOOL  is_button_down(DWORD button);
		void  mouse_move(int x, int y);
		void  enable_input(BYTE state);
	}

	namespace player
	{
		BOOL     is_valid(QWORD player_address);
		BOOL     is_zoomed_in(QWORD player_address);
		float    get_visible_time(QWORD player_address);
		int      get_team_num(QWORD player_address);
		int      get_health(QWORD player_address);
		int      get_max_health(QWORD player_address);
		vec3     get_origin(QWORD player_address);
		vec3     get_camera_origin(QWORD player_address);
		BOOL     get_bone_position(QWORD player_address, int index, vec3 *vec_out);
		vec3     get_velocity(QWORD player_address);
		vec2     get_viewangles(QWORD player_address);
		QWORD    get_weapon(QWORD player_address);
	}

	namespace weapon
	{
		float    get_bullet_speed(QWORD weapon_address);
		float    get_bullet_gravity(QWORD weapon_address);
		float    get_zoom_fov(QWORD weapon_address);
	}

	namespace cvar
	{
		float get_float(QWORD cvar);
		int   get_int(QWORD cvar);
	}
}

#endif /* apex.h */

