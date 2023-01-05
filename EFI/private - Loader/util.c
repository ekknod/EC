#include "stdafx.h"

typedef struct {
	UINTN Size;
	UINTN FileSize;
	UINTN PhysicalSize;
	EFI_TIME CreateTime;
	EFI_TIME LastAccessTime;
	EFI_TIME ModificationTime;
	UINT64 Attribute;
	CHAR16 FileName[1];
} EFI_FILE_INFO ;

#define EFI_FILE_INFO_ID \
{ \
0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
}
static EFI_GUID fileInfoId = EFI_FILE_INFO_ID;

static UINT64 getInfoSize(EFI_FILE* file)
{
	UINTN infoSize = 0;
	if (file->GetInfo(file, &fileInfoId, &infoSize, NULL) != EFI_BUFFER_TOO_SMALL)
		return 1;
	return (UINT64)infoSize;
}

static unsigned int getInfo(EFI_FILE* file, EFI_FILE_INFO* info, UINT64 size)
{
	return size > 1 && file->GetInfo(file, &fileInfoId, (UINTN*)&size, info) == EFI_SUCCESS;
}

EFI_STATUS LoadFile(const CHAR16 *path, VOID **buffer, UINTN *buffer_size)
{

	EFI_BOOT_SERVICES *bs = gBS;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_HANDLE* handles = NULL;
	UINTN handleCount = 0;

	EFI_STATUS status = bs->LocateHandleBuffer(ByProtocol,
		&sfspGuid, NULL, &handleCount, &handles);

	if (EFI_ERROR(status))
		return status;

	EFI_FILE_PROTOCOL *file = NULL;
	EFI_FILE_PROTOCOL *root = NULL;
	for (UINTN index = 0; index < handleCount; ++index) {
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
		status = bs->HandleProtocol(handles[index], &sfspGuid, (void**)&fs);
		if (EFI_ERROR(status))
			continue;

		status = fs->OpenVolume(fs, &root);
		if (EFI_ERROR(status))
			continue;

		status = root->Open(root, &file, (CHAR16*)path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
		if (!EFI_ERROR(status))
			break;
	}

	if (EFI_ERROR(status))
		return status;

	if (file == 0)
		return EFI_INVALID_PARAMETER;

	UINTN file_info_size = getInfoSize(file);
	if (file_info_size == 0)
		return EFI_INVALID_PARAMETER;

	EFI_FILE_INFO* info = AllocatePool(file_info_size);
	if (getInfo(file, info, file_info_size) == 0)
		return EFI_INVALID_PARAMETER;

	UINTN file_size = info->FileSize;
	for (UINTN i = file_info_size; i--;)
		((unsigned char *)info)[i] = 0;

	FreePool(info);

	gBS->AllocatePool(EfiBootServicesData, file_size, buffer); // AllocatePool(file_size);
	if (EFI_ERROR(file->Read(file, &file_size, *buffer))) {
		root->Close(file);
		root->Close(root);
		FreePool(*buffer);
		return EFI_INVALID_PARAMETER;
	}

	root->Close(file);
	root->Close(root);

	*buffer_size = file_size;

	return EFI_SUCCESS;
}

BOOLEAN FileExists(const CHAR16 *path)
{

	EFI_BOOT_SERVICES *bs = gBS;
	EFI_GUID sfspGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_HANDLE* handles = NULL;
	UINTN handleCount = 0;

	EFI_STATUS status = bs->LocateHandleBuffer(ByProtocol,
		&sfspGuid, NULL, &handleCount, &handles);

	if (EFI_ERROR(status))
		return 0;

	EFI_FILE_PROTOCOL *file = NULL;
	EFI_FILE_PROTOCOL *root = NULL;
	for (UINTN index = 0; index < handleCount; ++index) {
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
		status = bs->HandleProtocol(handles[index], &sfspGuid, (void**)&fs);
		if (EFI_ERROR(status))
			continue;

		status = fs->OpenVolume(fs, &root);
		if (EFI_ERROR(status))
			continue;

		status = root->Open(root, &file, (CHAR16*)path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
		if (!EFI_ERROR(status))
			break;
	}

	if (EFI_ERROR(status))
		return 0;

	if (file == 0)
		return 0;

	root->Close(file);
	root->Close(root);

	return 1;
}

VOID MemCopy(VOID* dest, VOID* src, UINTN size)
{
	for (UINT8* d = dest, *s = src; size--; *d++ = *s++)
		;
}

