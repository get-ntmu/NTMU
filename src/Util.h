#pragma once
#include "NTMU.h"
#include <string>

void trim(std::wstring &s);

HRESULT WaitForProcess(LPCWSTR pszCommandLine, DWORD *lpdwExitCode);