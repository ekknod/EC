#include <Windows.h>
#include <intrin.h>
#include "globals.h"

typedef ULONG_PTR QWORD;

#define PAGE_SIZE 0x1000
#define PAGE_ALIGN(Va) ((VOID*)((QWORD)(Va) & ~(PAGE_SIZE - 1)))

void SwapMemory(QWORD BaseAddress, QWORD ImageSize, QWORD NewBase)
{
	INT32 current_location = (INT32)((QWORD)_ReturnAddress() - BaseAddress);

	//
	// copy currently loaded image to new section
	//
	MemCopy((void *)NewBase, (void *)BaseAddress, ImageSize);

	//
	// swap memory
	//
	*(QWORD*)(_AddressOfReturnAddress()) = NewBase + current_location;
}

void SwapMemory2(QWORD CurrentBase, QWORD NewBase)
{
	INT32 current_location = (INT32)((QWORD)_ReturnAddress() - CurrentBase);
	//
	// swap memory
	//
	*(QWORD*)(_AddressOfReturnAddress()) = NewBase + current_location;
}

QWORD get_caller_base(QWORD return_address)
{
	return_address = (QWORD)PAGE_ALIGN(return_address);

	while (*(unsigned short*)(return_address) != IMAGE_DOS_SIGNATURE)
	{
		return_address -= PAGE_SIZE;
	}

	return return_address;
}

QWORD get_loader_block(QWORD winload_base)
{
	QWORD a0 = get_pe_entrypoint(winload_base);
	while (*(unsigned short*)a0 != 0xC35D) a0++;
	while (*(unsigned char*)a0 != 0xE8) a0--; a0--;
	while (*(unsigned char*)a0 != 0xE8) a0--;
	a0 = a0 + *(int*)(a0 + 1) + 5;
	while (*(DWORD*)a0 != 0x67636D30) a0++;
	while (*(unsigned short*)a0 != 0xD88B) a0--;
	a0 = a0 - 5;
	a0 = a0 + *(int*)(a0 + 1) + 5;
	while (*(unsigned short*)a0 != 0x8B48) a0++;
	return *(QWORD*)((a0 + 7) + *(int*)(a0 + 3));
}

void MemCopy(void* dest, void* src, QWORD size)
{
	for (unsigned char* d = (unsigned char*)dest, *s = (unsigned char*)src; size--; *d++ = *s++)
		;
}

int strcmp_imp(const char *cs, const char *ct)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}

int wcscmp_imp(const wchar_t *cs, const wchar_t *ct)
{
	unsigned short c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}


QWORD GetExportByName(QWORD base, const char* export_name)
{
	QWORD a0;
	DWORD a1[4];

	a0 = base + *(unsigned short*)(base + 0x3C);
	if (a0 == base)
	{
		return 0;
	}


	WORD machine = *(WORD*)(a0 + 0x4);

	a0 = machine == 0x8664 ? base + *(DWORD*)(a0 + 0x88) : base + *(DWORD*)(a0 + 0x78);

	if (a0 == base)
	{
		return 0;
	}


	a1[0] = *(DWORD*)(a0 + 0x18);
	a1[1] = *(DWORD*)(a0 + 0x1C);
	a1[2] = *(DWORD*)(a0 + 0x20);
	a1[3] = *(DWORD*)(a0 + 0x24);
	while (a1[0]--) {
		a0 = base + *(DWORD*)(base + a1[2] + (a1[0] * 4));
		if (strcmp_imp((const char *)a0, export_name) == 0)
		{
			return (base + *(DWORD*)(base + a1[1] + (*(unsigned short*)(base + a1[3] + (a1[0] * 2)) * 4)));
		}
	}
	return 0;
}

QWORD get_pe_entrypoint(QWORD base)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
	return base + nt->OptionalHeader.AddressOfEntryPoint;
}

DWORD get_pe_size(QWORD base)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
	return nt->OptionalHeader.SizeOfImage;
}

