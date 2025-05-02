#pragma once
#include <windows.h>
#include <commctrl.h>
#include "wil/com.h"
#include "Resource.h"

extern HINSTANCE g_hinst;

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)