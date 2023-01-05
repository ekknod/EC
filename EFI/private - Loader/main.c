#include "stdafx.h"
//
// bootx64.efi
//

const UINT32 _gUefiDriverRevision = 0x200;
CHAR8 *gEfiCallerBaseName = "";

UINTN
EFIAPI
Atoi (
  CHAR8  *Str
  )
{
  UINTN   RetVal;
  CHAR16  TempChar;
  UINTN   MaxVal;
  UINTN   ResteVal;


  if (Str == 0)
	return 0;

  MaxVal   = (UINTN)-1 / 10;
  ResteVal = (UINTN)-1 % 10;
  //
  // skip preceeding white space
  //
  while (*Str != '\0' && *Str == ' ') {
    Str += 1;
  }

  //
  // convert digits
  //
  RetVal   = 0;
  TempChar = *(Str++);
  while (TempChar != '\0') {
    if ((TempChar >= '0') && (TempChar <= '9')) {
      if ((RetVal > MaxVal) || ((RetVal == MaxVal) && (TempChar - '0' > (INTN)ResteVal))) {
        return (UINTN)-1;
      }

      RetVal = (RetVal * 10) + TempChar - '0';
    } else {
      break;
    }

    TempChar = *(Str++);
  }

  return RetVal;
}

static char *find_sub(const char *str, const char *sub)
{
	UINTN len = AsciiStrLen(sub) + 1;
	return AsciiStrStr(str, sub) + len;
}

static void color(int color)
{
	gST->ConOut->SetAttribute(gST->ConOut, color | EFI_BACKGROUND_BLACK);
}

static void gotoxy(int x, int y)
{
	gST->ConOut->SetCursorPosition(gST->ConOut, x, y);
}

static int _getch()
{
	EFI_STATUS         Status;
	EFI_EVENT          WaitList;
	EFI_INPUT_KEY      Key;
	UINTN              Index;
	WaitList = gST->ConIn->WaitForKey;
	Status = gBS->WaitForEvent(1, &WaitList, &Index);
	gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

	
	return Key.ScanCode + Key.UnicodeChar;
}

static int Menu(void)
{
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
	gST->ConOut->ClearScreen(gST->ConOut);

	Print(L"ekknod.xyz - Menu\n");
	
	int Set[] = { EFI_CYAN, EFI_WHITE, EFI_WHITE };
	int counter = 1;
	int key = 0;

	for (;;) {
		
		gotoxy(0, 3);
		color(Set[0]);
		Print(L"1. Apex Legends ");

		gotoxy(0, 4);
		color(Set[1]);
		Print(L"2. CS:GO ");

		gotoxy(0, 5);
		color(Set[2]);
		Print(L"3. HWID Tool ");

		gotoxy(0, 7);
		color(EFI_WHITE);


		key = _getch();
		if (key == SCAN_UP) // 72 uparrow (windows)
		{
			counter--;
		}

		if (key == SCAN_DOWN) // 80 uparrow (windows)
		{
			counter ++;
		}

		if (counter < 1)
			counter = 1;

		if (counter > 3)
			counter = 3;
	

		if (key == 0x000D)
		{
			if (counter == 1) {
				if (FileExists(L"apex.bin"))
					break;
				else
					Print(L"apex.bin was not found\n");

			}
			if (counter == 2) {		
				if (FileExists(L"csgo.bin"))
					break;
				else
					Print(L"csgo.bin was not found\n");

			}
			if (counter == 3) {
				if (FileExists(L"hwid.bin"))
					break;
				else
					Print(L"hwid.bin was not found\n");
			}
		}
		Set[0] = EFI_WHITE;
		Set[1] = EFI_WHITE;
		Set[2] = EFI_WHITE;

		if (counter == 1) {
			Set[0] = EFI_CYAN;
		}
		if (counter == 2) {
			Set[1] = EFI_CYAN;
		}
		if (counter == 3) {
			Set[2] = EFI_CYAN;
		}	
	}

	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
	gST->ConOut->ClearScreen(gST->ConOut);
	gotoxy(0, 0);
	return counter;
}

