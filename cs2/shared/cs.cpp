#include "cs.h"

namespace cs
{
	static vm_handle game_handle = 0;

	namespace interfaces
	{
		static QWORD resource;
		static QWORD entity;
		static QWORD cvar;
		static QWORD input;
		static QWORD player;
	}

	namespace direct
	{
		static QWORD local_player;
		static QWORD view_angles;
		static QWORD view_matrix;
		static DWORD button_state;
	}

	namespace convars
	{
		static QWORD sensitivity;
		static QWORD mp_teammates_are_enemies;
		static QWORD cl_crosshairalpha;
	}

	static BOOL initialize(void);
	static QWORD get_interface(QWORD base, PCSTR name);
	QWORD get_interface_function(QWORD interface_address, DWORD index);
}

BOOL cs::running(void)
{
	return cs::initialize();
}

/*
optional code:
interfaces::player = get_interface(vm::get_module(game_handle, "client.dll"), "Source2ClientPrediction0");
interfaces::player = vm::get_relative_address(game_handle, get_interface_function(interfaces::player, 120) + 0x16, 3, 7);
interfaces::player = vm::read_i64(game_handle, interfaces::player);
*/


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

static BOOL cs::initialize(void)
{
	if (game_handle)
	{
		if (vm::running(game_handle))
		{
			return 1;
		}
		game_handle = 0;
	}

	game_handle = vm::open_process_ex("cs2.exe", "client.dll");
	if (!game_handle)
	{
#ifdef DEBUG
		LOG("CS2 process not found\n");
#endif
		return 0;
	}

	QWORD client_dll;
	JZ(client_dll = vm::get_module(game_handle, "client.dll"), E1);

	interfaces::resource = get_interface(vm::get_module(game_handle, "engine2.dll"), "GameResourceServiceClientV0");
	if (interfaces::resource == 0)
	{
	E1:
		vm::close(game_handle);
		game_handle = 0;
		return 0;
	}

	JZ(interfaces::entity   = vm::read_i64(game_handle, interfaces::resource + 0x58), E1);
	interfaces::player      = interfaces::entity + 0x10;

	JZ(interfaces::cvar     = get_interface(vm::get_module(game_handle, "tier0.dll"), "VEngineCvar0"), E1);
	JZ(interfaces::input    = get_interface(vm::get_module(game_handle, "inputsystem.dll"), "InputSystemVersion0"), E1);
	direct::button_state    = vm::read_i32(game_handle, get_interface_function(interfaces::input, 18) + 0xE + 3);

	JZ(direct::local_player = get_interface(client_dll, "Source2ClientPrediction0"), E1);
	JZ(direct::local_player = get_interface_function(direct::local_player, 181), E1);
	direct::local_player    = vm::get_relative_address(game_handle, direct::local_player + 0xF0, 3, 7);

	JZ(direct::view_angles  = get_interface(client_dll, "Source2Client0"), E1);
	direct::view_angles     = vm::get_relative_address(game_handle, get_interface_function(direct::view_angles, 16), 3, 7);
	JZ(direct::view_angles  = vm::read_i64(game_handle, direct::view_angles), E1);
	direct::view_angles     += 0x4510;

	//
	// viewmatrix: 48 63 c2 48 8d 0d ? ? ? ? 48 c1
	//
	JZ(direct::view_matrix = vm::scan_pattern_direct(game_handle, client_dll, "\x48\x63\xc2\x48\x8d\x0d\x00\x00\x00\x00\x48\xc1", "xxxxxx????xx", 12), E1);
	direct::view_matrix    = vm::get_relative_address(game_handle, direct::view_matrix + 0x03, 3, 7);

	JZ(convars::sensitivity              = engine::get_convar("sensitivity"), E1);
	JZ(convars::mp_teammates_are_enemies = engine::get_convar("mp_teammates_are_enemies"), E1);
	JZ(convars::cl_crosshairalpha        = engine::get_convar("cl_crosshairalpha"), E1);

	//
	// to-do schemas
	//


#ifdef DEBUG
	LOG("interfaces::cvar       %llx\n", interfaces::cvar);
	LOG("interfaces::input      %llx\n", interfaces::input);
	LOG("interfaces::resource   %llx\n", interfaces::resource);
	LOG("interfaces::entity     %llx\n", interfaces::entity);
	LOG("interfaces::player     %llx\n", interfaces::player);
	LOG("direct::local_player   %llx\n", direct::local_player);
	LOG("direct::view_angles    %llx\n", direct::view_angles);
	LOG("direct::view_matrix    %llx\n", direct::view_matrix);
	LOG("game is running\n");
#endif
	return 1;
}

