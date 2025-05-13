#include "EnumPEResources.h"

STDMETHODIMP CEnumPEResources::Enum(ENUMRESPROC lpEnumFunc, LPVOID lpParam)
{
	struct ENUMPERESPARAMS
	{
		ENUMRESPROC lpEnumFunc;
		LPVOID lpParam;
	} params = { lpEnumFunc, lpParam };
	EnumResourceTypesExW(
		_hMod, [](HMODULE hMod, LPWSTR lpType, LONG_PTR lParam) -> BOOL
		{
			return EnumResourceNamesExW(
				hMod, lpType, [](HMODULE hMod, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) -> BOOL
				{
					return EnumResourceLanguagesExW(
						hMod, lpType, lpName, [](HMODULE hMod, LPCWSTR lpType, LPCWSTR lpName, WORD wIDLanguage, LONG_PTR lParam) -> BOOL
						{
							HRSRC hrSrc = FindResourceExW(hMod, lpType, lpName, wIDLanguage);
							if (!hrSrc)
								return FALSE;

							DWORD cbSize = SizeofResource(hMod, hrSrc);
							HGLOBAL hGlobal = LoadResource(hMod, hrSrc);
							if (!hGlobal)
								return FALSE;

							LPVOID lpData = LockResource(hGlobal);
							if (!lpData)
								return FALSE;

							ENUMPERESPARAMS *pParams = (ENUMPERESPARAMS *)lParam;
							return pParams->lpEnumFunc(pParams->lpParam, lpType, lpName, wIDLanguage, lpData, cbSize);
						},
						lParam, RESOURCE_ENUM_LN, 0
					);
				},
				lParam, RESOURCE_ENUM_LN, 0
			);
		}, (LONG_PTR)&params, RESOURCE_ENUM_LN, 0
	);
	return HRESULT_FROM_WIN32(GetLastError());
}

STDMETHODIMP_(void) CEnumPEResources::Destroy()
{
	delete this;
}

CEnumPEResources::CEnumPEResources()
	: _hMod(NULL)
{

}

CEnumPEResources::~CEnumPEResources()
{
	if (_hMod)
		FreeLibrary(_hMod);
}

HRESULT CEnumPEResources::Initialize(LPCWSTR lpFileName)
{
	_hMod = LoadLibraryW(lpFileName);
	if (!_hMod)
		return HRESULT_FROM_WIN32(GetLastError());
	return S_OK;
}

HRESULT CEnumPEResources_CreateInstance(LPCWSTR lpFileName, IEnumResources **ppObj)
{
	if (!lpFileName || !ppObj)
		return E_INVALIDARG;

	*ppObj = nullptr;

	CEnumPEResources *pObj = new CEnumPEResources;
	if (!pObj)
		return E_OUTOFMEMORY;

	HRESULT hr = pObj->Initialize(lpFileName);
	if (FAILED(hr))
	{
		delete pObj;
		return hr;
	}

	*ppObj = pObj;
	return S_OK;
}