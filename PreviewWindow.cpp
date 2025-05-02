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
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT rc;
			GetClientRect(hWnd, &rc);

			FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
			
			LOGBRUSH lb = { 0 };
			lb.lbColor = GetSysColor(COLOR_GRAYTEXT);
			lb.lbStyle = PS_SOLID;
			HPEN hpen = ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, nullptr);
			HPEN hpenOld = (HPEN)SelectObject(hdc, hpen);
			
			MoveToEx(hdc, rc.left, rc.top, nullptr);
			LineTo(hdc, rc.right - 1, rc.top);
			LineTo(hdc, rc.right - 1, rc.bottom - 1);
			LineTo(hdc, rc.left, rc.bottom - 1);
			LineTo(hdc, rc.left, rc.top);

			HFONT hfOld = (HFONT)SelectObject(hdc, _hfMessage);
			int bkOld = SetBkMode(hdc, TRANSPARENT);

			DrawTextW(
				hdc, _szNoPreview, -1,
				&rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORDBREAK
			);

			SetBkMode(hdc, bkOld);
			SelectObject(hdc, hfOld);
			SelectObject(hdc, hpenOld);
			DeleteObject(hpen);
			EndPaint(hWnd, &ps);
			return 0;
		}
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CPreviewWindow::_OnCreate()
{
	LoadStringW(g_hinst, IDS_NOPREVIEW, _szNoPreview, MAX_PATH);
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
	, _hfMessage(NULL)
{

}

void CPreviewWindow::SetFont(HFONT hFont)
{
	_hfMessage = hFont;
}