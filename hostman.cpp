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

#include "common.h"


/*===== プロトタイプ =====*/

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
static bool DispHostSetDlg(HWND hDlg);

/* 設定値 */
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern HFONT ListFont;
extern char DefaultLocalPath[FMAX_PATH+1];
extern int ConnectAndSet;
extern SIZE HostDlgSize;
extern int NoRasControl;

/*===== ローカルなワーク =====*/

static int Hosts = 0;						/* ホスト数 */
static int ConnectingHost;					/* 接続中のホスト */
static int CurrentHost;						/* カーソル位置のホスト */
static HOSTLISTDATA *HostListTop = NULL;	/* ホスト一覧データ */
static HOSTDATA TmpHost;					/* ホスト情報コピー用 */

// ホスト共通設定機能
HOSTDATA DefaultHost;


struct HostList {
	using result_t = int;
	Resizable<Controls<HOST_NEW, HOST_FOLDER, HOST_SET, HOST_COPY, HOST_DEL, HOST_DOWN, HOST_UP, HOST_SET_DEFAULT, IDHELP, HOST_SIZEGRIP>, Controls<IDOK, IDCANCEL, HOST_SIZEGRIP>, Controls<HOST_LIST>> resizable{ HostDlgSize };
	HIMAGELIST hImage = ImageList_LoadImageW(GetFtpInst(), MAKEINTRESOURCEW(hlist_bmp), 16, 8, RGB(255, 0, 0), IMAGE_BITMAP, 0);
	~HostList() {
		ImageList_Destroy(hImage);
	}
	INT_PTR OnInit(HWND hDlg) {
		if (AskConnecting() == YES) {
			/* 接続中は「変更」のみ許可 */
			EnableWindow(GetDlgItem(hDlg, HOST_NEW), FALSE);
			EnableWindow(GetDlgItem(hDlg, HOST_FOLDER), FALSE);
			EnableWindow(GetDlgItem(hDlg, HOST_COPY), FALSE);
			EnableWindow(GetDlgItem(hDlg, HOST_DEL), FALSE);
			EnableWindow(GetDlgItem(hDlg, HOST_DOWN), FALSE);
			EnableWindow(GetDlgItem(hDlg, HOST_UP), FALSE);
		}
		if (ListFont != NULL)
			SendDlgItemMessageW(hDlg, HOST_LIST, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
		SendDlgItemMessageW(hDlg, HOST_LIST, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImage);
		CurrentHost = 0;
		if (ConnectingHost >= 0)
			CurrentHost = ConnectingHost;
		SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
		return TRUE;
	}
	void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case IDOK:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;
				ConnectingHost = CurrentHost;
				EndDialog(hDlg, YES);
				return;
			}
			[[fallthrough]];
		case IDCANCEL:
			EndDialog(hDlg, NO);
			return;
		case HOST_NEW:
			CopyDefaultHost(&TmpHost);
			if (DispHostSetDlg(hDlg)) {
				int Level1 = -1;
				if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
					TVITEMW Item{ TVIF_PARAM, hItem };
					SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
					TmpHost.Level = GetNodeLevel((int)Item.lParam);
					Level1 = (int)Item.lParam + 1;
					CurrentHost = Level1;
				} else {
					TmpHost.Level = 0;
					CurrentHost = Hosts;
				}
				AddHostToList(&TmpHost, Level1, SET_LEVEL_SAME);
				SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
			}
			break;
		case HOST_FOLDER:
			CopyDefaultHost(&TmpHost);
			if (int Level1 = -1; InputDialog(group_dlg, hDlg, NULL, TmpHost.HostName, HOST_NAME_LEN + 1)) {
				if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
					TVITEMW Item{ TVIF_PARAM, hItem };
					SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
					TmpHost.Level = GetNodeLevel((int)Item.lParam) | SET_LEVEL_GROUP;
					Level1 = (int)Item.lParam + 1;
					CurrentHost = Level1;
				} else {
					TmpHost.Level = 0 | SET_LEVEL_GROUP;
					CurrentHost = Hosts;
				}
				AddHostToList(&TmpHost, Level1, SET_LEVEL_SAME);
				SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
			}
			break;
		case HOST_SET:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;
				CopyHostFromList(CurrentHost, &TmpHost);
				int Level1 = IsNodeGroup(CurrentHost);
				if (Level1 == NO && DispHostSetDlg(hDlg) || Level1 == YES && InputDialog(group_dlg, hDlg, NULL, TmpHost.HostName, HOST_NAME_LEN + 1)) {
					UpdateHostToList(CurrentHost, &TmpHost);
					SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
				}
			}
			break;
		case HOST_COPY:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;
				CopyHostFromList(CurrentHost, &TmpHost);
				strcpy(TmpHost.BookMark, "\0");
				CurrentHost++;
				AddHostToList(&TmpHost, CurrentHost, SET_LEVEL_SAME);
				SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
			}
			break;
		case HOST_DEL:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;
				int Level1 = IsNodeGroup(CurrentHost);
				if (Level1 == YES && Dialog(GetFtpInst(), groupdel_dlg, hDlg) || Level1 == NO && Dialog(GetFtpInst(), hostdel_dlg, hDlg)) {
					DelHostFromList(CurrentHost);
					if (CurrentHost >= Hosts)
						CurrentHost = std::max(0, Hosts - 1);
					SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
				}
			}
			break;
		case HOST_UP:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;

				if (CurrentHost > 0) {
					int Level1, Level2;
					auto Data1 = HostListTop;
					for (Level1 = CurrentHost; Level1 > 0; Level1--)
						Data1 = GetNextNode(Data1);
					Level1 = GetNodeLevel(CurrentHost);

					auto Data2 = HostListTop;
					for (Level2 = CurrentHost - 1; Level2 > 0; Level2--)
						Data2 = GetNextNode(Data2);
					Level2 = GetNodeLevel(CurrentHost - 1);

					if (Level1 == Level2 && (Data2->Set.Level & SET_LEVEL_GROUP)) {
						//Data2のchildへ
						if (Data1->Next != NULL)
							Data1->Next->Prev = Data1->Prev;
						if (Data1->Prev != NULL)
							Data1->Prev->Next = Data1->Next;
						if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
							Data1->Parent->Child = Data1->Next;
						if (Data1->Parent == NULL && HostListTop == Data1)
							HostListTop = Data1->Next;

						Data1->Next = Data2->Child;
						Data1->Prev = NULL;
						Data1->Parent = Data2;
						Data2->Child = Data1;
					} else if (Level1 < Level2) {
						//Data1のPrevのChildのNextの末尾へ
						Data2 = Data1->Prev->Child;
						while (Data2->Next != NULL)
							Data2 = Data2->Next;

						if (Data1->Next != NULL)
							Data1->Next->Prev = Data1->Prev;
						if (Data1->Prev != NULL)
							Data1->Prev->Next = Data1->Next;
						if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
							Data1->Parent->Child = Data1->Next;
						if (Data1->Parent == NULL && HostListTop == Data1)
							HostListTop = Data1->Next;

						Data2->Next = Data1;
						Data1->Prev = Data2;
						Data1->Next = NULL;
						Data1->Parent = Data2->Parent;
					} else {
						//Data2のprevへ
						if (Data1->Next != NULL)
							Data1->Next->Prev = Data1->Prev;
						if (Data1->Prev != NULL)
							Data1->Prev->Next = Data1->Next;
						if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
							Data1->Parent->Child = Data1->Next;
						if (Data1->Parent == NULL && HostListTop == Data1)
							HostListTop = Data1->Next;

						if (Data2->Prev != NULL)
							Data2->Prev->Next = Data1;
						Data1->Prev = Data2->Prev;
						Data2->Prev = Data1;
						Data1->Next = Data2;
						Data1->Parent = Data2->Parent;

						if (Data1->Parent != NULL && Data1->Parent->Child == Data2)
							Data1->Parent->Child = Data1;
						if (Data1->Parent == NULL && HostListTop == Data2)
							HostListTop = Data1;
					}

					CurrentHost = GetNodeNumByData(Data1);
					SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
				}
			}
			break;
		case HOST_DOWN:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;

				int Level1, Level2;
				auto Data1 = HostListTop;
				for (Level1 = CurrentHost; Level1 > 0; Level1--)
					Data1 = GetNextNode(Data1);
				Level1 = GetNodeLevel(CurrentHost);

				HOSTLISTDATA* Data2 = NULL;
				Level2 = SET_LEVEL_SAME;
				if (CurrentHost < Hosts - 1) {
					Data2 = HostListTop;
					for (Level2 = CurrentHost + 1; Level2 > 0; Level2--)
						Data2 = GetNextNode(Data2);
					Level2 = GetNodeLevel(CurrentHost + 1);

					if (Level1 < Level2) {
						if (Data1->Next != NULL) {
							//Data2 = Data1のNext
							Data2 = Data1->Next;
							Level2 = GetNodeLevelByData(Data2);
						} else if (Data1->Parent != NULL) {
							Data2 = NULL;
							Level2 = SET_LEVEL_SAME;
						}
					}
				}

				__assume(Data1);
				if (Data2 == NULL && Level1 > 0 || Level1 > Level2) {
					__assume(Data1->Parent);
					//Data1のParentのNextへ
					Data2 = Data1->Parent;

					if (Data1->Next != NULL)
						Data1->Next->Prev = Data1->Prev;
					if (Data1->Prev != NULL)
						Data1->Prev->Next = Data1->Next;
					if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
						Data1->Parent->Child = Data1->Next;
					if (Data1->Parent == NULL && HostListTop == Data1)
						HostListTop = Data1->Next;

					if (Data2->Next != NULL)
						Data2->Next->Prev = Data1;
					Data1->Next = Data2->Next;
					Data2->Next = Data1;
					Data1->Prev = Data2;
					Data1->Parent = Data2->Parent;

					if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
						Data1->Parent->Child = Data2;
					if (Data1->Parent == NULL && HostListTop == Data1)
						HostListTop = Data2;
				} else if (Level1 == Level2) {
					__assume(Data2);
					if (Data2->Set.Level & SET_LEVEL_GROUP) {
						//Data2のChildへ
						if (Data1->Next != NULL)
							Data1->Next->Prev = Data1->Prev;
						if (Data1->Prev != NULL)
							Data1->Prev->Next = Data1->Next;
						if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
							Data1->Parent->Child = Data1->Next;
						if (Data1->Parent == NULL && HostListTop == Data1)
							HostListTop = Data1->Next;

						if (Data2->Child != NULL)
							Data2->Child->Prev = Data1;
						Data1->Next = Data2->Child;
						Data1->Prev = NULL;
						Data1->Parent = Data2;
						Data2->Child = Data1;
					} else {
						//Data2のNextへ
						if (Data1->Next != NULL)
							Data1->Next->Prev = Data1->Prev;
						if (Data1->Prev != NULL)
							Data1->Prev->Next = Data1->Next;
						if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
							Data1->Parent->Child = Data1->Next;
						if (Data1->Parent == NULL && HostListTop == Data1)
							HostListTop = Data1->Next;

						if (Data2->Next != NULL)
							Data2->Next->Prev = Data1;
						Data1->Next = Data2->Next;
						Data2->Next = Data1;
						Data1->Prev = Data2;
						Data1->Parent = Data2->Parent;

						if (Data1->Parent != NULL && Data1->Parent->Child == Data1)
							Data1->Parent->Child = Data2;
						if (Data1->Parent == NULL && HostListTop == Data1)
							HostListTop = Data2;
					}
				}

				CurrentHost = GetNodeNumByData(Data1);
				SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
			}
			break;
		case HOST_SET_DEFAULT:
			CopyDefaultHost(&TmpHost);
			if (DispHostSetDlg(hDlg))
				SetDefaultHost(&TmpHost);
			break;
		case IDHELP:
			ShowHelp(IDH_HELP_TOPIC_0000027);
			break;
		}
		SetFocus(GetDlgItem(hDlg, HOST_LIST));
	}
	INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		if (nmh->idFrom == HOST_LIST)
			switch (nmh->code) {
			case NM_DBLCLK:
				if (IsWindowEnabled(GetDlgItem(hDlg, IDOK)) == TRUE)
					PostMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
				break;
			case TVN_SELCHANGEDW: {
				/* フォルダが選ばれたときは接続、コピーボタンは禁止 */
				TVITEMW Item{ TVIF_PARAM, reinterpret_cast<NMTREEVIEW*>(nmh)->itemNew.hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				if (IsNodeGroup((int)Item.lParam) == YES) {
					EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
					EnableWindow(GetDlgItem(hDlg, HOST_COPY), FALSE);
				} else {
					EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
					if (AskConnecting() == NO)
						EnableWindow(GetDlgItem(hDlg, HOST_COPY), TRUE);
				}
				break;
			}
			}
		return 0;
	}
};

