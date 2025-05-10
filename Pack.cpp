#include "Pack.h"
#include <pathcch.h>
#include <sstream>

CPack::PackOption *CPack::_FindOption(LPCWSTR pszID)
{
	if (!pszID)
		return nullptr;

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

// static
bool CPack::_ValidateOptionValue(PackOption &opt, UINT uValue)
{
	bool fDefaultIsValid = false;
	if (opt.radios.size() == 0)
	{
		fDefaultIsValid = (uValue == 0 || uValue == 1);
	}
	else
	{
		for (PackRadioOption &ropt : opt.radios)
		{
			if (ropt.uValue == uValue)
			{
				fDefaultIsValid = true;
				break;
			}
		}

	}
	if (!fDefaultIsValid)
	{
		std::wstring message = L"Invalid value ";
		message += std::to_wstring(uValue);
		message += L" for option '";
		message += opt.id;
		message += L"'";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}
	return true;
}

inline bool StringToUInt(const std::wstring &s, UINT *i)
{
	// Convert to int
	int temp;
	try
	{
		temp = std::stoi(s);
	}
	catch (const std::invalid_argument &)
	{
		std::wstring message = L"'";
		message += s;
		message += L"' is not an integer";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}
	// Disallow negative numbers
	if (temp < 0)
	{
		std::wstring message = L"Negative integer value '";
		message += std::to_wstring(temp);
		message += L"'";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}
	// Check for garbage characters
	std::wstring temp2 = std::to_wstring(temp);
	if (s.length() != temp2.length())
	{
		std::wstring message = L"Garbage characters after number string '";
		message += s;
		message += L"'";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}
	*i = temp;
	return true;
}

// static
bool CPack::ParseOptionString(const std::wstring &s, std::vector<PackOptionDef> &opts)
{
	std::wstringstream ss(s);
	std::wstring opt;
	opts.clear();
	while (std::getline(ss, opt, L','))
	{
		size_t index = opt.find(L'=');
		if (index == std::wstring::npos)
		{
			std::wstring message = L"Missing '=' in option string '";
			message += opt;
			message += L"'";
			MainWndMsgBox(message.c_str(), MB_ICONERROR);
			return false;
		}

		std::wstring name = opt.substr(0, index);
		std::wstring sval = opt.substr(index + 1);
		UINT uValue;
		if (!StringToUInt(sval, &uValue))
			return false;

		PackOptionDef optdef = { name, uValue };
		opts.push_back(optdef);
	}
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
		else if (0 == _wcsnicmp(sname, L"Options.", sizeof("Options")))
		{
			LPCWSTR lpOptionName = sname + sizeof("Options");
			if (_FindOption(lpOptionName))
			{
				std::wstring message = L"Duplicate option '";
				message += lpOptionName;
				message += L"'";
				MainWndMsgBox(message.c_str(), MB_ICONERROR);
				return false;
			}
			
			PackOption opt;
			opt.id = lpOptionName;

			UINT uDefaultValue = 0;
			std::wstring defaultValueStr = sec.GetPropByName(L"DefaultValue");
			if (!defaultValueStr.empty() && !StringToUInt(defaultValueStr, &uDefaultValue))
				return false;
			opt.uValue = uDefaultValue;

			std::wstring optionName = sec.GetPropByName(L"Name");
			opt.name = (optionName.empty()) ? lpOptionName : optionName;
			
			for (INIValue &val : sec.values)
			{
				const wchar_t *vname = val.name.c_str();
				if (0 != _wcsicmp(vname, L"Name") && 0 != _wcsicmp(vname, L"DefaultValue"))
				{
					PackRadioOption ropt;
					if (!StringToUInt(val.name, &ropt.uValue))
						return false;

					ropt.name = (val.value.empty()) ? val.name : val.value;
					opt.radios.push_back(ropt);
				}
			}
			
			// Make sure default value is valid
			if (!_ValidateOptionValue(opt, opt.uValue))
				return false;

			_options.push_back(opt);
		}
		else if (IsSection("Registry"))
		{
			PackSection psec = { PackSectionType::Registry };
			std::wstring require = sec.GetPropByName(L"Requires");
			if (!require.empty() && !ParseOptionString(require, psec.requires))
				return false;

			for (INIValue &val : sec.values)
			{
				const wchar_t *vname = val.name.c_str();
				if (0 != _wcsicmp(vname, L"Requires"))
				{
					// Destination (and as such, value name) doesn't matter for registry items
					PackItem item;
					if (!_ValidateAndConstructPath(val.value.c_str(), item.sourceFile))
						return false;
					psec.items.push_back(item);
				}
			}

			_sections.push_back(psec);
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