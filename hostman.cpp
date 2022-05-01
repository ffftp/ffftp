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

struct HOSTLISTDATA : HOSTDATA, std::enable_shared_from_this<HOSTLISTDATA> {
	std::shared_ptr<HOSTLISTDATA> Next;
	std::shared_ptr<HOSTLISTDATA> Prev;
	std::shared_ptr<HOSTLISTDATA> Child;
	std::shared_ptr<HOSTLISTDATA> Parent;
	explicit HOSTLISTDATA(HOSTDATA const& Set) : HOSTDATA{ Set } {}

	// 次の設定番号のノードを返す
	//   empty=次はない
	std::shared_ptr<HOSTLISTDATA> GetNext() {
		auto p = shared_from_this();
		if (p->Child)
			return p->Child;
		if (p->Next)
			return p->Next;
		while (p = p->Parent)
			if (p->Next)
				return p->Next;
		return {};
	}

	// ノードのレベル数を返す
	int GetLevel() {
		int level = 0;
		for (auto p = shared_from_this(); p->Parent; p = p->Parent)
			level++;
		return level;
	}
};

static int GetLevel(int Num);
static int GetNum(std::shared_ptr<HOSTLISTDATA> Data);
static std::shared_ptr<HOSTLISTDATA> GetNode(int Num);
static void SetNodeLevelAll();
static int UpdateHostToList(int Num, HOSTDATA *Set);
static int DelHostFromList(int Num);
static void DeleteChildAndNext(std::shared_ptr<HOSTLISTDATA> Pos);
static void SendAllHostNames(HWND hWnd, int Cur);
static int IsNodeGroup(int Num);
static bool DispHostSetDlg(HWND hDlg);
static int Hosts = 0;								/* ホスト数 */
static int ConnectingHost;							/* 接続中のホスト */
static int CurrentHost;								/* カーソル位置のホスト */
static std::shared_ptr<HOSTLISTDATA> HostListTop;	/* ホスト一覧データ */
static HOSTDATA TmpHost;							/* ホスト情報コピー用 */

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
					TmpHost.Level = GetLevel((int)Item.lParam);
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
			if (InputDialog(group_dlg, hDlg, 0, TmpHost.HostName, HOST_NAME_LEN + 1)) {
				int Level1 = -1;
				if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
					TVITEMW Item{ TVIF_PARAM, hItem };
					SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
					TmpHost.Level = GetLevel((int)Item.lParam) | SET_LEVEL_GROUP;
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
				int const Level1 = IsNodeGroup(CurrentHost);
				auto set = Level1 == NO && DispHostSetDlg(hDlg);
				if (!set && Level1 == YES)
					set = InputDialog(group_dlg, hDlg, 0, TmpHost.HostName, HOST_NAME_LEN + 1);
				if (set) {
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
				TmpHost.BookMark = {};
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
				int const Level1 = IsNodeGroup(CurrentHost);
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
					auto Data1 = GetNode(CurrentHost);
					int const Level1 = Data1->GetLevel();
					auto Data2 = GetNode(CurrentHost - 1);
					int const Level2 = Data2->GetLevel();
					if (Level1 == Level2 && (Data2->Level & SET_LEVEL_GROUP)) {
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

					CurrentHost = GetNum(Data1);
					SendAllHostNames(GetDlgItem(hDlg, HOST_LIST), CurrentHost);
				}
			}
			break;
		case HOST_DOWN:
			if (auto hItem = (HTREEITEM)SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETNEXTITEM, TVGN_CARET, 0)) {
				TVITEMW Item{ TVIF_PARAM, hItem };
				SendDlgItemMessageW(hDlg, HOST_LIST, TVM_GETITEMW, TVGN_CARET, (LPARAM)&Item);
				CurrentHost = (int)Item.lParam;

				auto Data1 = GetNode(CurrentHost);
				int const Level1 = Data1->GetLevel();
				std::shared_ptr<HOSTLISTDATA> Data2;
				int Level2 = SET_LEVEL_SAME;
				if (CurrentHost < Hosts - 1) {
					Data2 = GetNode(CurrentHost + 1);
					Level2 = Data2->GetLevel();

					if (Level1 < Level2) {
						if (Data1->Next != NULL) {
							//Data2 = Data1のNext
							Data2 = Data1->Next;
							Level2 = Data2->GetLevel();
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
					if (Data2->Level & SET_LEVEL_GROUP) {
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

				CurrentHost = GetNum(Data1);
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


// ノードのレベル数を返す
//   -1=設定がない
static int GetLevel(int Num) {
	if (Num < 0 || Hosts <= Num)
		return -1;
	return GetNode(Num)->GetLevel();
}


// ノードの設定番号を返す
static int GetNum(std::shared_ptr<HOSTLISTDATA> Data) {
	int num = 0;
	for (auto Pos = HostListTop; Pos != Data; Pos = Pos->GetNext())
		num++;
	return num;
}


// 指定番号のノードを返す
static std::shared_ptr<HOSTLISTDATA> GetNode(int Num) {
	auto Pos = HostListTop;
	while (0 < Num--)
		Pos = Pos->GetNext();
	return Pos;
}


// 設定値リストの各ノードのレベル番号をセット
static void SetNodeLevelAll() {
	auto Pos = HostListTop;
	for (int i = 0; i < Hosts; i++) {
		Pos->Level = Pos->Level & ~SET_LEVEL_MASK | Pos->GetLevel();
		Pos = Pos->GetNext();
	}
}


// 設定値リストに追加
//   HOSTDATA *Set : 追加する設定値
//   int Pos : 追加する位置 (0～ : -1=最後)
//   int Level : レベル数 (SET_LEVEL_SAME=追加位置のものと同レベル)
int AddHostToList(HOSTDATA* Set, int Pos, int Level) {
	if (Pos == -1)
		Pos = Hosts;
	if (Pos < 0 || Hosts < Pos)
		return FFFTP_FAIL;
	Level &= SET_LEVEL_MASK;

	auto New = std::make_shared<HOSTLISTDATA>(*Set);
	if (Pos == 0) {
		if (HostListTop)
			New->Next = HostListTop;
		HostListTop = New;
	} else {
		auto Last = GetNode(Pos - 1);
		int Cur = Last->GetLevel();
		if (Cur < Level && Level != SET_LEVEL_SAME) {
			New->Next = Last->Child;
			New->Parent = Last;
			Last->Child = New;
		} else {
			if (0 <= Level && Level < SET_LEVEL_SAME)
				while (Level < Cur--)
					Last = Last->Parent;
			New->Prev = Last;
			New->Next = Last->Next;
			New->Parent = Last->Parent;
			Last->Next = New;
		}
		if (New->Next)
			New->Next->Prev = New;
	}
	Hosts++;
	return FFFTP_SUCCESS;
}


// 設定値リストを更新する
static int UpdateHostToList(int Num, HOSTDATA* Set) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	static_cast<HOSTDATA&>(*Pos) = *Set;
	return FFFTP_SUCCESS;
}


// 設定値リストから削除
static int DelHostFromList(int Num) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	DeleteChildAndNext(Pos);
	if (Num == 0)
		HostListTop = Pos->Next;
	else {
		if (Pos->Next)
			Pos->Next->Prev = Pos->Prev;
		if (Pos->Prev)
			Pos->Prev->Next = Pos->Next;
		if (Pos->Parent && Pos->Parent->Child == Pos)
			Pos->Parent->Child = Pos->Next;
	}
	//free(Pos);
	Hosts--;
	return FFFTP_SUCCESS;
}


// 設定値リストからノードデータを削除
//   Pos->Next, Pos->Childの全てのノードを削除する
static void DeleteChildAndNext(std::shared_ptr<HOSTLISTDATA> Pos) {
	if (!Pos->Child)
		return;
	Pos = Pos->Child;
	while (Pos) {
		DeleteChildAndNext(Pos);
		auto Next = Pos->Next;
		//free(Pos);
		Hosts--;
		Pos = Next;
	}
}


// 設定値リストから設定値を取り出す
//   現在ホストに接続中の時は、GetConnectingHost() を使う事
int CopyHostFromList(int Num, HOSTDATA* Set) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	*Set = *Pos;
	return FFFTP_SUCCESS;
}


HOSTDATA GetConnectingHost() {
	auto host = GetCurHost();
	if (0 <= ConnectingHost && ConnectingHost < Hosts) {
		auto pos = GetNode(ConnectingHost);
		host.ChmodCmd = pos->ChmodCmd;
		host.Port = pos->Port;
		host.Anonymous = pos->Anonymous;
		host.KanjiCode = pos->KanjiCode;
		host.KanaCnv = pos->KanaCnv;
		host.NameKanjiCode = pos->NameKanjiCode;
		host.NameKanaCnv = pos->NameKanaCnv;
		host.Pasv = pos->Pasv;
		host.FireWall = pos->FireWall;
		host.ListCmdOnly = pos->ListCmdOnly;
		host.UseNLST_R = pos->UseNLST_R;
		host.LastDir = pos->LastDir;
		host.TimeZone = pos->TimeZone;
		host.UseNoEncryption = pos->UseNoEncryption;
		host.UseFTPES = pos->UseFTPES;
		host.UseFTPIS = pos->UseFTPIS;
		host.UseSFTP = pos->UseSFTP;
		host.MaxThreadCount = pos->MaxThreadCount;
		host.ReuseCmdSkt = pos->ReuseCmdSkt;
		host.UseMLSD = pos->UseMLSD;
		host.NoopInterval = pos->NoopInterval;
		host.TransferErrorMode = pos->TransferErrorMode;
		host.TransferErrorNotify = pos->TransferErrorNotify;
		host.TransferErrorReconnect = pos->TransferErrorReconnect;
		host.NoPasvAdrs = pos->NoPasvAdrs;
	}
	return host;
}


// 設定値リストのブックマークを更新
int SetHostBookMark(int Num, std::vector<std::wstring>&& bookmark) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	Pos->BookMark = std::move(bookmark);
	return FFFTP_SUCCESS;
}


// 設定値リストのブックマーク文字列を返す
std::optional<std::vector<std::wstring>> AskHostBookMark(int Num) {
	if (Num < 0 || Hosts <= Num)
		return {};
	auto Pos = GetNode(Num);
	return Pos->BookMark;
}


// 設定値リストのディレクトリを更新
int SetHostDir(int Num, std::wstring_view LocDir, std::wstring_view HostDir) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	Pos->LocalInitDir = LocDir;
	Pos->RemoteInitDir = HostDir;
	return FFFTP_SUCCESS;
}


