#include "shared.h"

//
// private data. only available for features.cpp
//

namespace features
{
	//
	// global shared variables
	// can be used at any features.cpp function
	//
	cs::WEAPON_CLASS weapon_class;
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
		mouse_down_ms = 0;
		mouse_up_ms = 0;
		rcs_old_punch = {};
		aimbot_active = 0;
		aimbot_target = 0;
		aimbot_bone = 0;
		aimbot_ms = 0;
	}

	inline void update_settings(void);


	static void has_target_event(QWORD local_player, QWORD target_player, float fov, vec2 aim_punch, vec3 aimbot_angle, vec2 view_angle, vec3 bone);


	static vec3 get_target_angle(QWORD local_player, vec3 position, DWORD num_shots, vec2 aim_punch);
	static void get_best_target(BOOL ffa, QWORD local_controller, QWORD local_player, DWORD num_shots, vec2 aim_punch, QWORD* target);
	static void standalone_rcs(DWORD shots_fired, vec2 vec_punch, float sensitivity);
	static void esp(QWORD local_player, QWORD target_player, vec3 head);
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
	return min + cs::engine::get_current_ms() % (max + 1 - min);
}

inline void features::update_settings(void)
{
	int crosshair_alpha = cs::get_crosshairalpha();


	//
	// default global settings
	//
	config::rcs = 1;
	config::aimbot_enabled = 1;
	config::aimbot_multibone = 1;


#ifdef _KERNEL_MODE
	config::visuals_enabled = 1;
#else
	config::visuals_enabled = 1;
#endif


	switch (weapon_class)
	{
	case cs::WEAPON_CLASS::Knife:
	case cs::WEAPON_CLASS::Grenade:
		config::aimbot_enabled = 0;
		break;
	}


	switch (crosshair_alpha)
	{
	case 255:
		config::aimbot_button = 314;
		config::triggerbot_button = 317;
		config::aimbot_fov = 0.7f;
		config::aimbot_smooth = 2.0f;
		break;
	default:
		config::aimbot_button = 314;
		config::triggerbot_button = 317;
		config::aimbot_fov = 0.7f;
		config::aimbot_smooth = 2.0f;
		break;
	}
}

//
// this event is called from get best target/aimbot,
// when we have active target
//
static void features::has_target_event(QWORD local_player, QWORD target_player, float fov, vec2 aim_punch, vec3 aimbot_angle, vec2 view_angle, vec3 bone)
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
		if (weapon_class == cs::WEAPON_CLASS::Pistol)
		{
			accurate_shots_fl = -0.02f;
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

			vec3 dir = math::vec_atd(vec3{ view_angle.x, view_angle.y, 0 });
			vec3 eye = cs::player::get_eye_position(local_player);

			matrix3x4_t matrix{};
			matrix[0][3] = bone.x;
			matrix[1][3] = bone.y;
			matrix[2][3] = bone.z;

			if (math::vec_min_max(eye, dir,
				math::vec_transform(coll.min, matrix),
				math::vec_transform(coll.max, matrix),
				coll.radius))
			{
				DWORD current_ms = cs::engine::get_current_ms();
				if (current_ms > mouse_up_ms)
				{
					client::mouse1_down();
					mouse_up_ms = 0;
					mouse_down_ms = random_number(30, 50) + current_ms;
				}
			}
		}
	}
}

