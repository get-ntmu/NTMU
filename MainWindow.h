#pragma once
#include "Window.h"

static constexpr WCHAR c_szMainWindowClass[] = L"NTMU_MainWindow";

class CMainWindow : public CWindow<CMainWindow, c_szMainWindowClass>
{
private:
	virtual LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

public:
	static HRESULT RegisterWindowClass();
	static CMainWindow *CreateAndShow(int nCmdShow);
};