// ホスト一覧
int SelectHost(int Type) {
	auto result = Dialog(GetFtpInst(), ConnectAndSet == YES || Type == DLG_TYPE_SET ? hostlist_dlg : hostconnect_dlg, GetMainHwnd(), HostList{});

	/* ホスト設定を保存 */
	SetNodeLevelAll();
	SaveRegistry();

	return result;
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

		if((New = (HOSTLISTDATA*)malloc(sizeof(HOSTLISTDATA))) != NULL)
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
		// 同時接続対応
		Set->MaxThreadCount = Pos->Set.MaxThreadCount;
		Set->ReuseCmdSkt = Pos->Set.ReuseCmdSkt;
		// MLSD対応
		Set->UseMLSD = Pos->Set.UseMLSD;
		// 自動切断対策
		Set->NoopInterval = Pos->Set.NoopInterval;
		// 再転送対応
		Set->TransferErrorMode = Pos->Set.TransferErrorMode;
		Set->TransferErrorNotify = Pos->Set.TransferErrorNotify;
		// セッションあたりの転送量制限対策
		Set->TransferErrorReconnect = Pos->Set.TransferErrorReconnect;
		// ホスト側の設定ミス対策
		Set->NoPasvAdrs = Pos->Set.NoPasvAdrs;
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
	// 同時接続対応
	Set->MaxThreadCount = 1;
	Set->ReuseCmdSkt = YES;
	Set->NoDisplayUI = NO;
	// MLSD対応
	Set->Feature = 0;
	Set->UseMLSD = YES;
	Set->CurNetType = NTYPE_AUTO;
	// 自動切断対策
	Set->NoopInterval = 60;
	// 再転送対応
	Set->TransferErrorMode = EXIST_OVW;
	Set->TransferErrorNotify = YES;
	// セッションあたりの転送量制限対策
	Set->TransferErrorReconnect = YES;
	// ホスト側の設定ミス対策
	Set->NoPasvAdrs = NO;
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

	if((Level = (HTREEITEM*)malloc(sizeof(HTREEITEM*) * Hosts + 1)) != NULL)
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
	HOSTDATA Host;
	int InHost;

	if (auto const path = SelectFile(true, GetMainHwnd(), IDS_OPEN_WSFTPINI, L"WS_FTP.INI", nullptr, { FileType::Ini, FileType::All }); !std::empty(path))
	{
		if((Strm = _wfopen(path.c_str(), L"rt")) != NULL)
		{
			char Buf[FMAX_PATH + 1];
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


struct General {
	static constexpr WORD dialogId = hset_main_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, HSET_HOST, EM_LIMITTEXT, HOST_NAME_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_ADRS, EM_LIMITTEXT, HOST_ADRS_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_LOCAL, EM_LIMITTEXT, INIT_DIR_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_REMOTE, EM_LIMITTEXT, INIT_DIR_LEN, 0);
		SendDlgItemMessage(hDlg, HSET_HOST, WM_SETTEXT, 0, (LPARAM)TmpHost.HostName);
		SendDlgItemMessage(hDlg, HSET_ADRS, WM_SETTEXT, 0, (LPARAM)TmpHost.HostAdrs);
		SendDlgItemMessage(hDlg, HSET_USER, WM_SETTEXT, 0, (LPARAM)TmpHost.UserName);
		SendDlgItemMessage(hDlg, HSET_PASS, WM_SETTEXT, 0, (LPARAM)TmpHost.PassWord);
		SendDlgItemMessage(hDlg, HSET_LOCAL, WM_SETTEXT, 0, (LPARAM)TmpHost.LocalInitDir);
		SendDlgItemMessage(hDlg, HSET_REMOTE, WM_SETTEXT, 0, (LPARAM)TmpHost.RemoteInitDir);
		SendDlgItemMessageW(hDlg, HSET_ANONYMOUS, BM_SETCHECK, TmpHost.Anonymous, 0);
		SendDlgItemMessageW(hDlg, HSET_LASTDIR, BM_SETCHECK, TmpHost.LastDir, 0);
		if (AskConnecting() == NO)
			EnableWindow(GetDlgItem(hDlg, HSET_REMOTE_CUR), FALSE);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SendDlgItemMessage(hDlg, HSET_HOST, WM_GETTEXT, HOST_NAME_LEN + 1, (LPARAM)TmpHost.HostName);
			SendDlgItemMessage(hDlg, HSET_ADRS, WM_GETTEXT, HOST_ADRS_LEN + 1, (LPARAM)TmpHost.HostAdrs);
			RemoveTailingSpaces(TmpHost.HostAdrs);
			SendDlgItemMessage(hDlg, HSET_USER, WM_GETTEXT, USER_NAME_LEN + 1, (LPARAM)TmpHost.UserName);
			SendDlgItemMessage(hDlg, HSET_PASS, WM_GETTEXT, PASSWORD_LEN + 1, (LPARAM)TmpHost.PassWord);
			SendDlgItemMessage(hDlg, HSET_LOCAL, WM_GETTEXT, INIT_DIR_LEN + 1, (LPARAM)TmpHost.LocalInitDir);
			SendDlgItemMessage(hDlg, HSET_REMOTE, WM_GETTEXT, INIT_DIR_LEN + 1, (LPARAM)TmpHost.RemoteInitDir);
			TmpHost.Anonymous = (int)SendDlgItemMessageW(hDlg, HSET_ANONYMOUS, BM_GETCHECK, 0, 0);
			TmpHost.LastDir = (int)SendDlgItemMessageW(hDlg, HSET_LASTDIR, BM_GETCHECK, 0, 0);
			if ((strlen(TmpHost.HostName) == 0) && (strlen(TmpHost.HostAdrs) > 0)) {
				memset(TmpHost.HostName, NUL, HOST_NAME_LEN + 1);
				strncpy(TmpHost.HostName, TmpHost.HostAdrs, HOST_NAME_LEN);
			} else if ((strlen(TmpHost.HostName) > 0) && (strlen(TmpHost.HostAdrs) == 0)) {
				memset(TmpHost.HostAdrs, NUL, HOST_ADRS_LEN + 1);
				strncpy(TmpHost.HostAdrs, TmpHost.HostName, HOST_ADRS_LEN);
			}
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000028);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case HSET_LOCAL_BR:
			if (SelectDir(hDlg, TmpHost.LocalInitDir, INIT_DIR_LEN) == TRUE)
				SendDlgItemMessage(hDlg, HSET_LOCAL, WM_SETTEXT, 0, (LPARAM)TmpHost.LocalInitDir);
			break;
		case HSET_REMOTE_CUR: {
			char Tmp[FMAX_PATH + 1];
			AskRemoteCurDir(Tmp, FMAX_PATH);
			SendDlgItemMessage(hDlg, HSET_REMOTE, WM_SETTEXT, 0, (LPARAM)Tmp);
			break;
		}
		case HSET_ANONYMOUS:
			if (SendDlgItemMessageW(hDlg, HSET_ANONYMOUS, BM_GETCHECK, 0, 0) == 1) {
				SendDlgItemMessage(hDlg, HSET_USER, WM_SETTEXT, 0, (LPARAM)"anonymous");
				auto wStyle = GetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
				SetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle & ~ES_PASSWORD);
				SendDlgItemMessage(hDlg, HSET_PASS, WM_SETTEXT, 0, (LPARAM)UserMailAdrs);
			} else {
				SendDlgItemMessage(hDlg, HSET_USER, WM_SETTEXT, 0, (LPARAM)"");
				auto wStyle = GetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
				SetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle | ES_PASSWORD);
				SendDlgItemMessage(hDlg, HSET_PASS, WM_SETTEXT, 0, (LPARAM)"");
			}
			break;
		}
	}
};

