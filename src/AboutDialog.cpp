#include "AboutDialog.h"
#include "Util.h"

const WCHAR c_szGitHubURL[] = L"https://github.com/get-ntmu/NTMU";

CAboutDialog::CAboutDialog()
	: _hwndIcon(NULL)
	, _hwndAppName(NULL)
	, _hwndAppVersion(NULL)
	, _hwndAppInfo(NULL)
	, _hwndGitHubLink(NULL)
	, _hwndOKButton(NULL)
	, _hIcon(NULL)
	, _cxIcon(0)
	, _cyIcon(0)
{

}

CAboutDialog::~CAboutDialog()
{
	if (_hIcon)
		DestroyIcon(_hIcon);
}

LRESULT CAboutDialog::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			_OnCreate();
			return 0;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
				SendMessageW(hWnd, WM_CLOSE, 0, 0);
			return 0;
		// Mimic dialog IDOK/IDCANCEL behavior
		case WM_KEYDOWN:
			if (wParam == VK_RETURN || wParam == VK_ESCAPE)
				SendMessageW(hWnd, WM_CLOSE, 0, 0);
			return 0;
		case WM_NOTIFY:
			if (IDC_GITHUB == wParam
			&& NM_CLICK == ((LPNMHDR)lParam)->code)
			{
				ShellExecuteW(
					hWnd, L"open",
					c_szGitHubURL,
					NULL, NULL,
					SW_SHOWNORMAL
				);
			}
			return 0;
		case WM_SETTINGCHANGE:
			if (wParam != SPI_SETNONCLIENTMETRICS)
				return 0;
			// fallthrough
		case WM_THEMECHANGED:
		case WM_DPICHANGED:
			_UpdateMetrics();
			return 0;
		// Re-enable parent window and bring it to front (for some reason it
		// messes up the z-order by default...)
		case WM_DESTROY:
			EnableWindow(g_hwndMain, TRUE);
			SetWindowPos(g_hwndMain, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			return 0;
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CAboutDialog::_OnCreate()
{
	_hwndIcon = CreateWindowExW(
		0, L"STATIC", nullptr,
		SS_ICON | WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, NULL, g_hinst, nullptr
	);

	_hwndAppName = CreateWindowExW(
		0, L"STATIC", L"Windows NT Modding Utility",
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, NULL, g_hinst, nullptr
	);

	WCHAR szVersion[256];
	swprintf_s(szVersion, L"Version %u.%u.%u", VER_MAJOR, VER_MINOR, VER_REVISION);

	_hwndAppVersion = CreateWindowExW(
		0, L"STATIC", szVersion,
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, NULL, g_hinst, nullptr
	);

	_hwndAppInfo = CreateWindowExW(
		0, L"STATIC", 
		L"NTMU is a system file modification tool for Windows. It is free and open-source software, licensed under the GNU General Public License (GPL) 3.0.",
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, NULL, g_hinst, nullptr
	);

	_hwndGitHubLink = CreateWindowExW(
		0, L"SysLink",
		L"<A>GitHub repository</A>",
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, (HMENU)IDC_GITHUB, g_hinst, nullptr
	);

	_hwndOKButton = CreateWindowExW(
		0, L"BUTTON",
		L"OK",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON,
		0, 0, 0, 0,
		_hwnd, (HMENU)IDOK, g_hinst, nullptr
	);

	_UpdateMetrics();
	RECT rc;
	GetClientRect(_hwnd, &rc);
	ParentCenteredRect(
		g_hwndMain, // GetParent returns NULL during WM_CREATE, and we know this
		            // will always be the parent.
		RECTWIDTH(rc),
		RECTHEIGHT(rc),
		WS_CAPTION | WS_SYSMENU,
		0, false,
		&rc
	);

	SetWindowPos(
		_hwnd, NULL,
		rc.left, rc.top,
		0, 0,
		SWP_NOZORDER | SWP_NOSIZE
	);
}

void CAboutDialog::_UpdateMetrics()
{
	CNTMUWindowBase::_UpdateMetrics();

#define UPDATEFONT(hwnd) SendMessageW(hwnd, WM_SETFONT, (WPARAM)_hfMessage, TRUE)
	UPDATEFONT(_hwndAppName);
	UPDATEFONT(_hwndAppVersion);
	UPDATEFONT(_hwndAppInfo);
	UPDATEFONT(_hwndGitHubLink);
	UPDATEFONT(_hwndOKButton);
#undef UPDATEFONT

	_cxIcon = DPIHelpers::GetSystemMetricsForDPI(SM_CXICON, _dpi);
	_cyIcon = DPIHelpers::GetSystemMetricsForDPI(SM_CYICON, _dpi);

	if (_hIcon)
		DestroyIcon(_hIcon);
	_hIcon = (HICON)LoadImageW(g_hinst, MAKEINTRESOURCEW(IDI_NTMU), IMAGE_ICON, _cxIcon, _cyIcon, LR_DEFAULTCOLOR);
	SendMessageW(_hwndIcon, STM_SETICON, (WPARAM)_hIcon, 0);

	_UpdateLayout();
}

void CAboutDialog::_UpdateLayout()
{
	constexpr int c_duDialogWidth  = (c_duMargin * 3) + c_duLabelWidth;
	constexpr int c_duDialogHeight = (c_duLabelHeight * 6) + (c_duMargin * 3) + (c_duLabelMargin * 2);
	RECT rc = {
		0, 0,
		_cxIcon + _XDUToXPix(c_duDialogWidth),
		_cyIcon + _YDUToYPix(c_duDialogHeight)
	};
	DPIHelpers::AdjustWindowRectForDPI(&rc, WS_CAPTION | WS_SYSMENU, 0, FALSE, _dpi);
	SetWindowPos(
		_hwnd, NULL,
		0, 0,
		RECTWIDTH(rc),
		RECTHEIGHT(rc),
		SWP_NOMOVE | SWP_NOZORDER
	);

	HDWP hdwp = BeginDeferWindowPos(c_numLayoutWindows);

	hdwp = DeferWindowPos(
		hdwp, _hwndIcon, NULL,
		_XDUToXPix(c_duMargin), _YDUToYPix(c_duMargin),
		_cxIcon, _cyIcon,
		SWP_NOZORDER
	);

	const int labelX = _cxIcon + _XDUToXPix(c_duMargin * 2);
	const int labelWidth = _XDUToXPix(c_duLabelWidth);
	const int labelHeight = _YDUToYPix(c_duLabelHeight);
	
	hdwp = DeferWindowPos(
		hdwp, _hwndAppName, NULL,
		labelX, _YDUToYPix(c_duMargin),
		labelWidth, labelHeight,
		SWP_NOZORDER
	);

	hdwp = DeferWindowPos(
		hdwp, _hwndAppVersion, NULL,
		labelX, _YDUToYPix(c_duMargin + c_duLabelHeight + c_duLabelMargin),
		labelWidth, labelHeight,
		SWP_NOZORDER
	);

	const int wideLabelX = _XDUToXPix(c_duMargin);
	const int wideLabelWidth = _cxIcon + _XDUToXPix(c_duMargin + c_duLabelWidth);

	hdwp = DeferWindowPos(
		hdwp, _hwndAppInfo, NULL,
		wideLabelX, _cyIcon + _YDUToYPix(c_duMargin * 2),
		wideLabelWidth, _YDUToYPix(c_duLabelHeight * 3),
		SWP_NOZORDER
	);

	constexpr int c_duGitHubLinkY = (c_duLabelHeight * 3) + (c_duMargin * 2) + c_duLabelMargin;

	hdwp = DeferWindowPos(
		hdwp, _hwndGitHubLink, NULL,
		wideLabelX, _cyIcon + _YDUToYPix(c_duGitHubLinkY),
		wideLabelWidth, labelHeight,
		SWP_NOZORDER
	);

	constexpr int c_duOKButtonX = c_duDialogWidth - c_duButtonMargin - c_duButtonWidth;
	constexpr int c_duOKButtonY = c_duDialogHeight - c_duButtonMargin - c_duButtonHeight;

	hdwp = DeferWindowPos(
		hdwp, _hwndOKButton, NULL,
		_cxIcon + _XDUToXPix(c_duOKButtonX), _cyIcon + _YDUToYPix(c_duOKButtonY),
		_XDUToXPix(c_duButtonWidth), _YDUToYPix(c_duButtonHeight),
		SWP_NOZORDER
	);

	EndDeferWindowPos(hdwp);
}

// static
HRESULT CAboutDialog::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	return CWindow::RegisterWindowClass(&wc);
}

// static
CAboutDialog *CAboutDialog::CreateAndShow(HWND hwndParent)
{
	CAboutDialog *pDialog = Create(
		WS_EX_DLGMODALFRAME,
		L"About Windows NT Modding Utility",
		WS_CAPTION | WS_SYSMENU,
		0, 0, 0, 0,
		hwndParent,
		NULL, g_hinst,
		nullptr
	);

	if (!pDialog)
		return nullptr;

	// Disable parent window to make it unclickable (like a modal dialog)
	EnableWindow(hwndParent, FALSE);
	ShowWindow(pDialog->_hwnd, SW_SHOW);
	return pDialog;
}