QWORD cs::engine::get_convar(const char *name)
{
	QWORD objs = vm::read_i64(game_handle, interfaces::cvar + 64);

	for (DWORD i = 0; i < vm::read_i32(game_handle, interfaces::cvar + 160); i++)
	{
		QWORD entry = vm::read_i64(game_handle, objs + 16 * i);
		if (entry == 0)
		{
			break;
		}
		
		char convar_name[120]{};
		vm::read(game_handle, vm::read_i64(game_handle, entry + 0x00), convar_name, 120);

		if (!strcmpi_imp(convar_name, name))
		{
#ifdef DEBUG
			LOG("get_convar [%s %llx]\n", name, entry);
#endif
			return entry;
		}
	}
	return 0;
}

DWORD cs::engine::get_current_tick(void)
{
	static DWORD offset = vm::read_i32(game_handle, get_interface_function(interfaces::input, 16) + 2);
	return vm::read_i32(game_handle, interfaces::input + offset);
}

vec2 cs::engine::get_viewangles(void)
{
	vec2 va{};
	if (!vm::read(game_handle, direct::view_angles, &va, sizeof(va)))
	{
		va = {};
	}
	return va;
}

view_matrix_t cs::engine::get_viewmatrix(void)
{
	view_matrix_t matrix{};

	vm::read(game_handle, direct::view_matrix, &matrix, sizeof(matrix));

	return matrix;
}

QWORD cs::entity::get_local_player_controller(void)
{
	return vm::read_i64(game_handle, direct::local_player);
}

QWORD cs::entity::get_client_entity(int index)
{
	QWORD v2 = vm::read_i64(game_handle, interfaces::entity + 8i64 * (index >> 9) + 16);
	if (v2 == 0)
		return 0;

	return vm::read_i64(game_handle, (QWORD)(120i64 * (index & 0x1FF) + v2));
}

BOOL cs::entity::is_player(QWORD controller)
{
	QWORD vfunc = get_interface_function(controller, 144);
	if (vfunc == 0)
		return 0;

	DWORD value = vm::read_i32(game_handle, vfunc);
	//
	// mov al, 0x1
	// ret
	// cc
	//
	return value == 0xCCC301B0;
}

QWORD cs::entity::get_player(QWORD controller)
{
	//
	// 5DC offset might be changing in time
	// 8B 91 ? 05 00 00 83
	//
	DWORD v1 = vm::read_i32(game_handle, controller + 0x5DC);
	if (v1 == (DWORD)(-1))
	{
		return 0;
	}

	QWORD v2 = vm::read_i64(game_handle, interfaces::player + 8 * ((unsigned __int64)(v1 & 0x7FFF) >> 9));
	if (v2 == 0)
	{
		return 0;
	}
	return vm::read_i64(game_handle, v2 + 120i64 * (v1 & 0x1FF));
}

int cs::get_crosshairalpha(void)
{
	return vm::read_i32(game_handle, convars::cl_crosshairalpha + 0x40);
}

float cs::mouse::get_sensitivity(void)
{
	return vm::read_float(game_handle, convars::sensitivity + 0x40);
}

BOOL cs::gamemode::is_ffa(void)
{
	return vm::read_i32(game_handle, convars::mp_teammates_are_enemies + 0x40) == 1;
}

BOOL cs::input::is_button_down(DWORD button)
{
	DWORD v = vm::read_i32(game_handle, (QWORD)(interfaces::input + (((QWORD(button) >> 5) * 4) + direct::button_state)));
	return (v >> (button & 31)) & 1;
}

DWORD cs::player::get_health(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x32c);
}

DWORD cs::player::get_team_num(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x3bf);
}

int cs::player::get_life_state(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x330);
}

vec3 cs::player::get_origin(QWORD player)
{
	vec3 value{};
	if (!vm::read(game_handle, player + 0x1214, &value, sizeof(value)))
	{
		value = {};
	}
	return value;
}

