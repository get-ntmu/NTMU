#include "TrustedInstaller.h"
#include <tlhelp32.h>

bool GetProcessIdByName(LPCWSTR lpProcessName, LPDWORD lpdwPid)
{
	*lpdwPid = -1;

	wil::unique_handle hSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL));
	if (hSnapshot.get() == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
	if (Process32FirstW(hSnapshot.get(), &pe))
	{
		do
		{
			if (0 == wcscmp(pe.szExeFile, lpProcessName))
			{
				*lpdwPid = pe.th32ProcessID;
				break;
			}
		} while (Process32NextW(hSnapshot.get(), &pe));
	}

	return (*lpdwPid != -1);
}

bool ImpersonateSystem(void)
{
	DWORD dwWinlogonPid;
	if (!GetProcessIdByName(L"winlogon.exe", &dwWinlogonPid))
		return false;

	wil::unique_handle hWinlogonProcess(OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, dwWinlogonPid));
	if (!hWinlogonProcess.get())
		return false;

	wil::unique_handle hWinlogonToken;
	if (!OpenProcessToken(
		hWinlogonProcess.get(),
		MAXIMUM_ALLOWED,
		&hWinlogonToken
	))
		return false;

	wil::unique_handle hDupToken;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
	if (!DuplicateTokenEx(
		hWinlogonToken.get(),
		MAXIMUM_ALLOWED,
		&sa,
		SecurityImpersonation,
		TokenImpersonation,
		&hDupToken
	))
		return false;

	if (!ImpersonateLoggedOnUser(hDupToken.get()))
		return false;

	return true;
}

bool StartTrustedInstallerService(LPDWORD lpdwPid)
{
	*lpdwPid = -1;

	wil::unique_schandle hSCManager(OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASE, GENERIC_EXECUTE));
	if (!hSCManager.get())
		return false;

	wil::unique_schandle hService(OpenServiceW(hSCManager.get(), L"TrustedInstaller", GENERIC_READ | GENERIC_EXECUTE));
	if (!hService.get())
		return false;

	SERVICE_STATUS_PROCESS status;
	DWORD cbNeeded;
	while (QueryServiceStatusEx(
		hService.get(),
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&status,
		sizeof(status),
		&cbNeeded
	))
	{
		switch (status.dwCurrentState)
		{
			case SERVICE_STOPPED:
				if (!StartServiceW(hService.get(), 0, nullptr))
					return false;
				break;
			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
				Sleep(status.dwWaitHint);
				continue;
			case SERVICE_RUNNING:
				*lpdwPid = status.dwProcessId;
				return true;
		}
	}

	return false;
}

bool CreateProcessAsTrustedInstaller(LPCWSTR pszCommandLine, LPPROCESS_INFORMATION ppi)
{
	static DWORD dwTIPid = -1;
	if (dwTIPid == -1 && !StartTrustedInstallerService(&dwTIPid))
		return false;

	wil::unique_handle hTIProcess(OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, dwTIPid));
	if (!hTIProcess.get())
		return false;

	wil::unique_handle hTIToken;
	if (!OpenProcessToken(
		hTIProcess.get(),
		MAXIMUM_ALLOWED,
		&hTIToken
	))
		return false;

	wil::unique_handle hDupToken;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
	if (!DuplicateTokenEx(
		hTIToken.get(),
		MAXIMUM_ALLOWED,
		&sa,
		SecurityImpersonation,
		TokenImpersonation,
		&hDupToken
	))
		return false;

	STARTUPINFOW si = { 0 };
	si.lpDesktop = (LPWSTR)L"Winsta0\\Default";
	ZeroMemory(ppi, sizeof(PROCESS_INFORMATION));
	if (!CreateProcessWithTokenW(
		hDupToken.get(),
		LOGON_WITH_PROFILE,
		nullptr,
		(LPWSTR)pszCommandLine,
		CREATE_UNICODE_ENVIRONMENT,
		nullptr,
		nullptr,
		&si,
		ppi
	))
		return false;

	return true;
}

bool WaitForProcessAsTrustedInstaller(LPCWSTR pszCommandLine, DWORD *lpdwExitCode)
{
	if (!lpdwExitCode)
		return false;

	PROCESS_INFORMATION pi;
	if (!CreateProcessAsTrustedInstaller(pszCommandLine, &pi))
		return false;

	WaitForSingleObject(pi.hProcess, INFINITE);

	return GetExitCodeProcess(pi.hProcess, lpdwExitCode);
}