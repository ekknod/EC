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
QWORD get_pe_entrypoint(QWORD base);

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


	UINTN page_count = EFI_SIZE_TO_PAGES (SIZE_4MB);
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
	EfiBaseAddress = (QWORD)rwx;
	EfiBaseSize    = (QWORD)current_image->ImageSize;


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

struct _I386_LOADER_BLOCK
{
	VOID* CommonDataArea;                                                   //0x0
	UINT32 MachineType;                                                      //0x8
	UINT32 VirtualBias;                                                      //0xc
};

struct _ARM_LOADER_BLOCK
{
	UINT32 PlaceHolder;                                                      //0x0
};

struct _EFI_FIRMWARE_INFORMATION
{
	UINT32 FirmwareVersion;                                                 //0x0
	struct _VIRTUAL_EFI_RUNTIME_SERVICES* VirtualEfiRuntimeServices;        //0x8
	UINT32 SetVirtualAddressMapStatus;                                      //0x10
	UINT32 MissedMappingsCount;                                             //0x14
	struct _LIST_ENTRY FirmwareResourceList;                                //0x18
	VOID* EfiMemoryMap;                                                     //0x28
	UINT32 EfiMemoryMapSize;                                                 //0x30
	UINT32 EfiMemoryMapDescriptorSize;                                       //0x34
};

struct _PCAT_FIRMWARE_INFORMATION
{
	UINT32 PlaceHolder;                                                      //0x0
};

struct _FIRMWARE_INFORMATION_LOADER_BLOCK
{
	UINT32 FirmwareTypeUefi:1;                                               //0x0
	UINT32 EfiRuntimeUseIum:1;                                               //0x0
	UINT32 EfiRuntimePageProtectionSupported:1;                              //0x0
	UINT32 Reserved:29;                                                      //0x0
	union
	{
		struct _EFI_FIRMWARE_INFORMATION EfiInformation;                 //0x8
		struct _PCAT_FIRMWARE_INFORMATION PcatInformation;               //0x8
	} u;                                                                     //0x8
};

enum _TYPE_OF_MEMORY
{
    LoaderExceptionBlock = 0,
    LoaderSystemBlock = 1,
    LoaderFree = 2,
    LoaderBad = 3,
    LoaderLoadedProgram = 4,
    LoaderFirmwareTemporary = 5,
    LoaderFirmwarePermanent = 6,
    LoaderOsloaderHeap = 7,
    LoaderOsloaderStack = 8,
    LoaderSystemCode = 9,
    LoaderHalCode = 10,
    LoaderBootDriver = 11,
    LoaderConsoleInDriver = 12,
    LoaderConsoleOutDriver = 13,
    LoaderStartupDpcStack = 14,
    LoaderStartupKernelStack = 15,
    LoaderStartupPanicStack = 16,
    LoaderStartupPcrPage = 17,
    LoaderStartupPdrPage = 18,
    LoaderRegistryData = 19,
    LoaderMemoryData = 20,
    LoaderNlsData = 21,
    LoaderSpecialMemory = 22,
    LoaderBBTMemory = 23,
    LoaderZero = 24,
    LoaderXIPRom = 25,
    LoaderHALCachedMemory = 26,
    LoaderLargePageFiller = 27,
    LoaderErrorLogMemory = 28,
    LoaderVsmMemory = 29,
    LoaderFirmwareCode = 30,
    LoaderFirmwareData = 31,
    LoaderFirmwareReserved = 32,
    LoaderEnclaveMemory = 33,
    LoaderFirmwareKsr = 34,
    LoaderEnclaveKsr = 35,
    LoaderSkMemory = 36,
    LoaderSkFirmwareReserved = 37,
    LoaderIoSpaceMemoryZeroed = 38,
    LoaderIoSpaceMemoryFree = 39,
    LoaderIoSpaceMemoryKsr = 40,
    LoaderMaximum = 41
};

typedef struct _MEMORY_ALLOCATION_DESCRIPTOR
{
	struct _LIST_ENTRY ListEntry;                                       //0x0
	enum _TYPE_OF_MEMORY MemoryType;                                    //0x10
	UINTN BasePage;                                                     //0x18
	UINTN PageCount;                                                    //0x20
} MEMORY_ALLOCATION_DESCRIPTOR, *PMEMORY_ALLOCATION_DESCRIPTOR  ; 

