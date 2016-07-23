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

#define  STRICT
// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <malloc.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdarg.h>
// IPv6対応
//#include <winsock.h>

#include "common.h"
#include "resource.h"


/*===== プロトタイプ =====*/

static void AddOpenMenu(HMENU hMenu, UINT Flg);
static LRESULT CALLBACK HistEditBoxWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/* 2007/09/21 sunasunamix  ここから *********************/
static LRESULT CALLBACK CountermeasureTbarMainProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam);
static LRESULT CALLBACK CountermeasureTbarLocalProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam);
static LRESULT CALLBACK CountermeasureTbarRemoteProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam);
/********************************************* ここまで */


/*===== 外部参照 =====*/

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

static WNDPROC HistEditBoxProcPtr;

static HFONT DlgFont = NULL;

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


/* 2007/09/21 sunasunamix  ここから *********************/
static WNDPROC pOldTbarMainProc   = NULL;
static WNDPROC pOldTbarLocalProc  = NULL;
static WNDPROC pOldTbarRemoteProc = NULL;
/********************************************* ここまで */


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



/*----- ツールバーを作成する --------------------------------------------------
*
*	Parameter
*		HWND hWnd : 親ウインドウのウインドウハンドル
*		HINSTANCE hInst : インスタンスハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeToolBarWindow(HWND hWnd, HINSTANCE hInst)
{
	int Sts;
	RECT Rect1;
	char Tmp[FMAX_PATH+1];
	char *Pos;
	int Tmp2;
	DWORD NoDrives;
	// 高DPI対応
	HBITMAP hOriginal;
	HBITMAP hResized;

	/*===== メインのツールバー =====*/

	// 高DPI対応
