#include "EnumRESResources.h"

LPCWSTR CEnumRESResources::_ReadTypeOrName()
{
	WORD wMagic;
	if (!_Peek(&wMagic))
		return nullptr;

	if (wMagic == 0xFFFF)
	{
		WORD wResID;
		if (_Read(&wResID) && _Read(&wResID))
			return MAKEINTRESOURCEW(wResID);
	}
	else
	{
		LPCWSTR lpString = (LPCWSTR)(_pbData + _cbCursor);
		_cbCursor += sizeof(WCHAR) * (wcslen(lpString) + 1);
		return lpString;
	}

	return nullptr;
}

void CEnumRESResources::_ReadDwordAlignment()
{
	DWORD dwMod = (_cbCursor & 3);
	if (dwMod)
		_cbCursor += 4 - dwMod;
}

STDMETHODIMP CEnumRESResources::Enum(ENUMRESPROC lpEnumFunc, LPVOID lpParam)
{
	while (true)
	{
		DWORD cbData, cbHeader;
		LPCWSTR lpType, lpName;

		if (!_Read(&cbData) || !_Read(&cbHeader))
			return E_FAIL;

		lpType = _ReadTypeOrName();
		lpName = _ReadTypeOrName();
		if (!lpType || !lpName)
			return E_FAIL;

		_ReadDwordAlignment();

		// Fields we don't care about.
		_cbCursor += sizeof(DWORD) + sizeof(WORD);

		WORD wLangID;
		if (!_Read(&wLangID))
			return E_FAIL;

		// More fields we don't GAF about.
		_cbCursor += sizeof(DWORD) * 2;
		_ReadDwordAlignment();

		LPVOID lpData = _pbData + _cbCursor;
		_cbCursor;
		if (!lpEnumFunc(lpParam, lpType, lpName, wLangID, lpData, cbData))
			break;

		_cbCursor += cbData;
		_ReadDwordAlignment();
		if (_cbCursor >= _cbData)
			break;
	}

	return S_OK;
}

STDMETHODIMP_(void) CEnumRESResources::Destroy()
{
	delete this;
}

CEnumRESResources::CEnumRESResources()
	: _pbData(nullptr)
	, _cbData(0)
	, _cbCursor(0)
{

}

CEnumRESResources::~CEnumRESResources()
{
	if (_pbData)
		delete[] _pbData;
}

HRESULT CEnumRESResources::Initialize(LPCWSTR lpFileName)
{
	wil::unique_handle hFile(CreateFileW(lpFileName, GENERIC_READ, NULL, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
	if (hFile.get() == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwFileSize = GetFileSize(hFile.get(), nullptr);
	_pbData = new BYTE[dwFileSize];
	_cbData = dwFileSize;

	if (!ReadFile(hFile.get(), _pbData, dwFileSize, nullptr, nullptr))
		return HRESULT_FROM_WIN32(GetLastError());

	const BYTE c_resHeader[] = {
		0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	BYTE header[ARRAYSIZE(c_resHeader)];
	if (!_Read(header, ARRAYSIZE(c_resHeader)))
		return E_FAIL;

	if (0 != memcmp(c_resHeader, header, ARRAYSIZE(c_resHeader)))
		return E_FAIL;

	return S_OK;
}

HRESULT CEnumRESResources_CreateInstance(LPCWSTR lpFileName, NTMU_IEnumResources **ppObj)
{
	if (!lpFileName || !ppObj)
		return E_INVALIDARG;

	*ppObj = nullptr;

	CEnumRESResources *pObj = new CEnumRESResources;
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