// 設定値リストのパスワードを更新
int SetHostPassword(int Num, std::wstring const& Pass) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	Pos->PassWord = Pass;
	return FFFTP_SUCCESS;
}


// 指定の設定名を持つ設定の番号を返す
//   -1=見つからない
int SearchHostName(std::wstring_view name) {
	auto Pos = HostListTop;
	for (int i = 0; i < Hosts; i++) {
		if (Pos->HostName == name)
			return i;
		Pos = Pos->GetNext();
	}
	return -1;
}


// 設定値リストのソート方法を更新
int SetHostSort(int Num, HostSort const& sort) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	Pos->Sort = sort;
	return FFFTP_SUCCESS;
}


// 現在接続中の設定番号を返す
int AskCurrentHost() noexcept {
	return ConnectingHost;
}


// 現在接続中の設定番号をセットする
void SetCurrentHost(int Num) noexcept {
	ConnectingHost = Num;
}


// デフォルト設定値を取り出す
void CopyDefaultHost(HOSTDATA* Set) {
	*Set = DefaultHost;
}


// ホスト共通設定機能
void ResetDefaultHost() {
	DefaultHost = {};
}

void SetDefaultHost(HOSTDATA* Set) {
	DefaultHost = *Set;
}

HostExeptPassword::HostExeptPassword() noexcept {
	LocalInitDir = DefaultLocalPath;
}