//	hWndTbarMain = CreateToolbarEx(
//				hWnd,
//				WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_TOP | TBSTYLE_FLAT,
//				1,
//				27,
//				hInst,
//				main_toolbar_bmp,
//				TbarDataMain,
//				sizeof(TbarDataMain)/sizeof(TBBUTTON),
//				16,16,
//				16,16,
//				sizeof(TBBUTTON));
	hOriginal = LoadImage(hInst, MAKEINTRESOURCE(main_toolbar_bmp), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
	if(hOriginal != NULL)
	{
		hResized = ResizeBitmap(hOriginal, 64, 64, 16, 64);
		DeleteObject(hOriginal);
	}
	hWndTbarMain = CreateToolbarEx(
				hWnd,
				WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_TOP | TBSTYLE_FLAT,
				1,
				30,
				NULL,
				(UINT_PTR)hResized,
				TbarDataMain,
				sizeof(TbarDataMain)/sizeof(TBBUTTON),
				CalcPixelX(16),CalcPixelY(16),
				CalcPixelX(16),CalcPixelY(16),
				sizeof(TBBUTTON));
	if(hResized != NULL)
		DeleteObject(hOriginal);
	hResized = NULL;

	if(hWndTbarMain != NULL)
	{
		/* 2007/09/21 sunasunamix  ここから *********************/
		// 64ビット対応
//		pOldTbarMainProc = (WNDPROC)SetWindowLong(hWndTbarMain, GWL_WNDPROC, (DWORD)CountermeasureTbarMainProc);
		pOldTbarMainProc = (WNDPROC)SetWindowLongPtr(hWndTbarMain, GWLP_WNDPROC, (LONG_PTR)CountermeasureTbarMainProc);
		/********************************************* ここまで */

		GetClientRect(hWnd, &Rect1);
		// 高DPI対応
//		MoveWindow(hWndTbarMain, 0, 0, Rect1.right, TOOLWIN_HEIGHT, FALSE);
		MoveWindow(hWndTbarMain, 0, 0, Rect1.right, AskToolWinHeight(), FALSE);
	}

	/*===== ローカルのツールバー =====*/

	// 高DPI対応
//	hWndTbarLocal = CreateToolbarEx(
//				hWnd,
//				WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_NORESIZE | TBSTYLE_FLAT,
//				2,
//				2,
//				hInst,
//				remote_toolbar_bmp,
//				TbarDataLocal,
//				sizeof(TbarDataLocal)/sizeof(TBBUTTON),
//				16,16,
//				16,16,
//				sizeof(TBBUTTON));
	hOriginal = LoadImage(hInst, MAKEINTRESOURCE(remote_toolbar_bmp), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
	if(hOriginal != NULL)
	{
		hResized = ResizeBitmap(hOriginal, 64, 64, 16, 64);
		DeleteObject(hOriginal);
	}
	hWndTbarLocal = CreateToolbarEx(
				hWnd,
				WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_NORESIZE | TBSTYLE_FLAT,
				2,
				2,
				NULL,
				(UINT_PTR)hResized,
				TbarDataLocal,
				sizeof(TbarDataLocal)/sizeof(TBBUTTON),
				CalcPixelX(16),CalcPixelY(16),
				CalcPixelX(16),CalcPixelY(16),
				sizeof(TBBUTTON));
	if(hResized != NULL)
		DeleteObject(hOriginal);
	hResized = NULL;

	if(hWndTbarLocal != NULL)
	{
		/* 2007/09/21 sunasunamix  ここから *********************/
		// 64ビット対応
//		pOldTbarLocalProc = (WNDPROC)SetWindowLong(hWndTbarLocal, GWL_WNDPROC, (DWORD)CountermeasureTbarLocalProc);
		pOldTbarLocalProc = (WNDPROC)SetWindowLongPtr(hWndTbarLocal, GWLP_WNDPROC, (LONG_PTR)CountermeasureTbarLocalProc);
		/********************************************* ここまで */

		// 高DPI対応
//		MoveWindow(hWndTbarLocal, 0, TOOLWIN_HEIGHT, LocalWidth, TOOLWIN_HEIGHT, FALSE);
		MoveWindow(hWndTbarLocal, 0, AskToolWinHeight(), LocalWidth, AskToolWinHeight(), FALSE);

		/*===== ローカルのディレクトリ名ウインドウ =====*/

		SendMessage(hWndTbarLocal, TB_GETITEMRECT, 3, (LPARAM)&Rect1);
		// UTF-8対応
		// 高DPI対応
//#ifndef FFFTP_ENGLISH
//		DlgFont = CreateFont(Rect1.bottom-Rect1.top-8, 0, 0, 0, 0, FALSE,FALSE,FALSE,SHIFTJIS_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,MSGJPN254);
//#else
//		DlgFont = CreateFont(Rect1.bottom-Rect1.top-8, 0, 0, 0, 0, FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,MSGJPN254);
//#endif
		DlgFont = CreateFont(Rect1.bottom-Rect1.top-CalcPixelY(8), 0, 0, 0, 0, FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,MSGJPN254);

		// 高DPI対応
//		hWndDirLocal = CreateWindowEx(WS_EX_CLIENTEDGE,
//					"COMBOBOX", "",
//					WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | CBS_DROPDOWN | CBS_SORT | CBS_AUTOHSCROLL,
//					Rect1.right, Rect1.top, LocalWidth - Rect1.right, 200,
//					hWndTbarLocal, (HMENU)COMBO_LOCAL, hInst, NULL);
		hWndDirLocal = CreateWindowEx(WS_EX_CLIENTEDGE,
					"COMBOBOX", "",
					WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | CBS_DROPDOWN | CBS_SORT | CBS_AUTOHSCROLL,
					Rect1.right, Rect1.top, LocalWidth - Rect1.right, CalcPixelY(200),
					hWndTbarLocal, (HMENU)COMBO_LOCAL, hInst, NULL);

		if(hWndDirLocal != NULL)
		{
			/* エディットコントロールを探す */
			hWndDirLocalEdit = GetWindow(hWndDirLocal, GW_CHILD);
			if(hWndDirLocalEdit != NULL)
				// 64ビット対応
//				HistEditBoxProcPtr = (WNDPROC)SetWindowLong(hWndDirLocalEdit, GWL_WNDPROC, (LONG)HistEditBoxWndProc);
				HistEditBoxProcPtr = (WNDPROC)SetWindowLongPtr(hWndDirLocalEdit, GWLP_WNDPROC, (LONG_PTR)HistEditBoxWndProc);

			SendMessage(hWndDirLocal, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE, 0));
			SendMessage(hWndDirLocal, CB_LIMITTEXT, FMAX_PATH, 0);

			/* ドライブ名をセットしておく */
			GetLogicalDriveStrings(FMAX_PATH, Tmp);
			NoDrives = LoadHideDriveListRegistry();
			Pos = Tmp;
			while(*Pos != NUL)
			{
				Tmp2 = toupper(*Pos) - 'A';
				if((NoDrives & (0x00000001 << Tmp2)) == 0)
					SetLocalDirHist(Pos);
				Pos = strchr(Pos, NUL) + 1;
			}
			SendMessage(hWndDirLocal, CB_SETCURSEL, 0, 0);
		}
	}

	/*===== ホストのツールバー =====*/

	// 高DPI対応
//	hWndTbarRemote = CreateToolbarEx(
//				hWnd,
//				WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_NORESIZE | TBSTYLE_FLAT,
//				3,
//				2,
//				hInst,
//				remote_toolbar_bmp,
//				TbarDataRemote,
//				sizeof(TbarDataRemote)/sizeof(TBBUTTON),
//				16,16,
//				16,16,
//				sizeof(TBBUTTON));
	hOriginal = LoadImage(hInst, MAKEINTRESOURCE(remote_toolbar_bmp), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS);
	if(hOriginal != NULL)
	{
		hResized = ResizeBitmap(hOriginal, 64, 64, 16, 64);
		DeleteObject(hOriginal);
	}
	hWndTbarRemote = CreateToolbarEx(
				hWnd,
				WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CCS_NORESIZE | TBSTYLE_FLAT,
				3,
				2,
				NULL,
				(UINT_PTR)hResized,
				TbarDataRemote,
				sizeof(TbarDataRemote)/sizeof(TBBUTTON),
				CalcPixelX(16),CalcPixelY(16),
				CalcPixelX(16),CalcPixelY(16),
				sizeof(TBBUTTON));
	if(hResized != NULL)
		DeleteObject(hOriginal);
	hResized = NULL;

	if(hWndTbarRemote != NULL)
	{
		/* 2007/09/21 sunasunamix  ここから *********************/
		// 64ビット対応
//		pOldTbarRemoteProc = (WNDPROC)SetWindowLong(hWndTbarRemote, GWL_WNDPROC, (DWORD)CountermeasureTbarRemoteProc);
		pOldTbarRemoteProc = (WNDPROC)SetWindowLongPtr(hWndTbarRemote, GWLP_WNDPROC, (LONG_PTR)CountermeasureTbarRemoteProc);
		/********************************************* ここまで */

		// 高DPI対応
//		MoveWindow(hWndTbarRemote, LocalWidth + SepaWidth, TOOLWIN_HEIGHT, RemoteWidth, TOOLWIN_HEIGHT, FALSE);
		MoveWindow(hWndTbarRemote, LocalWidth + SepaWidth, AskToolWinHeight(), RemoteWidth, AskToolWinHeight(), FALSE);

		/*===== ホストのディレクトリ名ウインドウ =====*/

		SendMessage(hWndTbarRemote, TB_GETITEMRECT, 3, (LPARAM)&Rect1);
		// 高DPI対応
//		hWndDirRemote = CreateWindowEx(WS_EX_CLIENTEDGE,
//					"COMBOBOX", "",
//					WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL,
//					Rect1.right, Rect1.top, RemoteWidth - Rect1.right, 200,
//					hWndTbarRemote, (HMENU)COMBO_REMOTE, hInst, NULL);
		hWndDirRemote = CreateWindowEx(WS_EX_CLIENTEDGE,
					"COMBOBOX", "",
					WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL,
					Rect1.right, Rect1.top, RemoteWidth - Rect1.right, CalcPixelY(200),
					hWndTbarRemote, (HMENU)COMBO_REMOTE, hInst, NULL);

		if(hWndDirRemote != NULL)
		{
			/* エディットコントロールを探す */
			hWndDirRemoteEdit = GetWindow(hWndDirRemote, GW_CHILD);
			if(hWndDirRemoteEdit != NULL)
				// 64ビット対応
//				HistEditBoxProcPtr = (WNDPROC)SetWindowLong(hWndDirRemoteEdit, GWL_WNDPROC, (LONG)HistEditBoxWndProc);
				HistEditBoxProcPtr = (WNDPROC)SetWindowLongPtr(hWndDirRemoteEdit, GWLP_WNDPROC, (LONG_PTR)HistEditBoxWndProc);

			SendMessage(hWndDirRemote, WM_SETFONT, (WPARAM)DlgFont, MAKELPARAM(TRUE, 0));
			SendMessage(hWndDirRemote, CB_LIMITTEXT, FMAX_PATH, 0);
			SendMessage(hWndDirRemote, CB_SETCURSEL, 0, 0);
		}
	}

	Sts = FFFTP_SUCCESS;
	if((hWndTbarMain == NULL) ||
	   (hWndTbarLocal == NULL) ||
	   (hWndTbarRemote == NULL) ||
	   (hWndDirLocal == NULL) ||
	   (hWndDirRemote == NULL))
	{
		Sts = FFFTP_FAIL;
	}
	return(Sts);
}





