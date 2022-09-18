#include "shared.h"

//
// private data. only available for features.cpp
//

namespace features
{
	vec2  m_rcs_old_punch { 0, 0 };
	float m_rcs_old_temp_punch_x = 0;
	DWORD m_rcs_old_shots_fired = 0;

	vec2  m_aimbot_old_punch { 0, 0 };
	float m_aimbot_old_temp_punch_x = 0;
	DWORD m_aimbot_old_shots_fired = 0;

	BOOL  m_previous_button = 0;
	cs::WEAPON_CLASS m_weapon_class = cs::WEAPON_CLASS::Invalid;
	C_Player m_previous_target = 0;
	C_Team   m_previous_team = 0;
	DWORD    m_previous_tick = 0;
	QWORD    m_mouse_down_ms = 0;
	QWORD    m_mouse_click_interval = 0;
	
	void reset(void)
	{
		m_previous_tick     = 0;
		m_previous_button   = 0;
		m_previous_target   = 0;
		m_previous_team     = 0;
		m_weapon_class      = cs::WEAPON_CLASS::Invalid;
	}

	void  standalone_rcs(C_Player local_player);
	void  triggerbot(C_Player local_player, C_Player target_player, C_Team target_team, cs::WEAPON_CLASS weapon_class);
	void  aimbot(C_Player local_player, C_Player target_player, cs::WEAPON_CLASS weapon_class, DWORD bullet_count, float aimbot_fov);
	vec3  get_target_angle(C_Player local_player, vec3 matrix, DWORD bullet_count);
	C_Player get_best_target(C_Player local_player, DWORD bullet_count, C_Team *target_team);
}

static INT64 GetRandomNumber();
static QWORD GetMilliSeconds();

INT64 random_int(int min, int max)
{
	return min + GetRandomNumber() % (max + 1 - min);
}

void features::run(void)
{
	//
	// release mouse click if needed to
	//
	if (m_mouse_down_ms)
	{
		QWORD ms = GetMilliSeconds();
		if (ms > m_mouse_click_interval)
		{
			input::mouse1_up();
			m_mouse_down_ms = 0;
			m_mouse_click_interval = ms + random_int(25, 50);
		}
	}

	C_Player local_player = cs::teams::get_local_player();
	if (local_player == 0)
	{
		reset();
		return;
	}

	BOOL mouse_1 = cs::input::get_button_state(107);
	BOOL aimbot_button     = cs::input::get_button_state(config::aimbot_button);
	BOOL triggerbot_button = cs::input::get_button_state(config::triggerbot_button);
	BOOL current_button    = aimbot_button + triggerbot_button + mouse_1;
	

	if (!m_previous_button && current_button)
	{
		m_weapon_class = cs::player::get_weapon_class(local_player);
	}

	m_previous_button = current_button;


	//
	// remove this in case some feature added what requires 24/7 action
	//
	if (current_button == FALSE)
	{
		return;
	}


	BOOL  b_can_aimbot = FALSE;
	DWORD rcs_bullet_count = 0;
	switch (m_weapon_class)
	{
		case cs::WEAPON_CLASS::Knife:
			b_can_aimbot = FALSE;
			break;

		case cs::WEAPON_CLASS::Grenade:
			b_can_aimbot = FALSE;
			break;

		case cs::WEAPON_CLASS::Pistol:
			b_can_aimbot = TRUE;
			rcs_bullet_count = 1;
			break;

		case cs::WEAPON_CLASS::Sniper:
			b_can_aimbot = TRUE;
			rcs_bullet_count = 2;
			break;

		case cs::WEAPON_CLASS::Rifle:
			b_can_aimbot = TRUE;
			rcs_bullet_count = 0;
			break;
		default:
			return;
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
				vec3 bonepos;
				bonepos.x = matrix[0][3];
				bonepos.y = matrix[1][3];
				bonepos.z = matrix[2][3];

				vec3 best_angle = get_target_angle(local_player, bonepos, rcs_bullet_count);
				float fov = math::get_fov(cs::engine::get_viewangles(), *(vec3 *)&best_angle);

				if (fov > config::aimbot_fov)
				{
					//
					// reset target
					//
					m_previous_target = 0;
				}
			} else {
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
		target_player = get_best_target(local_player, rcs_bullet_count, &target_team);
	}
	else
	{
		target_team = m_previous_team;
	}

	m_previous_target = target_player;
	m_previous_team   = target_team;

	//
	// target not found
	//
	if (target_player == 0)
	{
		return;
	}

	if (b_can_aimbot)
	{
		if (triggerbot_button)
		{
			triggerbot(local_player, target_player, target_team, m_weapon_class);
		}

		if (aimbot_button || triggerbot_button)
		{
			features::aimbot(local_player,
				target_player, m_weapon_class, rcs_bullet_count, config::aimbot_fov);
		}
	}
}

