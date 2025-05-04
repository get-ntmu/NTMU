#pragma once
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "wil/com.h"
#include "wil/resource.h"
#include "Resource.h"

extern HINSTANCE g_hinst;
extern HWND      g_hwndMain;

#define MainWndMsgBox(text, type) MessageBoxW(g_hwndMain, text, L"Windows NT Modding Utility", type)

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)