void features::run(void)
{
	//
	// reset triggerbot input
	//
	if (mouse_down_ms)
	{
		DWORD current_ms = cs::engine::get_current_ms();
		if (current_ms > mouse_down_ms)
		{
			client::mouse1_up();
			mouse_down_ms = 0;
			mouse_up_ms = random_number(30, 50) + current_ms;
		}
	}

	QWORD local_player_controller = cs::entity::get_local_player_controller();
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

	QWORD local_player = cs::entity::get_player(local_player_controller);

	if (local_player == 0)
	{
		goto NOT_INGAME;
	}

	weapon_class = cs::player::get_weapon_class(local_player);
	if (weapon_class == cs::WEAPON_CLASS::Invalid)
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
	b_triggerbot_button = cs::input::is_button_down(config::triggerbot_button);
	b_aimbot_button = cs::input::is_button_down(config::aimbot_button) | b_triggerbot_button;


	//
	// if we are holding triggerbot key, force head only
	//
	/*if (b_triggerbot_button)
	{
		config::aimbot_multibone = 0;
	}*/


	BOOL  ffa = cs::gamemode::is_ffa();
	DWORD num_shots = cs::player::get_shots_fired(local_player);
	vec2  aim_punch = cs::player::get_vec_punch(local_player);
	float sensitivity = cs::mouse::get_sensitivity() * cs::player::get_fov_multipler(local_player);

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
		aimbot_bone = 0;
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
		aimbot_bone = 0;
		aimbot_target = best_target;
	}
	else
	{
		if (!cs::player::is_valid(aimbot_target, cs::player::get_node(aimbot_target)))
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

	QWORD node = cs::player::get_node(aimbot_target);
	if (node == 0)
	{
		return;
	}

	vec2 view_angle = cs::player::get_viewangle(local_player);

	vec3  aimbot_angle{};
	vec3  aimbot_pos{};
	float aimbot_fov = 360.0f;

	if (config::aimbot_multibone)
	{
			for (DWORD i = 0; i < 9; i++)
			{
				vec3 pos{};
				if (!cs::node::get_bone_position(node, i, &pos))
				{
					continue;
				}

				vec3  angle = get_target_angle(local_player, pos, num_shots, aim_punch);
				float fov = math::get_fov(view_angle, angle);

				if (fov < aimbot_fov)
				{
					aimbot_fov = fov;
					aimbot_pos = pos;
					aimbot_angle = angle;
					aimbot_bone = i;
				}
			}
		
	}
	else
	{
		vec3 head{};
		if (!cs::node::get_bone_position(node, 6, &head))
		{
			return;
		}
		aimbot_pos = head;
		aimbot_angle = get_target_angle(local_player, head, num_shots, aim_punch);
		aimbot_fov = math::get_fov(view_angle, aimbot_angle);
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
		smooth_x = x;
		smooth_y = y;
		ms = 16;
	}

	DWORD current_ms = cs::engine::get_current_ms();
	if (current_ms - aimbot_ms > ms)
	{
		aimbot_ms = current_ms;
		client::mouse_move((int)smooth_x, (int)smooth_y);
	}
}

