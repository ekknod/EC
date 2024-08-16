#include "../../cs2/shared/cs2game.h"
#include "../../csgo/shared/csgogame.h"
#include "../../apex/shared/apexgame.h"
int _fltused;

//
// used by vm.cpp
//
QWORD g_memory_range_low  = 0;
QWORD g_memory_range_high = 0;


//
// used by system_thread
//
BOOL   gExitCalled   = 0;
PVOID  gThreadObject = 0;
HANDLE gThreadHandle = 0;

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
} MOUSE_OBJECT, * PMOUSE_OBJECT;

BOOL mouse_open(void);

extern "C" VOID MouseClassServiceCallback(
	PDEVICE_OBJECT DeviceObject,
	PMOUSE_INPUT_DATA InputDataStart,
	PMOUSE_INPUT_DATA InputDataEnd,
	PULONG InputDataConsumed
);

MOUSE_OBJECT gMouseObject;
extern "C" {
	QWORD _KeAcquireSpinLockAtDpcLevel;
	QWORD _KeReleaseSpinLockFromDpcLevel;
	QWORD _IofCompleteRequest;
	QWORD _IoReleaseRemoveLockEx;
};

namespace mouse
{
	void move(long x, long y, unsigned short button_flags);
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
	
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(w);
		UNREFERENCED_PARAMETER(h);
		UNREFERENCED_PARAMETER(r);
		UNREFERENCED_PARAMETER(g);
		UNREFERENCED_PARAMETER(b);
	}

	void DrawFillRect(void *hwnd, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b)
	{
		UNREFERENCED_PARAMETER(hwnd);
		UNREFERENCED_PARAMETER(x);
		UNREFERENCED_PARAMETER(y);
		UNREFERENCED_PARAMETER(w);
		UNREFERENCED_PARAMETER(h);
		UNREFERENCED_PARAMETER(r);
		UNREFERENCED_PARAMETER(g);
		UNREFERENCED_PARAMETER(b);
	}
}

static void NtSleep(DWORD milliseconds);

NTSTATUS system_thread(void)
{
	while (gExitCalled == 0)
	{
		NtSleep(1);
		if (cs2::game_handle)
		{
			if (cs2::running())
			{
				cs2::features::run();
			}
			else
			{
				cs2::features::reset();
			}
		}
		else if (csgo::game_handle)
		{
			if (csgo::running())
			{
				csgo::features::run();
			}
			else
			{
				csgo::features::reset();
			}
		}
		else if (apex::game_handle)
		{
			if (apex::running())
			{
				apex::features::run();
			}
			else
			{
				apex::features::reset();
			}
		}
		else
		{
			BOOL is_running = 0;
			if (!is_running) is_running = cs2::running();
			if (!is_running) is_running = csgo::running();
			if (!is_running) is_running = apex::running();
		}
	}
	return STATUS_SUCCESS;
}

VOID
DriverUnload(
	_In_ struct _DRIVER_OBJECT* DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);

	gExitCalled = 1;
	if (gThreadObject) {
		KeWaitForSingleObject(
			(PVOID)gThreadObject,
			Executive,
			KernelMode,
			FALSE,
			0
		);

		ObDereferenceObject(gThreadObject);

		ZwClose(gThreadHandle);
	}

	NtSleep(1000);


	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] EC-CS2.sys is closed\n");
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;

	_KeAcquireSpinLockAtDpcLevel = (QWORD)KeAcquireSpinLockAtDpcLevel;
	_KeReleaseSpinLockFromDpcLevel = (QWORD)KeReleaseSpinLockFromDpcLevel;
	_IofCompleteRequest = (QWORD)IofCompleteRequest;
	_IoReleaseRemoveLockEx = (QWORD)IoReleaseRemoveLockEx;

	PPHYSICAL_MEMORY_RANGE memory_range = MmGetPhysicalMemoryRanges();
	if (memory_range == 0)
		return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;

	int counter=0;
	while (1)
	{
		if (memory_range[counter].BaseAddress.QuadPart == 0)
		{
			break;
		}
		counter++;
	}
	
	g_memory_range_low = memory_range[0].BaseAddress.QuadPart;
	g_memory_range_high = memory_range[counter - 1].BaseAddress.QuadPart + memory_range[counter - 1].NumberOfBytes.QuadPart;
	ExFreePool(memory_range);

	CLIENT_ID thread_id;
	PsCreateSystemThread(&gThreadHandle, STANDARD_RIGHTS_ALL, NULL, NULL, &thread_id, (PKSTART_ROUTINE)system_thread, (PVOID)0);
	ObReferenceObjectByHandle(
		gThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		(PVOID*)&gThreadObject,
		NULL
	);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[+] EC-CS2.sys is started\n");

	return STATUS_SUCCESS;
}


extern "C" NTSYSCALLAPI
NTSTATUS
ObReferenceObjectByName(
      __in PUNICODE_STRING ObjectName,
      __in ULONG Attributes,
      __in_opt PACCESS_STATE AccessState,
      __in_opt ACCESS_MASK DesiredAccess,
      __in POBJECT_TYPE ObjectType,
      __in KPROCESSOR_MODE AccessMode,
      __inout_opt PVOID ParseContext,
      __out PVOID *Object
  );

extern "C" NTSYSCALLAPI POBJECT_TYPE* IoDriverObjectType;

BOOL mouse_open(void)
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

		PVOID class_driver_base = NULL;


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
				class_driver_base = class_driver_object->DriverStart;
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

#define KeMRaiseIrql(a,b) *(b) = KfRaiseIrql(a)
void mouse::move(long x, long y, unsigned short button_flags)
{
	KIRQL irql;
	ULONG input_data;
	MOUSE_INPUT_DATA mid = { 0 };
	mid.LastX = x;
	mid.LastY = y;
	mid.ButtonFlags = button_flags;
	if (!mouse_open()) {
		return;
	}
	mid.UnitId = 1;
	KeMRaiseIrql(DISPATCH_LEVEL, &irql);	
	MouseClassServiceCallback(gMouseObject.mouse_device, &mid, (PMOUSE_INPUT_DATA)&mid + 1, &input_data);
	KeLowerIrql(irql);
}

static void NtSleep(DWORD milliseconds)
{
	QWORD ms = milliseconds;
	ms = (ms * 1000) * 10;
	ms = ms * -1;
#ifdef _KERNEL_MODE
	KeDelayExecutionThread(KernelMode, 0, (PLARGE_INTEGER)&ms);
#else
	NtDelayExecution(0, (PLARGE_INTEGER)&ms);
#endif
}

