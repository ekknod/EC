#pragma once

#define CONTAINING_RECORD(address, type, field) ((type *)((UINT8 *)(address) - (UINTN)(&((type *)0)->field)))
#define RELATIVE_ADDR(addr, size) ((VOID *)((UINT8 *)(addr) + *(INT32 *)((UINT8 *)(addr) + ((size) - (INT32)sizeof(INT32))) + (size)))
#define JMP_SIZE (14)
#define IMAGE_REL_BASED_ABSOLUTE (0)
#define IMAGE_REL_BASED_DIR64 (10)
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES (16)
#define IMAGE_SIZEOF_SHORT_NAME (8)
#define IMAGE_DIRECTORY_ENTRY_EXPORT (0)
#define IMAGE_DIRECTORY_ENTRY_IMPORT (1)
#define IMAGE_DIRECTORY_ENTRY_BASERELOC (5)
#define IMAGE_DOS_SIGNATURE (0x5A4D)

typedef UINTN QWORD;
typedef UINT32 DWORD;
typedef long NTSTATUS;

struct _VIRTUAL_EFI_RUNTIME_SERVICES
{
	UINT64 GetTime;                                                      //0x0
	UINT64 SetTime;                                                      //0x8
	UINT64 GetWakeupTime;                                                //0x10
	UINT64 SetWakeupTime;                                                //0x18
	UINT64 SetVirtualAddressMap;                                         //0x20
	UINT64 ConvertPointer;                                               //0x28
	UINT64 GetVariable;                                                  //0x30
	UINT64 GetNextVariableName;                                          //0x38
	UINT64 SetVariable;                                                  //0x40
	UINT64 GetNextHighMonotonicCount;                                    //0x48
	UINT64 ResetSystem;                                                  //0x50
	UINT64 UpdateCapsule;                                                //0x58
	UINT64 QueryCapsuleCapabilities;                                     //0x60
	UINT64 QueryVariableInfo;                                            //0x68
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

typedef struct _UNICODE_STRING {
	UINT16 Length;
	UINT16 MaximumLength;
	CHAR16 *Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _DRIVER_OBJECT {
	UINT16             Type;
	UINT16             Size;
	VOID*     DeviceObject;
	UINT32             Flags;
	VOID*              DriverStart;
	UINT32             DriverSize;
	VOID*              DriverSection;
	VOID*  DriverExtension;
	UNICODE_STRING     DriverName;
	PUNICODE_STRING    HardwareDatabase;
	VOID*  FastIoDispatch;
	VOID* DriverInit;
	VOID*    DriverStartIo;
	VOID*    DriverUnload;
	VOID*    MajorFunction[0x1b + 1];
} DRIVER_OBJECT, *PDRIVER_OBJECT;

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

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
	UINT16 e_magic;                     // Magic number
	UINT16 e_cblp;                      // Bytes on last page of file
	UINT16 e_cp;                        // Pages in file
	UINT16 e_crlc;                      // Relocations
	UINT16 e_cparhdr;                   // Size of header in paragraphs
	UINT16 e_minalloc;                  // Minimum extra paragraphs needed
	UINT16 e_maxalloc;                  // Maximum extra paragraphs needed
	UINT16 e_ss;                        // Initial (relative) SS value
	UINT16 e_sp;                        // Initial SP value
	UINT16 e_csum;                      // Checksum
	UINT16 e_ip;                        // Initial IP value
	UINT16 e_cs;                        // Initial (relative) CS value
	UINT16 e_lfarlc;                    // File address of relocation table
	UINT16 e_ovno;                      // Overlay number
	UINT16 e_res[4];                    // Reserved words
	UINT16 e_oemid;                     // OEM identifier (for e_oeminfo)
	UINT16 e_oeminfo;                   // OEM information; e_oemid specific
	UINT16 e_res2[10];                  // Reserved words
	UINT32 e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
	UINT32   VirtualAddress;
	UINT32   Size;
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
	UINT16               Magic;
	UINT8                MajorLinkerVersion;
	UINT8                MinorLinkerVersion;
	UINT32               SizeOfCode;
	UINT32               SizeOfInitializedData;
	UINT32               SizeOfUninitializedData;
	UINT32               AddressOfEntryPoint;
	UINT32               BaseOfCode;
	UINT64               ImageBase;
	UINT32               SectionAlignment;
	UINT32               FileAlignment;
	UINT16               MajorOperatingSystemVersion;
	UINT16               MinorOperatingSystemVersion;
	UINT16               MajorImageVersion;
	UINT16               MinorImageVersion;
	UINT16               MajorSubsystemVersion;
	UINT16               MinorSubsystemVersion;
	UINT32               Win32VersionValue;
	UINT32               SizeOfImage;
	UINT32               SizeOfHeaders;
	UINT32               CheckSum;
	UINT16               Subsystem;
	UINT16               DllCharacteristics;
	UINT64               SizeOfStackReserve;
	UINT64               SizeOfStackCommit;
	UINT64               SizeOfHeapReserve;
	UINT64               SizeOfHeapCommit;
	UINT32               LoaderFlags;
	UINT32               NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_FILE_HEADER {
	UINT16  Machine;
	UINT16  NumberOfSections;
	UINT32  TimeDateStamp;
	UINT32  PointerToSymbolTable;
	UINT32  NumberOfSymbols;
	UINT16  SizeOfOptionalHeader;
	UINT16  Characteristics;
} IMAGE_FILE_HEADER;

typedef struct _IMAGE_NT_HEADERS64 {
	UINT32 Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;

typedef struct _IMAGE_SECTION_HEADER {
	UINT8    Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		UINT32   PhysicalAddress;
		UINT32   VirtualSize;
	} Misc;
	UINT32   VirtualAddress;
	UINT32   SizeOfRawData;
	UINT32   PointerToRawData;
	UINT32   PointerToRelocations;
	UINT32   PointerToLinenumbers;
	UINT16   NumberOfRelocations;
	UINT16   NumberOfLinenumbers;
	UINT32   Characteristics;
} IMAGE_SECTION_HEADER, * PIMAGE_SECTION_HEADER;

#pragma warning(push)
#pragma warning(disable: 4201)
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		UINT32   Characteristics;
		UINT32   OriginalFirstThunk;
	};

