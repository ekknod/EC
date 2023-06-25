#include "../../../csgo/shared/shared.h"

#define _AMD64_ 1

#include <ntifs.h>
#include <intrin.h>

//
// disables warnings for unused functions
//
#pragma warning (disable : 4505)

//
// float support for drivers
//
int _fltused;

//
// used by vm.cpp
//

//
// dynamic imports, used globally
//
extern "C" {
	ULONG (__cdecl *_DbgPrintEx)(ULONG, ULONG, PCSTR, ...);
	QWORD _KeAcquireSpinLockAtDpcLevel;
	QWORD _KeReleaseSpinLockFromDpcLevel;
	QWORD _IofCompleteRequest;
	QWORD _IoReleaseRemoveLockEx;
	PCSTR ( *_PsGetProcessImageFileName)(PEPROCESS);
	BOOLEAN ( *_PsGetProcessExitProcessCalled)(PEPROCESS);
	PVOID ( *_PsGetProcessPeb)(PEPROCESS);
	PVOID ( *_PsGetProcessWow64Process)(PEPROCESS);
	HANDLE ( *_PsGetProcessId)(_In_ PEPROCESS);
	PVOID ( *_MmMapIoSpace)(PHYSICAL_ADDRESS, SIZE_T, MEMORY_CACHING_TYPE);
	VOID ( *_MmUnmapIoSpace)(PVOID, SIZE_T);
	PPHYSICAL_MEMORY_RANGE (*_MmGetPhysicalMemoryRanges)(void);
	PVOID (*_ExAllocatePoolWithTag)(POOL_TYPE, SIZE_T, ULONG);
	VOID ( *_ExFreePoolWithTag)( PVOID, ULONG );
	NTSTATUS ( *_KeDelayExecutionThread)(KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER);
	VOID ( *_RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
	NTSTATUS ( *_ObReferenceObjectByName)(PUNICODE_STRING, ULONG, PACCESS_STATE, ACCESS_MASK,
		POBJECT_TYPE, KPROCESSOR_MODE, PVOID, PVOID*);
	LONG_PTR ( *_ObfDereferenceObject)(PVOID);
	POBJECT_TYPE* _IoDriverObjectType;
	PEPROCESS _PsInitialSystemProcess;

	QWORD g_system_previous_ms;

	__int64 (__fastcall *MiGetPteAddress)(unsigned __int64 a1);

	//
	// tested and stable approach, we can now fix IAT for them
	//


	PHYSICAL_ADDRESS
	(*_MmGetPhysicalAddress)(
	    _In_ PVOID BaseAddress
	    );

	BOOLEAN
	(*_MmIsAddressValid)(
	    _In_ PVOID VirtualAddress
	    );

	ULONG
	(*_KeQueryTimeIncrement)(
	    VOID
	    );


	NTSTATUS (__fastcall *memcpy_safe_func)(void *, void *, DWORD);
	void *(*memcpy_ptr)(void *, void *, QWORD);


	
	KIRQL (*_KfRaiseIrql)(
	    _In_ KIRQL NewIrql
	    );

	
	VOID
	(*_KeLowerIrql)(
	    _In_ _Notliteral_ _IRQL_restores_ KIRQL NewIrql
	   );

	LIST_ENTRY *_PsLoadedModuleList;
};



#ifdef _KERNEL_MODE
typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    WORD        Magic;
    BYTE        MajorLinkerVersion;
    BYTE        MinorLinkerVersion;
    DWORD       SizeOfCode;
    DWORD       SizeOfInitializedData;
    DWORD       SizeOfUninitializedData;
    DWORD       AddressOfEntryPoint;
    DWORD       BaseOfCode;
    ULONGLONG   ImageBase;
    DWORD       SectionAlignment;
    DWORD       FileAlignment;
    WORD        MajorOperatingSystemVersion;
    WORD        MinorOperatingSystemVersion;
    WORD        MajorImageVersion;
    WORD        MinorImageVersion;
    WORD        MajorSubsystemVersion;
    WORD        MinorSubsystemVersion;
    DWORD       Win32VersionValue;
    DWORD       SizeOfImage;
    DWORD       SizeOfHeaders;
    DWORD       CheckSum;
    WORD        Subsystem;
    WORD        DllCharacteristics;
    ULONGLONG   SizeOfStackReserve;
    ULONGLONG   SizeOfStackCommit;
    ULONGLONG   SizeOfHeapReserve;
    ULONGLONG   SizeOfHeapCommit;
    DWORD       LoaderFlags;
    DWORD       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[8];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;
#endif

#pragma warning(disable : 4201)
typedef struct _MOUSE_INPUT_DATA {
	USHORT UnitId;
	USHORT Flags;
	union {
	ULONG Buttons;
	struct {
	USHORT ButtonFlags;
	USHORT ButtonData;
	};
	};
	ULONG  RawButtons;
	LONG   LastX;
	LONG   LastY;
	ULONG  ExtraInformation;
} MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;

typedef VOID
(*MouseClassServiceCallbackFn)(
	PDEVICE_OBJECT DeviceObject,
	PMOUSE_INPUT_DATA InputDataStart,
	PMOUSE_INPUT_DATA InputDataEnd,
	PULONG InputDataConsumed
);

typedef struct _MOUSE_OBJECT
{
	PDEVICE_OBJECT              mouse_device;
	MouseClassServiceCallbackFn service_callback;
	BOOL                        use_mouse;
	QWORD                       target_routine;
} MOUSE_OBJECT, * PMOUSE_OBJECT;
MOUSE_OBJECT gMouseObject = {0,0,0,0};

namespace mouse
{
	static void move(long x, long y, unsigned short button_flags);
	static BOOL open(void);
}

namespace input
{
	void mouse_move(int x, int y)
	{
		mouse::move(x, y, 0);
	}

	void mouse1_down(void)
	{
		if (cs::allow_triggerbot())
		{
			mouse::move(0, 0, 0x01);
		}
	}

	void mouse1_up(void)
	{
		if (cs::allow_triggerbot())
		{
			mouse::move(0, 0, 0x02);
		}
	}
}

namespace config
{
	BOOL  rcs                     = 1;
	DWORD aimbot_button           = 107;
	float aimbot_fov              = 2.0f;
	float aimbot_smooth           = 100.0f;
	BOOL  aimbot_visibility_check = 0;
	DWORD triggerbot_button       = 111;
	BOOL  visuals_enabled         = 0;

	//
	// time_begin/time_end/licence_type used before in EC/W3
	//
	DWORD time_begin   = 0;
	DWORD time_end     = 0;
	DWORD licence_type = 0;
	DWORD invalid_hwid = 0;
}


static void  clear_image_traces(QWORD efi_image_base);
static QWORD FindPattern(QWORD module, BYTE *bMask, CHAR *szMask, QWORD len, int counter);
QWORD GetProcAddressQ(QWORD base, PCSTR export_name);

//
// it never should return zero, in case it does because of potential typo, loop forever
//
#define EXPORT_ADDRESS2(var, name) \
	*(QWORD*)&var = GetProcAddressQ((QWORD)ntoskrnl, name); \
	if (var == 0) while (1) ; \


// #include <intrin.h>
extern "C" QWORD DriverEntry(QWORD ntoskrnl, void *efi_image_base, void *efi_cfg, NTSTATUS (*FinishApplication)(VOID*));
extern "C" QWORD DriverEntry2(QWORD ntoskrnl, void *efi_image_base, void *efi_cfg, NTSTATUS (*FinishApplication)(VOID*));
DWORD gEntryPointAddress = 0;
QWORD gImageBase = 0;
DWORD gImageSize = 0;

DWORD hook_original_offset;





QWORD __fastcall PsGetProcessDxgProcessHook2(
	QWORD rcx
)
{
	// _disable();
	//
	// get current thread
	//
	QWORD current_thread = __readgsqword(0x188);

	//
	// previous mode == KernelMode
	//
	if (*(BYTE*)(current_thread + 0x232) == 0)
	{
		// _enable();
		return *(QWORD*)(rcx + hook_original_offset);
	}

	//
	// is getting called from DxgiSubmitCommand
	//
	QWORD return_address = (QWORD)_ReturnAddress();
	if (*(DWORD*)(return_address + 0x4D) != 0xccc35f30)
	{
		// _enable();
		return *(QWORD*)(rcx + hook_original_offset);
	}

	//
	// csgo process
	//
	PEPROCESS process = *(PEPROCESS*)(__readgsqword(0x188) + 0xB8);
	const char* image_name = _PsGetProcessImageFileName(process);
	QWORD cr3 = *(QWORD*)((QWORD)process + 0x28);
	if (cr3 == __readcr3())
	{
		//
		// explorer.exe
		//
		if (!gMouseObject.use_mouse)
		{
			if (image_name && *(QWORD*)(image_name) == 0x7265726f6c707865)
			{
				mouse::open();
			}
		}


		//
		// csgo.exe
		//
		if (image_name && *(QWORD*)image_name == 0x6578652e6f677363)
		{
			csgo::run();
		}
	}
	// _enable();
	return *(QWORD*)(rcx + hook_original_offset);
}

VOID *memcpy_wrapper(void *dst, void *src, QWORD length)
{
	typedef struct {
		QWORD dst_addr;
		DWORD _0xffffffff;
		char zero[17];
		bool error;
	} bullshit ;

	bullshit call_data{};
	call_data.dst_addr = (QWORD)dst;
	call_data._0xffffffff = 0xffffffff;

	if (memcpy_safe_func(&call_data, src, (DWORD)length) != STATUS_SUCCESS)
	{
		return 0;
	}

	return (void*)1;
}

static QWORD efi_loader_base_traces;
extern "C" QWORD ClearTraces(QWORD ret)
{

	clear_image_traces((QWORD)efi_loader_base_traces);

	return ret;
}

extern "C" void get_registers(QWORD *rcx, QWORD *rdx, QWORD *r8);

inline BOOL IsInRange(QWORD memory_begin, QWORD memory_end, QWORD address, QWORD length)
{
	if (address >= memory_end)
	{
		return 0;
	}
	
	if ((address + length) <= memory_begin)
	{
	    return 0;
	}
	return 1;
}

void *  __cdecl memset_hook2(void * _Dst, int _Val, QWORD _Size, ULONG flags)
{
	UNREFERENCED_PARAMETER(flags);

	QWORD rcx,rdx,r8;
	get_registers(&rcx, &rdx, &r8);


	QWORD virtual_base_begin = (QWORD)DriverEntry - gEntryPointAddress;
	if (IsInRange(virtual_base_begin, virtual_base_begin + gImageSize, *(QWORD*)rdx, r8))
	{
		*(QWORD*)rdx = 0x1000;
	}
	else if (IsInRange(gImageBase, gImageBase + gImageSize, *(QWORD*)rdx, r8))
	{
		*(QWORD*)rdx = 0xfffff80000000000;
	}

	return memset(_Dst, _Val, _Size);
}

QWORD ResolveRelativeAddress(
	QWORD Instruction,
	DWORD OffsetOffset,
	DWORD InstructionSize
)
{

	QWORD Instr = (QWORD)Instruction;
	int RipOffset = *(int*)(Instr + OffsetOffset);
	QWORD ResolvedAddr = (QWORD)(Instr + InstructionSize + RipOffset);
	return ResolvedAddr;
}

extern "C" QWORD DriverEntry2(QWORD ntoskrnl, void *efi_image_base, void *efi_cfg, NTSTATUS (*FinishApplication)(VOID*))
{
	void *efi_loader_base = efi_image_base;
	int *cfg = (int *)efi_cfg;

	config::aimbot_fov = (float)cfg[0];
	config::aimbot_smooth = (float)cfg[1];
	config::rcs = cfg[2];
	config::aimbot_button = cfg[3];
	config::aimbot_visibility_check = cfg[4];
	config::triggerbot_button = cfg[5];
	config::visuals_enabled = cfg[6];
	config::time_begin = cfg[7];
	config::time_end = cfg[8];
	config::licence_type = cfg[9];
	config::invalid_hwid = cfg[10];

	//
	// 48 89 5C 24 ? 48 89 4C 24 ? 57 48 83 EC 20 41
	//
	QWORD addr = FindPattern(ntoskrnl, (BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x20\x41", (CHAR*)"xxxx?xxxx?xxxxxx", 16, 1);
	if (addr == 0)
	{
		*(QWORD*)&memcpy_ptr = (QWORD)memcpy;
	}
	else
	{
		*(QWORD*)&memcpy_safe_func = addr;

		//
		// https://www.unknowncheats.me/forum/3742948-post13.html
		//
		*(QWORD*)&memcpy_ptr = (QWORD)memcpy_wrapper;
	}

	QWORD MmUnlockPreChargedPagedPoolAddress = 0;


	
	QWORD target_routine;
	QWORD target_routine2;
	EXPORT_ADDRESS2(_DbgPrintEx, "DbgPrintEx");
	EXPORT_ADDRESS2(target_routine, "PsGetProcessDxgProcess");
	EXPORT_ADDRESS2(target_routine2, "MmCopyMemory");
	EXPORT_ADDRESS2(MmUnlockPreChargedPagedPoolAddress, "MmUnlockPreChargedPagedPool");
	EXPORT_ADDRESS2(_KeAcquireSpinLockAtDpcLevel, "KeAcquireSpinLockAtDpcLevel");
	EXPORT_ADDRESS2(_KeReleaseSpinLockFromDpcLevel, "KeReleaseSpinLockFromDpcLevel");
	EXPORT_ADDRESS2(_IofCompleteRequest, "IofCompleteRequest");
	EXPORT_ADDRESS2(_IoReleaseRemoveLockEx, "IoReleaseRemoveLockEx");
	EXPORT_ADDRESS2(_PsGetProcessImageFileName, "PsGetProcessImageFileName");
	EXPORT_ADDRESS2(_PsGetProcessExitProcessCalled, "PsGetProcessExitProcessCalled");
	EXPORT_ADDRESS2(_PsGetProcessPeb, "PsGetProcessPeb");
	EXPORT_ADDRESS2(_PsGetProcessWow64Process, "PsGetProcessWow64Process");
	EXPORT_ADDRESS2(_PsGetProcessId, "PsGetProcessId");
	EXPORT_ADDRESS2(_MmMapIoSpace, "MmMapIoSpace");
	EXPORT_ADDRESS2(_MmUnmapIoSpace, "MmUnmapIoSpace");
	EXPORT_ADDRESS2(_MmGetPhysicalMemoryRanges, "MmGetPhysicalMemoryRanges");
	EXPORT_ADDRESS2(_ExAllocatePoolWithTag, "ExAllocatePoolWithTag");
	EXPORT_ADDRESS2(_ExFreePoolWithTag, "ExFreePoolWithTag");
	EXPORT_ADDRESS2(_KeDelayExecutionThread, "KeDelayExecutionThread");
	EXPORT_ADDRESS2(_RtlInitUnicodeString, "RtlInitUnicodeString");
	EXPORT_ADDRESS2(_ObReferenceObjectByName, "ObReferenceObjectByName");
	EXPORT_ADDRESS2(_ObfDereferenceObject, "ObfDereferenceObject");
	EXPORT_ADDRESS2(_IoDriverObjectType, "IoDriverObjectType");
	EXPORT_ADDRESS2(_PsInitialSystemProcess, "PsInitialSystemProcess");
	EXPORT_ADDRESS2(_MmGetPhysicalAddress, "MmGetPhysicalAddress");
	EXPORT_ADDRESS2(_MmIsAddressValid, "MmIsAddressValid");
	EXPORT_ADDRESS2(_KeQueryTimeIncrement, "KeQueryTimeIncrement");
	EXPORT_ADDRESS2(_KfRaiseIrql, "KfRaiseIrql");
	EXPORT_ADDRESS2(_KeLowerIrql, "KeLowerIrql");
	EXPORT_ADDRESS2(_PsLoadedModuleList, "PsLoadedModuleList");

	QWORD hook_routine = (QWORD)PsGetProcessDxgProcessHook2;
	hook_original_offset = *(DWORD*)(target_routine + 0x03);

	g_system_previous_ms = 0;
	*(QWORD*)&MiGetPteAddress = (QWORD)(*(int*)(MmUnlockPreChargedPagedPoolAddress + 8) + MmUnlockPreChargedPagedPoolAddress + 12);


	//
	// trying reduce weird bugs
	//
	cs::reset_globals();


	//
	// place win32k hook here
	//
	unsigned char PsGetProcessDxgProcessOriginal[8];
	for (int i = sizeof(PsGetProcessDxgProcessOriginal); i--;)
	{
		((unsigned char*)PsGetProcessDxgProcessOriginal)[i] = ((unsigned char*)target_routine)[i];
	}

	hook_original_offset = *(DWORD*)(target_routine + 0x03);
	unsigned char payload_bytes[] = { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3 } ;
	for (int i = sizeof(payload_bytes); i--;)
	{
		((unsigned char*)target_routine)[i] = payload_bytes[i];
	}

	int fixed_address = 0;
	if (hook_routine > target_routine)
	{
		fixed_address = (int)(hook_routine - target_routine);
	}
	else
	{
		fixed_address = (int)(target_routine - hook_routine);
	}

	*(int*)(target_routine + 1) = (int)fixed_address - 5;



	target_routine2 = target_routine2 + 0x58;

	if (*(unsigned char*)(target_routine2) == 0xE8)
	{
		fixed_address = 0;
	
		hook_routine = (QWORD)memset_hook2;
		if (hook_routine > target_routine2)
		{
			fixed_address = (int)(hook_routine - target_routine2);
		}
		else
		{
			fixed_address = (int)(target_routine2 - hook_routine);
		}
		*(int*)(target_routine2 + 1) = (int)fixed_address - 5;
	}

	efi_loader_base_traces = (QWORD)efi_loader_base;

	return (QWORD)FinishApplication;
}

QWORD GetProcAddressQ(QWORD base, PCSTR export_name)
{
	QWORD a0;
	DWORD a1[4];
	
	
	a0 = base + *(USHORT*)(base + 0x3C);
	a0 = base + *(DWORD*)(a0 + 0x88);
	a1[0] = *(DWORD*)(a0 + 0x18);
	a1[1] = *(DWORD*)(a0 + 0x1C);
	a1[2] = *(DWORD*)(a0 + 0x20);
	a1[3] = *(DWORD*)(a0 + 0x24);
	while (a1[0]--) {
		
		a0 = base + *(DWORD*)(base + a1[2] + (a1[0] * 4));
		if (strcmpi_imp((PCSTR)a0, export_name) == 0) {
			
			return (base + *(DWORD*)(base + a1[1] + (*(USHORT*)(base + a1[3] + (a1[0] * 2)) * 4)));
		}	
	}
	return 0;
}

static void clear_image_traces(QWORD efi_image_base)
{
	QWORD km_base = 0;
	{
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)efi_image_base;
		IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);


		km_base = efi_image_base + nt->OptionalHeader.SizeOfImage;
		for (int i = nt->OptionalHeader.SizeOfImage; i--;)
			((unsigned char*)efi_image_base)[i] = 0;
	}

	{
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)km_base;
		IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);

		gEntryPointAddress = nt->OptionalHeader.AddressOfEntryPoint;

		gImageBase = km_base;
		gImageSize = nt->OptionalHeader.SizeOfImage;

		for (int i = nt->OptionalHeader.SizeOfHeaders; i--;)
			((unsigned char*)km_base)[i] = 0;
	}
}



