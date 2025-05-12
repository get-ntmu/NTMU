#pragma once
#include "NTMU.h"

typedef void (*LogCallback)(void *lpParam, LPCWSTR pszText);

void Log(LPCWSTR pszFormat, ...);
void AddLogCallback(LogCallback pfnCallback, void *lpParam, DWORD *pdwCallbackID);
void RemoveLogCallback(DWORD dwCallbackID);