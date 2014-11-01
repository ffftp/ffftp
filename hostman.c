/*=============================================================================
*
*								ホスト一覧
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

#include <htmlhelp.h>
#include "helpid.h"

// UTF-8対応
#undef __MBSWRAPPER_H__
#include "mbswrapper.h"


/*===== プロトタイプ =====*/

// 64ビット対応
//static BOOL CALLBACK SelectHostProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK SelectHostProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK HostListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static HOSTLISTDATA *GetNextNode(HOSTLISTDATA *Pos);
static int GetNodeLevel(int Num);
static int GetNodeLevelByData(HOSTLISTDATA *Data);
static int GetNodeNumByData(HOSTLISTDATA *Data);
static HOSTLISTDATA *GetNodeByNum(int Num);
static int SetNodeLevelAll(void);
static int UpdateHostToList(int Num, HOSTDATA *Set);
static int DelHostFromList(int Num);
static int DeleteChildAndNext(HOSTLISTDATA *Pos);
static void SendAllHostNames(HWND hWnd, int Cur);
static int IsNodeGroup(int Num);
static int DispHostSetDlg(HWND hDlg);
// 64ビット対応
//static BOOL CALLBACK MainSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK AdvSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK CodeSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK DialupSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK Adv2SettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK MainSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK AdvSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK CodeSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DialupSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK Adv2SettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
// 暗号化通信対応
static INT_PTR CALLBACK CryptSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
// 同時接続対応
static INT_PTR CALLBACK Adv3SettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

/*===== 外部参照 =====*/

extern HWND hHelpWin;

/* 設定値 */
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern HFONT ListFont;
extern char DefaultLocalPath[FMAX_PATH+1];
extern int ConnectAndSet;
extern SIZE HostDlgSize;

/*===== ローカルなワーク =====*/

static int Hosts = 0;						/* ホスト数 */
static int ConnectingHost;					/* 接続中のホスト */
static int CurrentHost;						/* カーソル位置のホスト */
static HOSTLISTDATA *HostListTop = NULL;	/* ホスト一覧データ */
static HOSTDATA TmpHost;					/* ホスト情報コピー用 */
static int Apply;							/* プロパティシートでOKを押したフラグ */
static WNDPROC HostListProcPtr;

// ホスト共通設定機能
HOSTDATA DefaultHost;



/*----- ホスト一覧ウインドウ --------------------------------------------------
*
*	Parameter
*		int Type : ダイアログのタイプ (DLG_TYPE_xxx)
*
*	Return Value
*		ステータス (YES=実行/NO=取消)
*----------------------------------------------------------------------------*/

int SelectHost(int Type)
{
	int Sts;
	int Dlg;

	Dlg = hostconnect_dlg;
	if((ConnectAndSet == YES) || (Type == DLG_TYPE_SET))
		Dlg = hostlist_dlg;

	Sts = DialogBox(GetFtpInst(), MAKEINTRESOURCE(Dlg), GetMainHwnd(), SelectHostProc);

	/* ホスト設定を保存 */
	SetNodeLevelAll();
	SaveRegistry();

	return(Sts);
}


/*----- ホスト一覧ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK SelectHostProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK SelectHostProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static DIALOGSIZE DlgSize = {
		{ HOST_NEW, HOST_FOLDER, HOST_SET, HOST_COPY, HOST_DEL, HOST_DOWN, HOST_UP, IDHELP, HOST_SIZEGRIP, -1 },
		{ IDOK, IDCANCEL, HOST_SIZEGRIP, -1 },
		{ HOST_LIST, -1 },
		{ 0, 0 },
		{ 0, 0 }
	};
	static HIMAGELIST hImage;
	HTREEITEM hItem;
	TV_ITEM Item;
	int Level1;
	int Level2;
	HOSTLISTDATA *Data1;
	HOSTLISTDATA *Data2;
	// UTF-8対応
//	NM_TREEVIEW *tView;
	NM_TREEVIEWW *tView;
	HTREEITEM tViewPos;
	TV_HITTESTINFO HitInfo;

	switch (message)
	{
		case WM_INITDIALOG :
			/* TreeViewでのダブルクリックをつかまえるため */
			// 64ビット対応
//			HostListProcPtr = (WNDPROC)SetWindowLong(GetDlgItem(hDlg, HOST_LIST), GWL_WNDPROC, (LONG)HostListWndProc);
			HostListProcPtr = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, HOST_LIST), GWLP_WNDPROC, (LONG_PTR)HostListWndProc);


