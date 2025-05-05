#include "NTMU.h"
#include "MainWindow.h"

HINSTANCE g_hinst    = NULL;
HWND      g_hwndMain = NULL;

int WINAPI wWinMain(
	_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPWSTR    lpCmdLine,
	_In_     int       nShowCmd
)
{
#ifndef _WIN64
	BOOL fIsWow64;
	if (IsWow64Process(GetCurrentProcess(), &fIsWow64) && fIsWow64)
	{
		MainWndMsgBox(
			L"You are running the 32-bit version of NTMU on a "
			L"64-bit OS. Please download the 64-bit version.",
			MB_ICONERROR
		);
		return -1;
	}
#endif

	g_hinst = hInstance;
	
	CMainWindow::RegisterWindowClass();
	CMainWindow *pMainWnd = CMainWindow::CreateAndShow(nShowCmd);

	g_hwndMain = pMainWnd->GetHWND();
	HACCEL hMainAccel = pMainWnd->GetAccel();
	
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		if (!TranslateAcceleratorW(g_hwndMain, hMainAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	return 0;
}