#include "NTMU.h"
#include "EnumPEResources.h"

int WINAPI wWinMain(
	_In_     HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_     LPWSTR    lpCmdLine,
	_In_     int       nShowCmd
)
{
	IEnumResources *pEnum = nullptr;
	if (FAILED(CEnumPEResources_CreateInstance(L"C:\\Windows\\SystemResources\\shell32.dll.mun", &pEnum)))
	{
		MessageBoxW(NULL, L"FUCK", L"FUCK", MB_ICONERROR);
		return -1;
	}

	pEnum->Enum([](LPCWSTR lpType, LPCWSTR lpName, LANGID lcid, LPVOID lpData, DWORD cbData) -> BOOL {
		__debugbreak();
		return TRUE;
	});

	pEnum->Destroy();

	return 0;
}