void features::standalone_rcs(C_Player local_player)
{
	
	vec2 current_punch = cs::player::get_vec_punch(local_player);
	DWORD shots_fired = (DWORD)cs::player::get_shots_fired(local_player);

	if (shots_fired > 1)
	{
		vec2 new_punch;
		new_punch.x = current_punch.x - m_rcs_old_punch.x;
		new_punch.y = current_punch.y - m_rcs_old_punch.y;

		if (shots_fired > m_rcs_old_shots_fired && m_rcs_old_shots_fired < new_punch.x)
		{
			new_punch.x = m_rcs_old_temp_punch_x;
		}

		m_rcs_old_shots_fired  = shots_fired;
		m_rcs_old_temp_punch_x = new_punch.x;

		
		vec2 viewangles = cs::engine::get_viewangles();
		float x = viewangles.x - new_punch.x * 2.0f;
		float y = viewangles.y - new_punch.y * 2.0f;

		int zoom_fov = cs::player::get_fov(local_player);
		float sensitivity = cs::engine::get_sensitivity();
		if (zoom_fov != 0 && zoom_fov != 90) {
			sensitivity = (zoom_fov / 90.0f) * sensitivity;
		}

		int final_angle_x =
			(int)(((y - viewangles.y) / sensitivity) / -0.022f);

		int final_angle_y =
			(int)(((x - viewangles.x) / sensitivity) / 0.022f);


		input::mouse_move(final_angle_x, final_angle_y);
		// cs::input::mouse_move(final_angle_x, final_angle_y);
	}
	m_rcs_old_punch = current_punch;
}

void features::triggerbot(C_Player local_player, C_Player target_player, C_Team target_team, cs::WEAPON_CLASS weapon_class)
{
	//
	// mouse is still pressed
	//
	if (m_mouse_down_ms)
	{
		return;
	}

	//
	// accurate shots only :D
	//
	vec2 punch = cs::player::get_vec_punch(local_player);
	if (punch.x < -0.08f)
	{
		return;
	}
	
	//
	// force head only for pistols and scout (class Pistol contains ssg08)
	//
	BOOL  head_only = FALSE;
	QWORD random_interval = 0;
	switch (weapon_class)
	{
	case cs::WEAPON_CLASS::Pistol:
		random_interval = random_int(15, 50);
		head_only = TRUE;
		break;
	case cs::WEAPON_CLASS::Sniper:
		random_interval = random_int(30, 80);
		break;
	case cs::WEAPON_CLASS::Rifle:
		random_interval = random_int(125, 170);
		break;
	default: return;
	}

	BOOL found = 0;

	//
	// for snipers we can try use incross
	//
	if (head_only == FALSE)
	{
		if (cs::player::get_crosshair_id(local_player) == cs::player::get_player_id(target_player))
		{
			found = 1;
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
		} HITBOX ;

		HITBOX hitbox_list[2] = {
				
			{8, 3.900000f, {-1.100000f, 1.400000f,  0.100000f},  {3.000000f,  0.800000f, 0.100000f}},
			{8, 2.800000f, {-0.200000f, 1.100000f,  0.000000f},  {3.600000f,  0.100000f, 0.000000f}}
		};

		matrix3x4_t matrix;
		if (!cs::player::get_bone_position(target_player, 8, &matrix))
		{
			return;
		}

		vec2 viewangles_2 = cs::engine::get_viewangles();
		vec3 viewangles   = vec3{ viewangles_2.x, viewangles_2.y, 0 };

		vec3 dir = math::vec_atd(viewangles);
		vec3 eye = cs::player::get_eye_position(local_player);

		int  team_num = cs::teams::get_team_num(target_team);

		team_num = team_num - 2;

		if (team_num < 0 || team_num > 1)
		{
			return;
		}

		if (math::vec_min_max(eye,dir,
			math::vec_transform(hitbox_list[team_num].min, matrix),
			math::vec_transform(hitbox_list[team_num].max, matrix),
			hitbox_list[team_num].radius))
		{
			found = 1;
		}
	}

	if (found)
	{
		QWORD ms = GetMilliSeconds();
		if (m_mouse_click_interval < ms)
		{
			m_mouse_down_ms = ms;
			m_mouse_click_interval = random_interval + m_mouse_down_ms;
			input::mouse1_down();
		}
	}
}

