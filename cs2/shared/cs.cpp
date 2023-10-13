#include "cs.h"

namespace cs
{
	static vm_handle game_handle = 0;

	namespace sdl
	{
		static QWORD sdl_window;
		static QWORD sdl_mouse;
	}

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

	namespace netvars
	{
		static int m_flFOVSensitivityAdjust = 0;
		static int m_pGameSceneNode = 0;
		static int m_iHealth = 0;
		static int m_lifeState = 0;
		static int m_iTeamNum = 0;
		static int m_vecViewOffset = 0;
		static int m_vecOrigin = 0;
		static int m_bDormant = 0;
		static int m_hPawn = 0;
		static int m_modelState = 0;
		static int m_aimPunchCache = 0;
		static int m_iShotsFired = 0;
		static int m_angEyeAngles = 0;
		static int m_iIDEntIndex = 0;
		static int m_vOldOrigin = 0;
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

#ifdef __linux__
	#define PROCESS_NAME "cs2"
	#define CLIENT_DLL "libclient.so"
	#define ENGINE_DLL "libengine2.so"
	#define SDL3_DLL "libSDL3.so.0"
	#define TIER0_DLL "libtier0.so"
	#define INPUTSYSTEM_DLL "libinputsystem.so"

	#define ENTITY_OFF 0x50
	#define BUTTON_STATE_OFF 0x11
	#define VIEWANGLES_OFF 0x4520
#else
	#define PROCESS_NAME "cs2.exe"
	#define CLIENT_DLL "client.dll"
	#define ENGINE_DLL "engine2.dll"
	#define SDL3_DLL "SDL3.dll"
	#define TIER0_DLL "tier0.dll"
	#define INPUTSYSTEM_DLL "inputsystem.dll"

	#define ENTITY_OFF 0x58
	#define BUTTON_STATE_OFF 0xE
	#define VIEWANGLES_OFF 0x4510
#endif


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

	game_handle = vm::open_process_ex(PROCESS_NAME, CLIENT_DLL);
	if (!game_handle)
	{
#ifdef DEBUG
		LOG("CS2 process not found\n");
#endif
		return 0;
	}

	QWORD client_dll;
	JZ(client_dll = vm::get_module(game_handle, CLIENT_DLL), E1);

	QWORD sdl;
	JZ(sdl = vm::get_module(game_handle, SDL3_DLL), E1);

	interfaces::resource = get_interface(vm::get_module(game_handle, ENGINE_DLL), "GameResourceServiceClientV0");
	if (interfaces::resource == 0)
	{
	E1:
		vm::close(game_handle);
		game_handle = 0;
		return 0;
	}
	
#ifndef __linux__
	JZ(sdl::sdl_window   = vm::get_module_export(game_handle, sdl, "SDL_GetKeyboardFocus"), E1);
	sdl::sdl_window      = vm::get_relative_address(game_handle, sdl::sdl_window, 3, 7);
	JZ(sdl::sdl_window   = vm::read_i64(game_handle, sdl::sdl_window), E1);
	sdl::sdl_window      = vm::get_relative_address(game_handle, sdl::sdl_window, 3, 7);

	JZ(sdl::sdl_mouse     = vm::get_module_export(game_handle, sdl, "SDL_GetMouseState"), E1);
	sdl::sdl_mouse        = vm::get_relative_address(game_handle, sdl::sdl_mouse, 3, 7);
	JZ(sdl::sdl_mouse     = vm::read_i64(game_handle, sdl::sdl_mouse), E1);
	sdl::sdl_mouse        = vm::get_relative_address(game_handle, sdl::sdl_mouse + 0x10, 1, 5);
	sdl::sdl_mouse        = vm::get_relative_address(game_handle, sdl::sdl_mouse + 0x00, 3, 7);
#endif	

	JZ(interfaces::entity   = vm::read_i64(game_handle, interfaces::resource + ENTITY_OFF), E1);
	interfaces::player      = interfaces::entity + 0x10;

