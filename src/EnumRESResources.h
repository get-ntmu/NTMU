#pragma once
#include "NTMU.h"
#include "EnumResources.h"

class CEnumRESResources : public NTMU_IEnumResources
{
private:
	BYTE *_pbData;
	DWORD _cbData;
	DWORD _cbCursor;

	bool _Peek(void *pOut, size_t cbSize)
	{
		if (_cbData - cbSize < _cbCursor)
			return false;

		memcpy(pOut, _pbData + _cbCursor, cbSize);
		return true;
	}

	template <typename T>
	bool _Peek(T *pOut)
	{
		return _Peek(pOut, sizeof(T));
	}

	bool _Read(void *pOut, size_t cbSize)
	{
		if (!_Peek(pOut, cbSize))
			return false;
		_cbCursor += cbSize;
		return true;
	}

	template <typename T>
	bool _Read(T *pOut)
	{
		return _Read(pOut, sizeof(T));
	}

	LPCWSTR _ReadTypeOrName();
	void _ReadDwordAlignment();

public:
	// == Begin IEnumResources interface ==
	STDMETHODIMP Enum(ENUMRESPROC lpEnumFunc, LPVOID lpParam) override;
	STDMETHODIMP_(void) Destroy() override;
	// == End IEnumResources interface ==

	CEnumRESResources();
	~CEnumRESResources();

	HRESULT Initialize(LPCWSTR lpFileName);
};

HRESULT CEnumRESResources_CreateInstance(LPCWSTR lpFileName, NTMU_IEnumResources **ppObj);