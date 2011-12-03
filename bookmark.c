/*=============================================================================
*
*								ブックマーク
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

// UTF-8対応
//#define WINVER 0x400

#define	STRICT
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

#include "common.h"
#include "resource.h"

#include <htmlhelp.h>
#include "helpid.h"

// UTF-8対応
#undef __MBSWRAPPER_H__
#include "mbswrapper.h"


/*===== プロトタイプ =====*/

static int AddBookMark(char *Path);
static int GetBothPath(char *Str, char **Path1, char **Path2);
// 64ビット対応
//static BOOL CALLBACK EditBookMarkProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK BookMarkEditCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK EditBookMarkProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK BookMarkEditCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

/*===== 外部参照 =====*/

extern HWND hHelpWin;

/* 設定値 */
extern HFONT ListFont;		/* リストボックスのフォント */
extern SIZE BmarkDlgSize;



/*----- ブックマークをクリアする ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ClearBookMark(void)
{
	HMENU hMenu;

	hMenu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);
	while(GetMenuItemCount(hMenu) > DEFAULT_BMARK_ITEM)
		DeleteMenu(hMenu, DEFAULT_BMARK_ITEM, MF_BYPOSITION);
	return;
}


/*----- カレントディレクトリをブックマークに追加 ------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AddCurDirToBookMark(int Win)
{
	char Buf[BMARK_MARK_LEN + FMAX_PATH * 2 + BMARK_SEP_LEN + 1];

	if(Win == WIN_LOCAL)
	{
		strcpy(Buf, BMARK_MARK_LOCAL);
		AskLocalCurDir(Buf + BMARK_MARK_LEN, FMAX_PATH);
	}
	else if(Win == WIN_REMOTE)
	{
		strcpy(Buf, BMARK_MARK_REMOTE);
		AskRemoteCurDir(Buf + BMARK_MARK_LEN, FMAX_PATH);
	}
	else
	{
		strcpy(Buf, BMARK_MARK_BOTH);
		AskLocalCurDir(Buf + BMARK_MARK_LEN, FMAX_PATH);
		strcat(Buf, BMARK_SEP);
		AskRemoteCurDir(Buf + strlen(Buf), FMAX_PATH);
	}
	AddBookMark(Buf);
	return;
}


/*----- ブックマークにパスを登録する ------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int AddBookMark(char *Path)
{
	HMENU hMenu;
	int MarkID;
	int Sts;

	Sts = FFFTP_FAIL;
	hMenu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);
	MarkID = (GetMenuItemCount(hMenu) - DEFAULT_BMARK_ITEM) + MENU_BMARK_TOP;
	if(AppendMenu(hMenu, MF_STRING, MarkID, Path) == TRUE)
		Sts = FFFTP_SUCCESS;
	return(Sts);
}


/*----- 指定のIDを持つブックマークのパスを返す --------------------------------
*
*	Parameter
*		int MarkID : ID
*		char *Local : ローカル側のパスを返すバッファ
*		char *Remote : リモート側のパスを返すバッファ
*		int Max : バッファのサイズ
*
*	Return Value
*		int ステータス (BMARK_TYPE_xxx)
*----------------------------------------------------------------------------*/

int AskBookMarkText(int MarkID, char *Local, char *Remote, int Max)
{
	HMENU hMenu;
	MENUITEMINFO mInfo;
	int Sts;
	char Tmp[BMARK_MARK_LEN + FMAX_PATH * 2 + BMARK_SEP_LEN + 1];
	char *Path1;
	char *Path2;
	int Num;

	memset(Local, NUL, Max);
	memset(Remote, NUL, Max);

	Sts = BMARK_TYPE_NONE;
	hMenu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);

	mInfo.cbSize = sizeof(MENUITEMINFO);
	mInfo.fMask = MIIM_TYPE;
	mInfo.dwTypeData = Tmp;
	mInfo.cch = BMARK_MARK_LEN + FMAX_PATH * 2 + BMARK_SEP_LEN;
	if(GetMenuItemInfo(hMenu, MarkID, FALSE, &mInfo) == TRUE)
	{
		Num = GetBothPath(Tmp, &Path1, &Path2);
		if(strncmp(Tmp, BMARK_MARK_LOCAL, BMARK_MARK_LEN) == 0)
		{
			Sts = BMARK_TYPE_LOCAL;
			strncpy(Local, Path1, Max-1);
		}
		else if(strncmp(Tmp, BMARK_MARK_REMOTE, BMARK_MARK_LEN) == 0)
		{
			Sts = BMARK_TYPE_REMOTE;
			strncpy(Remote, Path1, Max-1);
		}
		else if(strncmp(Tmp, BMARK_MARK_BOTH, BMARK_MARK_LEN) == 0)
		{
			if(Num == 2)
			{
				strncpy(Local, Path1, Max-1);
				strncpy(Remote, Path2, Max-1);
				Sts = BMARK_TYPE_BOTH;
			}
		}
	}
	return(Sts);
}


