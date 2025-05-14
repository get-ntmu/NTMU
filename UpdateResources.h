#pragma once
#include "NTMU.h"
#include <winternl.h>

typedef struct _PEB
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN IsLongPathAwareProcess : 1;
		};
	};

	HANDLE Mutant;

	PVOID ImageBaseAddress;
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID SubSystemData;
	PVOID ProcessHeap;

	//
	// ...
	//

} PEB, *PPEB;

#define NtCurrentPeb()            (NtCurrentTeb()->ProcessEnvironmentBlock)
#define RtlProcessHeap()          (NtCurrentPeb()->ProcessHeap)

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
	IN PVOID HeapHandle,
	IN ULONG Flags,
	IN SIZE_T Size
);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
	IN PVOID HeapHandle,
	IN ULONG Flags,
	IN PVOID BaseAddress
);

typedef struct _SDATA
{
	ULONG discriminant; // alignment
	union
	{
		struct
		{
			struct _SDATA *pnext;
			ULONG ulOffsetToString;
			USHORT cbD;
			USHORT cb;
			WCHAR *sz;
		} ss;
		WORD Ordinal;
	};
} SDATA, *PSDATA, **PPSDATA;

#define IS_STRING 1
#define IS_ID     2

typedef struct _RESNAME
{
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

typedef struct _RESTYPE
{
	struct _RESTYPE *pnext;	// The first three fields should be the
	PSDATA Type;		// same in both res structures
	ULONG   OffsetToData;

	struct _RESNAME *NameHeadID;
	struct _RESNAME *NameHeadName;
	ULONG  NumberOfNamesID;
	ULONG  NumberOfNamesName;
} RESTYPE, *PRESTYPE, **PPRESTYPE;

typedef struct _UPDATEDATA
{
	ULONG	cbStringTable;
	PSDATA	StringHead;
	PRESNAME	ResHead;
	PRESTYPE	ResTypeHeadID;
	PRESTYPE	ResTypeHeadName;
	LONG	Status;
	HANDLE	hFileName;
} UPDATEDATA, *PUPDATEDATA;

HANDLE NTMU_BeginUpdateResource(LPCWSTR pszPath);
BOOL NTMU_UpdateResource(HANDLE hUpdate, LPCWSTR lpType, LPCWSTR lpName, WORD language, LPVOID cbData, ULONG cb);
BOOL NTMU_EndUpdateResource(HANDLE hUpdate, BOOL fDiscard);