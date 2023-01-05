#include "../../../library/vm.h"
#include <intrin.h>

#define LOG(...) DbgPrintEx(76, 0, __VA_ARGS__)

extern "C" {
	extern PCSTR ( *_PsGetProcessImageFileName)(PEPROCESS);
	extern BOOLEAN ( *_PsGetProcessExitProcessCalled)(PEPROCESS);
	extern PVOID ( *_PsGetProcessPeb)(PEPROCESS);
	extern PVOID ( *_PsGetProcessWow64Process)(PEPROCESS);
	extern HANDLE ( *_PsGetProcessId)(_In_ PEPROCESS);
	extern PVOID ( *_MmMapIoSpace)(PHYSICAL_ADDRESS, SIZE_T, MEMORY_CACHING_TYPE);
	extern VOID ( *_MmUnmapIoSpace)(PVOID, SIZE_T);
	extern PEPROCESS _PsInitialSystemProcess;

	extern PVOID (*_ExAllocatePoolWithTag)(POOL_TYPE, SIZE_T, ULONG);
	extern VOID ( *_ExFreePoolWithTag)( PVOID, ULONG );

	extern QWORD g_system_previous_ms;

	extern __int64 (__fastcall *MiGetPteAddress)(unsigned __int64 a1);

	extern PHYSICAL_ADDRESS
	(*_MmGetPhysicalAddress)(
	    _In_ PVOID BaseAddress
	    );

	extern BOOLEAN
	(*_MmIsAddressValid)(
	    _In_ PVOID VirtualAddress
	    );

	extern ULONG
	(*_KeQueryTimeIncrement)(
	    VOID
	    );
}

#pragma warning (disable: 4201)

typedef union _pte
{
	QWORD value;
	struct
	{
		QWORD present : 1;          
		QWORD rw : 1;       
		QWORD user_supervisor : 1;   
		QWORD page_write_through : 1;
		QWORD page_cache : 1;
		QWORD accessed : 1;         
		QWORD dirty : 1;            
		QWORD access_type : 1;   
		QWORD global : 1;           
		QWORD ignore_2 : 3;
		QWORD pfn : 36;
		QWORD reserved : 4;
		QWORD ignore_3 : 7;
		QWORD pk : 4;  
		QWORD nx : 1; 
	};
} pte, * ppte;

