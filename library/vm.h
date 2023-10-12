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

inline int wcscmpi_imp(unsigned short* s1, unsigned short* s2)
{
	while (*s1 && (to_lower_imp(*s1) == to_lower_imp(*s2)))
	{
		s1++;
		s2++;
	}
	return *(unsigned short*)s1 - *(unsigned short*)s2;
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
typedef unsigned long DWORD;
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
typedef char BOOLEAN;
typedef int INT32;
typedef const char *PCSTR;

extern QWORD get_module_base(int process_id, const char *module_name);

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

	#ifndef __linux__
	QWORD     get_peb(vm_handle process);
	QWORD     get_wow64_process(vm_handle process);

	inline PVOID dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type);
	inline void  free_module(PVOID dumped_module);
	#endif

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

	#ifndef __linux__
	inline QWORD scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length);
	inline QWORD scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length);


	namespace utils
	{
		inline BOOL  bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask);
		inline DWORD FindSectionOffset(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask);
		inline QWORD FindPatternEx(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask);
	}
	#endif
}

#ifndef __linux__
inline PVOID vm::dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type)
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
	ret = (BYTE*)ExAllocatePool2(POOL_FLAG_NON_PAGED, (QWORD)(image_size + 16), 'ofnI');
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

inline void vm::free_module(PVOID dumped_module)
{
	QWORD a0 = (QWORD)dumped_module;

	a0 -= 16;
#ifdef _KERNEL_MODE
	ExFreePoolWithTag((void*)a0, 'ofnI');
#else
	free((void*)a0);
#endif
}

#endif /* ifndef linux */

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

QWORD vm::get_module(vm_handle process, PCSTR dll_name)
{
	#ifdef __linux__

	return get_module_base(((int*)&process)[1], dll_name);

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

	a1 = read_ptr(process, peb + a0[1]);
	if (a1 == 0)
	{
		return 0;
	}

	a1 = read_ptr(process, a1 + a0[2]);
	if (a1 == 0)
	{
		return 0;
	}

	a2 = read_ptr(process, a1 + a0[0]);

	while (a1 != a2) {
		QWORD a4 = read_ptr(process, a1 + a0[3]);
		if (a4 != 0)
		{
			read(process, a4, a3, sizeof(a3));
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
		}
		a1 = read_ptr(process, a1);
		if (a1 == 0)
			break;
	}
	return 0;
	#endif
}

#ifdef __linux__

inline QWORD get_elf_address(vm_handle process, QWORD base, int tag)
{
	BOOL wow64 = (vm::read_i16(process, base + 0x12) == 62) ? 0 : 1;
	
	int pht_count;
	int pht_file_offset;
	int pht_size;

	if (wow64)
	{
		pht_count = 0x2C;
		pht_file_offset = 0x1C;
		pht_size = 32;
	}
	else
	{
		pht_count = 0x38;
		pht_file_offset = 0x20;
		pht_size = 56;
	}

	QWORD a0 = vm::read_i32(process, base + pht_file_offset) + base;

	for (WORD i = 0; i < vm::read_i16(process, base + pht_count); i++)
	{
		QWORD a2 = pht_size * i + a0;
		if (vm::read_i32(process, a2) == tag)
		{
			return a2;
		}
	}
	return 0;
}

inline QWORD get_dyn_address(vm_handle process, QWORD base, QWORD tag)
{
	QWORD dyn = get_elf_address(process, base, 2);
	if (dyn == 0)
	{
		return 0;
	}

	BOOL wow64 = (vm::read_i16(process, base + 0x12) == 62) ? 0 : 1;

	int reg_size;
	if (wow64)
	{
		reg_size = 4;
	}
	else
	{
		reg_size = 8;
	}
	
	vm::read(process, dyn + (2*reg_size), &dyn, reg_size);

	dyn += base;
	
	while (1)
	{
		QWORD dyn_tag = 0;
		vm::read(process, dyn, &dyn_tag, reg_size);

		if (dyn_tag == 0)
		{
			break;
		}

		if (dyn_tag == tag)
		{
			QWORD address = 0;
			vm::read(process, dyn + reg_size, &address, reg_size);
			return address;
		}

		dyn += (2*reg_size);
	}
	return 0;
}

QWORD vm::get_module_export(vm_handle process, QWORD base, PCSTR export_name)
{
	int offset, add, length;

	BOOL wow64 = (vm::read_i16(process, base + 0x12) == 62) ? 0 : 1;
	if (wow64)
	{
		offset = 0x20, add = 0x10, length = 0x04;
	}
	else
	{
		offset = 0x40, add = 0x18, length = 0x08;
	}

	QWORD str_tab = get_dyn_address(process, base, 5);
	QWORD sym_tab = get_dyn_address(process, base, 6);

	sym_tab += add;

	uint32_t st_name = 1;
	do
	{
		char sym_name[120]{};
		if (vm::read(process, str_tab + st_name, &sym_name, sizeof(sym_name)) == -1)
			break;
		
		if (strcmpi_imp(sym_name, export_name) == 0)
		{
			vm::read(process, sym_tab + length, &sym_tab, length);
			return sym_tab + base;
		}
		sym_tab += add;
	} while (vm::read(process, sym_tab, &st_name, sizeof(st_name)) != -1);

	return 0;
}

#else

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
		if (a0)
		{
			read(process, base + a0, &a2, name_length);
			a2[name_length] = 0;

			if (!strcmpi_imp(a2, export_name))
			{
				DWORD tmp = read_i16(process, base + a1[3] + ((QWORD)a1[0] * 2)) * 4;
				DWORD tmp2 = read_i32(process, base + a1[1] + tmp);
				return (base + tmp2);
			}
		}
	}
	return 0;
}

#endif

#ifndef __linux__
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

QWORD vm::scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length)
{
	if (base == 0)
	{
		return 0;
	}
	
	PVOID dumped_module = vm::dump_module(process, base, VM_MODULE_TYPE::CodeSectionsOnly);
	if (dumped_module == 0)
	{
		return 0;
	}

	QWORD patt = vm::scan_pattern(dumped_module, pattern, mask, length);

	vm::free_module(dumped_module);
	return patt;
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
#endif /* ifndef linux */

#endif /* VM_H */

