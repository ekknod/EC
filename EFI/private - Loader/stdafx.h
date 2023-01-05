#pragma once
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
#include "util.h"


typedef EFI_STATUS(EFIAPI *IMG_ARCH_START_BOOT_APPLICATION)(VOID *, VOID *, UINT32, UINT8, VOID *);
typedef EFI_STATUS(EFIAPI *OSL_FWP_KERNEL_SETUP_PHASE_1)(LOADER_PARAMETER_BLOCK *);
typedef NTSTATUS(*DRIVER_ENTRY)(PDRIVER_OBJECT, VOID*);
