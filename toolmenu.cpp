/*=============================================================================
*
*								ツールバーとメニュー
*
===============================================================================
/ Copyright (C) 1997-2007 Sota. All rights reserved.
/
/ Redistribution and use in source and binary forms, with or without 
/ modification, are permitted provided that the following conditions 
/ are met:
/
/  1. Redistributions of source code must retain the above copyright 
/     notice, this list of conditions and the following disclaimer.
/  2. Redistributions in binary form must reproduce the above copyright 
/     notice, this list of conditions and the following disclaimer in the 
/     documentation and/or other materials provided with the distribution.
/
/ THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
/ IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
/ OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
/ IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
/ INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
/ BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
/ USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
/ ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
/ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
/ THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/============================================================================*/

#include "common.h"

extern int SepaWidth;
extern int RemoteWidth;

extern int CancelFlg;

/* 設定値 */
extern int DotFile;
extern char AsciiExt[ASCII_EXT_LEN+1];
extern int TransMode;
extern int ListType;
extern int LocalWidth;
extern char ViewerName[VIEWERS][FMAX_PATH+1];
extern int TransMode;
extern int SortSave;
// UTF-8対応
extern int LocalKanjiCode;

/*===== ローカルなワーク =====*/

static HWND hWndTbarMain = NULL;
static HWND hWndTbarLocal = NULL;
static HWND hWndTbarRemote = NULL;
static HWND hWndDirLocal = NULL;
static HWND hWndDirRemote = NULL;
static HWND hWndDirLocalEdit = NULL;
static HWND hWndDirRemoteEdit = NULL;

static int TmpTransMode;
static int TmpHostKanjiCode;
static int TmpHostKanaCnv;

// TODO: ローカルの漢字コードをShift_JIS以外にも対応
static int TmpLocalKanjiCode;

static int TmpLocalFileSort;
static int TmpLocalDirSort;
static int TmpRemoteFileSort;
static int TmpRemoteDirSort;

static int SyncMove = NO;

// デッドロック対策
//static int HideUI = NO;
static int HideUI = 0;


/* 以前、コンボボックスにカレントフォルダを憶えさせていた流れで */
/* このファイルでカレントフォルダを憶えさせる */
static char LocalCurDir[FMAX_PATH+1];
static char RemoteCurDir[FMAX_PATH+1];


/* メインのツールバー */
static TBBUTTON TbarDataMain[] = {
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 0,  MENU_CONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 16, MENU_QUICK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 1,  MENU_DISCONNECT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 2,  MENU_DOWNLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 3,  MENU_UPLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 24, MENU_MIRROR_UPLOAD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 4,  MENU_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 5,  MENU_RENAME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 6,  MENU_MKDIR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 7,  MENU_TEXT, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 8,  MENU_BINARY, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 17, MENU_AUTO, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 27, MENU_L_KNJ_SJIS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 20, MENU_L_KNJ_EUC, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 21, MENU_L_KNJ_JIS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 28, MENU_L_KNJ_UTF8N, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 29, MENU_L_KNJ_UTF8BOM, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 27, MENU_KNJ_SJIS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 20, MENU_KNJ_EUC, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 21, MENU_KNJ_JIS, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 28, MENU_KNJ_UTF8N, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 29, MENU_KNJ_UTF8BOM, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 22, MENU_KNJ_NONE, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 23, MENU_KANACNV, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 15, MENU_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 18, MENU_LIST, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 19, MENU_REPORT, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 25, MENU_SYNC, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 },
	{ 0,  0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 26, MENU_ABORT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
};

/* ローカル側のツールバー */
static TBBUTTON TbarDataLocal[] = {
	{ 0, 0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 0, MENU_LOCAL_UPDIR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 1, MENU_LOCAL_CHDIR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0, 0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 }
};