// 設定名一覧をウィンドウに送る
static void SendAllHostNames(HWND hWnd, int Cur) {
	HTREEITEM hItemCur = NULL;
	SendMessageW(hWnd, WM_SETREDRAW, false, 0);				// ちらつき防止
	SendMessageW(hWnd, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);

	std::vector<HTREEITEM> Level(Hosts);
	auto Pos = HostListTop;
	for (int i = 0; i < Hosts; i++) {
		size_t const CurLevel = Pos->GetLevel();
		TVINSERTSTRUCTW is{
			.hParent = CurLevel == 0 ? TVI_ROOT : Level[CurLevel - 1],
			.hInsertAfter = TVI_LAST,
			.item = { .mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM, .pszText = data(Pos->HostName), .cChildren = 1, .lParam = i },
		};
		if (!(Pos->Level & SET_LEVEL_GROUP))
			is.item.iImage = is.item.iSelectedImage = 2;
		auto hItem = (HTREEITEM)SendMessageW(hWnd, TVM_INSERTITEMW, 0, (LPARAM)&is);
		if (Pos->Level & SET_LEVEL_GROUP)
			Level[CurLevel] = hItem;
		if (i == Cur)
			hItemCur = hItem;
		Pos = Pos->GetNext();
	}

	SendMessageW(hWnd, WM_SETREDRAW, true, 0);				// 描画再開
	if (hItemCur != NULL)
		SendMessageW(hWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItemCur);
	UpdateWindow(hWnd);
}


