#include "apexgame.h"

namespace apex
{
namespace features
{
	static BOOL b_aimbot_button;

	//
	// game settings
	//
	static BOOL firing_range;

	//
	// aimbot
	//
	static BOOL  aimbot_active;
	static QWORD aimbot_target;
	static int   aimbot_bone;
	static DWORD aimbot_tick;

	//
	// weapon data
	//
	static float bullet_speed;

	//
	// infov event state
	//
	static BOOL event_state;

	//
	// cached viewmatrix
	//
	static view_matrix_t view_matrix;

	void reset(void)
	{
		b_aimbot_button = 0;
		firing_range    = 0;
		aimbot_active   = 0;
		aimbot_target   = 0;
		aimbot_bone     = 0;
		aimbot_tick     = 0;
		bullet_speed    = 0;
		event_state     = 0;
	}

	inline void update_settings(void);
	static void has_target_event(QWORD local_player, QWORD target_player, float fov, vec3 bone);
	static vec3 get_target_angle(QWORD local_player, QWORD target_player, vec3 position);
	static void get_best_target(QWORD local_player, QWORD *target);
	static void esp(QWORD local_player, QWORD target_player, vec3 head);
}
}

namespace config
{
	static BOOL  aimbot_enabled;
	static DWORD aimbot_button;
	static float aimbot_fov;
	static float aimbot_smooth;
	static BOOL  aimbot_multibone;
	static BOOL  visuals_enabled;
}

inline void apex::features::update_settings(void)
{
	int controller_sensitivity = apex::get_controller_sensitivity();


	//
	// default global settings
	//
	config::aimbot_enabled = 1;
	config::aimbot_multibone = 1;


#ifdef _KERNEL_MODE
	config::visuals_enabled = 1;
#else
	config::visuals_enabled = 1;
#endif

	switch (controller_sensitivity)
	{
	//
	// mouse5 aimkey
	//
	case 1:
		config::aimbot_button     = 111;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 5.0f;
		config::visuals_enabled   = 0;
		break;
	case 2:
		config::aimbot_button     = 111;
		config::aimbot_fov        = 2.5f;
		config::aimbot_smooth     = 4.5f;
		break;
	case 3:
		config::aimbot_button     = 111;
		config::aimbot_fov        = 3.0f;
		config::aimbot_smooth     = 4.0f;
		break;
	case 4:
		config::aimbot_button     = 111;
		config::aimbot_fov        = 5.0f;
		config::aimbot_smooth     = 3.5f;
		break;
	//
	// mouse1 aimkey
	//
	case 5:
		config::aimbot_button     = 107;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 5.0f;
		config::visuals_enabled   = 0;
		break;
	case 6:
		config::aimbot_button     = 107;
		config::aimbot_fov        = 2.5f;
		config::aimbot_smooth     = 4.5f;
		break;
	case 7:
		config::aimbot_button     = 107;
		config::aimbot_fov        = 3.0f;
		config::aimbot_smooth     = 4.0f;
		break;
	case 8:
		config::aimbot_button     = 107;
		config::aimbot_fov        = 5.0f;
		config::aimbot_smooth     = 3.5f;
		break;
	default:
		config::aimbot_button     = 111;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 5.0f;
		config::visuals_enabled   = 0;
		break;
	}
}

//
// this event is called from get best target/aimbot,
// when we have active target
//
static void apex::features::has_target_event(QWORD local_player, QWORD target_player, float fov, vec3 bone)
{
#ifndef __linux__
	UNREFERENCED_PARAMETER(local_player);
	UNREFERENCED_PARAMETER(target_player);
	UNREFERENCED_PARAMETER(bone);
#endif

	if (config::visuals_enabled)
	{
		if (fov < 5.0f)
		{
			//
			// net_graph( r, g , b ) 
			//
		}

		//
		// update ESP
		//
		if (event_state == 0)
		{
			esp(local_player, target_player, bone);
		}
	}
}

