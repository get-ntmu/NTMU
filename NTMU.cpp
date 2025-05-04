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