// 設定値がグループかどうかを返す
//   -1=設定値がない
static int IsNodeGroup(int Num) {
	if (Num < 0 || Hosts <= Num)
		return -1;
	auto Pos = GetNode(Num);
	return (Pos->Level & SET_LEVEL_GROUP) ? YES : NO;
}


// WS_FTP.INIからのインポート
void ImportFromWSFTP() {
	if (auto const path = SelectFile(true, GetMainHwnd(), IDS_OPEN_WSFTPINI, L"WS_FTP.INI", nullptr, { FileType::Ini, FileType::All }); !path.empty())
		if (std::ifstream fs{ path }) {
			HOSTDATA host;
			auto hasHost = false;
			for (std::string line; getline(fs, line);) {
				if (line[0] == '[') {
					if (hasHost) {
						AddHostToList(&host, -1, 0);
						hasHost = false;
					}
					if (!ieq(line, "[_config_]"sv)) {
						CopyDefaultHost(&host);
						host.HostName = u8({ &line[1], size(line) - 2 });
						hasHost = true;
					}
				} else if (auto const pos = line.find('='); hasHost && pos != std::string::npos) {
					auto name = lc(line.substr(0, pos));
					name.erase(std::remove_if(begin(name), end(name), [](auto ch) { return ch == ' ' || ch == '\t' || ch == '\n'; }), end(name));
					auto value = line.substr(pos + 1);
					value.erase(std::remove_if(begin(value), end(value), [](char ch) { return ch == '"' || ch == '\n'; }), end(value));
					if (name == "host"sv)
						host.HostAdrs = u8(value);
					else if (name == "uid"sv) {
						host.UserName = u8(value);
						if (value == "anonymous"sv)
							host.PassWord = UserMailAdrs;
					} else if (name == "locdir"sv)
						host.LocalInitDir = u8(value);
					else if (name == "dir"sv)
						host.RemoteInitDir = u8(value);
					else if (name == "pasvmode"sv)
						host.Pasv = stoi(value) == 0 ? 0 : 1;
					else if (name == "firewall"sv)
						host.FireWall = stoi(value) == 0 ? 0 : 1;
				}
			}
			if (hasHost)
				AddHostToList(&host, -1, 0);
		}
}


