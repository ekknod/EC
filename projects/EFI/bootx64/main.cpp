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
	QWORD EfiBaseVirtualAddress = 0;
	QWORD EfiBaseAddress        = 0;
	QWORD EfiBaseSize           = 0;

	//
	// EFI global variables
	//
	EFI_EXIT_BOOT_SERVICES  oExitBootServices;
	EFI_STATUS EFIAPI ExitBootServicesHook(EFI_HANDLE ImageHandle,UINTN MapKey);
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


	UINTN page_count = EFI_SIZE_TO_PAGES (current_image->ImageSize);
	VOID  *rwx       = 0;


	//
	// allocate space for SwapMemory
	//
	if (EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesCode, page_count, (EFI_PHYSICAL_ADDRESS*)&rwx)))
	{
		Print(FILENAME L" Failed to start " SERVICE_NAME L" service.");
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
	// hook ExitBootServices
	//
	oExitBootServices = gBS->ExitBootServices;
	gBS->ExitBootServices = ExitBootServicesHook;

	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);


	Print(FILENAME L" " SERVICE_NAME L" is now started");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 1);
	PressAnyKey();
	return EFI_SUCCESS;
}

BOOLEAN EfiConvertPointer(QWORD loader_parameter_block, QWORD physical_address, QWORD *virtual_address)
{
	VOID *map = *(VOID**)(loader_parameter_block + 0x110 + 0x28);
	UINT32 map_size = *(UINT32*)(loader_parameter_block + 0x110 + 0x30);
	UINT32 descriptor_size = *(UINT32*)(loader_parameter_block + 0x110 + 0x34);
	UINT32 descriptor_count = map_size / descriptor_size;

	for (UINT32 i = 0; i < descriptor_count; i++)
	{
		EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR*)((char *)map + (i*descriptor_size));
		if (physical_address >= entry->PhysicalStart && physical_address <= (entry->PhysicalStart + EFI_PAGES_TO_SIZE(entry->NumberOfPages)))
		{
			INT64 delta = (INT64)physical_address - (INT64)entry->PhysicalStart;
			*virtual_address = (entry->VirtualStart + delta);
			return 1;
		}
	}
	return 0;
}

inline QWORD get_virtual_address(VOID *ptr)
{
	return (QWORD)((QWORD)EfiBaseVirtualAddress + ((QWORD)ptr - (QWORD)EfiBaseAddress));
}

namespace km
{
	BOOLEAN initialize(void);
}

BOOLEAN initialize_kernelmode(QWORD LoaderParameterBlock)
{
	QWORD ntoskrnl = GetModuleEntry((LIST_ENTRY*)(LoaderParameterBlock + 0x10), L"ntoskrnl.exe");
	if (!ntoskrnl)
	{
		return 0;
	}
	if (!pe_resolve_imports(ntoskrnl, EfiBaseVirtualAddress))
	{
		return 0;
	}
	pe_clear_headers(EfiBaseVirtualAddress);
	return ((BOOLEAN(*)(void))(get_virtual_address((VOID*)km::initialize)))();
}

extern "C" EFI_STATUS EFIAPI ExitBootServicesHook(EFI_HANDLE ImageHandle,UINTN MapKey)
{
	gBS->ExitBootServices = oExitBootServices;

	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);


	QWORD winload_base = get_winload_base((QWORD)_ReturnAddress());
	if (winload_base == 0)
	{
		//
		// failed to load
		//
		return 0;
	}

	
	enum WinloadContext
	{
		ApplicationContext,
		FirmwareContext
	};

	typedef void(__stdcall* BlpArchSwitchContext_t)(int target);
	BlpArchSwitchContext_t BlpArchSwitchContext;
	BlpArchSwitchContext = (BlpArchSwitchContext_t)(FindPattern(winload_base, (unsigned char*)"\x40\x53\x48\x83\xEC\x20\x48\x8B\x15", (unsigned char*)"xxxxxxxxx"));
	if (BlpArchSwitchContext == 0)
	{
		return 0;
	}

	QWORD loader_parameter_block = get_loader_block(winload_base);
	BlpArchSwitchContext(ApplicationContext);

	BOOLEAN status = 0;
	if (EfiConvertPointer(loader_parameter_block, EfiBaseAddress, &EfiBaseVirtualAddress))
	{
		status = initialize_kernelmode(loader_parameter_block);
	}

	BlpArchSwitchContext(FirmwareContext);

	if (status)
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

