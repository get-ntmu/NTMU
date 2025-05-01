#pragma once
#include "NTMU.h"

template <typename CImpl, LPCWSTR c_szClassName>
class CWindow
{
private:
	static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CWindow *pThis = (CWindow *)GetWindowLongPtrW(hWnd, 0);
		if (uMsg == WM_CREATE)
		{
			if (!pThis)
			{
				pThis = new CImpl;
				pThis->_hwnd = hWnd;
				SetWindowLongPtrW(hWnd, 0, (LONG_PTR)pThis);
			}
		}
		else if (uMsg == WM_DESTROY)
		{
			if (pThis)
			{
				LRESULT lr = pThis->v_WndProc(hWnd, uMsg, wParam, lParam);
				SetWindowLongPtrW(hWnd, 0, (LONG_PTR)nullptr);
				delete (CImpl *)pThis;
				return lr;
			}
			return 0;
		}

		if (pThis)
			return pThis->v_WndProc(hWnd, uMsg, wParam, lParam);
		else
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

protected:
	HWND _hwnd;

	/*
	 * Call this on children instead of RegisterClassW.
	 */
	static HRESULT RegisterWindowClass(WNDCLASSW *pWndClass)
	{
		pWndClass->lpszClassName = c_szClassName;
		pWndClass->lpfnWndProc = s_WndProc;
		pWndClass->cbWndExtra = sizeof(CImpl *); // Size to store class pointer in the window.
		return RegisterClassW(pWndClass) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	}

	virtual LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

public:
	static CImpl *Create(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle,
		int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, void *pParam)
	{
		HWND hWnd = CreateWindowExW(dwExStyle, c_szClassName, lpWindowName, dwStyle,
			x, y, cx, cy, hWndParent, hMenu, hInstance, pParam);
		return hWnd ? (CImpl *)GetWindowLongPtrW(hWnd, 0) : nullptr;
	}

	inline HWND GetHWND()
	{
		return _hwnd;
	}
};