struct General {
	static constexpr WORD dialogId = hset_main_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) noexcept {
		SendDlgItemMessageW(hDlg, HSET_HOST, EM_LIMITTEXT, HOST_NAME_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_ADRS, EM_LIMITTEXT, HOST_ADRS_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_LOCAL, EM_LIMITTEXT, INIT_DIR_LEN, 0);
		SendDlgItemMessageW(hDlg, HSET_REMOTE, EM_LIMITTEXT, INIT_DIR_LEN, 0);
		SetText(hDlg, HSET_HOST, TmpHost.HostName);
		SetText(hDlg, HSET_ADRS, TmpHost.HostAdrs);
		SetText(hDlg, HSET_USER, TmpHost.UserName);
		SetText(hDlg, HSET_PASS, TmpHost.PassWord);
		SetText(hDlg, HSET_LOCAL, TmpHost.LocalInitDir);
		SetText(hDlg, HSET_REMOTE, TmpHost.RemoteInitDir);
		SendDlgItemMessageW(hDlg, HSET_ANONYMOUS, BM_SETCHECK, TmpHost.Anonymous, 0);
		SendDlgItemMessageW(hDlg, HSET_LASTDIR, BM_SETCHECK, TmpHost.LastDir, 0);
		if (AskConnecting() == NO)
			EnableWindow(GetDlgItem(hDlg, HSET_REMOTE_CUR), FALSE);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			TmpHost.HostName = GetText(hDlg, HSET_HOST);
			TmpHost.HostAdrs = GetText(hDlg, HSET_ADRS);
			if (auto const p = TmpHost.HostAdrs.find_last_not_of(L' '); p != std::wstring::npos && p + 1 != size(TmpHost.HostAdrs))
				TmpHost.HostAdrs.erase(p + 1);
			TmpHost.UserName = GetText(hDlg, HSET_USER);
			TmpHost.PassWord = GetText(hDlg, HSET_PASS);
			TmpHost.LocalInitDir = GetText(hDlg, HSET_LOCAL);
			TmpHost.RemoteInitDir = GetText(hDlg, HSET_REMOTE);
			TmpHost.Anonymous = (int)SendDlgItemMessageW(hDlg, HSET_ANONYMOUS, BM_GETCHECK, 0, 0);
			TmpHost.LastDir = (int)SendDlgItemMessageW(hDlg, HSET_LASTDIR, BM_GETCHECK, 0, 0);
			if (empty(TmpHost.HostName) && !empty(TmpHost.HostAdrs))
				TmpHost.HostName = TmpHost.HostAdrs;
			else if (!empty(TmpHost.HostName) && empty(TmpHost.HostAdrs))
				TmpHost.HostAdrs = TmpHost.HostName;
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
			if (auto path = SelectDir(hDlg); !path.empty())
				SetText(hDlg, HSET_LOCAL, path);
			break;
		case HSET_REMOTE_CUR:
			SetText(hDlg, HSET_REMOTE, AskRemoteCurDir());
			break;
		case HSET_ANONYMOUS:
			if (SendDlgItemMessageW(hDlg, HSET_ANONYMOUS, BM_GETCHECK, 0, 0) == 1) {
				SetText(hDlg, HSET_USER, L"anonymous");
				auto const wStyle = GetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
				SetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle & ~ES_PASSWORD);
				SetText(hDlg, HSET_PASS, UserMailAdrs);
			} else {
				SetText(hDlg, HSET_USER, L"");
				auto const wStyle = GetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE);
				SetWindowLongPtrW(GetDlgItem(hDlg, HSET_PASS), GWL_STYLE, wStyle | ES_PASSWORD);
				SetText(hDlg, HSET_PASS, L"");
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
		SetText(hDlg, HSET_PORT, std::to_wstring(TmpHost.Port));
		SendDlgItemMessageW(hDlg, HSET_ACCOUNT, EM_LIMITTEXT, ACCOUNT_LEN, 0);
		SetText(hDlg, HSET_ACCOUNT, TmpHost.Account);
		SendDlgItemMessageW(hDlg, HSET_PASV, BM_SETCHECK, TmpHost.Pasv, 0);
		SendDlgItemMessageW(hDlg, HSET_FIREWALL, BM_SETCHECK, TmpHost.FireWall, 0);
		SendDlgItemMessageW(hDlg, HSET_SYNCMOVE, BM_SETCHECK, TmpHost.SyncMove, 0);
		for (int i = -12; i <= 12; i++) {
			auto tz = i == 0 ? L"GMT"s : i == 9 ? GetString(IDS_MSGJPN133) : std::format(L"GMT{:+03}:00"sv, i);
			SendDlgItemMessageW(hDlg, HSET_TIMEZONE, CB_ADDSTRING, 0, (LPARAM)tz.c_str());
		}
		SendDlgItemMessageW(hDlg, HSET_TIMEZONE, CB_SETCURSEL, (UINT_PTR)TmpHost.TimeZone + 12, 0);

		for (auto resourceId : { IDS_MSGJPN134, IDS_MSGJPN135, IDS_MSGJPN136, IDS_MSGJPN137, IDS_MSGJPN138 })
			SendDlgItemMessageW(hDlg, HSET_SECURITY, CB_ADDSTRING, 0, (LPARAM)GetString(resourceId).c_str());
		SendDlgItemMessageW(hDlg, HSET_SECURITY, CB_SETCURSEL, TmpHost.Security, 0);
		SendDlgItemMessageW(hDlg, HSET_INITCMD, EM_LIMITTEXT, INITCMD_LEN, 0);
		SetText(hDlg, HSET_INITCMD, TmpHost.InitCmd);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			TmpHost.Pasv = (int)SendDlgItemMessageW(hDlg, HSET_PASV, BM_GETCHECK, 0, 0);
			TmpHost.FireWall = (int)SendDlgItemMessageW(hDlg, HSET_FIREWALL, BM_GETCHECK, 0, 0);
			TmpHost.SyncMove = (int)SendDlgItemMessageW(hDlg, HSET_SYNCMOVE, BM_GETCHECK, 0, 0);
			TmpHost.Port = GetDecimalText(hDlg, HSET_PORT);
			TmpHost.Account = GetText(hDlg, HSET_ACCOUNT);
			TmpHost.TimeZone = (int)SendDlgItemMessageW(hDlg, HSET_TIMEZONE, CB_GETCURSEL, 0, 0) - 12;
			TmpHost.Security = (int)SendDlgItemMessageW(hDlg, HSET_SECURITY, CB_GETCURSEL, 0, 0);
			TmpHost.InitCmd = GetText(hDlg, HSET_INITCMD);
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
		case HSET_PORT_NOR:
			SetText(hDlg, HSET_PORT, std::to_wstring(IPPORT_FTP));
			break;
		}
	}
};