typedef struct _LOADER_PARAMETER_BLOCK {
	UINT32 OsMajorVersion;                                                   //0x0
	UINT32 OsMinorVersion;                                                   //0x4
	UINT32 Size;                                                             //0x8
	UINT32 OsLoaderSecurityVersion;                                          //0xc
	struct _LIST_ENTRY LoadOrderListHead;                                   //0x10
	struct _LIST_ENTRY MemoryDescriptorListHead;                            //0x20
	struct _LIST_ENTRY BootDriverListHead;                                  //0x30
	struct _LIST_ENTRY EarlyLaunchListHead;                                 //0x40
	struct _LIST_ENTRY CoreDriverListHead;                                  //0x50
	struct _LIST_ENTRY CoreExtensionsDriverListHead;                        //0x60
	struct _LIST_ENTRY TpmCoreDriverListHead;                               //0x70
	UINT64 KernelStack;                                                     //0x80
	UINT64 Prcb;                                                            //0x88
	UINT64 Process;                                                         //0x90
	UINT64 Thread;                                                          //0x98
	UINT32 KernelStackSize;                                                 //0xa0
	UINT32 RegistryLength;                                                  //0xa4
	VOID* RegistryBase;                                                     //0xa8
	struct _CONFIGURATION_COMPONENT_DATA* ConfigurationRoot;                //0xb0
	char* ArcBootDeviceName;                                                //0xb8
	char* ArcHalDeviceName;                                                 //0xc0
	char* NtBootPathName;                                                   //0xc8
	char* NtHalPathName;                                                    //0xd0
	char* LoadOptions;                                                      //0xd8
	struct _NLS_DATA_BLOCK* NlsData;                                        //0xe0
	struct _ARC_DISK_INFORMATION* ArcDiskInformation;                       //0xe8
	struct _LOADER_PARAMETER_EXTENSION* Extension;                          //0xf0
	union
	{
		struct _I386_LOADER_BLOCK I386;                                         //0xf8
		struct _ARM_LOADER_BLOCK Arm;                                           //0xf8
	} u;                                                                    //0xf8
	struct _FIRMWARE_INFORMATION_LOADER_BLOCK FirmwareInformation;          //0x108
	char* OsBootstatPathName;                                               //0x148
	char* ArcOSDataDeviceName;                                              //0x150
	char* ArcWindowsSysPartName;                                            //0x158
} LOADER_PARAMETER_BLOCK  ;

#define CONTAINING_RECORD(address, type, field) ((type *)((UINT8 *)(address) - (UINTN)(&((type *)0)->field)))

BOOLEAN GetEfiVirtualAddress(
	LOADER_PARAMETER_BLOCK* LoaderParameterBlock,
	VOID *Address,
	UINTN Length,
	QWORD *VirtualAddress
	)
{

	LIST_ENTRY *list = &LoaderParameterBlock->MemoryDescriptorListHead;
	while ((list = list->ForwardLink) != &LoaderParameterBlock->MemoryDescriptorListHead) {
		PMEMORY_ALLOCATION_DESCRIPTOR entry = CONTAINING_RECORD(list, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
		UINTN addr = entry->BasePage * 0x1000;
		UINTN addr_length = EFI_PAGES_TO_SIZE(entry->PageCount);

		if ((UINTN)Address >= addr &&  (UINTN)((UINTN)Address + Length) <= (addr + addr_length))
		{
			LIST_ENTRY *previous_entry = list->BackLink;
			LIST_ENTRY *next_entry = list->ForwardLink;
			previous_entry->ForwardLink = list->ForwardLink;
			next_entry->BackLink = list->BackLink;
		}
	}

	VOID *map = LoaderParameterBlock->FirmwareInformation.u.EfiInformation.EfiMemoryMap;
	UINT32 map_size = LoaderParameterBlock->FirmwareInformation.u.EfiInformation.EfiMemoryMapSize;
	UINT32 descriptor_size = LoaderParameterBlock->FirmwareInformation.u.EfiInformation.EfiMemoryMapDescriptorSize;
	UINT32 descriptor_count = map_size / descriptor_size;

	EFI_MEMORY_DESCRIPTOR *previous_entry = (EFI_MEMORY_DESCRIPTOR*)map;
	BOOLEAN copy_operation = 0;


	/* check if it's last item, then we don't need to move any pages */
	EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR*)((char *)map + ((descriptor_count - 1) * descriptor_size));
	UINTN addr = entry->PhysicalStart;
	UINTN addr_length = (entry->NumberOfPages * 0x1000);
	if ((UINTN)Address >= addr &&  (UINTN)((UINTN)Address + Length) <= (addr + addr_length)) {
		*VirtualAddress = entry->VirtualStart;
		copy_operation = 1;
		goto skip_remap;
	}

	for (UINT32 i = 0; i < descriptor_count; i++) {
		entry = (EFI_MEMORY_DESCRIPTOR*)((char *)map + (i*descriptor_size));

		if (copy_operation == 0) {
			addr = previous_entry->PhysicalStart;
			addr_length = EFI_PAGES_TO_SIZE(previous_entry->NumberOfPages);

			if ((UINTN)Address >= addr &&  (UINTN)((UINTN)Address + Length) <= (addr + addr_length)) {
				*VirtualAddress = previous_entry->VirtualStart;
				copy_operation = 1;
			}
		}

		if (copy_operation) {
			/* if it's first item, we don't need to copy it */
			if (previous_entry != entry) {
				MemCopy(previous_entry, entry, sizeof(EFI_MEMORY_DESCRIPTOR));
			}
		}

		previous_entry = entry;
	}
skip_remap:
	if (copy_operation == 1)
		LoaderParameterBlock->FirmwareInformation.u.EfiInformation.EfiMemoryMapSize -= descriptor_size;
	return copy_operation;
}

