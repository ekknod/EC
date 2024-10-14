#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Guid/MemoryAttributesTable.h>
#include <intrin.h>
#include "globals.h"

extern "C"
{
	//
	// EFI common variables
	//
	EFI_SYSTEM_TABLE       *gST = 0;
	EFI_RUNTIME_SERVICES   *gRT = 0;
	EFI_BOOT_SERVICES      *gBS = 0;

	//
	// EFI image variables
	//
	QWORD EfiBaseAddress        = 0;
	QWORD EfiBaseSize           = 0;
	DWORD GlobalStatusVariable  = 0;
	QWORD winload_base          = 0;
	QWORD ntoskrnl_base         = 0;


	//
	// fixed table for HVCI
	//
	EFI_MEMORY_ATTRIBUTES_TABLE *mTable;
	__int64 (__fastcall *BlMmMapPhysicalAddressEx)(__int64 *a1, __int64 a2, unsigned __int64 a3, unsigned int a4, char a5);
	__int64 (__fastcall *BlMmUnmapVirtualAddressEx)(unsigned __int64 a1, unsigned __int64 a2, char a3);


	//
	// EFI global variables
	//
	EFI_ALLOCATE_PAGES oAllocatePages;
	EFI_EXIT_BOOT_SERVICES oExitBootServices;
	EFI_STATUS EFIAPI ExitBootServicesHook(EFI_HANDLE ImageHandle, UINTN MapKey);
	EFI_STATUS EFIAPI AllocatePagesHook(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);


	//
	// utils
	//
	VOID *AllocateImage(QWORD ImageBase);
}

#define Print(Text) \
{ \
unsigned short private_text[] = Text; \
gST->ConOut->OutputString(gST->ConOut, private_text); \
} \

extern "C" EFI_GUID gEfiLoadedImageProtocolGuid   = { 0x5B1B31A1, 0x9562, 0x11D2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }};
extern "C" EFI_GUID gEfiMemoryAttributesTableGuid = EFI_MEMORY_ATTRIBUTES_TABLE_GUID;

inline void PressAnyKey()
{
	EFI_STATUS         Status;
	EFI_EVENT          WaitList;
	EFI_INPUT_KEY      Key;
	UINTN              Index;
	Print(FILENAME L" " L"Press F11 key to continue . . .");
	do {
		WaitList = gST->ConIn->WaitForKey;
		Status = gBS->WaitForEvent(1, &WaitList, &Index);
		gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
		if (Key.ScanCode == SCAN_F11)
			break;
	} while ( 1 );
	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
}

inline QWORD get_nt_header(QWORD image)
{
	QWORD nt_header = (QWORD)*(DWORD*)(image + 0x03C) + image;
	if (nt_header == image)
	{
		return 0;
	}
	return nt_header;
}

typedef unsigned short WORD;
inline QWORD get_section_headers(QWORD nt_header)
{
	WORD machine = *(WORD*)(nt_header + 0x4);
	QWORD section_header = machine == 0x8664 ?
		nt_header + 0x0108 :
		nt_header + 0x00F8;
	return section_header;
}

inline VOID* TrampolineHook(VOID* dest, VOID* src, UINT8 original[14])
{
	if (original) {
		MemCopy(original, src, 14);
	}
	MemCopy(src, (void *)"\xFF\x25\x00\x00\x00\x00", 6);
	*(VOID**)((UINT8*)src + 6) = dest;
	return src;
}

inline VOID TrampolineUnHook(VOID* src, UINT8 original[14]) {
        MemCopy(src, original, 14);
}

