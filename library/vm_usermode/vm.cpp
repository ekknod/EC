#include "../vm.h"
#include <TlHelp32.h>

#pragma comment(lib, "ntdll.lib")


extern "C" __kernel_entry NTSTATUS NtQueryInformationProcess(
	HANDLE           ProcessHandle,
	ULONG            ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength
);

BOOL vm::process_exists(PCSTR process_name)
{
	BOOL found = 0;

	PROCESSENTRY32 entry{};
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		if (!strcmpi_imp(entry.szExeFile, process_name))
		{
			found = 1;
			break;
		}
	}

	CloseHandle(snapshot);

	return found;
}

vm_handle vm::open_process(PCSTR process_name)
{
	vm_handle process_handle = 0;

	PROCESSENTRY32 entry{};
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		if (!strcmpi_imp(entry.szExeFile, process_name))
		{
			process_handle = OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);
			break;
		}
	}

	CloseHandle(snapshot);

	return process_handle;
}

vm_handle vm::open_process_ex(PCSTR process_name, PCSTR dll_name)
{
	vm_handle process_handle = 0;

	PROCESSENTRY32 entry{};
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		if (!strcmpi_imp(entry.szExeFile, process_name))
		{
			process_handle = OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);

			if (!process_handle)
			{
				continue;
			}

			if (get_module(process_handle, dll_name))
			{
				break;
			}

			CloseHandle(process_handle);
			process_handle = 0;
		}
	}

	CloseHandle(snapshot);

	return process_handle;
}

vm_handle vm::open_process_by_module_name(PCSTR dll_name)
{
	vm_handle process_handle = 0;

	PROCESSENTRY32 entry{};
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		process_handle = OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);

		if (!process_handle)
		{
			continue;
		}

		if (get_module(process_handle, dll_name))
		{
			break;
		}

		CloseHandle(process_handle);
		process_handle = 0;
	}

	CloseHandle(snapshot);

	return process_handle;
}

void vm::close(vm_handle process)
{
	if (process)
	{
		CloseHandle(process);
	}
}

BOOL vm::running(vm_handle process)
{
	if (!process)
		return 0;

	DWORD ret = 0;
	GetExitCodeProcess(process, &ret);

	return ret == STATUS_PENDING;
}

BOOL vm::read(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (process == 0)
		return 0;

	SIZE_T ret = 0;
	if (!ReadProcessMemory(process, (LPCVOID)address, buffer, length, &ret))
	{
		return 0;
	}
	return ret == length;
}

BOOL vm::write(vm_handle process, QWORD address, PVOID buffer, QWORD length)
{
	if (process == 0)
		return 0;

	SIZE_T ret = 0;
	if (!WriteProcessMemory(process, (LPVOID)address, buffer, length, &ret))
	{
		return 0;
	}
	return ret == length;
}

QWORD vm::get_peb(vm_handle process)
{
	QWORD peb[6]{};

	if (NtQueryInformationProcess(process, 0, &peb, 48, 0) != 0)
	{
		return 0;
	}

	return peb[1];
}

QWORD vm::get_wow64_process(vm_handle process)
{
	QWORD wow64_process = 0;

	if (process == 0)
		return wow64_process;

	if (NtQueryInformationProcess(process, 26, &wow64_process, 8, 0) != 0)
	{
		return 0;
	}

	return wow64_process;
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

