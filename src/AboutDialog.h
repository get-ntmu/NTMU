#pragma once
#include "NTMUWindowBase.h"
#include "translations/about_dialog.h"

static constexpr WCHAR c_szAboutDialogClass[] = L"NTMU_AboutDialog";

#define IDC_GITHUB             1000

class CAboutDialog : public CNTMUWindowBase<CAboutDialog, c_szAboutDialogClass>
{
private:
	HWND _hwndIcon;
	HWND _hwndAppName;
	HWND _hwndAppVersion;
	HWND _hwndAppInfo;
	HWND _hwndGitHubLink;
	HWND _hwndOKButton;

	HWND _hwndParent;

	HICON _hIcon;

	int _cxIcon;
	int _cyIcon;

	const mm_about_dialog_translations_t *_pTranslations;

	static constexpr int c_numLayoutWindows = 6;

	static constexpr int c_duMargin       =   8;
	static constexpr int c_duLabelWidth   = 155;
	static constexpr int c_duLabelHeight  =   8;
	static constexpr int c_duLabelMargin  =   2;
	static constexpr int c_duButtonWidth  =  50;
	static constexpr int c_duButtonHeight =  14;
	static constexpr int c_duButtonMargin =  6;

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void _OnCreate();
	void _UpdateMetrics();
	void _UpdateLayout();

public:
	CAboutDialog();
	~CAboutDialog();

	static HRESULT RegisterWindowClass();
	static CAboutDialog *CreateAndShow(HWND hwndParent);
};