static vm_handle get_process_by_name(PCSTR process_name)
{
	QWORD process;
	QWORD entry;
	PCSTR entry_name;

	DWORD gActiveProcessLink = *(DWORD*)((BYTE*)_PsGetProcessId + 3) + 8;
	process = (QWORD)_PsInitialSystemProcess;

	entry = process;
	do {
		if (_PsGetProcessExitProcessCalled((PEPROCESS)entry))
			goto L0;

		entry_name = _PsGetProcessImageFileName((PEPROCESS)entry);
		if (entry_name != 0 && entry_name[0] != 0)
		{
			if (strcmpi_imp(entry_name, process_name) == 0)
			{
				return (vm_handle)entry;
			}
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

static QWORD GetMilliSeconds()
{
#ifdef _KERNEL_MODE
	LARGE_INTEGER start_time;
	KeQueryTickCount(&start_time);
	QWORD start_time_in_msec = (QWORD)(start_time.QuadPart * _KeQueryTimeIncrement() / 10000);

	return start_time_in_msec;
#else
	return system_current_time_millis();
#endif
}

vm_handle vm::open_process_ex(PCSTR process_name, PCSTR dll_name)
{
	QWORD ms = GetMilliSeconds();

	//
	// wait 5 seconds before finding process again
	// optimization for useless process finds
	//
	if (ms - g_system_previous_ms < 5000)
	{
		return 0;
	}
	g_system_previous_ms = ms;


	QWORD process;
	QWORD entry;
	PCSTR entry_name;

	DWORD gActiveProcessLink = *(DWORD*)((BYTE*)_PsGetProcessId + 3) + 8;
	process = (QWORD)_PsInitialSystemProcess;
	entry = process;
	do {
		
		if (_PsGetProcessExitProcessCalled((PEPROCESS)entry))
			goto L0;

		entry_name = _PsGetProcessImageFileName((PEPROCESS)entry);
		if (entry_name != 0 && entry_name[0] != 0)
		{
			if (strcmpi_imp(entry_name, process_name) == 0)
			{
				if (get_module((vm_handle)entry, dll_name))
					return (vm_handle)entry;
			}
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

	DWORD gActiveProcessLink = *(DWORD*)((BYTE*)_PsGetProcessId + 3) + 8;
	process = (QWORD)_PsInitialSystemProcess;

	entry = process;
	do {
		if (_PsGetProcessExitProcessCalled((PEPROCESS)entry))
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
	return _PsGetProcessExitProcessCalled((PEPROCESS)process) == 0;
}

BOOLEAN memcpy_2XXX_physical(PVOID dest, PVOID src, QWORD length)
{
	if ((QWORD)src < (QWORD)g_memory_range_low)
	{
		return 0;
	}

	if (((QWORD)src + length) > g_memory_range_high)
	{
		return 0;
	}

	BOOLEAN ret = 0;
	{
		PVOID mapped = _MmMapIoSpace(*(PHYSICAL_ADDRESS*)&src, length, MmNonCached);
		if (mapped)
		{
			memcpy(dest, mapped, length);
			_MmUnmapIoSpace(mapped, length);
			ret = 1;
		}
	}
	return ret;
}

BOOLEAN memcpy_2XXX(PVOID dest, PVOID src, QWORD length)
{
	QWORD total_size = length;
	QWORD offset = 0;
	QWORD bytes_read=0;
	QWORD physical_address;
	QWORD current_size;
	DWORD page_count=0;

	while (total_size)
	{
		physical_address = _MmGetPhysicalAddress((PVOID)((QWORD)src + offset)).QuadPart;
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
		if (!memcpy_2XXX_physical((PVOID)((QWORD)dest + offset), (PVOID)physical_address, current_size))
		{
			break;
		}
		page_count++;
		bytes_read = current_size;
	E0:
		total_size -= bytes_read;
		offset += bytes_read;
	}
	return page_count != 0;
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

	ppte pte = (ppte)MiGetPteAddress(address);
	if (!pte)
	{
		return 0;
	}

	if (!pte->present)
	{
		return 0;
	}
	return memcpy_2XXX(buffer, (PVOID)address, length);
}

BOOLEAN memcpy_2XXX_physical_w(PVOID dest, PVOID src, QWORD length)
{
	if ((QWORD)dest < (QWORD)g_memory_range_low)
	{
		return 0;
	}

	if (((QWORD)dest + length) > g_memory_range_high)
	{
		return 0;
	}

	BOOLEAN ret = 0;
	{
		PVOID mapped = _MmMapIoSpace(*(PHYSICAL_ADDRESS*)&dest, length, MmNonCached);
		if (mapped)
		{
			memcpy(mapped, src, length);
			_MmUnmapIoSpace(mapped, length);
			ret = 1;
		}
	}
	return ret;
}

BOOLEAN memcpy_2XXX_w(PVOID dest, PVOID src, QWORD length)
{
	QWORD total_size = length;
	QWORD offset = 0;
	QWORD bytes_copy=0;
	QWORD physical_address;
	QWORD current_size;
	DWORD page_count=0;

	while (total_size)
	{
		physical_address = _MmGetPhysicalAddress((PVOID)((QWORD)dest + offset)).QuadPart;
		if (!physical_address)
		{
			if (total_size >= 0x1000)
			{
				bytes_copy = 0x1000;
			}
			else
			{
				bytes_copy = total_size;
			}
			goto E0;
		}

		current_size = min(0x1000 - (physical_address & 0xFFF), total_size);
		if (!memcpy_2XXX_physical_w((PVOID)physical_address, (PVOID)((QWORD)src + offset), current_size))
		{
			break;
		}
		page_count++;
		bytes_copy = current_size;
	E0:
		total_size -= bytes_copy;
		offset += bytes_copy;
	}
	return page_count != 0;
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

	ppte pte = (ppte)MiGetPteAddress(address);
	if (!pte)
	{
		return 0;
	}

	if (!pte->present)
	{
		return 0;
	}

	return memcpy_2XXX_w((void *)address, buffer, length);
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