float cs::player::get_vec_view(QWORD player)
{
	return vm::read_float(game_handle, player + 0xc48 + 8);
}

vec3 cs::player::get_eye_position(QWORD player)
{
	vec3 origin = get_origin(player);
	origin.z += get_vec_view(player);
	return origin;
}

DWORD cs::player::get_crosshair_id(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x152c);
}

DWORD cs::player::get_shots_fired(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x1404);
}

vec2 cs::player::get_eye_angles(QWORD player)
{
	vec2 value{};
	if (!vm::read(game_handle, player + 0x1500, &value, sizeof(value)))
	{
		value = {};
	}
	return value;
}

float cs::player::get_fov_multipler(QWORD player)
{
	return vm::read_float(game_handle, player + 0x120c);
}

vec2 cs::player::get_vec_punch(QWORD player)
{
	vec2 data{};

	QWORD aim_punch_cache[2]{};
	if (!vm::read(game_handle, player + 0x1728, &aim_punch_cache, sizeof(aim_punch_cache)))
	{
		return data;
	}

	DWORD aimpunch_size = ((DWORD*)&aim_punch_cache[0])[0];
	if (aimpunch_size < 1)
	{
		return data;
	}

	if (aimpunch_size == 129)
		aimpunch_size = 130;

	if (!vm::read(game_handle, aim_punch_cache[1] + ((aimpunch_size-1) * 12), &data, sizeof(data)))
	{
		data = {};
	}

	return data;
}

QWORD cs::player::get_node(QWORD player)
{
	return vm::read_i64(game_handle, player + 0x310);
}

BOOL cs::player::is_valid(QWORD player, QWORD node)
{
	if (player == 0)
	{
		return 0;
	}

	DWORD player_health = get_health(player);
	if (player_health < 1)
	{
		return 0;
	}

	if (player_health > 150)
	{
		return 0;
	}

	int player_lifestate = get_life_state(player);
	if (player_lifestate != 256)
	{
		return 0;
	}

	return node::get_dormant(node) == 0;
}

BOOLEAN cs::node::get_dormant(QWORD node)
{
	return vm::read_i8( game_handle, node + 0xe7 );
}

vec3 cs::node::get_origin(QWORD node)
{
	vec3 val{};
	if (!vm::read(game_handle, node + 0x80, &val, sizeof(val)))
	{
		val = {};
	}
	return val;
}

BOOL cs::node::get_bone_position(QWORD node, int index, vec3 *data)
{
	// get skeleton 
	// QWORD fun = get_interface_function(node, 8);
	//
	// get skeleton:
	// mov rax, rcx
	// ret
	//
	QWORD skeleton    = node;
	QWORD model_state = skeleton + 0x160;
	QWORD bone_data   = vm::read_i64(game_handle, model_state + 0x80);

	if (bone_data == 0)
		return 0;

	return vm::read(game_handle, bone_data + (index * 32), data, sizeof(vec3));
}

static QWORD cs::get_interface(QWORD base, PCSTR name)
{
	if (base == 0)
	{
		return 0;
	}

	QWORD export_address = vm::get_module_export(game_handle, base, "CreateInterface");
	if (export_address == 0)
	{
		return 0;
	}

	
	QWORD interface_entry = vm::read_i64(game_handle, (export_address + 7) + vm::read_i32(game_handle, export_address + 3));
	if (interface_entry == 0)
	{
		return 0;
	}

	QWORD name_length = strlen_imp(name);

	while (1)
	{
		char interface_name[120]{};
		vm::read(game_handle, 
			vm::read_i64(game_handle, interface_entry + 8),
			interface_name,
			name_length
			);
		
		if (!strcmpi_imp(interface_name, name))
		{
			//
			// lea    rax, [rip+0xXXXXXX]
			// ret
			//
			QWORD vfunc = vm::read_i64(game_handle, interface_entry);


			//
			// emulate vfunc call
			//
			QWORD addr = (vfunc + 7) + vm::read_i32(game_handle, vfunc + 3);

			return addr;
		}

		interface_entry = vm::read_i64(game_handle, interface_entry + 16);
		if (interface_entry == 0)
			break;
	}
	return 0;
}

QWORD cs::get_interface_function(QWORD interface_address, DWORD index)
{
	return vm::read_i64(game_handle, vm::read_i64(game_handle, interface_address) + (index * 8));
}

