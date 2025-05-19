#pragma once
#include "NTMU.h"
#include "EnumPEResources.h"
#include "EnumRESResources.h"
#include <vector>
#include <memory>

// the common header of RT_CURSOR, RT_ICON, icon/cursor files
typedef struct ICONDIR
{
	WORD           idReserved;  // Reserved (must be 0)
	WORD           idType;      // RES_ICON or RES_CURSOR
	WORD           idCount;     // How many images?
} ICONDIR, *LPICONDIR;

// the entry of group icon resource (RT_GROUP_ICON)
#include <pshpack2.h>
typedef struct GRPICONDIRENTRY
{
	BYTE   bWidth;              // Width, in pixels, of the image
	BYTE   bHeight;             // Height, in pixels, of the image
	BYTE   bColorCount;         // Number of colors in image (0 if >=8bpp)
	BYTE   bReserved;           // Reserved
	WORD   wPlanes;             // Color Planes
	WORD   wBitCount;           // Bits per pixel
	DWORD  dwBytesInRes;        // how many bytes in this resource?
	WORD   nID;                 // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#include <poppack.h>

// the entry of group cursor resource (RT_GROUP_CURSOR)
#include <pshpack2.h>
typedef struct GRPCURSORDIRENTRY
{
	WORD    wWidth;             // Width, in pixels, of the image
	WORD    wHeight;            // Height, in pixels, of the image
	WORD    wPlanes;            // Must be 1
	WORD    wBitCount;          // Bits per pixel
	DWORD   dwBytesInRes;       // how many bytes in this resource?
	WORD    nID;                // the ID
} GRPCURSORDIRENTRY, *LPGRPCURSORDIRENTRY;
#include <poppack.h>

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

inline void DeleteResource(std::vector<PEResource> &resources, const PEResourceType &type, const PEResourceType &name)
{
	for (auto &res : resources)
	{
		if (res.type == type && res.name == name)
		{
			auto it = std::find_if(resources.begin(), resources.end(), [&](PEResource &item)
			{
				return &item == &res;
			});
			resources.erase(it);
			break;
		}
	}
}

inline PEResource *_FindResource(std::vector<PEResource> &resources, const PEResourceType &type, const PEResourceType &name)
{
	for (auto &res : resources)
	{
		if (res.type == type && res.name == name)
		{
			return &res;
		}
	}
	return nullptr;
}

inline WORD LowestAvailableID(std::vector<PEResource> &resources, const PEResourceType &type)
{
	for (WORD i = 1;; i++)
	{
		PEResourceType name(MAKEINTRESOURCEW(i));
		if (!_FindResource(resources, type, name))
			return i;
	}
}

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
	delete pEnum;
	return fSucceeded;
}

void MergeResources(std::vector<PEResource> &from, std::vector<PEResource> &to)
{
	// HACKHACKHACK: We need to mess with icon resources as to not fuck shit over.
	// Move them all to the end so we process them last.
	// endless loop i fucking HATE C++
	for (auto &res : from)
	{
		if (res.type.fIsOrdinal
		&& (res.type.wOrdinal == (WORD)RT_ICON || res.type.wOrdinal == (WORD)RT_CURSOR))
		{
			auto it = std::find_if(from.begin(), from.end(), [&](PEResource &item)
			{
				return &item == &res;
			});
			std::rotate(it, it + 1, from.end());
		}
	}

	for (const auto &res : from)
	{
		// Mess with icons
		if (res.type.fIsOrdinal
		&& (res.type.wOrdinal == (WORD)RT_GROUP_ICON || res.type.wOrdinal == (WORD)RT_GROUP_CURSOR))
		{
			bool fCursor = res.type.wOrdinal == (WORD)RT_GROUP_CURSOR;

			// If the dest file has this icon group, delete all the corresponding icons
			PEResource *pOrigResource = _FindResource(to, res.type, res.name);
			if (pOrigResource)
			{
				ICONDIR *pIconDir = (ICONDIR *)&pOrigResource->data[0];
				void *pData = (void *)(pIconDir + 1);
				for (int i = 0; i < pIconDir->idCount; i++)
				{
					WORD nID;
					if (fCursor)
						nID = ((GRPCURSORDIRENTRY *)pData)[i].nID;
					else
						nID = ((GRPICONDIRENTRY *)pData)[i].nID;

					PEResourceType type(fCursor ? RT_CURSOR : RT_ICON);
					PEResourceType name(MAKEINTRESOURCEW(nID));
					DeleteResource(to, type, name);
				}
			}

			// Modify our own icons to make sure IDs don't conflict
			ICONDIR *pIconDir = (ICONDIR *)&res.data[0];
			void *pData = (void *)(pIconDir + 1);
			for (int i = 0; i < pIconDir->idCount; i++)
			{
				WORD *pID;
				if (fCursor)
					pID = &((GRPCURSORDIRENTRY *)pData)[i].nID;
				else
					pID = &((GRPICONDIRENTRY *)pData)[i].nID;

				PEResourceType type(fCursor ? RT_CURSOR : RT_ICON);
				PEResourceType name(MAKEINTRESOURCEW(*pID));
				if (_FindResource(to, type, name))
				{
					WORD newID = LowestAvailableID(to, type);
					PEResource *pIconRes = _FindResource(from, type, name);
					if (pIconRes)
					{
						PEResourceType newName(MAKEINTRESOURCEW(newID));
						*pID = newID;
						pIconRes->name = newName;
					}
				}
			}
		}

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

	// HACKHACKHACK: MUI resources need to be at the very end of this list or else
	// they will trigger ERROR_NOT_SUPPORTED due to M$'s crappy limitations with
	// UpdateResourceW. Just move all the "MUI" resources to the end.
	for (auto &toRes : to)
	{
		if (!toRes.type.fIsOrdinal && 0 == wcscmp(toRes.type.string.c_str(), L"MUI"))
		{
			auto it = std::find_if(to.begin(), to.end(), [&](PEResource &item)
			{
				return &item == &toRes;
			});
			std::rotate(it, it + 1, to.end());
		}
	}
}