static vec3 features::get_target_angle(QWORD local_player, vec3 position, DWORD num_shots, vec2 aim_punch)
{
	vec3 eye_position = cs::node::get_origin(cs::player::get_node(local_player));
	eye_position.z += cs::player::get_vec_view(local_player);

	//
	// which one is better???
	//
	// vec3 eye_position = cs::player::get_eye_position(local_player);

	vec3 angle{};
	angle.x = position.x - eye_position.x;
	angle.y = position.y - eye_position.y;
	angle.z = position.z - eye_position.z;

	math::vec_normalize(&angle);
	math::vec_angles(angle, &angle);

	if (num_shots > 0)
	{
		if (weapon_class == cs::WEAPON_CLASS::Sniper)
			goto skip_recoil;

		if (weapon_class == cs::WEAPON_CLASS::Pistol)
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

static void features::get_best_target(BOOL ffa, QWORD local_controller, QWORD local_player, DWORD num_shots, vec2 aim_punch, QWORD* target)
{
	vec2 va = cs::player::get_viewangle(local_player);
	float best_fov = 360.0f;
	vec3  angle{};
	vec3  aimpos{};

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

		if (config::visuals_enabled)
		{
			esp(local_player, player, head);
		}

		vec3 best_angle = get_target_angle(local_player, head, num_shots, aim_punch);

		float fov = math::get_fov(va, *(vec3*)&best_angle);

		if (fov < best_fov)
		{
			best_fov = fov;
			*target = player;
			aimpos = head;
			angle = best_angle;
		}
	}

	if (best_fov != 360.0f)
	{
		event_state = 1;
		features::has_target_event(local_player, *target, best_fov, aim_punch, angle, va, aimpos);
	}
}

static void features::standalone_rcs(DWORD num_shots, vec2 vec_punch, float sensitivity)
{
	if (num_shots > 1)
	{
		float x = (vec_punch.x - rcs_old_punch.x) * -0.05f;
		float y = (vec_punch.y - rcs_old_punch.y) * -0.05f;

		int mouse_angle_x = (int)(((y * 2.0f) / sensitivity) / -0.022f);
		int mouse_angle_y = (int)(((x * 2.0f) / sensitivity) / 0.022f);

		if (!aimbot_active)
		{
			client::mouse_move(mouse_angle_x, mouse_angle_y);
		}
	}
	rcs_old_punch = vec_punch;
}

// Adicione esta função para desenhar um ponto na tela


// Função ESP modificada com head dot
static void features::esp(QWORD local_player, QWORD target_player, vec3 head)
{
	// Obter janela SDL
	QWORD sdl_window = cs::sdl::get_window();
	if (sdl_window == 0)
		return;

	// Obter informações da janela
	cs::WINDOW_INFO window{};
	if (!cs::sdl::get_window_info(sdl_window, &window))
		return;

	// Excluir parâmetros específicos para o Linux
#ifndef __linux__
	UNREFERENCED_PARAMETER(local_player);
	UNREFERENCED_PARAMETER(head);
#endif

	// Obter origem do jogador e ajustar origem superior
	vec3 origin = cs::player::get_origin(target_player);
	vec3 top_origin = origin;
	top_origin.z += 75.0f;

	// Variáveis para posições na tela
	vec3 screen_bottom, screen_top;
	view_matrix_t view_matrix = cs::engine::get_viewmatrix();

	// Obter tamanho da tela
	vec2 screen_size{};
	screen_size.x = (float)window.w;
	screen_size.y = (float)window.h;

	// Converter coordenadas mundiais para coordenadas de tela
	if (!math::world_to_screen(screen_size, origin, screen_bottom, view_matrix) || !math::world_to_screen(screen_size, top_origin, screen_top, view_matrix))
	{
		return;
	}

	// Ajustar cor da barra de vida se visuais estiverem ativados e o alvo for o alvo do aimbot
	if (config::visuals_enabled == 2)
	{
		if (aimbot_target == target_player)
		{
			// Lógica específica para o alvo do aimbot (se necessário)
		}
	}

	// Calcular dimensões e posição da caixa ESP
	int box_height = (int)(screen_bottom.y - screen_top.y);
	int box_width = box_height / 2;

	int x = (int)window.x + (int)(screen_top.x - box_width / 2);
	int y = (int)window.y + (int)(screen_top.y);

	// Verificar se a caixa está dentro dos limites da janela
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


	// Calcular dimensões e posição da barra de vida
	int barWidth = 2;  // Largura da barra de vida ajustada para 2 pixels
	int barHeight = box_height;

	// Calcular cor da barra de vida
	unsigned char healthBarColorR = 0;
	unsigned char healthBarColorG = 255;
	unsigned char healthBarColorB = 0;

	// Calcular altura da parte verde da barra de vida
	int greenBarHeight = static_cast<int>((float)cs::player::get_health(target_player) / 100.0f * barHeight);

	// Calcular altura da parte vermelha da barra de vida
	int redOverlayHeight = barHeight - greenBarHeight;

	// Cor da ESP box branca
	unsigned char espBoxColorR = 255;
	unsigned char espBoxColorG = 255;
	unsigned char espBoxColorB = 255;

	// Desenho específico da plataforma
#ifdef __linux__
	client::DrawRect((void*)0, x, y, box_width, box_height, espBoxColorR, espBoxColorG, espBoxColorB);
	client::DrawRect((void*)0, x + box_width, y, barWidth, greenBarHeight, healthBarColorR, healthBarColorG, healthBarColorB);
	client::DrawRect((void*)0, x + box_width, y + greenBarHeight, barWidth, redOverlayHeight, 255, 0, 0);  // Sobreposição vermelha
#else
	QWORD sdl_window_data = cs::sdl::get_window_data(sdl_window);
	if (sdl_window_data == 0)
		return;
	QWORD hwnd = cs::sdl::get_hwnd(sdl_window_data);
	client::DrawRect((void*)hwnd, x, y, box_width, box_height, espBoxColorR, espBoxColorG, espBoxColorB);
	client::DrawRect((void*)hwnd, x + box_width, y, barWidth, greenBarHeight, healthBarColorR, healthBarColorG, healthBarColorB);
	client::DrawRect((void*)hwnd, x + box_width, y + greenBarHeight, barWidth, redOverlayHeight, 255, 0, 0);  // Sobreposição vermelha
	// client::DrawHeadDot(reinterpret_cast<void*>(hwnd), head, screen_size, view_matrix);
#endif
}
