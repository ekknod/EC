#include "shared.h"

//
// private data. only available for features.cpp
//

namespace features
{
	static vec2    m_rcs_old_punch{ 0, 0 };
	static float   m_rcs_old_temp_punch_x = 0;
	static DWORD   m_rcs_old_shots_fired = 0;

	static float   m_aimbot_old_temp_punch_x = 0;
	static DWORD   m_aimbot_old_shots_fired = 0;
	static BOOL    m_aimbot_active = 0;

	static BOOL  m_previous_button = 0;
	static cs::WEAPON_CLASS m_weapon_class = cs::WEAPON_CLASS::Invalid;
	static C_Player m_previous_target = 0;
	static C_Team   m_previous_team = 0;
	static DWORD    m_previous_tick = 0;
	static QWORD    m_mouse_down_tick = 0;
	static QWORD    m_mouse_click_ticks = 0;

	void reset(void)
	{
		m_mouse_click_ticks = 0;
		m_previous_tick = 0;
		m_previous_button = 0;
		m_previous_target = 0;
		m_previous_team = 0;
		m_weapon_class = cs::WEAPON_CLASS::Invalid;
	}

	static void     standalone_rcs(C_Player local_player);
	static BOOL     triggerbot(C_Player local_player, C_Player target_player, C_Team target_team, cs::WEAPON_CLASS weapon_class);
	static void     aimbot(C_Player local_player, C_Player target_player, cs::WEAPON_CLASS weapon_class, DWORD bullet_count, BOOL head_only);
	static vec3     get_target_angle(C_Player local_player, vec3 position, DWORD bullet_count);
	static C_Player get_best_target(C_Player local_player, DWORD bullet_count, C_Team* target_team);
}

static QWORD get_random_number();
static QWORD random_number(QWORD min, QWORD max)
{
	return min + get_random_number() % (max + 1 - min);
}

void features::run(void)
{
	//
	// release mouse click if needed to
	//
	if (m_mouse_down_tick)
	{
		QWORD current_tick = cs::engine::get_current_tick();
		if (current_tick > m_mouse_click_ticks)
		{
			input::mouse1_up();
			m_mouse_down_tick = 0;
			m_mouse_click_ticks = current_tick + random_number(25, 50);
		}
	}

	C_Player local_player = cs::teams::get_local_player();
	if (local_player == 0)
	{
		if (m_mouse_down_tick)
		{
			input::mouse1_up();
			m_mouse_down_tick = 0;
		}
		reset();
		return;
	}

	BOOL mouse_1 = cs::input::get_button_state(107);
	BOOL aimbot_button = cs::input::get_button_state(config::aimbot_button);
	BOOL triggerbot_button = cs::input::get_button_state(config::triggerbot_button);
	BOOL current_button = aimbot_button + triggerbot_button + mouse_1;


	if (!m_previous_button && current_button)
	{
		m_rcs_old_shots_fired = 0;
		m_weapon_class = cs::player::get_weapon_class(local_player);
	}

	m_previous_button = current_button;


	//
	// remove this in case some feature added what requires 24/7 action
	//
	if (current_button == 0)
	{
		return;
	}


	BOOL  b_can_aimbot = 0;
	DWORD rcs_bullet_count = 0;

	if (m_weapon_class == cs::WEAPON_CLASS::Knife)
	{
		b_can_aimbot = 0;
	}
	else if (m_weapon_class == cs::WEAPON_CLASS::Grenade)
	{
		b_can_aimbot = 0;
	}
	else if (m_weapon_class == cs::WEAPON_CLASS::Pistol)
	{
		b_can_aimbot = 1;
		rcs_bullet_count = 1;
	}
	else if (m_weapon_class == cs::WEAPON_CLASS::Sniper)
	{
		b_can_aimbot = 1;
		rcs_bullet_count = 2;
	}
	else if (m_weapon_class == cs::WEAPON_CLASS::Rifle)
	{
		b_can_aimbot = 1;
		rcs_bullet_count = 0;
	}
	else
	{
		//
		// invalid weapon class
		//
		return;
	}

	if (aimbot_button || triggerbot_button)
	{
		m_aimbot_active = 1;
	}
	else
	{
		m_aimbot_active = 0;
	}

	if (config::rcs)
	{
		if (m_weapon_class == cs::WEAPON_CLASS::Rifle)
		{
			standalone_rcs(local_player);
		}
	}

	if (m_previous_target)
	{
		if (!cs::player::is_valid(m_previous_target))
		{
			m_previous_target = 0;
		}
		else
		{
			matrix3x4_t matrix;
			if (cs::player::get_bone_position(m_previous_target, 8, &matrix))
			{
				vec3 bonepos={};
				bonepos.x = matrix[0][3];
				bonepos.y = matrix[1][3];
				bonepos.z = matrix[2][3];

				vec3 best_angle = get_target_angle(local_player, bonepos, rcs_bullet_count);
				float fov = math::get_fov(cs::engine::get_viewangles(), *(vec3*)&best_angle);

				if (fov > config::aimbot_fov)
				{
					//
					// reset target
					//
					m_previous_target = 0;
				}
			}
			else {
				//
				// reset target
				//
				m_previous_target = 0;
			}
		}
	}

	C_Team   target_team = 0;
	C_Player target_player = m_previous_target;
	if (target_player == 0)
	{
		m_aimbot_old_shots_fired = 0;
		target_player = get_best_target(local_player, rcs_bullet_count, &target_team);
	}
	else
	{
		target_team = m_previous_team;
	}

	m_previous_target = target_player;
	m_previous_team = target_team;

	//
	// target not found
	//
	if (target_player == 0)
	{
		return;
	}

	if (b_can_aimbot)
	{
		BOOL force_head_only = 0;
		if (triggerbot_button)
		{
			force_head_only = triggerbot(local_player, target_player, target_team, m_weapon_class);
		}

		if (aimbot_button || triggerbot_button)
		{
			features::aimbot(local_player,
				target_player, m_weapon_class, rcs_bullet_count, force_head_only);
		}
	}
}

