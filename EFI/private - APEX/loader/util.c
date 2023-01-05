#include "stdafx.h"

QWORD strleni(const char *s)
{
	const char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

int atoiq(char *s)
{
	int acum = 0;
	while((*s >= '0')&&(*s <= '9')) {
		acum = acum * 10;
		acum = acum + (*s - 48);
		s++;
	}
	return (acum);
}

char *strchr_impl(const char *s, int c)
{
	
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;
	
	return (char *)s;
}

VOID MemCopy(VOID* dest, VOID* src, UINTN size)
{
	for (UINT8* d = dest, *s = src; size--; *d++ = *s++)
		;
}

static BOOLEAN CheckMask(unsigned char* base, unsigned char* pattern, unsigned char* mask)
{
	for (; *mask; ++base, ++pattern, ++mask)
		if (*mask == 'x' && *base != *pattern)
			return FALSE;
	return TRUE;
}

VOID *FindPattern(unsigned char* base, UINTN size, unsigned char* pattern, unsigned char* mask)
{
	size -= strleni(mask);
	for (UINTN i = 0; i <= size; ++i) {
		VOID* addr = &base[i];
		if (CheckMask(addr, pattern, mask))
			return addr;
	}
	return NULL;
}

VOID* TrampolineHook(VOID* dest, VOID* src, UINT8 original[JMP_SIZE])
{
	if (original)
		MemCopy(original, src, JMP_SIZE);
	MemCopy(src, "\xFF\x25\x00\x00\x00\x00", 6);
	*(VOID**)((UINT8*)src + 6) = dest;
	return src;
}

VOID TrampolineUnHook(VOID* src, UINT8 original[JMP_SIZE])
{
	MemCopy(src, original, JMP_SIZE);
}


static BOOLEAN IsAddressEqual(UINTN address0, UINTN address2)
{
	INTN res = ABS(  (INTN)(address2 - address0)  );
	return res <= 0x1000;
}

static BOOLEAN UnlinkEfiMemoryMap(
	LOADER_PARAMETER_BLOCK* LoaderParameterBlock,
	VOID *Address,
	UINTN Length,
	QWORD *VirtualAddress
	)
{
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

static BOOLEAN UnlinkDescriptorList(LOADER_PARAMETER_BLOCK *LoaderParameterBlock, VOID *Address, UINTN Length)
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
			return 1;
		}
	}
	return 0;
}

DWORD g_encryption_key = 0x726D0100;
DWORD crc32(CHAR8 *buf, DWORD len, DWORD init);
QWORD GetProcAddress(QWORD base, DWORD crc, DWORD length)
{
	QWORD a0;
	DWORD a1[4];

	a0 = base + *(unsigned short*)(base + 0x3C);
	a0 = base + *(DWORD*)(a0 + 0x88);
	a1[0] = *(DWORD*)(a0 + 0x18);
	a1[1] = *(DWORD*)(a0 + 0x1C);
	a1[2] = *(DWORD*)(a0 + 0x20);
	a1[3] = *(DWORD*)(a0 + 0x24);
	while (a1[0]--) {
		a0 = base + *(DWORD*)(base + a1[2] + (a1[0] * 4));
		if (crc32((CHAR8*)a0, length, g_encryption_key) == crc) {
			return (base + *(DWORD*)(base + a1[1] + (*(unsigned short*)(base + a1[3] + (a1[0] * 2)) * 4)));
		}
	}
	return 0;
}

