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

HRESULT ImpersonateSystem(void)
{
	DWORD dwWinlogonPid;
	if (!GetProcessIdByName(L"winlogon.exe", &dwWinlogonPid))
		RETURN_LAST_ERROR_MSG("Failed to get Winlogon process ID.");

	wil::unique_handle hWinlogonProcess(OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, dwWinlogonPid));
	if (!hWinlogonProcess.get())
		RETURN_LAST_ERROR_MSG("Failed to get handle to Winlogon process.");

	wil::unique_handle hWinlogonToken;
	if (!OpenProcessToken(
		hWinlogonProcess.get(),
		MAXIMUM_ALLOWED,
		&hWinlogonToken
	))
		RETURN_LAST_ERROR_MSG("Failed to open token for Winlogon process.");

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
		RETURN_LAST_ERROR_MSG("Failed to duplicate the Winlogon token.");

	if (!ImpersonateLoggedOnUser(hDupToken.get()))
		RETURN_LAST_ERROR_MSG("Failed to impersonate the SYSTEM user account.");

	return S_OK;
}

/**
 * Retrieves service configuration information for a given service name.
 * 
 * @param pszServiceName
 *		The name of the service for which to retrieve configuration information.
 * @param ppQueryServiceConfig
 *		Ownership of this memory is transferred to the caller. The caller must free this memory
 *		with CoTaskMemFree once it is done using it.
 */
static HRESULT GetServiceConfig(PCWSTR pszServiceName, QUERY_SERVICE_CONFIG **ppQueryServiceConfig)
{
	if (!ppQueryServiceConfig)
		RETURN_HR(E_INVALIDARG);
	
	*ppQueryServiceConfig = nullptr;

	wil::unique_schandle hSCManager(OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASE, GENERIC_EXECUTE));
	if (!hSCManager.get())
		RETURN_LAST_ERROR_MSG("Failed to open the SC manager.");

	wil::unique_schandle hService(OpenServiceW(hSCManager.get(), pszServiceName, GENERIC_READ));
	if (!hService.get())
		RETURN_LAST_ERROR_MSG("Failed to open the %s service.", pszServiceName);

	DWORD cbNeeded;
	QueryServiceConfig(hService.get(), nullptr, 0, &cbNeeded);
	RETURN_LAST_ERROR_IF(ERROR_INSUFFICIENT_BUFFER != GetLastError());
	
	wil::unique_cotaskmem spConfig(CoTaskMemAlloc(cbNeeded));
	if (!spConfig.is_valid())
		RETURN_HR(E_OUTOFMEMORY);
	
	RETURN_LAST_ERROR_IF(!QueryServiceConfig(
		hService.get(), (QUERY_SERVICE_CONFIG *)spConfig.get(), cbNeeded, &cbNeeded));
	
	*(void **)ppQueryServiceConfig = spConfig.release();
	return S_OK;
}

// This is only used in implementation details, so I'll only define it here.
// The publicly exposed names are translated by wrapper functions.
#define NTMU_IMPERSONATION_E_SERVICE_STOPPED NTMU_ERROR(FACILITY_NTMU_IMPERSONATION, 32767)

/**
 * Ensures that a service which is required for impersonation is up and running.
 * 
 * @param pszServiceName 
 *		The name of the service to attempt to run.
 * @param lpdwPid
 *		A pointer to a value to which the resulting process ID will be assigned.
 * @return
 *		A generic HRESULT code, or one of the NTMU_IMPERSONATION_S_SERVICE_ codes.
 */
static HRESULT EnsureService(PCWSTR pszServiceName, LPDWORD lpdwPid)
{
	if (!pszServiceName || !lpdwPid)
		RETURN_HR(E_INVALIDARG);
	
	*lpdwPid = -1;

	wil::unique_schandle hSCManager(OpenSCManagerW(nullptr, SERVICES_ACTIVE_DATABASE, GENERIC_EXECUTE));
	if (!hSCManager.get())
		RETURN_LAST_ERROR_MSG("Failed to open the SC manager.");

	wil::unique_schandle hService(OpenServiceW(hSCManager.get(), pszServiceName, GENERIC_READ | GENERIC_EXECUTE));
	if (!hService.get())
		RETURN_LAST_ERROR_MSG("Failed to open the %s service.", pszServiceName);

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
					RETURN_HR_MSG(NTMU_IMPERSONATION_E_SERVICE_STOPPED,
						"Failed to start the %s service.", pszServiceName);
				break;
			case SERVICE_START_PENDING:
			case SERVICE_STOP_PENDING:
				Sleep(status.dwWaitHint);
				continue;
			case SERVICE_RUNNING:
				*lpdwPid = status.dwProcessId;
				return S_OK;
		}
	}

	RETURN_HR(E_FAIL);
}

