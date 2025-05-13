#pragma once
#include "NTMU.h"

/* Provides APIs for working as NT AUTHORITY\SYSTEM (TrustedInstaller). */

bool ImpersonateSystem(void);
bool ImpersonateTrustedInstaller(void);
bool CreateProcessAsTrustedInstaller(LPCWSTR pszCommandLine, LPPROCESS_INFORMATION ppi);
bool WaitForProcessAsTrustedInstaller(LPCWSTR pszCommandLine, DWORD *lpdwExitCode);