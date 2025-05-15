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

#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

typedef struct MY_STRING {
    ULONG discriminant;       // long to make the rest of the struct aligned
    union u {
        struct {
            struct MY_STRING *pnext;
            ULONG  ulOffsetToString;
            USHORT cbD;
            USHORT cb;
            WCHAR *sz;
        } ss;
        WORD     Ordinal;
    } uu;
} SDATA, *PSDATA, **PPSDATA;

typedef struct _RESNAME {
    struct _RESNAME *pnext;	// The first three fields should be the
    PSDATA Name;		// same in both res structures
    ULONG   OffsetToData;

    PSDATA	Type;
    ULONG	SectionNumber;
    ULONG	DataSize;
    ULONG_PTR   OffsetToDataEntry;
    USHORT  ResourceNumber;
    USHORT  NumberOfLanguages;
    WORD	LanguageId;
} RESNAME, *PRESNAME, **PPRESNAME;

typedef struct _RESTYPE {
    struct _RESTYPE *pnext;	// The first three fields should be the
    PSDATA Type;		// same in both res structures
    ULONG   OffsetToData;

    struct _RESNAME *NameHeadID;
    struct _RESNAME *NameHeadName;
    ULONG  NumberOfNamesID;
    ULONG  NumberOfNamesName;
} RESTYPE, *PRESTYPE, **PPRESTYPE;

typedef struct _UPDATEDATA {
    ULONG	cbStringTable;
    PSDATA	StringHead;
    PRESNAME	ResHead;
    PRESTYPE	ResTypeHeadID;
    PRESTYPE	ResTypeHeadName;
    LONG	Status;
    HANDLE	hFileName;
} UPDATEDATA, *PUPDATEDATA;