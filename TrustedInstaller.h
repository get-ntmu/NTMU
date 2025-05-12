#pragma once
#include "NTMU.h"

bool CreateProcessAsTrustedInstaller(LPCWSTR pszCommandLine, LPPROCESS_INFORMATION ppi);
bool WaitForProcessAsTrustedInstaller(LPCWSTR pszCommandLine, DWORD *lpdwExitCode);