static LRESULT CALLBACK HistEditBoxWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tmp[FMAX_PATH+1];

	switch (message)
	{
		case WM_CHAR :
			if(wParam == 0x0D)		/* リターンキーが押された */
			{
				if(hWnd == hWndDirLocalEdit)
				{
					SendMessage(hWndDirLocalEdit, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Tmp);
					DoLocalCWD(Tmp);
					GetLocalDirForWnd();
				}
				else
				{
					// 同時接続対応
					CancelFlg = NO;
					SendMessage(hWndDirRemoteEdit, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Tmp);
					if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
					{
						if(DoCWD(Tmp, YES, NO, YES) < FTP_RETRY)
							GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
					}
				}
			}
			else if(wParam == 0x09)		/* TABキーが押された */
			{
				if(hWnd == hWndDirLocalEdit)
				{
					SetFocus(GetLocalHwnd());
				}
				else
				{
					SetFocus(GetRemoteHwnd());
				}
			}
			else
				return(CallWindowProc(HistEditBoxProcPtr, hWnd, message, wParam, lParam));
			break;

		default :
			return(CallWindowProc(HistEditBoxProcPtr, hWnd, message, wParam, lParam));
	}
    return(0L);
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
//	if(DlgFont != NULL)
//		DeleteObject(DlgFont);

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
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_MIRROR_UPLOAD, MAKELONG(TRUE, 0));
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
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_MIRROR_UPLOAD, MAKELONG(FALSE, 0));
		}

		if(hWndFocus == GetLocalHwnd())
		{
			if((AskConnecting() == YES) && (Count > 0))
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_ENABLED);
			}
			else
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_GRAYED);
			}
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(FALSE, 0));
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
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_ENABLED);
			}
			else
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_GRAYED);
			}
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_GRAYED);
		}

		if((hWndFocus == GetLocalHwnd()) || (hWndFocus == GetRemoteHwnd()))
		{
			if(Count > 0)
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DELETE, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DELETE, MF_ENABLED);
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_RENAME, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_RENAME, MF_ENABLED);

				EnableMenuItem(GetMenu(hWndMain), MENU_CHMOD, MF_ENABLED);

			}
			else
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DELETE, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_DELETE, MF_GRAYED);
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_RENAME, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_RENAME, MF_GRAYED);

				EnableMenuItem(GetMenu(hWndMain), MENU_CHMOD, MF_GRAYED);
			}

			if((hWndFocus == GetLocalHwnd()) || (AskConnecting() == YES))
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_MKDIR, MAKELONG(TRUE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_MKDIR, MF_ENABLED);
				EnableMenuItem(GetMenu(hWndMain), MENU_FILESIZE, MF_ENABLED);
			}
			else
			{
				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_MKDIR, MAKELONG(FALSE, 0));
				EnableMenuItem(GetMenu(hWndMain), MENU_MKDIR, MF_GRAYED);
				EnableMenuItem(GetMenu(hWndMain), MENU_FILESIZE, MF_GRAYED);
			}
		}
		else
		{
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_UPLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_UPLOAD_AS, MF_GRAYED);
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DOWNLOAD, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS, MF_GRAYED);
			EnableMenuItem(GetMenu(hWndMain), MENU_DOWNLOAD_AS_FILE, MF_GRAYED);
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_DELETE, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_DELETE, MF_GRAYED);
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_RENAME, MAKELONG(FALSE, 0));
			EnableMenuItem(GetMenu(hWndMain), MENU_RENAME, MF_GRAYED);
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_MKDIR, MAKELONG(FALSE, 0));
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
		SendMessage(hWndTbarMain, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(FALSE, 0));
		SendMessage(hWndTbarLocal, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(FALSE, 0));
		SendMessage(hWndTbarRemote, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(FALSE, 0));
	}

	EnableWindow(hWndDirLocal, FALSE);
	EnableWindow(hWndDirRemote, FALSE);

	// 特定の操作を行うと異常終了するバグ修正
	EnableWindow(GetLocalHwnd(), FALSE);
	EnableWindow(GetRemoteHwnd(), FALSE);

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
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(TRUE, 0));
			SendMessage(hWndTbarLocal, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(TRUE, 0));
			SendMessage(hWndTbarRemote, TB_ENABLEBUTTON, HideMenus[i], MAKELONG(TRUE, 0));
		}
		EnableWindow(hWndDirLocal, TRUE);
		EnableWindow(hWndDirRemote, TRUE);

		// 特定の操作を行うと異常終了するバグ修正
		EnableWindow(GetLocalHwnd(), TRUE);
		EnableWindow(GetRemoteHwnd(), TRUE);

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
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_TEXT, MAKELONG(TRUE, 0));
			break;

		case TYPE_I :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_BINARY, MAKELONG(TRUE, 0));
			break;

		default :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_AUTO, MAKELONG(TRUE, 0));
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


