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
	static vec2  rcs_old_punch;

	//
	// aimbot
	//
	static BOOL  aimbot_active;
	static QWORD aimbot_target;
	static DWORD aimbot_tick;


	void reset(void)
	{
		mouse_down_tick  = 0;
		mouse_up_tick    = 0;
		rcs_old_punch    = {};
		aimbot_active    = 0;
		aimbot_target    = 0;
		aimbot_tick      = 0;
	}

	static vec3 get_target_angle(QWORD local_player, vec3 position, DWORD num_shots, vec2 aim_punch);
	static void get_best_target(BOOL ffa, QWORD local_controller, QWORD local_player, DWORD num_shots, vec2 aim_punch, QWORD *target);
	static void standalone_rcs(DWORD shots_fired, vec2 vec_punch, float sensitivity);
	static void triggerbot(BOOL ffa, QWORD local_player);

#ifdef _KERNEL_MODE
	static void esp(QWORD local_player, QWORD target_player, vec3 head);
#endif _KERNEL_MODE
}

inline DWORD random_number(DWORD min, DWORD max)
{
	return min + cs::engine::get_current_tick() % (max + 1 - min);
}

inline void update_settings(void)
{
	int crosshair_alpha = cs::get_crosshairalpha();

	switch (crosshair_alpha)
	{
	//
	// mouse5 aimkey, mouse4 triggerkey
	//
	case 244:
		config::aimbot_button     = 318;
		config::triggerbot_button = 317;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 30.0f;
		break;
	case 245:
		config::aimbot_button     = 318;
		config::triggerbot_button = 317;
		config::aimbot_fov        = 3.0f;
		config::aimbot_smooth     = 25.0f;
		break;
	case 246:
		config::aimbot_button     = 318;
		config::triggerbot_button = 317;
		config::aimbot_fov        = 4.0f;
		config::aimbot_smooth     = 20.0f;
		break;
	case 247:
		config::aimbot_button     = 318;
		config::triggerbot_button = 317;
		config::aimbot_fov        = 5.0f;
		config::aimbot_smooth     = 15.0f;
		break;
	case 248:
		config::aimbot_button     = 318;
		config::triggerbot_button = 317;
		config::aimbot_fov        = 6.0f;
		config::aimbot_smooth     = 10.0f;
		break;
	case 249:
		config::aimbot_button     = 318;
		config::triggerbot_button = 317;
		config::aimbot_fov        = 7.0f;
		config::aimbot_smooth     = 5.0f;
		break;
	//
	// mouse1 aimkey, mouse5 triggerkey
	//
	case 250:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 30.0f;
		break;
	case 251:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 3.0f;
		config::aimbot_smooth     = 25.0f;
		break;
	case 252:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 4.0f;
		config::aimbot_smooth     = 20.0f;
		break;
	case 253:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 5.0f;
		config::aimbot_smooth     = 15.0f;
		break;
	case 254:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 6.0f;
		config::aimbot_smooth     = 10.0f;
		break;
	case 255:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 7.0f;
		config::aimbot_smooth     = 5.0f;
		break;
	default:
		config::aimbot_button     = 314;
		config::triggerbot_button = 318;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 30.0f;
		break;
	}
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
	if (local_player_controller == 0)
	{
	NOT_INGAME:
		if (mouse_down_tick)
		{
			input::mouse1_up();
		}
		reset();
		return;
	}

	QWORD local_player = cs::entity::get_player(local_player_controller);

	if (local_player == 0)
	{
		goto NOT_INGAME;
	}

	//
	// update cheat settings
	//
	update_settings();


	BOOL  ffa         = cs::gamemode::is_ffa();
	DWORD num_shots   = cs::player::get_shots_fired(local_player);
	vec2  aim_punch   = cs::player::get_vec_punch(local_player);
	float sensitivity = cs::mouse::get_sensitivity() * cs::player::get_fov_multipler(local_player);

	if (config::rcs)
	{
		standalone_rcs(num_shots, aim_punch, sensitivity);
	}
	
	if (cs::input::is_button_down(config::triggerbot_button))
	{
		//
		// accurate shots only
		//
		if (aim_punch.x > -0.04f)
		{
			triggerbot(ffa, local_player);
		}
	}

	BOOL aimbot_key = cs::input::is_button_down(config::aimbot_button);

	if (!aimbot_key)
	{
		//
		// reset target
		//
		aimbot_target = 0;
	}

	QWORD best_target = 0;
	if (config::visuals_enabled == 2)
	{
		get_best_target(ffa, local_player_controller, local_player, num_shots, aim_punch, &best_target);
	}
	else
	{
		//
		// optimize: find target only when button not pressed
		//
		if (!aimbot_key)
		{
			get_best_target(ffa, local_player_controller, local_player, num_shots, aim_punch, &best_target);
		}
	}

	//
	// no previous target found, lets update target
	//
	if (aimbot_target == 0)
	{
		aimbot_target = best_target;
	}
	else
	{
		if (!cs::player::is_valid(aimbot_target, cs::player::get_node(aimbot_target)))
		{
			aimbot_target = best_target;

			if (aimbot_target == 0)
			{
				get_best_target(ffa, local_player_controller, local_player, num_shots, aim_punch, &aimbot_target);
			}
		}
	}


	aimbot_active = 0;


	//
	// no valid target found
	//
	if (aimbot_target == 0)
	{
		return;
	}

	if (!aimbot_key)
	{
		return;
	}

	QWORD node = cs::player::get_node(aimbot_target);
	if (node == 0)
	{
		return;
	}

	vec3 head{};
	if (!cs::node::get_bone_position(node, 6, &head))
	{
		return;
	}

	vec2 view_angle   = cs::engine::get_viewangles();
	vec3 aimbot_angle = get_target_angle(local_player, head, num_shots, aim_punch);

	if (math::get_fov(view_angle, aimbot_angle) > config::aimbot_fov)
	{
		return;
	}

	aimbot_active = 1;

	vec3 angles{};
	angles.x = view_angle.x - aimbot_angle.x;
	angles.y = view_angle.y - aimbot_angle.y;
	angles.z = 0;
	math::vec_clamp(&angles);

	float x = (angles.y / sensitivity) / 0.022f;
	float y = (angles.x / sensitivity) / -0.022f;
	
	float smooth_x = 0.00f;
	float smooth_y = 0.00f;

	DWORD aim_ticks = 0;
	if (config::aimbot_smooth >= 1.0f)
	{
		if (qabs(x) > 1.0f)
		{
			if (smooth_x < x)
				smooth_x = smooth_x + 1.0f + (x / config::aimbot_smooth);
			else if (smooth_x > x)
				smooth_x = smooth_x - 1.0f + (x / config::aimbot_smooth);
			else
				smooth_x = x;
		}
		else
		{
			smooth_x = x;
		}

		if (qabs(y) > 1.0f)
		{
			if (smooth_y < y)
				smooth_y = smooth_y + 1.0f + (y / config::aimbot_smooth);
			else if (smooth_y > y)
				smooth_y = smooth_y - 1.0f + (y / config::aimbot_smooth);
			else
				smooth_y = y;
		}
		else
		{
			smooth_y = y;
		}
		aim_ticks = (DWORD)(config::aimbot_smooth / 100.0f);
	}
	else
	{
		smooth_x = x;
		smooth_y = y;
	}

	DWORD current_tick = cs::engine::get_current_tick();
	if (current_tick - aimbot_tick > aim_ticks)
	{
		aimbot_tick = current_tick;
		input::mouse_move((int)smooth_x, (int)smooth_y);
	}
}

