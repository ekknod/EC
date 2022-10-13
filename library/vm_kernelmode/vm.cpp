#include "../vm.h"

namespace pm
{
	static BOOL  read(QWORD address, PVOID buffer, QWORD length, QWORD* ret);
	static BOOL  write(QWORD address, PVOID buffer, QWORD length);
	static QWORD read_i64(QWORD address);
	static QWORD translate(QWORD dir, QWORD va);
}

extern "C" __declspec(dllimport) PCSTR PsGetProcessImageFileName(PEPROCESS);
extern "C" __declspec(dllimport) BOOLEAN PsGetProcessExitProcessCalled(PEPROCESS);
extern "C" __declspec(dllimport) PVOID PsGetProcessPeb(PEPROCESS);
extern "C" __declspec(dllimport) PVOID PsGetProcessWow64Process(PEPROCESS);

static vm_handle get_process_by_name(PCSTR process_name)
{
	QWORD process;
	QWORD entry;

	DWORD gActiveProcessLink = *(DWORD*)((BYTE*)PsGetProcessId + 3) + 8;
	process = (QWORD)PsInitialSystemProcess;

	entry = process;
	do {
		if (PsGetProcessExitProcessCalled((PEPROCESS)entry))
			goto L0;

		if (PsGetProcessImageFileName((PEPROCESS)entry) &&
			strcmpi_imp(PsGetProcessImageFileName((PEPROCESS)entry), process_name) == 0) {
			return (vm_handle)entry;
		}
	L0:
		entry = *(QWORD*)(entry + gActiveProcessLink) - gActiveProcessLink;
	} while (entry != process);

	return 0;
}

BOOL vm::process_exists(PCSTR process_name)
{
	return get_process_by_name(process_name) != 0;
}

vm_handle vm::open_process(PCSTR process_name)
{
	return get_process_by_name(process_name);
}

vm_handle vm::open_process_ex(PCSTR process_name, PCSTR dll_name)
{
	QWORD process;
	QWORD entry;

	DWORD gActiveProcessLink = *(DWORD*)((BYTE*)PsGetProcessId + 3) + 8;
	process = (QWORD)PsInitialSystemProcess;

	entry = process;
	do {
		if (PsGetProcessExitProcessCalled((PEPROCESS)entry))
			goto L0;

		if (PsGetProcessImageFileName((PEPROCESS)entry) &&
			strcmpi_imp(PsGetProcessImageFileName((PEPROCESS)entry), process_name) == 0) {

			if (get_module((vm_handle)entry, dll_name))
				return (vm_handle)entry;
		}
	L0:
		entry = *(QWORD*)(entry + gActiveProcessLink) - gActiveProcessLink;
	} while (entry != process);

	return 0;
}

vm_handle vm::open_process_by_module_name(PCSTR dll_name)
{
	QWORD process;
	QWORD entry;

	DWORD gActiveProcessLink = *(DWORD*)((BYTE*)PsGetProcessId + 3) + 8;
	process = (QWORD)PsInitialSystemProcess;

	entry = process;
	do {
		if (PsGetProcessExitProcessCalled((PEPROCESS)entry))
			goto L0;

		if (get_module((vm_handle)entry, dll_name))
			return (vm_handle)entry;
	L0:
		entry = *(QWORD*)(entry + gActiveProcessLink) - gActiveProcessLink;
	} while (entry != process);

	return 0;
}

void vm::close(vm_handle process)
{
	//
	// do nothing
	//
	UNREFERENCED_PARAMETER(process);
}

BOOL vm::running(vm_handle process)
{
	if (process == 0)
		return 0;
	return PsGetProcessExitProcessCalled((PEPROCESS)process) == 0;
}

BOOL vm::read(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (process == 0)
	{
		return 0;
	}

	QWORD cr3 = *(QWORD*)((QWORD)process + 0x28);
	if (cr3 == 0)
	{
		return 0;
	}

	QWORD total_size = length;
	QWORD offset = 0;
	QWORD bytes_read=0;
	QWORD physical_address;
	QWORD current_size;

	while (total_size)
	{
		physical_address = pm::translate(cr3, address + offset);
		if (!physical_address)
		{
			if (total_size >= 0x1000)
			{
				bytes_read = 0x1000;
			}
			else
			{
				bytes_read = total_size;
			}
			goto E0;
		}

		current_size = min(0x1000 - (physical_address & 0xFFF), total_size);
		if (!pm::read(physical_address, (PVOID)((QWORD)buffer + offset), current_size, &bytes_read))
		{
			break;
		}
	E0:
		total_size -= bytes_read;
		offset += bytes_read;
	}
	return 1;
}