	JZ(interfaces::cvar     = get_interface(vm::get_module(game_handle, TIER0_DLL), "VEngineCvar0"), E1);
	JZ(interfaces::input    = get_interface(vm::get_module(game_handle, INPUTSYSTEM_DLL), "InputSystemVersion0"), E1);
	direct::button_state    = vm::read_i32(game_handle, get_interface_function(interfaces::input, 18) + BUTTON_STATE_OFF + 3);


#ifdef __linux__
	JZ(direct::local_player = vm::scan_pattern_direct(game_handle, client_dll, "\x48\x83\x3D\x00\x00\x00\x00\x00\x0F\x95\xC0\xC3", "xxx????xxxxx", 12), E1);
	direct::local_player    = vm::get_relative_address(game_handle, direct::local_player, 3, 8);
#else
	JZ(direct::local_player = get_interface(client_dll, "Source2ClientPrediction0"), E1);
	JZ(direct::local_player = get_interface_function(direct::local_player, 180), E1);
	direct::local_player    = vm::get_relative_address(game_handle, direct::local_player + 0xF0, 3, 7);
#endif
	JZ(direct::view_angles  = get_interface(client_dll, "Source2Client0"), E1);
	direct::view_angles     = vm::get_relative_address(game_handle, get_interface_function(direct::view_angles, 16), 3, 7);
	JZ(direct::view_angles  = vm::read_i64(game_handle, direct::view_angles), E1);
	direct::view_angles     += VIEWANGLES_OFF;

	//
	// viewmatrix: 48 63 c2 48 8d 0d ? ? ? ? 48 c1
	//
	#ifndef __linux__
	JZ(direct::view_matrix = vm::scan_pattern_direct(game_handle, client_dll, "\x48\x63\xc2\x48\x8d\x0d\x00\x00\x00\x00\x48\xc1", "xxxxxx????xx", 12), E1);
	direct::view_matrix    = vm::get_relative_address(game_handle, direct::view_matrix + 0x03, 3, 7);
	#endif

	JZ(convars::sensitivity              = engine::get_convar("sensitivity"), E1);
	JZ(convars::mp_teammates_are_enemies = engine::get_convar("mp_teammates_are_enemies"), E1);
	JZ(convars::cl_crosshairalpha        = engine::get_convar("cl_crosshairalpha"), E1);

	//
	// to-do schemas
	//

#ifdef DEBUG
	LOG("interfaces::cvar       %p\n", (void *)interfaces::cvar);
	LOG("interfaces::input      %p\n", (void *)interfaces::input);
	LOG("interfaces::resource   %p\n", (void *)interfaces::resource);
	LOG("interfaces::entity     %p\n", (void *)interfaces::entity);
	LOG("interfaces::player     %p\n", (void *)interfaces::player);
	LOG("direct::local_player   %p\n", (void *)direct::local_player);
	LOG("direct::view_angles    %p\n", (void *)direct::view_angles);
	LOG("direct::view_matrix    %p\n", (void *)direct::view_matrix);
	LOG("game is running\n");
#endif

	#ifdef __linux__
	PVOID dump_client = vm::dump_module(game_handle, client_dll, VM_MODULE_TYPE::Full);
	if (dump_client == 0)
	{
		goto E1;
	}

