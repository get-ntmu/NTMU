#pragma once
#include "NTMU.h"
#include <string>

void trim(std::wstring &s);

bool WaitForProcess(LPCWSTR pszCommandLine, DWORD *lpdwExitCode);