//		SetClassLong(hDlg, GCL_HICON, (LONG)LoadIcon(GetFtpInst(), MAKEINTRESOURCE(ffftp)));

			if(AskConnecting() == YES)
			{
				/* 接続中は「変更」のみ許可 */
				EnableWindow(GetDlgItem(hDlg, HOST_NEW), FALSE);
				EnableWindow(GetDlgItem(hDlg, HOST_FOLDER), FALSE);
				EnableWindow(GetDlgItem(hDlg, HOST_COPY), FALSE);
				EnableWindow(GetDlgItem(hDlg, HOST_DEL), FALSE);
				EnableWindow(GetDlgItem(hDlg, HOST_DOWN), FALSE);
				EnableWindow(GetDlgItem(hDlg, HOST_UP), FALSE);
			}
			if(ListFont != NULL)
				SendDlgItemMessage(hDlg, HOST_LIST, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
			hImage = ImageList_LoadBitmap(GetFtpInst(), MAKEINTRESOURCE(hlist_bmp), 16, 8, RGB(255,0,0));
			SendDlgItemMessage(hDlg, HOST_LIST, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImage);
			CurrentHost = 0;
			if(ConnectingHost >= 0)
				CurrentHost = ConnectingHost;
			SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
			DlgSizeInit(hDlg, &DlgSize, &HostDlgSize);
		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
					{
						Item.hItem = hItem;
						Item.mask = TVIF_PARAM;
						SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
						CurrentHost = Item.lParam;
						ConnectingHost = CurrentHost;
						AskDlgSize(hDlg, &DlgSize, &HostDlgSize);
						ImageList_Destroy(hImage);
						EndDialog(hDlg, YES);
						break;
					}
					/* ここにbreakはない */

				case IDCANCEL :
					AskDlgSize(hDlg, &DlgSize, &HostDlgSize);
					ImageList_Destroy(hImage);
					EndDialog(hDlg, NO);
					break;

				case HOST_NEW :
					CopyDefaultHost(&TmpHost);
					if(DispHostSetDlg(hDlg) == YES)
					{
						if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
						{
							Item.hItem = hItem;
							Item.mask = TVIF_PARAM;
							SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);

							TmpHost.Level = GetNodeLevel(Item.lParam);
							Level1 = Item.lParam + 1;
							CurrentHost = Level1;
						}
						else
						{
							TmpHost.Level = 0;
							Level1 = -1;
							CurrentHost = Hosts;
						}
						AddHostToList(&TmpHost, Level1, SET_LEVEL_SAME);
						SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
					}
					break;

				case HOST_FOLDER :
					CopyDefaultHost(&TmpHost);
					if(InputDialogBox(group_dlg, hDlg, NULL, TmpHost.HostName, HOST_NAME_LEN+1, &Level1, IDH_HELP_TOPIC_0000001) == YES)
					{
						if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
						{
							Item.hItem = hItem;
							Item.mask = TVIF_PARAM;
							SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);

							TmpHost.Level = GetNodeLevel(Item.lParam) | SET_LEVEL_GROUP ;
							Level1 = Item.lParam + 1;
							CurrentHost = Level1;
						}
						else
						{
							TmpHost.Level = 0 | SET_LEVEL_GROUP;
							Level1 = -1;
							CurrentHost = Hosts;
						}
						AddHostToList(&TmpHost, Level1, SET_LEVEL_SAME);
						SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
					}
					break;

				case HOST_SET :
					if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
					{
						Item.hItem = hItem;
						Item.mask = TVIF_PARAM;
						SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
						CurrentHost = Item.lParam;

						CopyHostFromList(CurrentHost, &TmpHost);
						Level1 = IsNodeGroup(CurrentHost);
						if(((Level1 == NO) && (DispHostSetDlg(hDlg) == YES)) ||
						   ((Level1 == YES) && (InputDialogBox(group_dlg, hDlg, NULL, TmpHost.HostName, HOST_NAME_LEN+1, &Level1, IDH_HELP_TOPIC_0000001) == YES)))
						{
							UpdateHostToList(CurrentHost, &TmpHost);
							SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
						}
					}
					break;

				case HOST_COPY :
					if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
					{
						Item.hItem = hItem;
						Item.mask = TVIF_PARAM;
						SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
						CurrentHost = Item.lParam;

						CopyHostFromList(CurrentHost, &TmpHost);
						strcpy(TmpHost.BookMark, "\0");
						CurrentHost++;
						AddHostToList(&TmpHost, CurrentHost, SET_LEVEL_SAME);
						SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
					}
					break;

				case HOST_DEL :
					if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
					{
						Item.hItem = hItem;
						Item.mask = TVIF_PARAM;
						SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
						CurrentHost = Item.lParam;
						Level1 = IsNodeGroup(CurrentHost);

						// バグ修正
//						if(((Level1 == YES) && (DialogBox(GetFtpInst(), MAKEINTRESOURCE(groupdel_dlg), GetMainHwnd(), ExeEscDialogProc) == YES)) ||
//						   ((Level1 == NO) && (DialogBox(GetFtpInst(), MAKEINTRESOURCE(hostdel_dlg), GetMainHwnd(), ExeEscDialogProc) == YES)))
						if(((Level1 == YES) && (DialogBox(GetFtpInst(), MAKEINTRESOURCE(groupdel_dlg), hDlg, ExeEscDialogProc) == YES)) ||
						   ((Level1 == NO) && (DialogBox(GetFtpInst(), MAKEINTRESOURCE(hostdel_dlg), hDlg, ExeEscDialogProc) == YES)))
						{
							DelHostFromList(CurrentHost);
							if(CurrentHost >= Hosts)
								CurrentHost = max1(0, Hosts-1);
							SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
						}
					}
					break;

				case HOST_UP :
					if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
					{
						Item.hItem = hItem;
						Item.mask = TVIF_PARAM;
						SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
						CurrentHost = Item.lParam;

						if(CurrentHost > 0)
						{
							Data1 = HostListTop;
							for(Level1 = CurrentHost; Level1 > 0; Level1--)
								Data1 = GetNextNode(Data1);
							Level1 = GetNodeLevel(CurrentHost);

							Data2 = HostListTop;
							for(Level2 = CurrentHost-1; Level2 > 0; Level2--)
								Data2 = GetNextNode(Data2);
							Level2 = GetNodeLevel(CurrentHost-1);

							if((Level1 == Level2) && (Data2->Set.Level & SET_LEVEL_GROUP))
							{
								//Data2のchildへ
								if(Data1->Next != NULL)
									Data1->Next->Prev = Data1->Prev;
								if(Data1->Prev != NULL)
									Data1->Prev->Next = Data1->Next;
								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
									Data1->Parent->Child = Data1->Next;
								if((Data1->Parent == NULL) && (HostListTop == Data1))
									HostListTop = Data1->Next;

								Data1->Next = Data2->Child;
								Data1->Prev = NULL;
								Data1->Parent = Data2;
								Data2->Child = Data1;
							}
							else if(Level1 < Level2)
							{
								//Data1のPrevのChildのNextの末尾へ
								Data2 = Data1->Prev->Child;
								while(Data2->Next != NULL)
									Data2 = Data2->Next;

								if(Data1->Next != NULL)
									Data1->Next->Prev = Data1->Prev;
								if(Data1->Prev != NULL)
									Data1->Prev->Next = Data1->Next;
								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
									Data1->Parent->Child = Data1->Next;
								if((Data1->Parent == NULL) && (HostListTop == Data1))
									HostListTop = Data1->Next;

								Data2->Next = Data1;
								Data1->Prev = Data2;
								Data1->Next = NULL;
								Data1->Parent = Data2->Parent;
							}
							else
							{
								//Data2のprevへ
								if(Data1->Next != NULL)
									Data1->Next->Prev = Data1->Prev;
								if(Data1->Prev != NULL)
									Data1->Prev->Next = Data1->Next;
								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
									Data1->Parent->Child = Data1->Next;
								if((Data1->Parent == NULL) && (HostListTop == Data1))
									HostListTop = Data1->Next;

								if(Data2->Prev != NULL)
									Data2->Prev->Next = Data1;
								Data1->Prev = Data2->Prev;
								Data2->Prev = Data1;
								Data1->Next = Data2;
								Data1->Parent = Data2->Parent;

								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data2))
									Data1->Parent->Child = Data1;
								if((Data1->Parent == NULL) && (HostListTop == Data2))
									HostListTop = Data1;
							}

							CurrentHost = GetNodeNumByData(Data1);
							SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
						}
					}
					break;

				case HOST_DOWN :
					if((hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) != NULL)
					{
						Item.hItem = hItem;
						Item.mask = TVIF_PARAM;
						SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
						CurrentHost = Item.lParam;

						Data1 = HostListTop;
						for(Level1 = CurrentHost; Level1 > 0; Level1--)
							Data1 = GetNextNode(Data1);
						Level1 = GetNodeLevel(CurrentHost);

						Data2 = NULL;
						Level2 = SET_LEVEL_SAME;
						if(CurrentHost < Hosts-1)
						{
							Data2 = HostListTop;
							for(Level2 = CurrentHost+1; Level2 > 0; Level2--)
								Data2 = GetNextNode(Data2);
							Level2 = GetNodeLevel(CurrentHost+1);

							if(Level1 < Level2)
							{
								if(Data1->Next != NULL)
								{
									//Data2 = Data1のNext
									Data2 = Data1->Next;
									Level2 = GetNodeLevelByData(Data2);
								}
								else if(Data1->Parent != NULL)
								{
									Data2 = NULL;
									Level2 = SET_LEVEL_SAME;
								}
							}
						}

						if(((Data2 == NULL) && (Level1 > 0)) ||
						   (Level1 > Level2))
						{
							//Data1のParentのNextへ
							Data2 = Data1->Parent;

							if(Data1->Next != NULL)
								Data1->Next->Prev = Data1->Prev;
							if(Data1->Prev != NULL)
								Data1->Prev->Next = Data1->Next;
							if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
								Data1->Parent->Child = Data1->Next;
							if((Data1->Parent == NULL) && (HostListTop == Data1))
								HostListTop = Data1->Next;

							if(Data2->Next != NULL)
								Data2->Next->Prev = Data1;
							Data1->Next = Data2->Next;
							Data2->Next = Data1;
							Data1->Prev = Data2;
							Data1->Parent = Data2->Parent;

							if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
								Data1->Parent->Child = Data2;
							if((Data1->Parent == NULL) && (HostListTop == Data1))
								HostListTop = Data2;
						}
						else if(Level1 == Level2)
						{
							if(Data2->Set.Level & SET_LEVEL_GROUP)
							{
								//Data2のChildへ
								if(Data1->Next != NULL)
									Data1->Next->Prev = Data1->Prev;
								if(Data1->Prev != NULL)
									Data1->Prev->Next = Data1->Next;
								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
									Data1->Parent->Child = Data1->Next;
								if((Data1->Parent == NULL) && (HostListTop == Data1))
									HostListTop = Data1->Next;

								if(Data2->Child != NULL)
									Data2->Child->Prev = Data1;
								Data1->Next = Data2->Child;
								Data1->Prev = NULL;
								Data1->Parent = Data2;
								Data2->Child = Data1;
							}
							else
							{
								//Data2のNextへ
								if(Data1->Next != NULL)
									Data1->Next->Prev = Data1->Prev;
								if(Data1->Prev != NULL)
									Data1->Prev->Next = Data1->Next;
								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
									Data1->Parent->Child = Data1->Next;
								if((Data1->Parent == NULL) && (HostListTop == Data1))
									HostListTop = Data1->Next;

								if(Data2->Next != NULL)
									Data2->Next->Prev = Data1;
								Data1->Next = Data2->Next;
								Data2->Next = Data1;
								Data1->Prev = Data2;
								Data1->Parent = Data2->Parent;

								if((Data1->Parent != NULL) && (Data1->Parent->Child == Data1))
									Data1->Parent->Child = Data2;
								if((Data1->Parent == NULL) && (HostListTop == Data1))
									HostListTop = Data2;
							}
						}

						CurrentHost = GetNodeNumByData(Data1);
						SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
					}
					break;

				// ホスト共通設定機能
				case HOST_SET_DEFAULT :
					CopyDefaultHost(&TmpHost);
					if(DispHostSetDlg(hDlg) == YES)
						SetDefaultHost(&TmpHost);
					break;

				case HOST_LIST :
					if(HIWORD(wParam) == LBN_DBLCLK)
						PostMessage(hDlg, WM_COMMAND, MAKEWORD(IDOK, 0), 0);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000027);
					break;
			}
			SetFocus(GetDlgItem(hDlg, HOST_LIST));
			return(TRUE);

		case WM_SIZING :
			DlgSizeChange(hDlg, &DlgSize, (RECT *)lParam, (int)wParam);
		    return(TRUE);

		case WM_SELECT_HOST :
			HitInfo.pt.x = LOWORD(lParam);
			HitInfo.pt.y = HIWORD(lParam);
			HitInfo.flags = TVHT_ONITEM;
			hItem = (HTREEITEM)SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0);
			HitInfo.hItem = hItem;
			if((HTREEITEM)SendMessage(GetDlgItem(hDlg, HOST_LIST), TVM_HITTEST, 0, (LPARAM)&HitInfo) == hItem)
			{
				if(IsWindowEnabled(GetDlgItem(hDlg, IDOK)) == TRUE)
					PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
			}
			break;

		case WM_NOTIFY:
			// UTF-8対応
//			tView = (NM_TREEVIEW FAR *)lParam;
			tView = (NM_TREEVIEWW FAR *)lParam;
			switch(tView->hdr.idFrom)
			{
				case HOST_LIST :
					tViewPos = tView->itemNew.hItem;
					hItem = tView->itemNew.hItem;
					switch(tView->hdr.code)
					{
						// UTF-8対応
//						case TVN_SELCHANGED :
						case TVN_SELCHANGEDW :
							/* フォルダが選ばれたときは接続、コピーボタンは禁止 */
							Item.hItem = hItem;
							Item.mask = TVIF_PARAM;
							SendDlgItemMessage(hDlg, HOST_LIST, TVM_GETITEM, TVGN_CARET, (LPARAM)&Item);
							if(IsNodeGroup(Item.lParam) == YES)
							{
								EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
								EnableWindow(GetDlgItem(hDlg, HOST_COPY), FALSE);
							}
							else
							{
								EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
								if(AskConnecting() == NO)
									EnableWindow(GetDlgItem(hDlg, HOST_COPY), TRUE);
							}
							break;
					}
					break;
			}
			break;
	}
    return(FALSE);
}