void features::reset_mouse(void)
{
	if (m_mouse_down_tick)
	{
		input::mouse1_up();
		m_mouse_down_tick = 0;
	}
}

static void features::standalone_rcs(C_Player local_player)
{

	vec2 current_punch = cs::player::get_vec_punch(local_player);
	DWORD shots_fired = (DWORD)cs::player::get_shots_fired(local_player);

	if (shots_fired > 1)
	{
		vec2 new_punch{};
		new_punch.x = current_punch.x - m_rcs_old_punch.x;
		new_punch.y = current_punch.y - m_rcs_old_punch.y;


		//
		// https://www.bilibili.com/video/BV1Ma4y147TS
		//
		if (shots_fired > m_rcs_old_shots_fired && m_rcs_old_temp_punch_x < new_punch.x)
		{
			new_punch.x = m_rcs_old_temp_punch_x;
		}

		m_rcs_old_shots_fired = shots_fired;
		m_rcs_old_temp_punch_x = new_punch.x;

		int zoom_fov = cs::player::get_fov(local_player);
		float sensitivity = cs::engine::get_sensitivity();
		if (zoom_fov != 0 && zoom_fov != 90) {
			sensitivity = (zoom_fov / 90.0f) * sensitivity;
		}

		int final_angle_x = (int)((( -new_punch.y * 2.0f) / sensitivity) / -0.022f);
		int final_angle_y = (int)((( -new_punch.x * 2.0f) / sensitivity) / 0.022f);

		if (!m_aimbot_active)
			input::mouse_move(final_angle_x, final_angle_y);
		// cs::input::mouse_move(final_angle_x, final_angle_y);
	}
	m_rcs_old_punch = current_punch;
}

static BOOL features::triggerbot(C_Player local_player, C_Player target_player, C_Team target_team, cs::WEAPON_CLASS weapon_class)
{
	//
	// force head only for pistols and scout (class Pistol contains ssg08)
	//
	BOOL  head_only = 0;
	QWORD tick_count_skip = 0;

	if (weapon_class == cs::WEAPON_CLASS::Pistol)
	{
		tick_count_skip = random_number(15, 50);
		head_only = 1;
	}
	else if (weapon_class == cs::WEAPON_CLASS::Sniper)
	{
		tick_count_skip = random_number(30, 80);
	}
	else if (weapon_class == cs::WEAPON_CLASS::Rifle)
	{
		tick_count_skip = random_number(125, 170);
		head_only = 1;
	}
	else
	{
		//
		// invalid class
		//
		return 0;
	}


	//
	// mouse is still pressed
	//
	if (m_mouse_down_tick)
	{
		return head_only;
	}

	//
	// accurate shots only :D
	//
	vec2 punch = cs::player::get_vec_punch(local_player);
	if (punch.x < -0.08f)
	{
		return head_only;
	}

	BOOL found = 0;

	//
	// for snipers we can try use incross
	//
	if (head_only == 0)
	{
		if (cs::player::get_crosshair_id(local_player) == cs::player::get_player_id(target_player))
		{
			found = 1;
		}
	}

	//
	// enemy is not visible, force headonly for any gun
	//
	if (!found)
	{
		if (!cs::player::is_visible(local_player, target_player))
		{
			head_only = 1;
		}
	}

	//
	// manually check if aiming at head
	//
	if (!found)
	{

		typedef struct {
			int   bone;
			float radius;
			vec3  min;
			vec3  max;
		} HITBOX;

		HITBOX hitbox_list[2] = {

			{8, 3.900000f, {-1.100000f, 1.400000f,  0.100000f},  {3.000000f,  0.800000f, 0.100000f}},
			{8, 2.800000f, {-0.200000f, 1.100000f,  0.000000f},  {3.600000f,  0.100000f, 0.000000f}}
		};

		matrix3x4_t matrix;
		if (!cs::player::get_bone_position(target_player, 8, &matrix))
		{
			return head_only;
		}

		vec2 viewangles_2 = cs::engine::get_viewangles();
		vec3 viewangles = vec3{ viewangles_2.x, viewangles_2.y, 0 };

		vec3 dir = math::vec_atd(viewangles);
		vec3 eye = cs::player::get_eye_position(local_player);

		int  team_num = cs::teams::get_team_num(target_team);

		team_num = team_num - 2;

		if (team_num < 0 || team_num > 1)
		{
			return head_only;
		}

		if (math::vec_min_max(eye, dir,
			math::vec_transform(hitbox_list[team_num].min, matrix),
			math::vec_transform(hitbox_list[team_num].max, matrix),
			hitbox_list[team_num].radius))
		{
			found = 1;
		}
	}

	if (found)
	{
		QWORD current_tick = cs::engine::get_current_tick();
		if (m_mouse_click_ticks < current_tick)
		{
			m_mouse_down_tick = current_tick;
			m_mouse_click_ticks = tick_count_skip + m_mouse_down_tick;
			input::mouse1_down();
		}
	}

	return head_only;
}