struct KanjiCode {
	static constexpr WORD dialogId = hset_code_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using KanjiButton = RadioButton<HSET_NO_CNV, HSET_SJIS_CNV, HSET_JIS_CNV, HSET_EUC_CNV, HSET_UTF8N_CNV, HSET_UTF8BOM_CNV>;
	using NameKanjiButton = RadioButton<HSET_FN_AUTO_CNV, HSET_FN_SJIS_CNV, HSET_FN_JIS_CNV, HSET_FN_EUC_CNV, HSET_FN_SMH_CNV, HSET_FN_SMC_CNV, HSET_FN_UTF8N_CNV, HSET_FN_UTF8HFSX_CNV>;
	static INT_PTR OnInit(HWND hDlg) noexcept {
		KanjiButton::Set(hDlg, TmpHost.KanjiCode);
		SendDlgItemMessageW(hDlg, HSET_HANCNV, BM_SETCHECK, TmpHost.KanaCnv, 0);
		NameKanjiButton::Set(hDlg, TmpHost.NameKanjiCode);
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
	static void OnCommand(HWND hDlg, WORD id) noexcept {
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
		SetRasEntryToComboBox(hDlg, HSET_DIALENTRY, TmpHost.DialEntry);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			TmpHost.Dialup = (int)SendDlgItemMessageW(hDlg, HSET_DIALUP, BM_GETCHECK, 0, 0);
			TmpHost.DialupAlways = (int)SendDlgItemMessageW(hDlg, HSET_DIALUSETHIS, BM_GETCHECK, 0, 0);
			TmpHost.DialupNotify = (int)SendDlgItemMessageW(hDlg, HSET_DIALNOTIFY, BM_GETCHECK, 0, 0);
			TmpHost.DialEntry = GetText(hDlg, HSET_DIALENTRY);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000031);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) noexcept {
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
		SetText(hDlg, HSET_CHMOD_CMD, TmpHost.ChmodCmd);
		SendDlgItemMessageW(hDlg, HSET_LS_FNAME, EM_LIMITTEXT, NLST_NAME_LEN, 0);
		SetText(hDlg, HSET_LS_FNAME, TmpHost.LsName);
		SendDlgItemMessageW(hDlg, HSET_LISTCMD, BM_SETCHECK, TmpHost.ListCmdOnly, 0);
		if (TmpHost.ListCmdOnly == YES)
			EnableWindow(GetDlgItem(hDlg, HSET_NLST_R), FALSE);
		else
			EnableWindow(GetDlgItem(hDlg, HSET_MLSDCMD), FALSE);
		SendDlgItemMessageW(hDlg, HSET_MLSDCMD, BM_SETCHECK, TmpHost.UseMLSD, 0);
		SendDlgItemMessageW(hDlg, HSET_NLST_R, BM_SETCHECK, TmpHost.UseNLST_R, 0);
		SendDlgItemMessageW(hDlg, HSET_FULLPATH, BM_SETCHECK, TmpHost.NoFullPath, 0);
		for (auto resourceId : { IDS_MSGJPN139, IDS_MSGJPN140, IDS_MSGJPN141, IDS_MSGJPN142, IDS_MSGJPN143, IDS_MSGJPN144, IDS_MSGJPN289, IDS_MSGJPN295 })
			SendDlgItemMessageW(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)GetString(resourceId).c_str());
#if defined(HAVE_TANDEM)
		SendDlgItemMessageW(hDlg, HSET_HOSTTYPE, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_MSGJPN2000).c_str());
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
			TmpHost.ChmodCmd = GetText(hDlg, HSET_CHMOD_CMD);
			TmpHost.LsName = GetText(hDlg, HSET_LS_FNAME);
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
	static void OnCommand(HWND hDlg, WORD id) noexcept {
		switch (id) {
		case HSET_CHMOD_NOR:
			SetText(hDlg, HSET_CHMOD_CMD, Host::DefaultChmod);
			break;
		case HSET_LS_FNAME_NOR:
			SetText(hDlg, HSET_LS_FNAME, Host::DefaultLsOption);
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
			if (auto const Num = (int)SendDlgItemMessageW(hDlg, HSET_HOSTTYPE, CB_GETCURSEL, 0, 0); Num == 2) {
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
	static INT_PTR OnInit(HWND hDlg) noexcept {
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
		SetText(hDlg, HSET_THREAD_COUNT, std::to_wstring(TmpHost.MaxThreadCount));
		SendDlgItemMessageW(hDlg, HSET_THREAD_COUNT_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(MAX_DATA_CONNECTION, 1));
		SendDlgItemMessageW(hDlg, HSET_REUSE_SOCKET, BM_SETCHECK, TmpHost.ReuseCmdSkt, 0);
		SendDlgItemMessageW(hDlg, HSET_NOOP_INTERVAL, EM_LIMITTEXT, (WPARAM)3, 0);
		SetText(hDlg, HSET_NOOP_INTERVAL, std::to_wstring(TmpHost.NoopInterval));
		SendDlgItemMessageW(hDlg, HSET_NOOP_INTERVAL_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(300, 0));
		for (auto resourceId : { IDS_MSGJPN335, IDS_MSGJPN336, IDS_MSGJPN337, IDS_MSGJPN338 })
			SendDlgItemMessageW(hDlg, HSET_ERROR_MODE, CB_ADDSTRING, 0, (LPARAM)GetString(resourceId).c_str());
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
			TmpHost.MaxThreadCount = std::clamp(GetDecimalText(hDlg, HSET_THREAD_COUNT), 1, MAX_DATA_CONNECTION);
			TmpHost.ReuseCmdSkt = (int)SendDlgItemMessageW(hDlg, HSET_REUSE_SOCKET, BM_GETCHECK, 0, 0);
			TmpHost.NoopInterval = std::clamp(GetDecimalText(hDlg, HSET_NOOP_INTERVAL), 0, 300);
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
	auto const result = PropSheet<General, Advanced, KanjiCode, Dialup, Special, Encryption, Feature>(hDlg, GetFtpInst(), IDS_HOSTSETTING, PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP);
	return 1 <= result;
}

// 暗号化通信対応
// ホストの暗号化設定を更新
int SetHostEncryption(int Num, int UseNoEncryption, int UseFTPES, int UseFTPIS, int UseSFTP) {
	if (Num < 0 || Hosts <= Num)
		return FFFTP_FAIL;
	auto Pos = GetNode(Num);
	Pos->UseNoEncryption = UseNoEncryption;
	Pos->UseFTPES = UseFTPES;
	Pos->UseFTPIS = UseFTPIS;
	Pos->UseSFTP = UseSFTP;
	return FFFTP_SUCCESS;
}