struct Advanced {
	static constexpr WORD dialogId = hset_adv_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, HSET_PORT, EM_LIMITTEXT, 5, 0);
		char Tmp[20];
		sprintf(Tmp, "%d", TmpHost.Port);
		SendDlgItemMessage(hDlg, HSET_PORT, WM_SETTEXT, 0, (LPARAM)Tmp);
		SendDlgItemMessageW(hDlg, HSET_ACCOUNT, EM_LIMITTEXT, ACCOUNT_LEN, 0);
		SendDlgItemMessage(hDlg, HSET_ACCOUNT, WM_SETTEXT, 0, (LPARAM)TmpHost.Account);
		SendDlgItemMessageW(hDlg, HSET_PASV, BM_SETCHECK, TmpHost.Pasv, 0);
		SendDlgItemMessageW(hDlg, HSET_FIREWALL, BM_SETCHECK, TmpHost.FireWall, 0);
		SendDlgItemMessageW(hDlg, HSET_SYNCMOVE, BM_SETCHECK, TmpHost.SyncMove, 0);
		for (int i = -12; i <= 12; i++) {
			if (i == 0)
				sprintf(Tmp, "GMT");
			else if (i == 9)
				sprintf(Tmp, MSGJPN133, i);
			else
				sprintf(Tmp, "GMT%+02d:00", i);
			SendDlgItemMessage(hDlg, HSET_TIMEZONE, CB_ADDSTRING, 0, (LPARAM)Tmp);
		}
		SendDlgItemMessageW(hDlg, HSET_TIMEZONE, CB_SETCURSEL, (UINT_PTR)TmpHost.TimeZone + 12, 0);

		SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN134);
		SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN135);
		SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN136);
		SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN137);
		SendDlgItemMessage(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN138);
		SendDlgItemMessageW(hDlg, HSET_SECURITY, CB_SETCURSEL, TmpHost.Security, 0);
		SendDlgItemMessageW(hDlg, HSET_INITCMD, EM_LIMITTEXT, INITCMD_LEN, 0);
		SendDlgItemMessage(hDlg, HSET_INITCMD, WM_SETTEXT, 0, (LPARAM)TmpHost.InitCmd);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			TmpHost.Pasv = (int)SendDlgItemMessageW(hDlg, HSET_PASV, BM_GETCHECK, 0, 0);
			TmpHost.FireWall = (int)SendDlgItemMessageW(hDlg, HSET_FIREWALL, BM_GETCHECK, 0, 0);
			TmpHost.SyncMove = (int)SendDlgItemMessageW(hDlg, HSET_SYNCMOVE, BM_GETCHECK, 0, 0);
			char Tmp[20];
			SendDlgItemMessage(hDlg, HSET_PORT, WM_GETTEXT, 5 + 1, (LPARAM)Tmp);
			TmpHost.Port = atoi(Tmp);
			SendDlgItemMessage(hDlg, HSET_ACCOUNT, WM_GETTEXT, ACCOUNT_LEN + 1, (LPARAM)TmpHost.Account);
			TmpHost.TimeZone = (int)SendDlgItemMessageW(hDlg, HSET_TIMEZONE, CB_GETCURSEL, 0, 0) - 12;
			TmpHost.Security = (int)SendDlgItemMessageW(hDlg, HSET_SECURITY, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hDlg, HSET_INITCMD, WM_GETTEXT, INITCMD_LEN + 1, (LPARAM)TmpHost.InitCmd);
			return PSNRET_NOERROR;
		}
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000029);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case HSET_PORT_NOR: {
			char Tmp[20];
			sprintf(Tmp, "%d", PORT_NOR);
			SendDlgItemMessage(hDlg, HSET_PORT, WM_SETTEXT, 0, (LPARAM)Tmp);
			break;
		}
		}
	}
};

