#pragma once
#include "Window.h"

static constexpr WCHAR c_szMainWindowClass[] = L"NTMU_MainWindow";

class CMainWindow : public CWindow<CMainWindow, c_szMainWindowClass>
{
private:
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

	void _OnCreate();
	void _UpdateFonts();

public:
	~CMainWindow();

	static HRESULT RegisterWindowClass();
	static CMainWindow *CreateAndShow(int nCmdShow);

	HACCEL GetAccel();
};