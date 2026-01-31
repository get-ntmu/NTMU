#include "INI.h"
#include "Util.h"
#include "translations/ini_errors.h"

bool ParseINIFile(const wchar_t *path, INIFile &out)
{
	static const mm_ini_errors_translations_t *pTranslations
		= mm_get_ini_errors_translations();

	out.clear();

	std::wifstream file(path);
	if (!file)
	{
		msgmap::wstring spszError = pTranslations->open_failed(path);
		MainWndMsgBox(spszError, MB_ICONERROR);
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
				msgmap::wstring spszError = pTranslations->unclosed_section_header(lines);
				MainWndMsgBox(spszError, MB_ICONERROR);
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
			msgmap::wstring spszError = pTranslations->value_outside_section(lines);
			MainWndMsgBox(spszError, MB_ICONERROR);
			return false;
		}
		if (line.find(L'=') == std::wstring::npos)
		{
			msgmap::wstring spszError = pTranslations->missing_equals(lines);
			MainWndMsgBox(spszError, MB_ICONERROR);
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