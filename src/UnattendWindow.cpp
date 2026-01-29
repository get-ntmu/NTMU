#include "UnattendWindow.h"
#include "Logging.h"
#include "ThumbCache.h"
#include <locale>
#include <codecvt>
#include <pathcch.h>

#include "Util.h"

LRESULT CUnattendWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
				case IDC_CLOSE:
					PostMessageW(hWnd, WM_CLOSE, 0, 0);
					break;
			}
			return 0;
		// Make the read-only text control use Window background
		// instead of ButtonFace to match the main window's log output.
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == _hwndText)
				return (LRESULT)(COLOR_WINDOW + 1);
			goto DWP;
		case WM_SETTINGCHANGE:
			if (wParam != SPI_SETNONCLIENTMETRICS)
				return 0;
			// fallthrough
		case WM_THEMECHANGED:
		case WM_DPICHANGED:
			_UpdateMetrics();
			return 0;
		case WM_SIZE:
			_UpdateLayout();
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_CLOSE && _fApplying)
				return 0;
			goto DWP;
		default:
DWP:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CUnattendWindow::_OnCreate()
{
	_hwndLabel = CreateWindowExW(
		NULL, L"STATIC", L"Loading pack...", WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);
	
	_hwndProgress = CreateWindowExW(
		NULL, PROGRESS_CLASSW, nullptr,
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH | PBS_SMOOTHREVERSE, 0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);
	SendMessageW(_hwndProgress, PBM_SETSTEP, 1, 0);

	WCHAR szCloseText[MAX_PATH] = { 0 };
	LoadStringW(g_hinst, IDS_CLOSE, szCloseText, MAX_PATH);
	_hwndClose = CreateWindowExW(
		NULL, WC_BUTTONW, szCloseText,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		_hwnd, (HMENU)IDC_CLOSE, NULL, NULL
	);
	EnableWindow(_hwndClose, FALSE);

	_hwndText = CreateWindowExW(
		WS_EX_CLIENTEDGE, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY,
		0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);

	_UpdateMetrics();
	
	if (_LoadPack())
	{
		_ApplyPack();
	}
	else
	{
		PostMessageW(_hwnd, WM_CLOSE, 0, 0);
	}
}

void CUnattendWindow::_UpdateMetrics()
{
	CNTMUWindowBase::_UpdateMetrics();

#define UPDATEFONT(hwnd) SendMessageW(hwnd, WM_SETFONT, (WPARAM)_hfMessage, TRUE)
	UPDATEFONT(_hwndLabel);
	UPDATEFONT(_hwndClose);
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

	_UpdateLayout();
}

void CUnattendWindow::_UpdateLayout()
{
	RECT rcClient;
	GetClientRect(_hwnd, &rcClient);

	const int marginX = _XDUToXPix(6);
	const int marginY = _YDUToYPix(4);
	const int weakMarginY = _YDUToYPix(2);

	constexpr int nWindows = c_numLayoutWindows;
	HDWP hdwp = BeginDeferWindowPos(nWindows);

	// Position and size labels
	const int labelWidth = _XDUToXPix(30);
	const int labelHeight = _YDUToYPix(8);
	hdwp = DeferWindowPos(
		hdwp, _hwndLabel, NULL,
		marginX, marginY,
		RECTWIDTH(rcClient) - (marginX * 2),
		labelHeight,
		SWP_NOZORDER
	);
	
	// Position and size progress bar
	const int progressBarY = marginY + labelHeight + weakMarginY;
	const int progressBarHeight = _YDUToYPix(14);
	hdwp = DeferWindowPos(
		hdwp, _hwndProgress, NULL,
		marginX, progressBarY,
		RECTWIDTH(rcClient) - (marginX * 2),
		progressBarHeight,
		SWP_NOZORDER
	);
	
	// Position and size bottom row buttons
	const int buttonWidth = _XDUToXPix(50);
	const int buttonHeight = _YDUToYPix(14);
	const int closeButtonX = RECTWIDTH(rcClient) - buttonWidth - marginX;
	const int buttonY = RECTHEIGHT(rcClient) - buttonHeight - marginY;
	hdwp = DeferWindowPos(
		hdwp, _hwndClose, NULL,
		closeButtonX, buttonY,
		buttonWidth, buttonHeight,
		SWP_NOZORDER
	);
	
	// Position and size text box
	const int textboxY = progressBarY + progressBarHeight + weakMarginY;
	const int textboxHeight = buttonY - textboxY - weakMarginY;
	hdwp = DeferWindowPos(
		hdwp, _hwndText, NULL,
		marginX, textboxY,
		RECTWIDTH(rcClient) - (marginX * 2),
		textboxHeight,
		SWP_NOZORDER
	);

	EndDeferWindowPos(hdwp);
}

bool CUnattendWindow::_LoadPack()
{
	if (!_pack.LoadCommandLineDefault())
	{
		MainWndMsgBox(L"Failed to load pack.", MB_ICONERROR | MB_OK);
		return false;
	}
	return true;
}

// static
void CUnattendWindow::s_ApplyProgressCallback(void *lpParam, DWORD dwItemsProcessed, DWORD dwTotalItems)
{
	CUnattendWindow *pThis = (CUnattendWindow *)lpParam;
	PostMessageW(pThis->_hwndProgress, PBM_DELTAPOS, 100 / dwTotalItems, 0);
}

