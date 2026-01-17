#pragma once
#include "Window.h"
#include "PlaceholderWindow.h"
#include <gdiplus.h>
#include <wincodec.h>

static constexpr WCHAR c_szPreviewWindowClass[] = L"NTMU_Preview";

class CPreviewWindow
	: public CWindow<CPreviewWindow, c_szPreviewWindowClass>
	, public CPlaceholderWindowBase
{
private:
	wil::com_ptr<IWICImagingFactory> _pFactory;
	Gdiplus::Bitmap *_pbmPreview;
	ULONG_PTR _ulGdipToken;

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	LRESULT _OnCreate();

public:
	static HRESULT RegisterWindowClass();
	static CPreviewWindow *CreateAndShow(HWND hwndParent);

	CPreviewWindow();
	~CPreviewWindow();
	
	HRESULT SetImage(LPCWSTR pszPath);
};