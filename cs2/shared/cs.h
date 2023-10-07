#ifndef CS_H
#define CS_H

#include "../../library/vm.h"
#include "../../library/math.h"

#ifndef _KERNEL_MODE
#define DEBUG
#include <stdio.h>
#define DEBUG
#define LOG(...) printf("[EC] " __VA_ARGS__)
#else
#define DEBUG
#define LOG(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[EC] " __VA_ARGS__)
#endif

namespace cs
{
	typedef struct
	{
		float x, y, w, h;
	} WINDOW_INFO ;

	namespace sdl
	{
		QWORD get_window(void);
		BOOL  get_window_info(QWORD window, WINDOW_INFO *info);
		QWORD get_window_data(QWORD window);
		QWORD get_hwnd(QWORD window_data);
		QWORD get_hdc(QWORD window_data);


		QWORD get_mouse(void);
		BOOL  get_mouse_button(QWORD sdl_mouse);
	}

	namespace engine
	{
		QWORD get_convar(const char *name);
		DWORD get_current_tick(void);
		vec2  get_viewangles(void);
		view_matrix_t get_viewmatrix(void);
	}

	namespace entity
	{
		QWORD get_local_player_controller(void);
		QWORD get_client_entity(int index);
		BOOL  is_player(QWORD controller);
		QWORD get_player(QWORD controller);
	}

	int get_crosshairalpha(void);

	namespace mouse
	{
		float get_sensitivity(void);
	}

	namespace input
	{
		BOOL  is_button_down(DWORD button);
	}

	namespace gamemode
	{
		BOOL is_ffa(void);
	}

	namespace player
	{
		DWORD get_health(QWORD player);
		DWORD get_team_num(QWORD player);
		DWORD visible_check(QWORD player);
		int   get_life_state(QWORD player);
		vec3  get_origin(QWORD player);
		float get_vec_view(QWORD player);
		vec3  get_eye_position(QWORD player);
		DWORD get_crosshair_id(QWORD player);
		DWORD get_shots_fired(QWORD player);
		vec2  get_eye_angles(QWORD player);
		float get_fov_multipler(QWORD player);
		vec2  get_vec_punch(QWORD player);
		QWORD get_node(QWORD player);
		BOOL  is_valid(QWORD player, QWORD node);
	}

	namespace node
	{
		BOOLEAN  get_dormant(QWORD node);
		vec3     get_origin(QWORD node);
		BOOL     get_bone_position(QWORD node, int index, vec3 *data);
	}

	BOOL running(void);
}

#endif /* cs.h */