QWORD get_pe_section(QWORD base, const char *section_name, DWORD *size)
{
	/*
	QWORD image_dos_header = (QWORD)base;
	QWORD image_nt_header = *(DWORD*)(image_dos_header + 0x03C) + image_dos_header;
	unsigned short machine = *(WORD*)(image_nt_header + 0x4);

	QWORD section_header_off = machine == 0x8664 ?
		image_nt_header + 0x0108 :
		image_nt_header + 0x00F8;

	for (WORD i = 0; i < *(WORD*)(image_nt_header + 0x06); i++) {
		QWORD section = section_header_off + (i * 40);
		ULONG section_virtual_address = *(ULONG*)(section + 0x0C);
		ULONG section_virtual_size = *(ULONG*)(section + 0x08);

		if (!strcmp((const char *)(UCHAR*)(section + 0x00), section_name))
		{
			if (size)
			{
				*size = section_virtual_size;
			}
			return base + section_virtual_address;
		}
	}
	return 0;
	*/



	

	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);

	QWORD section_header_off = (QWORD)nt + 0x0108;
	for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++)
	{
		PIMAGE_SECTION_HEADER section = (PIMAGE_SECTION_HEADER)(section_header_off + (i * 40));
		if (!strcmp((const char *)section->Name, section_name))
		{
			if (size)
			{
				*size = section->Misc.VirtualSize;
			}
			return base + section->VirtualAddress;
		}
	}
	return 0;
}

BOOL pe_resolve_imports(QWORD ntoskrnl, QWORD base)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);

	DWORD import_directory =
		nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
		.VirtualAddress;

	if (import_directory) {
		IMAGE_IMPORT_DESCRIPTOR* import_descriptor =
			(IMAGE_IMPORT_DESCRIPTOR*)(base + import_directory);



		for (; import_descriptor->FirstThunk; ++import_descriptor) {

			if (import_descriptor->FirstThunk == 0)
				break;

			IMAGE_THUNK_DATA64* thunk =
				(IMAGE_THUNK_DATA64*)(base +
					import_descriptor->FirstThunk);

			if (thunk == 0)
				break;

			if (import_descriptor->OriginalFirstThunk == 0)
				break;

			IMAGE_THUNK_DATA64* original_thunk =
				(IMAGE_THUNK_DATA64*)(base +
					import_descriptor->OriginalFirstThunk);

			for (; thunk->u1.AddressOfData; ++thunk, ++original_thunk) {
				UINT64 import = GetExportByName(
					(QWORD)ntoskrnl,
					((IMAGE_IMPORT_BY_NAME*)(base +
						original_thunk->u1.AddressOfData))
					->Name);

				if (import == 0)
				{
					return 0;
				}
				thunk->u1.Function = import;
			}
		}

	}
	return 1;
}

void pe_clear_headers(QWORD base)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
	for (DWORD i = nt->OptionalHeader.SizeOfHeaders; i--;)
	{
		((unsigned char*)base)[i] = (unsigned char)(i + 4 - ((unsigned char*)( base ))[i]) ;
	}
}

typedef struct _UNICODE_STRING {
	UINT16 Length;
	UINT16 MaximumLength;
	wchar_t *Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _KLDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	VOID* ExceptionTable;
	UINT32 ExceptionTableSize;
	VOID* GpValue;
	VOID* NonPagedDebugInfo;
	VOID* ImageBase;
	VOID* EntryPoint;
	UINT32 SizeOfImage;
	UNICODE_STRING FullImageName;
	UNICODE_STRING BaseImageName;
} KLDR_DATA_TABLE_ENTRY;