struct KanjiCode {
	static constexpr WORD dialogId = hset_code_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using KanjiButton = RadioButton<HSET_NO_CNV, HSET_SJIS_CNV, HSET_JIS_CNV, HSET_EUC_CNV, HSET_UTF8N_CNV, HSET_UTF8BOM_CNV>;
	using NameKanjiButton = RadioButton<HSET_FN_AUTO_CNV, HSET_FN_SJIS_CNV, HSET_FN_JIS_CNV, HSET_FN_EUC_CNV, HSET_FN_SMH_CNV, HSET_FN_SMC_CNV, HSET_FN_UTF8N_CNV, HSET_FN_UTF8HFSX_CNV>;
	static INT_PTR OnInit(HWND hDlg) {
		KanjiButton::Set(hDlg, TmpHost.KanjiCode);
		SendDlgItemMessageW(hDlg, HSET_HANCNV, BM_SETCHECK, TmpHost.KanaCnv, 0);
		NameKanjiButton::Set(hDlg, TmpHost.NameKanjiCode);
		if (!SupportIdn)
			EnableWindow(GetDlgItem(hDlg, HSET_FN_UTF8HFSX_CNV), FALSE);
		SendDlgItemMessageW(hDlg, HSET_FN_HANCNV, BM_SETCHECK, TmpHost.NameKanaCnv, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			TmpHost.KanjiCode = KanjiButton::Get(hDlg);
			TmpHost.KanaCnv = (int)SendDlgItemMessageW(hDlg, HSET_HANCNV, BM_GETCHECK, 0, 0);
			TmpHost.NameKanjiCode = NameKanjiButton::Get(hDlg);
			TmpHost.NameKanaCnv = (int)SendDlgItemMessageW(hDlg, HSET_FN_HANCNV, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000030);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case HSET_SJIS_CNV:
		case HSET_JIS_CNV:
		case HSET_EUC_CNV:
			EnableWindow(GetDlgItem(hDlg, HSET_HANCNV), TRUE);
			break;
		case HSET_NO_CNV:
		case HSET_UTF8N_CNV:
		case HSET_UTF8BOM_CNV:
			EnableWindow(GetDlgItem(hDlg, HSET_HANCNV), FALSE);
			break;
		case HSET_FN_JIS_CNV:
		case HSET_FN_EUC_CNV:
			EnableWindow(GetDlgItem(hDlg, HSET_FN_HANCNV), TRUE);
			break;
		case HSET_FN_AUTO_CNV:
		case HSET_FN_SJIS_CNV:
		case HSET_FN_SMH_CNV:
		case HSET_FN_SMC_CNV:
		case HSET_FN_UTF8N_CNV:
		case HSET_FN_UTF8HFSX_CNV:
			EnableWindow(GetDlgItem(hDlg, HSET_FN_HANCNV), FALSE);
			break;
		}
	}
};