extern "C" EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	gRT = SystemTable->RuntimeServices;
	gBS = SystemTable->BootServices;
	gST = SystemTable;


	EFI_LOADED_IMAGE_PROTOCOL *current_image;
	

	//
	// Get current image information
	//
	if (EFI_ERROR(gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&current_image)))
	{
		Print(FILENAME L" Failed to start " SERVICE_NAME L" service.");
		return 0;
	}


	//
	// our image is efi runtime driver btw :D
	//
	*(DWORD*)(((QWORD)current_image->ImageBase + *(DWORD*)((QWORD)current_image->ImageBase + 0x03C)) + 0x18 + 0x44) = 12;


	//
	// allocate space for SwapMemory
	//
	VOID *rwx = AllocateImage((QWORD)current_image->ImageBase);
	if (rwx == 0)
	{
		Print(FILENAME L" Compatibility version is required");
		return 0;
	}


	//
	// swap our context to new memory region
	//
	SwapMemory( (QWORD)current_image->ImageBase, (QWORD)current_image->ImageSize, (QWORD)rwx );


	//
	// clear old image from memory
	//
	for (QWORD i = current_image->ImageSize; i--;)
	{
		((unsigned char*)current_image->ImageBase)[i] = 0;
	}


	//
	// save our new EFI address information
	//
	EfiBaseAddress  = (QWORD)rwx;
	EfiBaseSize     = (QWORD)current_image->ImageSize;


	//
	// ExitBootServices: Output EFI status
	//
	oExitBootServices = gBS->ExitBootServices;
	gBS->ExitBootServices = ExitBootServicesHook;
	

	//
	// AllocatePages: winload/ntoskrnl.exe hooks
	//
	oAllocatePages = gBS->AllocatePages;
	gBS->AllocatePages = AllocatePagesHook;


	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);

	Print(FILENAME L" " SERVICE_NAME L" is now started");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 1);
	PressAnyKey();
	return EFI_SUCCESS;
}

namespace km
{
	BOOLEAN initialize(QWORD ntoskrnl);
}

extern "C" EFI_STATUS EFIAPI ExitBootServicesHook(EFI_HANDLE ImageHandle, UINTN MapKey)
{
	gBS->ExitBootServices = oExitBootServices;

	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);

	if (GlobalStatusVariable)
	{
		Print(FILENAME L" Success -> " SERVICE_NAME L" service is running.");
	}
	else
	{
		Print(FILENAME L" Failure -> Unsupported OS.");
	}
	
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 1);
	PressAnyKey();

	if (!GlobalStatusVariable)
	{
		return 0;
	}

	return gBS->ExitBootServices(ImageHandle, MapKey);
}

unsigned char BlMmMapPhysicalAddressExOriginal[14];
typedef __int64 (__fastcall *FnBlMmMapPhysicalAddressEx)(__int64 *a1, __int64 a2, unsigned __int64 a3, unsigned int a4, char a5);
static __int64 __fastcall BlMmMapPhysicalAddressExHook(__int64 *a1, __int64 a2, unsigned __int64 a3, unsigned int a4, char a5)
{
	TrampolineUnHook((void*)BlMmMapPhysicalAddressEx, BlMmMapPhysicalAddressExOriginal);
	__int64 status = BlMmMapPhysicalAddressEx(a1, a2, a3, a4, a5);

	if (status == 0 && a2 == (__int64)EfiBaseAddress)
	{
		QWORD EfiBaseVirtualAddress = *(QWORD*)(a1) - (EfiBaseAddress - a2);
		SwapMemory2(EfiBaseAddress, EfiBaseVirtualAddress);
		GlobalStatusVariable = km::initialize(ntoskrnl_base);
		SwapMemory2(EfiBaseVirtualAddress, EfiBaseAddress);
	}
	else
	{
		TrampolineHook((void *)BlMmMapPhysicalAddressExHook, (void *)BlMmMapPhysicalAddressEx, 0);
	}
	return status;
}

unsigned __int64 __fastcall RtlFindExportedRoutineByName(QWORD BaseAddress, const char *ExportName)
{
	if (!strcmp_imp(ExportName, "NtImageInfo"))
	{
		pe_resolve_imports(BaseAddress, EfiBaseAddress);
		ntoskrnl_base = BaseAddress;

		BlMmMapPhysicalAddressEx = (FnBlMmMapPhysicalAddressEx)TrampolineHook(
			(void *)BlMmMapPhysicalAddressExHook,
			(void*)BlMmMapPhysicalAddressEx,
			BlMmMapPhysicalAddressExOriginal
			);
	}
	return GetExportByName(BaseAddress, ExportName);
}

