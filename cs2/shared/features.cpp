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

	if (aimbot_target != 0)
	{
		vec3 head{};
		if (cs::node::get_bone_position(cs::player::get_node(aimbot_target), 6, &head))
		{
			// Check if the target is visible to the local player
			bool is_visible = cs::player::visible_check(aimbot_target);
			if (!is_visible)
			{
				aimbot_target = 0;  // Reset the target if not visible
				return;
			}
		}
		else
		{
			aimbot_target = 0;  // Reset the target if bone position retrieval fails
			return;
		}
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

		bool is_visible = cs::player::visible_check(player);
		if (!is_visible)
			continue;

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
		// Generate random values between -1.0 and 1.0
		float random_factor_x = static_cast<float>(rand()) / RAND_MAX * 4.0f - 2.0f;
		float random_factor_y = static_cast<float>(rand()) / RAND_MAX * 4.0f - 2.0f;

		// Apply random factors to vec_punch
		vec_punch.x += random_factor_x;
		vec_punch.y += random_factor_y;

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
		mouse_down_tick = random_number(5, 10) + current_tick;
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
	QWORD sdl_window = cs::sdl::get_window();
	if (sdl_window == 0)
		return;

	QWORD sdl_window_data = cs::sdl::get_window_data(sdl_window);
	if (sdl_window_data == 0)
		return;

	cs::WINDOW_INFO window{};
	if (!cs::sdl::get_window_info(sdl_window, &window))
		return;

	QWORD hwnd = cs::sdl::get_hwnd(sdl_window_data);

	
	float view = cs::player::get_vec_view(local_player) - 10.0f;
	
	vec3 bottom;
	bottom.x = head.x;
	bottom.y = head.y;
	bottom.z = head.z - view;

	vec3 top;
	top.x = bottom.x;
	top.y = bottom.y;
	top.z = bottom.z + view;
	

	view_matrix_t view_matrix = cs::engine::get_viewmatrix();


	vec2 screen_size;
	screen_size.x = window.w;
	screen_size.y = window.h;


	vec3 screen_bottom, screen_top;
	if (!math::world_to_screen(screen_size, bottom, screen_bottom, view_matrix) || !math::world_to_screen(screen_size, top, screen_top, view_matrix))
	{
		return;
	}

	float height  = (screen_bottom.y - screen_top.y) / 8.f;
	float width   = (screen_bottom.y - screen_top.y) / 14.f;

	int x = (int)((screen_top.x - (width  / 2.0f)) + window.x);
	int y = (int)((screen_top.y - (height / 2.0f)) + window.y);
	int w = (int)width;
	int h = (int)height;
	
	if (x > (LONG)(window.x + screen_size.x - (w)))
	{
		return;
	}
	else if (x < window.x)
	{
		return;
	}

	if (y > (LONG)(screen_size.y + window.y - (h)))
	{
		return;
	}
	else if (y < window.y)
	{
		return;
	}

	float target_health = ((float)cs::player::get_health(target_player) / 100.0f) * 255.0f;
	float r = 255.0f - target_health;
	float g = target_health;
	float b = 0.00f;
	

	gdi::DrawFillRect((void *)hwnd, x, y, w, h, (unsigned char)r, (unsigned char)g, (unsigned char)b);
	
	/*
	
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

	if (x > (LONG)(window.x + screen_size.x - (box_width)))
	{
		return;
	}
	else if (x < window.x)
	{
		return;
	}

	if (y > (LONG)(screen_size.y + window.y - (box_height)))
	{
		return;
	}
	else if (y < window.y)
	{
		return;
	}

	gdi::DrawRect((void *)hwnd, x, y, box_width, box_height, (unsigned char)r, (unsigned char)g, (unsigned char)b);
	*/
}

#endif