struct Dialup {
	static constexpr WORD dialogId = hset_dialup_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, HSET_DIALUP, BM_SETCHECK, TmpHost.Dialup, 0);
		SendDlgItemMessageW(hDlg, HSET_DIALUSETHIS, BM_SETCHECK, TmpHost.DialupAlways, 0);
		SendDlgItemMessageW(hDlg, HSET_DIALNOTIFY, BM_SETCHECK, TmpHost.DialupNotify, 0);
		if (NoRasControl != NO)
			EnableWindow(GetDlgItem(hDlg, HSET_DIALUP), FALSE);
		if (TmpHost.DialupAlways == NO || NoRasControl != NO)
			EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
		if (TmpHost.Dialup == NO || NoRasControl != NO) {
			EnableWindow(GetDlgItem(hDlg, HSET_DIALENTRY), FALSE);
			EnableWindow(GetDlgItem(hDlg, HSET_DIALUSETHIS), FALSE);
			EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
		}
		SetRasEntryToComboBox(hDlg, HSET_DIALENTRY, u8(TmpHost.DialEntry));
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			TmpHost.Dialup = (int)SendDlgItemMessageW(hDlg, HSET_DIALUP, BM_GETCHECK, 0, 0);
			TmpHost.DialupAlways = (int)SendDlgItemMessageW(hDlg, HSET_DIALUSETHIS, BM_GETCHECK, 0, 0);
			TmpHost.DialupNotify = (int)SendDlgItemMessageW(hDlg, HSET_DIALNOTIFY, BM_GETCHECK, 0, 0);
			SendDlgItemMessage(hDlg, HSET_DIALENTRY, WM_GETTEXT, RAS_NAME_LEN + 1, (LPARAM)TmpHost.DialEntry);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000031);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case HSET_DIALUP:
			if (SendDlgItemMessageW(hDlg, HSET_DIALUP, BM_GETCHECK, 0, 0) == 0) {
				EnableWindow(GetDlgItem(hDlg, HSET_DIALENTRY), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_DIALUSETHIS), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
				break;
			} else {
				EnableWindow(GetDlgItem(hDlg, HSET_DIALENTRY), TRUE);
				EnableWindow(GetDlgItem(hDlg, HSET_DIALUSETHIS), TRUE);
			}
			[[fallthrough]];
		case HSET_DIALUSETHIS:
			if (SendDlgItemMessageW(hDlg, HSET_DIALUSETHIS, BM_GETCHECK, 0, 0) == 0)
				EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), FALSE);
			else
				EnableWindow(GetDlgItem(hDlg, HSET_DIALNOTIFY), TRUE);
			break;
		}
	}
};