typedef UINT8 BYTE;
#define RELOC_FLAG(RelInfo) ((RelInfo >> 0x0C) == 10)
#define FIELD_OFFSET(type, field)    ((QWORD)&(((type *)0)->field))
#define IMAGE_FIRST_SECTION( ntheader ) ((IMAGE_SECTION_HEADER*)        \
    ((QWORD)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS64, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
	int vMenu = Menu();

	EFI_STATUS status;

        void *binary = 0;
        UINTN binary_size;




	DWORD licence_length = 0;

	int config_values[7];
	char registeration_key[260];

	for (int i = 260; i--;)
		registeration_key[i] = 0;

	if (vMenu == 1)
	{
		void *config;
		UINTN config_size;

		status = LoadFile(L"apex.txt", &config, &config_size);
		if (EFI_ERROR(status)) {	
			Print(L"apex.txt file not found\n");
			PressAnyKey();
			return status;
		}

		config_values[0] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::enabled"));
		config_values[1] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::key"));
		config_values[2] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::fov"));
		config_values[3] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::smooth"));
		config_values[4] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::visibleOnly"));
		config_values[5] = (int)Atoi(find_sub((CHAR8*)config, "visuals::enabled"));

		gBS->FreePool(config);



		status = LoadFile(L"ECLicence.dat", &config, &config_size);
		if (EFI_ERROR(status)) {	
			Print(L"ECLicence.dat not found\n");
			Print(L"Download licence at: https://ekknod.xyz/index.php?action=licence\n");
			PressAnyKey();
			return status;
		}
		if (config_size > 260)
			config_size = 260;

		licence_length = (DWORD)config_size;

		MemCopy(registeration_key, config, config_size);
		gBS->FreePool(config);


		status = LoadFile(L"apex.bin", &binary, &binary_size);
		if (EFI_ERROR(status)) {
			Print(L"Failed to load apex.bin\n");
			PressAnyKey();
			return EFI_INVALID_PARAMETER;
		}



	}

	if (vMenu == 2)
	{
		void *config;
		UINTN config_size;

		status = LoadFile(L"csgo.txt", &config, &config_size);
		if (EFI_ERROR(status)) {	
			Print(L"csgo.txt file not found\n");
			PressAnyKey();
			return status;
		}

		config_values[0] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::fov"));
		config_values[1] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::smooth"));
		config_values[2] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::rcs"));
		config_values[3] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::key"));
		config_values[4] = (int)Atoi(find_sub((CHAR8*)config, "aimbot::visibleOnly"));
		config_values[5] = (int)Atoi(find_sub((CHAR8*)config, "triggerbot::key"));
		config_values[6] = (int)Atoi(find_sub((CHAR8*)config, "security::legacy"));

		gBS->FreePool(config);


		status = LoadFile(L"ECLicence.dat", &config, &config_size);
		if (EFI_ERROR(status)) {	
			Print(L"ECLicence.dat not found\n");
			Print(L"Download licence at: https://ekknod.xyz/index.php?action=licence\n");
			PressAnyKey();
			return status;
		}
		if (config_size > 260)
			config_size = 260;

		licence_length = (DWORD)config_size;

		MemCopy(registeration_key, config, config_size);
		gBS->FreePool(config);

		status = LoadFile(L"csgo.bin", &binary, &binary_size);
		if (EFI_ERROR(status)) {
			Print(L"Failed to load csgo.bin\n");
			PressAnyKey();
			return EFI_INVALID_PARAMETER;
		}
	}

	if (vMenu == 3)
	{
		status = LoadFile(L"hwid.bin", &binary, &binary_size);
		if (EFI_ERROR(status)) {
			Print(L"Failed to load hwid.bin\n");
			PressAnyKey();
			return EFI_INVALID_PARAMETER;
		}
	
	}

	if (binary == 0)
	{
		Print(L"Failed to load\n");
		PressAnyKey();
		return EFI_INVALID_PARAMETER;
	}

	EFI_LOADED_IMAGE image;	
	void *empty_rwx = (void *)(0);
	UINTN pages = EFI_SIZE_TO_PAGES (SIZE_8MB);

	if (EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesCode, pages, (EFI_PHYSICAL_ADDRESS*)&empty_rwx))) {
		Print(L"Failed to allocate pages\n");
		PressAnyKey();
		return EFI_INVALID_PARAMETER;
	}


	image.Revision = licence_length;	
	image.reserved = (VOID*)registeration_key;
	image.LoadOptions = (VOID*)config_values;
	image.ImageBase = empty_rwx;
	image.ImageSize = EFI_PAGES_TO_SIZE(pages);
	image.DeviceHandle = ImageHandle;

	IMAGE_DOS_HEADER *binary_dos = (IMAGE_DOS_HEADER*)binary;
	IMAGE_NT_HEADERS64 *binary_nt = (IMAGE_NT_HEADERS64*)((BYTE*)binary_dos + binary_dos->e_lfanew);
	BYTE *target_base = (BYTE*)empty_rwx;


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

	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)target_base;
	IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((BYTE*)dos + dos->e_lfanew);
	IMAGE_OPTIONAL_HEADER64 *opt = &nt->OptionalHeader;
	BYTE* delta = target_base - opt->ImageBase;
	if (delta) {
		if (opt->DataDirectory[5].Size) {
			IMAGE_BASE_RELOCATION* pRelocData = (IMAGE_BASE_RELOCATION*)(target_base + opt->DataDirectory[5].VirtualAddress);
			const IMAGE_BASE_RELOCATION* pRelocEnd = (IMAGE_BASE_RELOCATION*)((QWORD)(pRelocData) + opt->DataDirectory[5].Size);
			while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock) {
				QWORD count = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
				UINT16* pRelativeInfo = (UINT16*)(pRelocData + 1);

				for (QWORD i = 0; i != count; ++i, ++pRelativeInfo) {
					if (RELOC_FLAG(*pRelativeInfo)) {
						QWORD* pPatch = (QWORD*)(target_base + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
						*pPatch += (QWORD)(delta);
					}
				}
				pRelocData = (IMAGE_BASE_RELOCATION*)((BYTE*)(pRelocData) + pRelocData->SizeOfBlock);
			}
		}
	}

	gBS->FreePool(binary);

	
	EFI_STATUS (EFIAPI *EntryPoint)(IN EFI_LOADED_IMAGE *LoadedImage, IN EFI_SYSTEM_TABLE *SystemTable);
	*(UINTN*)&EntryPoint = (UINTN)(target_base + nt->OptionalHeader.AddressOfEntryPoint);
	status = EntryPoint(&image, SystemTable);


	
	{
		EFI_STATUS         Status;
		EFI_EVENT          WaitList;
		EFI_INPUT_KEY      Key;
		UINTN              Index;

		Print(L"Please unplug (USB stick)\n");
		Print(L"Press any key to continue . . .\n");
		
		do {
			WaitList = gST->ConIn->WaitForKey;
			Status = gBS->WaitForEvent(1, &WaitList, &Index);
			gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
			if ((Key.UnicodeChar + Key.ScanCode) != 0)
				break;

			

		} while ( 1 );
		gST->ConOut->ClearScreen(gST->ConOut);
		gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
	}	


	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);

	return status;
}

EFI_STATUS EFIAPI UefiUnload(EFI_HANDLE ImageHandle)
{
	return EFI_SUCCESS;
}

