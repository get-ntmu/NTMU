#pragma once
#include "NTMU.h"
#include "EnumPEResources.h"
#include "EnumRESResources.h"
#include <vector>
#include <memory>

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

// We need this to be separate so we can get the actual size of
// just the header without the other junk
struct ICONDIRHEADER
{
	WORD           idReserved;  // Reserved (must be 0)
	WORD           idType;      // RES_ICON or RES_CURSOR
	WORD           idCount;     // How many images?
};

typedef struct ICONDIR : ICONDIRHEADER
{
	union
	{
		GRPICONDIRENTRY icons[];
		GRPCURSORDIRENTRY cursors[];
	};
} ICONDIR, *LPICONDIR;

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

struct PEIconEntry : GRPICONDIRENTRY
{
	std::vector<BYTE> data;
};

struct PEGroupIcon
{
	PEResourceType name;
	WORD wLangId;
	std::vector<PEIconEntry> entries;

	PEGroupIcon(const PEResourceType &_name, WORD _wLangId)
		: name(_name)
		, wLangId(_wLangId)
	{
	}
};

struct PECursorEntry : GRPCURSORDIRENTRY
{
	std::vector<BYTE> data;
};

struct PEGroupCursor
{
	PEResourceType name;
	WORD wLangId;
	std::vector<PECursorEntry> entries;

	PEGroupCursor(const PEResourceType &_name, WORD _wLangId)
		: name(_name)
		, wLangId(_wLangId)
	{
	}
};

struct PEResources
{
	std::vector<PEResource> resources;
	std::vector<PEGroupIcon> groupIcons;
	std::vector<PEGroupCursor> groupCursors;
};

struct LoadResourcesCtx
{
	PEResources *result;
	std::vector<PEResource> icons, groupIcons, cursors, groupCursors;
};

BOOL CALLBACK LoadResourcesEnumProc(LPVOID lpParam, LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID pvData, DWORD cbData)
{
	LoadResourcesCtx *pCtx = (LoadResourcesCtx *)lpParam;
	PEResource res(lpType, lpName, lcid, pvData, cbData);

	std::vector<PEResource> *dest = &pCtx->result->resources;
	switch ((WORD)lpType)
	{
		case (WORD)RT_ICON:
			dest = &pCtx->icons;
			break;
		case (WORD)RT_GROUP_ICON:
			dest = &pCtx->groupIcons;
			break;
		case (WORD)RT_CURSOR:
			dest = &pCtx->cursors;
			break;
		case (WORD)RT_GROUP_CURSOR:
			dest = &pCtx->groupCursors;
			break;
	}
	dest->push_back(res);
	return TRUE;
}

inline bool LoadResources(LPCWSTR pszFile, bool fTryRes, PEResources &result)
{
	HRESULT hr;
	NTMU_IEnumResources *pEnum;
	if (!fTryRes || FAILED(hr = CEnumRESResources_CreateInstance(pszFile, &pEnum)))
		hr = CEnumPEResources_CreateInstance(pszFile, &pEnum);
	if (FAILED(hr))
		return false;

	LoadResourcesCtx ctx = { 0 };
	ctx.result = &result;

	bool fSucceeded = SUCCEEDED(pEnum->Enum(LoadResourcesEnumProc, &ctx));
	delete pEnum;

	for (const auto &groupIconRes : ctx.groupIcons)
	{
		PEGroupIcon groupIcon(groupIconRes.name, groupIconRes.wLangId);
		ICONDIR *pDir = (ICONDIR *)(groupIconRes.data.data());
		for (int i = 0; i < pDir->idCount; i++)
		{
			PEIconEntry entry;
			memcpy(&entry, &pDir->icons[i], sizeof(GRPICONDIRENTRY));
			
			for (const auto &iconRes : ctx.icons)
			{
				if ((WORD)iconRes.name.AsID() == entry.nID)
				{
					size_t size = iconRes.data.size();
					entry.data.resize(size);
					memcpy(entry.data.data(), iconRes.data.data(), size);
					break;
				}
			}

			if (entry.data.size() > 0)
				groupIcon.entries.push_back(entry);
		}

		result.groupIcons.push_back(groupIcon);
	}

	for (const auto &groupCursorRes : ctx.groupCursors)
	{
		PEGroupCursor groupCursor(groupCursorRes.name, groupCursorRes.wLangId);
		ICONDIR *pDir = (ICONDIR *)(groupCursorRes.data.data());
		for (int i = 0; i < pDir->idCount; i++)
		{
			PECursorEntry entry;
			memcpy(&entry, &pDir->cursors[i], sizeof(GRPCURSORDIRENTRY));

			for (const auto &cursorRes : ctx.cursors)
			{
				if ((WORD)cursorRes.name.AsID() == entry.nID)
				{
					size_t size = cursorRes.data.size();
					entry.data.resize(size);
					memcpy(entry.data.data(), cursorRes.data.data(), size);
					break;
				}
			}

			if (entry.data.size() > 0)
				groupCursor.entries.push_back(entry);
		}

		result.groupCursors.push_back(groupCursor);
	}

	return fSucceeded;
}

void MergeResources(const PEResources &from, PEResources &to)
{
	for (const auto &res : from.resources)
	{
		// Find and replace the resource if one with the same name, type, and lang exists
		bool fReplaced = false;
		for (auto &toRes : to.resources)
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
			to.resources.push_back(res);
	}

	for (const auto &groupIcon : from.groupIcons)
	{
		// Find and replace the group icon if one with the same name and lang exists
		bool fReplaced = false;
		for (auto &toGroupIcon : to.groupIcons)
		{
			if (groupIcon.name == toGroupIcon.name
			&& groupIcon.wLangId == toGroupIcon.wLangId)
			{
				toGroupIcon = groupIcon;
				fReplaced = true;
				break;
			}
		}

		// Otherwise just append it
		if (!fReplaced)
			to.groupIcons.push_back(groupIcon);
	}

	for (const auto &groupCursor : from.groupCursors)
	{
		// Find and replace the group cursor if one with the same name and lang exists
		bool fReplaced = false;
		for (auto &toGroupCursor : to.groupCursors)
		{
			if (groupCursor.name == toGroupCursor.name
			&& groupCursor.wLangId == toGroupCursor.wLangId)
			{
				toGroupCursor = groupCursor;
				fReplaced = true;
				break;
			}
		}

		// Otherwise just append it
		if (!fReplaced)
			to.groupCursors.push_back(groupCursor);
	}

	// HACKHACKHACK: MUI resources need to be at the very end of this list or else
	// they will trigger ERROR_NOT_SUPPORTED due to M$'s crappy limitations with
	// UpdateResourceW. Just move all the "MUI" resources to the end.
	for (auto &toRes : to.resources)
	{
		if (!toRes.type.fIsOrdinal && 0 == wcscmp(toRes.type.string.c_str(), L"MUI"))
		{
			auto it = std::find_if(to.resources.begin(), to.resources.end(), [&](PEResource &item)
			{
				return &item == &toRes;
			});
			std::rotate(it, it + 1, to.resources.end());
		}
	}
}