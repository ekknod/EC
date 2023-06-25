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
	void DrawRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b);
	void DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b);
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
	unsigned char r, unsigned char g, unsigned b)
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
	FrameRect(hdc, &rect, brush, 2);
	NtUserReleaseDC(hdc);
	NtGdiDeleteObjectApp(brush);
}

void gdi::DrawFillRect(VOID *hwnd, LONG x, LONG y, LONG w, LONG h, unsigned char r, unsigned char g, unsigned b)
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
