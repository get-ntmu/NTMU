#pragma once
#include "NTMU.h"
#include "INI.h"

class CPack
{
public:
	struct PackRadioOption
	{
		std::wstring name;
		UINT uValue;
	};

	struct PackOption
	{
		UINT uValue;
		std::wstring id;
		std::wstring name;
		std::vector<PackRadioOption> radios;
	};

	struct PackOptionDef
	{
		std::wstring id;
		UINT uValue;
	};

private:
	WCHAR _szPackFolder[MAX_PATH];

	std::wstring _name;
	std::wstring _author;
	std::wstring _version;
	std::wstring _previewPath;
	std::wstring _readmePath;
	std::vector<PackOption> _options;

	enum class PackSectionType
	{
		Resources = 0,
		Files,
		Registry
	};

	struct PackItem
	{
		std::wstring sourceFile;
		std::wstring destFile;
	};

	struct PackSection
	{
		PackSectionType type;
		std::vector<PackOptionDef> requires;
		std::vector<PackItem> items;
	};

	std::vector<PackSection> _sections;

	PackOption *_FindOption(LPCWSTR pszID);
	bool _ValidateAndConstructPath(LPCWSTR pszPath, std::wstring &out);
	static bool _ValidateOptionValue(PackOption &opt, UINT uValue);

public:
	static bool ParseOptionString(const std::wstring &s, std::vector<PackOptionDef> &opts);

	void Reset();
	bool Load(LPCWSTR pszPath);
	bool Apply();

	std::wstring GetName()
	{
		return _name;
	}

	std::wstring GetAuthor()
	{
		return _author;
	}

	std::wstring GetVersion()
	{
		return _version;
	}

	std::wstring GetPreviewPath()
	{
		return _previewPath;
	}

	std::wstring GetReadmePath()
	{
		return _readmePath;
	}

	const std::vector<PackOption> &GetOptions()
	{
		return _options;
	}
};

