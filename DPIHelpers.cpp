#include "DPIHelpers.h"

UINT DPIHelpers::GetSystemDPI()
{
	typedef UINT (WINAPI *GetDpiForSystem_t)(void);
	static GetDpiForSystem_t pfnGetDpiForSystem =
		(GetDpiForSystem_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForSystem");
	if (pfnGetDpiForSystem)
		return pfnGetDpiForSystem();

	HDC hdc = GetDC(NULL);
	int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
	ReleaseDC(NULL, hdc);
	return dpi;
}

UINT DPIHelpers::GetWindowDPI(HWND hwnd)
{
	typedef UINT (WINAPI *GetDpiForWindow_t)(HWND hwnd);
	static GetDpiForWindow_t pfnGetDpiForWindow =
		(GetDpiForWindow_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
	if (pfnGetDpiForWindow)
		return pfnGetDpiForWindow(hwnd);

	HDC hdc = GetDC(hwnd);
	int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
	ReleaseDC(hwnd, hdc);
	return dpi;
}

BOOL DPIHelpers::AdjustWindowRectForDPI(LPRECT lprc, DWORD dwStyle, DWORD dwExStyle, BOOL fMenu, UINT dpi)
{
	typedef BOOL (WINAPI *AdjustWindowRectExForDpi_t)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
	static AdjustWindowRectExForDpi_t pfnAdjustWindowRectExForDpi =
		(AdjustWindowRectExForDpi_t)GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi");
	if (pfnAdjustWindowRectExForDpi)
		return pfnAdjustWindowRectExForDpi(lprc, dwStyle, fMenu, dwExStyle, dpi);

	return AdjustWindowRectEx(lprc, dwStyle, fMenu, dwExStyle);
}