static BOOLEAN bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if ((*szMask == 1 || *szMask == 'x') && *pData != *bMask)
			return 0;
	return (*szMask) == 0;
}

static QWORD FindPatternEx(UINT64 dwAddress, QWORD dwLen, BYTE *bMask, char * szMask)
{
	if (dwLen <= 0)
		return 0;
	for (QWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (QWORD)(dwAddress + i);
	return 0;
}

static QWORD FindPattern(QWORD module, BYTE *bMask, CHAR *szMask, QWORD len, int counter)
{
	ULONG_PTR ret = 0;
	PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);
	PIMAGE_SECTION_HEADER pish = (PIMAGE_SECTION_HEADER)((BYTE*)pinh + sizeof(IMAGE_NT_HEADERS64));
	
	for (USHORT sec = 0; sec < pinh->FileHeader.NumberOfSections; sec++)
	{
		
		if ((pish[sec].Characteristics & 0x00000020))
		{
			QWORD address = FindPatternEx(pish[sec].VirtualAddress + (ULONG_PTR)(module),
				pish[sec].Misc.VirtualSize - len, bMask, szMask);
 
			if (address) {
				ret = address;

				counter --;

				if (counter == 0)
					break;
			}
		}
		
	}
	return ret;
}

#pragma warning(disable : 4201)
typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	WORD LoadCount;
	WORD TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		struct
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union
	{
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	PVOID* EntryPointActivationContext;
	PVOID PatchInformation;
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

inline int wcscmp_imp(const unsigned short* s1, const unsigned short* s2)
{
	while (*s1 && (to_lower_imp(*s1) == to_lower_imp(*s2)))
	{
		s1++;
		s2++;
	}
	return *(const unsigned short*)s1 - *(const unsigned short*)s2;
}

QWORD GetModuleEntry(PCWSTR module_name)
{
	LIST_ENTRY *PsLoadedModuleList2 = (LIST_ENTRY*)*(QWORD*)_PsLoadedModuleList;

	for (PLIST_ENTRY pListEntry = PsLoadedModuleList2->Flink; pListEntry != PsLoadedModuleList2; pListEntry = pListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (pEntry->DllBase == 0)
			continue;

		if (pEntry->BaseDllName.Length && wcscmpi_imp((unsigned short *)pEntry->BaseDllName.Buffer, (unsigned short*)module_name) == 0)
		{			
			return (QWORD)pEntry->DllBase;
		}

	}

	return 0;
}

static BOOL mouse::open(void)
{
	// https://github.com/nbqofficial/norsefire

	if (gMouseObject.use_mouse == 0) {
		UNICODE_STRING class_string;
		_RtlInitUnicodeString(&class_string, L"\\Driver\\MouClass");
		PDRIVER_OBJECT class_driver_object = NULL;
		NTSTATUS status = _ObReferenceObjectByName(&class_string, OBJ_CASE_INSENSITIVE, NULL, 0, *_IoDriverObjectType, KernelMode, NULL, (PVOID*)&class_driver_object);
		if (!NT_SUCCESS(status)) {
			gMouseObject.use_mouse = 0;
			return 0;
		}

		UNICODE_STRING hid_string;
		_RtlInitUnicodeString(&hid_string, L"\\Driver\\MouHID");
		PDRIVER_OBJECT hid_driver_object = NULL;
		status = _ObReferenceObjectByName(&hid_string, OBJ_CASE_INSENSITIVE, NULL, 0, *_IoDriverObjectType, KernelMode, NULL, (PVOID*)&hid_driver_object);
		if (!NT_SUCCESS(status))
		{
			if (class_driver_object) {
				_ObfDereferenceObject(class_driver_object);
			}
			gMouseObject.use_mouse = 0;
			return 0;
		}


		PDEVICE_OBJECT hid_device_object = hid_driver_object->DeviceObject;
		while (hid_device_object && !gMouseObject.service_callback)
		{
			PDEVICE_OBJECT class_device_object = class_driver_object->DeviceObject;
			while (class_device_object && !gMouseObject.service_callback)
			{
				if (!class_device_object->NextDevice && !gMouseObject.mouse_device)
				{
					gMouseObject.mouse_device = class_device_object;
				}

				PULONG_PTR device_extension = (PULONG_PTR)hid_device_object->DeviceExtension;
				ULONG_PTR device_ext_size = ((ULONG_PTR)hid_device_object->DeviceObjectExtension - (ULONG_PTR)hid_device_object->DeviceExtension) / 4;
				for (ULONG_PTR i = 0; i < device_ext_size; i++)
				{
					if (device_extension[i] == (ULONG_PTR)class_device_object && device_extension[i + 1] > (ULONG_PTR)class_driver_object)
					{
						gMouseObject.service_callback = (MouseClassServiceCallbackFn)(device_extension[i + 1]);
					
						break;
					}
				}
				class_device_object = class_device_object->NextDevice;
			}
			hid_device_object = hid_device_object->AttachedDevice;
		}
	
		if (!gMouseObject.mouse_device)
		{
			PDEVICE_OBJECT target_device_object = class_driver_object->DeviceObject;
			while (target_device_object)
			{
				if (!target_device_object->NextDevice)
				{
					gMouseObject.mouse_device = target_device_object;
					break;
				}
				target_device_object = target_device_object->NextDevice;
			}
		}

		_ObfDereferenceObject(class_driver_object);
		_ObfDereferenceObject(hid_driver_object);

		if (gMouseObject.mouse_device && gMouseObject.service_callback) {
			gMouseObject.use_mouse = 1;
		}
	}
	return gMouseObject.mouse_device && gMouseObject.service_callback;
}

#define KeMRaiseIrql(a,b) *(b) = _KfRaiseIrql(a)
extern "C" VOID MouseClassServiceCallback(PDEVICE_OBJECT, PMOUSE_INPUT_DATA, PMOUSE_INPUT_DATA, PULONG);
static void mouse::move(long x, long y, unsigned short button_flags)
{
	if (gMouseObject.use_mouse == 0)
	{
		return;
	}
	ULONG input_data;
	MOUSE_INPUT_DATA mid = { 0 };
	KIRQL irql;
	mid.LastX = x;
	mid.LastY = y;
	mid.ButtonFlags = button_flags;
	mid.UnitId = 1;
	KeMRaiseIrql(DISPATCH_LEVEL, &irql);
	MouseClassServiceCallback(gMouseObject.mouse_device, &mid, (PMOUSE_INPUT_DATA)&mid + 1, &input_data);
	_KeLowerIrql(irql);
}

