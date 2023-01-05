#include "stdafx.h"
#include "map_driver_resource.h"

//
// efi common variables
//
EFI_SYSTEM_TABLE* gST;
EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;

//
// EFI global variables
//
void* g_efi_image_base;
QWORD                   g_efi_image_size;
int                     g_config_values[7 + 4];
LOADER_PARAMETER_BLOCK* gLoaderParameterBlock;
EFI_EXIT_BOOT_SERVICES  oExitBootServices;
VOID* gNotifyEvent;


//
// functions used at main
//
static BOOLEAN GetLicenceInformation(void*, DWORD, DWORD*, DWORD*, DWORD*, DWORD*);
EFI_STATUS EFIAPI ExitBootServicesHook(EFI_HANDLE, UINTN);
static DWORD GetCurrentHwid(void);
static EFI_STATUS EFIAPI MapDriver(VOID*, VOID*);



EFI_STATUS EFIAPI EfiMain(IN EFI_LOADED_IMAGE* LoadedImage, IN EFI_SYSTEM_TABLE* SystemTable)
{
	gRT = SystemTable->RuntimeServices;
	gBS = SystemTable->BootServices;
	gST = SystemTable;

	void* licence_data = LoadedImage->reserved;
	DWORD licence_length = LoadedImage->Revision;
	int* config_values = LoadedImage->LoadOptions;
	void* efi_image_base = LoadedImage->ImageBase;
	QWORD efi_image_size = LoadedImage->ImageSize;

	MemCopy(g_config_values, config_values, (7 * 4));
	g_efi_image_base = efi_image_base;
	g_efi_image_size = efi_image_size;

	DWORD licece_hwid = 0;
	if (!GetLicenceInformation(licence_data, licence_length, &licece_hwid, 0, 0, 0))
	{
		return 0;
	}

	g_config_values[7] = 0;
	g_config_values[8] = 0;
	g_config_values[9] = 0;
	g_config_values[10] = 0;

	if (licece_hwid != GetCurrentHwid())
	{
		g_config_values[10] = 1;
	}

	oExitBootServices = gBS->ExitBootServices;
	gBS->ExitBootServices = ExitBootServicesHook;

	Print(L"ekknod.xyz - CS:GO (freeware)\n");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 1);
	Print(L"ALERT: The original source of this software is: https://ekknod.xyz");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 2);
	Print(L"If you purchased this software, you have got scammed.");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 4);
	return EFI_SUCCESS;
}

VOID* (*_IoAllocateMdl)(VOID*, UINT32, BOOLEAN, BOOLEAN, VOID*);
VOID(*_MmProbeAndLockPages)(VOID*, UINT8, UINT32);
VOID* (*_MmMapLockedPagesSpecifyCache)(VOID*, UINT8, UINT32, VOID*, UINT32, UINT32);
VOID(*_MmUnlockPages)(VOID*);
VOID(*_IoFreeMdl)(VOID*);
VOID(*_MmUnmapLockedPages)(VOID*, VOID*);

static BOOLEAN MemCopyWP(VOID* dest, VOID* src, UINT32 length)
{
	VOID* mdl = _IoAllocateMdl(dest, length, FALSE, FALSE, NULL);
	if (mdl == 0)
		return FALSE;

	_MmProbeAndLockPages(mdl, 0, 2);
	VOID* mapped = _MmMapLockedPagesSpecifyCache(mdl, 0, 0, NULL, 0, 32);
	if (!mapped) {
		_MmUnlockPages(mdl);
		_IoFreeMdl(mdl);
		return FALSE;
	}

	for (UINT8* d = mapped, *s = src; length--; *d++ = *s++)
		;

	_MmUnmapLockedPages(mapped, mdl);
	_MmUnlockPages(mdl);
	_IoFreeMdl(mdl);
	return TRUE;
}

UINT8 DriverEntryOriginal[JMP_SIZE + 7];