KLDR_DATA_TABLE_ENTRY *GetModuleEntryCrc(LIST_ENTRY* entry, DWORD crc, DWORD length)
{
	LIST_ENTRY *list = entry;
	while ((list = list->ForwardLink) != entry) {
		KLDR_DATA_TABLE_ENTRY *module =
			CONTAINING_RECORD(list, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (module && crc32((CHAR8 *)module->BaseImageName.Buffer, length, g_encryption_key) == crc) {
                        
			return module;
		}
	}
	return NULL;
}

int strcmpi(const char *cs, const char *ct)
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

UINT64 GetExport(QWORD base, CHAR8* export)
{
	QWORD a0;
	DWORD a1[4];

	a0 = base + *(unsigned short*)(base + 0x3C);
	a0 = base + *(DWORD*)(a0 + 0x88);
	a1[0] = *(DWORD*)(a0 + 0x18);
	a1[1] = *(DWORD*)(a0 + 0x1C);
	a1[2] = *(DWORD*)(a0 + 0x20);
	a1[3] = *(DWORD*)(a0 + 0x24);
	while (a1[0]--) {
		a0 = base + *(DWORD*)(base + a1[2] + (a1[0] * 4));
		if (strcmpi((const char *)a0, export) == 0) {
			return (base + *(DWORD*)(base + a1[1] + (*(unsigned short*)(base + a1[3] + (a1[0] * 2)) * 4)));
		}
	}
	return 0;
}

BOOLEAN get_usb_serial_number(char serial[2048])
{
	// EFI_USB_IO_PROTOCOL_GUID
	EFI_GUID guid = {0x2B2F68D6, 0x0CD2, 0x44cf, {0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75 }};
	EFI_STATUS Status;
	EFI_USB_IO_PROTOCOL* UsbIo = 0;

	CHAR16* SerialNumber;
	EFI_USB_DEVICE_DESCRIPTOR    DevDesc;
	UINTN                        UsbIoHandleCount;
	EFI_HANDLE                   *UsbIoHandles;

	Status = gBS->LocateHandleBuffer (
			ByProtocol,
			&guid,
			NULL,
			&UsbIoHandleCount,
			&UsbIoHandles
			);

	if (EFI_ERROR (Status)) {
		UsbIoHandleCount = 0;
		UsbIoHandles     = NULL;
		return 0;
	}

	for (UINTN Index = 0; Index < UsbIoHandleCount; Index++ ) {
		Status = gBS->HandleProtocol(
				UsbIoHandles[Index],
				&guid,
				(VOID **) &UsbIo
				);

		if (EFI_ERROR(Status))
			continue;



		Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
		if (EFI_ERROR(Status))
			continue;

		UINT8 device_class = DevDesc.DeviceClass;
		UINT8 sub_class = DevDesc.DeviceSubClass;

		if (device_class == 0) {

			EFI_USB_INTERFACE_DESCRIPTOR IfDesc;
			UsbIo->UsbGetInterfaceDescriptor(UsbIo, &IfDesc);

			device_class = IfDesc.InterfaceClass;
			sub_class = IfDesc.InterfaceSubClass;
		}

		if (device_class != 8 && sub_class != 6)
			continue;

		Status = UsbIo->UsbGetStringDescriptor(
			UsbIo,
			0x0409,
			DevDesc.StrSerialNumber,
			&SerialNumber
		);

		if (EFI_ERROR(Status))
			continue;


		CHAR16* tmp = SerialNumber;

		
		while (*tmp) {
			*serial = (CHAR8)*tmp;

			serial++, tmp++;
		}


		gBS->FreePool(SerialNumber);

		break;
	}

	return Status == EFI_SUCCESS;
}

typedef struct _DMI_HEADER
{
	unsigned char type;
	unsigned char length;
	unsigned short handle;
} DMI_HEADER;

static const char* dmi_string(const DMI_HEADER* dm, unsigned char src)
{
	char* bp = (char*)dm;

	if (src == 0)
	{
		return "";
	}
	bp += dm->length;

	while (src > 1 && *bp) {
		bp += strlen(bp);
		bp++;
		src--;
	}

	if (!*bp)
	{
		return "";
	}

	return bp;

}

BOOLEAN get_motherboard_serial_number(DWORD *serial_number)
{
	DWORD hwid = 0;

	#define EFI_SMBIOS_PROTOCOL_GUID \
	    { 0x3583ff6, 0xcb36, 0x4940, { 0x94, 0x7e, 0xb9, 0xb3, 0x9f, 0x4a, 0xfa, 0xf7 }}

	EFI_GUID guid = EFI_SMBIOS_PROTOCOL_GUID;
	EFI_SMBIOS_PROTOCOL* Smbios;
	EFI_STATUS Status = gBS->LocateProtocol(&guid, NULL, (VOID**)&Smbios);

	if (!EFI_ERROR(Status)) {

		unsigned int found_mem = 0;
		unsigned int found_date = 0;

		EFI_SMBIOS_HANDLE SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
		EFI_SMBIOS_TABLE_HEADER* Record;
		Status = Smbios->GetNext(Smbios, &SmbiosHandle, NULL, &Record, NULL);
		while (!EFI_ERROR(Status)) {
			if (found_mem == 0 && Record->Type == SMBIOS_TYPE_MEMORY_DEVICE) {
				if (strcmpi(dmi_string((const DMI_HEADER*)Record, 5), "Unknown")) {
					found_mem = 1;
					const char *memory_str = dmi_string((const DMI_HEADER*)Record, 5);
					hwid = hwid + crc32((CHAR8*)memory_str, (DWORD)strleni(memory_str), 0x726D0100);
				}
			}	

			if (found_date == 0 && Record->Type == SMBIOS_TYPE_BIOS_INFORMATION) {
				
				found_date = 1;		
				const char* date_str = dmi_string((const DMI_HEADER*)Record, 3);
				hwid = hwid + crc32((CHAR8*)date_str, (DWORD)strleni(date_str), 0x726D0100);
			}

			Status = Smbios->GetNext(Smbios, &SmbiosHandle, NULL, &Record, NULL);
		}
	}

	*serial_number = hwid;

	return 1;
}

QWORD get_winload_base(QWORD return_address)
{
	while (*(unsigned short*)return_address != IMAGE_DOS_SIGNATURE)
		return_address = return_address - 1;
	return return_address;
}

enum WinloadContext
{
	ApplicationContext,
	FirmwareContext
};

typedef void(__stdcall* BlpArchSwitchContext_t)(int target);
BlpArchSwitchContext_t BlpArchSwitchContext;

BOOLEAN scan_and_unlink(QWORD winload_base, VOID *base, QWORD base_size, LOADER_PARAMETER_BLOCK **LoaderParameterBlock, QWORD *VirtualAddress)
{
	BOOLEAN status = 0;






	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)winload_base;
	IMAGE_NT_HEADERS64* nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
	UINT32 imageSize = nt->OptionalHeader.SizeOfImage;



	unsigned char bytes_3[] = { 'x','x','x','?','?','?','?','x','x','x','?','?','?','?',0 };

	UINT64 loaderBlockScan = (UINT64)FindPattern((unsigned char*)winload_base, imageSize,
		"\x48\x8B\x3D\x00\x00\x00\x00\x48\x8B\x8F\x00\x00\x00\x00", bytes_3);
	if (loaderBlockScan == 0) {

		/* 1909 */
		unsigned char bytes_2[] = { 'x','x','x','x','x','?','?','?','?',0 };
		loaderBlockScan = (UINT64)FindPattern((unsigned char*)winload_base, imageSize,
			"\x0F\x31\x48\x8B\x3D\x00\x00\x00\x00", bytes_2);
		if (loaderBlockScan != 0)
			loaderBlockScan += 2;

		/* 1809 */
		unsigned char bytes_1809[] = { 'x','x','x','?','?','?','?','x','x','x',0 };
		if (loaderBlockScan == 0)
			loaderBlockScan = (UINT64)FindPattern((unsigned char*)winload_base, imageSize,
				"\x48\x8B\x3D\x00\x00\x00\x00\x48\x8B\xCF", bytes_1809);

		unsigned char bytes_1[] = { 'x','x','x','?','?','?','?','x','x','x','x',0 };
		/* 1607 */
		if (loaderBlockScan == 0) {
			loaderBlockScan = (UINT64)FindPattern((unsigned char*)winload_base, imageSize,
				"\x48\x8B\x35\x00\x00\x00\x00\x48\x8B\x45\xF7", bytes_1);
		}

		if (loaderBlockScan == 0) {
			return 0;
		}
	}

	UINT64 resolvedAddress = *(UINT64*)((loaderBlockScan + 7) + *(int*)(loaderBlockScan + 3));
	if (resolvedAddress == 0) {
		return 0;
	}

	unsigned char bytes_0[] = { 'x','x','x','x','x','x','x','x','x',0 };
	BlpArchSwitchContext = (BlpArchSwitchContext_t)(FindPattern((VOID*)(winload_base), imageSize,
		"\x40\x53\x48\x83\xEC\x20\x48\x8B\x15", bytes_0));
	if (BlpArchSwitchContext == 0) {
		return 0;
	}

	BlpArchSwitchContext(ApplicationContext);

	LOADER_PARAMETER_BLOCK* loaderBlock = (LOADER_PARAMETER_BLOCK*)(resolvedAddress);
	*LoaderParameterBlock = loaderBlock;

	status = 1;
	if (!UnlinkDescriptorList(loaderBlock, base, base_size)) {
		status = 0;
	}

	if (!UnlinkEfiMemoryMap(loaderBlock, base, base_size, VirtualAddress)) {
		status = 0;
	}

	BlpArchSwitchContext(FirmwareContext);

	return status;
}