static DWORD *__fastcall GetMemoryAttributesTable(unsigned __int64 *a1)
{
	DWORD* v2; // rcx
	unsigned __int64 v3; // rax
	DWORD* v5; // [rsp+40h] [rbp+8h] BYREF
	__int64 v6 = (__int64)mTable; // [rsp+48h] [rbp+10h] BYREF

	*a1 = 4096i64;
	if ((int)BlMmMapPhysicalAddressEx((__int64*)&v5, v6, *a1, 0, 0) >= 0)
	{
		v2 = v5;
		if (*v5)
		{
			v3 = (unsigned int)(v5[1] * v5[2] + 16);
			*a1 = v3;
			if (v3 <= 0x1000)
				return v2;
			BlMmUnmapVirtualAddressEx((QWORD)v2, 4096i64, 0i64);
			if ((int)BlMmMapPhysicalAddressEx((__int64*)&v5, v6, *a1, 0, 0) >= 0)
				return v5;
		}
		else
		{
			BlMmUnmapVirtualAddressEx((QWORD)v5, *a1, 0i64);
		}
	}
	*a1 = 0i64;
	return 0i64;
}

extern "C" EFI_STATUS EFIAPI AllocatePagesHook(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory)
{
	QWORD return_address = (QWORD)_ReturnAddress();

	if (*(DWORD*)(return_address) != 0x48001F0F && *(DWORD*)(return_address) != 0x83F88B48)
		return oAllocatePages(Type, MemoryType, Pages, Memory);
	

	//
	// get winload.efi base
	//
	QWORD winload = get_caller_base(return_address);


	//
	// hook routine which is called later by winload.efi with ntoskrnl.exe
	//
	QWORD routine = GetExportByName(winload, "RtlFindExportedRoutineByName");
	if (!routine)
		return oAllocatePages(Type, MemoryType, Pages, Memory);

	*(QWORD*)(routine + 0x00) = 0x25FF;
	*(QWORD*)(routine + 0x06) = (QWORD)RtlFindExportedRoutineByName;


	//
	// hook routine for cloning ntoskrnl.exe file (Raw Version)
	//
	routine = GetExportByName(winload, "RtlImageNtHeaderEx");
	if (routine == 0)
	{
		routine = FindPattern(winload, (unsigned char *)"\x45\x33\xD2\x4D\x8B\xD8\x4D\x85\xC9", (unsigned char*)"xxxxxxxxx");
	}


	//
	// hook attributes table, we can return correct page attributes for the OS.
	//
	routine = GetExportByName(winload, "EfiGetMemoryAttributesTable");
	*(QWORD*)(routine + 0x00) = 0x25FF;
	*(QWORD*)(routine + 0x06) = (QWORD)GetMemoryAttributesTable;


	//
	// we need these routines for our attributes table hook
	//
	*(QWORD*)&BlMmMapPhysicalAddressEx  = GetExportByName(winload, "BlMmMapPhysicalAddressEx");
	*(QWORD*)&BlMmUnmapVirtualAddressEx = GetExportByName(winload, "BlMmUnmapVirtualAddressEx");


	//
	// save winload address for later use
	//
	winload_base = winload;

	
	//
	// unhook AllocatePages
	//
	gBS->AllocatePages = oAllocatePages;
	return oAllocatePages(Type, MemoryType, Pages, Memory);
}

__int64 __fastcall EfiGetSystemTable(QWORD SystemTable, QWORD* a1, QWORD* a2)
{
	unsigned int v2; // r8d
	unsigned int v3; // r9d
	unsigned __int64 v4; // r11
	__int64 v5; // r10
	__int64 v6; // rax

	v2 = 0;
	v3 = (unsigned int)-1073741275;
	v4 = *(QWORD*)((QWORD)SystemTable + 104);
	if (v4)
	{
		v5 = *(QWORD*)((QWORD)SystemTable + 112);
		v6 = 0i64;
		while (*(QWORD*)(v5 + 24 * v6) != *a1 || *(QWORD*)(v5 + 24 * v6 + 8) != a1[1])
		{
			v6 = ++v2;
			if (v2 >= v4)
				return v3;
		}
		v3 = 0;
		*a2 = *(QWORD*)(v5 + 24 * v6 + 16);
	}
	return v3;
}