NTSTATUS DriverEntryHook(PDRIVER_OBJECT DriverObject, VOID* RegistryPath)
{
	typedef NTSTATUS(*DRIVER_ENTRY)(PDRIVER_OBJECT, VOID*);
	DRIVER_ENTRY DriverEntry = 0;
	{
		QWORD base = (QWORD)DriverObject->DriverStart;
		IMAGE_NT_HEADERS64* nt_headers = (IMAGE_NT_HEADERS64*)((CHAR8*)base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
		*(VOID**)&DriverEntry = (VOID*)((CHAR8*)base + nt_headers->OptionalHeader.AddressOfEntryPoint);
	}


	if (!MemCopyWP((void*)DriverEntry, DriverEntryOriginal, JMP_SIZE + 7))
		*(int*)(0x10A0) = 0;


	/* call original driver entry */
	if (DriverEntry(DriverObject, RegistryPath) != 0)
		*(int*)(0x10A0) = 0;


	//
	// resolve driver base
	//
	QWORD efi_driver_base = 0;
	{
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)g_efi_image_base;
		IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((CHAR8*)dos + dos->e_lfanew);
		efi_driver_base = (QWORD)g_efi_image_base + nt->OptionalHeader.SizeOfImage;
	}


	IMAGE_NT_HEADERS64* nt_headers = (IMAGE_NT_HEADERS64*)((CHAR8*)efi_driver_base + ((IMAGE_DOS_HEADER*)efi_driver_base)->e_lfanew);
	NTSTATUS(__stdcall * entry)(PDRIVER_OBJECT, VOID*, VOID*);
	*(VOID**)&entry = (VOID*)((CHAR8*)efi_driver_base + nt_headers->OptionalHeader.AddressOfEntryPoint);



	/* jmp to entry, we can now clear UEFI driver there */
	return entry(DriverObject, g_efi_image_base, g_config_values);
}

#define CRC32_EXPORT(var, crc, crc_length) \
	*(QWORD*)&var = GetProcAddress((QWORD)ntoskrnl->ImageBase, crc, crc_length);


enum WinloadContext
{
	ApplicationContext,
	FirmwareContext
};

typedef void(__stdcall* BlpArchSwitchContext_t)(int target);
extern BlpArchSwitchContext_t BlpArchSwitchContext;


EFI_STATUS EFIAPI ExitBootServicesHook(
	IN  EFI_HANDLE                   ImageHandle,
	IN  UINTN                        MapKey
)
{
	gBS->ExitBootServices = oExitBootServices;
	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
	Print(L"ekknod.xyz - CS:GO");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 2);
	Print(L"Loading the driver . . .");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 4);

	if (g_config_values[10] == 1)
	{
		Print(L"Invalid HWID\n");
		gST->ConOut->SetCursorPosition(gST->ConOut, 0, 6);
		PressAnyKey();
		return oExitBootServices(ImageHandle, MapKey);
	}

	QWORD winload_base = get_winload_base((QWORD)_ReturnAddress());

	void* efi_base = g_efi_image_base;
	if (!scan_and_unlink(winload_base, efi_base, g_efi_image_size, &gLoaderParameterBlock, (QWORD*)&g_efi_image_base))
	{
		Print(L"Failed to start\n");
		gST->ConOut->SetCursorPosition(gST->ConOut, 0, 6);
		PressAnyKey();
		return 0;
	}

	BlpArchSwitchContext(ApplicationContext);

	KLDR_DATA_TABLE_ENTRY* ntoskrnl = GetModuleEntryCrc(&gLoaderParameterBlock->LoadOrderListHead, 0x59f44bf0, 26);
	KLDR_DATA_TABLE_ENTRY* acpiex = GetModuleEntryCrc(&gLoaderParameterBlock->LoadOrderListHead, 0x4840cee8, 22);
	CRC32_EXPORT(_IoAllocateMdl, 0x16833110, 14);
	CRC32_EXPORT(_MmProbeAndLockPages, 0xbc7855a4, 20);
	CRC32_EXPORT(_MmMapLockedPagesSpecifyCache, 0xd7bc7957, 29);
	CRC32_EXPORT(_MmUnlockPages, 0x85af60ed, 14);
	CRC32_EXPORT(_IoFreeMdl, 0xd0afdf3e, 10);
	CRC32_EXPORT(_MmUnmapLockedPages, 0xb38e8b38, 19);

	//
	// legacy boot
	//
	if (g_config_values[6] == 1)
	{
		gLoaderParameterBlock->FirmwareInformation.FirmwareTypeUefi = 0;
		gLoaderParameterBlock->FirmwareInformation.u.EfiInformation.EfiMemoryMap = 0;
	}

	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)g_efi_image_base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((CHAR8*)dos + dos->e_lfanew);
	void* manual_map_virtual_address = (void*)((CHAR8*)g_efi_image_base + nt->OptionalHeader.SizeOfImage);

	MapDriver(manual_map_virtual_address, ntoskrnl->ImageBase);
	MemCopy(DriverEntryOriginal, acpiex->EntryPoint, JMP_SIZE + 7);
	MemCopy(acpiex->EntryPoint, "\x4C\x8D\x05\xF9\xFF\xFF\xFF", 7);

	//
	// calculate new address for DriverEntryHook
	//
	void* DriverEntryPtr = (void*)((QWORD)g_efi_image_base + ((QWORD)DriverEntryHook - (QWORD)efi_base));
	TrampolineHook(DriverEntryPtr, (UINT8*)acpiex->EntryPoint + 7, NULL);

	BlpArchSwitchContext(FirmwareContext);

	Print(L"Succesfully loaded");
	gST->ConOut->SetCursorPosition(gST->ConOut, 0, 6);


	/*
	 * https://wikileaks.org/ciav7p1/cms/page_36896783.html
	 * In order to make sure your ExitBootServices call goes correctly,
	 * you will need to call GetMemoryMap first. Ironically,
	 * calling GetMemoryMap will require you to allocate memory for the map itself,
	 * which, in turn, will change the memory map.
	 *
	 * You can deal with this issue by looping your calls – allocating space for the map,
	 * then calling GetMemoryMap again. Eventually, you will have allocated enough space
	 * for the (again updated) map before you make the GetMemoryMap call, and you'll get the up-to-date map.
	*/

	UINTN MemoryMapSize;
	EFI_MEMORY_DESCRIPTOR* MemoryMap;
	UINTN LocalMapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	MemoryMap = NULL;
	MemoryMapSize = 0;
	EFI_STATUS Status;
	do {
		Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize, &DescriptorVersion);
		if (Status == EFI_BUFFER_TOO_SMALL) {
			gBS->AllocatePool(EfiBootServicesData, MemoryMapSize + 1, &MemoryMap);
			Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize, &DescriptorVersion);
		}
		else {
			/* Status is likely success - let the while() statement check success */
		}
	} while (Status != EFI_SUCCESS);

	return oExitBootServices(ImageHandle, LocalMapKey);
}