	QWORD base = *(QWORD*)((QWORD)dump_client - 16);
	QWORD size = *(QWORD*)((QWORD)dump_client - 8);
	for (QWORD i = size - sizeof(QWORD); i--;)
	{
		QWORD entry    = ((QWORD)dump_client + i);
		
		BOOL network_enable = 0;
		{
			QWORD ptr_to_name = *(QWORD*)(entry);
			if (ptr_to_name >= base && ptr_to_name <= (base + size))
			{
				ptr_to_name = *(QWORD*)((ptr_to_name - base) + (QWORD)dump_client);
				if (ptr_to_name >= base && ptr_to_name <= (base + size))
				{
					if (!strcmpi_imp((const char*)(ptr_to_name - base) + (QWORD)dump_client, "MNetworkEnable"))
					{
						network_enable = 1;
					}
				}
			}
		}

		QWORD name_ptr;
		if (network_enable == 0)
		{
			name_ptr = *(QWORD*)(entry);
			if (name_ptr < base)
			{
				continue;
			}
			if (name_ptr > (base + size))
			{
				continue;
			}
		}
		else
		{
			name_ptr = *(QWORD*)(entry + 0x08);
			if (name_ptr < base)
			{
				continue;
			}
			if (name_ptr > (base + size))
			{
				continue;
			}
		}

		PCSTR netvar_name = (PCSTR)((name_ptr - base) + (QWORD)dump_client);

		if (!netvars::m_iHealth && !strcmpi_imp(netvar_name, "m_iHealth") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_iHealth = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_iTeamNum && !strcmpi_imp(netvar_name, "m_iTeamNum") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_iTeamNum = *(int*)(entry + 0x08);
		}
		else if (!netvars::m_hPawn && !strcmpi_imp(netvar_name, "m_hPawn") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_hPawn = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_lifeState && !strcmpi_imp(netvar_name, "m_lifeState") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_lifeState = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_vecViewOffset && !strcmpi_imp(netvar_name, "m_vecViewOffset") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_vecViewOffset = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_aimPunchCache && !strcmpi_imp(netvar_name, "m_aimPunchCache") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_aimPunchCache = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_iShotsFired && !strcmpi_imp(netvar_name, "m_iShotsFired") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_iShotsFired = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_angEyeAngles && !strcmpi_imp(netvar_name, "m_angEyeAngles") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_angEyeAngles = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_flFOVSensitivityAdjust && !strcmpi_imp(netvar_name, "m_flFOVSensitivityAdjust"))
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08));
			netvars::m_flFOVSensitivityAdjust = *(int*)(entry + 0x08);
		}
		else if (!netvars::m_pGameSceneNode && !strcmpi_imp(netvar_name, "m_pGameSceneNode"))
		{
			LOG("%s, %x\n", netvar_name, *(WORD*)(entry + 0x10));
			netvars::m_pGameSceneNode = *(WORD*)(entry + 0x10);
		}
		else if (!netvars::m_bDormant && !strcmpi_imp(netvar_name, "m_bDormant"))
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08));
			netvars::m_bDormant = *(int*)(entry + 0x08);
		}
		else if (!netvars::m_modelState && !strcmpi_imp(netvar_name, "m_modelState"))
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08));
			netvars::m_modelState = *(int*)(entry + 0x08);
		}
		else if (!netvars::m_vecOrigin && !strcmpi_imp(netvar_name, "m_vecOrigin") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_vecOrigin = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_iIDEntIndex && !strcmpi_imp(netvar_name, "m_iIDEntIndex") && network_enable)
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x08 + 0x10));
			netvars::m_iIDEntIndex = *(int*)(entry + 0x08 + 0x10);
		}
		else if (!netvars::m_vOldOrigin && !strcmpi_imp(netvar_name, "m_vOldOrigin"))
		{
			LOG("%s, %x\n", netvar_name, *(int*)(entry + 0x10));
			netvars::m_vOldOrigin = *(int*)(entry + 0x10);
		}
	}
	vm::free_module(dump_client);
	#else


	PVOID dump_client = vm::dump_module(game_handle, client_dll, VM_MODULE_TYPE::Full);
	if (dump_client == 0)
	{
		goto E1;
	}

	{
		QWORD dos_header = (QWORD)dump_client;
		QWORD nt_header = (QWORD) * (DWORD*)(dos_header + 0x03C) + dos_header;
		WORD  machine = *(WORD*)(nt_header + 0x4);

		QWORD section_header = machine == 0x8664 ?
			nt_header + 0x0108 :
			nt_header + 0x00F8;

		for (WORD i = 0; i < *(WORD*)(nt_header + 0x06); i++) {
			QWORD section = section_header + ((QWORD)i * 40);
			DWORD section_characteristics = *(DWORD*)(section + 0x24);
			PCSTR name = (PCSTR)section;

			if (strcmpi_imp(name, ".data"))
			{
				continue;
			}
			
			DWORD section_size = *(DWORD*)(section + 0x08);
			DWORD section_addr = *(DWORD*)(section + 0x0C);

			for (DWORD j = section_addr; j < section_addr + section_size; j++)
			{
				BOOL network_enable = 0;
				{
					QWORD ptr_to_name = *(QWORD*)(dos_header + j);
					if (ptr_to_name >= client_dll && ptr_to_name <= (client_dll + *(QWORD*)(dos_header - 8)))
					{
						ptr_to_name = *(QWORD*)((ptr_to_name - client_dll) + dos_header);
						if (ptr_to_name >= client_dll && ptr_to_name <= (client_dll + *(QWORD*)(dos_header - 8)))
						{
							if (!strcmpi_imp((const char*)(ptr_to_name - client_dll) + dos_header, "MNetworkEnable"))
							{
								network_enable = 1;
							}
						}
					}
				}

				QWORD ptr_to_name;
				if (network_enable == 0)
				{
					ptr_to_name = *(QWORD*)(dos_header + j + 0x00);
					if (ptr_to_name < client_dll)
					{
						continue;
					}	
					if (ptr_to_name > (client_dll + *(QWORD*)(dos_header - 8)))
					{
						continue;
					}
				}
				else
				{
					ptr_to_name = *(QWORD*)(dos_header + j + 0x08);
					if (ptr_to_name < client_dll)
					{
						continue;
					}	
					if (ptr_to_name > (client_dll + *(QWORD*)(dos_header - 8)))
					{
						continue;
					}
				}


				PCSTR netvar_name = (PCSTR)((ptr_to_name - client_dll) + dos_header);

				if (!netvars::m_iHealth && !strcmpi_imp(netvar_name, "m_iHealth") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_iHealth = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_iTeamNum && !strcmpi_imp(netvar_name, "m_iTeamNum") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_iTeamNum = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_hPawn && !strcmpi_imp(netvar_name, "m_hPawn") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_hPawn = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_lifeState && !strcmpi_imp(netvar_name, "m_lifeState") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_lifeState = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_vecViewOffset && !strcmpi_imp(netvar_name, "m_vecViewOffset") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_vecViewOffset = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_aimPunchCache && !strcmpi_imp(netvar_name, "m_aimPunchCache") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_aimPunchCache = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_iShotsFired && !strcmpi_imp(netvar_name, "m_iShotsFired") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_iShotsFired = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_angEyeAngles && !strcmpi_imp(netvar_name, "m_angEyeAngles") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_angEyeAngles = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_flFOVSensitivityAdjust && !strcmpi_imp(netvar_name, "m_flFOVSensitivityAdjust"))
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x10));
					netvars::m_flFOVSensitivityAdjust = *(int*)(dos_header + j + 0x10);
				}
				else if (!netvars::m_pGameSceneNode && !strcmpi_imp(netvar_name, "m_pGameSceneNode"))
				{
					LOG("%s, %lx\n", netvar_name, *(WORD*)(dos_header + j + 0x10));
					netvars::m_pGameSceneNode = *(WORD*)(dos_header + j + 0x10);
				}
				else if (!netvars::m_bDormant && !strcmpi_imp(netvar_name, "m_bDormant"))
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x10));
					netvars::m_bDormant = *(int*)(dos_header + j + 0x10);
				}
				else if (!netvars::m_modelState && !strcmpi_imp(netvar_name, "m_modelState"))
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x10));
					netvars::m_modelState = *(int*)(dos_header + j + 0x10);
				}
				else if (!netvars::m_vecOrigin && !strcmpi_imp(netvar_name, "m_vecOrigin") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_vecOrigin = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_iIDEntIndex && !strcmpi_imp(netvar_name, "m_iIDEntIndex") && network_enable)
				{
					LOG("%s, %lx\n", netvar_name, *(int*)(dos_header + j + 0x08 + 0x10));
					netvars::m_iIDEntIndex = *(int*)(dos_header + j + 0x08 + 0x10);
				}
				else if (!netvars::m_vOldOrigin && !strcmpi_imp(netvar_name, "m_vOldOrigin"))
				{
					LOG("%s, %x\n", netvar_name, *(int*)(dos_header + j + 0x10));
					netvars::m_vOldOrigin = *(int*)(dos_header + j + 0x10);
				}
			}
		}
		vm::free_module(dump_client);
	}

	#endif

	JZ(netvars::m_flFOVSensitivityAdjust, E1);
	JZ(netvars::m_pGameSceneNode, E1);
	JZ(netvars::m_iHealth, E1);
	JZ(netvars::m_lifeState, E1);
	JZ(netvars::m_iTeamNum, E1);
	JZ(netvars::m_vecViewOffset, E1);
	JZ(netvars::m_vecOrigin, E1);
	JZ(netvars::m_bDormant, E1);
	JZ(netvars::m_hPawn, E1);
	JZ(netvars::m_modelState, E1);
	JZ(netvars::m_aimPunchCache, E1);
	JZ(netvars::m_iShotsFired, E1);
	JZ(netvars::m_angEyeAngles, E1);
	JZ(netvars::m_iIDEntIndex, E1);
	JZ(netvars::m_vOldOrigin, E1);

	return 1;
}