/*----- ホスト一覧TreeViewのメッセージ処理 ------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		UINT message  : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		メッセージに対応する戻り値
*----------------------------------------------------------------------------*/

static LRESULT CALLBACK HostListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_LBUTTONDBLCLK :
			PostMessage(GetParent(hWnd), WM_SELECT_HOST, 0, lParam);
			break;
	}
	return(CallWindowProc(HostListProcPtr, hWnd, message, wParam, lParam));
}


/*----- 次の設定番号のノードを返す --------------------------------------------
*
*	Parameter
*		HOSTLISTDATA *Pos : ノードデータ
*
*	Return Value
*		HOSTLISTDATA *次のノード
*			NULL=次はない
*----------------------------------------------------------------------------*/

static HOSTLISTDATA *GetNextNode(HOSTLISTDATA *Pos)
{
	HOSTLISTDATA *Ret;

	Ret = NULL;
	if(Pos->Child != NULL)
		Ret = Pos->Child;
	else
	{
		if(Pos->Next != NULL)
			Ret = Pos->Next;
		else
		{
			while(Pos->Parent != NULL)
			{
				Pos = Pos->Parent;
				if(Pos->Next != NULL)
				{
					Ret = Pos->Next;
					break;
				}
			}
		}
	}
	return(Ret);
}


/*----- ノードのレベル数を返す（設定番号指定） --------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*
*	Return Value
*		int レベル数 (-1=設定がない）
*----------------------------------------------------------------------------*/

static int GetNodeLevel(int Num)
{
	int Ret;
	HOSTLISTDATA *Pos;

	Ret = -1;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		Ret = 0;
		while(Pos->Parent != NULL)
		{
			Pos = Pos->Parent;
			Ret++;
		}
	}
	return(Ret);
}


/*----- ノードのレベル数を返す（ノードデータ指定）-----------------------------
*
*	Parameter
*		HOSTLISTDATA *Data : 設定値
*
*	Return Value
*		int レベル数
*----------------------------------------------------------------------------*/

static int GetNodeLevelByData(HOSTLISTDATA *Data)
{
	int Ret;

	Ret = 0;
	while(Data->Parent != NULL)
	{
		Data = Data->Parent;
		Ret++;
	}
	return(Ret);
}


/*----- ノードの設定番号を返す ------------------------------------------------
*
*	Parameter
*		HOSTLISTDATA *Data : 設定値
*
*	Return Value
*		int 設定番号
*----------------------------------------------------------------------------*/

static int GetNodeNumByData(HOSTLISTDATA *Data)
{
	int Ret;
	HOSTLISTDATA *Pos;

	Ret = 0;
	Pos = HostListTop;
	while(Pos != Data)
	{
		Pos = GetNextNode(Pos);
		Ret++;
	}
	return(Ret);
}


/*----- 指定番号のノードを返す ------------------------------------------------
*
*	Parameter
*		int Num : 設定番号
*
*	Return Value
*		HOSTLISTDATA * : 設定値
*----------------------------------------------------------------------------*/

static HOSTLISTDATA *GetNodeByNum(int Num)
{
	HOSTLISTDATA *Pos;

	Pos = HostListTop;
	for(; Num > 0; Num--)
		Pos = GetNextNode(Pos);

	return(Pos);
}


/*----- 設定値リストの各ノードのレベル番号をセット ----------------------------
*
*	Parameter
*		int Num : 設定番号
*
*	Return Value
*		HOSTLISTDATA * : 設定値
*----------------------------------------------------------------------------*/

static int SetNodeLevelAll(void)
{
	HOSTLISTDATA *Pos;
	int i;

	Pos = HostListTop;
	for(i = 0; i < Hosts; i++)
	{
		Pos->Set.Level &= ~SET_LEVEL_MASK;
		Pos->Set.Level |= GetNodeLevelByData(Pos);
		Pos = GetNextNode(Pos);
	}
	return(FFFTP_SUCCESS);
}


/*----- 設定値リストに追加 ----------------------------------------------------
*
*	Parameter
*		HOSTDATA *Set : 追加する設定値
*		int Pos : 追加する位置 (0～ : -1=最後)
*		int Level : レベル数 (SET_LEVEL_SAME=追加位置のものと同レベル)
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int AddHostToList(HOSTDATA *Set, int Pos, int Level)
{
	int Sts;
	HOSTLISTDATA *New;
	HOSTLISTDATA *Last;
	int Cur;

	Sts = FFFTP_FAIL;
	if((Pos >= -1) && (Pos <= Hosts))
	{
		if(Pos == -1)
			Pos = Hosts;
		Level &= SET_LEVEL_MASK;

		if((New = malloc(sizeof(HOSTLISTDATA))) != NULL)
		{
			memcpy(&New->Set, Set, sizeof(HOSTDATA));
			New->Next = NULL;
			New->Prev = NULL;
			New->Child = NULL;
			New->Parent = NULL;

			if(HostListTop == NULL)
			{
				if(Pos == 0)
					HostListTop = New;
			}
			else
			{
				if(Pos == 0)
				{
					New->Next = HostListTop;
					HostListTop = New;
				}
				else
				{
					Cur = GetNodeLevel(Pos-1);
					Last = HostListTop;
					for(Pos--; Pos > 0; Pos--)
						Last = GetNextNode(Last);
					if((Level != SET_LEVEL_SAME) && (Level > Cur))
					{
						New->Next = Last->Child;
						New->Parent = Last;
						Last->Child = New;
						if(New->Next != NULL)
							New->Next->Prev = New;
					}
					else
					{
						if((Level >= 0) && (Level < SET_LEVEL_SAME))
						{
							for(; Level < Cur; Cur--)
								Last = Last->Parent;
						}
						New->Prev = Last;
						New->Next = Last->Next;
						New->Parent = Last->Parent;
						Last->Next = New;
						if(New->Next != NULL)
							New->Next->Prev = New;
					}
				}
			}
			Hosts++;
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- 設定値リストを更新する ------------------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		HOSTDATA *Set : 設定値をコピーするワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int UpdateHostToList(int Num, HOSTDATA *Set)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		memcpy(&Pos->Set, Set, sizeof(HOSTDATA));
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 設定値リストから削除 --------------------------------------------------
*
*	Parameter
*		int Num : 削除する番号
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int DelHostFromList(int Num)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		if(Num == 0)
		{
			Pos = HostListTop;
			if(Pos->Child != NULL)
				DeleteChildAndNext(Pos->Child);
			HostListTop = Pos->Next;
		}
		else
		{
			Pos = GetNodeByNum(Num);
			if(Pos->Child != NULL)
				DeleteChildAndNext(Pos->Child);

			if(Pos->Next != NULL)
				Pos->Next->Prev = Pos->Prev;
			if(Pos->Prev != NULL)
				Pos->Prev->Next = Pos->Next;
			if((Pos->Parent != NULL) && (Pos->Parent->Child == Pos))
				Pos->Parent->Child = Pos->Next;
		}
		free(Pos);
		Hosts--;
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 設定値リストからノードデータを削除 ------------------------------------
*
*	Parameter
*		HOSTLISTDATA *Pos : 削除するノード
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*
*	Note
*		Pos->Next, Pos->Childの全てのノードを削除する
*----------------------------------------------------------------------------*/

static int DeleteChildAndNext(HOSTLISTDATA *Pos)
{
	HOSTLISTDATA *Next;

	while(Pos != NULL)
	{
		if(Pos->Child != NULL)
			DeleteChildAndNext(Pos->Child);

		Next = Pos->Next;
		free(Pos);
		Hosts--;
		Pos = Next;
	}
	return(FFFTP_SUCCESS);
}


/*----- 設定値リストから設定値を取り出す --------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		HOSTDATA *Set : 設定値をコピーするワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*
*	Note
*		現在ホストに接続中の時は、CopyHostFromListInConnect() を使う事
*----------------------------------------------------------------------------*/

int CopyHostFromList(int Num, HOSTDATA *Set)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		memcpy(Set, &Pos->Set, sizeof(HOSTDATA));
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 設定値リストから設定値を取り出す --------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		HOSTDATA *Set : 設定値をコピーするワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*
*	Note
*		現在ホストに接続中の時に使う
*----------------------------------------------------------------------------*/

