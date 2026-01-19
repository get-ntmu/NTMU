#pragma once
#include "Window.h"
#include "PlaceholderWindow.h"
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
	
	WCHAR _szNotSpecified[MAX_PATH];

	HWND _rghwndLabels[MI_COUNT];
	HWND _rghwndMetas[MI_COUNT];
	
	static constexpr int c_numLayoutWindows = 5;

	HWND _hwndProgress;
	HWND _hwndApply;

	HWND _hwndText;
	HWND _hwndPreview;
	HWND _hwndOptions;
	
	static constexpr int c_numPlaceholders = 2;
	
	HWND _hwndTextPlaceholder;
	HWND _hwndOptionsPlaceholder;
	
	struct
	{
		bool _fShowingTextPlaceholder : 1;
		bool _fShowingOptionsPlaceholder : 1;
	};
	
	CPlaceholderWindow *_pTextPlaceholderWnd;
	CPlaceholderWindow *_pOptionsPlaceholderWnd;

	enum OPTIONSIMAGEINDEX
	{
		OII_CPL = 0,
		OII_CHECK,
		OII_CHECK_ON,
		OII_RADIO,
		OII_RADIO_ON,
		OII_COUNT
	};

	HIMAGELIST _himlOptions;

	CPreviewWindow *_pPreviewWnd;

	HFONT _hfMessage;
	HFONT _hfMonospace;
	int _cxMsgFontChar;
	int _cyMsgFontChar;

	CPack _pack;
	bool _fApplying;

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
	void _LoadReadme();
	
	void _ToggleTextPlaceholder(bool fStatus);
	void _ToggleOptionsPlaceholder(bool fStatus);

	static void s_ApplyProgressCallback(void *lpParam, DWORD dwItemsProcessed, DWORD dwTotalItems);
	void _ApplyPack();
	static DWORD CALLBACK s_ApplyPackThreadProc(LPVOID lpParam);
	static void s_LogCallback(void *lpParam, LPCWSTR pszText);
	void _ApplyPackWorker();

public:
	CMainWindow();
	~CMainWindow();

	static HRESULT RegisterWindowClass();
	static CMainWindow *CreateAndShow(int nCmdShow);

	HACCEL GetAccel();
};