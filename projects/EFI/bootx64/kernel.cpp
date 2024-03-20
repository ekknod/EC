#include <ntifs.h>
#include "globals.h"
#include <intrin.h>
#include "../../../cs2/shared/cs2game.h"
#include "../../../csgo/shared/csgogame.h"
#include "../../../apex/shared/apexgame.h"

int _fltused;

extern "C"
{
	QWORD NtUserPeekMessage;
	QWORD _KeAcquireSpinLockAtDpcLevel;
	QWORD _KeReleaseSpinLockFromDpcLevel;
	QWORD _IofCompleteRequest;
	QWORD _IoReleaseRemoveLockEx;
}

namespace km
{
	QWORD   GetReadFile(void);
	BOOLEAN initialize(QWORD ntoskrnl, QWORD fbase, QWORD fsize);
	DWORD PsGetThreadWin32ThreadOffset;
	QWORD __fastcall PsGetThreadWin32ThreadHook(QWORD rcx);

	NTSTATUS (__fastcall *oIopReadFile)(
		__int64 a1,
		__int64 a2,
		__int32 a3,
		__int64 a4,
		__int64 a5,
		__int64 a6,
		__int32 a7,
		__int64 a8,
		__int64 a9,
		__int64 a10,
		__int64 a11,
		__int64 a12,
		__int64 a13,
		__int64 a14);

	NTSTATUS __fastcall IopReadFileHook(
		__int64 a1,
		__int64 a2,
		__int32 a3,
		__int64 a4,
		__int64 a5,
		__int64 a6,
		__int32 a7,
		__int64 a8,
		__int64 a9,
		__int64 a10,
		__int64 a11,
		__int64 a12,
		__int64 a13,
		__int64 a14);

	typedef struct
	{
		DWORD offset;
		QWORD value, original_value;
	} FILE_PATCH ;

	FILE_PATCH patch[3];
	FILE_PATCH hook(
		QWORD fbase,
		QWORD ntoskrnl, QWORD kernel_function, QWORD detour_routine, QWORD *original_function);
}

namespace mouse
{
	static void move(long x, long y, unsigned short button_flags);
	static BOOL open(void);
}

namespace gdi
{
	void DrawRect(void *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned char b);
	void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned char b);
}

namespace client
{
	void mouse_move(int x, int y)
	{
		mouse::move(x, y, 0);
	}

	void mouse1_down(void)
	{
		mouse::move(0, 0, 0x01);
	}

	void mouse1_up(void)
	{
		mouse::move(0, 0, 0x02);
	}

	void DrawRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		gdi::DrawRect(hwnd, x, y, w, h, r, g, b);
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		gdi::DrawFillRect(hwnd, x, y, w, h, r, g, b);
	}
}

namespace km
{
	NTSTATUS (__fastcall *memcpy_safe)(void *, void *, DWORD);
	BOOL memcpy_impl(void *dest, const void *src, QWORD size)
	{
		typedef struct {
			QWORD dst_addr;
			DWORD _0xffffffff;
			char zero[17];
			bool error;
		} data ;
		data call_data{};
		call_data.dst_addr = (QWORD)dest;
		call_data._0xffffffff = 0xffffffff;
		memcpy_safe(&call_data, (void *)src, (DWORD)size);
		return call_data.error == 0;
	}
}

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
MOUSE_OBJECT gMouseObject{};

extern "C" NTSYSCALLAPI PCSTR PsGetProcessImageFileName(PEPROCESS);
extern "C" NTSYSCALLAPI QWORD PsGetThreadWin32Thread(QWORD);
extern "C" NTSYSCALLAPI POBJECT_TYPE* IoDriverObjectType;
extern "C" NTSYSCALLAPI NTSTATUS ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID* Object
);

//
// SDL3.dll:PeekMessageW:NtUserPeekMessage:PsGetThreadWin32Thread
//
QWORD __fastcall km::PsGetThreadWin32ThreadHook(QWORD rcx)
{
	//
	// get current thread
	//
	QWORD current_thread = __readgsqword(0x188);
	if (current_thread != rcx)
	{
		return *(QWORD*)(rcx + PsGetThreadWin32ThreadOffset);
	}


	//
	// previous mode == KernelMode
	//
	if (*(unsigned char*)(current_thread + 0x232) == 0)
	{
		return *(QWORD*)(rcx + PsGetThreadWin32ThreadOffset);
	}


	//
	// is getting called from NtUserPeekMessage
	//
	QWORD return_address = (QWORD)_ReturnAddress();
	if (NtUserPeekMessage == 0)
	{
		QWORD routine_address = GetExportByName(get_caller_base(return_address), "NtUserPeekMessage");
		if (routine_address == 0)
		{
			return *(QWORD*)(rcx + PsGetThreadWin32ThreadOffset);
		}
		NtUserPeekMessage = routine_address;
	}

	if (return_address < NtUserPeekMessage)
	{
		return *(QWORD*)(rcx + PsGetThreadWin32ThreadOffset);
	}

	if (return_address > (NtUserPeekMessage + 0x191))
	{
		return *(QWORD*)(rcx + PsGetThreadWin32ThreadOffset);
	}

	PEPROCESS process = *(PEPROCESS*)(__readgsqword(0x188) + 0xB8);
	const char* image_name = PsGetProcessImageFileName(process);


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

	if (image_name)
	{
		if (*(QWORD*)image_name == 0x6578652e327363)
		{
			cs2::run();
		}
		else if (*(QWORD*)image_name == 0x6578652e6f677363)
		{
			csgo::run();
		}
		else if (*(QWORD*)image_name == 0x652e786570613572)
		{
			apex::run();
		}
	}
	return *(QWORD*)(rcx + PsGetThreadWin32ThreadOffset);
}