void apex::features::run(void)
{
	QWORD local_player = apex::entity::get_local_player();
	if (local_player == 0)
	{
		reset();
		return;
	}
	
	//
	// update cheat settings
	//
	update_settings();


	//
	// update buttons
	//
	b_aimbot_button = apex::input::is_button_down(config::aimbot_button);


	QWORD weapon = apex::player::get_weapon(local_player);


	float fov_multiplier = 1.0f;	
	if (apex::player::is_zoomed_in(local_player))
	{
		float weapon_zoom    = apex::weapon::get_zoom_fov(weapon);
		fov_multiplier = (weapon_zoom / 90.0f);
	}

	bullet_speed = apex::weapon::get_bullet_speed(weapon);


	float sensitivity = apex::mouse::get_sensitivity() * fov_multiplier;


	if (!b_aimbot_button)
	{
		//
		// reset target
		//
		aimbot_target = 0;
		aimbot_bone   = 0;
	}

	view_matrix = apex::engine::get_viewmatrix();

	event_state = 0;

	QWORD best_target = 0;
	if (config::visuals_enabled == 2)
	{
		get_best_target(local_player, &best_target);
	}
	else
	{
		//
		// optimize: find target only when button not pressed
		//
		if (!b_aimbot_button)
		{
			get_best_target(local_player, &best_target);
		}
	}

	//
	// no previous target found, lets update target
	//
	if (aimbot_target == 0)
	{
		aimbot_bone   = 0;
		aimbot_target = best_target;
	}
	else
	{
		BOOL valid=1;
		if (!firing_range)
		{
			valid = apex::player::is_valid(aimbot_target);
		}

		if (valid)
		{
			valid = apex::player::get_health(aimbot_target) > 0;
		}

		if (!valid)
		{
			aimbot_target = best_target;

			if (aimbot_target == 0)
			{
				aimbot_bone = 0;
				get_best_target(local_player, &aimbot_target);
			}
		}
	}

	aimbot_active = 0;

	if (!config::aimbot_enabled)
	{
		return;
	}

	//
	// no valid target found
	//
	if (aimbot_target == 0)
	{
		return;
	}

	if (!b_aimbot_button)
	{
		return;
	}

	vec2 view_angle = apex::player::get_viewangles(local_player);

	vec3  aimbot_angle{};
	vec3  aimbot_pos{};
	float aimbot_fov = 360.0f;

	if (config::aimbot_multibone)
	{
		//
		// use cached information for saving resources
		//
		if (aimbot_bone != 0)
		{
			vec3 pos{};
			if (!apex::player::get_bone_position(aimbot_target, aimbot_bone, &pos))
			{
				return;
			}
			aimbot_pos   = pos;
			aimbot_angle = get_target_angle(local_player, aimbot_target, pos);
			aimbot_fov   = math::get_fov(view_angle, aimbot_angle);
		}
		else
		{
			for (DWORD i = 2; i < 7; i++)
			{
				vec3 pos{};
				if (!apex::player::get_bone_position(aimbot_target, i, &pos))
				{
					continue;
				}

				vec3  angle = get_target_angle(local_player, aimbot_target, pos);
				float fov   = math::get_fov(view_angle, angle);

				if (fov < aimbot_fov)
				{
					aimbot_fov   = fov;
					aimbot_pos   = pos;
					aimbot_angle = angle;
					aimbot_bone  = i;
				}
			}
		}
	}
	else
	{
		vec3 head{};
		if (!apex::player::get_bone_position(aimbot_target, 2, &head))
		{
			return;
		}
		aimbot_pos   = head;
		aimbot_angle = get_target_angle(local_player, aimbot_target, head);
		aimbot_fov   = math::get_fov(view_angle, aimbot_angle);
	}

	if (event_state == 0)
	{
		if (aimbot_fov != 360.0f)
		{
			features::has_target_event(local_player, aimbot_target, aimbot_fov, aimbot_pos);
		}
	}

	if (aimbot_fov > config::aimbot_fov)
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
		aim_ticks = (DWORD)(config::aimbot_smooth / 100.0f) + 1;
	}
	else
	{
		smooth_x  = x;
		smooth_y  = y;
	}

	DWORD current_tick = apex::engine::get_current_tick();
	if (current_tick - aimbot_tick > aim_ticks)
	{
		aimbot_tick = current_tick;
		client::mouse_move((int)smooth_x, (int)smooth_y);
	}
}

static vec3 apex::features::get_target_angle(QWORD local_player, QWORD target_player, vec3 position)
{
	vec3 angle{};

	vec3 local_position  = apex::player::get_camera_origin(local_player);
	vec3 target_velocity = apex::player::get_velocity(target_player);

	float distance = math::vec_distance(position, local_position);
	float horizontal_time = (distance / bullet_speed);
	float vertical_time = (distance / bullet_speed);

	position.x += target_velocity.x * horizontal_time;
	position.y += target_velocity.y * horizontal_time;
	position.z += (750.0f * vertical_time * vertical_time);

	angle = math::CalcAngle(local_position, position);
	math::vec_clamp(&angle);

	return angle;
}

