#include "NTMU.h"
#include "MainWindow.h"

HINSTANCE g_hinst;

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
	
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}