int CopyHostFromListInConnect(int Num, HOSTDATA *Set)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		strcpy(Set->ChmodCmd, Pos->Set.ChmodCmd);
		Set->Port = Pos->Set.Port;
		Set->Anonymous = Pos->Set.Anonymous;
		Set->KanjiCode = Pos->Set.KanjiCode;
		Set->KanaCnv = Pos->Set.KanaCnv;
		Set->NameKanjiCode = Pos->Set.NameKanjiCode;
		Set->NameKanaCnv = Pos->Set.NameKanaCnv;
		Set->Pasv = Pos->Set.Pasv;
		Set->FireWall = Pos->Set.FireWall;
		Set->ListCmdOnly = Pos->Set.ListCmdOnly;
		Set->UseNLST_R = Pos->Set.UseNLST_R;
		Set->LastDir = Pos->Set.LastDir;
		Set->TimeZone = Pos->Set.TimeZone;
		// 暗号化通信対応
		Set->UseNoEncryption = Pos->Set.UseNoEncryption;
		Set->UseFTPES = Pos->Set.UseFTPES;
		Set->UseFTPIS = Pos->Set.UseFTPIS;
		Set->UseSFTP = Pos->Set.UseSFTP;
		strcpy(Set->PrivateKey, Pos->Set.PrivateKey);
		Set->NoWeakEncryption = Pos->Set.NoWeakEncryption;
		// 同時接続対応
		Set->MaxThreadCount = Pos->Set.MaxThreadCount;
		Set->ReuseCmdSkt = Pos->Set.ReuseCmdSkt;
		// MLSD対応
		Set->UseMLSD = Pos->Set.UseMLSD;
		// IPv6対応
		Set->NetType = Pos->Set.NetType;
		// 自動切断対策
		Set->NoopInterval = Pos->Set.NoopInterval;
		// 再転送対応
		Set->TransferErrorMode = Pos->Set.TransferErrorMode;
		Set->TransferErrorNotify = Pos->Set.TransferErrorNotify;
		// セッションあたりの転送量制限対策
		Set->TransferErrorReconnect = Pos->Set.TransferErrorReconnect;
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 設定値リストのブックマークを更新 --------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		char *Bmask : ブックマーク文字列
*		int Len : ブックマーク文字列の長さ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int SetHostBookMark(int Num, char *Bmask, int Len)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		memcpy(Pos->Set.BookMark, Bmask, Len);
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 設定値リストのブックマーク文字列を返す --------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*
*	Return Value
*		char *ブックマーク文字列
*----------------------------------------------------------------------------*/

char *AskHostBookMark(int Num)
{
	char *Ret;
	HOSTLISTDATA *Pos;

	Ret = NULL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		Ret = Pos->Set.BookMark;
	}
	return(Ret);
}


/*----- 設定値リストのディレクトリを更新 --------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		char *LocDir : ローカルのディレクトリ
*		char *HostDir : ホストのディレクトリ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int SetHostDir(int Num, char *LocDir, char *HostDir)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		strcpy(Pos->Set.LocalInitDir, LocDir);
		strcpy(Pos->Set.RemoteInitDir, HostDir);
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 設定値リストのパスワードを更新 ----------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		char *Pass : パスワード
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int SetHostPassword(int Num, char *Pass)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		strcpy(Pos->Set.PassWord, Pass);
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 指定の設定名を持つ設定の番号を返す ------------------------------------
*
*	Parameter
*		char *Name : 設定名
*
*	Return Value
*		int 設定番号 (0～)
*			-1=見つからない
*----------------------------------------------------------------------------*/

int SearchHostName(char *Name)
{
	int Ret;
	int i;
	HOSTLISTDATA *Pos;

	Ret = -1;
	Pos = HostListTop;
	for(i = 0; i < Hosts; i++)
	{
		if(strcmp(Name, Pos->Set.HostName) == 0)
		{
			Ret = i;
			break;
		}
		Pos = GetNextNode(Pos);
	}
	return(Ret);
}


/*----- 設定値リストのソート方法を更新 ----------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*		int LFSort : ローカルのファイルのソート方法
*		int LDSort : ローカルのフォルダのソート方法
*		int RFSort : リモートのファイルのソート方法
*		int RDSort : リモートのフォルダのソート方法
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int SetHostSort(int Num, int LFSort, int LDSort, int RFSort, int RDSort)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		Pos->Set.Sort = LFSort * 0x1000000 | LDSort * 0x10000 | RFSort * 0x100 | RDSort;
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 登録されているソート方法を分解する ------------------------------------
*
*	Parameter
*		ulong Sort : ソート方法 
*		int *LFSort : ローカルのファイルのソート方法を格納するワーク
*		int *LDSort : ローカルのフォルダのソート方法を格納するワーク
*		int *RFSort : リモートのファイルのソート方法を格納するワーク
*		int *RDSort : リモートのフォルダのソート方法を格納するワーク
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DecomposeSortType(ulong Sort, int *LFSort, int *LDSort, int *RFSort, int *RDSort)
{
	*LFSort = (int)((Sort / 0x1000000) & 0xFF);
	*LDSort = (int)((Sort / 0x10000) & 0xFF);
	*RFSort = (int)((Sort / 0x100) & 0xFF);
	*RDSort = (int)(Sort & 0xFF);
	return;
}


/*----- 現在接続中の設定番号を返す --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 設定番号
*----------------------------------------------------------------------------*/

int AskCurrentHost(void)
{
	return(ConnectingHost);
}


/*----- 現在接続中の設定番号をセットする --------------------------------------
*
*	Parameter
*		int Num : 設定番号
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetCurrentHost(int Num)
{
	ConnectingHost = Num;
	return;
}


/*----- デフォルト設定値を取り出す --------------------------------------------
*
*	Parameter
*		HOSTDATA *Set : 設定値をコピーするワーク
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void CopyDefaultHost(HOSTDATA *Set)
{
	// ホスト共通設定機能
//	Set->Level = 0;
//	strcpy(Set->HostName, "");
//	strcpy(Set->HostAdrs, "");
//	strcpy(Set->UserName, "");
//	strcpy(Set->PassWord, "");
//	strcpy(Set->Account, "");
//	strcpy(Set->LocalInitDir, DefaultLocalPath);
//	strcpy(Set->RemoteInitDir, "");
//	memcpy(Set->BookMark, "\0\0", 2);
//	strcpy(Set->ChmodCmd, CHMOD_CMD_NOR);
//	strcpy(Set->LsName, LS_FNAME);
//	strcpy(Set->InitCmd, "");
//	Set->Port = PORT_NOR;
//	Set->Anonymous = NO;
//	Set->KanjiCode = KANJI_NOCNV;
//	Set->KanaCnv = YES;
//	Set->NameKanjiCode = KANJI_NOCNV;
//	Set->NameKanaCnv = NO;
//	Set->Pasv = YES;
//	Set->FireWall = NO;
//	Set->ListCmdOnly = YES;
//	Set->UseNLST_R = YES;
//	Set->LastDir = NO;
//	Set->TimeZone = 9;				/* GMT+9 (JST) */
//	Set->HostType = HTYPE_AUTO;
//	Set->SyncMove = NO;
//	Set->NoFullPath = NO;
//	Set->Sort = SORT_NOTSAVED;
//	Set->Security = SECURITY_AUTO;
//	Set->Dialup = NO;
//	Set->DialupAlways = NO;
//	Set->DialupNotify = YES;
//	strcpy(Set->DialEntry, "");
	memcpy(Set, &DefaultHost, sizeof(HOSTDATA));
	return;
}


// ホスト共通設定機能
void ResetDefaultHost(void)
{
	CopyDefaultDefaultHost(&DefaultHost);
	return;
}

void SetDefaultHost(HOSTDATA *Set)
{
	memcpy(&DefaultHost, Set, sizeof(HOSTDATA));
	return;
}

void CopyDefaultDefaultHost(HOSTDATA *Set)
{
	// 国際化対応
	TIME_ZONE_INFORMATION tzi;
	Set->Level = 0;
	strcpy(Set->HostName, "");
	strcpy(Set->HostAdrs, "");
	strcpy(Set->UserName, "");
	strcpy(Set->PassWord, "");
	strcpy(Set->Account, "");
	strcpy(Set->LocalInitDir, DefaultLocalPath);
	strcpy(Set->RemoteInitDir, "");
	memcpy(Set->BookMark, "\0\0", 2);
	strcpy(Set->ChmodCmd, CHMOD_CMD_NOR);
	strcpy(Set->LsName, LS_FNAME);
	strcpy(Set->InitCmd, "");
	Set->Port = PORT_NOR;
	Set->Anonymous = NO;
	Set->KanjiCode = KANJI_NOCNV;
	Set->KanaCnv = YES;
	Set->NameKanjiCode = KANJI_NOCNV;
	// UTF-8対応
	Set->CurNameKanjiCode = KANJI_NOCNV;
	Set->NameKanaCnv = NO;
	Set->Pasv = YES;
	Set->FireWall = NO;
	Set->ListCmdOnly = YES;
	Set->UseNLST_R = YES;
	Set->LastDir = NO;
	// 国際化対応
//	Set->TimeZone = 9;				/* GMT+9 (JST) */
	GetTimeZoneInformation(&tzi);
	Set->TimeZone = (int)(tzi.Bias / -60);
	Set->HostType = HTYPE_AUTO;
	Set->SyncMove = NO;
	Set->NoFullPath = NO;
	Set->Sort = SORT_NOTSAVED;
	Set->Security = SECURITY_AUTO;
	Set->Dialup = NO;
	Set->DialupAlways = NO;
	Set->DialupNotify = YES;
	strcpy(Set->DialEntry, "");
	// 暗号化通信対応
	Set->CryptMode = CRYPT_NONE;
	Set->UseNoEncryption = YES;
	Set->UseFTPES = YES;
	Set->UseFTPIS = YES;
	Set->UseSFTP = YES;
	strcpy(Set->PrivateKey, "");
	Set->NoWeakEncryption = YES;
	// 同時接続対応
	Set->MaxThreadCount = 1;
	Set->ReuseCmdSkt = YES;
	Set->NoDisplayUI = NO;
	// MLSD対応
	Set->Feature = 0;
	Set->UseMLSD = YES;
	// IPv6対応
	Set->NetType = NTYPE_AUTO;
	Set->CurNetType = NTYPE_AUTO;
	// 自動切断対策
	Set->NoopInterval = 60;
	// 再転送対応
	Set->TransferErrorMode = EXIST_OVW;
	Set->TransferErrorNotify = YES;
	// セッションあたりの転送量制限対策
	Set->TransferErrorReconnect = YES;
	return;
}