BOOL vm::write(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (process == 0)
	{
		return 0;
	}

	QWORD cr3 = *(QWORD*)((QWORD)process + 0x28);
	if (cr3 == 0)
	{
		return 0;
	}

	QWORD total_size = length;
	QWORD offset = 0;
	QWORD bytes_write=0;

	QWORD physical_address;
	QWORD current_size;

	while (total_size) {
		physical_address = pm::translate(cr3, address + offset);
		if (!physical_address) {
			if (total_size >= 0x1000)
			{
				bytes_write = 0x1000;
			}
			else
			{
				bytes_write = total_size;
			}
			goto E0;
		}
		current_size = min(0x1000 - (physical_address & 0xFFF), total_size);
		if (!pm::write(physical_address, (PVOID)((QWORD)buffer + offset), current_size))
		{
			break;
		}
		bytes_write = current_size;
	E0:
		total_size -= bytes_write;
		offset += bytes_write;
	}
	return 1;
}

QWORD vm::get_peb(vm_handle process)
{
	if (process == 0)
		return 0;
	return (QWORD)PsGetProcessPeb((PEPROCESS)process);
}

QWORD vm::get_wow64_process(vm_handle process)
{
	if (process == 0)
		return 0;
	return (QWORD)PsGetProcessWow64Process((PEPROCESS)process);
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

static BYTE MM_COPY_BUFFER[0x1000];
static BOOL pm::read(QWORD address, PVOID buffer, QWORD length, QWORD* ret)
{
	if (address < (QWORD)g_memory_range_low)
	{
		return 0;
	}

	if (address + length > g_memory_range_high)
	{
		return 0;
	}

	if (length > 0x1000)
	{
		length = 0x1000;
	}

	MM_COPY_ADDRESS physical_address{};
	physical_address.PhysicalAddress.QuadPart = (LONGLONG)address;

	BOOL v = MmCopyMemory(MM_COPY_BUFFER, physical_address, length, MM_COPY_MEMORY_PHYSICAL, &length) == 0;
	if (v)
	{
		for (QWORD i = length; i--;)
		{
			((unsigned char*)buffer)[i] = ((unsigned char*)MM_COPY_BUFFER)[i];
		}
	}

	if (ret)
		*ret = length;

	return v;
}

static BOOL pm::write(QWORD address, PVOID buffer, QWORD length)
{
	if (address < (QWORD)g_memory_range_low)
	{
		return 0;
	}

	if (address + length > g_memory_range_high)
	{
		return 0;
	}

	PVOID va = MmMapIoSpace(*(PHYSICAL_ADDRESS*)&address, length, MEMORY_CACHING_TYPE::MmNonCached);
	if (va)
	{
		for (QWORD i = length; i--;)
		{
			((BYTE*)va)[i] = ((BYTE*)buffer)[i];
		}
		MmUnmapIoSpace(va, length);
		return 1;
	}
	return 0;
}

static QWORD pm::read_i64(QWORD address)
{
	QWORD result = 0;
	if (!read(address, &result, sizeof(result), 0))
	{
		return 0;
	}
	return result;
}

static QWORD pm::translate(QWORD dir, QWORD va)
{
	__int64 v2; // rax
	__int64 v3; // rax
	__int64 v5; // rax
	__int64 v6; // rax

	v2 = pm::read_i64(8 * ((va >> 39) & 0x1FF) + dir);
	if ( !v2 )
		return 0i64;

	if ( (v2 & 1) == 0 )
		return 0i64;

	v3 = pm::read_i64((v2 & 0xFFFFFFFFF000i64) + 8 * ((va >> 30) & 0x1FF));
	if ( !v3 || (v3 & 1) == 0 )
		return 0i64;

	if ( (v3 & 0x80u) != 0i64 )
		return (va & 0x3FFFFFFF) + (v3 & 0xFFFFFFFFF000i64);

	v5 = pm::read_i64((v3 & 0xFFFFFFFFF000i64) + 8 * ((va >> 21) & 0x1FF));
	if ( !v5 || (v5 & 1) == 0 )
		return 0i64;

	if ( (v5 & 0x80u) != 0i64 )
		return (va & 0x1FFFFF) + (v5 & 0xFFFFFFFFF000i64);

	v6 = pm::read_i64((v5 & 0xFFFFFFFFF000i64) + 8 * ((va >> 12) & 0x1FF));
	if ( v6 && (v6 & 1) != 0 )
		return (va & 0xFFF) + (v6 & 0xFFFFFFFFF000i64);

	return 0i64;
}