static const unsigned int crc32_table[] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
	0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
	0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
	0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
	0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
	0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
	0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
	0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
	0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
	0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
	0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
	0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
	0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
	0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
	0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
	0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
	0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
	0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
	0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
	0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
	0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
	0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
	0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
	0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
	0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
	0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
	0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
	0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
	0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
	0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
	0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
	0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
	0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
	0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
	0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
	0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
	0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
	0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
	0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
	0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
	0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
	0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
	0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
} ;

DWORD crc32(CHAR8 *buf, DWORD len, DWORD init)
{
	DWORD crc = init;
	while (len--) {
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}

void RC4(void* data, int length, unsigned char* key, int key_len)
{

	unsigned char T[256];
	unsigned char S[256];
	unsigned char tmp;
	int i, j = 0, x, t = 0;

	for (i = 0; i < 256; i++) {
		S[i] = (unsigned char)i;
		T[i] = key[i % key_len];
	}
	for (i = 0; i < 256; i++) {
		j = (j + S[i] + T[i]) % 256;
		tmp = S[j];
		S[j] = S[i];
		S[i] = tmp;
	}
	j = 0;
	for (x = 0; x < length; x++) {
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		tmp = S[j];
		S[j] = S[i];
		S[i] = tmp;
		t = (S[i] + S[j]) % 256;
		((unsigned char*)data)[x] = ((unsigned char*)data)[x] ^ S[t];
	}
}

