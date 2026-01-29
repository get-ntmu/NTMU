#include "Util.h"
#include "DPIHelpers.h"

void trim(std::wstring &s)
{
	// trim right
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
		return !std::isspace(ch);
		}).base(), s.end());
	// trim left
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
		return !std::isspace(ch);
		}));
}

void ScreenCenteredRect(
	int cx,
	int cy,
	DWORD dwStyle,
	DWORD dwExStyle,
	bool fMenu,
	LPRECT lprc
)
{
	UINT uDpi = DPIHelpers::GetSystemDPI();
	RECT rc = {
		0, 0,
		MulDiv(cx, uDpi, 96),
		MulDiv(cy, uDpi, 96)
	};
	DPIHelpers::AdjustWindowRectForDPI(&rc, dwStyle, dwExStyle, fMenu, uDpi);
	cx = RECTWIDTH(rc);
	cy = RECTHEIGHT(rc);
	int scx = GetSystemMetrics(SM_CXSCREEN);
	int scy = GetSystemMetrics(SM_CYSCREEN);
	int x = (scx - cx) / 2;
	int y = (scy - cy) / 2;
	lprc->left = x;
	lprc->top = y;
	lprc->right = x + cx;
	lprc->bottom = y + cy;
}

void ParentCenteredRect(
	HWND hwndParent,
	int cx,
	int cy,
	DWORD dwStyle,
	DWORD dwExStyle,
	bool fMenu,
	LPRECT lprc
)
{
	UINT uDpi = DPIHelpers::GetWindowDPI(hwndParent);
	RECT rc = {
		0, 0,
		MulDiv(cx, uDpi, 96),
		MulDiv(cy, uDpi, 96)
	};
	DPIHelpers::AdjustWindowRectForDPI(&rc, dwStyle, dwExStyle, fMenu, uDpi);
	cx = RECTWIDTH(rc);
	cy = RECTHEIGHT(rc);

	RECT rcParent;
	GetWindowRect(hwndParent, &rcParent);
	int pcx = RECTWIDTH(rcParent);
	int pcy = RECTHEIGHT(rcParent);
	int x = rcParent.left + (pcx - cx) / 2;
	int y = rcParent.top  + (pcy - cy) / 2;
	lprc->left = x;
	lprc->top = y;
	lprc->right = x + cx;
	lprc->bottom = y + cx;
}

HRESULT WaitForProcess(LPCWSTR pszCommandLine, DWORD *lpdwExitCode)
{
	if (!lpdwExitCode)
		RETURN_HR(E_INVALIDARG);

	PROCESS_INFORMATION pi;
	STARTUPINFOW si = { sizeof(si) };
	if (!CreateProcessW(
		nullptr,
		(LPWSTR)pszCommandLine,
		nullptr,
		nullptr,
		FALSE,
		NULL,
		nullptr,
		nullptr,
		&si,
		&pi
	))
		RETURN_LAST_ERROR_MSG("Failed to create process.");

	WaitForSingleObject(pi.hProcess, INFINITE);

	return GetExitCodeProcess(pi.hProcess, lpdwExitCode)
		? S_OK
		: S_FALSE;
}