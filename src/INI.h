#pragma once
#include <vector>
#include <fstream>
#include <string>
#include "NTMU.h"

struct INIValue
{
	std::wstring name;
	std::wstring value;
};

struct INISection
{
	std::wstring name;
	std::vector<INIValue> values;

	std::wstring GetPropByName(LPCWSTR pszName)
	{
		for (const INIValue &val : values)
		{
			if (0 == _wcsicmp(pszName, val.name.c_str()))
				return val.value;
		}
		return std::wstring();
	}
};

typedef std::vector<INISection> INIFile;

bool ParseINIFile(const wchar_t *path, INIFile &out);