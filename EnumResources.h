#pragma once
#include "NTMU.h"

typedef BOOL (CALLBACK *ENUMRESPROC)(LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID pvData, DWORD cbData);

interface IEnumResources
{
	STDMETHOD(Enum)(ENUMRESPROC lpEnumFunc) PURE;
	STDMETHOD_(void, Destroy)() PURE;
};