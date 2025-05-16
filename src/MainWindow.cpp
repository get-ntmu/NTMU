#include "MainWindow.h"
#include "DPIHelpers.h"
#include "Logging.h"
#include "ThumbCache.h"
#include <locale>
#include <codecvt>
#include <pathcch.h>

const WCHAR c_szGitHubURL[] = L"https://github.com/aubymori/NTMU";
const WCHAR c_szHelpURL[] = L"https://github.com/aubymori/NTMU/wiki";
const WCHAR c_szGetPacksURL[] = L"https://aubymori.github.io/NTMU/#!/packs";

INT_PTR CALLBACK CMainWindow::s_AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			WCHAR szFormat[256], szBuffer[256];
			GetDlgItemTextW(hWnd, IDC_VERSION, szFormat, 256);
			swprintf_s(
				szBuffer, 256, szFormat,
				VER_MAJOR, VER_MINOR, VER_REVISION
			);
			SetDlgItemTextW(hWnd, IDC_VERSION, szBuffer);
			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		case WM_COMMAND:
			switch (wParam)
			{
				case IDC_GITHUB:
					ShellExecuteW(
						hWnd, L"open",
						L"https://github.com/aubymori/NTMU",
						NULL, NULL,
						SW_SHOWNORMAL
					);
					// fall-thru
				case IDOK:
				case IDCANCEL:
					EndDialog(hWnd, wParam);
			}
			return TRUE;
		default:
			return FALSE;
	}
}