unsigned char key[] = "PRIV";
typedef UINT8 BYTE;
#define RELOC_FLAG(RelInfo) ((RelInfo >> 0x0C) == 10)


#define FIELD_OFFSET(type, field)    ((QWORD)&(((type *)0)->field))
#define IMAGE_FIRST_SECTION( ntheader ) ((IMAGE_SECTION_HEADER*)        \
    ((QWORD)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS64, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))


EFI_STATUS EFIAPI MapDriver(VOID* empty_rwx, VOID* ntoskrnl)
{
	for (DWORD i = 0; i < sizeof(map_driver_resource) / 4; i++)
		if (map_driver_resource[i] != 0)
			RC4(&map_driver_resource[i], 4, (unsigned char*)key, 4);

	void* binary = (void*)map_driver_resource;
	IMAGE_DOS_HEADER* binary_dos = (IMAGE_DOS_HEADER*)binary;
	IMAGE_NT_HEADERS64* binary_nt = (IMAGE_NT_HEADERS64*)((BYTE*)binary_dos + binary_dos->e_lfanew);
	BYTE* target_base = (BYTE*)empty_rwx;


	//
	// lets clear the memory, to avoid potential bugs
	//
	for (QWORD i = binary_nt->OptionalHeader.SizeOfImage; i--;)
	{
		((unsigned char*)target_base)[i] = 0;
	}


	MemCopy(
		target_base,
		binary,
		binary_nt->OptionalHeader.SizeOfHeaders
	);

	IMAGE_SECTION_HEADER* section_header = IMAGE_FIRST_SECTION(binary_nt);
	for (QWORD i = 0; i != binary_nt->FileHeader.NumberOfSections; ++i, ++section_header) {
		if (section_header->SizeOfRawData) {
			MemCopy(
				target_base + section_header->VirtualAddress,

				(BYTE*)binary + section_header->PointerToRawData,

				section_header->SizeOfRawData

			);
		}
	}

	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)target_base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((BYTE*)dos + dos->e_lfanew);
	IMAGE_OPTIONAL_HEADER64* opt = &nt->OptionalHeader;

	DWORD import_directory =
		nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
		.VirtualAddress;

	if (import_directory) {
		IMAGE_IMPORT_DESCRIPTOR* import_descriptor =
			(IMAGE_IMPORT_DESCRIPTOR*)(target_base + import_directory);



		for (; import_descriptor->FirstThunk; ++import_descriptor) {

			if (import_descriptor->FirstThunk == 0)
				break;

			IMAGE_THUNK_DATA64* thunk =
				(IMAGE_THUNK_DATA64*)(target_base +
					import_descriptor->FirstThunk);

			if (thunk == 0)
				break;

			if (import_descriptor->OriginalFirstThunk == 0)
				break;

			IMAGE_THUNK_DATA64* original_thunk =
				(IMAGE_THUNK_DATA64*)(target_base +
					import_descriptor->OriginalFirstThunk);

			for (; thunk->u1.AddressOfData; ++thunk, ++original_thunk) {
				UINT64 import = GetExport(
					(QWORD)ntoskrnl,
					((IMAGE_IMPORT_BY_NAME*)(target_base +
						original_thunk->u1.AddressOfData))
					->Name);

				thunk->u1.Function = import;
			}
		}

	}


	BYTE* LocationDelta = target_base - opt->ImageBase;
	if (LocationDelta) {
		if (opt->DataDirectory[5].Size) {
			IMAGE_BASE_RELOCATION* pRelocData = (IMAGE_BASE_RELOCATION*)(target_base + opt->DataDirectory[5].VirtualAddress);
			const IMAGE_BASE_RELOCATION* pRelocEnd = (IMAGE_BASE_RELOCATION*)((QWORD)(pRelocData)+opt->DataDirectory[5].Size);
			while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock) {
				QWORD AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
				UINT16* pRelativeInfo = (UINT16*)(pRelocData + 1);

				for (QWORD i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo) {
					if (RELOC_FLAG(*pRelativeInfo)) {
						QWORD* pPatch = (QWORD*)(target_base + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						*pPatch += (QWORD)(LocationDelta);
					}
				}
				pRelocData = (IMAGE_BASE_RELOCATION*)((BYTE*)(pRelocData)+pRelocData->SizeOfBlock);
			}
		}
	}


	UINT32 exportsRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	if (exportsRva) {
		IMAGE_EXPORT_DIRECTORY* exports =
			(IMAGE_EXPORT_DIRECTORY*)(target_base + exportsRva);

		if (exports->NumberOfNames) {
			UINT32* funcRva =
				(UINT32*)(target_base + exports->AddressOfFunctions);

			UINT16* ordinalRva =
				(UINT16*)(target_base + exports->AddressOfNameOrdinals);

			MemCopy(target_base + funcRva[ordinalRva[0]], g_config_values,
				sizeof(g_config_values));
		}
	}


	return EFI_SUCCESS;
}