HRESULT EnsureTrustedInstallerService(LPDWORD lpdwPid)
{
	HRESULT hr = EnsureService(L"TrustedInstaller", lpdwPid);
	
	if (NTMU_IMPERSONATION_E_SERVICE_STOPPED == hr)
	{
		// We'll check for a known condition here: the TrustedInstaller service
		// being disabled. If this is the case, then it's impossible to start a process
		// as another user account.
		QUERY_SERVICE_CONFIG *pSvcConfig;
		if (SUCCEEDED(GetServiceConfig(L"TrustedInstaller", &pSvcConfig)))
		{
			auto freeGuard = wil::scope_exit([pSvcConfig] { CoTaskMemFree(pSvcConfig); });
			
			if (SERVICE_DISABLED == pSvcConfig->dwStartType)
			{
				RETURN_HR(NTMU_IMPERSONATION_E_TRUSTEDINSTALLER_SVC_DISABLED);
			}
		}
		
		RETURN_HR(NTMU_IMPERSONATION_E_TRUSTEDINSTALLER_SVC_FAILED);
	}
	
	return hr;
}

HRESULT EnsureSecondaryLogonService(LPDWORD lpdwPid)
{
	HRESULT hr = EnsureService(L"seclogon", lpdwPid);
	
	if (NTMU_IMPERSONATION_E_SERVICE_STOPPED == hr)
	{
		// We'll check for a known condition here: the Secondary Logon (seclogon) service
		// being disabled. If this is the case, then it's impossible to start a process
		// as another user account.
		QUERY_SERVICE_CONFIG *pSvcConfig;
		if (SUCCEEDED(GetServiceConfig(L"seclogon", &pSvcConfig)))
		{
			auto freeGuard = wil::scope_exit([pSvcConfig] { CoTaskMemFree(pSvcConfig); });
			
			if (SERVICE_DISABLED == pSvcConfig->dwStartType)
			{
				RETURN_HR(NTMU_IMPERSONATION_E_SECLOGON_SVC_DISABLED);
			}
		}
		
		RETURN_HR(NTMU_IMPERSONATION_E_SECLOGON_SVC_FAILED);
	}
	
	return hr;
}

HRESULT ObtainTrustedInstallerToken(LPHANDLE phToken)
{
	if (!phToken)
		RETURN_HR(E_INVALIDARG);
	RETURN_IF_FAILED(ImpersonateSystem());

	auto stopImpersonating = wil::scope_exit([&]() noexcept { RevertToSelf(); });
	
	DWORD dwSecLogonPid;
	RETURN_IF_FAILED(EnsureSecondaryLogonService(&dwSecLogonPid));
	
	DWORD dwTIPid;
	RETURN_IF_FAILED(EnsureTrustedInstallerService(&dwTIPid));

	wil::unique_handle hTIProcess(OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, dwTIPid));
	if (!hTIProcess.get())
		RETURN_LAST_ERROR_MSG("Failed to open TrustedInstaller process.");

	wil::unique_handle hTIToken;
	if (!OpenProcessToken(
		hTIProcess.get(),
		MAXIMUM_ALLOWED,
		&hTIToken
	))
		RETURN_LAST_ERROR_MSG("Failed to open the token of the TrustedInstaller process.");

	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES) };
	if (!DuplicateTokenEx(
		hTIToken.get(),
		MAXIMUM_ALLOWED,
		&sa,
		SecurityImpersonation,
		TokenImpersonation,
		phToken
	))
		RETURN_LAST_ERROR_MSG("Failed to duplicate the TrustedInstaller token.");

	return S_OK;
}

HRESULT ImpersonateTrustedInstaller(void)
{
	wil::unique_handle hTIToken;
	RETURN_IF_FAILED(ObtainTrustedInstallerToken(&hTIToken));
	return ImpersonateLoggedOnUser(hTIToken.get());
}

HRESULT CreateProcessAsTrustedInstaller(LPCWSTR pszCommandLine, LPPROCESS_INFORMATION ppi)
{
	wil::unique_handle hTIToken;
	RETURN_IF_FAILED(ObtainTrustedInstallerToken(&hTIToken));

	STARTUPINFOW si = { 0 };
	si.lpDesktop = (LPWSTR)L"Winsta0\\Default";
	ZeroMemory(ppi, sizeof(PROCESS_INFORMATION));
	if (!CreateProcessWithTokenW(
		hTIToken.get(),
		LOGON_WITH_PROFILE,
		nullptr,
		(LPWSTR)pszCommandLine,
		CREATE_UNICODE_ENVIRONMENT,
		nullptr,
		nullptr,
		&si,
		ppi
	))
		RETURN_LAST_ERROR_MSG("Failed to create process with the TrustedInstaller profile.");

	return S_OK;
}

HRESULT WaitForProcessAsTrustedInstaller(LPCWSTR pszCommandLine, DWORD *lpdwExitCode)
{
	if (!lpdwExitCode)
		RETURN_HR(E_INVALIDARG);

	PROCESS_INFORMATION pi;
	RETURN_IF_FAILED(CreateProcessAsTrustedInstaller(pszCommandLine, &pi));

	WaitForSingleObject(pi.hProcess, INFINITE);

	return GetExitCodeProcess(pi.hProcess, lpdwExitCode)
		? S_OK
		: S_FALSE;
}