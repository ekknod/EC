#include "apex.h"

namespace apex
{
	static vm_handle apex_handle       = 0;
	static BOOL      netvar_status     = 0;

	static QWORD     IClientEntityList = 0;
	static QWORD     C_BasePlayer      = 0;
	static QWORD     IInputSystem      = 0;
	static QWORD     m_GetAllClasses   = 0;
	static QWORD     sensitivity       = 0;

	static DWORD     m_dwBulletSpeed   = 0;
	static DWORD     m_dwBulletGravity = 0;

	static DWORD     m_dwMuzzle        = 0;
	static DWORD     m_dwVisibleTime   = 0;

	static int       m_iHealth         = 0;
	static int       m_iViewAngles     = 0;
	static int       m_bZooming        = 0;
	static int       m_lifeState       = 0;
	static int       m_iCameraAngles   = 0;
	static int       m_iTeamNum        = 0;
	static int       m_iName           = 0;
	static int       m_vecAbsOrigin    = 0;
	static int       m_iWeapon         = 0;
	static int       m_iBoneMatrix     = 0;
	static int       m_playerData      = 0;

	static BOOL initialize(void);
	static BOOL dump_netvars(QWORD GetAllClassesAddress);
	static int  dump_table(QWORD table, const char *name);
}

BOOL apex::running(void)
{
	return apex::initialize();
}

C_Entity apex::entity::get_entity(int index)
{
	index = index + 1;
	index = index << 0x5;
	return vm::read_i64(apex_handle, (index + IClientEntityList) - 0x280050);
}

C_Player apex::teams::get_local_player(void)
{
	return vm::read_i64(apex_handle, C_BasePlayer);
}

float apex::engine::get_sensitivity(void)
{
	return vm::read_float(apex_handle, sensitivity);
}

DWORD apex::engine::get_current_tick(void)
{
	return vm::read_i32(apex_handle, IInputSystem + 0xcd8);
}

BOOL apex::input::get_button_state(DWORD button)
{
	button = button + 1;
	DWORD a0 = vm::read_i32(apex_handle, IInputSystem + ((button >> 5) * 4) + 0xb0);
	return (a0 >> (button & 31)) & 1;
}

void apex::input::mouse_move(int x, int y)
{
	typedef struct
	{
		int x, y;
	} mouse_data;
	mouse_data data;

	data.x = (int)x;
	data.y = (int)y;
	vm::write(apex_handle, IInputSystem + 0x1DB0, &data, sizeof(data));
}

BOOL apex::player::is_valid(C_Player player_address)
{
	if (player_address == 0)
	{
		return 0;
	}

	if (vm::read_i64(apex_handle, player_address + m_iName) != 125780153691248)
	{
		return 0;
	}

	if (vm::read_i32(apex_handle, player_address + m_iHealth) < 1)
	{
		return 0;
	}

	return 1;
}

float apex::player::get_visible_time(C_Player player_address)
{
	return vm::read_float(apex_handle, player_address + m_dwVisibleTime);
}

int apex::player::get_team_id(C_Player player_address)
{
	return vm::read_i32(apex_handle, player_address + m_iTeamNum);
}