/*----- 設定名一覧をウィンドウに送る ------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		int Cur : 
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void SendAllHostNames(HWND hWnd, int Cur)
{
	int i;
	HOSTLISTDATA *Pos;
	TV_INSERTSTRUCT tvIns;
	HTREEITEM hItem;
	HTREEITEM hItemCur;
	HTREEITEM *Level;
	int CurLevel;

	hItemCur = NULL;

	/* ちらつくので再描画禁止 */
	SendMessage(hWnd, WM_SETREDRAW, (WPARAM)FALSE, 0);

	SendMessage(hWnd, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);		/* 全てを削除 */

	if((Level = malloc(sizeof(HTREEITEM*) * Hosts + 1)) != NULL)
	{
		Pos = HostListTop;
		for(i = 0; i < Hosts; i++)
		{
			if(Pos->Set.Level & SET_LEVEL_GROUP)
			{
				tvIns.item.iImage = 0;
				tvIns.item.iSelectedImage = 0;
			}
			else
			{
				tvIns.item.iImage = 2;
				tvIns.item.iSelectedImage = 2;
			}

			CurLevel = GetNodeLevel(i);
			if(CurLevel == 0)
				tvIns.hParent = TVI_ROOT;
			else
				tvIns.hParent = Level[CurLevel - 1];

			tvIns.hInsertAfter = TVI_LAST;
			tvIns.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	//		tvIns.item.hItem = 0;
	//		tvIns.item.state = 0;
	//		tvIns.item.stateMask = 0;
			tvIns.item.pszText = Pos->Set.HostName;
			tvIns.item.cchTextMax = 0;
			tvIns.item.cChildren = 1;
			tvIns.item.lParam = i;
			hItem = (HTREEITEM)SendMessage(hWnd, TVM_INSERTITEM, 0, (LPARAM)&tvIns);

			if(Pos->Set.Level & SET_LEVEL_GROUP)
				Level[CurLevel] = hItem;

//			DoPrintf("%d = %x", i, hItem);
			if(i == Cur)
			{
				hItemCur = hItem;
			}
			Pos = GetNextNode(Pos);
		}
		free(Level);
	}

	/* 再描画 */
	SendMessage(hWnd, WM_SETREDRAW, (WPARAM)TRUE, 0);

	if(hItemCur != NULL)
	{
		SendMessage(hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItemCur);
//		SendMessage(hWnd, TVM_ENSUREVISIBLE, 0, (LPARAM)hItemCur);
	}
	UpdateWindow(hWnd);

	return;
}


/*----- 設定値がグループかどうかを返す ----------------------------------------
*
*	Parameter
*		int Num : 設定値号番号
*
*	Return Value
*		int グループかどうか
*			YES/NO/-1=設定値がない
*----------------------------------------------------------------------------*/

static int IsNodeGroup(int Num)
{
	int Ret;
	HOSTLISTDATA *Pos;

	Ret = -1;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		Ret = (Pos->Set.Level & SET_LEVEL_GROUP) ? YES : NO;
	}
	return(Ret);
}


