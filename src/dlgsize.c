/*=============================================================================
*
*					ダイアログボックスのサイズ変更処理
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

#define	STRICT
// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>
#include <windowsx.h>

#include "common.h"
#include "resource.h"


/*---------------------------------------------------------------------------*/
/* サイズ変更可能とするダイアログボックスは WS_CLIPCHILDREN スタイルを追加す */
/* ること */
/*---------------------------------------------------------------------------*/


/*----- ダイアログボックスの初期サイズを設定 ----------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		DIALOGSIZE *Dt : ダイアログサイズ設定パラメータ
*		SIZE *Size : ダイアログのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DlgSizeInit(HWND hDlg, DIALOGSIZE *Dt, SIZE *Size)
{
	RECT Rect;

	GetWindowRect(hDlg, &Rect);
	Dt->MinSize.cx = Rect.right - Rect.left;
	Dt->MinSize.cy = Rect.bottom - Rect.top;
	Dt->CurSize.cx = Dt->MinSize.cx;
	Dt->CurSize.cy = Dt->MinSize.cy;

	if(Size->cx != -1)
	{
		Rect.right = Rect.left + Size->cx;
		Rect.bottom = Rect.top + Size->cy;

		DlgSizeChange(hDlg, Dt, &Rect, WMSZ_BOTTOMRIGHT);

		GetWindowRect(hDlg, &Rect);
		MoveWindow(hDlg, Rect.left, Rect.top, Dt->CurSize.cx, Dt->CurSize.cy, TRUE);
	}
	return;
}


/*----- ダイアログボックスのサイズを返す --------------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		DIALOGSIZE *Dt : ダイアログサイズ設定パラメータ
*		SIZE *Size : ダイアログのサイズを返すワーク
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AskDlgSize(HWND hDlg, DIALOGSIZE *Dt, SIZE *Size)
{
	Size->cx = Dt->CurSize.cx;
	Size->cy = Dt->CurSize.cy;
	return;
}



/*----- ダイアログボックスのサイズ変更処理 ------------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		DIALOGSIZE *Dt : ダイアログサイズ設定パラメータ
*		RECT *New : 新しいダイアログのサイズ
*		int Flg : サイズ変更方向 (WMSZ_xxx)
*
*	Return Value
*		なし
*
*	Note
*		ダイアログボックスに WM_SIZING メッセージが来た時に呼ぶ事
*----------------------------------------------------------------------------*/

void DlgSizeChange(HWND hDlg, DIALOGSIZE *Dt, RECT *New, int Flg)
{
	int *Win;
	RECT Rect;
	POINT Point;

	/* 最少サイズより小さくならないようにする処理 */
	if((New->right - New->left) < Dt->MinSize.cx)
	{
		if((Flg == WMSZ_LEFT) || (Flg == WMSZ_TOPLEFT) || (Flg == WMSZ_BOTTOMLEFT))
			New->left = New->right - Dt->MinSize.cx;
		else
			New->right = New->left + Dt->MinSize.cx;
	}
	if((New->bottom - New->top) < Dt->MinSize.cy)
	{
		if((Flg == WMSZ_TOP) || (Flg == WMSZ_TOPLEFT) || (Flg == WMSZ_TOPRIGHT))
			New->top = New->bottom - Dt->MinSize.cy;
		else
			New->bottom = New->top + Dt->MinSize.cy;
	}

	/* 水平方向に移動する部品の処理 */
	if(Dt->CurSize.cx != New->right - New->left)
	{
		Win = Dt->HorMoveList;
		while(*Win != -1)
		{
			GetWindowRect(GetDlgItem(hDlg, *Win), &Rect);
			Point.x = Rect.left + (New->right - New->left) - Dt->CurSize.cx;
			Point.y = Rect.top;
			ScreenToClient(hDlg, &Point);

			GetClientRect(GetDlgItem(hDlg, *Win), &Rect);
			Rect.left = Point.x;
			Rect.top = Point.y;
			MoveWindow(GetDlgItem(hDlg, *Win), Rect.left, Rect.top, Rect.right, Rect.bottom, FALSE);

			Win++;
		}
	}

	/* 垂直方向に移動する部品の処理 */
	if(Dt->CurSize.cy != New->bottom - New->top)
	{
		Win = Dt->VarMoveList;
		while(*Win != -1)
		{
			GetWindowRect(GetDlgItem(hDlg, *Win), &Rect);
			Point.x = Rect.left;
			Point.y = Rect.top + (New->bottom - New->top) - Dt->CurSize.cy;
			ScreenToClient(hDlg, &Point);

			GetClientRect(GetDlgItem(hDlg, *Win), &Rect);
			Rect.left = Point.x;
			Rect.top = Point.y;
			MoveWindow(GetDlgItem(hDlg, *Win), Rect.left, Rect.top, Rect.right, Rect.bottom, FALSE);

			Win++;
		}
	}

	/* 大きさを変更する部品の処理 */
	if((Dt->CurSize.cx != New->right - New->left) ||
	   (Dt->CurSize.cy != New->bottom - New->top))
	{
		Win = Dt->ResizeList;
		while(*Win != -1)
		{
			GetWindowRect(GetDlgItem(hDlg, *Win), &Rect);
			Rect.right = (Rect.right - Rect.left) + (New->right - New->left) - Dt->CurSize.cx;
			Rect.bottom = (Rect.bottom - Rect.top) + (New->bottom - New->top) - Dt->CurSize.cy;

			Point.x = Rect.left;
			Point.y = Rect.top;
			ScreenToClient(hDlg, &Point);
			Rect.left = Point.x;
			Rect.top = Point.y;

			MoveWindow(GetDlgItem(hDlg, *Win), Rect.left, Rect.top, Rect.right, Rect.bottom, FALSE);

			Win++;
		}

	}

	Dt->CurSize.cx = New->right - New->left;
	Dt->CurSize.cy = New->bottom - New->top;
	InvalidateRect(hDlg, NULL, FALSE);

	return;
}