/*----- ブックマークの文字列から２つのパスを取り出す --------------------------
*
*	Parameter
*		char *Str : 文字列
*		char **Local : ローカル側のパスの先頭を返すワーク
*		char **Remote : リモート側のパスの先頭を返すワーク
*
*	Return Value
*		int パスの個数 (1 or 2)
*
*	Note
*		Strの内容を書き換える
*----------------------------------------------------------------------------*/

static int GetBothPath(char *Str, char **Path1, char **Path2)
{
	char *Pos;
	int Ret;

	Ret = 1;
	*Path1 = Str + BMARK_MARK_LEN;

	Pos = _mbsstr(Str, BMARK_SEP);
	if(Pos != NULL)
	{
		Ret = 2;
		*Pos = NUL;
		*Path2 = Pos + BMARK_SEP_LEN;
	}
	return(Ret);
}


/*----- ブックマークを接続中のホストリストに保存する --------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SaveBookMark(void)
{
	HMENU hMenu;
	MENUITEMINFO mInfo;
	int i;
	int Cnt;
	char *Buf;
	char *Pos;
	int Len;
	char Tmp[BMARK_MARK_LEN + FMAX_PATH * 2 + BMARK_SEP_LEN + 1];
	int CurHost;

	if(AskConnecting() == YES)
	{
		if((CurHost = AskCurrentHost()) != HOSTNUM_NOENTRY)
		{
			if((Buf = malloc(BOOKMARK_SIZE)) != NULL)
			{
				hMenu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);

				Pos = Buf;
				Len = 0;
				Cnt = GetMenuItemCount(hMenu);
				for(i = DEFAULT_BMARK_ITEM; i < Cnt; i++)
				{
					mInfo.cbSize = sizeof(MENUITEMINFO);
					mInfo.fMask = MIIM_TYPE;
					mInfo.dwTypeData = Tmp;
					mInfo.cch = FMAX_PATH;
					if(GetMenuItemInfo(hMenu, i, TRUE, &mInfo) == TRUE)
					{
						if(Len + strlen(Tmp) + 2 <= BOOKMARK_SIZE)
						{
							strcpy(Pos, Tmp);
							Pos += strlen(Tmp) + 1;
							Len += strlen(Tmp) + 1;
						}
					}
				}

				if(Pos == Buf)
				{
					memset(Buf, NUL, 2);
					Len = 2;
				}
				else
				{
					*Pos = NUL;
					Len++;
				}

				SetHostBookMark(CurHost, Buf, Len);

				free(Buf);
			}
		}
	}
	return;
}


/*----- ホストリストからブックマークを読み込む --------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void LoadBookMark(void)
{
	char *Buf;
	char *Pos;
	int CurHost;
	char Tmp[FMAX_PATH + BMARK_MARK_LEN + 1];

	if(AskConnecting() == YES)
	{
		if((CurHost = AskCurrentHost()) != HOSTNUM_NOENTRY)
		{
			if((Buf = AskHostBookMark(CurHost)) != NULL)
			{
				ClearBookMark();
				Pos = Buf;
				while(*Pos != NUL)
				{
					/* 旧フォーマットのための処理 */
					/* （パスに"L"や"H"がついてない物） */
					if((strncmp(Pos, BMARK_MARK_LOCAL, BMARK_MARK_LEN) != 0) &&
					   (strncmp(Pos, BMARK_MARK_REMOTE, BMARK_MARK_LEN) != 0) &&
					   (strncmp(Pos, BMARK_MARK_BOTH, BMARK_MARK_LEN) != 0))
					{
						strcpy(Tmp, BMARK_MARK_REMOTE);
						strcat(Tmp, Pos);
						AddBookMark(Tmp);
					}
					else
						AddBookMark(Pos);

					Pos += strlen(Pos) + 1;
				}
			}
		}
	}
	return;
}


