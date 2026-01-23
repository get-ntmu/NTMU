#include "Pack.h"
#include "Logging.h"
#include "TrustedInstaller.h"
#include "Util.h"
#include "EnumPEResources.h"
#include "EnumRESResources.h"
#include "PEResources.h"
#include <pathcch.h>
#include <shlwapi.h>
#include <shlobj.h>
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

bool CPack::_ParseMinAndMaxBuilds(const INISection &sec, PackSection &psec)
{
	std::wstring minBuild = sec.GetPropByName(L"MinBuild");
	psec.uMinBuild = 0;
	if (!minBuild.empty() && !StringToUInt(minBuild, &psec.uMinBuild))
		return false;

	std::wstring maxBuild = sec.GetPropByName(L"MaxBuild");
	psec.uMaxBuild = 0;
	if (!maxBuild.empty() && !StringToUInt(maxBuild, &psec.uMaxBuild))
		return false;

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
	if (FAILED(ImpersonateTrustedInstaller()))
	{
		Log(L"Failed to impersonate TrustedInstaller");
		return false;
	}

	auto stopImpersonating = wil::scope_exit([&]() noexcept { RevertToSelf(); });

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
	else
	{
		WCHAR szParentDirName[MAX_PATH];
		wcscpy_s(szParentDirName, pszTo);
		WCHAR *pBackslash = wcsrchr(szParentDirName, L'\\');
		if (!pBackslash)
		{
			Log(L"Failed to find backslash in path '%s'", szParentDirName);
		}
		*(pBackslash + 1) = L'\0';
		dwFileAttrs = GetFileAttributesW(szParentDirName);
		if (dwFileAttrs == INVALID_FILE_ATTRIBUTES || !(dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			int result = SHCreateDirectoryExW(NULL, szParentDirName, nullptr);
			if (result != ERROR_SUCCESS)
			{
				Log(L"Failed to create directory '%s'. Error: %d", szParentDirName, result);
				return false;
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
	_readmePath.clear();
	_options.clear();
	_sections.clear();
}

bool CPack::_Load(LPCWSTR pszPath, LoadSource loadSource)
{
	Reset();

	INIFile ini;
	if (!ParseINIFile(pszPath, ini))
		return false;

	LPCWSTR pszBackslash = wcsrchr(pszPath, L'\\');
	size_t length = (size_t)(pszBackslash - pszPath);
	wcsncpy_s(_szPackFolder, pszPath, length);

#define IsSection(name) (0 == _wcsicmp(sname, L ## name))
#define IsValue(name) (0 == _wcsicmp(vname, L ## name))

	bool fPackSectionParsed = false;
	for (INISection &sec : ini)
	{
		const wchar_t *sname = sec.name.c_str();
		if (IsSection("Pack") && !fPackSectionParsed)
		{
			std::wstring minBuild = sec.GetPropByName(L"MinBuild");
			UINT uMinBuild = 0;
			if (!minBuild.empty() && !StringToUInt(minBuild, &uMinBuild))
				return false;

			if (uMinBuild && g_dwOSBuild < uMinBuild)
			{
				std::wstring message = L"This pack was designed for at minimum Windows build ";
				message += std::to_wstring(uMinBuild);
				message += L". You are running build ";
				message += std::to_wstring(g_dwOSBuild);
				message += L".";
				MainWndMsgBox(message.c_str(), MB_ICONERROR);
				return false;
			}

			std::wstring maxBuild = sec.GetPropByName(L"MaxBuild");
			UINT uMaxBuild = 0;
			if (!maxBuild.empty() && !StringToUInt(maxBuild, &uMaxBuild))
				return false;

			if (uMaxBuild && g_dwOSBuild > uMaxBuild)
			{
				std::wstring message = L"This pack was designed for at most Windows build ";
				message += std::to_wstring(uMaxBuild);
				message += L". You are running build ";
				message += std::to_wstring(g_dwOSBuild);
				message += L".";
				MainWndMsgBox(message.c_str(), MB_ICONERROR);
				return false;
			}

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
				if (!IsValue("Name") && !IsValue("DefaultValue"))
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

			if (!_ParseMinAndMaxBuilds(sec, psec))
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
				if (!IsValue("Requires") && !IsValue("TrustedInstaller") && !IsValue("MinBuild") && !IsValue("MaxBuild"))
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

			if (!_ParseMinAndMaxBuilds(sec, psec))
				return false;

			for (INIValue &val : sec.values)
			{
				const wchar_t *vname = val.name.c_str();
				if (!IsValue("Requires") && !IsValue("MinBuild") && !IsValue("MaxBuild"))
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

			if (!_ParseMinAndMaxBuilds(sec, psec))
				return false;

			WCHAR szLocaleName[LOCALE_NAME_MAX_LENGTH];
			GetUserDefaultLocaleName(szLocaleName, LOCALE_NAME_MAX_LENGTH);

			for (INIValue &val : sec.values)
			{
				const wchar_t *vname = val.name.c_str();
				if (!IsValue("Requires") && !IsValue("MinBuild") && !IsValue("MaxBuild"))
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
	
	if (LoadSource::CommandLine == loadSource)
	{
		// If we're loading from the command line, then it's also possible that
		// we have "/option" arguments that we want to parse. We will parse these
		// options now.
		LOG_IF_FAILED(_LoadCommandLineSettings());
	}

	return true;
}

HRESULT CPack::_LoadCommandLineSettings()
{
	WCHAR *pszCommandLine = GetCommandLineW();
	WCHAR szMsgErrorBuffer[1024] = {};
	
	int nArgs = 0;
	WCHAR **ppszArgs = CommandLineToArgvW(pszCommandLine, &nArgs);
	auto freeArgs = wil::scope_exit([ppszArgs] { LocalFree(ppszArgs); });
		
	static constexpr int c_optionLength = sizeof("/option") - 1;
	
	for (int i = 0; i < nArgs; i++)
	{
		if (lstrlenW(ppszArgs[i]) >= c_optionLength
			&& 0 == StrCmpNIW(ppszArgs[i], L"/option", c_optionLength))
		{
			std::wstring arg = ppszArgs[i];
			
			if (std::wstring::npos == arg.find(L":"))
			{
				StringCchPrintfW(szMsgErrorBuffer, ARRAYSIZE(szMsgErrorBuffer), 
					L"Improperly formed option argument \"%s\". It must specify a key and value.",
					ppszArgs[i]);
				MessageBoxW(NULL, szMsgErrorBuffer, 
					L"Error", MB_ICONERROR | MB_OK);
				return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
			}
			
			std::wstring statement = arg.substr(arg.find(L":") + 1);
			if (std::wstring::npos == statement.find(L"="))
			{
				StringCchPrintfW(szMsgErrorBuffer, ARRAYSIZE(szMsgErrorBuffer), 
					L"Improperly formed option argument \"%s\". No value specified.",
					ppszArgs[i]);
				MessageBoxW(NULL, szMsgErrorBuffer, 
					L"Error", MB_ICONERROR | MB_OK);
				return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
			}
			
			std::wstring key = statement.substr(0, statement.find(L"="));
			
			PackOption *pOption = _FindOption(key.c_str());
			if (!pOption)
			{
				StringCchPrintfW(szMsgErrorBuffer, ARRAYSIZE(szMsgErrorBuffer), 
					L"Improperly formed option argument \"%s\". Invalid option name \"%s\" specified.",
					ppszArgs[i], key.c_str());
				MessageBoxW(NULL, szMsgErrorBuffer, 
					L"Error", MB_ICONERROR | MB_OK);
				return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
			}
			
			std::wstring strValue = statement.substr(statement.find(L"=") + 1);
			UINT uValue = 0;
			RETURN_IF_WIN32_BOOL_FALSE((BOOL)StringToUInt(strValue.c_str(), &uValue));
			
			// Make sure the requested value is valid:
			if (!_ValidateOptionValue(*pOption, uValue))
			{
				StringCchPrintfW(szMsgErrorBuffer, ARRAYSIZE(szMsgErrorBuffer), 
					L"Improperly formed option argument \"%s\". Invalid option value \"%s\" specified.",
					ppszArgs[i], strValue.c_str());
				MessageBoxW(NULL, szMsgErrorBuffer, 
					L"Error", MB_ICONERROR | MB_OK);
				return HRESULT_FROM_WIN32(ERROR_BAD_ARGUMENTS);
			}
			
			// Set the option value:
			pOption->uValue = uValue;
		}
	}
	
	return S_OK;
}

bool CPack::Apply(void *lpParam, PackApplyProgressCallback pfnCallback)
{
	std::vector<PackSection> secs;
	for (const auto &sec : _sections)
	{
		if (_OptionDefMatches(sec.requires))
		{
			if (sec.uMinBuild && g_dwOSBuild < sec.uMinBuild)
				continue;

			if (sec.uMaxBuild && g_dwOSBuild > sec.uMaxBuild)
				continue;

			secs.push_back(sec);
		}
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
					HRESULT hr = pfnWaitFunc(command.c_str(), &dwExitCode);
					if (NTMU_IMPERSONATION_E_TRUSTEDINSTALLER_SVC_DISABLED == hr)
					{
						Log(
							L"Failed to create regedit.exe process, which requires the TrustedInstaller service "
							L"to be available. A required service is disabled.");
						return false;
					}
					if (NTMU_IMPERSONATION_E_TRUSTEDINSTALLER_SVC_FAILED == hr)
					{
						Log(
							L"Failed to create regedit.exe process, which requires the TrustedInstaller service "
							L"to be available. A required service failed to start.");
						return false;
					}
					if (NTMU_IMPERSONATION_E_SECLOGON_SVC_DISABLED == hr)
					{
						Log(
							L"Failed to create regedit.exe process, which requires the Seclogon service "
							L"to be available. A required service is disabled.");
						return false;
					}
					if (NTMU_IMPERSONATION_E_SECLOGON_SVC_FAILED == hr)
					{
						Log(
							L"Failed to create regedit.exe process, which requires the Seclogon service "
							L"to be available. A required service failed to start.");
						return false;
					}
					if (FAILED(hr))
					{
						Log(L"Failed to create regedit.exe process. HRESULT: %8x. Last error: %u.", hr, GetLastError());
						return false;
					}
					if (S_FALSE == hr)
					{
						Log(L"Failed to create regedit.exe process. Exit code: %u.", dwExitCode);
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

					HANDLE hUpdateRes = NULL;
					bool fSucceeded = false;
					PEResources destResources, sourceResources;
					if (!LoadResources(szTempFile, false, destResources))
					{
						Log(L"Failed to load resources for file '%s'", szTempFile);
						goto cleanup;
					}
					if (!LoadResources(item.sourceFile.c_str(), true, sourceResources))
					{
						Log(L"Failed to load resources for file '%s", item.sourceFile.c_str());
						goto cleanup;
					}
					MergeResources(sourceResources, destResources);

					hUpdateRes = BeginUpdateResourceW(szTempFile, TRUE);
					
					// We do group icons and group cursors separately due to how shit the system is.
					// Group icons/icons and group cursors/cursors are technically separate resources,
					// but the group icon/group cursor resource is just a resource that has a list of
					// icon/cursor resources to use and other information e.g. what size they are. Why
					// didn't Microsoft clump this all into one big resource? I don't know. Maybe Win16
					// limitations prevented it and that somehow carried over into Win32 and the PE format.
					// All I know is that we have to handle these resources specially, otherwise we will
					// almost certainly cause nasty collisions.
					//
					// Also, we do these before regular resources so that we can do MUI resources last,
					// because Microsoft sucks shit.

					WORD wIconCount; // mmm i love c++ complaining about goto + initializers even on basic fucking types
					wIconCount = 0;
					for (const auto &groupIcon : destResources.groupIcons)
					{
						size_t entryCount = groupIcon.entries.size();
						size_t dataSize
							= sizeof(ICONDIRHEADER) + (sizeof(GRPICONDIRENTRY) * entryCount);

						ICONDIR *pIconDir = (ICONDIR *)LocalAlloc(LPTR, dataSize);
						if (!pIconDir)
						{
							Log(L"Failed to allocate buffer necessary for group icon");
							goto cleanup;
						}

						pIconDir->idReserved = 0;
						pIconDir->idType = RES_ICON;
						pIconDir->idCount = entryCount;
						
						for (int i = 0; i < entryCount; i++)
						{
							memcpy(&pIconDir->icons[i], &groupIcon.entries[i], sizeof(GRPICONDIRENTRY));
							WORD wIconID = ++wIconCount;
							pIconDir->icons[i].nID = wIconID;

							const auto &data = groupIcon.entries[i].data;
							if (!UpdateResourceW(
								hUpdateRes,
								RT_ICON, MAKEINTRESOURCEW(wIconID),
								groupIcon.wLangId, (LPBYTE)data.data(), data.size()
							))
							{
								Log(L"Failed to update icon resource");
								LocalFree(pIconDir);
								goto cleanup;
							}
						}

						if (!UpdateResourceW(
							hUpdateRes,
							RT_GROUP_ICON, groupIcon.name.AsID(),
							groupIcon.wLangId, pIconDir, dataSize
						))
						{
							Log(L"Failed to update group icon resource");
							LocalFree(pIconDir);
							goto cleanup;
						}

						LocalFree(pIconDir);
					}

					WORD wCursorCount; // mmm i love c++ complaining about goto + initializers even on basic fucking types
					wCursorCount = 0;
					for (const auto &groupCursor : destResources.groupCursors)
					{
						size_t entryCount = groupCursor.entries.size();
						size_t dataSize
							= sizeof(ICONDIRHEADER) + (sizeof(GRPCURSORDIRENTRY) * entryCount);

						ICONDIR *pIconDir = (ICONDIR *)LocalAlloc(LPTR, dataSize);
						if (!pIconDir)
						{
							Log(L"Failed to allocate buffer necessary for group cursor");
							goto cleanup;
						}

						pIconDir->idReserved = 0;
						pIconDir->idType = RES_CURSOR;
						pIconDir->idCount = entryCount;

						for (int i = 0; i < entryCount; i++)
						{
							memcpy(&pIconDir->cursors[i], &groupCursor.entries[i], sizeof(GRPCURSORDIRENTRY));
							WORD wCursorID = ++wCursorCount;
							pIconDir->cursors[i].nID = wCursorID;

							const auto &data = groupCursor.entries[i].data;
							if (!UpdateResourceW(
								hUpdateRes,
								RT_CURSOR, MAKEINTRESOURCEW(wCursorID),
								groupCursor.wLangId, (LPBYTE)data.data(), data.size()
							))
							{
								Log(L"Failed to update cursor resource");
								LocalFree(pIconDir);
								goto cleanup;
							}
						}

						if (!UpdateResourceW(
							hUpdateRes,
							RT_GROUP_CURSOR, groupCursor.name.AsID(),
							groupCursor.wLangId, pIconDir, dataSize
						))
						{
							Log(L"Failed to update group cursor resource");
							LocalFree(pIconDir);
							goto cleanup;
						}

						LocalFree(pIconDir);
					}

					for (const auto &res : destResources.resources)
					{
						if (!UpdateResourceW(
							hUpdateRes,
							res.type.AsID(), res.name.AsID(),
							res.wLangId,
							(LPVOID)res.data.data(), res.data.size()
						))
						{
							const PEResourceType *types[2] = { &res.type, &res.name };
							WCHAR szTypeStrings[2][256];
							for (int i = 0; i < 2; i++)
							{
								if (types[i]->fIsOrdinal)
									swprintf_s(szTypeStrings[i], L"%u", types[i]->wOrdinal);
								else
									swprintf_s(szTypeStrings[i], L"'%s'", types[i]->string.c_str());
							}
							Log(
								L"Failed to update resource with type %s, name %s, and language %u in file '%s'. Error: %d",
								szTypeStrings[0], szTypeStrings[1], res.wLangId, szTempFile, GetLastError()
							);
							goto cleanup;
						}
					}
					
					fSucceeded = true;
cleanup:
					if (hUpdateRes) EndUpdateResourceW(hUpdateRes, !fSucceeded);

					if (fSucceeded && !_CopyFileWithOldStack(szTempFile, item.destFile.c_str()))
						fSucceeded = false;

					DeleteFileW(szTempFile);
					if (!fSucceeded) return false;
				}
				break;
			}
		}
	}

	Log(L"All done!");

	return true;
}