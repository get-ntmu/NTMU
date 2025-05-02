#pragma once
#include "Window.h"

static constexpr WCHAR c_szMainWindowClass[] = L"NTMU_MainWindow";

class CMainWindow : public CWindow<CMainWindow, c_szMainWindowClass>
{
private:
	UINT _dpi;
	HACCEL _hAccel;

	enum METAINDEX
	{
		MI_NAME = 0,
		MI_AUTHOR,
		MI_VERSION,
		MI_COUNT
	};

	HWND _rghwndLabels[MI_COUNT];
	HWND _rghwndMetas[MI_COUNT];

	HFONT _hfMessage;
	int _cxMsgFontChar;
	int _cyMsgFontChar;

	virtual LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	inline int _XDUToXPix(int x)
	{
		return MulDiv(x, _cxMsgFontChar, 4);
	}

	inline int _YDUToYPix(int y)
	{
		return MulDiv(y, _cyMsgFontChar, 8);
	}

	void _OnCreate();
	void _UpdateFonts();
	void _UpdateLayout();

public:
	~CMainWindow();

	static HRESULT RegisterWindowClass();
	static CMainWindow *CreateAndShow(int nCmdShow);

	HACCEL GetAccel();
};