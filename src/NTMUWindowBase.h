#pragma once
#include "Window.h"
#include "DPIHelpers.h"
#include "pack.h"

template <typename CImpl, LPCWSTR c_szClassName>
class CNTMUWindowBase : public CWindow<CImpl, c_szClassName>
{
protected:
	UINT _dpi;

	HFONT _hfMessage;
	int _cxMsgFontChar;
	int _cyMsgFontChar;
	
	CNTMUWindowBase()
		: _dpi(0)
		, _hfMessage(NULL)
		, _cxMsgFontChar(0)
		, _cyMsgFontChar(0)
	{
	}

	~CNTMUWindowBase()
	{
		if (_hfMessage)
			DeleteObject(_hfMessage);
	}

	inline int _XDUToXPix(int x)
	{
		return MulDiv(x, _cxMsgFontChar, 4);
	}

	inline int _YDUToYPix(int y)
	{
		return MulDiv(y, _cyMsgFontChar, 8);
	}
	
	inline void _UpdateMetrics()
	{
		_dpi = DPIHelpers::GetWindowDPI(CWindow<CImpl, c_szClassName>::_hwnd);

		if (_hfMessage)
		{
			DeleteObject(_hfMessage);
			_hfMessage = NULL;
		}

		NONCLIENTMETRICSW ncm = { sizeof(ncm) };
		DPIHelpers::SystemParametersInfoForDPI(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE, _dpi);
		_hfMessage = CreateFontIndirectW(&ncm.lfMessageFont);

		HDC hdc = CreateCompatibleDC(NULL);
		SelectObject(hdc, _hfMessage);
	
		TEXTMETRICW tm = { 0 };
		GetTextMetricsW(hdc, &tm);

		_cyMsgFontChar = tm.tmHeight;
		if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
		{
			static const WCHAR wszAvgChars[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

			SIZE size;
			if (GetTextExtentPoint32W(hdc, wszAvgChars, ARRAYSIZE(wszAvgChars) - 1, &size))
			{
				// The above string is 26 * 2 characters. + 1 rounds the result.
				_cxMsgFontChar = ((size.cx / 26) + 1) / 2;
			}
		}
		else
		{
			_cxMsgFontChar = tm.tmAveCharWidth;
		}

		DeleteDC(hdc);
	}
};