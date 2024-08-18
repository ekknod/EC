#include "apex.h"

namespace apex
{
	vm_handle game_handle = 0;

	namespace interfaces
	{
		static QWORD IClientEntityList = 0;
		static QWORD cvar = 0;
		static QWORD input = 0;
		static QWORD engine = 0;
		static QWORD window_info = 0;
	}

	namespace direct
	{
		static QWORD C_BasePlayer = 0;
		static QWORD view_render = 0;
		static QWORD view_matrix = 0;
		static QWORD camera_origin = 0;
	}

	namespace convars
	{
		static QWORD sensitivity = 0;
		static QWORD gamepad_aim_speed = 0;
	}

	namespace netvars
	{
		static int m_iHealth;
		static int m_iMaxHealth;
		static int m_iViewAngles;
		static int m_bZooming;
		static int m_lifeState;
		static int m_iCameraAngles;
		static int m_iTeamNum;
		static int m_iName;
		static int m_vecAbsOrigin;
		static int m_iWeapon;
		static int m_iBoneMatrix;
		static int m_playerData;
		static int visible_time;
		static int projectile_speed;
		static int projectile_gravity;
	}

	static BOOL  initialize(void);
	static BOOL  dump_netvars(QWORD classes);
	static int   get_netvar(QWORD table, const char *name);
}

BOOL apex::running(void)
{
	return apex::initialize();
}

QWORD apex::engine::get_convar(PCSTR name)
{
	int len = (int)strlen_imp(name);
	QWORD a0 = vm::read_i64(game_handle, interfaces::cvar + 0x50);
	while (1)
	{
		a0 = vm::read_i64(game_handle, a0 + 0x8);
		if (a0 == 0)
			break;

		char buffer[260]{};
		vm::read(game_handle, vm::read_i64(game_handle, a0 + 0x18), buffer, len);
		if (!strcmpi_imp(buffer, name))
		{
			return a0;
		}
	}
	return 0;
}

int apex::engine::get_current_tick(void)
{
	return vm::read_i32(game_handle, interfaces::input + 0xcd8);
}

view_matrix_t apex::engine::get_viewmatrix(void)
{
	view_matrix_t matrix{};

	QWORD view_render = vm::read_i64(game_handle, direct::view_render);
	if (!view_render)
	{
		return {};
	}

	QWORD view_matrix = vm::read_i64(game_handle, view_render + direct::view_matrix);
	if (!view_matrix)
	{
		return {};
	}

	vm::read(game_handle, view_matrix, &matrix, sizeof(matrix));
	
	return matrix;
}

QWORD apex::engine::get_hwnd(void)
{
	return vm::read_i64(game_handle, interfaces::input + 0x10);
}

BOOL apex::engine::get_window_info(WINDOW_INFO *info)
{
	typedef struct
	{
		int x, y, w, h, idk, dw, dh;
	} WININFO;

	// x,y are updated in real-time, will return 0.0 if it's in the center of the desktop
	// w, h, dw, dh are only updated when the game starts

	WININFO win{};
	if (!vm::read(game_handle, interfaces::window_info, &win, sizeof(win)))
	{
		win.x = 0;
		win.y = 0;
		win.w = 1920;
		win.h = 1080;
	}
	else
	{
		if (win.x == 0 && win.y == 0)
		{
			win.x = (win.dw - win.w) / 2;
			win.y = (win.dh - win.h) / 2;
		}
	}

	info->x = (float)win.x;
	info->y = (float)win.y;
	info->w = (float)win.w;
	info->h = (float)win.h;
	return 1;
}

QWORD apex::entity::get_local_player(void)
{
	return vm::read_i64(game_handle, direct::C_BasePlayer);
}

QWORD apex::entity::get_client_entity(int index)
{
	index = index + 1;
	index = index << 0x5;
	return vm::read_i64(game_handle, (index + interfaces::IClientEntityList));
}

float apex::mouse::get_sensitivity(void)
{
	return cvar::get_float(convars::sensitivity);
}

