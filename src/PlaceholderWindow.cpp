#include "PlaceholderWindow.h"

HRESULT CPlaceholderWindowBase::CopyPlaceholderText(LPCWSTR pszPlaceholderText)
{
	return StringCchCopyW(_szMessage, ARRAYSIZE(_szMessage), pszPlaceholderText);
}

HRESULT CPlaceholderWindowBase::LoadPlaceholderText(HINSTANCE hInstance, UINT uID)
{
	LoadStringW(hInstance, uID, _szMessage, MAX_PATH);
	return S_OK;
}

HRESULT CPlaceholderWindowBase::DrawPlaceholder(HDC hdc, RECT rc)
{
	HFONT hfOld = (HFONT)SelectObject(hdc, _hfMessage);
	int bkOld = SetBkMode(hdc, TRANSPARENT);

	DrawTextW(
		hdc, _szMessage, -1,
		&rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORDBREAK
	);

	SetBkMode(hdc, bkOld);
	SelectObject(hdc, hfOld);
	
	return S_OK;
}

void CPlaceholderWindowBase::SetFont(HFONT hFont)
{
	_hfMessage = hFont;
}

LRESULT CPlaceholderWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			return _OnCreate();
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

			DrawPlaceholder(hdc, rc);

			SelectObject(hdc, hpenOld);
			DeleteObject(hpen);

			EndPaint(hWnd, &ps);
			return 0;
		}
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

LRESULT CPlaceholderWindow::_OnCreate()
{
	return 0;
}

// static
HRESULT CPlaceholderWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	return CWindow::RegisterWindowClass(&wc);
}

// static
CPlaceholderWindow *CPlaceholderWindow::CreateAndShow(HWND hwndParent)
{
	CPlaceholderWindow *pWindow = Create(
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