void CUnattendWindow::_ApplyPack()
{
	CreateThread(nullptr, 0, s_ApplyPackThreadProc, this, NULL, nullptr);
}

// static
DWORD CALLBACK CUnattendWindow::s_ApplyPackThreadProc(LPVOID lpParam)
{
	((CUnattendWindow *)lpParam)->_ApplyPackWorker();
	ExitThread(0);
	return 0;
}

// static
void CUnattendWindow::s_LogCallback(void *lpParam, LPCWSTR pszText)
{
	CUnattendWindow *pThis = (CUnattendWindow *)lpParam;

	HWND hwnd = pThis->_hwndText;
	size_t length = GetWindowTextLengthW(hwnd) + 1;
	LPWSTR pszBuffer = new WCHAR[length];
	GetWindowTextW(hwnd, pszBuffer, length);
	std::wstring newText = pszBuffer;
	delete[] pszBuffer;
	newText += pszText;
	newText += L"\r\n";
	SetWindowTextW(hwnd, newText.c_str());
}

void CUnattendWindow::_ApplyPackWorker()
{
	HMENU hmenuSystem = GetSystemMenu(_hwnd, FALSE);
	
	WCHAR szBuffer[1024] = {};
	swprintf_s(szBuffer, ARRAYSIZE(szBuffer),
		L"Applying pack \"%s\"...", _pack.GetName().c_str());
	SetWindowTextW(_hwndLabel, szBuffer);
	
	SetWindowTextW(_hwndText, L"");
	
	EnableMenuItem(hmenuSystem, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
	EnableWindow(_hwndClose, FALSE);

	DWORD dwCallbackID;
	AddLogCallback(s_LogCallback, this, &dwCallbackID);

	_fApplying = true;
	bool fSuccessfulApplication = false;

	if (!_pack.Apply(this, s_ApplyProgressCallback))
	{
		// If the progress bar is not progressed that much beyond 0, then set it to
		// 100% to emphasize the error. Otherwise, nothing would show at all.
		// This is currently done before setting the progress bar to the error state.
		// While it would be desirable to have the progress bar show as red when it's being
		// filled, this actually doesn't seem possible with the default implementation in
		// common controls, which means that there's an annoying animation where the progress
		// bar is filled with green before switching to red. This could be fixed with
		// subclassing, but I don't want to write that code right this minute, so I'll save
		// that work for the future.
		// TODO(leymonaide): Read above.
		if (SendMessageW(_hwndProgress, PBM_GETPOS, 0, 0) < 10)
		{
			SendMessageW(_hwndProgress, PBM_SETPOS, 100, 0);
		}
		
		SendMessageW(_hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);
		
		EnableMenuItem(hmenuSystem, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
		EnableWindow(_hwndClose, TRUE);
		
		ZeroMemory(szBuffer, ARRAYSIZE(szBuffer));
		swprintf_s(szBuffer, ARRAYSIZE(szBuffer),
			L"Failed to apply pack \"%s\".", _pack.GetName().c_str());
		SetWindowTextW(_hwndLabel, szBuffer);
		
		ZeroMemory(szBuffer, sizeof(szBuffer));
		swprintf_s(szBuffer, ARRAYSIZE(szBuffer), 
			L"Failed to apply pack \"%s\". See the log for more details.", _pack.GetName().c_str());
		
		MainWndMsgBox(szBuffer, MB_ICONERROR);
	}
	else
	{
		SendMessageW(_hwndProgress, PBM_SETSTATE, PBST_NORMAL, 0);
		SendMessageW(_hwndProgress, PBM_SETPOS, 0, 0);
		fSuccessfulApplication = true;
	}

	_fApplying = false;

	RemoveLogCallback(dwCallbackID);
	
	EnableMenuItem(hmenuSystem, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	EnableWindow(_hwndClose, TRUE);
	
	if (fSuccessfulApplication)
	{
		// If the application of the pack was successful, then an unattended window
		// should just close silently.
		PostMessageW(_hwnd, WM_CLOSE, 0, 0);
	}
}

CUnattendWindow::CUnattendWindow()
	: _hwndLabel(NULL)
	, _hwndProgress(NULL)
	, _hwndText(NULL)
	, _hwndClose(NULL)
	, _hfMonospace(NULL)
	, _fApplying(false)
{
}

CUnattendWindow::~CUnattendWindow()
{	
	if (_hfMonospace)
		DeleteObject(_hfMonospace);
}

// static
HRESULT CUnattendWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = LoadIconW(g_hinst, MAKEINTRESOURCEW(IDI_NTMU));
	return CWindow::RegisterWindowClass(&wc);
}

// static
CUnattendWindow *CUnattendWindow::CreateAndShow(int nCmdShow)
{
	constexpr DWORD c_dwMainWindowStyle = (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
	constexpr DWORD c_dwMainWindowExStyle = NULL;

	RECT rc;
	ScreenCenteredRect(450, 300, c_dwMainWindowStyle, c_dwMainWindowExStyle, true, &rc);

	CUnattendWindow *pWindow = Create(
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