int apex::get_controller_sensitivity()
{
	return cvar::get_int(convars::gamepad_aim_speed) + 1;
}

float apex::cvar::get_float(QWORD cvar)
{
	return vm::read_float(game_handle, cvar + 0x60);
}

int apex::cvar::get_int(QWORD cvar)
{
	return vm::read_i32(game_handle, cvar + 0x64);
}

BOOL apex::input::is_button_down(DWORD button)
{
	button = button + 1;
	DWORD a0 = vm::read_i32(game_handle, interfaces::input + ((button >> 5) * 4) + 0xb0);
	return (a0 >> (button & 31)) & 1;
}

void apex::input::mouse_move(int x, int y)
{
	typedef struct
	{
		int x, y;
	} mouse_data;
	mouse_data data{};

	data.x = (int)x;
	data.y = (int)y;
	vm::write(game_handle, interfaces::input + 0x1EB0, &data, sizeof(data));
}

void apex::input::enable_input(BYTE state)
{
	vm::write_i8(game_handle, interfaces::input + 0x18, state);
}

BOOL apex::player::is_valid(QWORD player_address)
{
	if (player_address == 0)
	{
		return 0;
	}

	if (vm::read_i64(game_handle, player_address + netvars::m_iName) != 125780153691248)
	{
		return 0;
	}
	return 1;
}

BOOL apex::player::is_zoomed_in(QWORD player_address)
{
	return vm::read_i8(game_handle, player_address + netvars::m_bZooming);
}

float apex::player::get_visible_time(QWORD player_address)
{
	return vm::read_float(game_handle, player_address + netvars::visible_time);
}

int apex::player::get_team_num(QWORD player_address)
{
	return vm::read_i32(game_handle, player_address + netvars::m_iTeamNum);
}

int apex::player::get_health(QWORD player_address)
{
	return vm::read_i32(game_handle, player_address + netvars::m_iHealth);
}

int apex::player::get_max_health(QWORD player_address)
{
	return vm::read_i32(game_handle, player_address + netvars::m_iMaxHealth);
}

vec3 apex::player::get_origin(QWORD player_address)
{
	vec3 muzzle{};

	if (!vm::read(game_handle, player_address + netvars::m_vecAbsOrigin, &muzzle, sizeof(muzzle)))
	{
		return {};
	}

	return muzzle;
}

vec3 apex::player::get_camera_origin(QWORD player_address)
{
	vec3 muzzle{};

	if (!vm::read(game_handle, player_address + direct::camera_origin, &muzzle, sizeof(muzzle)))
	{
		return {};
	}

	return muzzle;
}

BOOL apex::player::get_bone_position(QWORD player_address, int index, vec3 *vec_out)
{
	QWORD bonematrix = vm::read_i64(game_handle, player_address + netvars::m_iBoneMatrix);
	if (bonematrix == 0)
	{
		return 0;
	}

	vec3 position{};
	if (!vm::read(game_handle, player_address + netvars::m_vecAbsOrigin, &position, sizeof(position)))
	{
		return 0;
	}

	typedef struct
	{
		unsigned char pad1[0xCC];
		float x;
		unsigned char pad2[0xC];
		float y;
		unsigned char pad3[0xC];
		float z;
	} matrix3x4;

	matrix3x4 matrix{};
	if (!vm::read(game_handle, bonematrix + (0x30 * index), &matrix, sizeof(matrix)))
	{
		return 0;
	}
	vec_out->x = matrix.x + position.x;
	vec_out->y = matrix.y + position.y;
	vec_out->z = matrix.z + position.z;
	return 1;
}

vec3 apex::player::get_velocity(QWORD player_address)
{
	vec3 velocity{};
	if (!vm::read(game_handle, player_address + netvars::m_vecAbsOrigin - 0xC, &velocity, sizeof(velocity)))
	{
		return {};
	}
	return velocity;
}