static void apex::features::get_best_target(QWORD local_player, QWORD *target)
{
	vec2  va = apex::player::get_viewangles(local_player);
	float best_fov = 360.0f;
	vec3  angle{};
	vec3  aimpos{};

	int max_count = 70;
	int ent_count = 0;
	firing_range  = 0;
	
	for (int i = 0; i < max_count; i++)
	{
		QWORD player = apex::entity::get_client_entity(i);
		if (player == 0 || (player == local_player))
		{
			//
			// test if we are in firing range
			//
			if (max_count == (i+1) && ent_count == 0)
			{
				max_count    = 10000;
				firing_range = 1;
			}
			continue;
		}

		ent_count++;

		if (!firing_range)
		{
			if (!apex::player::is_valid(player))
			{
				continue;
			}

			if (apex::player::get_team_num(player) == apex::player::get_team_num(local_player))
			{
				continue;
			}
		}

		if (apex::player::get_health(player) < 1)
		{
			continue;
		}

		vec3 head{};
		if (!apex::player::get_bone_position(player, 2, &head))
		{
			continue;
		}
		
		if (config::visuals_enabled)
		{
			esp(local_player, player, head);
		}

		vec3 best_angle = get_target_angle(local_player, player, head);

		float fov = math::get_fov(va, *(vec3*)&best_angle);
		
		if (fov < best_fov)
		{
			best_fov = fov;
			*target  = player;
			aimpos   = head;
			angle    = best_angle;
		}
	}
	
	if (best_fov != 360.0f)
	{
		event_state = 1;
		features::has_target_event(local_player, *target, best_fov, aimpos);
	}
}

static void apex::features::esp(QWORD local_player, QWORD target_player, vec3 head)
{
	apex::WINDOW_INFO window{};
	if (!apex::engine::get_window_info(&window))
		return;

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
	
	if (x > (int)(window.x + screen_size.x - (w)))
	{
		return;
	}
	else if (x < window.x)
	{
		return;
	}

	if (y > (int)(screen_size.y + window.y - (h)))
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
	
#ifdef __linux__
	client::DrawFillRect((void *)0, x, y, w, h, (unsigned char)r, (unsigned char)g, (unsigned char)b);
#else
	QWORD sdl_window_data = cs::sdl::get_window_data(sdl_window);
	if (sdl_window_data == 0)
		return;
	QWORD hwnd = cs::sdl::get_hwnd(sdl_window_data);
	client::DrawFillRect((void *)hwnd, x, y, w, h, (unsigned char)r, (unsigned char)g, (unsigned char)b);
#endif
	*/
	
	
	
#ifndef __linux__
	UNREFERENCED_PARAMETER(local_player);
	UNREFERENCED_PARAMETER(head);
#endif

	vec3 origin = apex::player::get_origin(target_player);
	vec3 top_origin = origin;
	top_origin.z += 75.0f;


	vec3 screen_bottom, screen_top;

	vec2 screen_size{};
	screen_size.x = (float)window.w;
	screen_size.y = (float)window.h;


	if (!math::world_to_screen(screen_size, origin, screen_bottom, view_matrix) || !math::world_to_screen(screen_size, top_origin, screen_top, view_matrix))
	{
		return;
	}

	int  max_health     = apex::player::get_max_health(target_player);
	float target_health = ((float)apex::player::get_health(target_player) / (float)max_health) * 255.0f;
	float r = 255.0f - target_health;
	float g = target_health;
	float b = 0.00f;


	if (config::visuals_enabled == 2)
	{
		if (aimbot_target == target_player)
		{
			g = 144.0f;
			b = 255.0f;
		}
	}

	int box_height = (int)(screen_bottom.y - screen_top.y);
	int box_width = box_height / 2;

	int x = (int)window.x + (int)(screen_top.x - box_width / 2);
	int y = (int)window.y + (int)(screen_top.y);

	if (x > (int)(window.x + screen_size.x - (box_width)))
	{
		return;
	}
	else if (x < window.x)
	{
		return;
	}

	if (y > (int)(screen_size.y + window.y - (box_height)))
	{
		return;
	}
	else if (y < window.y)
	{
		return;
	}

	if (vm::get_target_os() == VmOs::Linux)
	{
		client::DrawRect((void *)0, x, y, box_width, box_height, (unsigned char)r, (unsigned char)g, (unsigned char)b);
	}
	else
	{
		QWORD hwnd = apex::engine::get_hwnd();
		client::DrawRect((void *)hwnd, x, y, box_width, box_height, (unsigned char)r, (unsigned char)g, (unsigned char)b);
	}
}