extern "C" VOID *AllocateImage(QWORD ImageBase)
{
	//
	// get memory attributes table
	//
	EFI_MEMORY_ATTRIBUTES_TABLE *table=0;
	if (EfiGetSystemTable((QWORD)gST, (QWORD*)&gEfiMemoryAttributesTableGuid, (QWORD*)&table) != 0)
	{
		return 0;
	}


	//
	// copy current runtime image entries to new attributes table
	//
	QWORD table_size = sizeof (EFI_MEMORY_ATTRIBUTES_TABLE) + table->DescriptorSize * (table->NumberOfEntries + 3);
	gBS->AllocatePool (EfiBootServicesData, table_size, (void**)&mTable);
	MemCopy( (void *)mTable, (void*)table, sizeof (EFI_MEMORY_ATTRIBUTES_TABLE) + table->DescriptorSize * (table->NumberOfEntries));
	table = mTable;


	QWORD hdr_count = 1;
	QWORD cde_count = 0;
	QWORD dta_count = 0;


	QWORD nt = get_nt_header(ImageBase);
	QWORD sh = get_section_headers(nt);

	for (unsigned short i = 0; i < *(unsigned short*)(nt + 0x06); i++) {
		QWORD section = sh + ((QWORD)i * 40);
		DWORD section_characteristics = *(DWORD*)(section + 0x24);
		QWORD section_count = EFI_SIZE_TO_PAGES( *(DWORD*)(section + 0x08) );

		if (section_characteristics & 0x00000020)
		{
			cde_count += section_count;
		}
		else
		{
			dta_count += section_count;
		}
	}
	
	EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *)(table + 1);
	EFI_MEMORY_DESCRIPTOR *last_page = NEXT_MEMORY_DESCRIPTOR (entry, (table->DescriptorSize * (table->NumberOfEntries-1)));
	
	UINTN page_count  = hdr_count+cde_count+dta_count;
	QWORD target_addr = last_page->PhysicalStart + EFI_PAGES_TO_SIZE(last_page->NumberOfPages);

	while (1)
	{
		EFI_PHYSICAL_ADDRESS addr = target_addr;
		if (!EFI_ERROR(gBS->AllocatePages(AllocateAddress, EfiRuntimeServicesCode, page_count, &addr)))
		{
			break;
		}
		target_addr += EFI_PAGE_SIZE;
	}

	QWORD hdr_begin = target_addr;
	QWORD cde_begin = hdr_begin + EFI_PAGES_TO_SIZE(hdr_count);
	QWORD dta_begin = cde_begin + EFI_PAGES_TO_SIZE(cde_count);
	

	//
	// headers
	//
	entry = NEXT_MEMORY_DESCRIPTOR (entry, (table->DescriptorSize * table->NumberOfEntries));
	entry->PhysicalStart = hdr_begin;
	entry->NumberOfPages = hdr_count;
	entry->Type = EfiRuntimeServicesCode;
	entry->Attribute = EFI_MEMORY_RUNTIME | EFI_MEMORY_XP;
	entry->VirtualStart = 0;
	table->NumberOfEntries++;


	//
	// text
	//
	entry = NEXT_MEMORY_DESCRIPTOR (entry, table->DescriptorSize);
	entry->PhysicalStart = cde_begin;
	entry->NumberOfPages = cde_count;
	entry->Type = EfiRuntimeServicesCode;
	entry->Attribute = EFI_MEMORY_RUNTIME | EFI_MEMORY_RO;
	entry->VirtualStart = 0;
	table->NumberOfEntries++;


	//
	// data
	//
	entry = NEXT_MEMORY_DESCRIPTOR (entry, table->DescriptorSize);
	entry->PhysicalStart = dta_begin;
	entry->NumberOfPages = dta_count;
	entry->Type = EfiRuntimeServicesCode;
	entry->Attribute = EFI_MEMORY_RUNTIME | EFI_MEMORY_XP;
	entry->VirtualStart = 0;
	table->NumberOfEntries++;

	return (void *)target_addr;
}
