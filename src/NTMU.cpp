#include "NTMU.h"

#include <pathcch.h>
#include <shlwapi.h>

#include "MainWindow.h"
#include "UnattendWindow.h"

HINSTANCE g_hinst    = NULL;
HWND      g_hwndMain = NULL;
WCHAR     g_szTempDir[MAX_PATH] = { 0 };
DWORD     g_dwOSBuild = 0;
bool      g_fUnattend = false;
WCHAR     g_szInitialPack[MAX_PATH] = { 0 };

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

// If we're in a debug version of NTMU, then it's useful to forward detailed error
// logs from WIL so developers more easily trace failures.
#ifdef _DEBUG
#include "Logging.h"
void CALLBACK WilLogCallback(wil::FailureInfo const &failure) noexcept
{
	wchar_t message[2048];
	if (SUCCEEDED(wil::GetFailureLogString(message, ARRAYSIZE(message), failure)))
	{
		Log(L"%s", message);
	}
}
#endif

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

#ifdef _DEBUG
	wil::SetResultLoggingCallback(WilLogCallback);
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
	
	// Parse command line arguments. Note that CommandLineToArgv has weird behavior
	// when no parameter is specified, where it will return a valid with the path
	// to the process's executable. This is the only case this is returned, and it
	// makes it unreliable, so we only run it under the condition that any command
	// line arguments are specified in the first place, hence the null check for
	// the first character of lpCmdLine.
	int nArgs = 0;
	WCHAR **ppszArgs = nullptr;
	if (L'\0' != lpCmdLine[0])
	{
		ppszArgs = CommandLineToArgvW(lpCmdLine, &nArgs);
	}
	auto freeArgs = wil::scope_exit([ppszArgs]
	{
		if (ppszArgs)
			LocalFree(ppszArgs);
	});
	
	for (int i = 0; i < nArgs; i++)
	{
		bool fOptionHandled = false;
		static constexpr int c_optionLength = sizeof("/option") - 1;
		
		auto SetInitialPack = [](const WCHAR *pszFrom) -> bool
		{
			WCHAR szBuffer[MAX_PATH];
			wcscpy_s(szBuffer, ARRAYSIZE(szBuffer), pszFrom);
			ExpandEnvironmentStringsW(szBuffer, g_szInitialPack, ARRAYSIZE(g_szInitialPack));
			
			const WCHAR *pszFileName;
			if ((pszFileName = PathFindFileNameW(g_szInitialPack)) == g_szInitialPack
				|| 0 != _wcsicmp(pszFileName, L"pack.ini"))
			{
				// In this case, we didn't specify a pack.ini path. In case the user specified
				// or dragged the parent folder, we'll check for a direct child.
				if (SUCCEEDED(PathCchAppend(g_szInitialPack, ARRAYSIZE(g_szInitialPack), L"pack.ini")))
				{
					if (!PathFileExistsW(g_szInitialPack))
					{
						// If this is the case, then we have a bad path.
						return false;
					}	
				}
			}
			
			return true;
		};
		
		if (0 == _wcsicmp(ppszArgs[i], L"/unattend"))
		{
			g_fUnattend = true;
			fOptionHandled = true;
		}
		else if (0 == _wcsicmp(ppszArgs[i], L"/pack"))
		{
			if (nArgs <= i + 1)
			{
				MainWndMsgBox(L"Not enough arguments for pack.", MB_ICONERROR);
				continue;
			}
			
			// If a bad initial path was specified here, then the GUI will show its own
			// error message, so we don't say anything here.
			SetInitialPack(ppszArgs[i + 1]);
			
			fOptionHandled = true;
			i++;
			continue;
		}
		else if (lstrlenW(ppszArgs[i]) >= c_optionLength
			&& 0 == _wcsnicmp(ppszArgs[i], L"/option", c_optionLength))
		{
			// These arguments are actually parsed by the pack loader as they are context
			// specific, but they are skipped here to avoid showing the invalid argument
			// message box.
			fOptionHandled = true;
		}
		// The following should remain as the last case, as it handles the fallback for
		// a direct path argument being provided without a prefix. This is conventional
		// for the Windows shell and enables drag/drop.
		else if (0 == i)
		{
			if (SetInitialPack(ppszArgs[i]))
			{
				fOptionHandled = true;
			}
			else
			{
				ZeroMemory(g_szInitialPack, sizeof(g_szInitialPack));
			}
		}
		
		if (!fOptionHandled)
		{
			WCHAR szBuffer[MAX_PATH];
			swprintf_s(szBuffer, ARRAYSIZE(szBuffer), L"Invalid argument \"%s\".", ppszArgs[i]);
			MainWndMsgBox(szBuffer, MB_ICONERROR);
		}
	}
	
	CMainWindow::RegisterWindowClass();
	CUnattendWindow::RegisterWindowClass();

	HACCEL hMainAccel = NULL;
	if (!g_fUnattend) // Full GUI mode:
	{
		CMainWindow *pMainWnd = CMainWindow::CreateAndShow(nShowCmd);
		g_hwndMain = pMainWnd->GetHWND();
		hMainAccel = pMainWnd->GetAccel();
	}
	else // Unattended mode:
	{
		CUnattendWindow *pUnattendWnd = CUnattendWindow::CreateAndShow(nShowCmd);
		g_hwndMain = pUnattendWnd->GetHWND();
	}

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