LRESULT CMainWindow::v_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			_OnCreate();
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_APPLY:
					_ApplyPack();
					break;
				case IDM_FILEOPEN:
				{
					WCHAR szFilePath[MAX_PATH] = { 0 };
					OPENFILENAMEW ofn = { 0 };
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"INI Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0\0";
					ofn.lpstrFile = szFilePath;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrDefExt = L"ini";
					ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
					if (GetOpenFileNameW(&ofn))
					{
						_UnloadPack();
						_LoadPack(szFilePath);
					}
					break;
				}
				case IDM_FILEUNLOAD:
				{
					_UnloadPack();
					break;
				}
				case IDM_FILEEXIT:
					PostMessageW(hWnd, WM_CLOSE, 0, 0);
					break;
				case IDM_TOOLSCLEARICOCACHE:
				{
					WCHAR szExePath[MAX_PATH];
					GetSystemDirectoryW(szExePath, MAX_PATH);
					PathCchAppend(szExePath, MAX_PATH, L"ie4uinit.exe");

					ShellExecuteW(
						NULL, L"open",
						szExePath, L"-show",
						nullptr, SW_SHOWNORMAL
					);

					wil::com_ptr<IEmptyVolumeCache> spEmptyVolCache;
					if (SUCCEEDED(CoCreateInstance(
						CLSID_EmptyVolumeCache, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spEmptyVolCache))))
					{
						wil::unique_hkey hKey;
						LSTATUS status = RegOpenKeyExW(
							HKEY_LOCAL_MACHINE,
							L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Thumbnail Cache",
							NULL,
							KEY_READ,
							&hKey
						);
						status;
						DWORD dwFlags = EVCF_SETTINGSMODE;
						wil::unique_cotaskmem_string spszDisplayName, spszDescription;
						if (SUCCEEDED(
							spEmptyVolCache->Initialize(hKey.get(), L"C:", &spszDisplayName, &spszDescription, &dwFlags)))
						{
							CEmptyVolumeCacheCallBack cb;
							spEmptyVolCache->Purge((DWORDLONG)-1, &cb);
							spEmptyVolCache->Deactivate(&dwFlags);
						}
					}
					// fall-thru
				}
				case IDM_TOOLSKILLEXPLORER:
				{
					DWORD dwExplorerPID = 0;
					HWND hwndShell = FindWindowW(L"Shell_TrayWnd", nullptr);
					if (!hwndShell)
						break;
					GetWindowThreadProcessId(hwndShell, &dwExplorerPID);
					if (!dwExplorerPID)
						break;

					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwExplorerPID);
					if (!hProcess)
						break;

					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);
					break;
				}
				case IDM_TOOLSSYSRESTORE:
				{
					WCHAR szExePath[MAX_PATH];
					GetSystemDirectoryW(szExePath, MAX_PATH);
					PathCchAppend(szExePath, MAX_PATH, L"SystemPropertiesProtection.exe");

					ShellExecuteW(
						NULL, L"open",
						szExePath, nullptr,
						nullptr, SW_SHOWNORMAL
					);
					break;
				}
				case IDM_HELPTOPICS:
				case IDM_HELPGETPACKS:
				{
					LPCWSTR pszURL = (LOWORD(wParam) == IDM_HELPTOPICS) ? c_szHelpURL : c_szGetPacksURL;
					ShellExecuteW(
						NULL, L"open",
						pszURL, nullptr,
						nullptr, SW_SHOWNORMAL
					);
					break;
				}
				case IDM_HELPABOUT:
					DialogBoxParamW(g_hinst, MAKEINTRESOURCEW(IDD_ABOUT), hWnd, s_AboutDlgProc, NULL);
					break;
			}
			return 0;
		// Make the read-only text control use Window background
		// instead of ButtonFace. This looks better with the options
		// pane.
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == _hwndText)
				return (LRESULT)(COLOR_WINDOW + 1);
			goto DWP;
		case WM_SETTINGCHANGE:
			if (wParam != SPI_SETNONCLIENTMETRICS)
				return 0;
			// fallthrough
		case WM_THEMECHANGED:
		case WM_DPICHANGED:
			_UpdateFonts();
			return 0;
		case WM_SIZE:
			_UpdateLayout();
			return 0;
		case WM_NOTIFY:
		{
			UINT uCode = ((LPNMHDR)lParam)->code;
			switch (uCode)
			{
				// Prevent user from collapsing radio options
				case TVN_ITEMEXPANDINGW:
				{
					LPNMTREEVIEWW lpnm = (LPNMTREEVIEWW)lParam;
					if (lpnm->itemNew.state & TVIS_EXPANDED)
						return TRUE;
					break;
				}
				case TVN_KEYDOWN:
					if (((LPNMTVKEYDOWN)lParam)->wVKey != VK_SPACE)
						break;
				// fall-thru
				case NM_CLICK:
				{
					HTREEITEM hItem;
					LPARAM lItemParam;
					if (uCode == NM_CLICK)
					{
						POINT pt;
						GetCursorPos(&pt);
						ScreenToClient(_hwndOptions, &pt);

						TVHITTESTINFO ht = { 0 };
						ht.pt = pt;
						hItem = TreeView_HitTest(_hwndOptions, &ht);
					}
					else
					{
						hItem = TreeView_GetSelection(_hwndOptions);
					}

					if (!hItem)
						return 0;

					TVITEMW tvi = { 0 };
					tvi.mask = TVIF_HANDLE | TVIF_PARAM;
					tvi.hItem = hItem;
					TreeView_GetItem(_hwndOptions, &tvi);
					lItemParam = tvi.lParam;

					HTREEITEM hItemParent = TreeView_GetParent(_hwndOptions, hItem);
					// Radio items
					if (hItemParent)
					{
						// Get parent param (option pointer)
						TVITEMW ptvi = { 0 };
						ptvi.mask = TVIF_HANDLE | TVIF_PARAM;
						ptvi.hItem = hItemParent;
						TreeView_GetItem(_hwndOptions, &ptvi);

						CPack::PackOption *pOpt = (CPack::PackOption *)ptvi.lParam;
						// Skip if user selects current radio
						if (pOpt->uValue == lItemParam)
							break;
						pOpt->uValue = lItemParam;

						// Deselect current item
						HTREEITEM hItemChild = TreeView_GetChild(_hwndOptions, hItemParent);
						do
						{
							ptvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
							ptvi.hItem = hItemChild;
							TreeView_GetItem(_hwndOptions, &ptvi);
							if (ptvi.iImage == OII_RADIO_ON)
							{
								ptvi.iImage = OII_RADIO;
								ptvi.iSelectedImage = OII_RADIO;
								TreeView_SetItem(_hwndOptions, &ptvi);
								break;
							}
						}
						while (hItemChild = TreeView_GetNextSibling(_hwndOptions, hItemChild));

						// Select current item
						ptvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
						ptvi.hItem = hItem;
						ptvi.iImage = OII_RADIO_ON;
						ptvi.iSelectedImage = OII_RADIO_ON;
						TreeView_SetItem(_hwndOptions, &ptvi);
					}
					else
					{
						CPack::PackOption *pOpt = (CPack::PackOption *)lItemParam;
						if (pOpt->radios.size() <= 0)
						{
							UINT uNewVal = (pOpt->uValue == 0);
							pOpt->uValue = uNewVal;
							TVITEMW ntvi = { 0 };
							ntvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
							ntvi.hItem = hItem;
							ntvi.iImage = uNewVal ? OII_CHECK_ON : OII_CHECK;
							ntvi.iSelectedImage = ntvi.iImage;
							TreeView_SetItem(_hwndOptions, &ntvi);
						}
						else break;
					}
				
					return (uCode == TVN_KEYDOWN) ? TRUE : 0;
				}
			}
			goto DWP;
		}
		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_CLOSE && _fApplying)
				return 0;
			goto DWP;
		default:
DWP:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
}

void CMainWindow::_OnCreate()
{
	_UnloadPack();
	_hAccel = LoadAcceleratorsW(g_hinst, MAKEINTRESOURCEW(IDA_MAIN));

	for (int i = 0; i < MI_COUNT; i++)
	{
		WCHAR szLabelText[MAX_PATH] = { 0 };
		LoadStringW(g_hinst, IDS_PACKNAME + i, szLabelText, MAX_PATH);
		_rghwndLabels[i] = CreateWindowExW(
			NULL, L"STATIC", szLabelText, WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0,
			_hwnd, NULL, NULL, NULL
		);
		_rghwndMetas[i] = CreateWindowExW(
			NULL, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0,
			_hwnd, NULL, NULL, NULL
		);
	}

	_hwndProgress = CreateWindowExW(
		NULL, PROGRESS_CLASSW, nullptr,
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH | PBS_SMOOTHREVERSE, 0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);
	SendMessageW(_hwndProgress, PBM_SETSTEP, 1, 0);

	WCHAR szApplyText[MAX_PATH] = { 0 };
	LoadStringW(g_hinst, IDS_APPLY, szApplyText, MAX_PATH);
	_hwndApply = CreateWindowExW(
		NULL, WC_BUTTONW, szApplyText,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		_hwnd, (HMENU)IDC_APPLY, NULL, NULL
	);
	EnableWindow(_hwndApply, FALSE);

	_hwndText = CreateWindowExW(
		WS_EX_CLIENTEDGE, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY,
		0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);

	_hwndOptions = CreateWindowExW(
		WS_EX_CLIENTEDGE, WC_TREEVIEWW, nullptr,
		WS_CHILD | WS_VISIBLE | TVS_FULLROWSELECT,
		0, 0, 0, 0,
		_hwnd, NULL, NULL, NULL
	);
	SetWindowTheme(_hwndOptions, L"Explorer", nullptr);

	// Remove the ugly link cursor from hot tracking
	SetWindowSubclass(_hwndOptions, [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT
	{
		if (uMsg == WM_SETCURSOR)
		{
			static HCURSOR hCursor = LoadCursorW(NULL, IDC_ARROW);
			SetCursor(hCursor);
			return 0;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}, (UINT_PTR)this, NULL);

	CPreviewWindow::RegisterWindowClass();
	_pPreviewWnd = CPreviewWindow::CreateAndShow(_hwnd);
	if (_pPreviewWnd)
		_hwndPreview = _pPreviewWnd->GetHWND();

	_UpdateFonts();
}

void CMainWindow::_UpdateFonts()
{
	_dpi = DPIHelpers::GetWindowDPI(_hwnd);

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

#define UPDATEFONT(hwnd) SendMessageW(hwnd, WM_SETFONT, (WPARAM)_hfMessage, TRUE)
	for (int i = 0; i < MI_COUNT; i++)
	{
		UPDATEFONT(_rghwndLabels[i]);
		UPDATEFONT(_rghwndMetas[i]);
	}
	UPDATEFONT(_hwndApply);
#undef UPDATEFONT

	if (_pPreviewWnd)
		_pPreviewWnd->SetFont(_hfMessage);

	// Update text box font (monospace)
	if (_hfMonospace)
	{
		DeleteObject(_hfMonospace);
		_hfMonospace = NULL;
	}

	LOGFONTW lf = { 0 };
	wcscpy_s(lf.lfFaceName, L"Courier New");
	lf.lfHeight = -MulDiv(10, _dpi, 72);
	_hfMonospace = CreateFontIndirectW(&lf);
	SendMessageW(_hwndText, WM_SETFONT, (WPARAM)_hfMonospace, NULL);

	// Update options style (hot tracking looks bad on classic)
	HTHEME hTheme = OpenThemeData(_hwndOptions, L"TreeView");
	DWORD dwStyle = GetWindowLongPtrW(_hwndOptions, GWL_STYLE);
	if (hTheme)
	{
		dwStyle |= TVS_TRACKSELECT;
		CloseThemeData(hTheme);
	}
	else
	{
		dwStyle &= ~TVS_TRACKSELECT;
	}
	SetWindowLongPtrW(_hwndOptions, GWL_STYLE, dwStyle);

	// Update checkboxes
	if (_himlOptions)
	{
		ImageList_Destroy(_himlOptions);
		_himlOptions = NULL;
	}

	int size = MulDiv(16, _dpi, 96);
	_himlOptions = ImageList_Create(size, size, ILC_COLOR32 | ILC_MASK, OII_COUNT, 1);

	HICON hiconCPL = (HICON)LoadImageW(
		GetModuleHandleW(L"shell32.dll"),
		MAKEINTRESOURCEW(137),
		IMAGE_ICON,
		size, size,
		LR_DEFAULTCOLOR
	);
	ImageList_AddIcon(_himlOptions, hiconCPL);

	static const int s_rgParts[OII_COUNT - 1]
		= { BP_CHECKBOX, BP_CHECKBOX, BP_RADIOBUTTON, BP_RADIOBUTTON };
	static const int s_rgStates[OII_COUNT - 1]
		= { CBS_UNCHECKEDNORMAL, CBS_CHECKEDNORMAL, RBS_UNCHECKEDNORMAL, RBS_CHECKEDNORMAL };

	static const int s_rgClassicStates[OII_COUNT - 1]
		= { DFCS_BUTTONCHECK, DFCS_BUTTONCHECK | DFCS_CHECKED, DFCS_BUTTONRADIO, DFCS_BUTTONRADIO | DFCS_CHECKED };

	hdc = CreateCompatibleDC(NULL);
	if (hdc)
	{
		BITMAPINFO bi = { 0 };
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
		bi.bmiHeader.biWidth = size;
		bi.bmiHeader.biHeight = size;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		HBITMAP hbmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, nullptr, NULL, 0);
		if (hbmp)
		{
			HTHEME hTheme = OpenThemeData(NULL, L"Button");
			RECT rc = { 0, 0, size, size };
			for (int i = OII_CHECK; i < OII_COUNT; i++)
			{
				HBITMAP hOld = (HBITMAP)SelectObject(hdc, hbmp);
				if (hTheme)
				{
					DTBGOPTS dtbg = { sizeof(dtbg), DTBG_DRAWSOLID };
					FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
					DrawThemeBackgroundEx(hTheme, hdc, s_rgParts[i - 1], s_rgStates[i - 1], &rc, &dtbg);
				}
				else
				{
					HBRUSH hbr = CreateSolidBrush(RGB(255, 0, 255));
					FillRect(hdc, &rc, hbr);

					int cSize = 13;
					if (i == OII_RADIO || i == OII_RADIO_ON)
						cSize = 12;
					cSize = MulDiv(cSize, _dpi, 96);
					int pos = (size - cSize) / 2;
					RECT rcControl = {
						pos,
						pos,
						pos + cSize,
						pos + cSize
					};

					DrawFrameControl(hdc, &rcControl, DFC_BUTTON, s_rgClassicStates[i - 1]);
					DeleteObject(hbr);
				}
				SelectObject(hdc, hOld);
				ImageList_AddMasked(_himlOptions, hbmp, RGB(255, 0, 255));
			}

			if (hTheme)
				CloseThemeData(hTheme);

			DeleteObject(hbmp);
		}

		DeleteDC(hdc);
	}

	TreeView_SetImageList(_hwndOptions, _himlOptions, TVSIL_NORMAL);
	TreeView_SetImageList(_hwndOptions, _himlOptions, TVSIL_STATE);

	_UpdateLayout();
}

void CMainWindow::_UpdateLayout()
{
	RECT rcClient;
	GetClientRect(_hwnd, &rcClient);

	const int marginX = _XDUToXPix(6);
	const int marginY = _YDUToYPix(4);

	constexpr int nWindows = (MI_COUNT * 2) + 5;
	HDWP hdwp = BeginDeferWindowPos(nWindows);

	// Position and size labels
	const int labelWidth = _XDUToXPix(30);
	const int labelHeight = _YDUToYPix(8);
	const int labelMargin = _YDUToYPix(2);
	const int metaWidth = RECTWIDTH(rcClient) - labelWidth - (2 * marginX);
	const int metaX = marginX + labelWidth;
	for (int i = 0; i < MI_COUNT; i++)
	{
		hdwp = DeferWindowPos(
			hdwp, _rghwndLabels[i], NULL,
			marginX, marginY + ((labelHeight + labelMargin) * i),
			labelWidth, labelHeight, SWP_NOZORDER
		);
		hdwp = DeferWindowPos(
			hdwp, _rghwndMetas[i], NULL,
			metaX, marginY + ((labelHeight + labelMargin) * i),
			metaWidth, labelHeight, SWP_NOZORDER
		);
	}

	// Position and size progress bar and Apply button
	const int buttonWidth = _XDUToXPix(50);
	const int buttonHeight = _YDUToYPix(14);
	const int buttonX = RECTWIDTH(rcClient) - buttonWidth - marginX;
	const int buttonY = RECTHEIGHT(rcClient) - buttonHeight - marginY;
	hdwp = DeferWindowPos(
		hdwp, _hwndApply, NULL,
		buttonX, buttonY,
		buttonWidth, buttonHeight,
		SWP_NOZORDER
	);
	hdwp = DeferWindowPos(
		hdwp, _hwndProgress, NULL,
		marginX, buttonY,
		buttonX - (marginX * 2),
		buttonHeight,
		SWP_NOZORDER
	);
	
	// Position and size panes
	const int panesY = (marginY * 2) + (labelHeight * MI_COUNT) + (labelMargin * (MI_COUNT - 1));
	const int paneWidth = (RECTWIDTH(rcClient) - (marginX * 2)) / 2 - (marginX / 2);
	const int panesHeight = buttonY - marginY - panesY;
	hdwp = DeferWindowPos(
		hdwp, _hwndText, NULL,
		marginX, panesY,
		paneWidth, panesHeight,
		SWP_NOZORDER
	);

	const int rightPaneX = (marginX * 2) + paneWidth;
	const int rightPaneHeight = (panesHeight - marginY) / 2;
	hdwp = DeferWindowPos(
		hdwp, _hwndOptions, NULL,
		rightPaneX, panesY + rightPaneHeight + marginY,
		paneWidth, rightPaneHeight,
		SWP_NOZORDER
	);
	hdwp = DeferWindowPos(
		hdwp, _hwndPreview, NULL,
		rightPaneX, panesY,
		paneWidth, rightPaneHeight,
		SWP_NOZORDER
	);
	InvalidateRect(_hwndPreview, nullptr, TRUE);

	EndDeferWindowPos(hdwp);
}

void CMainWindow::_LoadPack(LPCWSTR pszPath)
{
	if (!_pack.Load(pszPath))
		return;

	SetWindowTextW(_rghwndMetas[MI_NAME], _pack.GetName().c_str());
	SetWindowTextW(_rghwndMetas[MI_AUTHOR], _pack.GetAuthor().c_str());
	SetWindowTextW(_rghwndMetas[MI_VERSION], _pack.GetVersion().c_str());

	EnableWindow(_hwndApply, TRUE);

	if (_pPreviewWnd)
		_pPreviewWnd->SetImage(_pack.GetPreviewPath().c_str());

	_LoadReadme();

	const auto &options = _pack.GetOptions();
	for (const auto &opt : options)
	{
		TVINSERTSTRUCTW tvis = { 0 };
		TVITEMW &tvi = tvis.item;
		tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
		tvi.pszText = (LPWSTR)opt.name.c_str();
		tvi.cchTextMax = wcslen(tvi.pszText) + 1;
		if (opt.radios.size() > 0)
			tvi.iImage = OII_CPL;
		else
			tvi.iImage = opt.uValue ? OII_CHECK_ON : OII_CHECK;
		tvi.iSelectedImage = tvi.iImage;
		tvi.lParam = (LPARAM)&opt;
		HTREEITEM hItem = TreeView_InsertItem(_hwndOptions, &tvis);
		tvis.hParent = hItem;
		for (const auto &radio : opt.radios)
		{
			tvi.pszText = (LPWSTR)radio.name.c_str();
			tvi.cchTextMax = wcslen(tvi.pszText) + 1;
			tvi.iImage = (opt.uValue == radio.uValue) ? OII_RADIO_ON : OII_RADIO;
			tvi.iSelectedImage = tvi.iImage;
			tvi.lParam = (LPARAM)radio.uValue;
			TreeView_InsertItem(_hwndOptions, &tvis);
			// Have to do this after every subitem or else it only expands one.
			// Great job, MS.
			TreeView_Expand(_hwndOptions, hItem, TVE_EXPAND);
		}	
	}
}

void CMainWindow::_UnloadPack()
{
	_pack.Reset();

	for (int i = 0; i < MI_COUNT; i++)
	{
		SetWindowTextW(_rghwndMetas[i], L"");
	}

	SetWindowTextW(_hwndText, L"");

	EnableWindow(_hwndApply, FALSE);

	if (_pPreviewWnd)
		_pPreviewWnd->SetImage(nullptr);

	TreeView_DeleteAllItems(_hwndOptions);
}

void CMainWindow::_LoadReadme()
{
	std::wstring readmePath = _pack.GetReadmePath();
	if (!readmePath.empty())
	{
		std::ifstream readmeFile(readmePath);
		std::string buffer(
			(std::istreambuf_iterator<char>(readmeFile)),
			std::istreambuf_iterator<char>()
		);

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring readmeText = converter.from_bytes(buffer);

		// Standardize line endings
		size_t pos = 0;
		while ((pos = readmeText.find(L'\n', pos)) != std::wstring::npos)
		{
			readmeText.replace(pos, 1, L"\r\n");
			pos += 2;
		}

		SetWindowTextW(_hwndText, readmeText.c_str());
		return;
	}
	SetWindowTextW(_hwndText, L"");
}

// static
void CMainWindow::s_ApplyProgressCallback(void *lpParam, DWORD dwItemsProcessed, DWORD dwTotalItems)
{
	CMainWindow *pThis = (CMainWindow *)lpParam;
	PostMessageW(pThis->_hwndProgress, PBM_DELTAPOS, 100 / dwTotalItems, 0);
}

void CMainWindow::_ApplyPack()
{
	int result = MainWndMsgBox(
		L"Applying a pack can make changes to system files and registry entries. "
		L"If the pack you have selected does so, it is recommended that you:\n\n"
		L"- Create a restore point (Tools -> Manage System Restore...)\n"
		L"- Disable Windows Update (can be done through external tools)\n\n"
		L"Do you wish to proceed?",
		MB_ICONWARNING | MB_YESNO
	);
	if (result == IDNO)
		return;

	CreateThread(nullptr, 0, s_ApplyPackThreadProc, this, NULL, nullptr);
}

// static
DWORD CALLBACK CMainWindow::s_ApplyPackThreadProc(LPVOID lpParam)
{
	((CMainWindow *)lpParam)->_ApplyPackWorker();
	ExitThread(0);
	return 0;
}

// static
void CMainWindow::s_LogCallback(void *lpParam, LPCWSTR pszText)
{
	CMainWindow *pThis = (CMainWindow *)lpParam;

	HWND hwnd = pThis->_hwndText;
	size_t length = GetWindowTextLengthW(hwnd) + 1;
	LPWSTR pszBuffer = new WCHAR[length];
	GetWindowTextW(hwnd, pszBuffer, length);
	std::wstring newText = pszBuffer;
	delete[] pszBuffer;
	newText += pszText;
	newText += L"\r\n";
	SetWindowTextW(hwnd, newText.c_str());
}

void CMainWindow::_ApplyPackWorker()
{
	HMENU hmenu = GetMenu(_hwnd);
	HMENU hmenuSystem = GetSystemMenu(_hwnd, FALSE);

	SetWindowTextW(_hwndText, L"");

	EnableMenuItem(hmenu, IDM_FILEEXIT, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
	EnableMenuItem(hmenuSystem, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
	EnableWindow(_hwndApply, FALSE);
	EnableWindow(_hwndOptions, FALSE);

	DWORD dwCallbackID;
	AddLogCallback(s_LogCallback, this, &dwCallbackID);

	_fApplying = true;

	if (_pack.Apply(this, s_ApplyProgressCallback))
	{
		MainWndMsgBox(L"The pack was applied successfully.", MB_ICONINFORMATION);
		_LoadReadme();
	}
	else
	{
		SendMessageW(_hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);
		MainWndMsgBox(L"Failed to apply the selected pack. See the log for more details.", MB_ICONERROR);
	}

	_fApplying = false;

	RemoveLogCallback(dwCallbackID);

	EnableMenuItem(hmenu, IDM_FILEEXIT, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hmenuSystem, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	EnableWindow(_hwndApply, TRUE);
	EnableWindow(_hwndOptions, TRUE);

	SendMessageW(_hwndProgress, PBM_SETSTATE, PBST_NORMAL, 0);
	SendMessageW(_hwndProgress, PBM_SETPOS, 0, 0);
}

CMainWindow::CMainWindow()
	: _dpi(0)
	, _hAccel(NULL)
	, _rghwndLabels{ NULL }
	, _rghwndMetas{ NULL }
	, _hwndProgress(NULL)
	, _hwndApply(NULL)
	, _hwndText(NULL)
	, _hwndOptions(NULL)
	, _hfMessage(NULL)
	, _hfMonospace(NULL)
	, _cxMsgFontChar(0)
	, _cyMsgFontChar(0)
{
}

CMainWindow::~CMainWindow()
{	
	if (_hfMessage)
		DeleteObject(_hfMessage);
	if (_hfMonospace)
		DeleteObject(_hfMonospace);
}

// static
HRESULT CMainWindow::RegisterWindowClass()
{
	WNDCLASSW wc = { 0 };
	wc.hInstance = g_hinst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	wc.hIcon = LoadIconW(g_hinst, MAKEINTRESOURCEW(IDI_NTMU));
	wc.lpszMenuName = MAKEINTRESOURCEW(IDM_MAINMENU);
	return CWindow::RegisterWindowClass(&wc);
}

static inline void ScreenCenteredRect(
	int cx,
	int cy,
	DWORD dwStyle,
	DWORD dwExStyle,
	bool fMenu,
	LPRECT lprc
)
{
	UINT uDpi = DPIHelpers::GetSystemDPI();
	RECT rc = { 0, 0, cx, cy };
	DPIHelpers::AdjustWindowRectForDPI(&rc, dwStyle, dwExStyle, fMenu, uDpi);
	cx = RECTWIDTH(rc);
	cy = RECTHEIGHT(rc);
	int scx = GetSystemMetrics(SM_CXSCREEN);
	int scy = GetSystemMetrics(SM_CYSCREEN);
	int x = (scx - cx) / 2;
	int y = (scy - cy) / 2;
	lprc->left = x;
	lprc->top = y;
	lprc->right = x + cx;
	lprc->bottom = y + cy;
}

// static
CMainWindow *CMainWindow::CreateAndShow(int nCmdShow)
{
	constexpr DWORD c_dwMainWindowStyle = WS_OVERLAPPEDWINDOW;
	constexpr DWORD c_dwMainWindowExStyle = NULL;

	RECT rc;
	ScreenCenteredRect(575, 500, c_dwMainWindowStyle, c_dwMainWindowExStyle, true, &rc);

	CMainWindow *pWindow = Create(
		c_dwMainWindowExStyle,
		L"Windows NT Modding Utility",
		c_dwMainWindowStyle,
		rc.left, rc.top,
		RECTWIDTH(rc), RECTHEIGHT(rc),
		NULL, NULL,
		g_hinst,
		nullptr
	);

	if (!pWindow)
		return nullptr;

	ShowWindow(pWindow->_hwnd, nCmdShow);
	return pWindow;
}

HACCEL CMainWindow::GetAccel()
{
	return _hAccel;
}