#include "Logging.h"
#include <unordered_map>

struct LogCallbackStore
{
	LogCallback pfnCallback;
	void *lpParam;
};

std::unordered_map<DWORD, LogCallbackStore> s_logCallbacks;

void Log(LPCWSTR pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);

	WCHAR szBuffer[2048];
	_vsnwprintf_s(szBuffer, 2048, pszFormat, args);

	for (const auto &callback : s_logCallbacks)
	{
		(callback.second.pfnCallback)(callback.second.lpParam, szBuffer);
	}
}

void AddLogCallback(LogCallback pfnCallback, void *lpParam, DWORD *pdwCallbackID)
{
	*pdwCallbackID = s_logCallbacks.size();
	s_logCallbacks[*pdwCallbackID] = {
		pfnCallback,
		lpParam
	};
}

void RemoveLogCallback(DWORD dwCallbackID)
{
	s_logCallbacks.erase(dwCallbackID);
}