#include "PreviewWindow.h"

#include <cmath>
#include <limits>

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

			if (_pbmPreviewOriginal)
			{ 
				Gdiplus::Graphics gfx(hdc);
				int width = _pbmPreviewOriginal->GetWidth();
				int height = _pbmPreviewOriginal->GetHeight();

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
				
				// Select the desirable mipmapped image for the scaled width/height:
				Gdiplus::Bitmap *pBitmapToDraw;
				bool fIsBadFpResult = false;
				float flBestMipmapIdx = floor(log10((float)scaledWidth / (float)width) / log10(0.5f));
				if (std::numeric_limits<float>::infinity() == flBestMipmapIdx || std::isnan(flBestMipmapIdx))
				{
					fIsBadFpResult = true;
				}
				
				UINT idxMipmap = flBestMipmapIdx - 1;

				if (!_prgpbmMipmapPreviews || 0 == _cMipmapPreviews
					|| fIsBadFpResult || idxMipmap >= _cMipmapPreviews)
				{
					pBitmapToDraw = _pbmPreviewOriginal;
				}
				else
				{
					pBitmapToDraw = _prgpbmMipmapPreviews[idxMipmap];
				}
				
				gfx.SetInterpolationMode(Gdiplus::InterpolationModeBicubic);
				gfx.DrawImage(pBitmapToDraw, rect);
				
				
// #define MIPMAP_SHOWDEBUG
#ifdef MIPMAP_SHOWDEBUG
				if (pBitmapToDraw != _pbmPreviewOriginal)
				{
					static const WCHAR rgszNumberMap[][3] = {
						L"0",
						L"1",
						L"2",
						L"3",
						L"4",
						L"5",
						L"6",
						L"7",
						L"8",
						L"9",
						L"10",
						L"11",
						L"12",
						L"13",
						L"14",
						L"15",
						L"16",
						L"17",
						L"18",
						L"19",
						L"20",
						L"21",
						L"22",
						L"23",
						L"24",
						L"25",
						L"26",
						L"27",
						L"28",
						L"29",
						L"30",
						L"31",
					};
					Gdiplus::FontFamily arial(L"Arial");
					Gdiplus::Font font(&arial, 12);
					Gdiplus::SolidBrush brush(Gdiplus::Color::Red);
					Gdiplus::PointF point(rect.X, rect.Y);
					gfx.DrawString( (LPCWSTR)rgszNumberMap[idxMipmap], (idxMipmap < 10 ? 1 : 2), &font, point, &brush);
				}
#endif // MIPMAP_SHOWDEBUG
			}
			else
			{
				DrawPlaceholder(hdc, rc);
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

	LoadPlaceholderText(g_hinst, IDS_NOPREVIEW);
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
	: _pFactory(nullptr)
	, _pbmPreviewOriginal(nullptr)
	, _prgpbmMipmapPreviews(nullptr)
	, _cMipmapPreviews(0)
	, _ulGdipToken(0)
{
	
}

CPreviewWindow::~CPreviewWindow()
{
	if (_ulGdipToken)
		Gdiplus::GdiplusShutdown(_ulGdipToken);
	
	delete[] _prgpbmMipmapPreviews;

	CoUninitialize();
}

/**
 * Creates a single mipmap step of the requested image.
 * 
 * This allows the preservation of quality by using a slow, high quality blurring algorithm for
 * major steps while still allowing for real time scaling as the steps are precomputed and
 * cached.
 * 
 * @param pBitmapSource A pointer to the original bitmap source.
 * @param width The requested width of the image.
 * @param height The requested height of the image.
 * @param ppMipmapOut Pointer to the location at which to store the resulting bitmap pointer.
 * @return 
 *		If successful and the width and height of the resulting image are the same as requested,
 *		then S_OK.
 *		If successful, but the width and height of the resulting image were changed by an
 *		intermediate operation, then S_FALSE.
 */
HRESULT CPreviewWindow::_CreateMipmap(IWICBitmapSource *pBitmapSource, int width, int height, Gdiplus::Bitmap **ppMipmapOut)
{
	RETURN_HR_IF_NULL(E_INVALIDARG, pBitmapSource);
	RETURN_HR_IF_NULL(E_INVALIDARG, ppMipmapOut);

	*ppMipmapOut = nullptr;

	HRESULT hr = S_OK;
	
	wil::com_ptr<IWICBitmapScaler> spScaler;
	RETURN_IF_FAILED(_pFactory->CreateBitmapScaler(&spScaler));
	
	RETURN_IF_FAILED(spScaler->Initialize(
		pBitmapSource, width, height, WICBitmapInterpolationModeHighQualityCubic));
	
	UINT workingWidth, workingHeight;
	RETURN_IF_FAILED(spScaler->GetSize(&workingWidth, &workingHeight));
	
	if (workingWidth != width || workingHeight != height)
	{
		// This is just a weird case. It shouldn't necessarily mean anything bad, but
		// maybe we'll return S_FALSE here instead of S_OK.h
		hr = S_FALSE;
	}
	
	Gdiplus::Bitmap *pScaledBitmap = new Gdiplus::Bitmap(workingWidth, workingHeight);
	Gdiplus::BitmapData bd;
	Gdiplus::Rect rect(0, 0, workingWidth, workingHeight);
	pScaledBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bd);
	RETURN_IF_FAILED(spScaler->CopyPixels(nullptr, bd.Stride, bd.Stride * height, (LPBYTE)bd.Scan0));
	pScaledBitmap->UnlockBits(&bd);
	
	*ppMipmapOut = std::move(pScaledBitmap);
	return hr;
}