/*----- 実際の転送モードを返す ------------------------------------------------
*
*	Parameter
*		char Fname : ファイル名
*		int Type : 設定上の転送モード (TYPE_xx)
*
*	Return Value
*		int 転送モード (TYPE_xx)
*----------------------------------------------------------------------------*/

int AskTransferTypeAssoc(char *Fname, int Type)
{
	int Ret;
	char *Pos;
	char *Name;

	Ret = Type;
	if(Ret == TYPE_X)
	{
		Ret = TYPE_I;
		if(StrMultiLen(AsciiExt) > 0)
		{
			Name = GetFileName(Fname);
			Pos = AsciiExt;
			while(*Pos != NUL)
			{
				if(CheckFname(Name, Pos) == FFFTP_SUCCESS)
				{
					Ret = TYPE_A;
					break;
				}
				Pos += strlen(Pos) + 1;
			}
		}
	}
	return(Ret);
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
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_SJIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_EUC :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_EUC, MAKELONG(TRUE, 0));
			break;

		case KANJI_JIS :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_JIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8N :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_UTF8N, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8BOM :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			break;

		default :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KNJ_NONE, MAKELONG(TRUE, 0));
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
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_SJIS, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_EUC, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_JIS, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8N, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8BOM, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_NONE, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
			break;

		default :
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_SJIS, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_EUC, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_JIS, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8N, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KNJ_NONE, MAKELONG(TRUE, 0));
//			if(TmpHostKanjiCode != KANJI_NOCNV)
//				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
//			else
//				SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
//			break;
			// 現在カナ変換はShift_JIS、JIS、EUC間でのみ機能する
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
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
					SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
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
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_SJIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_EUC :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_EUC, MAKELONG(TRUE, 0));
			break;

		case KANJI_JIS :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_JIS, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8N :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_UTF8N, MAKELONG(TRUE, 0));
			break;

		case KANJI_UTF8BOM :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_L_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
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
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_SJIS, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_EUC, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_JIS, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8N, MAKELONG(FALSE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8BOM, MAKELONG(FALSE, 0));
			break;

		default :
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_SJIS, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_EUC, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_JIS, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8N, MAKELONG(TRUE, 0));
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_L_KNJ_UTF8BOM, MAKELONG(TRUE, 0));
			// 現在カナ変換はShift_JIS、JIS、EUC間でのみ機能する
			SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
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
					SendMessage(hWndTbarMain, TB_ENABLEBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
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
		SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KANACNV, MAKELONG(TRUE, 0));
	else
		SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_KANACNV, MAKELONG(FALSE, 0));
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