char decryption_key[] = "k2h(mb4k>m7mbkh?bZ";
static BOOLEAN GetLicenceInformation(void* licence_data, DWORD licence_length,
	DWORD* hwid, DWORD* start_time, DWORD* end_time, DWORD* type)
{
	char message[260];
	for (int i = 260; i--;)
		message[i] = ((char*)licence_data)[i];


	for (int i = 0; i < 18; i++)
		decryption_key[i] = decryption_key[i] ^ 90;

	int message_length = licence_length;
	RC4(message, message_length, decryption_key, 17);

	for (int i = 0; i < message_length; i++) {
		if (message[i] == 'v')
		{
			message[i] = '0';
		}
		else if (message[i] == 'x')
		{
			message[i] = '1';
		}
		else if (message[i] == '%')
		{
			message[i] = '2';
		}
		else if (message[i] == 'y')
		{
			message[i] = '3';
		}
		else if (message[i] == '^')
		{
			message[i] = '4';
		}
		else if (message[i] == ',')
		{
			message[i] = '5';
		}
		else if (message[i] == 'q')
		{
			message[i] = '6';
		}
		else if (message[i] == '@')
		{
			message[i] = '7';
		}
		else if (message[i] == 'z')
		{
			message[i] = '8';
		}
		else if (message[i] == 'l')
		{
			message[i] = '9';
		}
		else if (message[i] == 'p')
		{
			message[i] = ';';
		}
	}

	int skip_status = 0;

	char* entry = message;
	const char* str_hwid = entry;
	entry = strchr_impl(message, ';');

	if (entry == 0) {
		skip_status = 1;
		goto skip;
	}

	*(char*)entry = 0, entry += 1;
	const char* time_begin = entry;
	entry = strchr_impl(entry, ';');

	if (entry == 0) {
		skip_status = 1;
		goto skip;
	}

	*(char*)entry = 0, entry += 1;
	const char* time_end = entry;
	entry = strchr_impl(entry, ';');

	if (entry == 0) {
		skip_status = 1;
		goto skip;
	}

	*(char*)entry = 0, entry += 1;
	const char* licence_type = entry;
skip:

	if (skip_status == 0) {
		if (hwid)
			*hwid = atoiq((char*)str_hwid);
		if (start_time)
			*start_time = atoiq((char*)time_begin);
		if (end_time)
			*end_time = atoiq((char*)time_end);
		if (type)
			*type = atoiq((char*)licence_type);

		for (int i = 260; i--;)
			message[i] = 0;

		return 1;
	}
	return 0;
}

static DWORD GetCurrentHwid(void)
{
	DWORD hwid = 0;

	char serial[2048];
	for (int i = 0; i < sizeof(serial); i++)
		serial[i] = 0;

	if (!get_usb_serial_number(serial)) {
		return 0;
	}

	hwid = hwid + crc32(serial, (DWORD)strleni(serial), 0x726D0100);

	DWORD motherboard_serial = 0;
	if (!get_motherboard_serial_number(&motherboard_serial)) {
		return 0;
	}

	hwid = hwid + motherboard_serial;

	return hwid;
}