vec2 apex::player::get_viewangles(QWORD player_address)
{
	vec2 va{};
	if (!vm::read(game_handle, player_address + netvars::m_iViewAngles - 0x10, &va, sizeof(va)))
	{
		return {};
	}
	return va;
}

QWORD apex::player::get_weapon(QWORD player_address)
{
	DWORD weapon_id = vm::read_i32(game_handle, player_address + netvars::m_iWeapon) & 0xFFFF;
	return entity::get_client_entity(weapon_id - 1);
}

float apex::weapon::get_bullet_speed(QWORD weapon_address)
{
	return vm::read_float(game_handle, weapon_address + netvars::projectile_speed);
}

float apex::weapon::get_bullet_gravity(QWORD weapon_address)
{
	return vm::read_float(game_handle, weapon_address + netvars::projectile_gravity);
}

float apex::weapon::get_zoom_fov(QWORD weapon_address)
{
	return vm::read_float(game_handle, weapon_address + netvars::m_playerData + 0xb8);
}

#ifdef DEBUG

#define JZ(val,field) \
if ((val) == 0)  \
{ \
LOG(__FILE__ ": line %d\n", __LINE__); \
goto field; \
} \

#else

#define JZ(val,field) \
	if ((val) == 0) goto field;

#endif

static BOOL apex::initialize(void)
{
	if (game_handle)
	{
		if (vm::running(game_handle))
		{
			return 1;
		}
		game_handle = 0;
	}

	game_handle = vm::open_process("r5apex.exe");
	if (!game_handle)
	{
		LOG("Apex process not found\n");
		return 0;
	}

	PVOID apex_dump = 0;
	QWORD apex_base = vm::get_module(game_handle, 0);
	if (apex_base == 0)
	{
	E1:
		vm::close(game_handle);
		game_handle = 0;
		return 0;
	E2:
		vm::free_module(apex_dump);
		goto E1;
	}

	QWORD vtable_list     = 0;
	QWORD bullet_vars     = 0;
	QWORD visible_time    = 0;
	QWORD get_all_classes = 0;

	JZ(apex_dump = vm::dump_module(game_handle, apex_base, VM_MODULE_TYPE::CodeSectionsOnly), E1);
	JZ(vtable_list = vm::scan_pattern(apex_dump, "\x48\x0F\x44\xCE\x48\x89\x05", "xxxxxxx", 8), E2);
	JZ(bullet_vars = vm::scan_pattern(
		apex_dump,
		"\xF3\x0F\x10\x8E\xC4\x1E\x00\x00\xF3\x0F\x10\x86\xFC\x1C\x00\x00", "xxxxxxxxxxxxxxxx", 17), E2);
	JZ(visible_time = vm::scan_pattern(apex_dump, "\x74\x44\x48\x8B\x93\x00\x00\x00\x00\x48\x85\xD2", "xxxxx????xxx", 13), E2);
	JZ(interfaces::IClientEntityList = vm::scan_pattern(apex_dump, "\x48\x8D\x35\x00\x00\x00\x00\x48\x8B\xD9\x83\xF8\xFF\x74\x63\x0F\xB7\xC8", "xxx????xxxxxxxxxxx", 18), E2);
	JZ(direct::C_BasePlayer = vm::scan_pattern(apex_dump, "\x48\x89\x05\x00\x00\x00\x00\x48\x85\xC9\x74\x0D", "xxx????xxxxx", 13), E2);
	JZ(get_all_classes = vm::scan_pattern(apex_dump, "\x4C\x8B\xD8\xEB\x07\x4C\x8B\x1D", "xxxxxxxx", 9), E2);
	JZ(direct::view_render = vm::scan_pattern(apex_dump, "\x40\x53\x48\x83\xEC\x20\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\xDA\xBA\xFF\xFF\xFF\xFF",
		"xxxxxxxxx????xxxxxxxx", 22), E2);
	JZ(direct::view_matrix = vm::scan_pattern(apex_dump, "\x49\x8B\xB5\x00\x00\x00\x00\x49\x8B\x9D\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "xxx????xxx????x????", 19), E2);
	JZ(direct::camera_origin = vm::scan_pattern(apex_dump, "\x48\x8B\xF9\x0F\x2E\x89\x00\x00\x00\x00\x7A", "xxxxxx????x", 11), E2);
	JZ(interfaces::window_info = vm::scan_pattern(apex_dump, "\x89\x05\xCC\xCC\xCC\xCC\x32\xC0\x89\x0D", "xx????xxxx", 10), E2);

	vm::free_module(apex_dump);


	interfaces::IClientEntityList = vm::get_relative_address(game_handle, interfaces::IClientEntityList, 3, 7);
	direct::C_BasePlayer      = vm::get_relative_address(game_handle, direct::C_BasePlayer, 3, 7) + 0x08;
	get_all_classes               = vm::get_relative_address(game_handle, get_all_classes + 0x05, 3, 7);
	JZ(get_all_classes            = vm::read_i64(game_handle, get_all_classes), E1);
	direct::view_render           = vm::get_relative_address(game_handle, direct::view_render + 0x06, 3, 7);
	JZ(direct::view_matrix        = vm::read_i32(game_handle, direct::view_matrix + 3), E1);
	direct::camera_origin         = vm::read_i32(game_handle, direct::camera_origin + 6);
	interfaces::window_info       = vm::get_relative_address(game_handle, interfaces::window_info, 2, 6);
	JZ(dump_netvars(get_all_classes), E1);
	
	//
	// initialize vtable list interfaces
	//
	interfaces::input  = vm::get_relative_address(game_handle, vtable_list - 0x0007, 3, 7);
	interfaces::cvar   = vm::get_relative_address(game_handle, vtable_list - 0x011D, 3, 7);
	interfaces::engine = vm::get_relative_address(game_handle, vtable_list + 0x0035, 3, 7);

	//
	// initialize bullet information
	//
	netvars::projectile_speed   = vm::read_i32(game_handle, bullet_vars + 4);
	netvars::projectile_gravity = vm::read_i32(game_handle, bullet_vars + 4 + 0x08);
	// ???????????????????????? = vm::read_i32(game_handle, bullet_vars + 4 + 0x17);

	netvars::visible_time = vm::read_i32(game_handle, visible_time + 3 + 0x02);

	//
	// get convars
	//
	JZ(convars::sensitivity = engine::get_convar("mouse_sensitivity"), E1);
	JZ(convars::gamepad_aim_speed = engine::get_convar("gamepad_aim_speed"), E1);


	

	LOG("IClientEntityList:  %p\n", (PVOID)(interfaces::IClientEntityList - apex_base));
	LOG("C_BasePlayer:       %p\n", (PVOID)(direct::C_BasePlayer - apex_base));
	LOG("cvar:               %p\n", (PVOID)(interfaces::cvar - apex_base));
	LOG("input:              %p\n", (PVOID)(interfaces::input - apex_base));
	LOG("engine:             %p\n", (PVOID)(interfaces::engine - apex_base));
	LOG("window_info:        %p\n", (PVOID)(interfaces::window_info - apex_base));
	LOG("view_render:        %p\n", (PVOID)(direct::view_render - apex_base));
	LOG("view_matrix:        %p\n", (PVOID)direct::view_matrix);
	LOG("camera_origin:      %p\n", (PVOID)direct::camera_origin);


	LOG("m_iHealth:          %x\n", netvars::m_iHealth);
	LOG("m_iMaxHealth:       %x\n", netvars::m_iMaxHealth);
	LOG("m_iViewAngles:      %x\n", netvars::m_iViewAngles);
	LOG("m_bZooming:         %x\n", netvars::m_bZooming);
	LOG("m_lifeState:        %x\n", netvars::m_lifeState);
	LOG("m_iCameraAngles:    %x\n", netvars::m_iCameraAngles);
	LOG("m_iTeamNum:         %x\n", netvars::m_iTeamNum);
	LOG("m_iName:            %x\n", netvars::m_iName);
	LOG("m_vecAbsOrigin:     %x\n", netvars::m_vecAbsOrigin);
	LOG("m_iWeapon:          %x\n", netvars::m_iWeapon);
	LOG("m_iBoneMatrix:      %x\n", netvars::m_iBoneMatrix);
	LOG("m_playerData:       %x\n", netvars::m_playerData);
	LOG("visible_time:       %x\n", netvars::visible_time);
	LOG("projectile_speed:   %x\n", netvars::projectile_speed);
	LOG("projectile_gravity: %x\n", netvars::projectile_gravity);

	LOG("running\n");

	return 1;
}

static int apex::get_netvar(QWORD table, const char *name)
{
	for (DWORD i = 0; i < vm::read_i32(game_handle, table + 0x10); i++)
	{
		QWORD recv_prop = vm::read_i64(game_handle, table + 0x8);
		if (!recv_prop) {
			continue;
		}

		recv_prop = vm::read_i64(game_handle, recv_prop + 0x8 * i);
		char recv_prop_name[260]{};
		vm::read(game_handle, vm::read_i64(game_handle, recv_prop + 0x28), recv_prop_name, 260);

		if (!strcmpi_imp(recv_prop_name, name)) {
			return vm::read_i32(game_handle, recv_prop + 0x4);
		}
	}
	return 0;
}

static BOOL apex::dump_netvars(QWORD classes)
{
	using namespace netvars;

	static int init_status = 0;

	if (init_status)
		return 1;

	int counter = 0;
	QWORD entry = classes;
	while (entry) {
		QWORD recv_table = vm::read_i64(game_handle, entry + 0x18);
		QWORD recv_name  = vm::read_i64(game_handle, recv_table + 0x4C8);

		char name[260]{};
		vm::read(game_handle, recv_name, name, 260 );
		
		if (!strcmpi_imp(name, "DT_Player"))
		{
			m_iHealth = get_netvar(recv_table, "m_iHealth");
			if (m_iHealth == 0)
			{
				break;
			}

			m_iMaxHealth = get_netvar(recv_table, "m_iMaxHealth");
			if (m_iMaxHealth == 0)
			{
				break;
			}

			m_iViewAngles = get_netvar(recv_table, "m_ammoPoolCapacity");
			if (m_iViewAngles == 0)
			{
				break;
			}
			m_iViewAngles -= 0x14;

			m_bZooming = get_netvar(recv_table, "m_bZooming");
			if (m_bZooming == 0)
			{
				break;
			}

			m_lifeState = get_netvar(recv_table, "m_lifeState");
			if (m_lifeState == 0)
			{
				break;
			}

			m_iCameraAngles = get_netvar(recv_table, "m_zoomFullStartTime");
			if (m_iCameraAngles == 0)
			{
				break;
			}
			m_iCameraAngles += 0x2EC;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseEntity")) {
			m_iTeamNum = get_netvar(recv_table, "m_iTeamNum");
			if (m_iTeamNum == 0)
			{
				break;
			}
			m_iName = get_netvar(recv_table, "m_iName");
			if (m_iName == 0)
			{
				break;
			}

			m_vecAbsOrigin = 0x017c;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseCombatCharacter")) {
			m_iWeapon = get_netvar(recv_table, "m_latestPrimaryWeapons");
			if (m_iWeapon == 0)
			{
				break;
			}
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseAnimating")) {
			m_iBoneMatrix = get_netvar(recv_table, "m_nForceBone");
			if (m_iBoneMatrix == 0)
			{
				break;
			}
			m_iBoneMatrix = m_iBoneMatrix + 0x50 - 0x8;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_WeaponX")) {
			m_playerData = get_netvar(recv_table, "m_playerData");
			if (m_playerData == 0)
			{
				break;
			}
			counter++;
		}

		entry = vm::read_i64(game_handle, entry + 0x20);
	}
	if (counter == 5)
	{
		init_status = 1;
		return 1;
	}
	return 0;
}

