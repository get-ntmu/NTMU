#pragma once
#include "NTMU.h"

typedef BOOL (CALLBACK *ENUMRESPROC)(LPVOID lpParam, LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID pvData, DWORD cbData);

interface NTMU_IEnumResources
{
	STDMETHOD(Enum)(ENUMRESPROC lpEnumFunc, LPVOID lpParam) PURE;
	STDMETHOD_(void, Destroy)() PURE;
};