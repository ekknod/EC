#include "shared.h"

namespace features
{
	static C_Player m_previous_target = 0;
	static DWORD    m_previous_tick = 0;
	static float    m_previous_target_visible_time  = 0;
	static BOOL     m_previous_button = 0;

	void reset(void)
	{
		m_previous_button = 0;
		m_previous_target_visible_time = 0;
		m_previous_target = 0;
		m_previous_tick = 0;
	}

	static void     aimbot(C_Player local_player, C_Player target_player, float target_visible_time);
	static C_Player get_best_target(C_Player local_player, float *visible_time);
	static BOOL     get_target_angle(C_Player local_player, C_Player target_player, int bone_index, vec3 *vec_out);
}

void features::run(void)
{
	C_Player local_player = apex::teams::get_local_player();
	if (local_player == 0)
	{
		reset();
		return;
	}

	BOOL aimbot_button = apex::input::get_button_state(config::aimbot_button);


	if (!m_previous_button && aimbot_button)
	{
		//
		// reset target
		//
		m_previous_target = 0;
	}


	if (m_previous_target)
	{
		if (!apex::player::is_valid(m_previous_target))
		{
			m_previous_target = 0;
		}
		else
		{
			vec3 target_angle;
			vec2 va;
			
			if (get_target_angle(local_player, m_previous_target, 2, &target_angle) &&
				apex::player::get_viewangles(local_player, &va)
				)
			{
				float fov = math::get_fov(va, target_angle);
				if (fov > 25.0f)
				{
					m_previous_target = 0;
				}
			} else {
				m_previous_target = 0;
			}
		}
	}

	float    target_visible_time = 0; 
	C_Player target_player = m_previous_target;

	if (target_player == 0)
	{
		target_player = get_best_target(local_player, &target_visible_time);
	}
	else
	{
		target_visible_time = m_previous_target_visible_time ;
	}

	m_previous_target = target_player;
	m_previous_target_visible_time = target_visible_time;

	//
	// target not found
	//
	if (target_player == 0)
	{
		reset();
		return;
	}

	if (aimbot_button)
	{
		aimbot(local_player, target_player, target_visible_time);
	}

}

static void features::aimbot(C_Player local_player, C_Player target_player, float target_visible_time)
{
	BOOL best_angle_found = 0;

	float best_fov = 360.0f;
	int bone_list[] = { 2, 3, 5, 8 };
	vec3 aimbot_angle{};

	vec2 viewangles;
	if (!apex::player::get_viewangles(local_player, &viewangles))
	{
		return;
	}

	for (int i = 0; i < 4; i++)
	{
		vec3 angle;
		if (get_target_angle(local_player, target_player, bone_list[i], &angle))
		{
			float fov  = math::get_fov( viewangles, angle );
			if (fov < best_fov)
			{
				best_fov = fov;
				aimbot_angle = angle;
				best_angle_found = 1;
			}
		}
	}

	if (best_angle_found == 0)
	{
		return;
	}

	float fov = math::get_fov(viewangles, aimbot_angle);
	if (fov > config::aimbot_fov)
	{
		return;
	}

	float zoom_fov = apex::weapon::get_zoom_fov(apex::player::get_weapon(local_player));
	if (zoom_fov == 0.00f)
	{
		return;
	}

	float sensitivity = apex::engine::get_sensitivity();
	if (sensitivity == 0.00f)
	{
		return;
	}

	if (zoom_fov != 0 && zoom_fov != 90)
	{
		sensitivity = (zoom_fov / 90.0f) * sensitivity;
	}

	vec3 angles;
	angles.x = viewangles.x - aimbot_angle.x;
	angles.y = viewangles.y - aimbot_angle.y;
	angles.z = 0;
	math::vec_clamp(&angles);
	if (qabs(angles.x) > 25.00f || qabs(angles.y) > 25.00f)
	{
		return;
	}

	float x = angles.y;
	float y = angles.x;

	x = (x / sensitivity) / 0.022f;
	y = (y / sensitivity) / -0.022f;

	float sx = 0.0f, sy = 0.0f;
	float smooth = config::aimbot_smooth;

	//
	// xd
	//
	if (smooth < 2.0f)
	{
		smooth = 2.0f;
	}

	DWORD aim_ticks = 0;
	if (smooth >= 1.0f) {
		if (sx < x)
			sx = sx + 1.0f + (x / smooth);
		else if (sx > x)
			sx = sx - 1.0f + (x / smooth);
		else
			sx = x;

		if (sy < y)
			sy = sy + 1.0f + (y / smooth);
		else if (sy > y)
			sy = sy - 1.0f + (y / smooth);
		else
			sy = y;
		aim_ticks = (DWORD)(smooth / 100.0f);
	}
	else {
		sx = x;
		sy = y;
	}

	if (qabs((int)sx) > 127)
		return;

	if (qabs((int)sy) > 127)
		return;

	DWORD current_tick = apex::engine::get_current_tick();
	if (current_tick - m_previous_tick > aim_ticks)
	{
		m_previous_tick = current_tick;

		if (config::aimbot_visibility_check)
		{
			float visible_time = apex::player::get_visible_time(target_player);
			if (visible_time > target_visible_time)
			{
				m_previous_target_visible_time = visible_time;
				input::mouse_move((int)sx, (int)sy);
			}
		} else {
			input::mouse_move((int)sx, (int)sy);
		}
	}
}

