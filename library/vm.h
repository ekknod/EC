#ifndef VM_H
#define VM_H


#ifdef _KERNEL_MODE

#include <ntifs.h>
typedef unsigned __int8  BYTE;
typedef unsigned __int16 WORD;
typedef unsigned __int32 DWORD;
typedef unsigned __int64 QWORD;
typedef int BOOL;

extern PPHYSICAL_MEMORY_RANGE g_memory_range;
extern int g_memory_range_count;
inline int _strcmpi(const char* s1, const char* s2)
{	
	while(*s1 && (tolower(*s1) == tolower(*s2)))
	{
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

#else

#include <windows.h>
typedef unsigned __int64 QWORD;

#endif


typedef void  *vm_handle;

enum class VM_MODULE_TYPE {
	Full = 1,
	CodeSectionsOnly = 2,
	Raw = 3 // used for dump to file
} ;

namespace vm
{
	BOOL      process_exists(PCSTR process_name);
	vm_handle open_process(PCSTR process_name);
	vm_handle open_process_ex(PCSTR process_name, PCSTR dll_name);
	vm_handle open_process_by_module_name(PCSTR dll_name);
	void      close(vm_handle process);
	BOOL      running(vm_handle process);

	BOOL      read(vm_handle process, QWORD address, PVOID buffer, QWORD length);
	BYTE      read_i8(vm_handle process, QWORD address);
	WORD      read_i16(vm_handle process, QWORD address);
	DWORD     read_i32(vm_handle process, QWORD address);
	QWORD     read_i64(vm_handle process, QWORD address);
	float     read_float(vm_handle process, QWORD address);

	BOOL      write(vm_handle process, QWORD address, PVOID buffer, QWORD length);
	BOOL      write_i8(vm_handle process, QWORD address, BYTE value);
	BOOL      write_i16(vm_handle process, QWORD address, WORD value);
	BOOL      write_i32(vm_handle process, QWORD address, DWORD value);
	BOOL      write_i64(vm_handle process, QWORD address, QWORD value);
	BOOL      write_float(vm_handle process, QWORD address, float value);

	QWORD     get_relative_address(vm_handle process, QWORD instruction, DWORD offset, DWORD instruction_size);
	QWORD     get_peb(vm_handle process);
	QWORD     get_wow64_process(vm_handle process);

	QWORD     get_module(vm_handle process, PCSTR dll_name);
	QWORD     get_module_export(vm_handle process, QWORD base, PCSTR export_name);

	PVOID     dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type);
	void      free_module(PVOID dumped_module);
	QWORD     scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length);
}

#endif /* VM_H */

