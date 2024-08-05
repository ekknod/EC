#include "cs2game.h"

//
// private data. only available for features.cpp
//

namespace cs2
{
namespace features
{
	//
	// global shared variables
	// can be used at any features.cpp function
	//
	cs2::WEAPON_CLASS weapon_class;
	BOOL             b_aimbot_button;
	BOOL             b_triggerbot_button;


	//
	// triggerbot
	//
	static DWORD mouse_down_ms;
	static DWORD mouse_up_ms;

	//
	// rcs
	//
	static vec2  rcs_old_punch;

	//
	// aimbot
	//
	static BOOL  aimbot_active;
	static QWORD aimbot_target;
	static int   aimbot_bone;
	static DWORD aimbot_ms;

	//
	// infov event state
	//
	static BOOL event_state;


	void reset(void)
	{
		mouse_down_ms    = 0;
		mouse_up_ms      = 0;
		rcs_old_punch    = {};
		aimbot_active    = 0;
		aimbot_target    = 0;
		aimbot_bone      = 0;
		aimbot_ms        = 0;
	}

	inline void update_settings(void);

	
	static void has_target_event(QWORD local_player, QWORD target_player, float fov, vec2 aim_punch, vec3 aimbot_angle, vec2 view_angle, vec3 bone);


	static vec3 get_target_angle(QWORD local_player, vec3 position, DWORD num_shots, vec2 aim_punch);
	static void get_best_target(BOOL ffa, QWORD local_controller, QWORD local_player, DWORD num_shots, vec2 aim_punch, QWORD *target);
	static void standalone_rcs(DWORD shots_fired, vec2 vec_punch, float sensitivity);
	static void esp(QWORD local_player, QWORD target_player, vec3 head);
}
}

namespace config
{
	static BOOL  rcs;
	static BOOL  aimbot_enabled;
	static DWORD aimbot_button;
	static float aimbot_fov;
	static float aimbot_smooth;
	static BOOL  aimbot_multibone;
	static DWORD triggerbot_button;
	static BOOL  visuals_enabled;
}

inline DWORD random_number(DWORD min, DWORD max)
{
	return min + cs2::engine::get_current_ms() % (max + 1 - min);
}