static vec3 features::get_target_angle(QWORD local_player, vec3 position, DWORD num_shots, vec2 aim_punch)
{
	vec3 eye_position = cs::node::get_origin(cs::player::get_node(local_player));
	eye_position.z    += cs::player::get_vec_view(local_player);
	// vec3 eye_position = cs::player::get_eye_position(local_player);

	vec3 angle{};
	angle.x = position.x - eye_position.x;
	angle.y = position.y - eye_position.y;
	angle.z = position.z - eye_position.z;

	math::vec_normalize(&angle);
	math::vec_angles(angle, &angle);

	if (num_shots > 0)
	{
		angle.x -= aim_punch.x * 2.0f;
		angle.y -= aim_punch.y * 2.0f;
	}

	math::vec_clamp(&angle);

	return angle;
}

static void features::get_best_target(BOOL ffa, QWORD local_controller, QWORD local_player, DWORD num_shots, vec2 aim_punch, QWORD *target)
{
	vec2 va = cs::engine::get_viewangles();

	float best_fov = 360.0f;
	
	for (int i = 1; i < 32; i++)
	{
		QWORD ent = cs::entity::get_client_entity(i);
		if (ent == 0 || (ent == local_controller))
		{
			continue;
		}

		//
		// is controller
		//
		if (!cs::entity::is_player(ent))
		{
			continue;
		}

		QWORD player = cs::entity::get_player(ent);
		if (player == 0)
		{
			continue;
		}

		if (ffa == 0)
		{
			if (cs::player::get_team_num(player) == cs::player::get_team_num(local_player))
			{
				continue;
			}
		}

		QWORD node = cs::player::get_node(player);
		if (node == 0)
		{
			continue;
		}

		if (!cs::player::is_valid(player, node))
		{
			continue;
		}

		vec3 head{};
		if (!cs::node::get_bone_position(node, 6, &head))
		{
			continue;
		}
		
#ifdef _KERNEL_MODE
		if (config::visuals_enabled == 2)
		{
			esp(local_player, player, head);
		}
#endif

		vec3 best_angle = get_target_angle(local_player, head, num_shots, aim_punch);

		float fov = math::get_fov(va, *(vec3*)&best_angle);
		
		if (fov < best_fov)
		{
			best_fov = fov;
			*target = player;
		}
	}
}

