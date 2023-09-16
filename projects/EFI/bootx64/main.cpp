#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/UsbFunctionIo.h>
#include <Protocol/Smbios.h>
#include <intrin.h>
#include "globals.h"

extern "C"
{
	//
	// EFI common variables
	//
	EFI_SYSTEM_TABLE     *gST = 0;
	EFI_RUNTIME_SERVICES *gRT = 0;
	EFI_BOOT_SERVICES    *gBS = 0;

	//
	// EFI image variables
	//
	QWORD EfiBaseAddress        = 0;
	QWORD EfiBaseSize           = 0;
	DWORD GlobalStatusVariable  = 0;

	//
	// EFI global variables
	//
	EFI_GET_MEMORY_MAP oGetMemoryMap;
	EFI_ALLOCATE_PAGES oAllocatePages;
	EFI_EXIT_BOOT_SERVICES oExitBootServices;
	EFI_STATUS EFIAPI ExitBootServicesHook(EFI_HANDLE ImageHandle, UINTN MapKey);
	EFI_STATUS EFIAPI AllocatePagesHook(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);
	EFI_STATUS EFIAPI GetMemoryMapHook(UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR* MemoryMap, UINTN* MapKey, UINTN* DescriptorSize, UINT32* DescriptorVersion);
}

namespace config
{
	BOOL  rcs = 0;
	DWORD aimbot_button = 107;
	float aimbot_fov = 2.0f;
	float aimbot_smooth = 20.0f;
	BOOL  aimbot_visibility_check = 0;
	DWORD triggerbot_button = 111;
	BOOL  visuals_enabled = 1;
}

#define Print(Text) \
{ \
unsigned short private_text[] = Text; \
gST->ConOut->OutputString(gST->ConOut, private_text); \
} \

EFI_GUID gEfiLoadedImageProtocolGuid    = { 0x5B1B31A1, 0x9562, 0x11D2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }};

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
	// save image information
	//
	EfiBaseAddress = (QWORD)current_image->ImageBase;
	EfiBaseSize    = current_image->ImageSize;

	//
	// prevent gBS->StartImage for terminating us
	//
	*(QWORD *)((QWORD)current_image - 0x18) = EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
	

	//
	// GetMemoryMapHook: Reserve RAM space
	//
	oGetMemoryMap = gBS->GetMemoryMap;
	gBS->GetMemoryMap = GetMemoryMapHook;

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
	BOOLEAN initialize(void);
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

	return gBS->ExitBootServices(ImageHandle, MapKey);
}

unsigned __int64 __fastcall RtlFindExportedRoutineByName(QWORD BaseAddress, const char *ExportName)
{
	if (!strcmp_imp(ExportName, "NtImageInfo"))
	{
		pe_resolve_imports(BaseAddress, EfiBaseAddress);
		pe_clear_headers(EfiBaseAddress);

		__int64 (__fastcall *BlMmMapPhysicalAddressEx)(__int64 *a1, __int64 a2, unsigned __int64 a3, unsigned int a4, char a5);
		*(QWORD*)&BlMmMapPhysicalAddressEx = GetExportByName(get_caller_base((QWORD)_ReturnAddress()), "BlMmMapPhysicalAddressEx");
		
		QWORD EfiBaseVirtualAddress = EfiBaseAddress;	
		EFI_STATUS status = BlMmMapPhysicalAddressEx((__int64*)&EfiBaseVirtualAddress,
							EfiBaseVirtualAddress,
							EfiBaseSize,
							/*0x24000*/ 0x424000 /*0x1044008*/,
							0);
		
		if (!EFI_ERROR(status))
		{
			SwapMemory2(EfiBaseAddress, EfiBaseVirtualAddress);
			GlobalStatusVariable = km::initialize();
			SwapMemory2(EfiBaseVirtualAddress, EfiBaseAddress);
		}
	}
	return GetExportByName(BaseAddress, ExportName);
}

extern "C" EFI_STATUS EFIAPI AllocatePagesHook(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory)
{
	QWORD return_address = (QWORD)_ReturnAddress();
	if (*(DWORD*)(return_address) == 0x48001F0F)
	{
		QWORD target_routine = GetExportByName(get_caller_base(return_address), "RtlFindExportedRoutineByName");
		if (target_routine)
		{
			*(QWORD*)(target_routine + 0x00) = 0x25FF;
			*(QWORD*)(target_routine + 0x06) = (QWORD)RtlFindExportedRoutineByName;
			gBS->AllocatePages = oAllocatePages;
		}
	}
	return oAllocatePages(Type, MemoryType, Pages, Memory);
}

extern "C" EFI_STATUS EFIAPI GetMemoryMapHook(UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR* MemoryMap, UINTN* MapKey, UINTN* DescriptorSize, UINT32* DescriptorVersion)
{
	EFI_STATUS status = oGetMemoryMap(MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
	if (!EFI_ERROR(status))
	{
		VOID *map = MemoryMap;
		UINTN map_size = *MemoryMapSize;
		UINTN descriptor_size = *DescriptorSize;
		UINTN descriptor_count = map_size / descriptor_size;

		for (UINT32 i = 0; i < descriptor_count; i++)
		{
			EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR*)((char *)map + (i*descriptor_size));
			if (EfiBaseAddress >= entry->PhysicalStart && EfiBaseAddress <= (entry->PhysicalStart + EFI_PAGES_TO_SIZE(entry->NumberOfPages)))
			{
				entry->Type = EfiRuntimeServicesCode;
			}
		}
	}
	return status;
}