/*----- WS_FTP.INIからのインポート --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ImportFromWSFTP(void)
{
	FILE *Strm;
	char Buf[FMAX_PATH+1];
	HOSTDATA Host;
	int InHost;

	strcpy(Buf, "WS_FTP.INI");
	if(SelectFile(GetMainHwnd(), Buf, MSGJPN126, MSGJPN276, NULL, OFN_FILEMUSTEXIST, 0) == TRUE)
	{
		if((Strm = fopen(Buf, "rt")) != NULL)
		{
			InHost = NO;
			while(fgets(Buf, FMAX_PATH, Strm) != NULL)
			{
				if(Buf[0] == '[')
				{
					if(InHost == YES)
					{
						AddHostToList(&Host, -1, 0);
						InHost = NO;
					}
					if(_stricmp(Buf, "[_config_]\n") != 0)
					{
						CopyDefaultHost(&Host);

						*(Buf + strlen(Buf) - 2) = NUL;
						memset(Host.HostName, NUL, HOST_NAME_LEN+1);
						strncpy(Host.HostName, Buf+1, HOST_NAME_LEN);
						InHost = YES;
					}
				}
				else if(InHost == YES)
				{
					FormatIniString(Buf);

					if(_strnicmp(Buf, "HOST=", 5) == 0)
					{
						memset(Host.HostAdrs, NUL, HOST_ADRS_LEN+1);
						strncpy(Host.HostAdrs, Buf+5, HOST_ADRS_LEN);
					}
					else if(_strnicmp(Buf, "UID=", 4) == 0)
					{
						memset(Host.UserName, NUL, USER_NAME_LEN+1);
						strncpy(Host.UserName, Buf+4, USER_NAME_LEN);
						if(strcmp(Host.UserName, "anonymous") == 0)
							strcpy(Host.PassWord, UserMailAdrs);
					}
					else if(_strnicmp(Buf, "LOCDIR=", 7) == 0)
					{
						memset(Host.LocalInitDir, NUL, INIT_DIR_LEN+1);
						strncpy(Host.LocalInitDir, Buf+7, INIT_DIR_LEN);
					}
					else if(_strnicmp(Buf, "DIR=", 4) == 0)
					{
						memset(Host.RemoteInitDir, NUL, INIT_DIR_LEN+1);
						strncpy(Host.RemoteInitDir, Buf+4, INIT_DIR_LEN);
					}
					else if(_strnicmp(Buf, "PASVMODE=", 9) == 0)
						Host.Pasv = (atoi(Buf+9) == 0) ? 0 : 1;
					else if(_strnicmp(Buf, "FIREWALL=", 9) == 0)
						Host.FireWall = (atoi(Buf+9) == 0) ? 0 : 1;
				}
			}

			if(InHost == YES)
				AddHostToList(&Host, -1, 0);
			fclose(Strm);
		}
	}
	return;
}



/*----- ホスト設定のプロパティシート ------------------------------------------
*
*	Parameter
*		HWND hDlg : 親ウインドウのハンドル
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static int DispHostSetDlg(HWND hDlg)
{
	// SFTP、FTPES、FTPIS対応
	// 同時接続対応
//	PROPSHEETPAGE psp[5];
	PROPSHEETPAGE psp[7];
	PROPSHEETHEADER psh;

	// 変数が未初期化のバグ修正
	memset(&psp, 0, sizeof(psp));
	memset(&psh, 0, sizeof(psh));

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[0].hInstance = GetFtpInst();
	psp[0].pszTemplate = MAKEINTRESOURCE(hset_main_dlg);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = MainSettingProc;
	psp[0].pszTitle = MSGJPN127;
	psp[0].lParam = 0;
	psp[0].pfnCallback = NULL;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[1].hInstance = GetFtpInst();
	psp[1].pszTemplate = MAKEINTRESOURCE(hset_adv_dlg);
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = AdvSettingProc;
	psp[1].pszTitle = MSGJPN128;
	psp[1].lParam = 0;
	psp[1].pfnCallback = NULL;

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[2].hInstance = GetFtpInst();
	psp[2].pszTemplate = MAKEINTRESOURCE(hset_code_dlg);
	psp[2].pszIcon = NULL;
	psp[2].pfnDlgProc = CodeSettingProc;
	psp[2].pszTitle = MSGJPN129;
	psp[2].lParam = 0;
	psp[2].pfnCallback = NULL;

	psp[3].dwSize = sizeof(PROPSHEETPAGE);
	psp[3].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[3].hInstance = GetFtpInst();
	psp[3].pszTemplate = MAKEINTRESOURCE(hset_dialup_dlg);
	psp[3].pszIcon = NULL;
	psp[3].pfnDlgProc = DialupSettingProc;
	psp[3].pszTitle = MSGJPN130;
	psp[3].lParam = 0;
	psp[3].pfnCallback = NULL;

	psp[4].dwSize = sizeof(PROPSHEETPAGE);
	psp[4].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[4].hInstance = GetFtpInst();
	psp[4].pszTemplate = MAKEINTRESOURCE(hset_adv2_dlg);
	psp[4].pszIcon = NULL;
	psp[4].pfnDlgProc = Adv2SettingProc;
	psp[4].pszTitle = MSGJPN131;
	psp[4].lParam = 0;
	psp[4].pfnCallback = NULL;

	// SFTP、FTPES、FTPIS対応
	psp[5].dwSize = sizeof(PROPSHEETPAGE);
	psp[5].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[5].hInstance = GetFtpInst();
	psp[5].pszTemplate = MAKEINTRESOURCE(hset_crypt_dlg);
	psp[5].pszIcon = NULL;
	psp[5].pfnDlgProc = CryptSettingProc;
	psp[5].pszTitle = MSGJPN313;
	psp[5].lParam = 0;
	psp[5].pfnCallback = NULL;

	// 同時接続対応
	psp[6].dwSize = sizeof(PROPSHEETPAGE);
	psp[6].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[6].hInstance = GetFtpInst();
	psp[6].pszTemplate = MAKEINTRESOURCE(hset_adv3_dlg);
	psp[6].pszIcon = NULL;
	psp[6].pfnDlgProc = Adv3SettingProc;
	psp[6].pszTitle = MSGJPN320;
	psp[6].lParam = 0;
	psp[6].pfnCallback = NULL;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_HASHELP | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
	psh.hwndParent = hDlg;
	psh.hInstance = GetFtpInst();
	psh.pszIcon = NULL;
	psh.pszCaption = MSGJPN132;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp = (LPCPROPSHEETPAGE)&psp;
	psh.pfnCallback = NULL;

	Apply = NO;
	PropertySheet(&psh);

	return(Apply);
}


/*----- 基本設定ウインドウのコールバック --------------------------------------
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
//static BOOL CALLBACK MainSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK MainSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	// 64ビット対応
//	long wStyle;
	LONG_PTR wStyle;
	char Tmp[FMAX_PATH+1];
	NMHDR *pnmhdr;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			// プロセス保護
			ProtectAllEditControls(hDlg);
			SendDlgItemMessage(hDlg, HSET_HOST, EM_LIMITTEXT, HOST_NAME_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_ADRS, EM_LIMITTEXT, HOST_ADRS_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_LOCAL, EM_LIMITTEXT, INIT_DIR_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_REMOTE, EM_LIMITTEXT, INIT_DIR_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_HOST, WM_SETTEXT, 0, (LPARAM)TmpHost.HostName);
			SendDlgItemMessage(hDlg, HSET_ADRS, WM_SETTEXT, 0, (LPARAM)TmpHost.HostAdrs);
			SendDlgItemMessage(hDlg, HSET_USER, WM_SETTEXT, 0, (LPARAM)TmpHost.UserName);
			SendDlgItemMessage(hDlg, HSET_PASS, WM_SETTEXT, 0, (LPARAM)TmpHost.PassWord);
			SendDlgItemMessage(hDlg, HSET_LOCAL, WM_SETTEXT, 0, (LPARAM)TmpHost.LocalInitDir);
			SendDlgItemMessage(hDlg, HSET_REMOTE, WM_SETTEXT, 0, (LPARAM)TmpHost.RemoteInitDir);
			SendDlgItemMessage(hDlg, HSET_ANONYMOUS, BM_SETCHECK, TmpHost.Anonymous, 0);
			SendDlgItemMessage(hDlg, HSET_LASTDIR, BM_SETCHECK, TmpHost.LastDir, 0);
			if(AskConnecting() == NO)
				EnableWindow(GetDlgItem(hDlg, HSET_REMOTE_CUR), FALSE);
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SendDlgItemMessage(hDlg, HSET_HOST, WM_GETTEXT, HOST_NAME_LEN+1, (LPARAM)TmpHost.HostName);
					SendDlgItemMessage(hDlg, HSET_ADRS, WM_GETTEXT, HOST_ADRS_LEN+1, (LPARAM)TmpHost.HostAdrs);
					RemoveTailingSpaces(TmpHost.HostAdrs);
					SendDlgItemMessage(hDlg, HSET_USER, WM_GETTEXT, USER_NAME_LEN+1, (LPARAM)TmpHost.UserName);
					SendDlgItemMessage(hDlg, HSET_PASS, WM_GETTEXT, PASSWORD_LEN+1, (LPARAM)TmpHost.PassWord);
					SendDlgItemMessage(hDlg, HSET_LOCAL, WM_GETTEXT, INIT_DIR_LEN+1, (LPARAM)TmpHost.LocalInitDir);
					SendDlgItemMessage(hDlg, HSET_REMOTE, WM_GETTEXT, INIT_DIR_LEN+1, (LPARAM)TmpHost.RemoteInitDir);
					TmpHost.Anonymous = SendDlgItemMessage(hDlg, HSET_ANONYMOUS, BM_GETCHECK, 0, 0);
					TmpHost.LastDir = SendDlgItemMessage(hDlg, HSET_LASTDIR, BM_GETCHECK, 0, 0);
					if((strlen(TmpHost.HostName) == 0) && (strlen(TmpHost.HostAdrs) > 0))
					{
						memset(TmpHost.HostName, NUL, HOST_NAME_LEN+1);
						strncpy(TmpHost.HostName, TmpHost.HostAdrs, HOST_NAME_LEN);
					}
					else if((strlen(TmpHost.HostName) > 0) && (strlen(TmpHost.HostAdrs) == 0))
					{
						memset(TmpHost.HostAdrs, NUL, HOST_ADRS_LEN+1);
						strncpy(TmpHost.HostAdrs, TmpHost.HostName, HOST_ADRS_LEN);
					}
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000028);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case HSET_LOCAL_BR :
					if(SelectDir(hDlg, TmpHost.LocalInitDir, INIT_DIR_LEN) == TRUE)
						SendDlgItemMessage(hDlg, HSET_LOCAL, WM_SETTEXT, 0, (LPARAM)TmpHost.LocalInitDir);
					break;

				case HSET_REMOTE_CUR :
						AskRemoteCurDir(Tmp, FMAX_PATH);
						SendDlgItemMessage(hDlg, HSET_REMOTE, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;

				case HSET_ANONYMOUS :
					if(SendDlgItemMessage(hDlg, HSET_ANONYMOUS, BM_GETCHECK, 0, 0) == 1)
					{
						SendDlgItemMessage(hDlg, HSET_USER, WM_SETTEXT, 0, (LPARAM)"anonymous");
						// 64ビット対応
//						wStyle = GetWindowLong(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
						wStyle = GetWindowLongPtr(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
						wStyle &= ~ES_PASSWORD;
						// 64ビット対応
//						SetWindowLong(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle);
						SetWindowLongPtr(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle);
						SendDlgItemMessage(hDlg, HSET_PASS, WM_SETTEXT, 0, (LPARAM)UserMailAdrs);
					}
					else
					{
						SendDlgItemMessage(hDlg, HSET_USER, WM_SETTEXT, 0, (LPARAM)"");
						// 64ビット対応
//						wStyle = GetWindowLong(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
						wStyle = GetWindowLongPtr(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
						wStyle |= ES_PASSWORD;
						// 64ビット対応
//						SetWindowLong(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle);
						SetWindowLongPtr(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle);
						SendDlgItemMessage(hDlg, HSET_PASS, WM_SETTEXT, 0, (LPARAM)"");
					}
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


/*----- 拡張設定ウインドウのコールバック --------------------------------------
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
//static BOOL CALLBACK AdvSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK AdvSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	char Tmp[20];
	int i;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, HSET_PORT, EM_LIMITTEXT, 5, 0);
			sprintf(Tmp, "%d", TmpHost.Port);
			SendDlgItemMessage(hDlg, HSET_PORT, WM_SETTEXT, 0, (LPARAM)Tmp);
			SendDlgItemMessage(hDlg, HSET_ACCOUNT, EM_LIMITTEXT, ACCOUNT_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_ACCOUNT, WM_SETTEXT, 0, (LPARAM)TmpHost.Account);
			SendDlgItemMessage(hDlg, HSET_PASV, BM_SETCHECK, TmpHost.Pasv, 0);
			SendDlgItemMessage(hDlg, HSET_FIREWALL, BM_SETCHECK, TmpHost.FireWall, 0);
			SendDlgItemMessage(hDlg, HSET_SYNCMOVE, BM_SETCHECK, TmpHost.SyncMove, 0);
			for(i = -12; i <= 12; i++)
			{
				if(i == 0)
					sprintf(Tmp, "GMT");
				else if(i == 9)
					sprintf(Tmp, MSGJPN133, i);
				else
					sprintf(Tmp, "GMT%+02d:00", i);
				SendDlgItemMessage(hDlg, HSET_TIMEZONE, CB_ADDSTRING, 0, (LPARAM)Tmp);
			}
			SendDlgItemMessage(hDlg, HSET_TIMEZONE, CB_SETCURSEL, TmpHost.TimeZone+12, 0);

			SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN134);
			SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN135);
			SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN136);
			SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN137);
			SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN138);
			SendDlgItemMessage(hDlg, HSET_SECURITY, CB_SETCURSEL, TmpHost.Security, 0);
			SendDlgItemMessage(hDlg, HSET_INITCMD, EM_LIMITTEXT, INITCMD_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_INITCMD, WM_SETTEXT, 0, (LPARAM)TmpHost.InitCmd);
			// IPv6対応
			SendDlgItemMessage(hDlg, HSET_NETTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN332);
			SendDlgItemMessage(hDlg, HSET_NETTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN333);
			SendDlgItemMessage(hDlg, HSET_NETTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN334);
			SendDlgItemMessage(hDlg, HSET_NETTYPE, CB_SETCURSEL, TmpHost.NetType, 0);
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					TmpHost.Pasv = SendDlgItemMessage(hDlg, HSET_PASV, BM_GETCHECK, 0, 0);
					TmpHost.FireWall = SendDlgItemMessage(hDlg, HSET_FIREWALL, BM_GETCHECK, 0, 0);
					TmpHost.SyncMove = SendDlgItemMessage(hDlg, HSET_SYNCMOVE, BM_GETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, HSET_PORT, WM_GETTEXT, 5+1, (LPARAM)Tmp);
					TmpHost.Port = atoi(Tmp);
					SendDlgItemMessage(hDlg, HSET_ACCOUNT, WM_GETTEXT, ACCOUNT_LEN+1, (LPARAM)TmpHost.Account);
					TmpHost.TimeZone = SendDlgItemMessage(hDlg, HSET_TIMEZONE, CB_GETCURSEL, 0, 0) - 12;
					TmpHost.Security = SendDlgItemMessage(hDlg, HSET_SECURITY, CB_GETCURSEL, 0, 0);
					SendDlgItemMessage(hDlg, HSET_INITCMD, WM_GETTEXT, INITCMD_LEN+1, (LPARAM)TmpHost.InitCmd);
					// IPv6対応
					TmpHost.NetType = SendDlgItemMessage(hDlg, HSET_NETTYPE, CB_GETCURSEL, 0, 0);
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000029);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case HSET_PORT_NOR :
					sprintf(Tmp, "%d", PORT_NOR);
					SendDlgItemMessage(hDlg, HSET_PORT, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;
			}
			return(TRUE);
	}
	return(FALSE);
}


/*----- 文字コード設定ウインドウのコールバック --------------------------------
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
//static BOOL CALLBACK CodeSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK CodeSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;

	// UTF-8対応
	static const RADIOBUTTON KanjiButton[] = {
		{ HSET_NO_CNV, KANJI_NOCNV },
		{ HSET_SJIS_CNV, KANJI_SJIS },
		{ HSET_JIS_CNV, KANJI_JIS },
		{ HSET_EUC_CNV, KANJI_EUC },
		{ HSET_UTF8N_CNV, KANJI_UTF8N },
		{ HSET_UTF8BOM_CNV, KANJI_UTF8BOM }
	};
	#define KANJIBUTTONS	(sizeof(KanjiButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON NameKanjiButton[] = {
		{ HSET_FN_AUTO_CNV, KANJI_AUTO },
		{ HSET_FN_SJIS_CNV, KANJI_SJIS },
		{ HSET_FN_JIS_CNV, KANJI_JIS },
		{ HSET_FN_EUC_CNV, KANJI_EUC },
		{ HSET_FN_SMH_CNV, KANJI_SMB_HEX },
		{ HSET_FN_SMC_CNV, KANJI_SMB_CAP },
		// UTF-8 HFS+対応
//		{ HSET_FN_UTF8N_CNV, KANJI_UTF8N }		// UTF-8対応
		{ HSET_FN_UTF8N_CNV, KANJI_UTF8N },		// UTF-8対応
		{ HSET_FN_UTF8HFSX_CNV, KANJI_UTF8HFSX }
	};
	#define NAMEKANJIBUTTONS	(sizeof(NameKanjiButton)/sizeof(RADIOBUTTON))

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SetRadioButtonByValue(hDlg, TmpHost.KanjiCode, KanjiButton, KANJIBUTTONS);
			SendDlgItemMessage(hDlg, HSET_HANCNV, BM_SETCHECK, TmpHost.KanaCnv, 0);
			SetRadioButtonByValue(hDlg, TmpHost.NameKanjiCode, NameKanjiButton, NAMEKANJIBUTTONS);
			// UTF-8 HFS+対応
			if(IsUnicodeNormalizationDllLoaded() == NO)
				EnableWindow(GetDlgItem(hDlg, HSET_FN_UTF8HFSX_CNV), FALSE);
			SendDlgItemMessage(hDlg, HSET_FN_HANCNV, BM_SETCHECK, TmpHost.NameKanaCnv, 0);
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					TmpHost.KanjiCode = AskRadioButtonValue(hDlg, KanjiButton, KANJIBUTTONS);
					TmpHost.KanaCnv = SendDlgItemMessage(hDlg, HSET_HANCNV, BM_GETCHECK, 0, 0);
					TmpHost.NameKanjiCode = AskRadioButtonValue(hDlg, NameKanjiButton, NAMEKANJIBUTTONS);
					TmpHost.NameKanaCnv = SendDlgItemMessage(hDlg, HSET_FN_HANCNV, BM_GETCHECK, 0, 0);
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000030);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case HSET_SJIS_CNV :
				case HSET_JIS_CNV :
				case HSET_EUC_CNV :
					EnableWindow(GetDlgItem(hDlg, HSET_HANCNV), TRUE);
					break;

				// UTF-8対応
				case HSET_NO_CNV :
				case HSET_UTF8N_CNV :
				case HSET_UTF8BOM_CNV :
					EnableWindow(GetDlgItem(hDlg, HSET_HANCNV), FALSE);
					break;

				case HSET_FN_JIS_CNV :
				case HSET_FN_EUC_CNV :
					EnableWindow(GetDlgItem(hDlg, HSET_FN_HANCNV), TRUE);
					break;

				case HSET_FN_AUTO_CNV :
				case HSET_FN_SJIS_CNV :
				case HSET_FN_SMH_CNV :
				case HSET_FN_SMC_CNV :
				case HSET_FN_UTF8N_CNV :	// UTF-8対応
				// UTF-8 HFS+対応
				case HSET_FN_UTF8HFSX_CNV :
					EnableWindow(GetDlgItem(hDlg, HSET_FN_HANCNV), FALSE);
					break;
			}
			return(TRUE);
	}
	return(FALSE);
}


/*----- ダイアルアップ設定ウインドウのコールバック ----------------------------
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
//static BOOL CALLBACK DialupSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK DialupSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, HSET_DIALUP, BM_SETCHECK, TmpHost.Dialup, 0);
			SendDlgItemMessage(hDlg, HSET_DIALUSETHIS, BM_SETCHECK, TmpHost.DialupAlways, 0);
			SendDlgItemMessage(hDlg, HSET_DIALNOTIFY, BM_SETCHECK, TmpHost.DialupNotify, 0);
			if(AskRasUsable() == NO)
				EnableWindow(GetDlgItem(hDlg, HSET_DIALUP), FALSE);
			if((TmpHost.DialupAlways == NO) || (AskRasUsable() == NO))
				EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
			if((TmpHost.Dialup == NO) || (AskRasUsable() == NO))
			{
				EnableWindow(GetDlgItem(hDlg, HSET_DIALENTRY), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_DIALUSETHIS), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
			}
			SetRasEntryToComboBox(hDlg, HSET_DIALENTRY, TmpHost.DialEntry);
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					TmpHost.Dialup = SendDlgItemMessage(hDlg, HSET_DIALUP, BM_GETCHECK, 0, 0);
					TmpHost.DialupAlways = SendDlgItemMessage(hDlg, HSET_DIALUSETHIS, BM_GETCHECK, 0, 0);
					TmpHost.DialupNotify = SendDlgItemMessage(hDlg, HSET_DIALNOTIFY, BM_GETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, HSET_DIALENTRY, WM_GETTEXT, RAS_NAME_LEN+1, (LPARAM)TmpHost.DialEntry);
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000031);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case HSET_DIALUP :
					if(SendDlgItemMessage(hDlg, HSET_DIALUP, BM_GETCHECK, 0, 0) == 0)
					{
						EnableWindow(GetDlgItem(hDlg, HSET_DIALENTRY), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_DIALUSETHIS), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
						break;
					}
					else
					{
						EnableWindow(GetDlgItem(hDlg, HSET_DIALENTRY), TRUE);
						EnableWindow(GetDlgItem(hDlg, HSET_DIALUSETHIS), TRUE);
					}
					/* ここにbreakはない */

				case HSET_DIALUSETHIS :
					if(SendDlgItemMessage(hDlg, HSET_DIALUSETHIS, BM_GETCHECK, 0, 0) == 0)
						EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
					else
						EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), TRUE);
					break;
			}
			return(TRUE);
	}
	return(FALSE);
}