static C_Player features::get_best_target(C_Player local_player, float *visible_time)
{
	C_Player best_target = 0;
	float best_fov = 360.0f;
	vec2 viewangles;


	if (!apex::player::get_viewangles(local_player, &viewangles))
	{
		return 0;
	}


	int team_id = apex::player::get_team_id(local_player);
	for (int i = 0; i < 70; i++)
	{
		C_Player entity = (C_Player)apex::entity::get_entity(i);
		if (entity == local_player)
		{
			continue;
		}

		if (entity == 0)
		{
			continue;
		}

		if (apex::player::get_team_id(entity) == team_id)
		{
			continue;
		}
		
		if (!apex::player::is_valid(entity))
		{
			continue;
		}	

		if (config::visuals_enabled)
		{
			apex::player::enable_glow(entity);
		}

		float target_visible_time = apex::player::get_visible_time(entity);
		if (target_visible_time == 0)
		{
			continue;
		}

		vec3 target_angle;
		if (get_target_angle(local_player, entity, 2, &target_angle))
		{
			float fov = math::get_fov(viewangles, target_angle);
			if (fov < best_fov)
			{
				best_fov = fov;
				best_target = entity;
				*visible_time = target_visible_time;
			}
		}
	}
	return best_target;
}

static BOOL features::get_target_angle(C_Player local_player, C_Player target_player, int bone_index, vec3 *vec_out)
{
	vec3 target_position;
	if (!apex::player::get_bone_position(target_player, bone_index, &target_position))
	{
		return 0;
	}

	vec3 local_position;
	if (!apex::player::get_muzzle(local_player, &local_position))
	{
		return 0;
	}

	C_Weapon local_weapon = apex::player::get_weapon(local_player);
	if (local_weapon == 0)
	{
		return 0;
	}

	float bullet_speed = apex::weapon::get_bullet_speed(local_weapon);
	if (bullet_speed == 0.00f)
	{
		return 0;
	}

	vec3 target_velocity;
	if (!apex::player::get_velocity(target_player, &target_velocity))
	{
		return 0;
	}

	float distance = math::vec_distance(target_position, local_position);
	if (distance == 0.0f)
	{
		return 0;
	}

	float horizontal_time = (distance / bullet_speed);
	float vertical_time = (distance / bullet_speed);

	target_position.x += target_velocity.x * horizontal_time;
	target_position.y += target_velocity.y * horizontal_time;
	target_position.z += (750.0f * vertical_time * vertical_time);

	*vec_out = math::CalcAngle(local_position, target_position);
	math::vec_clamp(vec_out);
	return 1;
}

