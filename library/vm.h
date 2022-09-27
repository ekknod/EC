#ifndef VM_H
#define VM_H

inline int to_lower_imp(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	else
		return c;
}

inline int strcmpi_imp(const char* s1, const char* s2)
{
	while (*s1 && (to_lower_imp(*s1) == to_lower_imp(*s2)))
	{
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

//
// sometimes compiler uses precompiled strlen, this is added to prevent that happen in any case.
//
inline unsigned long long strlen_imp(const char *str)
{
	const char *s;

	for (s = str; *s; ++s)
		;

	return (s - str);
}

#ifdef _KERNEL_MODE
#pragma warning (disable: 4996)
#include <ntifs.h>
typedef unsigned __int8  BYTE;
typedef unsigned __int16 WORD;
typedef unsigned __int32 DWORD;
typedef unsigned __int64 QWORD;
typedef int BOOL;

extern QWORD g_memory_range_low;
extern QWORD g_memory_range_high;
#else


#ifdef __linux__

#include <inttypes.h>
#include <malloc.h>
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned long QWORD;
typedef void *PVOID;
typedef int BOOL;
typedef int INT32;
typedef const char *PCSTR;

#else

#include <windows.h>
typedef unsigned __int64 QWORD;

#endif


#endif


typedef void* vm_handle;
enum class VM_MODULE_TYPE {
	Full = 1,
	CodeSectionsOnly = 2,
	Raw = 3 // used for dump to file
};

namespace vm
{
	BOOL      process_exists(PCSTR process_name);
	vm_handle open_process(PCSTR process_name);
	vm_handle open_process_ex(PCSTR process_name, PCSTR dll_name);
	vm_handle open_process_by_module_name(PCSTR dll_name);
	void      close(vm_handle process);
	BOOL      running(vm_handle process);

	BOOL      read(vm_handle process, QWORD address, PVOID buffer, QWORD length);
	BOOL      write(vm_handle process, QWORD address, PVOID buffer, QWORD length);

	QWORD     get_peb(vm_handle process);
	QWORD     get_wow64_process(vm_handle process);

	PVOID     dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type);
	void      free_module(PVOID dumped_module);


	inline BYTE  read_i8(vm_handle process, QWORD address);
	inline WORD  read_i16(vm_handle process, QWORD address);
	inline DWORD read_i32(vm_handle process, QWORD address);
	inline QWORD read_i64(vm_handle process, QWORD address);
	inline float read_float(vm_handle process, QWORD address);

	inline BOOL  write_i8(vm_handle process, QWORD address, BYTE value);
	inline BOOL  write_i16(vm_handle process, QWORD address, WORD value);
	inline BOOL  write_i32(vm_handle process, QWORD address, DWORD value);
	inline BOOL  write_i64(vm_handle process, QWORD address, QWORD value);
	inline BOOL  write_float(vm_handle process, QWORD address, float value);

	inline QWORD get_relative_address(vm_handle process, QWORD instruction, DWORD offset, DWORD instruction_size);
	inline QWORD get_module(vm_handle process, PCSTR dll_name);
	inline QWORD get_module_export(vm_handle process, QWORD base, PCSTR export_name);

	inline QWORD scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length);
	inline QWORD scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length);

	namespace utils
	{
		inline BOOL  bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
		inline DWORD FindSectionOffset(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask);
		inline QWORD FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask);
		inline QWORD scan_section(vm_handle process, QWORD section_address,
			DWORD section_size, PCSTR pattern, PCSTR mask, DWORD len);
	}
}

inline BYTE vm::read_i8(vm_handle process, QWORD address)
{
	BYTE result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

inline WORD vm::read_i16(vm_handle process, QWORD address)
{
	WORD result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

inline DWORD vm::read_i32(vm_handle process, QWORD address)
{
	DWORD result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

inline QWORD vm::read_i64(vm_handle process, QWORD address)
{
	QWORD result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

inline float vm::read_float(vm_handle process, QWORD address)
{
	float result = 0;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

inline BOOL vm::write_i8(vm_handle process, QWORD address, BYTE value)
{
	return write(process, address, &value, sizeof(value));
}

inline BOOL vm::write_i16(vm_handle process, QWORD address, WORD value)
{
	return write(process, address, &value, sizeof(value));
}

inline BOOL vm::write_i32(vm_handle process, QWORD address, DWORD value)
{
	return write(process, address, &value, sizeof(value));
}

inline BOOL vm::write_i64(vm_handle process, QWORD address, QWORD value)
{
	return write(process, address, &value, sizeof(value));
}

inline BOOL vm::write_float(vm_handle process, QWORD address, float value)
{
	return write(process, address, &value, sizeof(value));
}

inline QWORD vm::get_relative_address(vm_handle process, QWORD instruction, DWORD offset, DWORD instruction_size)
{
	INT32 rip_address = read_i32(process, instruction + offset);
	return (QWORD)(instruction + instruction_size + rip_address);
}

//
// this function is very old, because back in that time i used short variable names it's quite difficult
// to remember how exactly this works :D
// it's mimimal port of Windows GetModuleHandleA to external. structure offsets are just fixed for both x86/x64.
//
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

//
// this function is very old, because back in that time i used short variable names it's quite difficult
// to remember how exactly this works :D
// it's mimimal port of Windows GetProcAddress to external. structure offsets are just fixed for both x86/x64.
//
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

inline QWORD vm::scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length)
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

inline QWORD vm::scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length)
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

inline BOOL vm::utils::bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{

	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;

	return (*szMask) == 0;
}

inline DWORD vm::utils::FindSectionOffset(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask)
{

	if (dwLen <= 0)
		return 0;
	for (DWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return i;

	return 0;
}

inline QWORD vm::utils::FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask)
{

	if (dwLen <= 0)
		return 0;
	for (QWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (QWORD)(dwAddress + i);

	return 0;
}

//
// disables stack check, maybe there is better workaround for this as well
//
static BYTE GLOBAL_SECTION_DATA[0x1000];
inline QWORD vm::utils::scan_section(vm_handle process, QWORD section_address,
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

#endif /* VM_H */

