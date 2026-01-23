#pragma once
#include "NTMU.h"
#include <string>

void trim(std::wstring &s);

HRESULT WaitForProcess(LPCWSTR pszCommandLine, DWORD *lpdwExitCode);
void ScreenCenteredRect(int cx, int cy, DWORD dwStyle, DWORD dwExStyle, bool fMenu, LPRECT lprc);