struct Special {
	static constexpr WORD dialogId = hset_adv2_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, HSET_CHMOD_CMD, EM_LIMITTEXT, CHMOD_CMD_LEN, 0);
		SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, WM_SETTEXT, 0, (LPARAM)TmpHost.ChmodCmd);
		SendDlgItemMessageW(hDlg, HSET_LS_FNAME, EM_LIMITTEXT, NLST_NAME_LEN, 0);
		SendDlgItemMessage(hDlg, HSET_LS_FNAME, WM_SETTEXT, 0, (LPARAM)TmpHost.LsName);
		SendDlgItemMessageW(hDlg, HSET_LISTCMD, BM_SETCHECK, TmpHost.ListCmdOnly, 0);
		if (TmpHost.ListCmdOnly == YES)
			EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
		else
			EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), FALSE);
		SendDlgItemMessageW(hDlg, HSET_MLSDCMD, BM_SETCHECK, TmpHost.UseMLSD, 0);
		SendDlgItemMessageW(hDlg, HSET_NLST_R, BM_SETCHECK, TmpHost.UseNLST_R, 0);
		SendDlgItemMessageW(hDlg, HSET_FULLPATH, BM_SETCHECK, TmpHost.NoFullPath, 0);
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
		SendDlgItemMessageW(hDlg, HSET_HOSTTYPE, CB_SETCURSEL, TmpHost.HostType, 0);
