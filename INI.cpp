#include "INI.h"
#include "Util.h"

bool ParseINIFile(const wchar_t *path, INIFile &out)
{
	out.clear();

	std::wifstream file(path);
	if (!file)
	{
		WCHAR szError[MAX_PATH * 2];
		swprintf_s(szError, L"Failed to open file '%s' for reading.", path);
		MainWndMsgBox(szError, MB_ICONERROR);
		return false;
	}

	std::wstring line;
	int lines = 0;
	while (std::getline(file, line))
	{
		// Keep track of line number for error reporting
		lines++;

		// Remove comments and trim whitespace
		line = line.substr(0, line.find(L';'));
		trim(line);

		// Skip empty lines
		if (line.length() == 0)
			continue;

		// Parse section header
		if (line.at(0) == L'[')
		{
			if (line.back() != L']')
			{
				WCHAR szError[MAX_PATH];
				swprintf_s(szError, L"Unclosed section header on line %d.", lines);
				MainWndMsgBox(szError, MB_ICONERROR);
				out.clear();
				return false;
			}

			INISection sec = {};
			sec.name = line.substr(1, line.length() - 2);
			trim(sec.name);
			out.push_back(sec);
			continue;
		}

		// Parse values
		if (out.size() == 0)
		{
			WCHAR szError[MAX_PATH];
			swprintf_s(szError, L"Value outside of section on line %d.", lines);
			MainWndMsgBox(szError, MB_ICONERROR);
			return false;
		}
		if (line.find(L'=') == std::wstring::npos)
		{
			WCHAR szError[MAX_PATH];
			swprintf_s(szError, L"Missing = on line %d.", lines);
			MainWndMsgBox(szError, MB_ICONERROR);
			out.clear();
			return false;
		}
		// Push to last section we added
		INISection &sec = out.back();
		INIValue val;
		val.name = line.substr(0, line.find(L'='));
		trim(val.name);
		val.value = line.substr(line.find(L'=') + 1);
		trim(val.value);
		sec.values.push_back(val);
	}
	return true;
}