int AskSortType(int Name)
{
	int Ret;

	switch(Name)
	{
		case ITEM_LFILE :
			Ret = TmpLocalFileSort;
			break;

		case ITEM_LDIR :
			Ret = TmpLocalDirSort;
			break;

		case ITEM_RFILE :
			Ret = TmpRemoteFileSort;
			break;

		case ITEM_RDIR :
			Ret = TmpRemoteDirSort;
			break;
	}
	return(Ret);
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
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_LIST, MAKELONG(TRUE, 0));
			CheckMenuItem(GetMenu(hWndMain), MENU_LIST, MF_CHECKED);
			CheckMenuItem(GetMenu(hWndMain), MENU_REPORT, MF_UNCHECKED);
			break;

		default :
			SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_REPORT, MAKELONG(TRUE, 0));
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
		SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_SYNC, MAKELONG(TRUE, 0));
		CheckMenuItem(GetMenu(GetMainHwnd()), MENU_SYNC, MF_CHECKED);
	}
	else
	{
		SendMessage(hWndTbarMain, TB_CHECKBUTTON, MENU_SYNC, MAKELONG(FALSE, 0));
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
	int i;

	if((i = SendMessage(hWndDirRemote, CB_FINDSTRINGEXACT, 0, (LPARAM)Path)) != CB_ERR)
		SendMessage(hWndDirRemote, CB_DELETESTRING, i, 0);

	SendMessage(hWndDirRemote, CB_ADDSTRING, 0, (LPARAM)Path);
	i = SendMessage(hWndDirRemote, CB_GETCOUNT, 0, 0);
	SendMessage(hWndDirRemote, CB_SETCURSEL, i-1, 0);

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

void SetLocalDirHist(char *Path)
{
	int i;

	if((i = SendMessage(hWndDirLocal, CB_FINDSTRINGEXACT, 0, (LPARAM)Path)) == CB_ERR)
		SendMessage(hWndDirLocal, CB_ADDSTRING, 0, (LPARAM)Path);
	i = SendMessage(hWndDirLocal, CB_FINDSTRINGEXACT, 0, (LPARAM)Path);
	SendMessage(hWndDirLocal, CB_SETCURSEL, i, 0);

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

void AskLocalCurDir(char *Buf, int Max)
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

void AskRemoteCurDir(char *Buf, int Max)
{
	memset(Buf, 0, Max);
	strncpy(Buf, RemoteCurDir, Max-1);
	return;
}


/*----- カレントディレクトリを設定する ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetCurrentDirAsDirHist(void)
{
	SetCurrentDirectory(LocalCurDir);
	return;
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


/*----- ローカル側の右ボタンメニューを表示 ------------------------------------------------
*
*	Parameter
*		int Pos : メニューの位置
*					0=マウスカーソルの位置
*					1=ウインドウの左上
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void LocalRbuttonMenu(int Pos)
{
	HMENU hMenu;
	POINT point;
	RECT Rect;
	UINT Flg1;
	UINT Flg2;
	UINT Flg3;
	int Count;

	// デッドロック対策
//	if(HideUI == NO)
	if(HideUI == 0)
	{
		Flg1 = 0;
		if(AskConnecting() == NO)
			Flg1 = MF_GRAYED;

		Count = GetSelectedCount(WIN_LOCAL);
		Flg2 = 0;
		if(Count == 0)
			Flg2 = MF_GRAYED;

		Flg3 = 0;
		if(Count != 1)
			Flg3 = MF_GRAYED;

		hMenu = CreatePopupMenu();
		AddOpenMenu(hMenu, Flg3);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_UPLOAD, MSGJPN255);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_UPLOAD_AS, MSGJPN256);
		AppendMenu(hMenu, MF_STRING | Flg1, MENU_UPLOAD_ALL, MSGJPN257);
		AppendMenu(hMenu, MF_STRING | Flg2, MENU_DELETE, MSGJPN258);
		AppendMenu(hMenu, MF_STRING | Flg2, MENU_RENAME, MSGJPN259);
		AppendMenu(hMenu, MF_STRING , MENU_MKDIR, MSGJPN260);
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING , MENU_FILESIZE, MSGJPN261);
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, REFRESH_LOCAL, MSGJPN262);

		if(Pos == 0)
			GetCursorPos(&point);
		else
		{
			GetWindowRect(GetLocalHwnd(), &Rect);
			point.x = Rect.left + 20;
			point.y = Rect.top + 20;
		}
		TrackPopupMenu(hMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, 0, GetMainHwnd(), NULL);

		DestroyMenu(hMenu);
	}
	return;
}


/*----- ホスト側の右ボタンメニューを表示 --------------------------------------
*
*	Parameter
*		int Pos : メニューの位置
*					0=マウスカーソルの位置
*					1=ウインドウの左上
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RemoteRbuttonMenu(int Pos)
{
	HMENU hMenu;
	POINT point;
	RECT Rect;
	UINT Flg1;
	UINT Flg2;
	UINT Flg3;
	int Count;

	// デッドロック対策
//	if(HideUI == NO)
	if(HideUI == 0)
	{
		Flg1 = 0;
		if(AskConnecting() == NO)
			Flg1 = MF_GRAYED;

		Count = GetSelectedCount(WIN_REMOTE);
		Flg2 = 0;
		if(Count == 0)
			Flg2 = MF_GRAYED;

		Flg3 = 0;
		if(Count != 1)
			Flg3 = MF_GRAYED;

		hMenu = CreatePopupMenu();
		AddOpenMenu(hMenu, Flg1 | Flg3);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_DOWNLOAD, MSGJPN263);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_DOWNLOAD_AS, MSGJPN264);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_DOWNLOAD_AS_FILE, MSGJPN265);
		AppendMenu(hMenu, MF_STRING | Flg1, MENU_DOWNLOAD_ALL, MSGJPN266);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_DELETE, MSGJPN267);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_RENAME, MSGJPN268);
#if defined(HAVE_TANDEM)
		/* HP NonStop Server では CHMOD の仕様が異なるため使用不可 */
		if (AskRealHostType() != HTYPE_TANDEM)
#endif
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_CHMOD, MSGJPN269);
		AppendMenu(hMenu, MF_STRING | Flg1, MENU_MKDIR, MSGJPN270);
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_URL_COPY, MSGJPN271);
#if defined(HAVE_TANDEM)
		/* OSS モードのときに表示されるように AskRealHostType() を使用する */
		if (AskRealHostType() == HTYPE_TANDEM)
			AppendMenu(hMenu, MF_STRING | Flg1, MENU_SWITCH_OSS, MSGJPN2001);