/* ホスト側のツールバー */
static TBBUTTON TbarDataRemote[] = {
	{ 0, 0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 },
	{ 0, MENU_REMOTE_UPDIR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 1, MENU_REMOTE_CHDIR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
	{ 0, 0, TBSTATE_ENABLED, BTNS_SEP, 0, 0 }
};

/* 全ボタン／メニュー項目 */
static const int HideMenus[] = {
	MENU_CONNECT,		MENU_QUICK,			MENU_DISCONNECT,
	MENU_SET_CONNECT,	MENU_IMPORT_WS,		MENU_EXIT,
	MENU_DOWNLOAD,		MENU_UPLOAD,		MENU_DOWNLOAD_AS,	MENU_MIRROR_UPLOAD,
	MENU_UPLOAD_AS,		MENU_DOWNLOAD_NAME,	MENU_MIRROR_UPLOAD,
	MENU_FILESIZE,		MENU_DELETE,		MENU_RENAME,
	MENU_CHMOD,			MENU_MKDIR,			MENU_SOMECMD,
	MENU_SYNC,
	MENU_BMARK_ADD,		MENU_BMARK_ADD_LOCAL, MENU_BMARK_ADD_BOTH,
	MENU_BMARK_EDIT,
	MENU_FILTER,		MENU_FIND,			MENU_FINDNEXT,		MENU_SELECT,
	MENU_SELECT_ALL,	MENU_LIST,			MENU_REPORT,
	MENU_SORT,			MENU_DOTFILE,
	MENU_DIRINFO,		MENU_TASKINFO,		MENU_REFRESH,
	MENU_OPTION,
	MENU_OTPCALC,
	MENU_HELP,			MENU_HELP_TROUBLE,	MENU_ABOUT,
	MENU_REGINIT,
	MENU_TEXT,			MENU_BINARY,		MENU_AUTO,
	MENU_KNJ_SJIS,		MENU_KNJ_EUC,		MENU_KNJ_JIS,		MENU_KNJ_UTF8N,		MENU_KNJ_UTF8BOM,	MENU_KNJ_NONE,
	MENU_L_KNJ_SJIS,	MENU_L_KNJ_EUC,		MENU_L_KNJ_JIS,		MENU_L_KNJ_UTF8N,	MENU_L_KNJ_UTF8BOM,
	MENU_KANACNV,
	MENU_LOCAL_UPDIR,	MENU_LOCAL_CHDIR,
	MENU_REMOTE_UPDIR,	MENU_REMOTE_CHDIR,
	MENU_HIST_1,		MENU_HIST_2,		MENU_HIST_3,		MENU_HIST_4,
	MENU_HIST_5,		MENU_HIST_6,		MENU_HIST_7,		MENU_HIST_8,
	MENU_HIST_9,		MENU_HIST_10,		MENU_HIST_11,		MENU_HIST_12,
	MENU_HIST_13,		MENU_HIST_14,		MENU_HIST_15,		MENU_HIST_16,
	MENU_HIST_17,		MENU_HIST_18,		MENU_HIST_19,		MENU_HIST_20
};


static HBITMAP GetImage(int iamgeId) {
	HBITMAP resized = 0;
	if (auto original = (HBITMAP)LoadImageW(GetFtpInst(), MAKEINTRESOURCEW(iamgeId), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS)) {
		if (auto dc = GetDC(0)) {
			if (auto src = CreateCompatibleDC(dc)) {
				if (auto dest = CreateCompatibleDC(dc)) {
					if (BITMAP bitmap; GetObjectW(original, sizeof(BITMAP), &bitmap) > 0) {
						auto width = bitmap.bmWidth / 64 * CalcPixelX(16);
						auto height = bitmap.bmHeight / 64 * CalcPixelY(16);
						if (resized = CreateCompatibleBitmap(dc, width, height)) {
							auto hSrcOld = SelectObject(src, original);
							auto hDstOld = SelectObject(dest, resized);
							SetStretchBltMode(dest, COLORONCOLOR);
							StretchBlt(dest, 0, 0, width, height, src, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
							SelectObject(src, hSrcOld);
							SelectObject(dest, hDstOld);
						}
					}
					DeleteDC(dest);
				}
				DeleteDC(src);
			}
			ReleaseDC(0, dc);
		}
		DeleteObject(original);
	}
	return resized;
}


static LRESULT CALLBACK IgnoreRightClick(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
	switch (uMsg) {
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		return TRUE;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK HistoryEdit(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR isLocal) {
	switch (uMsg) {
	case WM_CHAR:
		switch (wParam) {
		case 0x0D:			/* リターンキーが押された */
			if (isLocal) {
				DoLocalCWD(u8(GetText(hWnd)).c_str());
				GetLocalDirForWnd();
			} else {
				CancelFlg = NO;
				if (CheckClosedAndReconnect() == FFFTP_SUCCESS && DoCWD(u8(GetText(hWnd)).c_str(), YES, NO, YES) < FTP_RETRY)
					GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			}
			return 0;
		case 0x09:			/* TABキーが押された */
			SetFocus(isLocal ? GetLocalHwnd() : GetRemoteHwnd());
			return 0;
		}
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


static auto CreateToolbar(DWORD ws, UINT id, int bitmaps, HBITMAP image, const TBBUTTON* buttons, int size, int x, int y, int width, int height) {
	ws |= WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT;
	auto toolbar = CreateToolbarEx(GetMainHwnd(), ws, id, bitmaps, 0, (UINT_PTR)image, buttons, size, CalcPixelX(16), CalcPixelY(16), CalcPixelX(16), CalcPixelY(16), sizeof(TBBUTTON));
	if (toolbar) {
		SetWindowSubclass(toolbar, IgnoreRightClick, 0, 0);
		MoveWindow(toolbar, x, y, width, height, false);
	}
	return toolbar;
}


static std::tuple<HWND, HWND> CreateComboBox(HWND toolbar, DWORD style, int width, int menuId, bool isLocal, HFONT font) {
	style |= WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL;
	RECT rect;
	SendMessageW(toolbar, TB_GETITEMRECT, 3, (LPARAM)&rect);
	auto combobox = CreateWindowExW(WS_EX_CLIENTEDGE, WC_COMBOBOXW, nullptr, style, rect.right, rect.top, width - rect.right, CalcPixelY(200), toolbar, (HMENU)IntToPtr(menuId), GetFtpInst(), nullptr);
	HWND edit = 0;
	if (combobox) {
		if (COMBOBOXINFO ci{ sizeof(COMBOBOXINFO) }; SendMessageW(combobox, CB_GETCOMBOBOXINFO, 0, (LPARAM)&ci)) {
			__assume(ci.hwndItem);
			edit = ci.hwndItem;
			SetWindowSubclass(edit, HistoryEdit, 0, isLocal);
		}
		SendMessageW(combobox, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
		SendMessageW(combobox, CB_LIMITTEXT, FMAX_PATH, 0);
	}
	return { combobox, edit };
}


// ツールバーを作成する
bool MakeToolBarWindow() {
	auto mainImage = GetImage(main_toolbar_bmp);
	if (!mainImage)
		return false;
	auto remoteImage = GetImage(remote_toolbar_bmp);
	if (!mainImage)
		return false;
	RECT rect;
	GetClientRect(GetMainHwnd(), &rect);

	// main toolbar
	if (hWndTbarMain = CreateToolbar(BTNS_SEP, 1, 30, mainImage, TbarDataMain, size_as<int>(TbarDataMain), 0, 0, rect.right, AskToolWinHeight()); !hWndTbarMain)
		return false;

	// local toobar
	if (hWndTbarLocal = CreateToolbar(BTNS_GROUP, 2, 2, remoteImage, TbarDataLocal, size_as<int>(TbarDataLocal), 0, AskToolWinHeight(), LocalWidth, AskToolWinHeight()); !hWndTbarLocal)
		return false;
	SendMessageW(hWndTbarLocal, TB_GETITEMRECT, 3, (LPARAM)&rect);
	auto font = CreateFontW(rect.bottom - rect.top - CalcPixelY(8), 0, 0, 0, 0, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"MS Shell Dlg");
	std::tie(hWndDirLocal, hWndDirLocalEdit) = CreateComboBox(hWndTbarLocal, CBS_SORT, LocalWidth, COMBO_LOCAL, true, font);
	if (hWndDirLocal == NULL)
		return false;
	GetDrives([](const wchar_t drive[]) { SetLocalDirHist(u8(drive).c_str()); });
	SendMessageW(hWndDirLocal, CB_SETCURSEL, 0, 0);

	// remote toolbar
	if (hWndTbarRemote = CreateToolbar(BTNS_GROUP, 3, 2, remoteImage, TbarDataRemote, size_as<int>(TbarDataRemote), LocalWidth + SepaWidth, AskToolWinHeight(), RemoteWidth, AskToolWinHeight()); !hWndTbarRemote)
		return false;
	std::tie(hWndDirRemote, hWndDirRemoteEdit) = CreateComboBox(hWndTbarRemote, 0, RemoteWidth, COMBO_REMOTE, false, font);
	if (hWndDirRemote == NULL)
		return false;
	SendMessageW(hWndDirRemote, CB_SETCURSEL, 0, 0);

	return true;
}


/*----- ツールバーを削除 ------------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteToolBarWindow(void)
{
	if(hWndTbarMain != NULL)
		DestroyWindow(hWndTbarMain);
	if(hWndTbarLocal != NULL)
		DestroyWindow(hWndTbarLocal);
	if(hWndTbarRemote != NULL)
		DestroyWindow(hWndTbarRemote);
	if(hWndDirLocal != NULL)
		DestroyWindow(hWndDirLocal);
	if(hWndDirRemote != NULL)
		DestroyWindow(hWndDirRemote);
	return;
}


/*----- メインのツールバーのウインドウハンドルを返す --------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetMainTbarWnd(void)
{
	return(hWndTbarMain);
}


/*----- ローカル側のヒストリウインドウのウインドウハンドルを返す --------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetLocalHistHwnd(void)
{
	return(hWndDirLocal);
}


/*----- ホスト側のヒストリウインドウのウインドウハンドルを返す ----------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetRemoteHistHwnd(void)
{
	return(hWndDirRemote);
}


/*----- ローカル側のヒストリエディットのウインドウハンドルを返す --------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetLocalHistEditHwnd(void)
{
	return(hWndDirLocalEdit);
}


/*----- ホスト側のヒストリエディットのウインドウハンドルを返す ----------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetRemoteHistEditHwnd(void)
{
	return(hWndDirRemoteEdit);
}


/*----- ローカル側のツールバーのウインドウハンドルを返す ----------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetLocalTbarWnd(void)
{
	return(hWndTbarLocal);
}


/*----- ホスト側のツールバーのウインドウハンドルを返す ------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetRemoteTbarWnd(void)
{
	return(hWndTbarRemote);
}


/*----- HideUI の状態を返す ---------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int HideUI の状態
*----------------------------------------------------------------------------*/

int GetHideUI(void)
{
	// デッドロック対策
//	return(HideUI);
	return (HideUI > 0 ? YES : NO);
}


/*----- ツールボタン／メニューのハイド処理 ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MakeButtonsFocus(void)
{
	HWND hWndFocus;
	HWND hWndMain;
	int Count;
	int Win;

	// デッドロック対策
//	if(HideUI == NO)
	if(HideUI == 0)
	{
		hWndMain = GetMainHwnd();
		hWndFocus = GetFocus();
		Win = WIN_LOCAL;
		if(hWndFocus == GetRemoteHwnd())
			Win = WIN_REMOTE;

		Count = GetSelectedCount(Win);

		if(AskConnecting() == YES)
		{
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_ADD, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_ADD_LOCAL, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_ADD_BOTH, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_EDIT, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DIRINFO, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_MIRROR_UPLOAD, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_MIRROR_DOWNLOAD, MF_ENABLED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_NAME, MF_ENABLED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_MIRROR_UPLOAD, MAKELONG(TRUE, 0));
		}
		else
		{
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_ADD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_ADD_LOCAL, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_ADD_BOTH, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_BMARK_EDIT, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DIRINFO, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_MIRROR_UPLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_MIRROR_DOWNLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_NAME, MF_GRAYED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_MIRROR_UPLOAD, MAKELONG(FALSE, 0));
		}

		if(hWndFocus == GetLocalHwnd())
		{
			if((AskConnecting() == YES) && (Count > 0))
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_ENABLED);
			}
			else
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_GRAYED);
			}
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_SOMECMD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_GRAYED);
		}

		if(hWndFocus == GetRemoteHwnd())
		{
			if(AskConnecting() == YES)
			{
				EnableMenuItem(GetMenu(hWndMain), MENU_SOMECMD, MF_ENABLED);
			}
			else
			{
				EnableMenuItem(GetMenu(hWndMain), MENU_SOMECMD, MF_GRAYED);
			}

			if((AskConnecting() == YES) && (Count > 0))
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_ENABLED);
			}
			else
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_GRAYED);
			}
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_GRAYED);
		}

		if((hWndFocus == GetLocalHwnd()) || (hWndFocus == GetRemoteHwnd()))
		{
			if(Count > 0)
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DELETE, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DELETE, MF_ENABLED);
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_RENAME, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_RENAME, MF_ENABLED);

				EnableMenuItem(GetMenu(hWndMain), MENU_CHMOD, MF_ENABLED);

			}
			else
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DELETE, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DELETE, MF_GRAYED);
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_RENAME, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_RENAME, MF_GRAYED);

				EnableMenuItem(GetMenu(hWndMain), MENU_CHMOD, MF_GRAYED);
			}

			if((hWndFocus == GetLocalHwnd()) || (AskConnecting() == YES))
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_MKDIR, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_MKDIR, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_FILESIZE, MF_ENABLED);
			}
			else
			{
				SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_MKDIR, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_MKDIR, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_FILESIZE, MF_GRAYED);
			}
		}
		else
		{
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_GRAYED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_GRAYED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_DELETE, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_DELETE, MF_GRAYED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_RENAME, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_RENAME, MF_GRAYED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_MKDIR, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_MKDIR, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_CHMOD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_FILESIZE, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_SOMECMD, MF_GRAYED);
		}
	}
	return;
}


/*----- ユーザの操作を禁止する ------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DisableUserOpe(void)
{
	int i;

	// デッドロック対策
//	HideUI = YES;
	HideUI++;

	for(i = 0; i < sizeof(HideMenus) / sizeof(int); i++)
	{
		EnableMenuItem(GetMenu(GetMainHwnd()), HideMenus[i], MF_GRAYED);
		SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(FALSE, 0));
		SendMessageW(hWndTbarLocal, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(FALSE, 0));
		SendMessageW(hWndTbarRemote, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(FALSE, 0));
	}

	EnableWindow(hWndDirLocal, FALSE);
	EnableWindow(hWndDirRemote, FALSE);

	return;
}


/*----- ユーザの操作を許可する ------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void EnableUserOpe(void)
{
	int i;

	// デッドロック対策
//	if(HideUI == YES)
	if(HideUI > 0)
		HideUI--;
	if(HideUI == 0)
	{
		for(i = 0; i < sizeof(HideMenus) / sizeof(int); i++)
		{
			EnableMenuItem(GetMenu(GetMainHwnd()), HideMenus[i], MF_ENABLED);
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarLocal, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarRemote, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(TRUE, 0));
		}
		EnableWindow(hWndDirLocal, TRUE);
		EnableWindow(hWndDirRemote, TRUE);

		// 選択不可な漢字コードのボタンが表示されるバグを修正
		HideHostKanjiButton();
		HideLocalKanjiButton();

		// バグ修正
//		HideUI = NO;

		// バグ修正
		SetFocus(NULL);
		SetFocus(GetMainHwnd());

		MakeButtonsFocus();
	}
	return;
}


/*----- ユーザの操作が禁止されているかどうかを返す ----------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス
*			YES=禁止されている/NO
*----------------------------------------------------------------------------*/

int AskUserOpeDisabled(void)
{
	// デッドロック対策
//	return(HideUI);
	return (HideUI > 0 ? YES : NO);
}


/*===================================================
*			転送モード
*===================================================*/

/*----- 転送モードを設定する --------------------------------------------------
*
*	Parameter
*		int Mode : 転送モード (TYPE_xx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetTransferTypeImm(int Mode)
{
	TmpTransMode = Mode;
	HideHostKanjiButton();
	HideLocalKanjiButton();
	return;
}


/*----- メニューにより転送モードを設定する ------------------------------------
*
*	Parameter
*		int Type : 転送モード (MENU_xxxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetTransferType(int Type)
{
	switch(Type)
	{
		case MENU_TEXT :
			TmpTransMode = TYPE_A;
			break;

		case MENU_BINARY :
			TmpTransMode = TYPE_I;
			break;

		default :
			TmpTransMode = TYPE_X;
			break;
	}
	HideHostKanjiButton();
	HideLocalKanjiButton();
	return;
}


/*----- 転送モードにしたがってボタンを表示する --------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispTransferType(void)
{
	switch(TmpTransMode)
	{
		case TYPE_A :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_TEXT, MAKELONG(TRUE, 0));
			break;

		case TYPE_I :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_BINARY, MAKELONG(TRUE, 0));
			break;

		default :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_AUTO, MAKELONG(TRUE, 0));
			break;
	}
	return;
}


/*----- 設定上の転送モードを返す ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 転送モード (TYPE_xx)
*----------------------------------------------------------------------------*/

int AskTransferType(void)
{
	return(TmpTransMode);
}


// 実際の転送モードを返す
int AskTransferTypeAssoc(char* Fname, int Type) {
	if (Type != TYPE_X)
		return Type;
	if (0 < StrMultiLen(AsciiExt)) {
		auto wName = u8(GetFileName(Fname));
		for (char* Pos = AsciiExt; *Pos != NUL; Pos += strlen(Pos) + 1)
			if (CheckFname(wName, u8(Pos)))
				return TYPE_A;
	}
	return TYPE_I;
}


/*----- 転送モードを保存する --------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*
*	Note
*		現在の転送モードがレジストリに保存される
*----------------------------------------------------------------------------*/

void SaveTransferType(void)
{
	TransMode = TmpTransMode;
	return;
}


/*===================================================
*			漢字モード
*===================================================*/

/*----- ホストの漢字モードをセットする ----------------------------------------
*
*	Parameter
*		int Mode : 漢字モード (KANJI_xxxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetHostKanjiCodeImm(int Mode)
{
	TmpHostKanjiCode = Mode;
	DispHostKanjiCode();
	HideHostKanjiButton();
	return;
}


/*----- メニューによりホストの漢字モードを設定する -----------------------------
*
*	Parameter
*		int Type : 漢字モード (MENU_xxxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetHostKanjiCode(int Type)
{
	switch(Type)
	{
		// UTF-8対応
		case MENU_KNJ_SJIS :
			TmpHostKanjiCode = KANJI_SJIS;
			break;

		case MENU_KNJ_EUC :
			TmpHostKanjiCode = KANJI_EUC;
			break;

		case MENU_KNJ_JIS :
			TmpHostKanjiCode = KANJI_JIS;
			break;

		case MENU_KNJ_UTF8N :
			TmpHostKanjiCode = KANJI_UTF8N;
			break;

		case MENU_KNJ_UTF8BOM :
			TmpHostKanjiCode = KANJI_UTF8BOM;
			break;

		default :
			TmpHostKanjiCode = KANJI_NOCNV;
			break;
	}
	DispHostKanjiCode();
	HideHostKanjiButton();
	return;
}


/*----- ホストの漢字モードにしたがってボタンを表示する ------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispHostKanjiCode(void)
{
	switch(TmpHostKanjiCode)
	{
		// UTF-8対応
		case KANJI_SJIS :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_SJIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_EUC :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_EUC, MAKELONG(TRUE, 0));
			break;

		case KANJI_JIS :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_JIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8N :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_UTF8N, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8BOM :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			break;

		default :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_NONE, MAKELONG(TRUE, 0));
			break;
	}
	return;
}


/*----- ホストの漢字モードを返す ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 漢字モード (KANJI_xxxx)
*----------------------------------------------------------------------------*/

int AskHostKanjiCode(void)
{
	return(TmpHostKanjiCode);
}


/*----- 漢字モードボタンのハイド処理を行う ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void HideHostKanjiButton(void)
{
	switch(TmpTransMode)
	{
		// UTF-8対応
		case TYPE_I : 
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_SJIS, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_EUC, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_JIS, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8N, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8BOM, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_NONE, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
			break;

		default :
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_SJIS, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_EUC, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_JIS, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8N, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_NONE, MAKELONG(TRUE, 0));
			// 現在カナ変換はShift_JIS、JIS、EUC間でのみ機能する
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
			switch(TmpHostKanjiCode)
			{
			case KANJI_SJIS:
			case KANJI_JIS:
			case KANJI_EUC:
				switch(TmpLocalKanjiCode)
				{
				case KANJI_SJIS:
				case KANJI_JIS:
				case KANJI_EUC:
					SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
					break;
				}
				break;
			}
	}
	return;
}


// ローカルの漢字コード
// テキストモード転送時に使用
// ホスト側が無変換の時はローカルも無変換

void SetLocalKanjiCodeImm(int Mode)
{
	TmpLocalKanjiCode = Mode;
	DispLocalKanjiCode();
	HideLocalKanjiButton();
	return;
}

void SetLocalKanjiCode(int Type)
{
	switch(Type)
	{
		// UTF-8対応
		case MENU_L_KNJ_SJIS :
			TmpLocalKanjiCode = KANJI_SJIS;
			break;

		case MENU_L_KNJ_EUC :
			TmpLocalKanjiCode = KANJI_EUC;
			break;

		case MENU_L_KNJ_JIS :
			TmpLocalKanjiCode = KANJI_JIS;
			break;

		case MENU_L_KNJ_UTF8N :
			TmpLocalKanjiCode = KANJI_UTF8N;
			break;

		case MENU_L_KNJ_UTF8BOM :
			TmpLocalKanjiCode = KANJI_UTF8BOM;
			break;
	}
	DispLocalKanjiCode();
	HideLocalKanjiButton();
	return;
}

void DispLocalKanjiCode(void)
{
	switch(TmpLocalKanjiCode)
	{
		// UTF-8対応
		case KANJI_SJIS :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_SJIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_EUC :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_EUC, MAKELONG(TRUE, 0));
			break;

		case KANJI_JIS :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_JIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8N :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_UTF8N, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8BOM :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			break;
	}
	return;
}

int AskLocalKanjiCode(void)
{
	return(TmpLocalKanjiCode);
}

void HideLocalKanjiButton(void)
{
	switch(TmpTransMode)
	{
		// UTF-8対応
		case TYPE_I : 
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_SJIS, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_EUC, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_JIS, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8N, MAKELONG(FALSE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8BOM, MAKELONG(FALSE, 0));
			break;

		default :
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_SJIS, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_EUC, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_JIS, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8N, MAKELONG(TRUE, 0));
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			// 現在カナ変換はShift_JIS、JIS、EUC間でのみ機能する
			SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
			switch(TmpHostKanjiCode)
			{
			case KANJI_SJIS:
			case KANJI_JIS:
			case KANJI_EUC:
				switch(TmpLocalKanjiCode)
				{
				case KANJI_SJIS:
				case KANJI_JIS:
				case KANJI_EUC:
					SendMessageW(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
					break;
				}
				break;
			}
			break;
	}
	return;
}

void SaveLocalKanjiCode(void)
{
	LocalKanjiCode = TmpLocalKanjiCode;
	return;
}


/*===================================================
*			半角変換モード
*===================================================*/

/*----- ホストの半角変換モードを設定する --------------------------------------
*
*	Parameter
*		int Mode : 半角変換モード(YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetHostKanaCnvImm(int Mode)
{
	TmpHostKanaCnv = Mode;
	DispHostKanaCnv();
	return;
}


/*----- ホストの半角変換モードを反転する --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetHostKanaCnv(void)
{
	TmpHostKanaCnv ^= 1;
	DispHostKanaCnv();
	return;
}


/*----- ホストの半角変換モードにしたがってボタンを表示する --------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispHostKanaCnv(void)
{
	if(TmpHostKanaCnv != 0)
		SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
	else
		SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
	return;
}


/*----- ホストの半角変換モードを返す ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 半角変換モード
*----------------------------------------------------------------------------*/

int AskHostKanaCnv(void)
{
	return(TmpHostKanaCnv);
}


/*===================================================
*			ソート方法
*===================================================*/

/*----- ソート方法をセットする ------------------------------------------------
*
*	Parameter
*		int LFsort : ローカル側のファイルのソート方法 (SORT_xxx)
*		int LDsort : ローカル側のディレクトリのソート方法 (SORT_xxx)
*		int RFsort : ホスト側のファイルのソート方法 (SORT_xxx)
*		int RDsort : ホスト側のディレクトリのソート方法 (SORT_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetSortTypeImm(int LFsort, int LDsort, int RFsort, int RDsort)
{
	TmpLocalFileSort = LFsort;
	TmpLocalDirSort = LDsort;
	TmpRemoteFileSort = RFsort;
	TmpRemoteDirSort = RDsort;
	return;
}


/*----- リストビューのタブクリックによるソート方法のセット --------------------
*
*	Parameter
*		int Win : ウインドウ番号
*		int Tab : カラム番号
*
*	Return Value
*		int ソート方法 (SORT_xxx)
*----------------------------------------------------------------------------*/

void SetSortTypeByColumn(int Win, int Tab)
{
	if(Win == WIN_LOCAL)
	{
		if((TmpLocalFileSort & SORT_MASK_ORD) == Tab)
			TmpLocalFileSort ^= SORT_GET_ORD;
		else
			TmpLocalFileSort = Tab;

		if((Tab == SORT_NAME) || (Tab == SORT_DATE))
			TmpLocalDirSort = TmpLocalFileSort;
		else
			TmpLocalDirSort = SORT_NAME;
	}
	else
	{
		if(Tab != 4)
		{
			if((TmpRemoteFileSort & SORT_MASK_ORD) == Tab)
				TmpRemoteFileSort ^= SORT_GET_ORD;
			else
				TmpRemoteFileSort = Tab;

			if((Tab == SORT_NAME) || (Tab == SORT_DATE))
				TmpRemoteDirSort = TmpRemoteFileSort;
			else
				TmpRemoteDirSort = SORT_NAME;
		}
	}
	return;
}


/*----- ソート方法を返す ------------------------------------------------------
*
*	Parameter
*		int Name : どの部分か (ITEM_xxx)
*
*	Return Value
*		int ソート方法 (SORT_xxx)
*----------------------------------------------------------------------------*/

int AskSortType(int Name) {
	switch (Name) {
	case ITEM_LFILE:
		return TmpLocalFileSort;
	case ITEM_LDIR:
		return TmpLocalDirSort;
	case ITEM_RFILE:
		return TmpRemoteFileSort;
	case ITEM_RDIR:
		return TmpRemoteDirSort;
	}
	return TmpLocalFileSort;
}


/*----- ホストごとにソートを保存するかどうかをセットする-----------------------
*
*	Parameter
*		int Sw : スイッチ (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetSaveSortToHost(int Sw)
{
	SortSave = Sw;
	return;
}


/*----- ホストごとにソートを保存するかどうかを返す ----------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int スイッチ (YES/NO)
*----------------------------------------------------------------------------*/

int AskSaveSortToHost(void)
{
	return(SortSave);
}



/*===================================================
*			リストモード
*===================================================*/

/*----- リストモードにしたがってボタンを表示する ------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispListType(void)
{
	HWND hWndMain;

	hWndMain = GetMainHwnd();
	switch(ListType)
	{
		case LVS_LIST :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_LIST, MAKELONG(TRUE, 0));
			CheckMenuItem(GetMenu(hWndMain), MENU_LIST, MF_CHECKED);
			CheckMenuItem(GetMenu(hWndMain), MENU_REPORT, MF_UNCHECKED);
			break;

		default :
			SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_REPORT, MAKELONG(TRUE, 0));
			CheckMenuItem(GetMenu(hWndMain), MENU_REPORT, MF_CHECKED);
			CheckMenuItem(GetMenu(hWndMain), MENU_LIST, MF_UNCHECKED);
			break;
	}
	return;
}


/*===================================================
*			フォルダ同時移動モード
*===================================================*/

/*----- 転送モードを設定する --------------------------------------------------
*
*	Parameter
*		int Mode : 転送モード (TYPE_xx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetSyncMoveMode(int Mode)
{
	SyncMove = Mode;
	DispSyncMoveMode();
	return;
}


/*----- フォルダ同時移動モードを切り替える ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ToggleSyncMoveMode(void)
{
	SyncMove ^= 1;
	DispSyncMoveMode();
	return;
}


/*----- フォルダ同時移動を行うかどうかをによってメニュー／ボタンを表示 --------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispSyncMoveMode(void)
{
	if(SyncMove != 0)
	{
		SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_SYNC, MAKELONG(TRUE, 0));
		CheckMenuItem(GetMenu(GetMainHwnd()), MENU_SYNC, MF_CHECKED);
	}
	else
	{
		SendMessageW(hWndTbarMain, TB_CHECKBUTTON, MENU_SYNC, MAKELONG(FALSE, 0));
		CheckMenuItem(GetMenu(GetMainHwnd()), MENU_SYNC, MF_UNCHECKED);
	}
	return;
}


/*----- フォルダ同時移動モードを返す ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 半角変換モード
*----------------------------------------------------------------------------*/

int AskSyncMoveMode(void)
{
	return(SyncMove);
}


/*===================================================
*			ディレクトリヒストリ
*===================================================*/

/*----- ホスト側のヒストリ一覧ウインドウに登録 --------------------------------
*
*	Parameter
*		char *Path : パス
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetRemoteDirHist(char *Path)
{
	LONG_PTR i;

	if((i = SendMessageW(hWndDirRemote, CB_FINDSTRINGEXACT, 0, (LPARAM)u8(Path).c_str())) != CB_ERR)
		SendMessageW(hWndDirRemote, CB_DELETESTRING, i, 0);

	SendMessageW(hWndDirRemote, CB_ADDSTRING, 0, (LPARAM)u8(Path).c_str());
	i = SendMessageW(hWndDirRemote, CB_GETCOUNT, 0, 0);
	SendMessageW(hWndDirRemote, CB_SETCURSEL, i-1, 0);

	strcpy(RemoteCurDir, Path);
	return;
}


/*----- ローカル側のヒストリ一覧ウインドウに登録 -------------------------------
*
*	Parameter
*		char *Path : パス
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetLocalDirHist(const char *Path)
{
	int i;

	if((i = (int)SendMessageW(hWndDirLocal, CB_FINDSTRINGEXACT, 0, (LPARAM)u8(Path).c_str())) == CB_ERR)
		SendMessageW(hWndDirLocal, CB_ADDSTRING, 0, (LPARAM)u8(Path).c_str());
	i = (int)SendMessageW(hWndDirLocal, CB_FINDSTRINGEXACT, 0, (LPARAM)u8(Path).c_str());
	SendMessageW(hWndDirLocal, CB_SETCURSEL, i, 0);

	strcpy(LocalCurDir, Path);
	return;
}


/*----- ローカルのカレントディレクトリを返す ----------------------------------
*
*	Parameter
*		char *Buf : カレントディレクトリ名を返すバッファ
*		int Max : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AskLocalCurDir(char *Buf, size_t Max)
{
	memset(Buf, 0, Max);
	strncpy(Buf, LocalCurDir, Max-1);
	return;
}


/*----- ホストのカレントディレクトリを返す ------------------------------------
*
*	Parameter
*		char *Buf : カレントディレクトリ名を返すバッファ
*		int Max : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AskRemoteCurDir(char *Buf, size_t Max)
{
	memset(Buf, 0, Max);
	strncpy(Buf, RemoteCurDir, Max-1);
	return;
}


// カレントディレクトリを設定する
void SetCurrentDirAsDirHist() {
	std::error_code ec;
	fs::current_path(fs::u8path(LocalCurDir), ec);
}


/*===================================================
*			メニュー
*===================================================*/

/*----- ドットファイルを表示するかどうかをメニューに表示する ------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispDotFileMode(void)
{
	CheckMenuItem(GetMenu(GetMainHwnd()), MENU_DOTFILE, MF_UNCHECKED);
	if(DotFile == YES)
		CheckMenuItem(GetMenu(GetMainHwnd()), MENU_DOTFILE, MF_CHECKED);
	return;
}


// 右ボタンメニューを表示
void ShowPopupMenu(int Win, int Pos) {
	if (HideUI != 0)
		return;
	auto selectCount = GetSelectedCount(Win);
	auto connecting = AskConnecting() == YES;
	auto selecting = 0 < selectCount;
	auto canOpen = selectCount == 1 && (Win == WIN_LOCAL || connecting);
	auto menu = LoadMenuW(GetFtpInst(), MAKEINTRESOURCEW(popup_menu));
	auto submenu = GetSubMenu(menu, Win);
	EnableMenuItem(submenu, MENU_OPEN, canOpen ? 0 : MF_GRAYED);
	constexpr UINT MenuID[VIEWERS] = { MENU_OPEN1, MENU_OPEN2, MENU_OPEN3 };
	for (int i = VIEWERS - 1; i >= 0; i--) {
		if (strlen(ViewerName[i]) != 0) {
			static auto const format = GetString(IDS_OPEN_WITH);
			wchar_t text[FMAX_PATH + 1];
			swprintf_s(text, format.c_str(), fs::u8path(ViewerName[i]).filename().c_str(), i + 1);
			MENUITEMINFOW mii{ sizeof(MENUITEMINFOW), MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_STRING, MFT_STRING, UINT(canOpen ? 0 : MFS_GRAYED), MenuID[i] };
			mii.dwTypeData = text;
			InsertMenuItemW(submenu, 1, true, &mii);
		}
	}
	if (Win == WIN_LOCAL) {
		EnableMenuItem(submenu, MENU_UPLOAD, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_UPLOAD_AS, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_UPLOAD_ALL, connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_DELETE, selecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_RENAME, selecting ? 0 : MF_GRAYED);
	} else {
		EnableMenuItem(submenu, MENU_DOWNLOAD, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_DOWNLOAD_AS, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_DOWNLOAD_AS_FILE, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_DOWNLOAD_ALL, connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_DELETE, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_RENAME, selecting && connecting ? 0 : MF_GRAYED);
#if defined(HAVE_TANDEM)
		/* HP NonStop Server では CHMOD の仕様が異なるため使用不可 */
		if (AskRealHostType() == HTYPE_TANDEM)
			RemoveMenu(submenu, MENU_CHMOD, MF_BYCOMMAND);
		else
#endif
			EnableMenuItem(submenu, MENU_CHMOD, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_MKDIR, connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_URL_COPY, selecting && connecting ? 0 : MF_GRAYED);
#if defined(HAVE_TANDEM)
		/* OSS モードのときに表示されるように AskRealHostType() を使用する */
		if (AskRealHostType() == HTYPE_TANDEM)
			EnableMenuItem(submenu, MENU_SWITCH_OSS, connecting ? 0 : MF_GRAYED);
		else
#endif
			RemoveMenu(submenu, MENU_SWITCH_OSS, MF_BYCOMMAND);
		EnableMenuItem(submenu, MENU_REMOTE_MOVE_UPDIR, selecting && connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, MENU_FILESIZE, connecting ? 0 : MF_GRAYED);
		EnableMenuItem(submenu, REFRESH_REMOTE, connecting ? 0 : MF_GRAYED);
	}
	POINT point;
	if (Pos == 0)
		GetCursorPos(&point);
	else {
		RECT Rect;
		GetWindowRect(Win == WIN_LOCAL ? GetLocalHwnd() : GetRemoteHwnd(), &Rect);
		point.x = Rect.left + 20;
		point.y = Rect.top + 20;
	}
	TrackPopupMenu(submenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, 0, GetMainHwnd(), NULL);
	DestroyMenu(menu);
}