	UINT32   TimeDateStamp;
	UINT32   ForwarderChain;
	UINT32   Name;
	UINT32   FirstThunk;
	} IMAGE_IMPORT_DESCRIPTOR;
#pragma warning(pop)

typedef struct _IMAGE_THUNK_DATA64 {
	union {
		UINT64 ForwarderString;
		UINT64 Function;
		UINT64 Ordinal;
		UINT64 AddressOfData;
	} u1;
} IMAGE_THUNK_DATA64;

typedef struct _IMAGE_IMPORT_BY_NAME {
	UINT16 Hint;
	CHAR8  Name[1];
} IMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_EXPORT_DIRECTORY {
	UINT32 Characteristics;
	UINT32 TimeDateStamp;
	UINT16 MajorVersion;
	UINT16 MinorVersion;
	UINT32 Name;
	UINT32 Base;
	UINT32 NumberOfFunctions;
	UINT32 NumberOfNames;
	UINT32 AddressOfFunctions;
	UINT32 AddressOfNames;
	UINT32 AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_BASE_RELOCATION {
	UINT32 VirtualAddress;
	UINT32 SizeOfBlock;
} IMAGE_BASE_RELOCATION;

typedef struct _MEMORY_ALLOCATION_DESCRIPTOR
{
	struct _LIST_ENTRY ListEntry;                                       //0x0
	enum _TYPE_OF_MEMORY MemoryType;                                    //0x10
	UINTN BasePage;                                                     //0x18
	UINTN PageCount;                                                    //0x20
} MEMORY_ALLOCATION_DESCRIPTOR, *PMEMORY_ALLOCATION_DESCRIPTOR  ; 

EFI_STATUS LoadFile(const CHAR16 *path, VOID **buffer, UINTN *buffer_size);
BOOLEAN    FileExists(const CHAR16 *path);
VOID       MemCopy(VOID* dest, VOID* src, UINTN size);

inline void PressAnyKey()
{
	EFI_STATUS         Status;
	EFI_EVENT          WaitList;
	EFI_INPUT_KEY      Key;
	UINTN              Index;
	
	Print(L"Press any key to continue . . .");
	do {
		WaitList = gST->ConIn->WaitForKey;
		Status = gBS->WaitForEvent(1, &WaitList, &Index);
		gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
		if (Key.ScanCode != SCAN_PAUSE)
			break;
	} while ( 1 );
	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
}

