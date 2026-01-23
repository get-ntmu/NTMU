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

	enum class PackSectionFlags
	{
		TrustedInstaller = 1 << 0,
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
		Registry,
	};

	struct PackItem
	{
		std::wstring sourceFile;
		std::wstring destFile;
	};

	struct PackSection
	{
		PackSectionType type;
		PackSectionFlags flags;
		UINT uMinBuild;
		UINT uMaxBuild;
		std::vector<PackOptionDef> requires;
		std::vector<PackItem> items;
	};

	std::vector<PackSection> _sections;

	PackOption *_FindOption(LPCWSTR pszID);
	bool _ConstructPackFilePath(LPCWSTR pszPath, std::wstring &out);
	bool _ConstructExternalFilePath(LPCWSTR pszPath, std::wstring &out);
	static bool _ValidateOptionValue(PackOption &opt, UINT uValue);
	bool _OptionDefMatches(const std::vector<PackOptionDef> &defs);
	bool _ParseMinAndMaxBuilds(const INISection &sec, PackSection &psec);

	inline static bool _ValidateBoolean(UINT uValue)
	{
		if (uValue != 0 && uValue != 1)
		{
			std::wstring message = L"Invalid boolean value '";
			message += std::to_wstring(uValue);
			message += L"'";
			MainWndMsgBox(message.c_str(), MB_ICONERROR);
			return false;
		}
		return true;
	}

	static bool _CopyFileWithOldStack(LPCWSTR pszFrom, LPCWSTR pszTo);
	
	enum class LoadSource
	{
		Default = 0,
		CommandLine = 1,
	};
	
	bool _Load(LPCWSTR pszPath, LoadSource loadSource);
	HRESULT _LoadCommandLineSettings();

public:
	static bool ParseOptionString(const std::wstring &s, std::vector<PackOptionDef> &opts);

	void Reset();
	bool Load(LPCWSTR pszPath) { return _Load(pszPath, LoadSource::Default); }
	bool LoadCommandLineDefault() { return _Load(g_szInitialPack, LoadSource::CommandLine); }

	typedef void (*PackApplyProgressCallback)(void *lpParam, DWORD dwItemsProcessed, DWORD dwTotalItems);

	bool Apply(void *lpParam, PackApplyProgressCallback pfnProgressCalback);

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

inline CPack::PackSectionFlags operator|(CPack::PackSectionFlags a, CPack::PackSectionFlags b)
{
	return (CPack::PackSectionFlags)((int)a | (int)b);
}

inline CPack::PackSectionFlags &operator|=(CPack::PackSectionFlags &a, CPack::PackSectionFlags b)
{
	return a = (a | b);
}

inline bool operator&(CPack::PackSectionFlags a, CPack::PackSectionFlags b)
{
	return (bool)((int)a & (int)b);
}