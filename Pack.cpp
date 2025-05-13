#include "Pack.h"
#include "Logging.h"
#include "TrustedInstaller.h"
#include "Util.h"
#include "EnumPEResources.h"
#include "EnumRESResources.h"
#include <pathcch.h>
#include <shlwapi.h>
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
bool CPack::_ConstructPackFilePath(LPCWSTR pszPath, std::wstring &out)
{
	std::wstring path = pszPath;
	// De-Unix the path.
	size_t pos = 0;
	while ((pos = path.find(L'/', pos)) != std::wstring::npos)
	{
		path[pos] = L'\\';
		pos++;
	}

	if (path.find(L':') != std::wstring::npos // Has drive letter
	|| path[0] == L'\\') // Root or UNC path
	{
		std::wstring message = L"Encountered non-relative path '";
		message += pszPath;
		message += L"'";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}

	if (path.find(L'"') != std::wstring::npos
	|| path.find(L'\'') != std::wstring::npos)
	{
		std::wstring message = L"Encountered path with quotes '";
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

/**
  * Validates a user-defined path for an external file and expands
  * environment strings.
  */
bool CPack::_ConstructExternalFilePath(LPCWSTR pszPath, std::wstring &out)
{
	WCHAR szPath[MAX_PATH];
	if (!ExpandEnvironmentStringsW(pszPath, szPath, MAX_PATH))
	{
		std::wstring message = L"Failed to expand environment strings for path '";
		message += pszPath;
		message += L"'";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}

	if (PathIsRelativeW(szPath))
	{
		std::wstring message = L"Relative path '";
		message += szPath;
		message += L"' for external file encountered";
		MainWndMsgBox(message.c_str(), MB_ICONERROR);
		return false;
	}

	out = szPath;
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

bool CPack::_OptionDefMatches(const std::vector<PackOptionDef> &defs)
{
	for (const auto &def : defs)
	{
		const PackOption *pOpt = _FindOption(def.id.c_str());
		if (!pOpt || pOpt->uValue != def.uValue)
			return false;
	}
	return true;
}

/**
  * Copies a file and if the original exists, renames it to append .old + the number of
  * .old files that already exist.
  * 
  * For example:
  * imageres.dll
  * imageres.dll.old.0
  * imageres.dll.old.1
  * imageres.dll.old.2
  */
// static
bool CPack::_CopyFileWithOldStack(LPCWSTR pszFrom, LPCWSTR pszTo)
{
	// Handle .old files
	DWORD dwFileAttrs = GetFileAttributesW(pszTo);
	if (dwFileAttrs != INVALID_FILE_ATTRIBUTES && !(dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
	{
		for (int i = 0;; i++)
		{
			std::wstring filePath = pszTo;
			filePath += L".old.";
			filePath += std::to_wstring(i);

			dwFileAttrs = GetFileAttributesW(filePath.c_str());
			if (dwFileAttrs == INVALID_FILE_ATTRIBUTES || (dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
			{
				Log(L"Moving file '%s' to '%s'...", pszTo, filePath.c_str());
				if (!MoveFileW(pszTo, filePath.c_str()))
				{
					Log(
						L"Failed to move file '%s' to '%s'. Error: %d",
						pszTo, filePath.c_str(), GetLastError()
					);
					return false;
				}
				break;
			}
		}
	}

	Log(L"Copying file '%s' to '%s'...", pszFrom, pszTo);
	if (!CopyFileW(pszFrom, pszTo, TRUE))
	{
		Log(
			L"Failed to copy file '%s' to '%s'. Error: %d",
			pszFrom, pszTo, GetLastError()
		);
		return false;
	}
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
			if (!previewPath.empty() && !_ConstructPackFilePath(previewPath.c_str(), _previewPath))
				return false;

			std::wstring readmePath = sec.GetPropByName(L"Readme");
			if (!readmePath.empty() && !_ConstructPackFilePath(readmePath.c_str(), _readmePath))
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

			std::wstring trustedInstaller = sec.GetPropByName(L"TrustedInstaller");
			UINT uValue = 0;
			if (!trustedInstaller.empty() && !StringToUInt(trustedInstaller, &uValue)
			|| !_ValidateBoolean(uValue))
				return false;

			if (uValue)
				psec.flags |= PackSectionFlags::TrustedInstaller;

			for (INIValue &val : sec.values)
			{
				const wchar_t *vname = val.name.c_str();
				if (0 != _wcsicmp(vname, L"Requires") && 0 != _wcsicmp(vname, L"TrustedInstaller"))
				{
					// Destination (and as such, value name) doesn't matter for registry items
					PackItem item;
					if (!_ConstructPackFilePath(val.value.c_str(), item.sourceFile))
						return false;
					psec.items.push_back(item);
				}
			}

			_sections.push_back(psec);
		}
		else if (IsSection("Files"))
		{
			PackSection psec = { PackSectionType::Files };
			std::wstring require = sec.GetPropByName(L"Requires");
			if (!require.empty() && !ParseOptionString(require, psec.requires))
				return false;

			WCHAR szLocaleName[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(szLocaleName, LOCALE_NAME_MAX_LENGTH);

			for (INIValue &val : sec.values)
			{
				for (size_t i = 0; i <= val.name.length(); i++)
				{
					if (0 == _wcsnicmp(val.name.c_str(), L"<Locale>", sizeof("<Locale>") - 1))
					{
						val.name.replace(i, sizeof("<Locale>") - 1, szLocaleName);
					}
				}

				const wchar_t *vname = val.name.c_str();
				if (0 != _wcsicmp(vname, L"Requires"))
				{
					PackItem item;
					if (!_ConstructExternalFilePath(val.name.c_str(), item.destFile))
						return false;
					if (!_ConstructPackFilePath(val.value.c_str(), item.sourceFile))
						return false;
					psec.items.push_back(item);
				}
			}

			_sections.push_back(psec);
		}
		else if (IsSection("Resources"))
		{
			PackSection psec = { PackSectionType::Resources };
			std::wstring require = sec.GetPropByName(L"Requires");
			if (!require.empty() && !ParseOptionString(require, psec.requires))
				return false;

			WCHAR szLocaleName[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(szLocaleName, LOCALE_NAME_MAX_LENGTH);

			for (INIValue &val : sec.values)
			{
				const wchar_t *vname = val.name.c_str();
				if (0 != _wcsicmp(vname, L"Requires"))
				{
					for (size_t i = 0; i <= val.name.length(); i++)
					{
						if (0 == _wcsnicmp(val.name.c_str() + i, L"<Locale>", sizeof("<Locale>") - 1))
						{
							val.name.replace(i, sizeof("<Locale>") - 1, szLocaleName);
						}
					}

					PackItem item;
					if (!_ConstructPackFilePath(val.value.c_str(), item.sourceFile))
						return false;

					if (0 == _wcsnicmp(val.name.c_str(), L"<System>\\", sizeof("<System>\\") - 1))
					{
						enum PATHINDEX
						{
							PI_SYSTEM32 = 0,
							PI_SYSWOW64,
							PI_SYSRES,
							PI_COUNT
						};

						std::wstring filePath = val.name.substr(sizeof("<System>\\") - 1);
						WCHAR szPathsToTry[PI_COUNT][MAX_PATH];

						// C:\Windows\System32\XXX
						GetSystemDirectoryW(szPathsToTry[PI_SYSTEM32], MAX_PATH);
						PathCchAppend(szPathsToTry[PI_SYSTEM32], MAX_PATH, filePath.c_str());

						// C:\Windows\SysWOW64\XXX
						GetWindowsDirectoryW(szPathsToTry[PI_SYSWOW64], MAX_PATH);
						PathCchAppend(szPathsToTry[PI_SYSWOW64], MAX_PATH, L"SysWOW64");
						PathCchAppend(szPathsToTry[PI_SYSWOW64], MAX_PATH, filePath.c_str());

						// C:\Windows\SystemResources\XXX.mun
						GetWindowsDirectoryW(szPathsToTry[PI_SYSRES], MAX_PATH);
						PathCchAppend(szPathsToTry[PI_SYSRES], MAX_PATH, L"SystemResources");
						PathCchAppend(szPathsToTry[PI_SYSRES], MAX_PATH, filePath.c_str());
						wcscat_s(szPathsToTry[PI_SYSRES], L".mun");

						DWORD dwFileAttrs = GetFileAttributesW(szPathsToTry[PI_SYSRES]);
						if (dwFileAttrs != INVALID_FILE_ATTRIBUTES && !(dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
						{
							item.destFile = szPathsToTry[PI_SYSRES];
							psec.items.push_back(item);
							// We don't need to copy resources to System32 or SysWOW64 if we have a MUN.
							continue;
						}

						for (int i = PI_SYSTEM32; i <= PI_SYSWOW64; i++)
						{
							dwFileAttrs = GetFileAttributesW(szPathsToTry[i]);
							if (dwFileAttrs != INVALID_FILE_ATTRIBUTES && !(dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
							{
								item.destFile = szPathsToTry[i];
								psec.items.push_back(item);
							}
						}
					}
					else
					{
						if (!_ConstructExternalFilePath(val.name.c_str(), item.destFile))
							return false;
						psec.items.push_back(item);
					}
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

bool CPack::Apply(void *lpParam, PackApplyProgressCallback pfnCallback)
{
	std::vector<PackSection> secs;
	for (const auto &sec : _sections)
	{
		if (_OptionDefMatches(sec.requires))
			secs.push_back(sec);
	}

	if (!ImpersonateTrustedInstaller())
	{
		Log(L"Failed to impersonate TrustedInstaller");
		return false;
	}

	size_t totalItems = secs.size();
	size_t processedItems = 0;
	for (const auto &sec : secs)
	{
		switch (sec.type)
		{
			case PackSectionType::Registry:
			{
				WCHAR szRegeditPath[MAX_PATH];
				GetWindowsDirectoryW(szRegeditPath, MAX_PATH);
				PathCchAppend(szRegeditPath, MAX_PATH, L"regedit.exe");

				auto pfnWaitFunc = (sec.flags & PackSectionFlags::TrustedInstaller) ? WaitForProcessAsTrustedInstaller : WaitForProcess;

				for (const auto &item : sec.items)
				{
					Log(L"Applying registry file '%s'...", item.sourceFile.c_str());

					std::wstring command = L"\"";
					command += szRegeditPath;
					command += L"\" /s \"";
					command += item.sourceFile;
					command += L"\"";

					DWORD dwExitCode;
					if (!pfnWaitFunc(command.c_str(), &dwExitCode))
					{
						Log(L"Failed to create regedit.exe process. Error: %u", GetLastError());
						return false;
					}
					
					if (dwExitCode != 0)
					{
						Log(L"regedit.exe exited with error code %u.", dwExitCode);
						return false;
					}

					processedItems++;
					pfnCallback(lpParam, processedItems, totalItems);
				}
				break;
			}
			case PackSectionType::Files:
			{
				for (const auto &item : sec.items)
				{
					if (!_CopyFileWithOldStack(item.sourceFile.c_str(), item.destFile.c_str()))
						return false;

					processedItems++;
					pfnCallback(lpParam, processedItems, totalItems);
				}
				break;
			}
			case PackSectionType::Resources:
			{
				for (const auto &item : sec.items)
				{
					WCHAR szTempFile[MAX_PATH];
					wcscpy_s(szTempFile, g_szTempDir);
					PathCchAppend(szTempFile, MAX_PATH, PathFindFileNameW(item.destFile.c_str()));

					if (!CopyFileW(item.destFile.c_str(), szTempFile, FALSE))
					{
						Log(L"Failed to copy file '%s' to '%s'", item.destFile.c_str(), szTempFile);
						return false;
					}

					IEnumResources *pEnum = nullptr;
					//HANDLE hUpdateRes = NULL;
					bool fSucceeded = false;
					HRESULT hr = CEnumRESResources_CreateInstance(item.sourceFile.c_str(), &pEnum);
					if (FAILED(hr))
						hr = CEnumPEResources_CreateInstance(item.sourceFile.c_str(), &pEnum);
					if (FAILED(hr))
					{
						Log(L"Failed to enumerate resources of file '%s'. Error: 0x%X",
							item.sourceFile.c_str(), hr);
						goto cleanup;
					}

					//hUpdateRes = BeginUpdateResourceW(szTempFile, FALSE);
					//__debugbreak();
					if (FAILED(pEnum->Enum([](LPVOID lpParam, LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID pvData, DWORD cbData) -> BOOL
					{
						return TRUE;
					}, nullptr)))
					{
						Log(L"Failed to enum resources in file '%s'", item.sourceFile.c_str());
						goto cleanup;
					}

					fSucceeded = true;
cleanup:
					//if (hUpdateRes) EndUpdateResourceW(hUpdateRes, !fSucceeded);
					if (pEnum) pEnum->Destroy();

					if (fSucceeded && !_CopyFileWithOldStack(szTempFile, item.destFile.c_str()))
						fSucceeded = false;

					DeleteFileW(szTempFile);
					if (!fSucceeded) return false;
				}
				break;
			}
		}
	}

	RevertToSelf();

	Log(L"All done!");

	return true;
}