QWORD GetModuleEntry(LIST_ENTRY* entry, const unsigned short *name)
{
	LIST_ENTRY *list = entry;
	while ((list = list->Flink) != entry) {
		KLDR_DATA_TABLE_ENTRY *module =
			CONTAINING_RECORD(list, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (module && wcscmp_imp((wchar_t*)module->BaseImageName.Buffer, name) == 0)
		{
			return (QWORD)module->ImageBase;
		}
	}
	return NULL;
}

static int CheckMask(unsigned char* base, unsigned char* pattern, unsigned char* mask)
{
	for (; *mask; ++base, ++pattern, ++mask)
		if (*mask == 'x' && *base != *pattern)
			return 0;
	return 1;
}

static QWORD strleni(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

void *FindPatternEx(unsigned char* base, QWORD size, unsigned char* pattern, unsigned char* mask)
{
	size -= strleni((const char *)mask);
	for (QWORD i = 0; i <= size; ++i) {
		void* addr = &base[i];
		if (CheckMask((unsigned char *)addr, pattern, mask))
			return addr;
	}
	return 0;
}

QWORD FindPattern(QWORD base, unsigned char* pattern, unsigned char* mask)
{
	if (base == 0)
	{
		return 0;
	}

	QWORD nt_header = (QWORD)*(DWORD*)(base + 0x03C) + base;
	if (nt_header == base)
	{
		return 0;
	}

	WORD machine = *(WORD*)(nt_header + 0x4);
	QWORD section_header = machine == 0x8664 ?
		nt_header + 0x0108 :
		nt_header + 0x00F8;

	for (WORD i = 0; i < *(WORD*)(nt_header + 0x06); i++) {
		QWORD section = section_header + ((QWORD)i * 40);

		DWORD section_characteristics = *(DWORD*)(section + 0x24);

		if (section_characteristics & 0x00000020 && !(section_characteristics & 0x02000000))
		{
			QWORD virtual_address = base + (QWORD)*(DWORD*)(section + 0x0C);
			DWORD virtual_size = *(DWORD*)(section + 0x08);

			void *found_pattern = FindPatternEx( (unsigned char*)virtual_address, virtual_size, pattern, mask);
			if (found_pattern)
			{
				return (QWORD)found_pattern;
			}
		}
	}
	return 0;
}

static PIMAGE_SECTION_HEADER get_image_sections(QWORD nt)
{
	return (PIMAGE_SECTION_HEADER)(nt + 0x0108);
}

DWORD get_file_offset(QWORD base, QWORD address)
{
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS *nt  = (IMAGE_NT_HEADERS*)((char*)dos + dos->e_lfanew);
	PIMAGE_SECTION_HEADER section = get_image_sections((QWORD)nt);
	for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++)
	{
		QWORD section_addr = base + section[i].VirtualAddress;	
		if (address >= section_addr && address <= (section_addr + section[i].Misc.VirtualSize))
		{
			DWORD delta = (DWORD)(address - section_addr);
			return section[i].PointerToRawData + delta;
		}
	}
	return 0;
}

DWORD get_checksum(QWORD base)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
	return nt->OptionalHeader.CheckSum;
}

DWORD get_checksum_off(QWORD base)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	return dos->e_lfanew + 0x58;
}

void set_checksum(QWORD base, DWORD checksum)
{
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
	nt->OptionalHeader.CheckSum = checksum;
}

//
// https://github.com/mrexodia/portable-executable-library/blob/master/pe_lib/pe_checksum.cpp
//
DWORD calculate_checksum(PVOID file, DWORD file_size)
{
	//Checksum value
	unsigned long long checksum = 0;

	//Read DOS header
	IMAGE_DOS_HEADER* header = (IMAGE_DOS_HEADER*)file;

	//Calculate PE checksum
	unsigned long long top = 0xFFFFFFFF;
	top++;

	//"CheckSum" field position in optional PE headers - it's always 64 for PE and PE+
	static const unsigned long checksum_pos_in_optional_headers = 64;
	//Calculate real PE headers "CheckSum" field position
	//Sum is safe here
	unsigned long pe_checksum_pos = header->e_lfanew + sizeof(IMAGE_FILE_HEADER) + sizeof(DWORD) + checksum_pos_in_optional_headers;

	//Calculate checksum for each byte of file

	for (long long i = 0; i < file_size; i += 4)
	{
		unsigned long dw = *(unsigned long*)((char*)file + i);
		//Skip "CheckSum" DWORD
		if (i == pe_checksum_pos)
			continue;

		//Calculate checksum
		checksum = (checksum & 0xffffffff) + dw + (checksum >> 32);
		if (checksum > top)
			checksum = (checksum & 0xffffffff) + (checksum >> 32);
	}

	//Finish checksum
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = (checksum)+(checksum >> 16);
	checksum = checksum & 0xffff;

	checksum += static_cast<unsigned long>(file_size);
	return static_cast<DWORD>(checksum);
}

