/*=============================================================================
*
*							リストビューティップス
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

/* このソースは MFC Programmer's SourceBook (http://www.codeguru.com/)を参考にしました */

#include "common.h"


/*===== プロトタイプ =====*/

static void TipsShow(HWND hWnd, RECT rectTitle, LPCTSTR lpszTitleText, int xoffset, int xoffset2, int InRect);
static int CellRectFromPoint(HWND hWnd, POINT  point, RECT *cellrect, int *col);

/*===== ローカルなワーク =====*/

static HWND hWndTips;	/* tipsのウインドウハンドル */


// リストビューティップスのウインドウを作成
int InitListViewTips(HWND hWnd, HINSTANCE hInst) {
	WNDCLASSEXW classEx{ sizeof(WNDCLASSEXW), 0, DefWindowProcW, 0, 0, hInst, 0, 0, CreateSolidBrush(GetSysColor(COLOR_INFOBK)), nullptr, L"XTitleTip" };
	RegisterClassExW(&classEx);
	hWndTips = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"XTitleTip", nullptr, WS_BORDER | WS_POPUP, 0, 0, 0, 0, hWnd, 0, hInst, nullptr);
	return hWndTips ? FFFTP_SUCCESS : FFFTP_FAIL;
}


/*----- リストビューティップスのウインドウを削除 ------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteListViewTips(void)
{
	if(hWndTips != NULL)
		DestroyWindow(hWndTips);
	return;
}


/*----- リストビューティップスのウインドウを消去 ------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void EraseListViewTips(void)
{
	ReleaseCapture();
	ShowWindow(hWndTips, SW_HIDE);
	return;
}


/*----- リストビューティップスのウインドウハンドルを返す ----------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetListViewTipsHwnd(void)
{
	return(hWndTips);
}


/*----- リストビューティップスの表示チェック ----------------------------------
*
*	Parameter
*		HWND hWnd : ListViewのウインドウハンドル
*		LPARAM lParam : WM_MOUSEMOVEのLPARAM値
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void CheckTipsDisplay(HWND hWnd, LPARAM lParam)
{
	RECT cellrect;
	static RECT cur_rect;
	static int InRect;
	POINT Point;
	int row, col;
	int offset;
	int offset2;
	RECT rcLabel;
	char Buf[256];

	Point.x = LOWORD(lParam);
	Point.y = HIWORD(lParam);
	if(InRect == NO)
	{
		row = CellRectFromPoint(hWnd, Point, &cellrect, &col);
		if(row != -1)
		{
			cur_rect=cellrect;
			offset = 6;
			offset2 = offset;
			if( col == 0 ) 
			{
				ListView_GetItemRect(hWnd,  row, &rcLabel, LVIR_LABEL);
				offset = rcLabel.left - cellrect.left + offset / 2;
				offset2 = 1;
			}
			cellrect.top--;
			strcpy(Buf, "");
			ListView_GetItemText(hWnd,  row, col, Buf, 256 );
			if(strlen(Buf) > 0)
				TipsShow(hWnd, cellrect, Buf, offset-1, offset2-1, InRect);
			InRect = YES;
		}
	}
	else
	{
		if(PtInRect(&cur_rect, Point) == FALSE)
		{
			EraseListViewTips();
			InRect = NO;
		}
	}
	return;
}


/*----- リストビューティップスを表示 ------------------------------------------
*
*	Parameter
*		HWND hWnd : ListViewのウインドウのハンドル
*		RECT rectTitle : ListViewコントルールのアイテムの矩形
*		LPCTSTR lpszTitleText : 文字列
*		int xoffset : オフセット
*		int xoffset2 : オフセット２
*		int InRect : 表示するかどうか
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void TipsShow(HWND hWnd, RECT rectTitle, LPCTSTR lpszTitleText, int xoffset, int xoffset2, int InRect) {
	if (InRect != NO || GetFocus() == NULL)
		return;

	RectClientToScreen(hWnd, &rectTitle);
	auto wText = u8(lpszTitleText);
	auto font = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);

	auto dc = GetDC(hWndTips);
	auto saved = (HFONT)SelectObject(dc, font);
	SetTextColor(dc, GetSysColor(COLOR_INFOTEXT));
	SetBkMode(dc, TRANSPARENT);
	SIZE size;
	GetTextExtentPoint32W(dc, wText.c_str(), size_as<int>(wText), &size);
	if (rectTitle.right - rectTitle.left < size.cx + xoffset + xoffset2 + 2) {
		SetWindowPos(hWndTips, HWND_TOPMOST, rectTitle.left + xoffset, rectTitle.top, size.cx + 3, rectTitle.bottom - rectTitle.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
		TextOutW(dc, 0, 0, wText.c_str(), size_as<int>(wText));
		SetCapture(hWnd);
	}
	SelectObject(dc, saved);
	ReleaseDC(hWndTips, dc);
}


/*----- リストビューティップスを表示する位置を返す ----------------------------
*
*	Parameter
*		HWND hWnd : ListViewのウインドウハンドル
*		POINT point : カーソルの位置
*		RECT *cellrect : アイテムの矩形を返すワーク
*		int *col : 桁番号を返すワーク
*
*	Return Value
*		int 行番号
*			-1=該当なし
*----------------------------------------------------------------------------*/

static int CellRectFromPoint(HWND hWnd, POINT point, RECT *cellrect, int *col)
{
	int colnum;
	int nColumnCount;
	RECT rect;
	RECT rectClient;
	int row;
	int bottom;
	int colwidth;
	int Ret;

	Ret = -1;
	if((GetWindowLongPtrW(hWnd, GWL_STYLE) & LVS_TYPEMASK) == LVS_REPORT )
	{
		row = ListView_GetTopIndex(hWnd);
		bottom = row + ListView_GetCountPerPage(hWnd);
		if(bottom > ListView_GetItemCount(hWnd))
			bottom = ListView_GetItemCount(hWnd);

		nColumnCount = Header_GetItemCount(GetDlgItem(hWnd, 0));

		for(; row <= bottom; row++)
		{
			ListView_GetItemRect(hWnd, row, &rect, LVIR_BOUNDS);
			if(PtInRect(&rect, point))
			{
				for(colnum = 0; colnum < nColumnCount; colnum++)
				{
					colwidth = ListView_GetColumnWidth(hWnd, colnum);
					if((point.x >= rect.left) && 
					   (point.x < (rect.left + colwidth)))
					{
						GetClientRect(hWnd, &rectClient);
						if(point.x <= rectClient.right)
						{
							rect.right = rect.left + colwidth;
							if(rect.right > rectClient.right)
								rect.right = rectClient.right;
							*cellrect = rect;
							*col = colnum;
							Ret = row;
							break;
						}
					}
					rect.left += colwidth;
				}
				break;
			}
		}
	}
	return(Ret);
}
