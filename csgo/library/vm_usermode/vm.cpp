#include "../globals.h"
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

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		if (!_strcmpi(entry.szExeFile, process_name))
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

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		if (!_strcmpi(entry.szExeFile, process_name))
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

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(snapshot, &entry))
	{
		if (!_strcmpi(entry.szExeFile, process_name))
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

	PROCESSENTRY32 entry;
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

BYTE vm::read_i8(vm_handle process, QWORD address)
{
	BYTE result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

WORD vm::read_i16(vm_handle process, QWORD address)
{
	WORD result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

DWORD vm::read_i32(vm_handle process, QWORD address)
{
	DWORD result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

QWORD vm::read_i64(vm_handle process, QWORD address)
{
	QWORD result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

float vm::read_float(vm_handle process, QWORD address)
{
	float result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
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

BOOL vm::write_i8(vm_handle process, QWORD address, BYTE value)
{
	return write(process, address, &value, sizeof(value));
}

BOOL vm::write_i16(vm_handle process, QWORD address, WORD value)
{
	return write(process, address, &value, sizeof(value));
}

BOOL vm::write_i32(vm_handle process, QWORD address, DWORD value)
{
	return write(process, address, &value, sizeof(value));
}

BOOL vm::write_i64(vm_handle process, QWORD address, QWORD value)
{
	return write(process, address, &value, sizeof(value));
}

BOOL vm::write_float(vm_handle process, QWORD address, float value)
{
	return write(process, address, &value, sizeof(value));
}

QWORD vm::get_relative_address(vm_handle process, QWORD instruction, DWORD offset, DWORD instruction_size)
{
	INT32 rip_address = read_i32(process, instruction + offset);
	return (QWORD)(instruction + instruction_size + rip_address);
}

QWORD vm::get_peb(vm_handle process)
{
	QWORD peb[6];

	if (NtQueryInformationProcess(process, 0, &peb, 48, 0) != 0)
	{
		return 0;
	}

	return peb[1];
}

QWORD vm::get_wow64_process(vm_handle process)
{
	QWORD wow64_process;

	if (process == 0)
		return 0;

	if (NtQueryInformationProcess(process, 26, &wow64_process, 8, 0) != 0)
	{
		return 0;
	}

	return wow64_process;
}

QWORD vm::get_module(vm_handle process, PCSTR dll_name)
{
	QWORD peb = get_wow64_process(process);

	DWORD a0[6];
	QWORD a1, a2;
	unsigned short a3[120];

	QWORD(*read_ptr)(vm_handle process, QWORD address);
	if (peb)
	{
		*(QWORD*)&read_ptr = (QWORD)read_i32;
		a0[0] = 0x04, a0[1] = 0x0C, a0[2] = 0x14, a0[3] = 0x28, a0[4] = 0x10, a0[5] = 0x20;
	}
	else
	{
		*(QWORD*)&read_ptr = (QWORD)read_i64;
		peb = get_peb(process);
		a0[0] = 0x08, a0[1] = 0x18, a0[2] = 0x20, a0[3] = 0x50, a0[4] = 0x20, a0[5] = 0x40;
	}

	if (peb == 0)
	{
		return 0;
	}

	a1 = read_ptr(process, read_ptr(process, peb + a0[1]) + a0[2]);
	a2 = read_ptr(process, a1 + a0[0]);

	while (a1 != a2) {
		read(process, read_ptr(process, a1 + a0[3]), a3, sizeof(a3));
		if (dll_name == 0)
			return read_ptr(process, a1 + a0[4]);

		char final_name[120];
		for (int i = 0; i < 120; i++) {
			final_name[i] = (char)a3[i];
			if (a3[i] == 0)
				break;
		}

		if (_strcmpi((PCSTR)final_name, dll_name) == 0)
		{
			return read_ptr(process, a1 + a0[4]);
		}

		a1 = read_ptr(process, a1);
		if (a1 == 0)
			break;
	}
	return 0;
}

QWORD vm::get_module_export(vm_handle process, QWORD base, PCSTR export_name)
{
	QWORD a0;
	DWORD a1[4];
	char a2[260];

	a0 = base + read_i16(process, base + 0x3C);
	if (a0 == base)
	{
		return 0;
	}

	WORD machine = read_i16(process, a0 + 0x4);
	BOOL wow64 = machine == 0x8664 ? 0 : 1;

	a0 = base + read_i32(process, a0 + 0x88 - wow64 * 16);
	if (a0 == base)
	{
		return 0;
	}

	read(process, a0 + 0x18, &a1, sizeof(a1));
	while (a1[0]--) {
		memset(a2, 0, 260);
		a0 = read_i32(process, base + a1[2] + (a1[0] * 4));
		read(process, base + a0, &a2, sizeof(a2));
		if (!_strcmpi(a2, export_name)) {
			return (base + read_i32(process, base + a1[1] +
				(read_i16(process, base + a1[3] + (a1[0] * 2)) * 4)));
		}
	}
	return 0;
}

PVOID vm::dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type)
{
	QWORD nt_header;
	QWORD image_size;
	BYTE* ret;

	if (base == 0)
	{
		return 0;
	}

	nt_header = read_i32(process, base + 0x03C) + base;
	if (nt_header == base)
	{
		return 0;
	}

	image_size = read_i32(process, nt_header + 0x050);
	if (image_size == 0)
	{
		return 0;
	}

	ret = (BYTE*)malloc(image_size + 16);
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


		QWORD target_address = (QWORD)ret + read_i32(process, section + ((module_type == VM_MODULE_TYPE::Raw) ? 0x14 : 0x0C));
		QWORD virtual_address = base + read_i32(process, section + 0x0C);
		DWORD virtual_size = read_i32(process, section + 0x08);

		read(process, virtual_address, (PVOID)target_address, virtual_size);
	}

	return (PVOID)ret;
}

void vm::free_module(PVOID dumped_module)
{
	QWORD a0 = (QWORD)dumped_module;

	a0 -= 16;
	free((void*)a0);
}

static BOOL bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{

	for (; *szMask; ++szMask, ++pData, ++bMask)
		if ((*szMask == 1 || *szMask == 'x') && *pData != *bMask)
			return 0;

	return (*szMask) == 0;
}

static QWORD FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask)
{

	if (dwLen <= 0)
		return 0;
	for (QWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (QWORD)(dwAddress + i);

	return 0;
}

QWORD vm::scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length)
{
	QWORD ret = 0;

	if (dumped_module == 0)
		return 0;

	QWORD dos_header = (QWORD)dumped_module;
	QWORD nt_header = *(DWORD*)(dos_header + 0x03C) + dos_header;
	WORD  machine = *(WORD*)(nt_header + 0x4);

	QWORD section_header = machine == 0x8664 ?
		nt_header + 0x0108 :
		nt_header + 0x00F8;

	for (WORD i = 0; i < *(WORD*)(nt_header + 0x06); i++) {

		QWORD section = section_header + ((QWORD)i * 40);
		DWORD section_characteristics = *(DWORD*)(section + 0x24);

		if (section_characteristics & 0x00000020)
		{
			QWORD section_address = dos_header + *(DWORD*)(section + 0x0C);
			DWORD section_size = *(DWORD*)(section + 0x08);
			QWORD address = FindPatternEx(section_address, section_size - length, (BYTE*)pattern, (char*)mask);
			if (address)
			{
				ret = (address - (QWORD)dumped_module) +
					*(QWORD*)((QWORD)dumped_module - 16);
				break;
			}
		}

	}
	return ret;
}

