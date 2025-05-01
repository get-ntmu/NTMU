#pragma once
#include "NTMU.h"
#include "EnumResources.h"

/**
  * Enumerates resources in a PE file using the built-in Windows API.
  * This is used to extract from DLL/EXE/MUI/etc. files.
  */
class CEnumPEResources : public IEnumResources
{
private:
	HMODULE _hMod;

public:
	// == Begin IEnumResources interface ==
	STDMETHODIMP Enum(ENUMRESPROC lpEnumFunc) override;
	STDMETHODIMP_(void) Destroy() override;
	// == End IEnumResources interface ==

	CEnumPEResources();
	~CEnumPEResources();

	HRESULT Initialize(LPCWSTR lpFileName);
};

HRESULT CEnumPEResources_CreateInstance(LPCWSTR lpFileName, IEnumResources **ppObj);