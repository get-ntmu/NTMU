#pragma once
#include "NTMU.h"
#include "EnumPEResources.h"
#include "EnumRESResources.h"
#include <vector>
#include <memory>

struct PEResourceType
{
	bool fIsOrdinal;
	WORD wOrdinal;
	std::wstring string;

	PEResourceType(LPCWSTR lpType)
	{
		fIsOrdinal = IS_INTRESOURCE(lpType);
		if (fIsOrdinal)
			wOrdinal = (WORD)(UINT_PTR)lpType;
		else
			string = lpType;
	}

	bool operator==(const PEResourceType &other) const
	{
		if (fIsOrdinal != other.fIsOrdinal)
			return false;

		if (fIsOrdinal)
			return wOrdinal == other.wOrdinal;
		else
			return string == other.string;
	}

	LPCWSTR AsID() const
	{
		return fIsOrdinal ? MAKEINTRESOURCEW(wOrdinal) : string.c_str();
	}
};

struct PEResource
{
	PEResourceType type;
	PEResourceType name;
	WORD wLangId;
	std::vector<BYTE> data;

	PEResource(LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID pvResource, DWORD cbResource)
		: type(lpType)
		, name(lpName)
		, wLangId(lcid)
	{
		data.resize(cbResource);
		memcpy((LPVOID)data.data(), pvResource, cbResource);
	}
};

inline bool LoadResources(LPCWSTR pszFile, bool fTryRes, std::vector<PEResource> &result)
{
	HRESULT hr;
	NTMU_IEnumResources *pEnum;
	if (!fTryRes || FAILED(hr = CEnumRESResources_CreateInstance(pszFile, &pEnum)))
		hr = CEnumPEResources_CreateInstance(pszFile, &pEnum);
	if (FAILED(hr))
		return false;

	bool fSucceeded = SUCCEEDED(pEnum->Enum(
		[](LPVOID lpParam, LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID pvData, DWORD cbData) -> BOOL
		{
			std::vector<PEResource> *pResult = (std::vector<PEResource> *)lpParam;
			PEResource res(lpType, lpName, lcid, pvData, cbData);
			pResult->push_back(res);
			return TRUE;
		},
		&result
	));
	pEnum->Destroy();
	return fSucceeded;
}

void MergeResources(const std::vector<PEResource> &from, std::vector<PEResource> &to)
{
	for (const auto &res : from)
	{
		// Find and replace the resource if one with the same name, type, and lang exists
		bool fReplaced = false;
		for (auto &toRes : to)
		{
			if (res.type == toRes.type
			&& res.name == toRes.name
			&& res.wLangId == toRes.wLangId)
			{
				toRes = res;
				fReplaced = true;
				break;
			}
		}
		
		// Otherwise just append it
		if (!fReplaced)
			to.push_back(res);
	}
}