vec3 apex::player::get_muzzle(C_Player player_address)
{
	vec3 muzzle;
	if (!vm::read(apex_handle, player_address + m_dwMuzzle, &muzzle, sizeof(muzzle)))
	{
		return vec3 {0, 0, 0};
	}
	return muzzle;
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

vec3 apex::player::get_bone_position(C_Player player_address, int index)
{
	QWORD bonematrix = vm::read_i64(apex_handle, player_address + m_iBoneMatrix);
	if (bonematrix == 0)
	{
		return vec3 {0, 0, 0};
	}

	vec3 position;
	if (!vm::read(apex_handle, player_address + m_vecAbsOrigin, &position, sizeof(position)))
	{
		return vec3 {0, 0, 0};
	}

	matrix3x4 matrix;
	if (!vm::read(apex_handle, bonematrix + (0x30 * index), &matrix, sizeof(matrix3x4)))
	{
		return vec3 {0, 0, 0};
	}
	
	vec3 bonepos;
	bonepos.x = matrix.x + position.x;
	bonepos.y = matrix.y + position.y;
	bonepos.z = matrix.z + position.z;
	return bonepos;
}

vec3 apex::player::get_velocity(C_Player player_address)
{
	vec3 value;

	if (!vm::read(apex_handle, player_address + m_vecAbsOrigin - 0xC, &value, sizeof(value)))
	{
		return vec3 {0, 0, 0};
	}

	return value;
}

vec2 apex::player::get_viewangles(C_Player player_address)
{
	vec2 breath_angles;

	if (!vm::read(apex_handle, player_address + m_iViewAngles - 0x10, &breath_angles, sizeof(breath_angles)))
	{
		return vec2 {0, 0};
	}

	return breath_angles;
}

void apex::player::enable_glow(C_Player player_address)
{
	vm::write_i32(apex_handle, player_address + 0x2C4, 1512990053);
	vm::write_i32(apex_handle, player_address + 0x3c8, 1);
	vm::write_i32(apex_handle, player_address + 0x3d0, 2);
	vm::write_float(apex_handle, player_address + 0x1D0, 70.000f);
	vm::write_float(apex_handle, player_address + 0x1D4, 0.000f);
	vm::write_float(apex_handle, player_address + 0x1D8, 0.000f);
}

C_Weapon apex::player::get_weapon(C_Player player_address)
{
	DWORD weapon_id = vm::read_i32(apex_handle, player_address + m_iWeapon) & 0xFFFF;
	return entity::get_entity(weapon_id - 1);
}

float apex::weapon::get_bullet_speed(C_Weapon weapon_address)
{
	return vm::read_float(apex_handle, weapon_address + m_dwBulletSpeed);
}

float apex::weapon::get_bullet_gravity(C_Weapon weapon_address)
{
	return vm::read_float(apex_handle, weapon_address + m_dwBulletGravity);
}

float apex::weapon::get_zoom_fov(C_Weapon weapon_address)
{
	return vm::read_float(apex_handle, weapon_address + m_playerData + 0xb8);
}

static BOOL apex::initialize(void)
{
	QWORD apex_base = 0;
	PVOID apex_base_dump = 0;
	QWORD temp_address = 0;

	if (apex_handle)
	{
		if (vm::running(apex_handle))
		{
			return 1;
		}
		apex_handle = 0;
	}

	apex_handle = vm::open_process("r5apex.exe");
	if (!apex_handle)
	{
#ifdef DEBUG
		LOG("[-] r5apex.exe process not found\n");
#endif
		return 0;
	}

	apex_base = vm::get_module(apex_handle, 0);
	if (apex_base == 0)
	{
#ifdef DEBUG
		LOG("[-] r5apex.exe base address not found\n");
#endif
		goto cleanup;
	}


	apex_base_dump = vm::dump_module(apex_handle, apex_base, VM_MODULE_TYPE::CodeSectionsOnly);
	if (apex_base_dump == 0)
	{
#ifdef DEBUG
		LOG("[-] r5apex.exe base dump failed\n");
#endif
		goto cleanup;
	}

	IClientEntityList = vm::scan_pattern(apex_base_dump, "\x4C\x8B\x15\x00\x00\x00\x00\x33\xF6", "xxx????xx", 9);
	if (IClientEntityList == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IClientEntityList\n");
#endif
		goto cleanup2;
	}

	IClientEntityList = vm::get_relative_address(apex_handle, IClientEntityList, 3, 7);
	IClientEntityList = IClientEntityList + 0x08;
	if (IClientEntityList == (QWORD)0x08)
	{
#ifdef DEBUG
		LOG("[-] failed to find IClientEntityList\n");
#endif
		goto cleanup2;
	}

	C_BasePlayer = vm::scan_pattern(apex_base_dump, "\x89\x41\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0", "xxxxxx????xxx", 13);
	if (C_BasePlayer == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwLocalPlayer\n");
#endif
		goto cleanup2;
	}

	C_BasePlayer = C_BasePlayer + 0x03;
	C_BasePlayer = vm::get_relative_address(apex_handle, C_BasePlayer, 3, 7);
	if (C_BasePlayer == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwLocalPlayer\n");
#endif
		goto cleanup2;
	}

	IInputSystem = vm::scan_pattern(apex_base_dump,
		"\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x4C\x24\x20\xBA\x01\x00\x00\x00\xC7", "xxx????xxxxxxxxxxx", 18);

	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IInputSystem\n");
#endif
		goto cleanup2;
	}

	IInputSystem = vm::get_relative_address(apex_handle, IInputSystem, 3, 7);
	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IInputSystem\n");
#endif
		goto cleanup2;
	}

	IInputSystem = IInputSystem - 0x10;

	m_GetAllClasses = vm::scan_pattern(apex_base_dump,
		"\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x74\x24\x20", "xxx????xxxxxxxxxxxxxx", 21);

	if (m_GetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find m_GetAllClasses\n");
#endif
		goto cleanup2;
	}

	m_GetAllClasses = vm::get_relative_address(apex_handle, m_GetAllClasses, 3, 7);
	m_GetAllClasses = vm::read_i64(apex_handle, m_GetAllClasses);
	if (m_GetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find m_GetAllClasses\n");
#endif
		goto cleanup2;
	}

	sensitivity = vm::scan_pattern(apex_base_dump,
		"\x48\x8B\x05\x00\x00\x00\x00\xF3\x0F\x10\x3D\x00\x00\x00\x00\xF3\x0F\x10\x70\x68", "xxx????xxxx????xxxxx", 20);
	
	if (sensitivity == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find sensitivity\n");
#endif
		goto cleanup2;
	}

	sensitivity = vm::get_relative_address(apex_handle, sensitivity, 3, 7);
	sensitivity = vm::read_i64(apex_handle, sensitivity);
	if (sensitivity == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find sensitivity\n");
#endif
		goto cleanup2;
	}


	if (netvar_status == 0)
	{
		temp_address = vm::scan_pattern(apex_base_dump, "\x75\x0F\xF3\x44\x0F\x10\xBF", "xxxxxxx", 7);
		if (temp_address == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet speed / gravity\n");
	#endif
			goto cleanup2;
		}

		m_dwBulletGravity = vm::read_i32(apex_handle, temp_address + 0x02 + 0x05);
		m_dwBulletSpeed = vm::read_i32(apex_handle, temp_address - 0x6D + 0x04);

		if (m_dwBulletGravity == 0 || m_dwBulletSpeed == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet m_dwBulletGravity/m_dwBulletSpeed\n");
	#endif
			goto cleanup2;
		}


		temp_address = vm::scan_pattern(apex_base_dump,
			"\xF3\x0F\x10\x91\x00\x00\x00\x00\x48\x8D\x04\x40", "xxxx????xxxx", 12);

		if (temp_address == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet dwMuzzle\n");
	#endif
			goto cleanup2;
		}

		temp_address = temp_address + 0x04;
		m_dwMuzzle = vm::read_i32(apex_handle, temp_address) - 0x4;

		if (m_dwMuzzle == (DWORD)-0x4)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet dwMuzzle\n");
	#endif
			goto cleanup2;
		}

		temp_address = vm::scan_pattern(apex_base_dump,
			"\x48\x8B\xCE\x00\x00\x00\x00\x00\x84\xC0\x0F\x84\xBA\x00\x00\x00", "xxx?????xxxxxxxx", 16);
		
		if (temp_address == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find dwVisibleTime\n");
	#endif
			goto cleanup2;
		}

		temp_address = temp_address + 0x10;
		m_dwVisibleTime = vm::read_i32(apex_handle, temp_address + 0x4);
		if (m_dwVisibleTime == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find m_dwVisibleTime\n");
	#endif
			goto cleanup2;
		}
	}


	vm::free_module(apex_base_dump);
	apex_base_dump = 0;


	if (netvar_status == 0)
	{
		netvar_status = dump_netvars(m_GetAllClasses);

		if (netvar_status == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to get netvars\n");
	#endif
			goto cleanup;
		}
	}

