#pragma once
#include "Window.h"
#include "PreviewWindow.h"
#include "pack.h"

static constexpr WCHAR c_szMainWindowClass[] = L"NTMU_MainWindow";

#define IDC_APPLY        1000

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

	HWND _hwndProgress;
	HWND _hwndApply;

	HWND _hwndText;
	HWND _hwndPreview;
	HWND _hwndOptions;

	CPreviewWindow *_pPreviewWnd;

	HFONT _hfMessage;
	HFONT _hfMonospace;
	int _cxMsgFontChar;
	int _cyMsgFontChar;

	CPack _pack;

	static INT_PTR CALLBACK s_AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

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

	void _LoadPack(LPCWSTR pszPath);
	void _UnloadPack();

public:
	CMainWindow();
	~CMainWindow();

	static HRESULT RegisterWindowClass();
	static CMainWindow *CreateAndShow(int nCmdShow);

	HACCEL GetAccel();
};