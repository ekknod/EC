#include "../vm.h"
#include <TlHelp32.h>

#pragma comment(lib, "ntdll.lib")

namespace vm
{
	namespace utils
	{
		static BOOL  bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
		static DWORD FindSectionOffset(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask);
		static QWORD FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask);
		static QWORD scan_section(vm_handle process, QWORD section_address,
			DWORD section_size, PCSTR pattern, PCSTR mask, DWORD len);
	}
}

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

QWORD vm::get_module(vm_handle process, PCSTR dll_name)
{
	#ifdef __linux__
	return (QWORD)0x140000000;
	#else

	QWORD peb = get_wow64_process(process);

	DWORD a0[6]{};
	QWORD a1, a2;
	unsigned short a3[120]{};

	QWORD(*read_ptr)(vm_handle process, QWORD address) = 0;
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

		char final_name[120]{};
		for (int i = 0; i < 120; i++) {
			final_name[i] = (char)a3[i];
			if (a3[i] == 0)
				break;
		}

		if (strcmpi_imp((PCSTR)final_name, dll_name) == 0)
		{
			return read_ptr(process, a1 + a0[4]);
		}

		a1 = read_ptr(process, a1);
		if (a1 == 0)
			break;
	}
	return 0;
	#endif
}

QWORD vm::get_module_export(vm_handle process, QWORD base, PCSTR export_name)
{
	QWORD a0;
	DWORD a1[4]{};
	char a2[260]{};

	a0 = base + read_i16(process, base + 0x3C);
	if (a0 == base)
	{
		return 0;
	}

	WORD  machine = read_i16(process, a0 + 0x4);
	DWORD wow64_offset = machine == 0x8664 ? 0x88 : 0x78;

	a0 = base + (QWORD)read_i32(process, a0 + wow64_offset);
	if (a0 == base)
	{
		return 0;
	}

	int name_length = (int)strlen_imp(export_name);
	if (name_length > 259)
		name_length = 259;

	read(process, a0 + 0x18, &a1, sizeof(a1));
	while (a1[0]--)
	{
		a0 = (QWORD)read_i32(process, base + a1[2] + ((QWORD)a1[0] * 4));
		read(process, base + a0, &a2, name_length);
		a2[name_length] = 0;

		if (!strcmpi_imp(a2, export_name))
		{
			DWORD tmp = read_i16(process, base + a1[3] + ((QWORD)a1[0] * 2)) * 4;
			DWORD tmp2 = read_i32(process, base + a1[1] + tmp);
			return (base + tmp2);
		}
	}
	return 0;
}

QWORD vm::scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length)
{
	if (base == 0)
	{
		return 0;
	}

	QWORD nt_header = (QWORD)read_i32(process, base + 0x03C) + base;
	if (nt_header == base)
	{
		return 0;
	}

	WORD machine = read_i16(process, nt_header + 0x4);
	QWORD section_header = machine == 0x8664 ?
		nt_header + 0x0108 :
		nt_header + 0x00F8;

	for (WORD i = 0; i < read_i16(process, nt_header + 0x06); i++) {
		QWORD section = section_header + ((QWORD)i * 40);
		if (!(read_i32(process, section + 0x24) & 0x00000020))
			continue;

		QWORD virtual_address = base + (QWORD)read_i32(process, section + 0x0C);
		DWORD virtual_size = read_i32(process, section + 0x08);
		QWORD found_pattern = utils::scan_section(process, virtual_address, virtual_size, pattern, mask, length);
		if (found_pattern)
		{
			return found_pattern;
		}
	}
	return 0;
}

QWORD vm::scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length)
{
	QWORD ret = 0;

	if (dumped_module == 0)
		return 0;

	QWORD dos_header = (QWORD)dumped_module;
	QWORD nt_header = (QWORD) * (DWORD*)(dos_header + 0x03C) + dos_header;
	WORD  machine = *(WORD*)(nt_header + 0x4);

	QWORD section_header = machine == 0x8664 ?
		nt_header + 0x0108 :
		nt_header + 0x00F8;

	for (WORD i = 0; i < *(WORD*)(nt_header + 0x06); i++) {

		QWORD section = section_header + ((QWORD)i * 40);
		DWORD section_characteristics = *(DWORD*)(section + 0x24);

		if (section_characteristics & 0x00000020)
		{
			QWORD section_address = dos_header + (QWORD) * (DWORD*)(section + 0x0C);
			DWORD section_size = *(DWORD*)(section + 0x08);
			QWORD address = utils::FindPatternEx(section_address, section_size - length, (BYTE*)pattern, (char*)mask);
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

static BOOL vm::utils::bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{

	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;

	return (*szMask) == 0;
}

static DWORD vm::utils::FindSectionOffset(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask)
{

	if (dwLen <= 0)
		return 0;
	for (DWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return i;

	return 0;
}

static QWORD vm::utils::FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask)
{

	if (dwLen <= 0)
		return 0;
	for (QWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (QWORD)(dwAddress + i);

	return 0;
}

static BYTE GLOBAL_SECTION_DATA[0x1000];
static QWORD vm::utils::scan_section(vm_handle process, QWORD section_address,
	DWORD section_size, PCSTR pattern, PCSTR mask, DWORD len)
{
	DWORD offset = 0;
	while (section_size)
	{
		for (int i = 0x1000; i--;)
			GLOBAL_SECTION_DATA[i] = 0;

		if (section_size >= 0x1000)
		{
			BOOL read_status = vm::read(process, section_address + offset, GLOBAL_SECTION_DATA, 0x1000);
			section_size -= 0x1000;

			if (!read_status)
			{
				offset += 0x1000;
				continue;
			}
		}
		else
		{
			if (!vm::read(process, section_address + offset, GLOBAL_SECTION_DATA, section_size))
			{
				return 0;
			}
			section_size = 0;
		}

		DWORD index = utils::FindSectionOffset((QWORD)GLOBAL_SECTION_DATA,
			(QWORD)0x1000 - len, (BYTE*)pattern, (char*)mask);
		if (index)
		{
			return section_address + offset + index;
		}
		offset += 0x1000;
	}
	return 0;
}