static void features::standalone_rcs(DWORD num_shots, vec2 vec_punch, float sensitivity)
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

static void features::triggerbot(BOOL ffa, QWORD local_player)
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

	if (!ffa)
	{
		if (cs::player::get_team_num(local_player) == cs::player::get_team_num(crosshair_target))
			return;
	}

	DWORD current_tick = cs::engine::get_current_tick();
	if (current_tick > mouse_up_tick)
	{
		input::mouse1_down();
		mouse_up_tick   = 0;
		mouse_down_tick = random_number(30, 50) + current_tick;
	}
}

#ifdef _KERNEL_MODE

namespace gdi
{
	void DrawRect(void *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b);
	void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b);
}

static void features::esp(QWORD local_player, QWORD target_player, vec3 head)
{
	input::WINDOW_INFO window = input::get_window_info();

	/*
	float view = cs::player::get_vec_view(local_player) - 10.0f;
	
	vec3 bottom;
	bottom.x = head.x;
	bottom.y = head.y;
	bottom.z = head.z - view;

	vec3 top;
	top.x = bottom.x;
	top.y = bottom.y;
	top.z = bottom.z + view;
	
	vec3 screen_bottom, screen_top;
	view_matrix_t view_matrix = cs::engine::get_viewmatrix();

	vec2 screen_size;
	screen_size.x = window.w;
	screen_size.y = window.h;

	if (!math::world_to_screen(screen_size, bottom, screen_bottom, view_matrix) || !math::world_to_screen(screen_size, top, screen_top, view_matrix))
	{
		return;
	}


	float fheight = (screen_bottom.y - screen_top.y);
	float height = fheight / 8.f;
	float width = fheight / 14.f;

	int box_height = (int)height;
	int box_width  = (int)width;

	int box_x = (int)screen_top.x;
	int box_y = (int)screen_top.y;

	box_x += (int)window.x;
	box_y += (int)window.y;
	

	gdi::DrawFillRect(GetActiveWindow(), box_x, box_y, box_width, box_height, 255, 0, 0);
	*/

	UNREFERENCED_PARAMETER(local_player);
	UNREFERENCED_PARAMETER(head);

	vec3 origin = cs::player::get_origin(target_player);
	vec3 top_origin = origin;
	top_origin.z += 75.0f;


	vec3 screen_bottom, screen_top;
	view_matrix_t view_matrix = cs::engine::get_viewmatrix();

	vec2 screen_size{};
	screen_size.x = (float)window.w;
	screen_size.y = (float)window.h;


	if (!math::world_to_screen(screen_size, origin, screen_bottom, view_matrix) || !math::world_to_screen(screen_size, top_origin, screen_top, view_matrix))
	{
		return;
	}


	float target_health = ((float)cs::player::get_health(target_player) / 100.0f) * 255.0f;
	float r = 255.0f - target_health;
	float g = target_health;
	float b = 0.00f;

	int box_height = (int)(screen_bottom.y - screen_top.y);
	int box_width = box_height / 2;

	LONG x = (LONG)window.x + (LONG)(screen_top.x - box_width / 2);
	LONG y = (LONG)window.y + (LONG)(screen_top.y);

	//if (screen_pos.x != 0)
	{
		if (x > (LONG)(window.x + screen_size.x - (box_width)))
		{
			return;
		}
		else if (x < window.x)
		{
			return;
		}
	}

	//if (screen_pos.y != 0)
	{
		if (y > (LONG)(screen_size.y + window.y - (box_height)))
		{
			return;
		}
		else if (y < window.y)
		{
			return;
		}
	}
	gdi::DrawRect((void *)0x10010, x, y, box_width, box_height, (unsigned char)r, (unsigned char)g, (unsigned char)b);
}

#endif

