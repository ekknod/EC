#include <ntifs.h>
#include <WinDef.h>
#include <wingdi.h>

typedef ULONG_PTR QWORD;
typedef DWORD LFTYPE;

namespace gdi
{
	typedef HWND(*GetForegroundWindow_t)();//verified
	typedef HDC(*GetDC_t)(HWND hwnd);//verified
	typedef HDC(*GetDCEx_t)(HWND hwnd, HANDLE region, ULONG flags);//verified
	typedef BOOL(*PatBlt_t)(HDC hdcDest, INT x, INT y, INT cx, INT cy, DWORD dwRop);//verified
	typedef HBRUSH (*SelectBrush_t)(HDC hdc, HBRUSH hbrush); //verified
	typedef int (*ReleaseDC_t)(HDC hdc); //verified
	typedef HBRUSH (*CreateSolidBrush_t)( COLORREF cr, HBRUSH hbr); //verified
	typedef BOOL (*DeleteObjectApp_t)(HANDLE hobj); //verified
	typedef BOOL (*ExtTextOutW_t)(IN HDC hDC, //verified
		IN INT 	XStart,
		IN INT 	YStart,
		IN UINT 	fuOptions,
		IN OPTIONAL LPRECT 	UnsafeRect,
		IN LPWSTR 	UnsafeString,
		IN INT 	Count,
		IN OPTIONAL LPINT 	UnsafeDx,
		IN DWORD 	dwCodePage
	);
	typedef HFONT (*HfontCreate_t)(IN PENUMLOGFONTEXDVW pelfw, IN ULONG cjElfw, IN LFTYPE lft, IN FLONG fl, IN PVOID pvCliData); //verified
	typedef HFONT (*SelectFont_t)(_In_ HDC 	hdc, //verified
		_In_ HFONT 	hfont
	);

	GetForegroundWindow_t NtUserGetForegroundWindow = NULL;
	GetDC_t NtUserGetDC = NULL;
	GetDCEx_t NtUserGetDCEx = NULL;
	SelectBrush_t NtGdiSelectBrush = NULL;
	PatBlt_t NtGdiPatBlt = NULL;
	ReleaseDC_t NtUserReleaseDC = NULL;
	CreateSolidBrush_t NtGdiCreateSolidBrush = NULL;
	DeleteObjectApp_t NtGdiDeleteObjectApp = NULL;
	ExtTextOutW_t NtGdiExtTextOutW = NULL;
	HfontCreate_t NtGdiHfontCreate = NULL;
	SelectFont_t NtGdiSelectFont = NULL;

	BOOL init_status = 0;
	BOOL init();
	bool FrameRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr, int thickness);
	bool FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr);
	void DrawRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned char b);
	void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned char b);
}


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
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;
extern "C" __declspec(dllimport) LIST_ENTRY *PsLoadedModuleList;


QWORD GetModuleEntry(PCWSTR module_name)
{
	for (PLIST_ENTRY pListEntry = PsLoadedModuleList->Flink; pListEntry != PsLoadedModuleList; pListEntry = pListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (pEntry->ImageBase == 0)
			continue;

		if (pEntry->BaseImageName.Buffer && !wcscmp(pEntry->BaseImageName.Buffer, module_name))
		{			
			return (QWORD)pEntry->ImageBase;
		}

	}
	return 0;
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
		if (strcmp((PCSTR)a0, export_name) == 0) {
			
			return (base + *(DWORD*)(base + a1[1] + (*(USHORT*)(base + a1[3] + (a1[0] * 2)) * 4)));
		}	
	}
	return 0;
}


QWORD GetModuleEntry(PCWSTR module_name);
QWORD GetProcAddressQ(QWORD base, PCSTR export_name);

