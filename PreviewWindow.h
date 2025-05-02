#pragma once
#include "Window.h"

static constexpr WCHAR c_szPreviewWindowClass[] = L"NTMU_Preview";

class CPreviewWindow : public CWindow<CPreviewWindow, c_szPreviewWindowClass>
{
private:
	WCHAR _szNoPreview[MAX_PATH];
	HFONT _hfMessage;

	LRESULT v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	void _OnCreate();

public:
	static HRESULT RegisterWindowClass();
	static CPreviewWindow *CreateAndShow(HWND hwndParent);

	CPreviewWindow();

	void SetFont(HFONT hFont);
};