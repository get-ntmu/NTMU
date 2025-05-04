#include "Pack.h"
#include <pathcch.h>

CPack::PackOption *CPack::_FindOption(LPCWSTR pszID)
{
	for (PackOption &opt : _options)
	{
		if (0 == _wcsicmp(pszID, opt.id.c_str()))
			return &opt;
	}
	return nullptr;
}

/**
  * Validates a user-defined source path and constructs into a full path,
  * relative to the pack.
  */
bool CPack::_ValidateAndConstructPath(LPCWSTR pszPath, std::wstring &out)
{
	std::wstring path = pszPath;
	// De-Unix the path.
	size_t pos = 0;
	while ((pos = path.find(L'/', pos)) != std::wstring::npos)
	{
		path[pos] = L'\\';
		pos++;
	}

	if (path.find(':') != std::wstring::npos // Has drive letter
	|| path[0] == L'\\') // Root or UNC path
	{
		std::wstring message = L"Encountered non-relative path '";
		message += pszPath;
		message += L"'";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}

	pos = 0;
	while ((pos = path.find(L"..", pos)) != std::wstring::npos)
	{
		if ((pos == 0 || path[pos - 1] == '\\') || (pos == path.length() - 1 || path[pos + 2] == '\\'))
		{
			std::wstring message = L"Encountered path with upwards directory traversal '";
			message += pszPath;
			message += L"'";
			MainWndMsgBox(message.c_str(), MB_ICONERROR);
			return false;
		}
		pos += 2;
	}

	WCHAR szResult[MAX_PATH];
	wcscpy_s(szResult, _szPackFolder);
	PathCchAppend(szResult, MAX_PATH, path.c_str());

	DWORD dwFileAttrs = GetFileAttributesW(szResult);
	if (dwFileAttrs == INVALID_FILE_ATTRIBUTES)
	{
		std::wstring message = L"File at path '";
		message += szResult;
		message += L"' doesn't exist.";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}

	if (dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY)
	{
		std::wstring message = L"Path '";
		message += szResult;
		message += L"' points to a directory, not a file.";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}

	out = szResult;

	return true;
}

void CPack::Reset()
{
	ZeroMemory(_szPackFolder, sizeof(_szPackFolder));
	_name.clear();
	_author.clear();
	_version.clear();
	_previewPath.clear();
	_options.clear();
	_sections.clear();
}

bool CPack::Load(LPCWSTR pszPath)
{
	Reset();

	INIFile ini;
	if (!ParseINIFile(pszPath, ini))
		return false;

	LPCWSTR pszBackslash = wcsrchr(pszPath, L'\\');
	size_t length = (size_t)(pszBackslash - pszPath);
	wcsncpy_s(_szPackFolder, pszPath, length);

#define IsSection(name) (0 == _wcsicmp(sname, L ## name))

	bool fPackSectionParsed = false;
	for (INISection &sec : ini)
	{
		const wchar_t *sname = sec.name.c_str();
		if (IsSection("Pack") && !fPackSectionParsed)
		{
			_name = sec.GetPropByName(L"Name");
			_author = sec.GetPropByName(L"Author");
			_version = sec.GetPropByName(L"Version");
			
			std::wstring previewPath = sec.GetPropByName(L"Preview");
			if (!previewPath.empty() && !_ValidateAndConstructPath(previewPath.c_str(), _previewPath))
				return false;

			std::wstring readmePath = sec.GetPropByName(L"Readme");
			if (!readmePath.empty() && !_ValidateAndConstructPath(readmePath.c_str(), _readmePath))
				return false;

			fPackSectionParsed = true;
		}
		else
		{
			std::wstring message = L"Unexpected section with name '";
			message += sname;
			message += L"'";
			MainWndMsgBox(message.c_str(), MB_ICONERROR);
			return false;
		}
	}

	if (!fPackSectionParsed)
	{
		MainWndMsgBox(L"No Pack section found", MB_ICONERROR);
	}

	return true;
}