NTSTATUS __fastcall km::IopReadFileHook(
	__int64 a1,
	__int64 a2,
	__int32 a3,
	__int64 a4,
	__int64 a5,
	__int64 a6,
	__int32 a7,
	__int64 a8,
	__int64 a9,
	__int64 a10,
	__int64 a11,
	__int64 a12,
	__int64 a13,
	__int64 a14
	)
{
	LARGE_INTEGER read_offset = a8 ?
		*(LARGE_INTEGER*)a8 :
		((PFILE_OBJECT)a1)->CurrentByteOffset;



	NTSTATUS status = oIopReadFile(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
	if (!NT_SUCCESS(status))
	{
		return status;
	}


	//
	// check if someone is reading file from byte patch range
	//
	BOOL in_range = 0;
	for (int i = sizeof(patch) / sizeof(*patch); i--;)
	{
		if ((patch[i].offset + sizeof(QWORD)) > read_offset.LowPart &&
			(patch[i].offset) < (read_offset.LowPart + a7))
		{
			in_range = 1;
			break;
		}
	}


	//
	// we dont have to do anything
	//
	if (!in_range)
	{
		return status;
	}


	//
	// get file path by file object
	//
	WCHAR                   buffer[260]{};
	ULONG                   return_length = 0;
	OBJECT_NAME_INFORMATION info{ {0, 0, buffer} };
	if (!NT_SUCCESS(ObQueryNameString((PVOID)a1, &info, sizeof(buffer), &return_length)))
	{
		return status;
	}


	//
	// if file path contains ntoskrnl (optional)
	//
	if (!wcsstr((WCHAR*)buffer, L"ntoskrnl"))
	{
		return status;
	}


	//
	// lets apply our patches
	//
	DWORD read_size   = a7;
	QWORD read_buffer = a6;
	for (int i = sizeof(patch) / sizeof(*patch); i--;)
	{
		if ((patch[i].offset + sizeof(QWORD)) > read_offset.LowPart &&
			(patch[i].offset) < (read_offset.LowPart + a7))
		{
			//
			// relative offset / offset
			//
			int   rof = read_offset.LowPart - patch[i].offset;
			DWORD off = 0;


			if (rof > 0)
			{
				off = (patch[i].offset + rof) - read_offset.LowPart;
			}
			else
			{
				rof = 0;
				off = patch[i].offset - read_offset.LowPart;
			}

			QWORD max_size = (sizeof(QWORD) - rof);
			DWORD data_left = (read_offset.LowPart + read_size) - (patch[i].offset + rof);
			if (data_left < max_size)
			{
				max_size = data_left;
			}

			//
			// LOG("patching: %lx, %ld, data left: %ld, hyp: %ld\n", patch[i].offset + rof, max_size, data_left, rof);
			//
			memcpy(
				(void*)(read_buffer + off),
				(const void*)((unsigned char*)&patch[i].value + rof),
				max_size
			);

		}

	}

	return status;
}

int get_relative_address_offset(QWORD hook, QWORD target)
{
	return (hook > target ? (int)(hook - target) : (int)(target - hook)) - 5;
}

int get_relative_address_offset2(QWORD hook, QWORD target)
{
	return (hook < target ? (int)(hook - target) : (int)(target - hook)) - 5;
}

inline QWORD get_relative_address(QWORD instruction, DWORD offset, DWORD instruction_size)
{
	INT32 rip_address = *(INT32*)(instruction + offset);
	return (QWORD)(instruction + instruction_size + rip_address);
}
DWORD get_file_offset(QWORD base, QWORD address);

km::FILE_PATCH km::hook(QWORD fbase, QWORD ntoskrnl, QWORD kernel_function, QWORD detour_routine, QWORD *original_function)
{
	km::FILE_PATCH entry{};


	entry.offset         = get_file_offset(ntoskrnl, kernel_function);
	entry.original_value = *(QWORD*)kernel_function;

	if (original_function)
		*original_function = get_relative_address((QWORD)kernel_function, 1, 5);

	*(int*)((QWORD)kernel_function + 1) = get_relative_address_offset( (QWORD)detour_routine, (QWORD)kernel_function );
	if (get_relative_address((QWORD)kernel_function, 1, 5) != (QWORD)detour_routine)
	{
		*(int*)((QWORD)kernel_function + 1) = get_relative_address_offset2( (QWORD)detour_routine, (QWORD)kernel_function );
		if (get_relative_address((QWORD)kernel_function, 1, 5) != (QWORD)detour_routine)
		{
			return {};
		}
	}

	entry.value = *(QWORD*)kernel_function;


	//
	// apply patch to fbase ntoskrnl [raw]
	//
	*(QWORD*)(fbase + entry.offset) = entry.value;


	return entry;
}

DWORD calculate_checksum(PVOID file, DWORD file_size);
DWORD get_checksum(QWORD base);
DWORD get_checksum_off(QWORD base);
void  set_checksum(QWORD base, DWORD checksum);

QWORD km::GetReadFile(void)
{
	QWORD begin = (QWORD)NtReadFile;
	while (*(QWORD*)begin != 0xCCCCCCCCCCCCCCCC) begin++;
	while (*(WORD*)begin != 0xCCC3) begin--;
	while (*(BYTE*)begin != 0xE8) begin--;
	return begin;
}

BOOLEAN km::initialize(QWORD ntoskrnl, QWORD fbase, QWORD fsize)
{
	QWORD addr = FindPattern(ntoskrnl,
		(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x20\x41", (BYTE*)"xxxx?xxxx?xxxxxx");
	if (addr == 0)
	{
		return 0;
	}

	*(QWORD*)&km::memcpy_safe = addr;
	PsGetThreadWin32ThreadOffset = *(DWORD*)((QWORD)PsGetThreadWin32Thread + 3);
	*(unsigned char*)((QWORD)PsGetThreadWin32Thread + 0) = 0xE9;
	patch[0] = hook(
		fbase,
		ntoskrnl, (QWORD)PsGetThreadWin32Thread, (QWORD)PsGetThreadWin32ThreadHook, 0);

	patch[1] = hook(
		fbase,
		ntoskrnl, GetReadFile(), (QWORD)IopReadFileHook, (QWORD*)&oIopReadFile);

	for (int i = 0; i < 2; i++) if (patch[i].original_value == 0) return 0;

	NtUserPeekMessage = 0;
	_KeAcquireSpinLockAtDpcLevel = (QWORD)KeAcquireSpinLockAtDpcLevel;
	_KeReleaseSpinLockFromDpcLevel = (QWORD)KeReleaseSpinLockFromDpcLevel;
	_IofCompleteRequest = (QWORD)IofCompleteRequest;
	_IoReleaseRemoveLockEx = (QWORD)IoReleaseRemoveLockEx;
	

	DWORD checksum = calculate_checksum((PVOID)fbase, (DWORD)fsize);
	set_checksum(ntoskrnl, checksum);

	QWORD nval = *(QWORD*)(ntoskrnl + get_checksum_off(ntoskrnl));
	QWORD oval = nval;
	((DWORD*)&nval)[0] = checksum;

	patch[2].offset         = get_checksum_off(ntoskrnl);
	patch[2].value          = nval;
	patch[2].original_value = oval;

	return TRUE;
}

static BOOL mouse::open(void)
{
	// https://github.com/nbqofficial/norsefire

	if (gMouseObject.use_mouse == 0) {
		UNICODE_STRING class_string;
		RtlInitUnicodeString(&class_string, L"\\Driver\\MouClass");
		PDRIVER_OBJECT class_driver_object = NULL;
		NTSTATUS status = ObReferenceObjectByName(&class_string, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&class_driver_object);
		if (!NT_SUCCESS(status)) {
			gMouseObject.use_mouse = 0;
			return 0;
		}

		UNICODE_STRING hid_string;
		RtlInitUnicodeString(&hid_string, L"\\Driver\\MouHID");
		PDRIVER_OBJECT hid_driver_object = NULL;
		status = ObReferenceObjectByName(&hid_string, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&hid_driver_object);
		if (!NT_SUCCESS(status))
		{
			if (class_driver_object) {
				ObfDereferenceObject(class_driver_object);
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

		ObfDereferenceObject(class_driver_object);
		ObfDereferenceObject(hid_driver_object);

		if (gMouseObject.mouse_device && gMouseObject.service_callback) {
			gMouseObject.use_mouse = 1;
		}
	}

	
	return gMouseObject.mouse_device && gMouseObject.service_callback;
}

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
	KeRaiseIrql(DISPATCH_LEVEL, &irql);
	MouseClassServiceCallback(gMouseObject.mouse_device, &mid, (PMOUSE_INPUT_DATA)&mid + 1, &input_data);
	KeLowerIrql(irql);
}