#endif
		// 上位のディレクトリへ移動対応
		AppendMenu(hMenu, MF_STRING | Flg1 | Flg2, MENU_REMOTE_MOVE_UPDIR, MSGJPN355);
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING | Flg1, MENU_FILESIZE, MSGJPN272);
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING | Flg1, REFRESH_REMOTE, MSGJPN273);

		if(Pos == 0)
			GetCursorPos(&point);
		else
		{
			GetWindowRect(GetRemoteHwnd(), &Rect);
			point.x = Rect.left + 20;
			point.y = Rect.top + 20;
		}
		if(TrackPopupMenu(hMenu, TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, 0, GetMainHwnd(), NULL) == 0)
			Count = GetLastError();

		DestroyMenu(hMenu);
	}
	return;
}


/*----- 右ボタンメニューに「開く」を追加  -------------------------------------
*
*	Parameter
*		HMENU hMenu : メニューハンドル
*		UINT Flg : フラグ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AddOpenMenu(HMENU hMenu, UINT Flg)
{
	static const UINT MenuID[VIEWERS] = { MENU_OPEN1, MENU_OPEN2, MENU_OPEN3 };
	char Tmp[FMAX_PATH+1];
	int i;

	// ローカルフォルダを開く
//	AppendMenu(hMenu, MF_STRING | Flg, MENU_DCLICK, MSGJPN274);
	AppendMenu(hMenu, MF_STRING | Flg, MENU_OPEN, MSGJPN274);
	for(i = 0; i < VIEWERS; i++)
	{
		if(strlen(ViewerName[i]) != 0)
		{
			sprintf(Tmp, MSGJPN275, GetToolName(ViewerName[i]), i+1);
			AppendMenu(hMenu, MF_STRING | Flg, MenuID[i], Tmp);
		}
	}
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	return;
}

/* 2007/09/21 sunasunamix  ここから *********************/