#if defined(HAVE_TANDEM)
		if (TmpHost.HostType == 2 || TmpHost.HostType == HTYPE_TANDEM)  /* VAX or Tandem */
#else
		if (TmpHost.HostType == 2)
#endif
		{
			EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
			EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), FALSE);
			EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), FALSE);
		}
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, WM_GETTEXT, CHMOD_CMD_LEN + 1, (LPARAM)TmpHost.ChmodCmd);
			SendDlgItemMessage(hDlg, HSET_LS_FNAME, WM_GETTEXT, NLST_NAME_LEN + 1, (LPARAM)TmpHost.LsName);
			TmpHost.ListCmdOnly = (int)SendDlgItemMessageW(hDlg, HSET_LISTCMD, BM_GETCHECK, 0, 0);
			TmpHost.UseMLSD = (int)SendDlgItemMessageW(hDlg, HSET_MLSDCMD, BM_GETCHECK, 0, 0);
			TmpHost.UseNLST_R = (int)SendDlgItemMessageW(hDlg, HSET_NLST_R, BM_GETCHECK, 0, 0);
			TmpHost.NoFullPath = (int)SendDlgItemMessageW(hDlg, HSET_FULLPATH, BM_GETCHECK, 0, 0);
			TmpHost.HostType = (int)SendDlgItemMessageW(hDlg, HSET_HOSTTYPE, CB_GETCURSEL, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000032);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case HSET_CHMOD_NOR:
			SendDlgItemMessage(hDlg, HSET_CHMOD_CMD, WM_SETTEXT, 0, (LPARAM)CHMOD_CMD_NOR);
			break;
		case HSET_LS_FNAME_NOR:
			SendDlgItemMessage(hDlg, HSET_LS_FNAME, WM_SETTEXT, 0, (LPARAM)LS_FNAME);
			break;
		case HSET_LISTCMD:
			if (SendDlgItemMessageW(hDlg, HSET_LISTCMD, BM_GETCHECK, 0, 0) == 0) {
				EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
			} else {
				EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), TRUE);
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
			}
			break;
		case HSET_HOSTTYPE:
			if (auto Num = (int)SendDlgItemMessageW(hDlg, HSET_HOSTTYPE, CB_GETCURSEL, 0, 0); Num == 2) {
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), FALSE);
			}