/*----- 高度設定ウインドウのコールバック --------------------------------------
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
//static BOOL CALLBACK Adv2SettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK Adv2SettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	int Num;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, EM_LIMITTEXT, CHMOD_CMD_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, WM_SETTEXT, 0, (LPARAM)TmpHost.ChmodCmd);
			SendDlgItemMessage(hDlg, HSET_LS_FNAME, EM_LIMITTEXT, NLST_NAME_LEN, 0);
			SendDlgItemMessage(hDlg, HSET_LS_FNAME, WM_SETTEXT, 0, (LPARAM)TmpHost.LsName);
			SendDlgItemMessage(hDlg, HSET_LISTCMD, BM_SETCHECK, TmpHost.ListCmdOnly, 0);
			if(TmpHost.ListCmdOnly == YES)
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
			// MLSD対応
			else
				EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), FALSE);
			SendDlgItemMessage(hDlg, HSET_MLSDCMD, BM_SETCHECK, TmpHost.UseMLSD, 0);
			SendDlgItemMessage(hDlg, HSET_NLST_R, BM_SETCHECK, TmpHost.UseNLST_R, 0);
			SendDlgItemMessage(hDlg, HSET_FULLPATH, BM_SETCHECK, TmpHost.NoFullPath, 0);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN139);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN140);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN141);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN142);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN143);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN144);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN289);
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN295);
#if defined(HAVE_TANDEM)
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN2000);
#endif
			SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_SETCURSEL, TmpHost.HostType, 0);
#if defined(HAVE_TANDEM)
			if(TmpHost.HostType == 2 || TmpHost.HostType == HTYPE_TANDEM)  /* VAX or Tandem */