inline QWORD get_virtual_address(VOID *ptr)
{
	return (QWORD)((QWORD)EfiBaseVirtualAddress + ((QWORD)ptr - (QWORD)EfiBaseAddress));
}

namespace km
{
	BOOLEAN initialize(void);
}

BOOLEAN InitializeKernel(LOADER_PARAMETER_BLOCK *LoaderParameterBlock)
{
	QWORD ntoskrnl = GetModuleEntry(&LoaderParameterBlock->LoadOrderListHead, L"ntoskrnl.exe");
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

	BOOLEAN status = 0;

	QWORD winload_base = get_winload_base((QWORD)_ReturnAddress());
	if (winload_base == 0)
	{
		//
		// failed to load
		//
		goto E0;
	}

	{
		enum WinloadContext
		{
			ApplicationContext,
			FirmwareContext
		};

		typedef void(__stdcall* BlpArchSwitchContext_t)(int target);
		BlpArchSwitchContext_t BlpArchSwitchContext;

		unsigned char bytes_3[] = { 'x','x','x','?','?','?','?','x','x','x','?','?','?','?',0 };

		UINT64 loaderBlockScan = (UINT64)FindPattern(winload_base,
			(unsigned char*)"\x48\x8B\x3D\x00\x00\x00\x00\x48\x8B\x8F\x00\x00\x00\x00", bytes_3);
		if (loaderBlockScan == 0) {

			/* 1909 */
			unsigned char bytes_2[] = { 'x','x','x','x','x','?','?','?','?',0 };
			loaderBlockScan = (UINT64)FindPattern(winload_base,
				(unsigned char*)"\x0F\x31\x48\x8B\x3D\x00\x00\x00\x00", bytes_2);
			if (loaderBlockScan != 0)
				loaderBlockScan += 2;

			/* 1809 */
			unsigned char bytes_1809[] = { 'x','x','x','?','?','?','?','x','x','x',0 };
			if (loaderBlockScan == 0)
				loaderBlockScan = (UINT64)FindPattern(winload_base,
					(unsigned char*)"\x48\x8B\x3D\x00\x00\x00\x00\x48\x8B\xCF", bytes_1809);

			unsigned char bytes_1[] = { 'x','x','x','?','?','?','?','x','x','x','x',0 };
			/* 1607 */
			if (loaderBlockScan == 0) {
				loaderBlockScan = (UINT64)FindPattern(winload_base,
					(unsigned char*)"\x48\x8B\x35\x00\x00\x00\x00\x48\x8B\x45\xF7", bytes_1);
			}

			if (loaderBlockScan == 0) {
				goto E0;
			}
		}

		UINT64 resolvedAddress = *(UINT64*)((loaderBlockScan + 7) + *(int*)(loaderBlockScan + 3));
		if (resolvedAddress == 0) {
			goto E0;
		}

		unsigned char bytes_0[] = { 'x','x','x','x','x','x','x','x','x',0 };
		BlpArchSwitchContext = (BlpArchSwitchContext_t)(FindPattern(winload_base,
			(unsigned char*)"\x40\x53\x48\x83\xEC\x20\x48\x8B\x15", bytes_0));
		if (BlpArchSwitchContext == 0) {
			goto E0;
		}

		BlpArchSwitchContext(ApplicationContext);

		LOADER_PARAMETER_BLOCK* loaderBlock = (LOADER_PARAMETER_BLOCK*)(resolvedAddress);

		if (GetEfiVirtualAddress(loaderBlock, (VOID*)EfiBaseAddress, EfiBaseSize, &EfiBaseVirtualAddress))
		{
			status = InitializeKernel(loaderBlock);
		}


		BlpArchSwitchContext(FirmwareContext);
	}
E0:
	if (status) {
		Print(FILENAME L" Success -> " SERVICE_NAME L" service is running.");
	} else {
		Print(FILENAME L" Failure -> Unsupported OS.");
	}

	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 1);
	PressAnyKey();

	UINTN MemoryMapSize;
	EFI_MEMORY_DESCRIPTOR* MemoryMap;
	UINTN LocalMapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	MemoryMap = 0;
	MemoryMapSize = 0;
	EFI_STATUS Status;
	do {
		Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize, &DescriptorVersion);
		if (Status == EFI_BUFFER_TOO_SMALL) {
			gBS->AllocatePool(EfiBootServicesData, MemoryMapSize + 1, (void **)&MemoryMap);
			Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize, &DescriptorVersion);
		}
		else {
			/* Status is likely success - let the while() statement check success */
		}
	} while (Status != EFI_SUCCESS);
	return oExitBootServices(ImageHandle, LocalMapKey);
}