/*----- ブックマーク編集ウインドウ --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		ステータス (YES=実行/NO=取消)
*----------------------------------------------------------------------------*/

int EditBookMark(void)
{
	int Sts;

	Sts = DialogBox(GetFtpInst(), MAKEINTRESOURCE(bmark_dlg), GetMainHwnd(), EditBookMarkProc);
	return(Sts);
}


/*----- ブックマーク編集ウインドウのコールバック ------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		UINT message : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		BOOL TRUE/FALSE
*----------------------------------------------------------------------------*/

// 64ビット対応
//static BOOL CALLBACK EditBookMarkProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK EditBookMarkProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	MENUITEMINFO mInfo;
	int Cur;
	int Max;
	char Tmp[BMARK_MARK_LEN + FMAX_PATH * 2 + BMARK_SEP_LEN + 1];

	static DIALOGSIZE DlgSize = {
		{ BMARK_NEW, BMARK_SET, BMARK_DEL, BMARK_DOWN, BMARK_UP, IDHELP, BMARK_SIZEGRIP, -1 },
		{ IDOK, BMARK_JUMP, BMARK_SIZEGRIP, -1 },
		{ BMARK_LIST, -1 },
		{ 0, 0 },
		{ 0, 0 }
	};

	switch (message)
	{
		case WM_INITDIALOG :
			if(ListFont != NULL)
				SendDlgItemMessage(hDlg, BMARK_LIST, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));

			hMenu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);
			Max = GetMenuItemCount(hMenu);
			for(Cur = DEFAULT_BMARK_ITEM; Cur < Max; Cur++)
			{
				mInfo.cbSize = sizeof(MENUITEMINFO);
				mInfo.fMask = MIIM_TYPE;
				mInfo.dwTypeData = Tmp;
				mInfo.cch = FMAX_PATH;
				if(GetMenuItemInfo(hMenu, Cur, TRUE, &mInfo) == TRUE)
					SendDlgItemMessage(hDlg, BMARK_LIST, LB_ADDSTRING, 0, (LPARAM)Tmp);
			}
			DlgSizeInit(hDlg, &DlgSize, &BmarkDlgSize);
		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case BMARK_JUMP :
					if((Cur = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
						PostMessage(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(Cur+MENU_BMARK_TOP, 0), 0);
					/* ここに break はない */

				case IDCANCEL :
				case IDOK :
					ClearBookMark();
					Max = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCOUNT, 0, 0);
					for(Cur = 0; Cur < Max; Cur++)
					{
						SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETTEXT, Cur, (LPARAM)Tmp);
						AddBookMark(Tmp);
					}
					AskDlgSize(hDlg, &DlgSize, &BmarkDlgSize);
					EndDialog(hDlg, YES);
					break;

				case BMARK_SET :
					if((Cur = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
					{
						SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETTEXT, Cur, (LPARAM)Tmp);
						if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(bmark_edit_dlg), hDlg, BookMarkEditCallBack, (LPARAM)Tmp) == YES)
						{
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_DELETESTRING, Cur, 0);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_INSERTSTRING, Cur, (LPARAM)Tmp);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_SETCURSEL, Cur, 0);
						}
					}
					break;

				case BMARK_NEW :
					strcpy(Tmp, "");
					if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(bmark_edit_dlg), hDlg, BookMarkEditCallBack, (LPARAM)Tmp) == YES)
					{
						SendDlgItemMessage(hDlg, BMARK_LIST, LB_ADDSTRING, 0, (LPARAM)Tmp);
						Cur = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCOUNT, 0, 0) - 1;
						SendDlgItemMessage(hDlg, BMARK_LIST, LB_SETCURSEL, Cur, 0);
					}
					break;

				case BMARK_DEL :
					if((Cur = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
					{
						SendDlgItemMessage(hDlg, BMARK_LIST, LB_DELETESTRING, Cur, 0);
						if(Cur >= SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCOUNT, 0, 0))
							Cur = max1(0, SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCOUNT, 0, 0)-1);
						if(SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCOUNT, 0, 0) > 0)
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_SETCURSEL, Cur, 0);
					}
					break;

				case BMARK_UP :
					if((Cur = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
					{
						if(Cur > 0)
						{
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETTEXT, Cur, (LPARAM)Tmp);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_DELETESTRING, Cur, 0);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_INSERTSTRING, --Cur, (LPARAM)Tmp);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_SETCURSEL, Cur, 0);
						}
					}
					break;

				case BMARK_DOWN :
					if((Cur = SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
					{
						if(Cur < SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETCOUNT, 0, 0)-1)
						{
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_GETTEXT, Cur, (LPARAM)Tmp);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_DELETESTRING, Cur, 0);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_INSERTSTRING, ++Cur, (LPARAM)Tmp);
							SendDlgItemMessage(hDlg, BMARK_LIST, LB_SETCURSEL, Cur, 0);
						}
					}
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000019);
					break;
			}
			return(TRUE);

		case WM_SIZING :
			DlgSizeChange(hDlg, &DlgSize, (RECT *)lParam, (int)wParam);
		    return(TRUE);

	}
    return(FALSE);
}


