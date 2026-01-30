#pragma once
#include "NTMUWindowBase.h"
#include "Pack.h"
#include "translations/unattend_window.h"

static constexpr WCHAR c_szUnattendWindowClass[] = L"NTMU_UnattendWindow";

#define IDC_CLOSE        1000

class CUnattendWindow : public CNTMUWindowBase<CUnattendWindow, c_szUnattendWindowClass>
{
private:
	/**
	 * The label at the top of the view. This is the "Applying pack \"%s\"..." text or
	 * the "Failed to apply pack \"%s\"." text.
	 */
	HWND _hwndLabel;

	/**
	 * The progress bar window.
	 */
	HWND _hwndProgress;

	/**
	 * The logs text box.
	 */
	HWND _hwndText;

	/**
	 * The "Close" button.
	 */
	HWND _hwndClose;
	
	static constexpr int c_numLayoutWindows = 4;

	const mm_unattend_window_translations_t *_pTranslations;
	
	HFONT _hfMonospace;

	CPack _pack;
	bool _fApplying;

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void _OnCreate();
	void _UpdateMetrics();
	void _UpdateLayout();

	bool _LoadPack();

	static void s_ApplyProgressCallback(void *lpParam, DWORD dwItemsProcessed, DWORD dwTotalItems);
	void _ApplyPack();
	static DWORD CALLBACK s_ApplyPackThreadProc(LPVOID lpParam);
	static void s_LogCallback(void *lpParam, LPCWSTR pszText);
	void _ApplyPackWorker();

public:
	CUnattendWindow();
	~CUnattendWindow();

	static HRESULT RegisterWindowClass();
	static CUnattendWindow *CreateAndShow(int nCmdShow);
};