#include "MainWindow.h"
#include "DPIHelpers.h"

LRESULT CMainWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

// static
HRESULT CMainWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = LoadIconW(g_hinst, MAKEINTRESOURCEW(IDI_NTMU));
	return CWindow::RegisterWindowClass(&wc);
}

static inline void ScreenCenteredRect(
	int cx,
	int cy,
	DWORD dwStyle,
	DWORD dwExStyle,
	bool fMenu,
	LPRECT lprc
)
{
	UINT uDpi = DPIHelpers::GetSystemDPI();
	RECT rc = { 0, 0, cx, cy };
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

// static
CMainWindow *CMainWindow::CreateAndShow(int nCmdShow)
{
	constexpr DWORD c_dwMainWindowStyle = WS_OVERLAPPEDWINDOW;
	constexpr DWORD c_dwMainWindowExStyle = NULL;

	RECT rc;
	ScreenCenteredRect(500, 575, c_dwMainWindowStyle, c_dwMainWindowExStyle, false, &rc);

	CMainWindow *pWindow = Create(
		c_dwMainWindowExStyle,
		L"Windows NT Modding Utility",
		c_dwMainWindowStyle,
		rc.left, rc.top,
		RECTWIDTH(rc), RECTHEIGHT(rc),
		NULL, NULL,
		g_hinst,
		nullptr
	);

	if (!pWindow)
		return nullptr;

	ShowWindow(pWindow->_hwnd, nCmdShow);
	return pWindow;
}