BOOL gdi::init()
{
	if (init_status)
	{
		return 1;
	}

	QWORD win32kbase = GetModuleEntry(L"win32kbase.sys");
	if (!win32kbase)
	{
		return 0;
	}

	QWORD win32kfull = GetModuleEntry(L"win32kfull.sys");
	if (!win32kfull)
	{
		return 0;
	}

	NtUserGetForegroundWindow = (GetForegroundWindow_t)GetProcAddressQ(win32kfull, "NtUserGetForegroundWindow");
	if (!NtUserGetForegroundWindow)
	{
		return 0;
	}

	NtUserGetDC = (GetDC_t)GetProcAddressQ(win32kbase, "NtUserGetDC");
	if (!NtUserGetDC)
	{
		return 0;
	}

	NtUserGetDCEx = (GetDCEx_t)GetProcAddressQ(win32kfull, "NtUserGetDCEx");
	if (!NtUserGetDCEx)
	{
		return 0;
	}

	NtGdiPatBlt = (PatBlt_t)GetProcAddressQ(win32kfull, "NtGdiPatBlt");
	if (!NtGdiPatBlt)
	{
		return 0;
	}

	NtGdiSelectBrush = (SelectBrush_t)GetProcAddressQ(win32kbase, "GreSelectBrush");
	if (!NtGdiSelectBrush)
	{
		return 0;
	}

	NtUserReleaseDC = (ReleaseDC_t)GetProcAddressQ(win32kbase, "NtUserReleaseDC");
	if (!NtUserReleaseDC)
	{
		return 0;
	}

	NtGdiCreateSolidBrush = (CreateSolidBrush_t)GetProcAddressQ(win32kfull, "NtGdiCreateSolidBrush");
	if (!NtGdiCreateSolidBrush)
	{
		return 0;
	}

	NtGdiDeleteObjectApp = (DeleteObjectApp_t)GetProcAddressQ(win32kbase, "NtGdiDeleteObjectApp");
	if (!NtGdiDeleteObjectApp)
	{
		return 0;
	}

	NtGdiExtTextOutW = (ExtTextOutW_t)GetProcAddressQ(win32kfull, "NtGdiExtTextOutW");
	if (!NtGdiExtTextOutW)
	{
		return 0;
	}

	NtGdiHfontCreate = (HfontCreate_t)GetProcAddressQ(win32kfull, "hfontCreate");
	if (!NtGdiHfontCreate)
	{
		return 0;
	}

	NtGdiSelectFont = (SelectFont_t)GetProcAddressQ(win32kfull, "NtGdiSelectFont");
	if (!NtGdiSelectFont)
	{
		return 0;
	}
	
	init_status = 1;

	return init_status;
}

bool gdi::FrameRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr, int thickness)
{
	HBRUSH oldbrush;
	RECT r = *lprc;

	if ((r.right <= r.left) || (r.bottom <= r.top)) return false;
	
	oldbrush = NtGdiSelectBrush(hDC, hbr);

	NtGdiPatBlt(hDC, r.left, r.top, thickness, r.bottom - r.top, PATCOPY);
	NtGdiPatBlt(hDC, r.right - thickness, r.top, thickness, r.bottom - r.top, PATCOPY);
	NtGdiPatBlt(hDC, r.left, r.top, r.right - r.left, thickness, PATCOPY);
	NtGdiPatBlt(hDC, r.left, r.bottom - thickness, r.right - r.left, thickness, PATCOPY);

	if (oldbrush)
		NtGdiSelectBrush(hDC, oldbrush);
	return TRUE;
}

bool gdi::FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr)
{
	BOOL Ret;
	HBRUSH prevhbr = NULL;


	prevhbr = NtGdiSelectBrush(hDC, hbr);
	Ret = NtGdiPatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
		lprc->bottom - lprc->top, PATCOPY);

	/* Select old brush */
	if (prevhbr)
		NtGdiSelectBrush(hDC, prevhbr);

	return Ret;
}


void gdi::DrawRect(
	VOID *hwnd,
	LONG x, LONG y, LONG w, LONG h,
	unsigned char r, unsigned char g, unsigned char b)
{
	if (!gdi::init())
	{
		return;
	}

	if (NtUserGetForegroundWindow() != (HWND)hwnd)
	{
		return;
	}

	HDC hdc = NtUserGetDCEx(0x0, 0, 1);
	if (!hdc)
		return;

	HBRUSH brush = NtGdiCreateSolidBrush(RGB(r, g, b), NULL);
	if (!brush)
		return;

	RECT rect = { x, y, x + w, y + h };
	FrameRect(hdc, &rect, brush, 2);
	NtUserReleaseDC(hdc);
	NtGdiDeleteObjectApp(brush);
}

void gdi::DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned char b)
{
	if (!gdi::init())
	{
		return;
	}

	if (NtUserGetForegroundWindow() != (HWND)hwnd)
	{
		return;
	}

	// HDC hdc = NtUserGetDC((HWND)hwnd);
	HDC hdc = NtUserGetDCEx(0x0, 0, 1);
	UNREFERENCED_PARAMETER(hwnd);
	if (!hdc)
		return;

	HBRUSH brush = NtGdiCreateSolidBrush(RGB(r, g, b), NULL);
	if (!brush)
		return;

	RECT rect = { x, y, x + w, y + h };
	FillRect(hdc, &rect, brush);
	NtUserReleaseDC(hdc);
	NtGdiDeleteObjectApp(brush);
}