inline void cs2::features::update_settings(void)
{
	int crosshair_alpha = cs2::get_crosshairalpha();


	//
	// default global settings
	//
	config::rcs = 0;
	config::aimbot_enabled = 1;
	config::aimbot_multibone = 1;


#ifdef _KERNEL_MODE
	config::visuals_enabled = 1;
#else
	config::visuals_enabled = 1;
#endif


	switch (weapon_class)
	{
	case cs2::WEAPON_CLASS::Knife:
	case cs2::WEAPON_CLASS::Grenade:
		config::aimbot_enabled = 0;
		break;
	case cs2::WEAPON_CLASS::Pistol:
		config::aimbot_multibone = 0;
		break;
	}


	switch (crosshair_alpha)
	{
	//
	// mouse5 aimkey, mouse4 triggerkey
	//
	case 244:
		config::aimbot_button     = 321;
		config::triggerbot_button = 320;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 5.0f;
		config::visuals_enabled   = 0;
		break;
	case 245:
		config::aimbot_button     = 321;
		config::triggerbot_button = 320;
		config::aimbot_fov        = 2.5f;
		config::aimbot_smooth     = 4.5f;
		break;
	case 246:
		config::aimbot_button     = 321;
		config::triggerbot_button = 320;
		config::aimbot_fov        = 3.0f;
		config::aimbot_smooth     = 4.0f;
		break;
	case 247:
		config::aimbot_button     = 321;
		config::triggerbot_button = 320;
		config::aimbot_fov        = 3.5f;
		config::aimbot_smooth     = 3.5f;
		break;
	case 248:
		config::aimbot_button     = 321;
		config::triggerbot_button = 320;
		config::aimbot_fov        = 4.0f;
		config::aimbot_smooth     = 3.0f;
		break;
	case 249:
		config::aimbot_button     = 321;
		config::triggerbot_button = 320;
		config::aimbot_fov        = 4.5f;
		config::aimbot_smooth     = 2.5f;
		break;
	//
	// mouse1 aimkey, mouse5 triggerkey
	//
	case 250:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
		config::aimbot_fov        = 2.0f;
		config::aimbot_smooth     = 5.0f;
		config::visuals_enabled   = 0;
		break;
	case 251:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
		config::aimbot_fov        = 2.5f;
		config::aimbot_smooth     = 4.5f;
		break;
	case 252:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
		config::aimbot_fov        = 3.0f;
		config::aimbot_smooth     = 4.0f;
		break;
	case 253:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
		config::aimbot_fov        = 3.5f;
		config::aimbot_smooth     = 3.5f;
		break;
	case 254:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
		config::aimbot_fov        = 4.0f;
		config::aimbot_smooth     = 3.0f;
		break;
	case 255:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
		config::aimbot_fov        = 4.5f;
		config::aimbot_smooth     = 2.5f;
		break;
	default:
		config::aimbot_button     = 317;
		config::triggerbot_button = 321;
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
static void cs2::features::has_target_event(QWORD local_player, QWORD target_player, float fov, vec2 aim_punch, vec3 aimbot_angle, vec2 view_angle, vec3 bone)
{
#ifndef __linux__
	UNREFERENCED_PARAMETER(local_player);
	UNREFERENCED_PARAMETER(target_player);
	UNREFERENCED_PARAMETER(aimbot_angle);
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

	if (b_triggerbot_button && config::aimbot_enabled && mouse_down_ms == 0 && event_state == 0)
	{
		float accurate_shots_fl = -0.08f;
		if (weapon_class == cs2::WEAPON_CLASS::Pistol)
		{
			accurate_shots_fl = -0.04f;
		}

		//
		// accurate shots only
		//
		if (aim_punch.x > accurate_shots_fl)
		{
			typedef struct {
				float radius;
				vec3  min;
				vec3  max;
			} COLL;

			COLL coll = {
				2.800000f, {-0.200000f, 1.100000f,  0.000000f},  {3.600000f,  0.100000f, 0.000000f}
			};

			vec3 dir = math::vec_atd(vec3{view_angle.x, view_angle.y, 0});
			vec3 eye = cs2::player::get_eye_position(local_player);

			matrix3x4_t matrix{};
			matrix[0][3] = bone.x;
			matrix[1][3] = bone.y;
			matrix[2][3] = bone.z;

			if (math::vec_min_max(eye, dir,
				math::vec_transform(coll.min, matrix),
				math::vec_transform(coll.max, matrix),
				coll.radius))
			{
				DWORD current_ms = cs2::engine::get_current_ms();
				if (current_ms > mouse_up_ms)
				{
					client::mouse1_down();
					mouse_up_ms   = 0;
					mouse_down_ms = random_number(30, 50) + current_ms;
				}
			}
		}
	}
}

void cs2::features::run(void)
{
	//
	// reset triggerbot input
	//
	if (mouse_down_ms)
	{
		DWORD current_ms = cs2::engine::get_current_ms();
		if (current_ms > mouse_down_ms)
		{
			client::mouse1_up();
			mouse_down_ms   = 0;
			mouse_up_ms     = random_number(30, 50) + current_ms;
		}
	}

	QWORD local_player_controller = cs2::entity::get_local_player_controller();
	if (local_player_controller == 0)
	{
	NOT_INGAME:
		if (mouse_down_ms)
		{
			client::mouse1_up();
		}
		reset();
		return;
	}

	QWORD local_player = cs2::entity::get_player(local_player_controller);

	if (local_player == 0)
	{
		goto NOT_INGAME;
	}

	weapon_class = cs2::player::get_weapon_class(local_player);
	if (weapon_class == cs2::WEAPON_CLASS::Invalid)
	{
		return;
	}


	//
	// update cheat settings
	//
	update_settings();



	//
	// update buttons
	//
	b_triggerbot_button = cs2::input::is_button_down(config::triggerbot_button);
	b_aimbot_button     = cs2::input::is_button_down(config::aimbot_button) | b_triggerbot_button;
	

	//
	// if we are holding triggerbot key, force head only
	//
	if (b_triggerbot_button)
	{
		config::aimbot_multibone = 0;
	}


	BOOL  ffa         = cs2::gamemode::is_ffa();
	DWORD num_shots   = cs2::player::get_shots_fired(local_player);
	vec2  aim_punch   = cs2::player::get_vec_punch(local_player);
	float sensitivity = cs2::mouse::get_sensitivity() * cs2::player::get_fov_multipler(local_player);

	if (config::rcs)
	{
		standalone_rcs(num_shots, aim_punch, sensitivity);
	}
	
	if (!b_aimbot_button)
	{
		//
		// reset target
		//
		aimbot_target = 0;
		aimbot_bone   = 0;
	}

	event_state = 0;

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
		if (!b_aimbot_button)
		{
			get_best_target(ffa, local_player_controller, local_player, num_shots, aim_punch, &best_target);
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
		if (!cs2::player::is_valid(aimbot_target, cs2::player::get_node(aimbot_target)))
		{
			aimbot_target = best_target;

			if (aimbot_target == 0)
			{
				aimbot_bone = 0;
				get_best_target(ffa, local_player_controller, local_player, num_shots, aim_punch, &aimbot_target);
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

	QWORD node = cs2::player::get_node(aimbot_target);
	if (node == 0)
	{
		return;
	}

	vec2 view_angle = cs2::player::get_viewangle(local_player);
	
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
			if (!cs2::node::get_bone_position(node, aimbot_bone, &pos))
			{
				return;
			}
			aimbot_pos   = pos;
			aimbot_angle = get_target_angle(local_player, pos, num_shots, aim_punch);
			aimbot_fov   = math::get_fov(view_angle, aimbot_angle);
		}
		else
		{
			for (DWORD i = 2; i < 7; i++)
			{
				vec3 pos{};
				if (!cs2::node::get_bone_position(node, i, &pos))
				{
					continue;
				}

				vec3  angle = get_target_angle(local_player, pos, num_shots, aim_punch);
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
		if (!cs2::node::get_bone_position(node, 6, &head))
		{
			return;
		}
		aimbot_pos   = head;
		aimbot_angle = get_target_angle(local_player, head, num_shots, aim_punch);
		aimbot_fov   = math::get_fov(view_angle, aimbot_angle);
	}

	if (event_state == 0)
	{
		if (aimbot_fov != 360.0f)
		{
			features::has_target_event(local_player, aimbot_target, aimbot_fov, aim_punch, aimbot_angle, view_angle, aimbot_pos);
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

	DWORD ms = 0;
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
		ms = (DWORD)(config::aimbot_smooth / 100.0f) + 1;
		ms = ms * 16;
	}
	else
	{
		smooth_x  = x;
		smooth_y  = y;
		ms        = 16;
	}

	DWORD current_ms = cs2::engine::get_current_ms();
	if (current_ms - aimbot_ms > ms)
	{
		aimbot_ms = current_ms;
		client::mouse_move((int)smooth_x, (int)smooth_y);
	}
}

static vec3 cs2::features::get_target_angle(QWORD local_player, vec3 position, DWORD num_shots, vec2 aim_punch)
{
	vec3 eye_position = cs2::node::get_origin(cs2::player::get_node(local_player));
	eye_position.z    += cs2::player::get_vec_view(local_player);

	//
	// which one is better???
	//
	// vec3 eye_position = cs2::player::get_eye_position(local_player);

	vec3 angle{};
	angle.x = position.x - eye_position.x;
	angle.y = position.y - eye_position.y;
	angle.z = position.z - eye_position.z;

	math::vec_normalize(&angle);
	math::vec_angles(angle, &angle);

	if (num_shots > 0)
	{
		if (weapon_class == cs2::WEAPON_CLASS::Sniper)
			goto skip_recoil;

		if (weapon_class == cs2::WEAPON_CLASS::Pistol)
		{
			if (num_shots < 2)
			{
				goto skip_recoil;
			}
		}
		angle.x -= aim_punch.x * 2.0f;
		angle.y -= aim_punch.y * 2.0f;
	}
skip_recoil:

	math::vec_clamp(&angle);

	return angle;
}

static void cs2::features::get_best_target(BOOL ffa, QWORD local_controller, QWORD local_player, DWORD num_shots, vec2 aim_punch, QWORD *target)
{
	vec2 va = cs2::player::get_viewangle(local_player);
	float best_fov = 360.0f;
	vec3  angle{};
	vec3  aimpos{};
	
	for (int i = 1; i < 64; i++)
	{
		QWORD ent = cs2::entity::get_client_entity(i);
		if (ent == 0 || (ent == local_controller))
		{
			continue;
		}

		//
		// is controller
		//
		if (!cs2::entity::is_player(ent))
		{
			continue;
		}

		QWORD player = cs2::entity::get_player(ent);
		if (player == 0)
		{
			continue;
		}

		if (ffa == 0)
		{
			if (cs2::player::get_team_num(player) == cs2::player::get_team_num(local_player))
			{
				continue;
			}
		}

		QWORD node = cs2::player::get_node(player);
		if (node == 0)
		{
			continue;
		}

		if (!cs2::player::is_valid(player, node))
		{
			continue;
		}

		vec3 head{};
		if (!cs2::node::get_bone_position(node, 6, &head))
		{
			continue;
		}
		
		if (config::visuals_enabled)
		{
			esp(local_player, player, head);
		}

		vec3 best_angle = get_target_angle(local_player, head, num_shots, aim_punch);

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
		features::has_target_event(local_player, *target, best_fov, aim_punch, angle, va, aimpos);
	}
}

static void cs2::features::standalone_rcs(DWORD num_shots, vec2 vec_punch, float sensitivity)
{
	if (num_shots > 1)
	{
		float x = (vec_punch.x - rcs_old_punch.x) * -1.0f;
		float y = (vec_punch.y - rcs_old_punch.y) * -1.0f;
		
		int mouse_angle_x = (int)(((y * 2.0f) / sensitivity) / -0.022f);
		int mouse_angle_y = (int)(((x * 2.0f) / sensitivity) / 0.022f);

		if (!aimbot_active)
		{
			client::mouse_move(mouse_angle_x, mouse_angle_y);
		}
	}
	rcs_old_punch = vec_punch;
}

static void cs2::features::esp(QWORD local_player, QWORD target_player, vec3 head)
{
	QWORD sdl_window = cs2::sdl::get_window();
	if (sdl_window == 0)
		return;


	cs2::WINDOW_INFO window{};
	if (!cs2::sdl::get_window_info(sdl_window, &window))
		return;

	/*
	float view = cs2::player::get_vec_view(local_player) - 10.0f;

	
	vec3 bottom;
	bottom.x = head.x;
	bottom.y = head.y;
	bottom.z = head.z - view;

	vec3 top;
	top.x = bottom.x;
	top.y = bottom.y;
	top.z = bottom.z + view;
	

	view_matrix_t view_matrix = cs2::engine::get_viewmatrix();


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

	float target_health = ((float)cs2::player::get_health(target_player) / 100.0f) * 255.0f;
	float r = 255.0f - target_health;
	float g = target_health;
	float b = 0.00f;
	
#ifdef __linux__
	client::DrawFillRect((void *)0, x, y, w, h, (unsigned char)r, (unsigned char)g, (unsigned char)b);
#else
	QWORD sdl_window_data = cs2::sdl::get_window_data(sdl_window);
	if (sdl_window_data == 0)
		return;
	QWORD hwnd = cs2::sdl::get_hwnd(sdl_window_data);
	client::DrawFillRect((void *)hwnd, x, y, w, h, (unsigned char)r, (unsigned char)g, (unsigned char)b);
#endif
	*/
	
	
	
#ifndef __linux__
	UNREFERENCED_PARAMETER(local_player);
	UNREFERENCED_PARAMETER(head);
#endif

	vec3 origin = cs2::player::get_origin(target_player);
	vec3 top_origin = origin;
	top_origin.z += 75.0f;


	vec3 screen_bottom, screen_top;
	view_matrix_t view_matrix = cs2::engine::get_viewmatrix();

	vec2 screen_size{};
	screen_size.x = (float)window.w;
	screen_size.y = (float)window.h;


	if (!math::world_to_screen(screen_size, origin, screen_bottom, view_matrix) || !math::world_to_screen(screen_size, top_origin, screen_top, view_matrix))
	{
		return;
	}


	float target_health = ((float)cs2::player::get_health(target_player) / 100.0f) * 255.0f;
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

#ifdef __linux__
	client::DrawRect((void *)0, x, y, box_width, box_height, (unsigned char)r, (unsigned char)g, (unsigned char)b);
#else
	QWORD sdl_window_data = cs2::sdl::get_window_data(sdl_window);
	if (sdl_window_data == 0)
		return;
	QWORD hwnd = cs2::sdl::get_hwnd(sdl_window_data);
	client::DrawRect((void *)hwnd, x, y, box_width, box_height, (unsigned char)r, (unsigned char)g, (unsigned char)b);
#endif
}

