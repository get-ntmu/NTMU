#pragma once
#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <commctrl.h>
#include <stdio.h>
#include "wil/com.h"
#include "wil/resource.h"
#include "Resource.h"

extern HINSTANCE g_hinst;
extern HWND      g_hwndMain;
extern WCHAR     g_szTempDir[MAX_PATH];
extern DWORD     g_dwOSBuild;

#define MainWndMsgBox(text, type) MessageBoxW(g_hwndMain, text, L"Windows NT Modding Utility", type)

#define NTMU_ERROR(facility, code) (HRESULT)(((code) & 0x0000FFFF) | (facility << 16) | 0x80000000)

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)