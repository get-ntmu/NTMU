#pragma once
#include "NTMUWindowBase.h"
#include "PlaceholderWindow.h"
#include "PreviewWindow.h"
#include "Pack.h"
#include "translations/main_window.h"

static constexpr WCHAR c_szMainWindowClass[] = L"NTMU_MainWindow";

#define IDC_APPLY        1000

#define IDM_FILEOPEN           201
#define IDM_FILEUNLOAD         202
#define IDM_FILEEXIT           203
#define IDM_TOOLSKILLEXPLORER  204
#define IDM_TOOLSCLEARICOCACHE 205
#define IDM_TOOLSSYSRESTORE    206
#define IDM_HELPTOPICS         207
#define IDM_HELPGETPACKS       208
#define IDM_HELPABOUT          209

class CMainWindow : public CNTMUWindowBase<CMainWindow, c_szMainWindowClass>
{
private:
	const mm_main_window_translations_t *_pTranslations;

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
	
	HFONT _hfMonospace;

	CPack _pack;
	bool _fApplying;

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void _CreateMenu();
	void _OnCreate();
	void _UpdateMetrics();
	void _UpdateLayout();
	
	enum class LoadSource
	{
		Default,
		CommandLine,
	};

	void _LoadPack(LPCWSTR pszPath, LoadSource loadSource);
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