#ifdef DEBUG
	LOG("[+] IClientEntityList: %lx\n", IClientEntityList - apex_base);
	LOG("[+] dwLocalPlayer: %lx\n", C_BasePlayer - apex_base);
	LOG("[+] IInputSystem: %lx\n", IInputSystem - apex_base);
	LOG("[+] m_GetAllClasses: %lx\n", m_GetAllClasses - apex_base);
	LOG("[+] sensitivity: %lx\n", sensitivity - apex_base);
	LOG("[+] dwBulletSpeed: %x\n", (DWORD)m_dwBulletSpeed);
	LOG("[+] dwBulletGravity: %x\n", (DWORD)m_dwBulletGravity);
	LOG("[+] dwMuzzle: %x\n", m_dwMuzzle);
	LOG("[+] dwVisibleTime: %x\n", m_dwVisibleTime);
	LOG("[+] m_iHealth: %x\n", m_iHealth);
	LOG("[+] m_iViewAngles: %x\n", m_iViewAngles);
	LOG("[+] m_bZooming: %x\n", m_bZooming);
	LOG("[+] m_lifeState: %x\n", m_lifeState);
	LOG("[+] m_iCameraAngles: %x\n", m_iCameraAngles);
	LOG("[+] m_iTeamNum: %x\n", m_iTeamNum);
	LOG("[+] m_iName: %x\n", m_iName);
	LOG("[+] m_vecAbsOrigin: %x\n", m_vecAbsOrigin);
	LOG("[+] m_iWeapon: %x\n", m_iWeapon);
	LOG("[+] m_iBoneMatrix: %x\n", m_iBoneMatrix);
	LOG("[+] r5apex.exe is running\n");
