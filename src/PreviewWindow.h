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
	ULONG_PTR _ulGdipToken;
	Gdiplus::Bitmap *_pbmPreviewOriginal;
	
	Gdiplus::Bitmap **_prgpbmMipmapPreviews;
	int _cMipmapPreviews;

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	LRESULT _OnCreate();
	HRESULT _CreateMipmap(IWICBitmapSource *pBitmapSource, int width, int height, Gdiplus::Bitmap **ppMipmapOut);

public:
	static HRESULT RegisterWindowClass();
	static CPreviewWindow *CreateAndShow(HWND hwndParent);

	CPreviewWindow();
	~CPreviewWindow();
	
	HRESULT SetImage(LPCWSTR pszPath);
};