HRESULT CPreviewWindow::SetImage(LPCWSTR pszPath)
{
	if (!pszPath || !pszPath[0])
	{
		if (_pbmPreviewOriginal)
		{
			delete _pbmPreviewOriginal;
			_pbmPreviewOriginal = nullptr;
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
	RETURN_IF_FAILED(pConverter->GetSize(&width, &height));
	_pbmPreviewOriginal = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);

	Gdiplus::BitmapData bd;
	Gdiplus::Rect rect(0, 0, width, height);
	_pbmPreviewOriginal->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bd);
	RETURN_IF_FAILED(pConverter->CopyPixels(nullptr, bd.Stride, bd.Stride * height, (LPBYTE)bd.Scan0));
	_pbmPreviewOriginal->UnlockBits(&bd);
	
	// Next, we'll find the number of mipmap steps to load. This can be approximated through
	// a simple binary logarithm, as the steps decrease in size exponentially from the
	// original image. Once we hit 16x16, we'll cut it off.
	UINT nSteps = log2(max(width, height));
	if (UINT_MAX != nSteps)
	{
		_prgpbmMipmapPreviews = new Gdiplus::Bitmap *[nSteps];
		_cMipmapPreviews = nSteps;
		
		double nextWidth = width, nextHeight = height;
		for (int i = 0; i < nSteps; i++)
		{
			nextWidth /= 2;
			nextHeight /= 2;

			if (floor(nextWidth) < 16 || floor(nextHeight) < 16)
			{
				// We'll break at this point to ensure that the size of things remains
				// rather small.
				_cMipmapPreviews = i;
				break;
			}
			
			HRESULT hrFailed = LOG_IF_FAILED_MSG(
				_CreateMipmap(pConverter.get(), nextWidth, nextHeight, &_prgpbmMipmapPreviews[i]),
				"Failed to create mipmap %d with dimensions %dx%d", i, nextWidth, nextHeight);
			
			if (FAILED(hrFailed))
			{
				delete[] _prgpbmMipmapPreviews;
				_prgpbmMipmapPreviews = nullptr;
				_cMipmapPreviews = 0;
				break;
			}
		}
	}
	
	InvalidateRect(_hwnd, nullptr, TRUE);
	return S_OK;
}