QWORD cs::sdl::get_window(void)
{
	return vm::read_i64(game_handle, sdl::sdl_window);
}

BOOL cs::sdl::get_window_info(QWORD window, WINDOW_INFO *info)
{
	typedef struct 
	{
		int x,y,w,h;
	} wvec4 ;

	wvec4 buffer{};
	if (!vm::read(game_handle, window + 0x20, &buffer, sizeof(buffer)))
		return 0;

	info->x = (float)buffer.x;
	info->y = (float)buffer.y;
	info->w = (float)buffer.w;
	info->h = (float)buffer.h;

	return 1;
}

QWORD cs::sdl::get_window_data(QWORD window)
{
	return vm::read_i64(game_handle, window + 0x110);
}

QWORD cs::sdl::get_hwnd(QWORD window_data)
{
	return vm::read_i64(game_handle, window_data + 0x08);
}

QWORD cs::sdl::get_hdc(QWORD window_data)
{
	return vm::read_i64(game_handle, window_data + 0x10);
}

QWORD cs::sdl::get_mouse(void)
{
	/*
	    0x60 float x;
	    0x64 float y;
	    0x68 float xdelta;
	    0x6C float ydelta;
	    0x70 float last_x, last_y;
	    0x78 SDL_bool has_position;
	    0x79 SDL_bool relative_mode;
	    0x7A SDL_bool relative_mode_warp;
	    0x7B SDL_bool relative_mode_warp_motion;
	    0x7C SDL_bool enable_normal_speed_scale;
	*/
	return sdl::sdl_mouse;
}

