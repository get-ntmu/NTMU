#include "MainWindow.h"
#include "DPIHelpers.h"

LRESULT CMainWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			_OnCreate();
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDM_FILEEXIT:
					PostMessageW(hWnd, WM_CLOSE, 0, 0);
					break;
			}
			return 0;
		case WM_SETTINGCHANGE:
			if (wParam != SPI_SETNONCLIENTMETRICS)
				return 0;
			// fallthrough
		case WM_DPICHANGED:
			_UpdateFonts();
			return 0;
		case WM_SIZE:
			_UpdateLayout();
			return 0;
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CMainWindow::_OnCreate()
{
	_hAccel = LoadAcceleratorsW(g_hinst, MAKEINTRESOURCEW(IDA_MAIN));

	for (int i = 0; i < MI_COUNT; i++)
	{
		WCHAR szLabelText[MAX_PATH];
		LoadStringW(g_hinst, IDS_PACKNAME + i, szLabelText, MAX_PATH);
		_rghwndLabels[i] = CreateWindowExW(
			NULL, L"STATIC", szLabelText, WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0,
			_hwnd, NULL, NULL, NULL
		);
		_rghwndMetas[i] = CreateWindowExW(
			NULL, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0,
			_hwnd, NULL, NULL, NULL
		);
	}

	_UpdateFonts();
}

void CMainWindow::_UpdateFonts()
{
	_dpi = DPIHelpers::GetWindowDPI(_hwnd);

	if (_hfMessage)
	{
		DeleteObject(_hfMessage);
		_hfMessage = NULL;
	}

	NONCLIENTMETRICSW ncm = { sizeof(ncm) };
	DPIHelpers::SystemParametersInfoForDPI(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE, _dpi);
	_hfMessage = CreateFontIndirectW(&ncm.lfMessageFont);

	HDC hdc = CreateCompatibleDC(NULL);
	SelectObject(hdc, _hfMessage);
	
	TEXTMETRICW tm = { 0 };
	GetTextMetricsW(hdc, &tm);

	_cyMsgFontChar = tm.tmHeight;
	if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
	{
		static const WCHAR wszAvgChars[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

		SIZE size;
		if (GetTextExtentPoint32W(hdc, wszAvgChars, ARRAYSIZE(wszAvgChars) - 1, &size))
		{
			// The above string is 26 * 2 characters. + 1 rounds the result.
			_cxMsgFontChar = ((size.cx / 26) + 1) / 2;
		}
	}
	else
	{
		_cxMsgFontChar = tm.tmAveCharWidth;
	}

	for (HWND hwnd : _rghwndLabels)
	{
		SendMessageW(hwnd, WM_SETFONT, (WPARAM)_hfMessage, TRUE);
	}
	for (HWND hwnd : _rghwndMetas)
	{
		SendMessageW(hwnd, WM_SETFONT, (WPARAM)_hfMessage, TRUE);
	}

	_UpdateLayout();
}

void CMainWindow::_UpdateLayout()
{
	RECT rcClient;
	GetClientRect(_hwnd, &rcClient);

	const int marginX = _XDUToXPix(6);
	const int marginY = _YDUToYPix(4);

	constexpr int nWindows = (MI_COUNT * 2);
	HDWP hdwp = BeginDeferWindowPos(nWindows);

	const int labelWidth = _XDUToXPix(30);
	const int labelHeight = _YDUToYPix(10);
	const int metaWidth = RECTWIDTH(rcClient) - labelWidth - (2 * marginX);
	const int metaX = marginX + labelWidth;
	for (int i = 0; i < MI_COUNT; i++)
	{
		hdwp = DeferWindowPos(
			hdwp, _rghwndLabels[i], NULL,
			marginX, marginY + (labelHeight * i),
			labelWidth, labelHeight, SWP_NOZORDER
		);
		hdwp = DeferWindowPos(
			hdwp, _rghwndMetas[i], NULL,
			metaX, marginY + (labelHeight * i),
			metaWidth, labelHeight, SWP_NOZORDER
		);
	}

	EndDeferWindowPos(hdwp);
}

CMainWindow::~CMainWindow()
{	
	if (_hfMessage)
		DeleteObject(_hfMessage);
}

// static
HRESULT CMainWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = LoadIconW(g_hinst, MAKEINTRESOURCEW(IDI_NTMU));
	wc.lpszMenuName = MAKEINTRESOURCEW(IDM_MAINMENU);
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
	ScreenCenteredRect(500, 575, c_dwMainWindowStyle, c_dwMainWindowExStyle, true, &rc);

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

HACCEL CMainWindow::GetAccel()
{
	return _hAccel;
}