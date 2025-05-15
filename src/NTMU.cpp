#include "NTMU.h"
#include "MainWindow.h"

HINSTANCE g_hinst    = NULL;
HWND      g_hwndMain = NULL;
WCHAR     g_szTempDir[MAX_PATH] = { 0 };
DWORD     g_dwOSBuild = 0;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

EXTERN_C NTSYSAPI NTSTATUS RtlGetVersion(PRTL_OSVERSIONINFOW lpVersionInformation);

bool EnablePrivilege(LPCWSTR lpPrivilegeName)
{
	wil::unique_handle hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
		return false;

	LUID luid;
	if (!LookupPrivilegeValueW(nullptr, lpPrivilegeName, &luid))
		return false;

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	BOOL fSucceeded = AdjustTokenPrivileges(hToken.get(), FALSE, &tp, sizeof(tp), nullptr, nullptr);
	return fSucceeded;
}

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

	RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
	if (!NT_SUCCESS(RtlGetVersion(&osvi)))
	{
		MainWndMsgBox(
			L"Failed to obtain OS version information.",
			MB_ICONERROR
		);
		return -1;
	}

	g_dwOSBuild = osvi.dwBuildNumber;

	if (!EnablePrivilege(SE_DEBUG_NAME) || !EnablePrivilege(SE_IMPERSONATE_NAME))
	{
		MainWndMsgBox(
			L"Failed to acquire the necessary privileges.",
			MB_ICONERROR
		);
		return -1;
	}

	ExpandEnvironmentStringsW(L"%TEMP%\\NTMU\\", g_szTempDir, MAX_PATH);
	DWORD dwAttr = GetFileAttributesW(g_szTempDir);
	if (dwAttr == INVALID_FILE_ATTRIBUTES && !CreateDirectoryW(g_szTempDir, nullptr))
	{
		MainWndMsgBox(
			L"Failed to create the temporary directory.",
			MB_ICONERROR
		);
		return -1;
	}

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