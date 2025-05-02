#include "MainWindow.h"
#include "DPIHelpers.h"
#include <stdio.h>

INT_PTR CALLBACK CMainWindow::s_AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			WCHAR szFormat[256], szBuffer[256];
			GetDlgItemTextW(hWnd, IDC_VERSION, szFormat, 256);
			swprintf_s(
				szBuffer, 256, szFormat,
				VER_MAJOR, VER_MINOR, VER_REVISION
			);
			SetDlgItemTextW(hWnd, IDC_VERSION, szBuffer);
			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDC_GITHUB:
					ShellExecuteW(
						hWnd, L"open",
						L"https://github.com/aubymori/NTMU",
						NULL, NULL,
						SW_SHOWNORMAL
					);
					// fall-thru
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

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
				case IDM_HELPABOUT:
					DialogBoxParamW(g_hinst, MAKEINTRESOURCEW(IDD_ABOUT), hWnd, s_AboutDlgProc, NULL);
					break;
			}
			return 0;
		// Make the read-only text control use Window background
		// instead of ButtonFace. This looks better with the options
		// pane.
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == _hwndText)
				return (LRESULT)(COLOR_WINDOW + 1);
			goto DWP;
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
DWP:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CMainWindow::_OnCreate()
{
	_hAccel = LoadAcceleratorsW(g_hinst, MAKEINTRESOURCEW(IDA_MAIN));

	for (int i = 0; i < MI_COUNT; i++)
	{
		WCHAR szLabelText[MAX_PATH] = { 0 };
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

	_hwndProgress = CreateWindowExW(
		NULL, PROGRESS_CLASSW, nullptr,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);

	WCHAR szApplyText[MAX_PATH] = { 0 };
	LoadStringW(g_hinst, IDS_APPLY, szApplyText, MAX_PATH);
	_hwndApply = CreateWindowExW(
		NULL, WC_BUTTONW, szApplyText,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		_hwnd, (HMENU)IDC_APPLY, NULL, NULL
	);

	_hwndText = CreateWindowExW(
		WS_EX_CLIENTEDGE, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOHSCROLL | ES_READONLY,
		0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);

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

#define UPDATEFONT(hwnd) SendMessageW(hwnd, WM_SETFONT, (WPARAM)_hfMessage, TRUE)
	for (int i = 0; i < MI_COUNT; i++)
	{
		UPDATEFONT(_rghwndLabels[i]);
		UPDATEFONT(_rghwndMetas[i]);
	}
	UPDATEFONT(_hwndApply);
#undef UPDATEFONT

	// Update text box font (monospace)
	if (_hfMonospace)
	{
		DeleteObject(_hfMonospace);
		_hfMonospace = NULL;
	}

	LOGFONTW lf = { 0 };
	wcscpy_s(lf.lfFaceName, L"Courier New");
	lf.lfHeight = -MulDiv(10, _dpi, 72);
	_hfMonospace = CreateFontIndirectW(&lf);
	SendMessageW(_hwndText, WM_SETFONT, (WPARAM)_hfMonospace, NULL);
	SetWindowTextW(_hwndText, L"test");

	_UpdateLayout();
}

void CMainWindow::_UpdateLayout()
{
	RECT rcClient;
	GetClientRect(_hwnd, &rcClient);

	const int marginX = _XDUToXPix(6);
	const int marginY = _YDUToYPix(4);

	constexpr int nWindows = (MI_COUNT * 2) + 3;
	HDWP hdwp = BeginDeferWindowPos(nWindows);

	// Position and size labels
	const int labelWidth = _XDUToXPix(30);
	const int labelHeight = _YDUToYPix(8);
	const int labelMargin = _YDUToYPix(2);
	const int metaWidth = RECTWIDTH(rcClient) - labelWidth - (2 * marginX);
	const int metaX = marginX + labelWidth;
	for (int i = 0; i < MI_COUNT; i++)
	{
		hdwp = DeferWindowPos(
			hdwp, _rghwndLabels[i], NULL,
			marginX, marginY + ((labelHeight + labelMargin) * i),
			labelWidth, labelHeight, SWP_NOZORDER
		);
		hdwp = DeferWindowPos(
			hdwp, _rghwndMetas[i], NULL,
			metaX, marginY + ((labelHeight + labelMargin) * i),
			metaWidth, labelHeight, SWP_NOZORDER
		);
	}

	// Position and size progress bar and Apply button
	const int buttonWidth = _XDUToXPix(50);
	const int buttonHeight = _YDUToYPix(14);
	const int buttonX = RECTWIDTH(rcClient) - buttonWidth - marginX;
	const int buttonY = RECTHEIGHT(rcClient) - buttonHeight - marginY;
	hdwp = DeferWindowPos(
		hdwp, _hwndApply, NULL,
		buttonX, buttonY,
		buttonWidth, buttonHeight,
		SWP_NOZORDER
	);
	hdwp = DeferWindowPos(
		hdwp, _hwndProgress, NULL,
		marginX, buttonY,
		buttonX - (marginX * 2),
		buttonHeight,
		SWP_NOZORDER
	);
	
	// Position and size panes
	const int panesY = marginY + (labelHeight + labelMargin) * MI_COUNT;
	const int paneWidth = (RECTWIDTH(rcClient) - (marginX * 2)) / 2 - (marginX / 2);
	const int panesHeight = buttonX - (marginY * 2) - panesY;
	hdwp = DeferWindowPos(
		hdwp, _hwndText, NULL,
		marginX, panesY,
		paneWidth, panesHeight,
		SWP_NOZORDER
	);

	EndDeferWindowPos(hdwp);
}

CMainWindow::CMainWindow()
	: _dpi(0)
	, _hAccel(NULL)
	, _rghwndLabels{ NULL }
	, _rghwndMetas{ NULL }
	, _hwndProgress(NULL)
	, _hwndApply(NULL)
	, _hwndText(NULL)
	, _hwndOptions(NULL)
	, _hfMessage(NULL)
	, _hfMonospace(NULL)
	, _cxMsgFontChar(0)
	, _cyMsgFontChar(0)
{
}

CMainWindow::~CMainWindow()
{	
	if (_hfMessage)
		DeleteObject(_hfMessage);
	if (_hfMonospace)
		DeleteObject(_hfMonospace);
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
	ScreenCenteredRect(575, 500, c_dwMainWindowStyle, c_dwMainWindowExStyle, true, &rc);

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