/*----- CreateToolbarEx のマウスクリック関連を無視する(TbarMain用) -----------
*       (サブクラス化を行うためのウインドウプロシージャ)
*----------------------------------------------------------------------------*/
static LRESULT CALLBACK CountermeasureTbarMainProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	switch (uMessage) {
	case WM_DESTROY :
		// 64ビット対応
//		SetWindowLong(hWnd,GWL_WNDPROC,(DWORD)pOldTbarMainProc);
		SetWindowLongPtr(hWnd,GWLP_WNDPROC,(LONG_PTR)pOldTbarMainProc);
		break;
	case WM_RBUTTONDBLCLK :
	case WM_RBUTTONDOWN :
	case WM_RBUTTONUP :
		return TRUE;
	}
	return CallWindowProc(pOldTbarMainProc, hWnd, uMessage, wParam, lParam);
}

/*----- CreateToolbarEx のマウスクリック関連を無視する(TbarLocal用) ----------
*       (サブクラス化を行うためのウインドウプロシージャ)
*----------------------------------------------------------------------------*/
static LRESULT CALLBACK CountermeasureTbarLocalProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	switch (uMessage) {
	case WM_DESTROY :
		// 64ビット対応
//		SetWindowLong(hWnd,GWL_WNDPROC,(DWORD)pOldTbarLocalProc);
		SetWindowLongPtr(hWnd,GWLP_WNDPROC,(LONG_PTR)pOldTbarLocalProc);
		break;
	case WM_RBUTTONDBLCLK :
	case WM_RBUTTONDOWN :
	case WM_RBUTTONUP :
		return TRUE;
	}
	return CallWindowProc(pOldTbarLocalProc, hWnd, uMessage, wParam, lParam);
}

/*----- CreateToolbarEx のマウスクリック関連を無視する(TbarRemote用) ---------
*       (サブクラス化を行うためのウインドウプロシージャ)
*----------------------------------------------------------------------------*/
static LRESULT CALLBACK CountermeasureTbarRemoteProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	switch (uMessage) {
	case WM_DESTROY :
		// 64ビット対応
//		SetWindowLong(hWnd,GWL_WNDPROC,(DWORD)pOldTbarRemoteProc);
		SetWindowLongPtr(hWnd,GWLP_WNDPROC,(LONG_PTR)pOldTbarRemoteProc);
		break;
	case WM_RBUTTONDBLCLK :
	case WM_RBUTTONDOWN :
	case WM_RBUTTONUP :
		return TRUE;
	}
	return CallWindowProc(pOldTbarRemoteProc, hWnd, uMessage, wParam, lParam);
}
/********************************************* ここまで */
