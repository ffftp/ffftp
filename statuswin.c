/*=============================================================================
*
*							ステータスウインドウ
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


/*===== ローカルなワーク =====*/

static HWND hWndSbar = NULL;
static int SbarColWidth[5] = { 70, 230, 410, 570, -1 };



/*----- ステータスウインドウを作成する ----------------------------------------
*
*	Parameter
*		HWND hWnd : 親ウインドウのウインドウハンドル
*		HINSTANCE hInst : インスタンスハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeStatusBarWindow(HWND hWnd, HINSTANCE hInst)
{
	int Sts;
	// 高DPI対応
	int i;

	Sts = FFFTP_FAIL;
	hWndSbar = CreateWindowEx(0,
			STATUSCLASSNAME, NULL,
			WS_CHILD | SBS_SIZEGRIP | WS_CLIPSIBLINGS | SBT_NOBORDERS,
			0, 0, 0, 0,
			hWnd, (HMENU)1500, hInst, NULL);

	if(hWndSbar != NULL)
	{
		// 高DPI対応
		for(i = 0; i < sizeof(SbarColWidth) / sizeof(int) - 1; i++)
			SbarColWidth[i] = CalcPixelX(SbarColWidth[i]);
		SendMessage(hWndSbar, SB_SETPARTS, sizeof(SbarColWidth)/sizeof(int), (LPARAM)SbarColWidth);
		ShowWindow(hWndSbar, SW_SHOW);
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- ステータスウインドウを削除 --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteStatusBarWindow(void)
{
	if(hWndSbar != NULL)
		DestroyWindow(hWndSbar);
	return;
}


/*----- ステータスウインドウのウインドウハンドルを返す ------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetSbarWnd(void)
{
	return(hWndSbar);
}


/*----- カレントウインドウを表示 ----------------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx : -1=なし)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispCurrentWindow(int Win)
{
	if(Win == WIN_LOCAL)
		SendMessage(GetSbarWnd(), SB_SETTEXT, 0 | 0, (LPARAM)MSGJPN245);
	else if(Win == WIN_REMOTE)
		SendMessage(GetSbarWnd(), SB_SETTEXT, 0 | 0, (LPARAM)MSGJPN246);
	else
		SendMessage(GetSbarWnd(), SB_SETTEXT, 0 | 0, (LPARAM)"");
	return;
}


/*----- 選択されているファイル数とサイズを表示 --------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispSelectedSpace(void)
{
	char Buf1[50];
	char Buf2[50];
	int Win;

	Win = WIN_LOCAL;
	if(GetFocus() == GetRemoteHwnd())
		Win = WIN_REMOTE;

	MakeSizeString(GetSelectedTotalSize(Win), Buf1);
	sprintf(Buf2, MSGJPN247, GetSelectedCount(Win), Buf1);
	SendMessage(GetSbarWnd(), SB_SETTEXT, 1 | 0, (LPARAM)Buf2);
	return;
}


/*----- ローカル側の空き容量を表示 --------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispLocalFreeSpace(char *Path)
{
	char Buf[40];

	sprintf(Buf, MSGJPN248, AskLocalFreeSpace(Path));
	SendMessage(GetSbarWnd(), SB_SETTEXT, 2 | 0, (LPARAM)Buf);
	return;
}


/*----- 転送するファイルの数を表示 --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispTransferFiles(void)
{
	char Buf[50];

	sprintf(Buf, MSGJPN249, AskTransferFileNum());
	SendMessage(GetSbarWnd(), SB_SETTEXT, 3 | 0, (LPARAM)Buf);
	return;
}


/*----- 受信中のバイト数を表示 ------------------------------------------------
*
*	Parameter
*		LONGLONG Size : バイト数 (-1=表示を消す)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispDownloadSize(LONGLONG Size)
{
	char Buf[50];
	char Tmp[50];

	strcpy(Buf, "");
	if(Size >= 0)
	{
		MakeSizeString((double)Size, Tmp);
		sprintf(Buf, MSGJPN250, Tmp);
	}
	SendMessage(GetSbarWnd(), SB_SETTEXT, 4 | 0, (LPARAM)Buf);
	return;
}


