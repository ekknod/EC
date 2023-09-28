#include "cs.h"

namespace cs
{
	namespace input
	{
		static DWORD m_ButtonState   = 0;
		static DWORD m_nLastPollTick = 0;
		static DWORD m_mouseRawAccum = 0;
	}

	static vm_handle game_handle = 0;

	static BOOL initialize(void);


	//
	// get dll game objects
	//
	static QWORD get_interface(QWORD base, PCSTR name);
	QWORD get_interface_function(QWORD interface_address, DWORD index);

	//
	// get convar
	//
	static QWORD get_convar(PCSTR name);
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
#ifdef DEBUG
		LOG("CS2 process not found\n");
#endif
		return 0;
	}



	//
	// cvar + 0x30 = flags
	// cvar + 0x40 = value
	// cvar + 0x50 = default value
	//
	QWORD sensitivity = get_convar("sensitivity");


	LOG("sensitivity is: %f\n", vm::read_float(game_handle, sensitivity + 0x40));



	LOG("game is running\n");

	return 1;
}

static QWORD cs::get_interface(QWORD base, PCSTR name)
{
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

static QWORD cs::get_convar(PCSTR name)
{
	QWORD tier0 = vm::get_module(game_handle, "tier0.dll");
	if (tier0 == 0)
	{
		return 0;
	}

	QWORD engine_cvar = get_interface(tier0, "VEngineCvar0");
	if (engine_cvar == 0)
	{
		return 0;
	}

	QWORD objs = vm::read_i64(game_handle, engine_cvar + 64);

	for (DWORD i = 0; i < vm::read_i32(game_handle, engine_cvar + 160); i++)
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
	/*
	WORD  convar_id = vm::read_i16(game_handle, engine_cvar + 80);
	while (1)
	{
		QWORD entry = vm::read_i64(game_handle, objs + 16 * convar_id);
		
		char convar_name[120]{};
		vm::read(game_handle, vm::read_i64(game_handle, entry + 0x00), convar_name, 120);

		if (!strcmpi_imp(convar_name, name))
		{
		}

		convar_id = vm::read_i16(game_handle, objs + 16 * convar_id + 10);
		if (convar_id == 0xFFFF)
			break;
	}
	*/
}

