#pragma once
#include "NTMU.h"

namespace DPIHelpers
{
	UINT GetSystemDPI();
	UINT GetWindowDPI(HWND hwnd);
	BOOL AdjustWindowRectForDPI(LPRECT lprc, DWORD dwStyle, DWORD dwExStyle, BOOL fMenu, UINT dpi);
}