static void features::aimbot(C_Player local_player, C_Player target_player, cs::WEAPON_CLASS weapon_class, DWORD bullet_count, BOOL head_only)
{
	if (config::aimbot_visibility_check && !cs::player::is_visible(local_player, target_player))
	{
		return;
	}

	//
	// anti-shake for triggerbot (disables RCS from first 2 bullets)
	//
	if (head_only)
	{
		bullet_count = 609;
	}

	vec3 aimbot_angle = vec3{ 0, 0, 0 };
	if (weapon_class == cs::WEAPON_CLASS::Pistol || head_only)
	{
		matrix3x4_t matrix;
		if (!cs::player::get_bone_position(target_player, 8, &matrix))
		{
			return;
		}

		vec3 temp_pos{};
		temp_pos.x = matrix[0][3];
		temp_pos.y = matrix[1][3];
		temp_pos.z = matrix[2][3];

		aimbot_angle = get_target_angle(local_player, temp_pos, bullet_count);
	}
	else
	{
		BOOL best_target_found = 0;
		float best_fov = 360.0f;
		int bones[] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};

		//
		// this code is probably written drunk, but hey. it works
		//
		for (int j = 0; j < sizeof(bones) / sizeof(*bones); j++) {
			matrix3x4_t matrix;

			int bone_index = bones[j];

			if (bones[j] == 9)
				bone_index = 5;

			else if (bones[j] == 10)
				bone_index = 5;

			else if (bones[j] == 11)
				bone_index = 4;

			else if (bones[j] == 12)
				bone_index = 4;

			else if (bones[j] == 13)
				bone_index = 3;

			else if (bones[j] == 14)
				bone_index = 3;

			else if (bones[j] == 15)
				bone_index = 0;

			else if (bones[j] == 16)
				bone_index = 0;

			else if (bones[j] == 17)
				bone_index = 7;

			else if (bones[j] == 18)
				bone_index = 7;

			if (!cs::player::get_bone_position(target_player, bone_index, &matrix))
			{
				return;
			}


			vec3 temp_pos{};
			temp_pos.x = matrix[0][3];
			temp_pos.y = matrix[1][3];
			temp_pos.z = matrix[2][3];


			if (bones[j] == 9) {
				temp_pos.y -= -7.0f;
			}

			else if (bones[j] == 10) {
				temp_pos.y += -7.0f;
			}

			else if (bones[j] == 11) {
				temp_pos.y -= -7.0f;
			}

			else if (bones[j] == 12) {
				temp_pos.y += -5.0f;
			}

			else if (bones[j] == 13) {
				temp_pos.y -= -5.0f;
			}

			else if (bones[j] == 14) {
				temp_pos.y += -5.0f;
			}

			else if (bones[j] == 15) {
				temp_pos.y -= -5.0f;
			}

			else if (bones[j] == 16) {
				temp_pos.y += -5.0f;
			}

			else if (bones[j] == 17) {
				temp_pos.y -= -5.0f;
				temp_pos.z += -2.0f;
			}

			else if (bones[j] == 18) {
				temp_pos.y += -5.0f;
				temp_pos.z += -2.0f;
			}


			vec3 best_angle = get_target_angle(local_player, temp_pos, bullet_count);
			float fov = math::get_fov(cs::engine::get_viewangles(), *(vec3*)&best_angle);

			if (fov < best_fov)
			{
				best_fov = fov;
				aimbot_angle = best_angle;
				best_target_found = 1;
			}
		}

		if (best_target_found == 0)
		{
			return;
		}
	}


	float fov = math::get_fov(cs::engine::get_viewangles(), aimbot_angle);
	if (fov > config::aimbot_fov)
	{
		return;
	}

	int zoom_fov = cs::player::get_fov(local_player);
	float sensitivity = cs::engine::get_sensitivity();
	if (zoom_fov != 0 && zoom_fov != 90) {
		sensitivity = (zoom_fov / 90.0f) * sensitivity;
	}

	vec2 viewangles = cs::engine::get_viewangles();

	vec3 angles{};
	angles.x = viewangles.x - aimbot_angle.x;
	angles.y = viewangles.y - aimbot_angle.y;
	angles.z = 0;
	math::vec_clamp(&angles);

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

	//
	// we can only aim max 100 pixels / tick (higher smooth = higher fov)
	//
	if (qabs((int)sx) > 100)
		return;

	if (qabs((int)sy) > 100)
		return;

	DWORD current_tick = cs::engine::get_current_tick();
	if (current_tick - m_previous_tick > aim_ticks)
	{
		m_previous_tick = current_tick;
		input::mouse_move((int)sx, (int)sy);
		// cs::input::mouse_move((int)sx, (int)sy);
	}
}

