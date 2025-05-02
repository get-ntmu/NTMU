#include "PreviewWindow.h"

LRESULT CPreviewWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			_OnCreate();
			return 0;
		case WM_PAINT:
		{
			return 0;
		}
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CPreviewWindow::_OnCreate()
{
	LoadStringW(g_hinst, IDS_NOPREVIEW, _szNoPreview, MAX_PATH);
	LoadStringW(g_hinst, IDS_PREVIEWFAIL, _szPreviewFailed, MAX_PATH);
}

// static
HRESULT CPreviewWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	return CWindow::RegisterWindowClass(&wc);
}

// static
CPreviewWindow *CPreviewWindow::CreateAndShow(HWND hwndParent)
{
	CPreviewWindow *pWindow = Create(
		NULL,
		nullptr,
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		hwndParent, NULL,
		g_hinst,
		nullptr
	);

	if (!pWindow)
		return nullptr;

	ShowWindow(pWindow->_hwnd, SW_SHOW);
	return pWindow;
}

CPreviewWindow::CPreviewWindow()
	: _szNoPreview{ 0 }
	, _szPreviewFailed{ 0 }
	, _hfMessage(NULL)
{

}

void CPreviewWindow::SetFont(HFONT hFont)
{
	_hfMessage = hFont;
}