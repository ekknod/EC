#pragma warning(disable: 4200)

#include "./pcileech/leechcore.h"
#include "./pcileech/vmmdll.h"
#include "../vm.h"

#pragma comment(lib, "..\\..\\library\\vm_pcileech\\pcileech\\leechcore.lib")
#pragma comment(lib, "..\\..\\library\\vm_pcileech\\pcileech\\vmm.lib")


static VMM_HANDLE vmdll = 0;

static BOOL initialize(void)
{
	if (vmdll == 0)
	{
		LPSTR args[] = { (LPSTR)"", (LPSTR)"-device", (LPSTR)"FPGA" };
		vmdll = VMMDLL_Initialize(3, args);
	}

	return vmdll != 0;
}

BOOL vm::process_exists(PCSTR process_name)
{
	DWORD pid = 0;

	if (!initialize()) return 0;

	return VMMDLL_PidGetFromName(vmdll, (LPSTR)process_name, &pid);
}

vm_handle vm::open_process(PCSTR process_name)
{
	if (!initialize()) return 0;

	DWORD pid = 0;
	if (!VMMDLL_PidGetFromName(vmdll, (LPSTR)process_name, &pid))
	{
		return 0;
	}
	
	return (vm_handle)(QWORD)pid;
}

vm_handle vm::open_process_ex(PCSTR process_name, PCSTR dll_name)
{
	if (!initialize()) return 0;

	DWORD i, pid = 0;

	SIZE_T process_count = 0;
	if (!VMMDLL_PidList(vmdll, 0, &process_count))
	{
		return 0;
	}

	DWORD *pid_array = (DWORD*)malloc(process_count*4);

	if (!VMMDLL_PidList(vmdll, pid_array, &process_count))
	{
		goto E0;
	}


	for (i = 0; i < process_count; i++)
	{
		VMMDLL_MAP_MODULE *module_map = 0;
		if (!VMMDLL_Map_GetModuleU(vmdll, pid_array[i], &module_map, 0))
		{
			continue;
		}

		DWORD count=0;
		DWORD target_count = 2;

		if (process_name == 0)
		{
			target_count = 1;
		}

		for (DWORD j = 0; j < module_map->cMap; j++)
		{
			if (process_name && !_strcmpi(module_map->pMap[j].uszText, process_name))
			{
				count++;
			}

			else if (!_strcmpi(module_map->pMap[j].uszText, dll_name))
			{
				count++;
			}

			if (count == target_count)
			{
				break;
			}
		}

		if (count == target_count)
		{
			pid = pid_array[i];
		}

		VMMDLL_MemFree(module_map);

		if (pid != 0)
		{
			target_count++;
			break;
		}
	}
E0:
	free(pid_array);
	return (vm_handle)(QWORD)pid;
}

vm_handle vm::open_process_by_module_name(PCSTR dll_name)
{
	if (!initialize()) return 0;

	return open_process_ex(0, dll_name);
}

void vm::close(vm_handle process)
{
	UNREFERENCED_PARAMETER(process);
}

BOOL vm::running(vm_handle process)
{
	UNREFERENCED_PARAMETER(process);

	if (!initialize()) return 0;
	//
	// not implemented yet
	//

	return 1;
}

BOOL vm::read(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (!initialize()) return 0;

	return VMMDLL_MemReadEx(vmdll, (DWORD)(QWORD)process, address, (PBYTE)buffer, (DWORD)length, 0, VMMDLL_FLAG_NOCACHE);
}

BOOL vm::write(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (!initialize()) return 0;

	return VMMDLL_MemWrite(vmdll, (DWORD)(QWORD)process, address, (PBYTE)buffer, (DWORD)length);
}

QWORD vm::get_peb(vm_handle process)
{
	if (!initialize()) return 0;

	SIZE_T info_size = sizeof(VMMDLL_PROCESS_INFORMATION);
	VMMDLL_PROCESS_INFORMATION info{};
	info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
	if (!VMMDLL_ProcessGetInformation(vmdll, (DWORD)(QWORD)process, &info, &info_size))
	{
		return 0;
	}
	return info.win.vaPEB;
}

QWORD vm::get_wow64_process(vm_handle process)
{
	if (!initialize()) return 0;

	SIZE_T info_size = sizeof(VMMDLL_PROCESS_INFORMATION);
	VMMDLL_PROCESS_INFORMATION info{};
	info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
	info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
	if (!VMMDLL_ProcessGetInformation(vmdll, (DWORD)(QWORD)process, &info, &info_size))
	{
		return 0;
	}
	return info.win.vaPEB32;
}

PVOID vm::dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type)
{
	if (!initialize()) return 0;

	QWORD nt_header;
	DWORD image_size;
	BYTE* ret;

	if (base == 0)
	{
		return 0;
	}

	nt_header = (QWORD)read_i32(process, base + 0x03C) + base;
	if (nt_header == base)
	{
		return 0;
	}

	image_size = read_i32(process, nt_header + 0x050);
	if (image_size == 0)
	{
		return 0;
	}

#ifdef _KERNEL_MODE
	ret = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, (QWORD)(image_size + 16), 'ofnI');
#else
	ret = (BYTE*)malloc((QWORD)image_size + 16);
#endif
	if (ret == 0)
		return 0;

	*(QWORD*)(ret + 0) = base;
	*(QWORD*)(ret + 8) = image_size;
	ret += 16;

	DWORD headers_size = read_i32(process, nt_header + 0x54);
	read(process, base, ret, headers_size);

	WORD machine = read_i16(process, nt_header + 0x4);
	QWORD section_header = machine == 0x8664 ?
		nt_header + 0x0108 :
		nt_header + 0x00F8;


	for (WORD i = 0; i < read_i16(process, nt_header + 0x06); i++) {
		QWORD section = section_header + ((QWORD)i * 40);
		if (module_type == VM_MODULE_TYPE::CodeSectionsOnly)
		{
			DWORD section_characteristics = read_i32(process, section + 0x24);
			if (!(section_characteristics & 0x00000020))
				continue;
		}

		QWORD target_address = (QWORD)ret + (QWORD)read_i32(process, section + ((module_type == VM_MODULE_TYPE::Raw) ? 0x14 : 0x0C));
		QWORD virtual_address = base + (QWORD)read_i32(process, section + 0x0C);
		DWORD virtual_size = read_i32(process, section + 0x08);
		read(process, virtual_address, (PVOID)target_address, virtual_size);
	}

	return (PVOID)ret;
}

void vm::free_module(PVOID dumped_module)
{
	QWORD a0 = (QWORD)dumped_module;

	a0 -= 16;
#ifdef _KERNEL_MODE
	ExFreePoolWithTag((void*)a0, 'ofnI');
#else
	free((void*)a0);
#endif
}

