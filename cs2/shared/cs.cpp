#include "cs.h"

namespace cs
{
	static vm_handle game_handle = 0;

	//
	// interfaces
	//
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
	}

	namespace mouse
	{
		static QWORD sensitivity;
	}


	static BOOL initialize(void);


	//
	// get dll game objects
	//
	static QWORD get_interface(QWORD base, PCSTR name);
	QWORD get_interface_function(QWORD interface_address, DWORD index);
}

BOOL cs::running(void)
{
	return cs::initialize();
}

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
E0:
#ifdef DEBUG
		LOG("CS2 process not found\n");
#endif
		return 0;
	}


	//
	// initialize game addresses
	//
	interfaces::resource = get_interface(vm::get_module(game_handle, "engine2.dll"), "GameResourceServiceClientV0");
	if (interfaces::resource == 0)
	{
	E1:
		vm::close(game_handle);
		game_handle = 0;
		goto E0;
	}

	interfaces::entity = vm::read_i64(game_handle, interfaces::resource + 0x58);
	if (interfaces::entity == 0)
	{
		goto E1;
	}

	interfaces::cvar = get_interface(vm::get_module(game_handle, "tier0.dll"), "VEngineCvar0");
	interfaces::input = get_interface(vm::get_module(game_handle, "inputsystem.dll"), "InputSystemVersion0");
	/*
	optional code:
	interfaces::player = get_interface(vm::get_module(game_handle, "client.dll"), "Source2ClientPrediction0");
	interfaces::player = vm::get_relative_address(game_handle, get_interface_function(interfaces::player, 120) + 0x16, 3, 7);
	interfaces::player = vm::read_i64(game_handle, interfaces::player);
	*/
	interfaces::player = interfaces::entity + 0x10;
	direct::local_player = get_interface(vm::get_module(game_handle, "client.dll"), "Source2ClientPrediction0");
	direct::local_player = vm::get_relative_address(game_handle, get_interface_function(direct::local_player, 181) + 0xF0, 3, 7);

	direct::view_angles = get_interface(vm::get_module(game_handle, "client.dll"), "Source2Client0");
	direct::view_angles = vm::get_relative_address(game_handle, get_interface_function(direct::view_angles, 16), 3, 7);
	direct::view_angles = vm::read_i64(game_handle, direct::view_angles) + 0x4510;


	//
	// viewmatrix: 48 63 c2 48 8d 0d ? ? ? ? 48 c1
	//
	direct::view_matrix = vm::scan_pattern_direct(game_handle,
		vm::get_module(game_handle, "client.dll"), "\x48\x63\xc2\x48\x8d\x0d\x00\x00\x00\x00\x48\xc1", "xxxxxx????xx", 12);
	if (direct::view_matrix == 0)
	{
		goto E1;
	}

	direct::view_matrix = vm::get_relative_address(game_handle, direct::view_matrix + 0x03, 3, 7);
	mouse::sensitivity  = engine::get_convar("sensitivity");

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
	LOG("mouse::sensitivity     %llx\n", mouse::sensitivity);
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

float cs::mouse::get_sensitivity(void)
{
	return vm::read_float(game_handle, mouse::sensitivity + 0x40);
}

DWORD cs::player::get_health(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x32c);
}

DWORD cs::player::get_team_num(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x3bf);
}

DWORD cs::player::get_crosshair_id(QWORD player)
{
	return vm::read_i32(game_handle, player + 0x152c);
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

	QWORD name_length = strlen(name);

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

