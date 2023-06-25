#include "../vm.h"
#include <intrin.h>

extern "C" {
	extern BOOLEAN ( *_PsGetProcessExitProcessCalled)(PEPROCESS);
	extern PVOID ( *_PsGetProcessPeb)(PEPROCESS);
	extern PVOID ( *_PsGetProcessWow64Process)(PEPROCESS);
	extern PVOID (*_ExAllocatePoolWithTag)(POOL_TYPE, SIZE_T, ULONG);
	extern VOID ( *_ExFreePoolWithTag)( PVOID, ULONG );

	extern PHYSICAL_ADDRESS
	(*_MmGetPhysicalAddress)(
	    _In_ PVOID BaseAddress
	    );

	extern BOOLEAN
	(*_MmIsAddressValid)(
	    _In_ PVOID VirtualAddress
	    );

	extern void *(*memcpy_ptr)(void *, void *, QWORD);
}

static vm_handle get_process_by_name(PCSTR process_name)
{
	UNREFERENCED_PARAMETER(process_name);
	return *(vm_handle*)(__readgsqword(0x188) + 0xB8);
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
	UNREFERENCED_PARAMETER(process_name);
	UNREFERENCED_PARAMETER(dll_name);
	return *(vm_handle*)(__readgsqword(0x188) + 0xB8);
}

vm_handle vm::open_process_by_module_name(PCSTR dll_name)
{
	UNREFERENCED_PARAMETER(dll_name);
	return *(vm_handle*)(__readgsqword(0x188) + 0xB8);
}

void vm::close(vm_handle process)
{
	UNREFERENCED_PARAMETER(process);
}

BOOL vm::running(vm_handle process)
{
	if (process == 0)
		return 0;
	return _PsGetProcessExitProcessCalled((PEPROCESS)process) == 0;
}

BOOL vm::read(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (!running(process))
	{
		return 0;
	}

	QWORD cr3 = *(QWORD*)((QWORD)process + 0x28);
	if (cr3 == 0)
	{
		return 0;
	}

	if (cr3 != __readcr3())
	{
		return 0;
	}

	if (address < (QWORD)0x10000)
	{
		return 0;
	}

	if ((address + length) > (ULONG_PTR)0x7FFFFFFEFFFF)
	{
		return 0;
	}

	if (_MmGetPhysicalAddress((PVOID)address).QuadPart == 0)
	{
		return 0;
	}
	
	if (!_MmIsAddressValid((PVOID)address))
	{
		return 0;
	}

	memcpy_ptr(buffer, (PVOID)address, length);
	return 1;
}

BOOL vm::write(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (!running(process))
	{
		return 0;
	}

	QWORD cr3 = *(QWORD*)((QWORD)process + 0x28);
	if (cr3 == 0)
	{
		return 0;
	}

	if (cr3 != __readcr3())
	{
		return 0;
	}

	if (address < (QWORD)0x10000)
	{
		return 0;
	}

	if (address + length > (ULONG_PTR)0x7FFFFFFEFFFF)
	{
		return 0;
	}

	if (_MmGetPhysicalAddress((PVOID)address).QuadPart == 0)
	{
		return 0;
	}
	
	if (!_MmIsAddressValid((PVOID)address))
	{
		return 0;
	}

	memcpy_ptr((void *)address, buffer, length);
	return 1;
}

QWORD vm::get_peb(vm_handle process)
{
	if (process == 0)
		return 0;
	return (QWORD)_PsGetProcessPeb((PEPROCESS)process);
}

QWORD vm::get_wow64_process(vm_handle process)
{
	if (process == 0)
		return 0;
	return (QWORD)_PsGetProcessWow64Process((PEPROCESS)process);
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
	ret = (BYTE*)_ExAllocatePoolWithTag(NonPagedPool, (QWORD)(image_size + 16), 'ofnI');
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
	_ExFreePoolWithTag((void*)a0, 'ofnI');
#else
	free((void*)a0);
#endif
}

