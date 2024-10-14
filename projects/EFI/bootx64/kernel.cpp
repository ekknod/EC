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
	BOOLEAN initialize(QWORD ntoskrnl);
	DWORD PsGetThreadWin32ThreadOffset;
	QWORD __fastcall PsGetThreadWin32ThreadHook(QWORD rcx);

	BOOL hook(QWORD kernel_function, QWORD detour_routine, QWORD *original_function);
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
// SDL3.dll:PeekMessageW:NtUserPeekMessage:xxxInternalGetMessage:PsGetThreadWin32Thread
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

BOOL km::hook(QWORD kernel_function, QWORD detour_routine, QWORD *original_function)
{
	if (original_function)
		*original_function = get_relative_address((QWORD)kernel_function, 1, 5);

	*(int*)((QWORD)kernel_function + 1) = get_relative_address_offset( (QWORD)detour_routine, (QWORD)kernel_function );
	if (get_relative_address((QWORD)kernel_function, 1, 5) != (QWORD)detour_routine)
	{
		*(int*)((QWORD)kernel_function + 1) = get_relative_address_offset2( (QWORD)detour_routine, (QWORD)kernel_function );
		if (get_relative_address((QWORD)kernel_function, 1, 5) != (QWORD)detour_routine)
		{
			return 0;
		}
	}

	return 1;
}

BOOLEAN km::initialize(QWORD ntoskrnl)
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
	if (!hook((QWORD)PsGetThreadWin32Thread, (QWORD)PsGetThreadWin32ThreadHook, 0))
	{
		return 0;
	}

	NtUserPeekMessage = 0;
	_KeAcquireSpinLockAtDpcLevel = (QWORD)KeAcquireSpinLockAtDpcLevel;
	_KeReleaseSpinLockFromDpcLevel = (QWORD)KeReleaseSpinLockFromDpcLevel;
	_IofCompleteRequest = (QWORD)IofCompleteRequest;
	_IoReleaseRemoveLockEx = (QWORD)IoReleaseRemoveLockEx;

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
