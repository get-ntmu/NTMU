#include "Util.h"

void trim(std::wstring &s)
{
	// trim right
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
		return !std::isspace(ch);
		}).base(), s.end());
	// trim left
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
		return !std::isspace(ch);
		}));
}

HRESULT WaitForProcess(LPCWSTR pszCommandLine, DWORD *lpdwExitCode)
{
	if (!lpdwExitCode)
		RETURN_HR(E_INVALIDARG);

	PROCESS_INFORMATION pi;
	STARTUPINFOW si = { sizeof(si) };
	if (!CreateProcessW(
		nullptr,
		(LPWSTR)pszCommandLine,
		nullptr,
		nullptr,
		FALSE,
		NULL,
		nullptr,
		nullptr,
		&si,
		&pi
	))
		RETURN_LAST_ERROR_MSG("Failed to create process.");

	WaitForSingleObject(pi.hProcess, INFINITE);

	return GetExitCodeProcess(pi.hProcess, lpdwExitCode)
		? S_OK
		: S_FALSE;
}