#else
			if(TmpHost.HostType == 2)
#endif
			{
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), FALSE);
			}
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, WM_GETTEXT, CHMOD_CMD_LEN+1, (LPARAM)TmpHost.ChmodCmd);
					SendDlgItemMessage(hDlg, HSET_LS_FNAME, WM_GETTEXT, NLST_NAME_LEN+1, (LPARAM)TmpHost.LsName);
					TmpHost.ListCmdOnly = SendDlgItemMessage(hDlg, HSET_LISTCMD, BM_GETCHECK, 0, 0);
					// MLSD対応
					TmpHost.UseMLSD = SendDlgItemMessage(hDlg, HSET_MLSDCMD, BM_GETCHECK, 0, 0);
					TmpHost.UseNLST_R = SendDlgItemMessage(hDlg, HSET_NLST_R, BM_GETCHECK, 0, 0);
					TmpHost.NoFullPath = SendDlgItemMessage(hDlg, HSET_FULLPATH, BM_GETCHECK, 0, 0);
					TmpHost.HostType = SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_GETCURSEL, 0, 0);
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000032);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case HSET_CHMOD_NOR :
					SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, WM_SETTEXT, 0, (LPARAM)CHMOD_CMD_NOR);
					break;

				case HSET_LS_FNAME_NOR :
					SendDlgItemMessage(hDlg, HSET_LS_FNAME, WM_SETTEXT, 0, (LPARAM)LS_FNAME);
					break;

				case HSET_LISTCMD :
					if(SendDlgItemMessage(hDlg, HSET_LISTCMD, BM_GETCHECK, 0, 0) == 0)
						// MLSD対応
//						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
					{
						EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
					}
					else
						// MLSD対応
//						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
					{
						EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), TRUE);
						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
					}
					break;

				case HSET_HOSTTYPE :
					Num = SendDlgItemMessage(hDlg, HSET_HOSTTYPE, CB_GETCURSEL, 0, 0);
					if(Num == 2)
					{
						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), FALSE);
					}
#if defined(HAVE_TANDEM)
					else if(Num == HTYPE_TANDEM) /* Tandem */
					{
						/* Tandem を選択すると自動的に LIST にチェックをいれる */
						SendDlgItemMessage(hDlg, HSET_LISTCMD, BM_SETCHECK, 1, 0);
						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), FALSE);
						EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), FALSE);
					}
					else
					{
						if(SendDlgItemMessage(hDlg, HSET_LISTCMD, BM_GETCHECK, 0, 0) == 0) {
							EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
							EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), TRUE);
						} else {
							EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
							EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), TRUE);
						}
						EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), TRUE);
					}
#else
					else
					{
						EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
						EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), TRUE);
						EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), TRUE);
					}
#endif
					break;
			}
			return(TRUE);
	}
	return(FALSE);
}


// 暗号化通信対応
static INT_PTR CALLBACK CryptSettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
//	int Num;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, HSET_NO_ENCRYPTION, BM_SETCHECK, TmpHost.UseNoEncryption, 0);
			if(IsOpenSSLLoaded())
			{
				SendDlgItemMessage(hDlg, HSET_FTPES, BM_SETCHECK, TmpHost.UseFTPES, 0);
				SendDlgItemMessage(hDlg, HSET_FTPIS, BM_SETCHECK, TmpHost.UseFTPIS, 0);
			}
			else
			{
				SendDlgItemMessage(hDlg, HSET_FTPES, BM_SETCHECK, BST_UNCHECKED, 0);
				EnableWindow(GetDlgItem(hDlg, HSET_FTPES), FALSE);
				SendDlgItemMessage(hDlg, HSET_FTPIS, BM_SETCHECK, BST_UNCHECKED, 0);
				EnableWindow(GetDlgItem(hDlg, HSET_FTPIS), FALSE);
			}
			SendDlgItemMessage(hDlg, HSET_SFTP, BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(hDlg, HSET_SFTP), FALSE);
			EnableWindow(GetDlgItem(hDlg, PKEY_FILE_BR), FALSE);
			EnableWindow(GetDlgItem(hDlg, HSET_PRIVATE_KEY), FALSE);
			SendDlgItemMessage(hDlg, HSET_NO_WEAK, BM_SETCHECK, TmpHost.NoWeakEncryption, 0);
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					TmpHost.UseNoEncryption = SendDlgItemMessage(hDlg, HSET_NO_ENCRYPTION, BM_GETCHECK, 0, 0);
					if(IsOpenSSLLoaded())
					{
						TmpHost.UseFTPES = SendDlgItemMessage(hDlg, HSET_FTPES, BM_GETCHECK, 0, 0);
						TmpHost.UseFTPIS = SendDlgItemMessage(hDlg, HSET_FTPIS, BM_GETCHECK, 0, 0);
					}
					TmpHost.NoWeakEncryption = SendDlgItemMessage(hDlg, HSET_NO_WEAK, BM_GETCHECK, 0, 0);
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000065);
					break;
			}
			break;
	}
	return(FALSE);
}

// 同時接続対応
static INT_PTR CALLBACK Adv3SettingProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
//	int Num;

	// UTF-8対応
	static const RADIOBUTTON KanjiButton[] = {
		{ HSET_NO_CNV, KANJI_NOCNV },
		{ HSET_SJIS_CNV, KANJI_SJIS },
		{ HSET_JIS_CNV, KANJI_JIS },
		{ HSET_EUC_CNV, KANJI_EUC },
		{ HSET_UTF8N_CNV, KANJI_UTF8N },
		{ HSET_UTF8BOM_CNV, KANJI_UTF8BOM }
	};
	#define KANJIBUTTONS	(sizeof(KanjiButton)/sizeof(RADIOBUTTON))

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, HSET_THREAD_COUNT, EM_LIMITTEXT, (WPARAM)1, 0);
			SetDecimalText(hDlg, HSET_THREAD_COUNT, TmpHost.MaxThreadCount);
			SendDlgItemMessage(hDlg, HSET_THREAD_COUNT_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(MAX_DATA_CONNECTION, 1));
			SendDlgItemMessage(hDlg, HSET_REUSE_SOCKET, BM_SETCHECK, TmpHost.ReuseCmdSkt, 0);
			SendDlgItemMessage(hDlg, HSET_NOOP_INTERVAL, EM_LIMITTEXT, (WPARAM)3, 0);
			SetDecimalText(hDlg, HSET_NOOP_INTERVAL, TmpHost.NoopInterval);
			SendDlgItemMessage(hDlg, HSET_NOOP_INTERVAL_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(300, 0));
			SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN335);
			SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN336);
			SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN337);
			SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN338);
			if(TmpHost.TransferErrorNotify == YES)
				SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 0, 0);
			else if(TmpHost.TransferErrorMode == EXIST_OVW)
				SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 1, 0);
			else if(TmpHost.TransferErrorMode == EXIST_RESUME)
				SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 2, 0);
			else if(TmpHost.TransferErrorMode == EXIST_IGNORE)
				SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 3, 0);
			else
				SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hDlg, HSET_ERROR_RECONNECT, BM_SETCHECK, TmpHost.TransferErrorReconnect, 0);
			return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					TmpHost.MaxThreadCount = GetDecimalText(hDlg, HSET_THREAD_COUNT);
					CheckRange2(&TmpHost.MaxThreadCount, MAX_DATA_CONNECTION, 1);
					TmpHost.ReuseCmdSkt = SendDlgItemMessage(hDlg, HSET_REUSE_SOCKET, BM_GETCHECK, 0, 0);
					TmpHost.NoopInterval = GetDecimalText(hDlg, HSET_NOOP_INTERVAL);
					CheckRange2(&TmpHost.NoopInterval, 300, 0);
					switch(SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_GETCURSEL, 0, 0))
					{
					case 0:
						TmpHost.TransferErrorMode = EXIST_OVW;
						TmpHost.TransferErrorNotify = YES;
						break;
					case 1:
						TmpHost.TransferErrorMode = EXIST_OVW;
						TmpHost.TransferErrorNotify = NO;
						break;
					case 2:
						TmpHost.TransferErrorMode = EXIST_RESUME;
						TmpHost.TransferErrorNotify = NO;
						break;
					case 3:
						TmpHost.TransferErrorMode = EXIST_IGNORE;
						TmpHost.TransferErrorNotify = NO;
						break;
					}
					TmpHost.TransferErrorReconnect = SendDlgItemMessage(hDlg, HSET_ERROR_RECONNECT, BM_GETCHECK, 0, 0);
					Apply = YES;
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000066);
					break;
			}
			break;
	}
	return(FALSE);
}

// 暗号化通信対応
// ホストの暗号化設定を更新
int SetHostEncryption(int Num, int UseNoEncryption, int UseFTPES, int UseFTPIS, int UseSFTP)
{
	int Sts;
	HOSTLISTDATA *Pos;

	Sts = FFFTP_FAIL;
	if((Num >= 0) && (Num < Hosts))
	{
		Pos = GetNodeByNum(Num);
		Pos->Set.UseNoEncryption = UseNoEncryption;
		Pos->Set.UseFTPES = UseFTPES;
		Pos->Set.UseFTPIS = UseFTPIS;
		Pos->Set.UseSFTP = UseSFTP;
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}

