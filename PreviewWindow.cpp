#include "PreviewWindow.h"

LRESULT CPreviewWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			return _OnCreate();
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT rc;
			GetClientRect(hWnd, &rc);

			FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
			
			LOGBRUSH lb = { 0 };
			lb.lbColor = GetSysColor(COLOR_GRAYTEXT);
			lb.lbStyle = PS_SOLID;
			HPEN hpen = ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, nullptr);
			HPEN hpenOld = (HPEN)SelectObject(hdc, hpen);
			
			MoveToEx(hdc, rc.left, rc.top, nullptr);
			LineTo(hdc, rc.right - 1, rc.top);
			LineTo(hdc, rc.right - 1, rc.bottom - 1);
			LineTo(hdc, rc.left, rc.bottom - 1);
			LineTo(hdc, rc.left, rc.top);

			if (_pbmPreview)
			{ 
				Gdiplus::Graphics gfx(hdc);
				int width = _pbmPreview->GetWidth();
				int height = _pbmPreview->GetHeight();

				InflateRect(&rc, -1, -1);

				float aspectRatio = (float)width / (float)height;
				float canvasRatio = (float)RECTWIDTH(rc) / (float)RECTHEIGHT(rc);
				int scaledWidth = width;
				int scaledHeight = height;
				if (width > RECTWIDTH(rc) || height > RECTHEIGHT(rc))
				{
					scaledWidth = RECTWIDTH(rc);
					scaledHeight = RECTHEIGHT(rc);
					if (aspectRatio < canvasRatio)
					{
						scaledWidth = width * (float)((float)RECTHEIGHT(rc) / (float)height);
					}
					else if (aspectRatio > canvasRatio)
					{
						scaledHeight = height * (float)((float)RECTWIDTH(rc) / (float)width);
					}
				}

				Gdiplus::Rect rect(
					1 + (RECTWIDTH(rc) - scaledWidth) / 2,
					1 + (RECTHEIGHT(rc) - scaledHeight) / 2,
					scaledWidth, scaledHeight
				);
				gfx.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);
				gfx.DrawImage(_pbmPreview, rect);
			}
			else
			{
				HFONT hfOld = (HFONT)SelectObject(hdc, _hfMessage);
				int bkOld = SetBkMode(hdc, TRANSPARENT);

				DrawTextW(
					hdc, _szNoPreview, -1,
					&rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_WORDBREAK
				);

				SetBkMode(hdc, bkOld);
				SelectObject(hdc, hfOld);
			}

			SelectObject(hdc, hpenOld);
			DeleteObject(hpen);

			EndPaint(hWnd, &ps);
			return 0;
		}
		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

LRESULT CPreviewWindow::_OnCreate()
{
	if (FAILED(CoInitialize(NULL))
	|| FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_pFactory))))
		return -1;

	Gdiplus::GdiplusStartupInput si;
	if (Gdiplus::Ok != Gdiplus::GdiplusStartup(&_ulGdipToken, &si, nullptr))
		return -1;

	LoadStringW(g_hinst, IDS_NOPREVIEW, _szNoPreview, MAX_PATH);

	//SetImage(L"C:\\Users\\Aubrey\\Pictures\\patchy.png");
	return 0;
}

// static
HRESULT CPreviewWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	return CWindow::RegisterWindowClass(&wc);
}

// static
CPreviewWindow *CPreviewWindow::CreateAndShow(HWND hwndParent)
{
	CPreviewWindow *pWindow = Create(
		NULL,
		nullptr,
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		hwndParent, NULL,
		g_hinst,
		nullptr
	);

	if (!pWindow)
		return nullptr;

	ShowWindow(pWindow->_hwnd, SW_SHOW);
	return pWindow;
}

CPreviewWindow::CPreviewWindow()
	: _szNoPreview{ 0 }
	, _hfMessage(NULL)
	, _pFactory(nullptr)
	, _pbmPreview(nullptr)
	, _ulGdipToken(0)
{
	
}

CPreviewWindow::~CPreviewWindow()
{
	if (_ulGdipToken)
		Gdiplus::GdiplusShutdown(_ulGdipToken);

	CoUninitialize();
}

void CPreviewWindow::SetFont(HFONT hFont)
{
	_hfMessage = hFont;
}

HRESULT CPreviewWindow::SetImage(LPCWSTR pszPath)
{
	if (!pszPath || !pszPath[0])
	{
		if (_pbmPreview)
		{
			delete _pbmPreview;
			_pbmPreview = nullptr;
		}
		InvalidateRect(_hwnd, nullptr, TRUE);
		return S_OK;
	}

	wil::com_ptr<IWICBitmapDecoder> pDecoder;
	wil::com_ptr<IWICBitmapFrameDecode> pFrame;
	wil::com_ptr<IWICFormatConverter> pConverter;

	RETURN_IF_FAILED(_pFactory->CreateDecoderFromFilename(
		pszPath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder
	));
	RETURN_IF_FAILED(pDecoder->GetFrame(0, &pFrame));
	RETURN_IF_FAILED(_pFactory->CreateFormatConverter(&pConverter));
	RETURN_IF_FAILED(pConverter->Initialize(
		pFrame.get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeCustom
	));

	UINT width, height;
	pConverter->GetSize(&width, &height);
	_pbmPreview = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);

	Gdiplus::BitmapData bd;
	Gdiplus::Rect rect(0, 0, width, height);
	_pbmPreview->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bd);
	RETURN_IF_FAILED(pConverter->CopyPixels(nullptr, bd.Stride, bd.Stride * height, (LPBYTE)bd.Scan0));
	_pbmPreview->UnlockBits(&bd);
	InvalidateRect(_hwnd, nullptr, TRUE);
	return S_OK;
}