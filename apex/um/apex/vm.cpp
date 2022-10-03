#include "../../../library/vm.h"
#include "../rx/rx.h"
#include <string.h>
#include <malloc.h>

/*
 *
 * this is very lazy compatibility layer and only works for r5apex.exe
 *
 */

static int get_process_id(PCSTR process_name)
{
	int process_id = 0;
	rx_handle snapshot = rx_create_snapshot(RX_SNAP_TYPE_PROCESS, 0);
	RX_PROCESS_ENTRY entry;

	while (rx_next_process(snapshot, &entry))
	{
		if (!strcmpi_imp(entry.name, process_name))
		{
			process_id = entry.pid;
			break;
		}
	}
	rx_close_handle(snapshot);
	return process_id;
}

BOOL vm::process_exists(PCSTR process_name)
{
	return get_process_id(process_name) != 0;
}

vm_handle vm::open_process(PCSTR process_name)
{
	if (!strcmpi_imp(process_name, "r5apex.exe"))
	{
		return open_process_ex("wine64-preloader", "easyanticheat_x64.dll");
	}

	int pid = get_process_id(process_name);
	if (pid == 0)
	{
		return 0;
	}

	return rx_open_process(pid, RX_ACCESS_MASK::RX_ALL_ACCESS);
}

vm_handle vm::open_process_ex(PCSTR process_name, PCSTR dll_name)
{
	rx_handle snapshot = rx_create_snapshot(RX_SNAP_TYPE_PROCESS, 0);
	RX_PROCESS_ENTRY entry;

	int pid = 0;
	while (rx_next_process(snapshot, &entry))
	{
		if (!strcmpi_imp(entry.name, process_name))
		{
			rx_handle snapshot_2 = rx_create_snapshot(RX_SNAP_TYPE_LIBRARY, entry.pid);
			
			RX_LIBRARY_ENTRY library_entry;
			while (rx_next_library(snapshot_2, &library_entry))
			{
				if (!strcmpi_imp(library_entry.name, dll_name))
				{
					pid = entry.pid;
					break;
				}
			}

			rx_close_handle(snapshot_2);

			//	
			// process found
			//
			if (pid != 0)
			{
				break;
			}
		}
	}
	rx_close_handle(snapshot);
	if (pid == 0)
	{
		return 0;
	}

	return rx_open_process(pid, RX_ACCESS_MASK::RX_ALL_ACCESS);
}

vm_handle vm::open_process_by_module_name(PCSTR dll_name)
{
	int pid = 0;
	rx_handle snapshot = rx_create_snapshot(RX_SNAP_TYPE_PROCESS, 0);
	RX_PROCESS_ENTRY entry;

	while (rx_next_process(snapshot, &entry))
	{
		rx_handle snapshot_2 = rx_create_snapshot(RX_SNAP_TYPE_LIBRARY, entry.pid);
		RX_LIBRARY_ENTRY library_entry;
		while (rx_next_library(snapshot_2, &library_entry))
		{
			if (!strcmpi_imp(library_entry.name, dll_name))
			{
				pid = entry.pid;
				break;
			}
		}
		rx_close_handle(snapshot_2);

		//
		// process found
		//
		if (pid != 0)
		{
			break;
		}
	}
	rx_close_handle(snapshot);

	if (pid == 0)
	{
		return 0;
	}
	return rx_open_process(pid, RX_ACCESS_MASK::RX_ALL_ACCESS);
}

void vm::close(vm_handle process)
{
	rx_close_handle(process);
}

BOOL vm::running(vm_handle process)
{
	return (BOOL)rx_process_exists(process);
}

BOOL vm::read(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	return rx_read_process(process, address, buffer, length) == (ssize_t)length;
}

BOOL vm::write(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	return rx_write_process(process, address, buffer, length) == (ssize_t)length;
}

QWORD vm::get_peb(vm_handle)
{
	return 0;
}

QWORD vm::get_wow64_process(vm_handle)
{
	return 0;
}

PVOID vm::dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type)
{
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