static vec3 features::get_target_angle(C_Player local_player, vec3 position, DWORD bullet_count)
{
	vec3 eye_position = cs::player::get_eye_position(local_player);
	vec3 angle = position;

	angle.x = position.x - eye_position.x;
	angle.y = position.y - eye_position.y;
	angle.z = position.z - eye_position.z;

	math::vec_normalize(&angle);
	math::vec_angles(angle, &angle);

	vec2 current_punch = cs::player::get_vec_punch(local_player);
	DWORD current_shots_fired = cs::player::get_shots_fired(local_player);
	if (current_shots_fired > bullet_count)
	{
		//
		// https://www.bilibili.com/video/BV1Ma4y147TS
		//
		if (m_aimbot_old_shots_fired > 0 && current_shots_fired > m_aimbot_old_shots_fired)
		{
			if (m_aimbot_old_temp_punch_x < current_punch.x)
			{
				current_punch.x = m_aimbot_old_temp_punch_x;
			}
		}

		m_aimbot_old_shots_fired = current_shots_fired;
		m_aimbot_old_temp_punch_x = current_punch.x;


		angle.x -= current_punch.x * 2.0f;
		angle.y -= current_punch.y * 2.0f;
	}
	math::vec_clamp(&angle);
	return angle;
}

static C_Player features::get_best_target(C_Player local_player, DWORD bullet_count, C_Team* target_team)
{
	C_Player target_address = 0;
	float best_fov = 360.0f;

	int localplayer_id = cs::player::get_player_id(local_player);

	C_TeamList team_list = cs::teams::get_team_list();
	if (team_list == 0)
	{
		return 0;
	}

	BOOL ffa = cs::engine::is_gamemode_ffa();
	for (int team_id = 0; team_id < cs::teams::get_team_count(); team_id++) {
		C_Team team = cs::teams::get_team(team_list, team_id);
		if (team == 0)
		{
			continue;
		}

		int player_count = cs::teams::get_player_count(team);
		if (player_count == 0)
		{
			continue;
		}

		C_PlayerList player_list = cs::teams::get_player_list(team);
		if (player_list == 0)
		{
			continue;
		}

		if (!ffa && cs::teams::contains_player(player_list, player_count, localplayer_id))
		{
			continue;
		}

		for (int i = 0; i < player_count; i++)
		{
			C_Player entity_address = cs::teams::get_player(player_list, i);

			if (entity_address == local_player)
			{
				continue;
			}

			if (!cs::player::is_valid(entity_address))
			{
				continue;
			}

			if (config::aimbot_visibility_check && !cs::player::is_visible(local_player, entity_address))
			{
				continue;
			}

			matrix3x4_t matrix;
			if (!cs::player::get_bone_position(entity_address, 8, &matrix))
			{
				continue;
			}

			vec3 bonepos{};
			bonepos.x = matrix[0][3];
			bonepos.y = matrix[1][3];
			bonepos.z = matrix[2][3];

			vec3 best_angle = get_target_angle(local_player, bonepos, bullet_count);
			float fov = math::get_fov(cs::engine::get_viewangles(), *(vec3*)&best_angle);

			if (fov < best_fov)
			{
				best_fov = fov;
				target_address = entity_address;
				*target_team = team;
			}
		}
	}
	return target_address;
}

//
// not random really
//
static QWORD get_random_number()
{
	return cs::engine::get_current_tick();
}