#if defined(HAVE_TANDEM)
			else if (Num == HTYPE_TANDEM) /* Tandem */
			{
				/* Tandem を選択すると自動的に LIST にチェックをいれる */
				SendDlgItemMessageW(hDlg, HSET_LISTCMD, BM_SETCHECK, 1, 0);
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), FALSE);
				EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), FALSE);
			} else {
				if (SendDlgItemMessageW(hDlg, HSET_LISTCMD, BM_GETCHECK, 0, 0) == 0) {
					EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
					EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), TRUE);
				} else {
					EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
					EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), TRUE);
				}
				EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), TRUE);
			}
#else
			else {
				EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), TRUE);
				EnableWindow(GetDlgItem(hDlg, HSET_LISTCMD), TRUE);
				EnableWindow(GetDlgItem(hDlg, HSET_FULLPATH), TRUE);
			}
#endif
			break;
		}
	}
};

struct Encryption {
	static constexpr WORD dialogId = hset_crypt_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, HSET_NO_ENCRYPTION, BM_SETCHECK, TmpHost.UseNoEncryption, 0);
		SendDlgItemMessageW(hDlg, HSET_FTPES, BM_SETCHECK, TmpHost.UseFTPES, 0);
		SendDlgItemMessageW(hDlg, HSET_FTPIS, BM_SETCHECK, TmpHost.UseFTPIS, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			TmpHost.UseNoEncryption = (int)SendDlgItemMessageW(hDlg, HSET_NO_ENCRYPTION, BM_GETCHECK, 0, 0);
			TmpHost.UseFTPES = (int)SendDlgItemMessageW(hDlg, HSET_FTPES, BM_GETCHECK, 0, 0);
			TmpHost.UseFTPIS = (int)SendDlgItemMessageW(hDlg, HSET_FTPIS, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000065);
			break;
		}
		return 0;
	}
};

struct Feature {
	static constexpr WORD dialogId = hset_adv3_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, HSET_THREAD_COUNT, EM_LIMITTEXT, (WPARAM)1, 0);
		SetDecimalText(hDlg, HSET_THREAD_COUNT, TmpHost.MaxThreadCount);
		SendDlgItemMessageW(hDlg, HSET_THREAD_COUNT_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(MAX_DATA_CONNECTION, 1));
		SendDlgItemMessageW(hDlg, HSET_REUSE_SOCKET, BM_SETCHECK, TmpHost.ReuseCmdSkt, 0);
		SendDlgItemMessageW(hDlg, HSET_NOOP_INTERVAL, EM_LIMITTEXT, (WPARAM)3, 0);
		SetDecimalText(hDlg, HSET_NOOP_INTERVAL, TmpHost.NoopInterval);
		SendDlgItemMessageW(hDlg, HSET_NOOP_INTERVAL_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(300, 0));
		SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN335);
		SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN336);
		SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN337);
		SendDlgItemMessage(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)MSGJPN338);
		if (TmpHost.TransferErrorNotify == YES)
			SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 0, 0);
		else if (TmpHost.TransferErrorMode == EXIST_OVW)
			SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 1, 0);
		else if (TmpHost.TransferErrorMode == EXIST_RESUME)
			SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 2, 0);
		else if (TmpHost.TransferErrorMode == EXIST_IGNORE)
			SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 3, 0);
		else
			SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_SETCURSEL, 0, 0);
		SendDlgItemMessageW(hDlg, HSET_ERROR_RECONNECT, BM_SETCHECK, TmpHost.TransferErrorReconnect, 0);
		SendDlgItemMessageW(hDlg, HSET_NO_PASV_ADRS, BM_SETCHECK, TmpHost.NoPasvAdrs, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			TmpHost.MaxThreadCount = GetDecimalText(hDlg, HSET_THREAD_COUNT);
			CheckRange2(&TmpHost.MaxThreadCount, MAX_DATA_CONNECTION, 1);
			TmpHost.ReuseCmdSkt = (int)SendDlgItemMessageW(hDlg, HSET_REUSE_SOCKET, BM_GETCHECK, 0, 0);
			TmpHost.NoopInterval = GetDecimalText(hDlg, HSET_NOOP_INTERVAL);
			CheckRange2(&TmpHost.NoopInterval, 300, 0);
			switch (SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_GETCURSEL, 0, 0)) {
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
			TmpHost.TransferErrorReconnect = (int)SendDlgItemMessageW(hDlg, HSET_ERROR_RECONNECT, BM_GETCHECK, 0, 0);
			TmpHost.NoPasvAdrs = (int)SendDlgItemMessageW(hDlg, HSET_NO_PASV_ADRS, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000066);
			break;
		}
		return 0;
	}
};

// ホスト設定のプロパティシート
static bool DispHostSetDlg(HWND hDlg) {
	auto result = PropSheet<General, Advanced, KanjiCode, Dialup, Special, Encryption, Feature>(hDlg, GetFtpInst(), IDS_HOSTSETTING, PSH_NOAPPLYNOW);
	return 1 <= result;
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

