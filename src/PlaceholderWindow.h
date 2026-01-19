#pragma once
#include "Window.h"
#include <gdiplus.h>
#include <wincodec.h>

/**
 * Basic functionality of the placeholder window.
 * 
 * This basically just exists to allow other windows to reuse functionality from
 * the placeholder window without inheriting from it, since CPlaceholderWindow
 * derives from CWindow and that would result in duplicate window related members.
 */
class CPlaceholderWindowBase
{
private:
	WCHAR _szMessage[MAX_PATH] = {};
	HFONT _hfMessage           = NULL;
	
public:
	/**
	 * Copy the placeholder text from another string.
	 */
	HRESULT CopyPlaceholderText(LPCWSTR pszPlaceholderText);

	/**
	 * Load the placeholder text from a string resource.
	 */
	HRESULT LoadPlaceholderText(HINSTANCE hInstance, UINT uID);

	/**
	 * Draw the placeholder contents into a HDC.
	 */
	HRESULT DrawPlaceholder(HDC hdc, RECT rc);

	/**
	 * Set the font of the placeholder window.
	 */
	void SetFont(HFONT hFont);
};

static constexpr WCHAR c_szPlaceholderWindowClass[] = L"NTMU_Placeholder";

/**
 * Class for the placeholder window itself.
 */
class CPlaceholderWindow
	: public CWindow<CPlaceholderWindow, c_szPlaceholderWindowClass>
	, public CPlaceholderWindowBase
{
private:
	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	LRESULT _OnCreate();

public:
	static HRESULT RegisterWindowClass();
	static CPlaceholderWindow *CreateAndShow(HWND hwndParent);
};