#include "../vm.h"

namespace pm
{
	static BOOL  read(QWORD address, PVOID buffer, QWORD length, QWORD* ret);
	static BOOL  write(QWORD address, PVOID buffer, QWORD length);
	static QWORD read_i64(QWORD address);
	static QWORD translate(QWORD dir, QWORD va);
}

namespace vm
{
	static QWORD scan_section(vm_handle process, QWORD section_address, DWORD section_size, PCSTR pattern, PCSTR mask, DWORD len);
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

BYTE vm::read_i8(vm_handle process, QWORD address)
{
	BYTE result;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

WORD vm::read_i16(vm_handle process, QWORD address)
{
	WORD result;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

DWORD vm::read_i32(vm_handle process, QWORD address)
{
	DWORD result;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

QWORD vm::read_i64(vm_handle process, QWORD address)
{
	QWORD result;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
}

float vm::read_float(vm_handle process, QWORD address)
{
	float result;
	if (!read(process, address, &result, sizeof(result)))
	{
		return 0;
	}
	return result;
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

		if (strcmpi_imp((PCSTR)final_name, dll_name) == 0)
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
		if (!strcmpi_imp(a2, export_name)) {
			return (base + read_i32(process, base + a1[1] +
				(read_i16(process, base + a1[3] + (a1[0] * 2)) * 4)));
		}
	}
	return 0;
}

static BOOL bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{

	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;

	return (*szMask) == 0;
}

static DWORD FindSectionOffset(QWORD dwAddress, QWORD dwLen, BYTE* bMask, char* szMask)
{

	if (dwLen <= 0)
		return 0;
	for (DWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return i;

	return 0;
}

static BYTE GLOBAL_SECTION_DATA[0x1000];
static QWORD vm::scan_section(vm_handle process, QWORD section_address, DWORD section_size, PCSTR pattern, PCSTR mask, DWORD len)
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
		} else {
			if (!vm::read(process, section_address + offset, GLOBAL_SECTION_DATA, section_size))
			{
				return 0;
			}
			section_size = 0;
		}


		DWORD index = FindSectionOffset((QWORD)GLOBAL_SECTION_DATA, 0x1000 - len, (BYTE*)pattern, (char*)mask);
		if (index)
		{
			return section_address + offset + index;
		}
		offset += 0x1000;
	}
	return 0;
}

QWORD vm::scan_pattern_direct(vm_handle process, QWORD base, PCSTR pattern, PCSTR mask, DWORD length)
{
	if (base == 0)
	{
		return 0;
	}

	QWORD nt_header = read_i32(process, base + 0x03C) + base;
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

		QWORD virtual_address = base + read_i32(process, section + 0x0C);
		DWORD virtual_size = read_i32(process, section + 0x08);
		QWORD found_pattern = scan_section(process, virtual_address, virtual_size, pattern, mask, length);
		if (found_pattern)
		{
			return found_pattern;
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

	ret = (BYTE*)ExAllocatePoolWithTag(NonPagedPool, image_size + 16, 'ofnI');
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
	if (dumped_module)
	{
		QWORD a0 = (QWORD)dumped_module;
		a0 -= 16;

		ExFreePoolWithTag((void*)a0, 'ofnI');
	}
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

static BYTE MM_COPY_BUFFER[0x1000];
static BOOL pm::read(QWORD address, PVOID buffer, QWORD length, QWORD* ret)
{
	PHYSICAL_MEMORY_RANGE low_address  = g_memory_range[0];
	PHYSICAL_MEMORY_RANGE high_address = g_memory_range[g_memory_range_count - 1];

	if (address < (QWORD)low_address.BaseAddress.QuadPart)
	{
		return 0;
	}

	if (address + length > (QWORD)(high_address.BaseAddress.QuadPart + high_address.NumberOfBytes.QuadPart))
	{
		return 0;
	}

	if (length > 0x1000)
	{
		length = 0x1000;
	}

	MM_COPY_ADDRESS physical_address;
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
	PHYSICAL_MEMORY_RANGE low_address = g_memory_range[0];
	PHYSICAL_MEMORY_RANGE high_address = g_memory_range[g_memory_range_count - 1];

	if (address < (QWORD)low_address.BaseAddress.QuadPart)
	{
		return 0;
	}

	if (address + length > (QWORD)(high_address.BaseAddress.QuadPart + high_address.NumberOfBytes.QuadPart))
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
	QWORD result;
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