#endif

	return 1;
cleanup2:
	if (apex_base_dump)
	{
		vm::free_module(apex_base_dump);
		apex_base_dump = 0;
	}

cleanup:
	if (apex_handle)
	{
		vm::close(apex_handle);
		apex_handle = 0;
	}
	return 0;
}

static int apex::dump_table(QWORD table, const char *name)
{
	for (DWORD i = 0; i < vm::read_i32(apex_handle, table + 0x10); i++) {
		

		QWORD recv_prop = vm::read_i64(apex_handle, table + 0x8);
		if (!recv_prop) {
			continue;
		}

		recv_prop = vm::read_i64(apex_handle, recv_prop + 0x8 * i);
		char recv_prop_name[260];
		{
			QWORD name_ptr = vm::read_i64(apex_handle, recv_prop + 0x28);
			vm::read(apex_handle, name_ptr, recv_prop_name, 260);
		}

		if (!strcmpi_imp(recv_prop_name, name)) {
			return vm::read_i32(apex_handle, recv_prop + 0x4);
		}
	}
	return 0;
}

static BOOL apex::dump_netvars(QWORD GetAllClassesAddress)
{
	int counter = 0;
	QWORD entry = GetAllClassesAddress;
	while (entry) {
		QWORD recv_table = vm::read_i64(apex_handle, entry + 0x18);
		QWORD recv_name  = vm::read_i64(apex_handle, recv_table + 0x4C8);

		char name[260];
		vm::read( apex_handle, recv_name, name, 260 );
		
		if (!strcmpi_imp(name, "DT_Player")) {
			m_iHealth = dump_table(recv_table, "m_iHealth");
			if (m_iHealth == 0)
			{
				break;
			}

			m_iViewAngles = dump_table(recv_table, "m_ammoPoolCapacity");
			if (m_iViewAngles == 0)
			{
				break;
			}
			m_iViewAngles -= 0x14;

			m_bZooming = dump_table(recv_table, "m_bZooming");
			if (m_bZooming == 0)
			{
				break;
			}

			m_lifeState = dump_table(recv_table, "m_lifeState");
			if (m_lifeState == 0)
			{
				break;
			}

			m_iCameraAngles = dump_table(recv_table, "m_zoomFullStartTime");
			if (m_iCameraAngles == 0)
			{
				break;
			}
			m_iCameraAngles += 0x2EC;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseEntity")) {
			m_iTeamNum = dump_table(recv_table, "m_iTeamNum");
			if (m_iTeamNum == 0)
			{
				break;
			}
			m_iName = dump_table(recv_table, "m_iName");
			if (m_iName == 0)
			{
				break;
			}

			m_vecAbsOrigin = 0x014c;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseCombatCharacter")) {
			m_iWeapon = dump_table(recv_table, "m_latestPrimaryWeapons");
			if (m_iWeapon == 0)
			{
				break;
			}
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseAnimating")) {
			m_iBoneMatrix = dump_table(recv_table, "m_nForceBone");
			if (m_iBoneMatrix == 0)
			{
				break;
			}
			m_iBoneMatrix = m_iBoneMatrix + 0x50 - 0x8;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_WeaponX")) {
			m_playerData = dump_table(recv_table, "m_playerData");
			if (m_playerData == 0)
			{
				break;
			}
			counter++;
		}

		entry = vm::read_i64(apex_handle, entry + 0x20);
	}
	return counter == 5;
}

