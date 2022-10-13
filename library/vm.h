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

	QWORD     get_module(vm_handle process, PCSTR dll_name);
	QWORD     get_module_export(vm_handle process, QWORD base, PCSTR export_name);

	QWORD     scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length);
	QWORD     scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length);

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

#endif /* VM_H */