void features::aimbot(C_Player local_player, C_Player target_player, cs::WEAPON_CLASS weapon_class, DWORD bullet_count, float aimbot_fov)
{
	if (config::aimbot_visibility_check && !cs::player::is_visible(local_player, target_player))
	{
		return;
	}

	vec3 aimbot_angle = vec3{0, 0, 0};
	if (weapon_class == cs::WEAPON_CLASS::Pistol)
	{
		matrix3x4_t matrix;
		if (!cs::player::get_bone_position(target_player, 8, &matrix))
		{
			return;
		}

		vec3 temp_pos;
		temp_pos.x = matrix[0][3];
		temp_pos.y = matrix[1][3];
		temp_pos.z = matrix[2][3];

		aimbot_angle = get_target_angle(local_player, temp_pos, bullet_count);
	}
	else
	{
		BOOL best_target_found = 0;
		float best_fov = 360.0f;
		int bones[] = { 5, 4, 3, 0, 8 } ;
		for (int j = 0; j < sizeof(bones) / sizeof(*bones); j++ ) {
			matrix3x4_t matrix;
			if (!cs::player::get_bone_position(target_player, bones[j], &matrix))
			{
				return;
			}

			vec3 temp_pos;
			temp_pos.x = matrix[0][3];
			temp_pos.y = matrix[1][3];
			temp_pos.z = matrix[2][3];

			vec3 best_angle = get_target_angle(local_player, temp_pos, bullet_count);
			float fov = math::get_fov(cs::engine::get_viewangles(), *(vec3 *)&best_angle);

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

	if (fov > aimbot_fov)
	{
		return;
	}

	int zoom_fov = cs::player::get_fov(local_player);
	float sensitivity = cs::engine::get_sensitivity();
	if (zoom_fov != 0 && zoom_fov != 90) {
		sensitivity = (zoom_fov / 90.0f) * sensitivity;
	}

	vec2 viewangles = cs::engine::get_viewangles();

	vec3 angles;
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
	} else {
		sx = x;
		sy = y;
	}

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

vec3 features::get_target_angle(C_Player local_player, vec3 matrix, DWORD bullet_count)
{
	vec3 m;
	vec3 c;

	m = matrix;
	c = cs::player::get_eye_position(local_player);
	m.x -= c.x;
	m.y -= c.y;
	m.z -= c.z;
	c.x = m.x;
	c.y = m.y;
	c.z = m.z;

	math::vec_normalize(&c);
	math::vec_angles(c, &c);

	vec2 current_punch = cs::player::get_vec_punch(local_player);
	DWORD bct = cs::player::get_shots_fired(local_player);
	if (bct > bullet_count)
	{
		vec2 p = current_punch;

		if (m_aimbot_old_temp_punch_x < p.x && bct > m_aimbot_old_shots_fired) {
			p.x = m_aimbot_old_temp_punch_x;
		}

		c.x -= p.x * 2.0f;
		c.y -= p.y * 2.0f;
		
		m_aimbot_old_temp_punch_x = p.x;
		m_aimbot_old_shots_fired = bct;
	}
	m_aimbot_old_punch = current_punch;
	math::vec_clamp(&c);
	return c;
}

C_Player features::get_best_target(C_Player local_player, DWORD bullet_count, C_Team *target_team)
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

			vec3 bonepos;
			bonepos.x = matrix[0][3];
			bonepos.y = matrix[1][3];
			bonepos.z = matrix[2][3];

			vec3 best_angle = get_target_angle(local_player, bonepos, bullet_count);
			float fov = math::get_fov(cs::engine::get_viewangles(), *(vec3 *)&best_angle);

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



#ifndef _KERNEL_MODE
#include <sys/timeb.h>
static QWORD system_current_time_millis()
{
#if defined(_WIN32) || defined(_WIN64)
    struct _timeb timebuffer;
    _ftime64_s(&timebuffer);
    return (QWORD)(((timebuffer.time * 1000) + timebuffer.millitm));
#else
    struct timeb timebuffer;
    ftime(&timebuffer);
    return (uint64_t)(((timebuffer.time * 1000) + timebuffer.millitm));
#endif
}
#endif


#ifndef _KERNEL_MODE
#define KI_USER_SHARED_DATA 0x7FFE0000
typedef struct _KUSER_SHARED_DATA KUSER_SHARED_DATA;
#define SharedUserData ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)
#define SharedInterruptTime (KI_USER_SHARED_DATA + 0x8)
#define SharedSystemTime (KI_USER_SHARED_DATA + 0x14)
#define SharedTickCount (KI_USER_SHARED_DATA + 0x320)
#endif

#define KeQuerySystemTime(CurrentCount)                                     \
    *((PULONG64)(CurrentCount)) = *((volatile ULONG64 *)(SharedSystemTime))

#define KeQueryTickCount(CurrentCount)                                      \
    *((PULONG64)(CurrentCount)) = *((volatile ULONG64 *)(SharedTickCount))

#define KeQuerySharedInterruptTime(CurrentCount)                                      \
    *((PULONG64)(CurrentCount)) = *((volatile ULONG64 *)(SharedInterruptTime))


static ULONG RtlpRandomExAuxVarY = 0x7775fb16;

 #define LCG_A 0x7fffffed
 #define LCG_C 0x7fffffc3
 #define LCG_M MAXLONG

static ULONG RtlpRandomExConstantVector[128] =
 {
     0x4c8bc0aa, 0x51a0b326, 0x7112b3f1, 0x1b9ca4e1,  /*   0 */
     0x735fc311, 0x6fe48580, 0x320a1743, 0x494045ca,  /*   4 */
     0x103ad1c5, 0x4ba26e25, 0x62f1d304, 0x280d5677,  /*   8 */
     0x070294ee, 0x7acef21a, 0x62a407d5, 0x2dd36af5,  /*  12 */
     0x194f0f95, 0x1f21d7b4, 0x307cfd67, 0x66b9311e,  /*  16 */
     0x60415a8a, 0x5b264785, 0x3c28b0e4, 0x08faded7,  /*  20 */
     0x556175ce, 0x29c44179, 0x666f23c9, 0x65c057d8,  /*  24 */
     0x72b97abc, 0x7c3be3d0, 0x478e1753, 0x3074449b,  /*  28 */
     0x675ee842, 0x53f4c2de, 0x44d58949, 0x6426cf59,  /*  32 */
     0x111e9c29, 0x3aba68b9, 0x242a3a09, 0x50ddb118,  /*  36 */
     0x7f8bdafb, 0x089ebf23, 0x5c37d02a, 0x27db8ca6,  /*  40 */
     0x0ab48f72, 0x34995a4e, 0x189e4bfa, 0x2c405c36,  /*  44 */
     0x373927c1, 0x66c20c71, 0x5f991360, 0x67a38fa3,  /*  48 */
     0x4edc56aa, 0x25a59126, 0x34b639f2, 0x1679b2ce,  /*  52 */
     0x54f7ba7a, 0x319d28b5, 0x5155fa54, 0x769e6b87,  /*  56 */
     0x323e04be, 0x4565a5aa, 0x5974b425, 0x5c56a104,  /*  60 */
     0x25920c78, 0x362912dc, 0x7af3996f, 0x5feb9c87,  /*  64 */
     0x618361bf, 0x433fbe97, 0x0244da8e, 0x54e3c739,  /*  68 */
     0x33183689, 0x3533f398, 0x0d24eb7c, 0x06428590,  /*  72 */
     0x09101613, 0x53ce5c5a, 0x47af2515, 0x2e003f35,  /*  76 */
     0x15fb4ed5, 0x5e5925f4, 0x7f622ea7, 0x0bb6895f,  /*  80 */
     0x2173cdb6, 0x0467bb41, 0x2c4d19f1, 0x364712e1,  /*  84 */
     0x78b99911, 0x0a39a380, 0x3db8dd44, 0x6b4793b8,  /*  88 */
     0x09b0091c, 0x47ef52b0, 0x293cdcb3, 0x707b9e7b,  /*  92 */
     0x26d33ca3, 0x1e527faa, 0x3fe08625, 0x42560b04,  /*  96 */
     0x139d2e78, 0x0b558cdb, 0x28a68b82, 0x7ba3a51d,  /* 100 */
     0x52dabe9d, 0x59c3da1d, 0x5676cf9c, 0x152e972f,  /* 104 */
     0x6d8ac746, 0x5eb33591, 0x78b30601, 0x0ab68db0,  /* 108 */
     0x34737bb4, 0x1b6dd168, 0x76d9750b, 0x2ddc4ff2,  /* 112 */
     0x18a610cd, 0x2bacc08c, 0x422db55f, 0x169b89b6,  /* 116 */
     0x5274c742, 0x615535dd, 0x46ad005d, 0x4128f8dd,  /* 120 */
     0x29f5875c, 0x62c6f3ef, 0x2b3be507, 0x4a8e003f   /* 124 */
 };
 
static ULONG
 NTAPI
 random_num( IN OUT QWORD *Seed
     )
 {
    ULONG Rand;
    int Pos;
    Pos = RtlpRandomExAuxVarY & (sizeof(RtlpRandomExConstantVector) / sizeof(RtlpRandomExConstantVector[0]) - 1);
    RtlpRandomExAuxVarY = RtlpRandomExConstantVector[Pos];
    Rand = (  ((ULONG)*Seed) * LCG_A + LCG_C) % LCG_M;
    RtlpRandomExConstantVector[Pos] = Rand;
    *Seed = (QWORD)Rand;
    return Rand;
}

static INT64 GetRandomNumber()
{
	LARGE_INTEGER counter;
#ifdef _KERNEL_MODE
	KeQueryTickCount(&counter);
	QWORD ret = random_num((QWORD*)&counter.QuadPart);
	return ret;
#else
	KeQueryTickCount(&counter);
	QWORD ret = random_num((QWORD*)&counter.QuadPart);
	return ret;
#endif
}

static QWORD GetMilliSeconds()
{
#ifdef _KERNEL_MODE
	LARGE_INTEGER start_time;
	KeQueryTickCount(&start_time);
	QWORD start_time_in_msec = (QWORD)(start_time.QuadPart * KeQueryTimeIncrement() / 10000);

	return start_time_in_msec;
#else
	return system_current_time_millis();
#endif
}