BOOL cs::sdl::get_mouse_button(QWORD mouse)
{
	DWORD i;
	DWORD buttonstate = 0;
	
	DWORD num_sources = vm::read_i32(game_handle, mouse + 0xD0);
	QWORD sources     = mouse + 0xD8;

	for (i = 0; i < num_sources; ++i)
	{
		QWORD mouse_source = vm::read_i64(game_handle, (sources + (i * 8)));
		DWORD mouse_id     = vm::read_i32(game_handle, mouse_source + 0x00);
		DWORD button_state = vm::read_i32(game_handle, mouse_source + 0x04);
				
		if (mouse_id != (DWORD)-1) {
			buttonstate |= button_state;
		}
	}
	return buttonstate;
}

QWORD cs::engine::get_convar(const char *name)
{
	QWORD objs = vm::read_i64(game_handle, interfaces::cvar + 64);

	QWORD convar_length = strlen_imp(name);

	for (DWORD i = 0; i < vm::read_i32(game_handle, interfaces::cvar + 160); i++)
	{
		QWORD entry = vm::read_i64(game_handle, objs + 16 * i);
		if (entry == 0)
		{
			break;
		}
		
		char convar_name[120]{};
		vm::read(game_handle, vm::read_i64(game_handle, entry + 0x00), convar_name, convar_length);
		convar_name[convar_length] = 0;

		if (!strcmpi_imp(convar_name, name))
		{
#ifdef DEBUG
			LOG("get_convar [%s %p]\n", convar_name, (void *)entry);
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
	QWORD v2 = vm::read_i64(game_handle, interfaces::entity + 8 * (index >> 9) + 16);
	if (v2 == 0)
		return 0;

	return vm::read_i64(game_handle, (QWORD)(120 * (index & 0x1FF) + v2));
}

BOOL cs::entity::is_player(QWORD controller)
{
#ifdef __linux__
	return 1;
#else
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
#endif
}

QWORD cs::entity::get_player(QWORD controller)
{
	//
	// 5DC offset might be changing in time
	// 8B 91 ? 05 00 00 83
	//
	DWORD v1 = vm::read_i32(game_handle, controller + netvars::m_hPawn);
	if (v1 == (DWORD)(-1))
	{
		return 0;
	}

	QWORD v2 = vm::read_i64(game_handle, interfaces::player + 8 * ((QWORD)(v1 & 0x7FFF) >> 9));
	if (v2 == 0)
	{
		return 0;
	}
	return vm::read_i64(game_handle, v2 + 120 * (v1 & 0x1FF));
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
	return vm::read_i32(game_handle, player + netvars::m_iHealth);
}

DWORD cs::player::get_team_num(QWORD player)
{
	return vm::read_i32(game_handle, player + netvars::m_iTeamNum);
}

int cs::player::get_life_state(QWORD player)
{
	return vm::read_i32(game_handle, player + netvars::m_lifeState);
}

vec3 cs::player::get_origin(QWORD player)
{
	vec3 value{};
	if (!vm::read(game_handle, player + netvars::m_vOldOrigin, &value, sizeof(value)))
	{
		value = {};
	}
	return value;
}

float cs::player::get_vec_view(QWORD player)
{
	return vm::read_float(game_handle, player + netvars::m_vecViewOffset + 8);
}

vec3 cs::player::get_eye_position(QWORD player)
{
	vec3 origin = get_origin(player);
	origin.z += get_vec_view(player);
	return origin;
}

DWORD cs::player::get_crosshair_id(QWORD player)
{
	return vm::read_i32(game_handle, player + netvars::m_iIDEntIndex);
}

DWORD cs::player::get_shots_fired(QWORD player)
{
	return vm::read_i32(game_handle, player + netvars::m_iShotsFired);
}

vec2 cs::player::get_eye_angles(QWORD player)
{
	vec2 value{};
	if (!vm::read(game_handle, player + netvars::m_angEyeAngles, &value, sizeof(value)))
	{
		value = {};
	}
	return value;
}

float cs::player::get_fov_multipler(QWORD player)
{
	return vm::read_float(game_handle, player + netvars::m_flFOVSensitivityAdjust);
}

vec2 cs::player::get_vec_punch(QWORD player)
{
	vec2 data{};

	QWORD aim_punch_cache[2]{};
	if (!vm::read(game_handle, player + netvars::m_aimPunchCache, &aim_punch_cache, sizeof(aim_punch_cache)))
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
	return vm::read_i64(game_handle, player + netvars::m_pGameSceneNode);
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
	return vm::read_i8( game_handle, node + netvars::m_bDormant );
}

vec3 cs::node::get_origin(QWORD node)
{
	vec3 val{};
	if (!vm::read(game_handle, node + netvars::m_vecOrigin, &val, sizeof(val)))
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
	QWORD model_state = skeleton + netvars::m_modelState;
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

	#ifdef __linux__
	export_address = vm::get_relative_address(game_handle, export_address, 1, 5);
	export_address += 0x10;
	#endif
	
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

		interface_name[name_length] = 0;
		
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