/*----- ブックマーク入力ダイアログのコールバック ------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		UINT message : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		BOOL TRUE/FALSE
*----------------------------------------------------------------------------*/

// 64ビット対応
//static BOOL CALLBACK BookMarkEditCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK BookMarkEditCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static char *Str;
	char *Path1;
	char *Path2;
	char Local[FMAX_PATH+1];
	char Remote[FMAX_PATH+1];
	int Num;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			Str = (char *)lParam;
			SendDlgItemMessage(hDlg, BEDIT_LOCAL, EM_LIMITTEXT, FMAX_PATH-1, 0);
			SendDlgItemMessage(hDlg, BEDIT_REMOTE, EM_LIMITTEXT, FMAX_PATH-1, 0);
			if(strlen(Str) > BMARK_MARK_LEN)
			{
				Num = GetBothPath(Str, &Path1, &Path2);
				if(strncmp(Str, BMARK_MARK_LOCAL, BMARK_MARK_LEN) == 0)
					SendDlgItemMessage(hDlg, BEDIT_LOCAL, WM_SETTEXT, 0, (LPARAM)Path1);
				else if(strncmp(Str, BMARK_MARK_REMOTE, BMARK_MARK_LEN) == 0)
				{
					SendDlgItemMessage(hDlg, BEDIT_REMOTE, WM_SETTEXT, 0, (LPARAM)Path1);
					/* ホスト側にカーソルを移動しておく */
					SetFocus(GetDlgItem(hDlg, BEDIT_REMOTE));
					SendDlgItemMessage(hDlg, BEDIT_REMOTE, EM_SETSEL, 0, -1);
					return(FALSE);
				}
				else if((strncmp(Str, BMARK_MARK_BOTH, BMARK_MARK_LEN) == 0) && (Num == 2))
				{
					SendDlgItemMessage(hDlg, BEDIT_LOCAL, WM_SETTEXT, 0, (LPARAM)Path1);
					SendDlgItemMessage(hDlg, BEDIT_REMOTE, WM_SETTEXT, 0, (LPARAM)Path2);
				}
			}
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, BEDIT_LOCAL, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Local);
					SendDlgItemMessage(hDlg, BEDIT_REMOTE, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Remote);
					if(strlen(Local) > 0)
					{
						if(strlen(Remote) > 0)
						{
							/* 両方 */
							strcpy(Str, BMARK_MARK_BOTH);
							strcat(Str, Local);
							strcat(Str, BMARK_SEP);
							strcat(Str, Remote);
						}
						else
						{
							/* ローカルのみ */
							strcpy(Str, BMARK_MARK_LOCAL);
							strcat(Str, Local);
						}
						EndDialog(hDlg, YES);
					}
					else if(strlen(Remote) > 0)
					{
						/* ホストのみ */
						strcpy(Str, BMARK_MARK_REMOTE);
						strcat(Str, Remote);
						EndDialog(hDlg, YES);
					}
					else
						EndDialog(hDlg, NO);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


