/*=============================================================================
*
*								ＦＴＰコマンド操作
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

static int CheckRemoteFile(TRANSPACKET *Pkt, std::vector<FILELIST> const& ListList);
static void DispMirrorFiles(std::vector<FILELIST> const& Local, std::vector<FILELIST> const& Remote);
static void MirrorDeleteAllLocalDir(std::vector<FILELIST> const& Local, TRANSPACKET& item, std::vector<TRANSPACKET>& list);
static int CheckLocalFile(TRANSPACKET *Pkt) noexcept;
static std::wstring RemoveAfterSemicolon(std::wstring&& path);
static void MirrorDeleteAllDir(std::vector<FILELIST> const& Remote, TRANSPACKET& item, std::vector<TRANSPACKET>& list);
static int MirrorNotify(bool upload) noexcept;
static void CountMirrorFiles(HWND hDlg, std::vector<TRANSPACKET> const& list);
static int AskMirrorNoTrn(std::wstring_view path, int Mode);
static int AskUploadFileAttr(std::wstring const& path);
static bool UpDownAsDialog(std::wstring& name, int win) noexcept;
static void DeleteAllDir(std::vector<FILELIST> const& Dt, int Win, int *Sw, int *Flg, std::wstring& CurDir);
static void DelNotifyAndDo(FILELIST const& Dt, int Win, int *Sw, int *Flg, std::wstring& CurDir);
static std::wstring ReformToVMSstyleDirName(std::wstring&& path);
static std::wstring ReformToVMSstylePathName(std::wstring_view path);
static std::wstring RenameUnuseableName(std::wstring&& filename) noexcept;
static int ExistNotify;		/* 確認ダイアログを出すかどうか YES/NO */


static inline auto RemoteName(std::wstring&& name) {
	return FnameCnv == FNAME_LOWER ? lc(std::move(name)) : FnameCnv == FNAME_UPPER ? uc(std::move(name)) : std::move(name);
}

/*----- ファイル一覧で指定されたファイルをダウンロードする --------------------
*
*	Parameter
*		int ChName : 名前を変えるかどうか (YES/NO)
*		int ForceFile : ディレクトリをファイル見なすかどうか (YES/NO)
*		int All : 全てが選ばれている物として扱うかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// ローカル側のパスから必要なフォルダを作成
void MakeDirFromLocalPath(fs::path const& LocalFile, fs::path const& Old) {
	TRANSPACKET Pkt;
	auto oit = Old.begin();
	fs::path path;
	for (auto& current : LocalFile) {
		if (oit != Old.end() && current == *oit) {
			++oit;
			path /= current;
		} else {
			oit = Old.end();
			path /= RemoteName(current);
			Pkt.Local = path;
			Pkt.Command = L"MKD "s;
			Pkt.Remote.clear();
			AddTransFileList(&Pkt);
		}
	}
}

void DownloadProc(int ChName, int ForceFile, int All)
{
	TRANSPACKET Pkt;
	// ファイル一覧バグ修正
	int ListSts;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		ExistNotify = YES;

		std::vector<FILELIST> FileListBase;
		ListSts = MakeSelectedFileList(WIN_REMOTE, ForceFile == YES ? NO : YES, All, FileListBase, &CancelFlg);

		if(AskNoFullPathMode() == YES)
		{
			Pkt.Command = L"SETCUR"s;
			Pkt.Remote = AskRemoteCurDir();
			AddTransFileList(&Pkt);
		}

		for (auto const& f : FileListBase) {
			// ファイル一覧バグ修正
			if (AbortOnListError == YES && ListSts == FFFTP_FAIL)
				break;
			Pkt.Local = AskLocalCurDir();
			auto name = f.Name;
			if (ChName == NO || ForceFile == NO && f.Node == NODE_DIR)
				name = RemoveAfterSemicolon(RemoteName(std::move(name)));
			else {
				if (!UpDownAsDialog(name, WIN_REMOTE))
					break;
			}
			name = RenameUnuseableName(std::move(name));
			if (empty(name))
				break;
			Pkt.Local /= name;

			if (ForceFile == NO && f.Node == NODE_DIR) {
				Pkt.Command = L"MKD "s;
				Pkt.Remote.clear();
				AddTransFileList(&Pkt);
			} else if (f.Node == NODE_FILE || ForceFile == YES && f.Node == NODE_DIR) {
				Pkt.Remote
					= AskHostType() == HTYPE_ACOS ? std::format(L"'{}({})'"sv, GetConnectingHost().LsName, f.Name)
					: AskHostType() == HTYPE_ACOS_4 ? f.Name
					: ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + f.Name, L'\\', L'/');

				Pkt.Command = L"RETR "s;
#if defined(HAVE_TANDEM)
				if (AskHostType() == HTYPE_TANDEM) {
					if (AskTransferType() != TYPE_X) {
						Pkt.Type = AskTransferType();
					} else {
						Pkt.Attr = f.Attr;
						if (Pkt.Attr == 101)
							Pkt.Type = TYPE_A;
						else
							Pkt.Type = TYPE_I;
					}
				} else
#endif
					Pkt.Type = AskTransferTypeAssoc(Pkt.Remote, AskTransferType());
				Pkt.Size = f.Size;
				Pkt.Time = f.Time;
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();

				auto Tmp = Pkt.Local;
				Pkt.Mode = CheckLocalFile(&Pkt);	/* Pkt.ExistSize がセットされる */
				// ミラーリング設定追加
				Pkt.NoTransfer = NO;
				if (Pkt.Mode == EXIST_ABORT)
					break;
				if (Pkt.Mode != EXIST_IGNORE) {
					if (MakeAllDir == YES)
						MakeDirFromLocalPath(Pkt.Local, Tmp);
					AddTransFileList(&Pkt);
				}
			}
		}

		if(AskNoFullPathMode() == YES)
		{
			Pkt.Command = L"BACKCUR"s;
			Pkt.Remote = AskRemoteCurDir();
			AddTransFileList(&Pkt);
		}

		// バグ対策
		AddNullTransFileList();

		GoForwardTransWindow();
//		KeepTransferDialog(NO);

		EnableUserOpe();
	}
	return;
}


// 指定されたファイルを一つダウンロードする
void DirectDownloadProc(std::wstring_view Fname) {
	TRANSPACKET Pkt;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		ExistNotify = YES;
//		KeepTransferDialog(YES);

		if(AskNoFullPathMode() == YES)
		{
			Pkt.Command = L"SETCUR"s;
			Pkt.Remote = AskRemoteCurDir();
			AddTransFileList(&Pkt);
		}

		if (!empty(Fname)) {
			Pkt.Local = AskLocalCurDir();
			if (auto const filename = RenameUnuseableName(RemoveAfterSemicolon(RemoteName(std::wstring{ Fname }))); !empty(filename)) {
				Pkt.Local /= filename;

				Pkt.Remote
					= AskHostType() == HTYPE_ACOS ? std::format(L"'{}({})'"sv, GetConnectingHost().LsName, Fname)
					: AskHostType() == HTYPE_ACOS_4 ? std::wstring{ Fname }
					: ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + Fname, L'\\', L'/');

				Pkt.Command = L"RETR-S "s;
				Pkt.Type = AskTransferTypeAssoc(Pkt.Remote, AskTransferType());

				/* サイズと日付は転送側スレッドで取得し、セットする */

				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();

				auto Tmp = Pkt.Local;
				Pkt.Mode = CheckLocalFile(&Pkt);	/* Pkt.ExistSize がセットされる */
				// ミラーリング設定追加
				Pkt.NoTransfer = NO;
				if((Pkt.Mode != EXIST_ABORT) && (Pkt.Mode != EXIST_IGNORE))
				// ディレクトリ自動作成
//					AddTransFileList(&Pkt);
				{
					if(MakeAllDir == YES)
						MakeDirFromLocalPath(Pkt.Local, Tmp);
					AddTransFileList(&Pkt);
				}
			}
		}

		if(AskNoFullPathMode() == YES)
		{
			Pkt.Command = L"BACKCUR"s;
			Pkt.Remote = AskRemoteCurDir();
			AddTransFileList(&Pkt);
		}

		// バグ対策
		AddNullTransFileList();

		GoForwardTransWindow();
//		KeepTransferDialog(NO);

		EnableUserOpe();
	}
	return;
}


struct MirrorList {
	using result_t = bool;
	Resizable<Controls<MIRROR_DEL, MIRROR_SIZEGRIP>, Controls<IDOK, IDCANCEL, IDHELP, MIRROR_DEL, MIRROR_COPYNUM, MIRROR_MAKENUM, MIRROR_DELNUM, MIRROR_SIZEGRIP, MIRROR_NO_TRANSFER>, Controls<MIRROR_LIST>> resizable{ MirrorDlgSize };
	std::vector<TRANSPACKET>& list;
	MirrorList(std::vector<TRANSPACKET>& list) noexcept : list{ list } {}
	INT_PTR OnInit(HWND hDlg) {
		for (auto const& item : list) {
			std::tuple<int, std::wstring_view> data;
			if (item.Command.starts_with(L"R-DELE"sv) || item.Command.starts_with(L"R-RMD"sv))
				data = { IDS_MSGJPN052, item.Remote };
			else if (item.Command.starts_with(L"R-MKD"sv))
				data = { IDS_MSGJPN053, item.Remote };
			else if (item.Command.starts_with(L"STOR"sv))
				data = { IDS_MSGJPN054, item.Remote };
			else if (item.Command.starts_with(L"L-DELE"sv) || item.Command.starts_with(L"L-RMD"sv))
				data = { IDS_MSGJPN052, item.Local.native() };
			else if (item.Command.starts_with(L"L-MKD"sv))
				data = { IDS_MSGJPN053, item.Local.native() };
			else if (item.Command.starts_with(L"RETR"sv))
				data = { IDS_MSGJPN054, item.Local.native() };
			if (auto [id, str] = data; id != 0)
				SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_ADDSTRING, 0, (LPARAM)std::format(L"{}: {}"sv, GetString(id), str).c_str());
		}
		CountMirrorFiles(hDlg, list);
		EnableWindow(GetDlgItem(hDlg, MIRROR_DEL), FALSE);
		SendDlgItemMessageW(hDlg, MIRROR_NO_TRANSFER, BM_SETCHECK, MirrorNoTransferContents, 0);
		return TRUE;
	}
	void OnCommand(HWND hDlg, WORD cmd, WORD id) {
		switch (id) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, id == IDOK);
			return;
		case MIRROR_DEL: {
			std::vector<int> indexes((size_t)SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_GETSELCOUNT, 0, 0));
			auto const count = (int)SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_GETSELITEMS, size_as<WPARAM>(indexes), (LPARAM)data(indexes));
			assert(size_as<int>(indexes) == count);
			for (int const index : indexes | std::ranges::views::reverse)
				if (index < size_as<int>(list)) {
					list.erase(begin(list) + index);
					SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_DELETESTRING, index, 0);
				} else
					MessageBeep(-1);
			CountMirrorFiles(hDlg, list);
			break;
		}
		case MIRROR_LIST:
			if (cmd == LBN_SELCHANGE)
				EnableWindow(GetDlgItem(hDlg, MIRROR_DEL), 0 < SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_GETSELCOUNT, 0, 0));
			break;
		case MIRROR_NO_TRANSFER:
			for (auto& item : list)
				if (item.Command.starts_with(L"STOR"sv) || item.Command.starts_with(L"RETR"sv))
					item.NoTransfer = (int)SendDlgItemMessageW(hDlg, MIRROR_NO_TRANSFER, BM_GETCHECK, 0, 0);
			break;
		case IDHELP:
			ShowHelp(IDH_HELP_TOPIC_0000012);
			break;
		}
	}
};

/*----- ミラーリングダウンロードを行う ----------------------------------------
*
*	Parameter
*		int Notify : 確認を行うかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MirrorDownloadProc(int Notify)
{
	TRANSPACKET Pkt;
	int Level;
	int Mode;
	// ファイル一覧バグ修正
	int ListSts;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		std::vector<TRANSPACKET> list;

		Notify = Notify == YES ? MirrorNotify(false) : YES;

		if((Notify == YES) || (Notify == YES_LIST))
		{
			/*===== ファイルリスト取得 =====*/

			std::vector<FILELIST> LocalListBase;
			ListSts = MakeSelectedFileList(WIN_LOCAL, YES, YES, LocalListBase, &CancelFlg);
			std::vector<FILELIST> RemoteListBase;
			if(ListSts == FFFTP_SUCCESS)
				ListSts = MakeSelectedFileList(WIN_REMOTE, YES, YES, RemoteListBase, &CancelFlg);

			for (auto& f : RemoteListBase)
				f.Attr = YES;		/* RemotePos->Attrは転送するかどうかのフラグに使用 (YES/NO) */

			for (auto LocalPos = begin(LocalListBase); LocalPos != end(LocalListBase);) {
				if (AskMirrorNoTrn(LocalPos->Name, 1) == NO) {
					LocalPos->Attr = YES;
					++LocalPos;
				} else {
					LocalPos->Attr = NO;	/* LocalPos->Attrは削除するかどうかのフラグに使用 (YES/NO) */

					if (LocalPos->Node == NODE_DIR) {
						Level = LocalPos->DirLevel();
						++LocalPos;
						while (LocalPos != end(LocalListBase)) {
							if (LocalPos->Node == NODE_DIR && LocalPos->DirLevel() <= Level)
								break;
							LocalPos->Attr = NO;
							++LocalPos;
						}
					} else
						++LocalPos;
				}
			}

			/*===== ファイルリスト比較 =====*/

			for (auto RemotePos = begin(RemoteListBase); RemotePos != end(RemoteListBase);) {
				if (AskMirrorNoTrn(RemotePos->Name, 0) == NO) {
					Mode = MirrorFnameCnv == YES ? COMP_LOWERMATCH : COMP_STRICT;
					if (auto LocalPos = SearchFileList(RemotePos->Name, LocalListBase, Mode)) {
						if ((RemotePos->Node == NODE_DIR) && (LocalPos->Node == NODE_DIR)) {
							LocalPos->Attr = NO;
							RemotePos->Attr = NO;
						} else if ((RemotePos->Node == NODE_FILE) && (LocalPos->Node == NODE_FILE)) {
							LocalPos->Attr = NO;
							if (CompareFileTime(&RemotePos->Time, &LocalPos->Time) <= 0)
								RemotePos->Attr = NO;
						}
					}
					++RemotePos;
				} else {
					if (RemotePos->Node == NODE_FILE) {
						RemotePos->Attr = NO;
						++RemotePos;
					} else {
						RemotePos->Attr = NO;
						Level = RemotePos->DirLevel();
						++RemotePos;
						while (RemotePos != end(RemoteListBase)) {
							if (RemotePos->Node == NODE_DIR && RemotePos->DirLevel() <= Level)
								break;
							RemotePos->Attr = NO;
							++RemotePos;
						}
					}
				}
			}

			DispMirrorFiles(LocalListBase, RemoteListBase);

			/*===== 削除／アップロード =====*/

			for (auto const& f : LocalListBase)
				if (f.Attr == YES && f.Node == NODE_FILE) {
					Pkt.Local = AskLocalCurDir() / f.Name;
					Pkt.Remote.clear();
					Pkt.Command = L"L-DELE "s;
					list.push_back(Pkt);
				}
			MirrorDeleteAllLocalDir(LocalListBase, Pkt, list);


			for (auto const& f : RemoteListBase)
				if (f.Attr == YES) {
					Pkt.Local = AskLocalCurDir();
					auto name = f.Name;
					if (MirrorFnameCnv == YES)
						name = lc(std::move(name));
					Pkt.Local /= RemoveAfterSemicolon(std::move(name));

					if (f.Node == NODE_DIR) {
						Pkt.Remote.clear();
						Pkt.Command = L"L-MKD "s;
						list.push_back(Pkt);
					} else if (f.Node == NODE_FILE) {
						Pkt.Remote = ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + f.Name, L'\\', L'/');

						Pkt.Command = L"RETR "s;
						Pkt.Type = AskTransferTypeAssoc(Pkt.Remote, AskTransferType());
						Pkt.Size = f.Size;
						Pkt.Time = f.Time;
						Pkt.KanjiCode = AskHostKanjiCode();
						// UTF-8対応
						Pkt.KanjiCodeDesired = AskLocalKanjiCode();
						Pkt.KanaCnv = AskHostKanaCnv();
						Pkt.Mode = EXIST_OVW;
						// ミラーリング設定追加
						Pkt.NoTransfer = MirrorNoTransferContents;
						list.push_back(Pkt);
					}
				}

			if ((AbortOnListError == NO || ListSts == FFFTP_SUCCESS) && (Notify == YES || Dialog(GetFtpInst(), mirrordown_notify_dlg, GetMainHwnd(), MirrorList{ list })))
			{
				if(AskNoFullPathMode() == YES)
				{
					Pkt.Command = L"SETCUR"s;
					Pkt.Remote = AskRemoteCurDir();
					AddTransFileList(&Pkt);
				}
				AppendTransFileList(std::move(list));

				if(AskNoFullPathMode() == YES)
				{
					Pkt.Command = L"BACKCUR"s;
					Pkt.Remote = AskRemoteCurDir();
					AddTransFileList(&Pkt);
				}
			}

			// バグ対策
			AddNullTransFileList();

			GoForwardTransWindow();
		}

		EnableUserOpe();
	}
	return;
}


// ミラーリングのファイル一覧を表示
static void DispMirrorFiles(std::vector<FILELIST> const& Local, std::vector<FILELIST> const& Remote) {
	if (DebugConsole != YES)
		return;
	FILETIME ft;
	SYSTEMTIME st;
	Debug(L"---- MIRROR FILE LIST ----"sv);
	for (auto& [type, list] : { std::tuple{ L"LOCAL"sv, Local }, std::tuple{ L"REMOTE"sv, Remote } })
		for (auto const& f : list) {
			auto const date = FileTimeToLocalFileTime(&f.Time, &ft) && FileTimeToSystemTime(&ft, &st) ? std::format(L"{:04}/{:02}/{:02} {:02}:{:02}:{:02}.{:03}"sv, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds) : L""s;
			Debug(L"{:6} : {:3} {:4} [{}] {}"sv, type, f.Attr == 1 ? L"YES"sv : L"NO"sv, f.Node == NODE_DIR ? L"DIR"sv : L"FILE"sv, date, f.Name);
		}
	Debug(L"---- END ----"sv);
}


// ミラーリング時のローカル側のフォルダ削除
static void MirrorDeleteAllLocalDir(std::vector<FILELIST> const& Local, TRANSPACKET& item, std::vector<TRANSPACKET>& list) {
	for (auto it = rbegin(Local); it != rend(Local); ++it)
		if (it->Node == NODE_DIR && it->Attr == YES) {
			item.Local = AskLocalCurDir() / it->Name;
			item.Remote.clear();
			item.Command = L"L-RMD "s;
			list.push_back(item);
		}
}


// ファイル名のセミコロン以降を取り除く
static std::wstring RemoveAfterSemicolon(std::wstring&& path) {
	if (VaxSemicolon == YES)
		if (auto const pos = path.find(';'); pos != std::wstring::npos)
			path.resize(pos);
	return path;
}


/*----- ローカルに同じ名前のファイルがないかチェック --------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 処理方法
*			EXIST_OVW/EXIST_RESUME/EXIST_IGNORE
*
*	Note
*		Pkt.ExistSize, ExistMode、ExistNotify が変更される
*----------------------------------------------------------------------------*/

static int CheckLocalFile(TRANSPACKET *Pkt) noexcept {
	struct DownExistDialog {
		using result_t = bool;
		using DownExistButton = RadioButton<DOWN_EXIST_OVW, DOWN_EXIST_NEW, DOWN_EXIST_RESUME, DOWN_EXIST_IGNORE, DOWN_EXIST_LARGE>;
		TRANSPACKET* Pkt;
		DownExistDialog(TRANSPACKET* Pkt) noexcept : Pkt{ Pkt } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, DOWN_EXIST_NAME, EM_LIMITTEXT, FMAX_PATH, 0);
			SetText(hDlg, DOWN_EXIST_NAME, Pkt->Local);
			if (Pkt->Type == TYPE_A || Pkt->ExistSize <= 0)
				EnableWindow(GetDlgItem(hDlg, DOWN_EXIST_RESUME), FALSE);
			DownExistButton::Set(hDlg, ExistMode);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK_ALL:
				ExistNotify = NO;
				[[fallthrough]];
			case IDOK:
				ExistMode = DownExistButton::Get(hDlg);
				Pkt->Local = GetText(hDlg, DOWN_EXIST_NAME);
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000009);
				break;
			}
		}
	};
	int Ret;
	// タイムスタンプのバグ修正
	SYSTEMTIME TmpStime;

	Ret = EXIST_OVW;
	Pkt->ExistSize = 0;
	if(RecvMode != TRANS_OVW)
	{
		if (WIN32_FILE_ATTRIBUTE_DATA attr; GetFileAttributesExW(Pkt->Local.c_str(), GetFileExInfoStandard, &attr))
		{
			Pkt->ExistSize = LONGLONG(attr.nFileSizeHigh) << 32 | attr.nFileSizeLow;

			if(ExistNotify == YES)
			{
				Sound::Error.Play();
				Ret = Dialog(GetFtpInst(), down_exist_dlg, GetMainHwnd(), DownExistDialog{ Pkt }) ? ExistMode : EXIST_ABORT;
			}
			else
				Ret = ExistMode;

			if(Ret == EXIST_NEW)
			{
				/*ファイル日付チェック */
				// タイムスタンプのバグ修正
				if (FileTimeToSystemTime(&attr.ftLastWriteTime, &TmpStime)) {
					if (DispTimeSeconds == NO)
						TmpStime.wSecond = 0;
					TmpStime.wMilliseconds = 0;
					SystemTimeToFileTime(&TmpStime, &attr.ftLastWriteTime);
				} else
					attr.ftLastWriteTime = {};
				if(CompareFileTime(&attr.ftLastWriteTime, &Pkt->Time) < 0)
					Ret = EXIST_OVW;
				else
					Ret = EXIST_IGNORE;
			}
			// 同じ名前のファイルの処理方法追加
			if (Ret == EXIST_LARGE)
				Ret = (LONGLONG(attr.nFileSizeHigh) << 32 | attr.nFileSizeLow) < Pkt->Size ? EXIST_OVW : EXIST_IGNORE;
		}
	}
	return(Ret);
}


/*----- ファイル一覧で指定されたファイルをアップロードする --------------------
*
*	Parameter
*		int ChName : 名前を変えるかどうか (YES/NO)
*		int All : 全てが選ばれている物として扱うかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// ディレクトリ自動作成
// リモート側のパスから必要なディレクトリを作成
int MakeDirFromRemotePath(fs::path const& RemoteFile, fs::path const& Old, int FirstAdd) {
	fs::path path;
	auto const RemotePath = RemoteFile.parent_path();
	auto const OldPath = Old.parent_path();
	auto rit = RemotePath.begin(), rend = RemotePath.end();
	for (auto oit = OldPath.begin(), oend = OldPath.end(); rit != rend && oit != oend && *rit == *oit; ++rit, ++oit)
		path /= *rit;
	if (rit == rend)
		return NO;
	TRANSPACKET Pkt;
	if (FirstAdd == YES && AskNoFullPathMode() == YES) {
		Pkt.Command = L"SETCUR"s;
		Pkt.Remote = AskRemoteCurDir();
		AddTransFileList(&Pkt);
	}
	do {
		auto name = RemoteName(*rit);
		path /= name;
#if defined(HAVE_TANDEM)
		Pkt.FileCode = 0;
		Pkt.PriExt = DEF_PRIEXT;
		Pkt.SecExt = DEF_SECEXT;
		Pkt.MaxExt = DEF_MAXEXT;
#endif
		Pkt.Remote = AskHostType() == HTYPE_ACOS ? std::format(L"'{}({})'"sv, GetConnectingHost().LsName, name) : AskHostType() == HTYPE_ACOS_4 ? name : path.generic_wstring();
		Pkt.Command = L"MKD "s;
		Pkt.Local.clear();
		AddTransFileList(&Pkt);
	} while (++rit != rend);
	return YES;
}

void UploadListProc(int ChName, int All)
{
#if defined(HAVE_TANDEM)
	struct UpDownAsWithExt {
		using result_t = bool;
		std::wstring name;
		int win;
		int filecode;
		UpDownAsWithExt(std::wstring_view name, int win) : name{ name }, win{ win }, filecode{} {}
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, GetString(win == WIN_LOCAL ? IDS_MSGJPN064 : IDS_MSGJPN065));
			SendDlgItemMessageW(hDlg, UPDOWNAS_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SetText(hDlg, UPDOWNAS_NEW, name);
			SetText(hDlg, UPDOWNAS_TEXT, name);
			SendDlgItemMessageW(hDlg, UPDOWNAS_FILECODE, EM_LIMITTEXT, 4, 0);
			SetText(hDlg, UPDOWNAS_FILECODE, L"0");
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				name = GetText(hDlg, UPDOWNAS_NEW);
				filecode = GetDecimalText(hDlg, UPDOWNAS_FILECODE);
				EndDialog(hDlg, true);
				break;
			case UPDOWNAS_STOP:
				EndDialog(hDlg, false);
				break;
			}
		}
	};
#endif
	TRANSPACKET Pkt;
	TRANSPACKET Pkt1;
	int FirstAdd;
	// ファイル一覧バグ修正
	int ListSts;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		// ローカル側で選ばれているファイルをFileListBaseに登録
		std::vector<FILELIST> FileListBase;
		ListSts = MakeSelectedFileList(WIN_LOCAL, YES, All, FileListBase, &CancelFlg);

		// 現在ホスト側のファイル一覧に表示されているものをRemoteListに登録
		// 同名ファイルチェック用
		std::vector<FILELIST> RemoteList;
		AddRemoteTreeToFileList(0, {}, RDIR_NONE, RemoteList);

		FirstAdd = YES;
		ExistNotify = YES;

		for (auto const& f : FileListBase) {
			// ファイル一覧バグ修正
			if((AbortOnListError == YES) && (ListSts == FFFTP_FAIL))
				break;
			Pkt.Remote = SetSlashTail(std::wstring{ AskRemoteCurDir() });
			auto offset = size(Pkt.Remote);
			if (ChName == NO || f.Node == NODE_DIR) {
				Pkt.Remote += RemoteName(std::wstring{ f.Name });
#if defined(HAVE_TANDEM)
				Pkt.FileCode = 0;
				Pkt.PriExt = DEF_PRIEXT;
				Pkt.SecExt = DEF_SECEXT;
				Pkt.MaxExt = DEF_MAXEXT;
#endif
			}
			else
			{
				// 名前を変更する
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					UpDownAsWithExt data{ f.Name, WIN_LOCAL };
					if (!Dialog(GetFtpInst(), updown_as_with_ext_dlg, GetMainHwnd(), data))
						break;
					Pkt.Remote += data.name;
					Pkt.FileCode = data.filecode;
				} else
#endif
				if (std::wstring name = f.Name; UpDownAsDialog(name, WIN_LOCAL))
					Pkt.Remote += name;
				else
					break;
			}
			if (Pkt.Remote.starts_with(SetSlashTail(std::wstring{ AskRemoteCurDir() }))) {
				if (offset = Pkt.Remote.rfind(L'/'); offset != std::wstring::npos)
					offset++;
				else
					offset = 0;
			}
			Pkt.Remote = ReplaceAll(std::move(Pkt.Remote), L'\\', L'/');

			if (AskHostType() == HTYPE_ACOS)
				Pkt.Remote = std::format(L"'{}({})'"sv, GetConnectingHost().LsName, std::wstring_view{ Pkt.Remote }.substr(offset));
			else if (AskHostType() == HTYPE_ACOS_4)
				Pkt.Remote = Pkt.Remote.substr(offset);

			if(f.Node == NODE_DIR)
			{
				// フォルダの場合

				// ホスト側のファイル一覧をRemoteListに登録
				// 同名ファイルチェック用
				RemoteList.clear();

				auto const Tmp = AskRemoteCurDir();
				if(DoCWD(Pkt.Remote, NO, NO, NO) == FTP_COMPLETE)
				{
					if(DoDirList(L""sv, 998, &CancelFlg) == FTP_COMPLETE)
						AddRemoteTreeToFileList(998, {}, RDIR_NONE, RemoteList);
					DoCWD(Tmp, NO, NO, NO);
				}
				else
				{
					// フォルダを作成
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						Pkt1.Command = L"SETCUR"s;
						Pkt1.Remote = AskRemoteCurDir();
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					Pkt.Command = L"MKD "s;
					Pkt.Local.clear();
					AddTransFileList(&Pkt);
				}
			}
			else if(f.Node == NODE_FILE)
			{
				// ファイルの場合
				Pkt.Local = AskLocalCurDir() / f.Name;

				Pkt.Command = L"STOR "s;
				Pkt.Type = AskTransferTypeAssoc(Pkt.Local.native(), AskTransferType());
				Pkt.Size = f.Size;
				Pkt.Time = f.Time;
				Pkt.Attr = AskUploadFileAttr(Pkt.Remote);
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					CalcExtentSize(&Pkt, f.Size);
				}
#endif
				auto dir = Pkt.Remote;
				Pkt.Mode = CheckRemoteFile(&Pkt, RemoteList);
				// ミラーリング設定追加
				Pkt.NoTransfer = NO;
				if(Pkt.Mode == EXIST_ABORT)
					break;
				else if(Pkt.Mode != EXIST_IGNORE)
				{
					// ディレクトリ自動作成
					if(MakeAllDir == YES)
					{
						if(MakeDirFromRemotePath(Pkt.Remote, dir, FirstAdd) == YES)
							FirstAdd = NO;
					}
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						Pkt1.Command = L"SETCUR"s;
						Pkt1.Remote = AskRemoteCurDir();
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					AddTransFileList(&Pkt);
				}
			}
		}

		if((FirstAdd == NO) && (AskNoFullPathMode() == YES))
		{
			Pkt.Command = L"BACKCUR"s;
			Pkt.Remote = AskRemoteCurDir();
			AddTransFileList(&Pkt);
		}

		// バグ対策
		AddNullTransFileList();

		GoForwardTransWindow();

		EnableUserOpe();
	}
	return;
}


/*----- ドラッグ＆ドロップで指定されたファイルをアップロードする --------------
*
*	Parameter
*		WPARAM wParam : ドロップされたファイルの情報
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void UploadDragProc(WPARAM wParam)
{
	TRANSPACKET Pkt;
	TRANSPACKET Pkt1;
	int FirstAdd;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		// ローカル側で選ばれているファイルをFileListBaseに登録
		auto [cur, files] = MakeDroppedFileList(wParam);

		// 現在ホスト側のファイル一覧に表示されているものをRemoteListに登録
		// 同名ファイルチェック用
		std::vector<FILELIST> RemoteList;
		AddRemoteTreeToFileList(0, {}, RDIR_NONE, RemoteList);

		FirstAdd = YES;
		ExistNotify = YES;

		for (auto const& f : files) {
			auto Cat = RemoteName(std::wstring{ f.Name });
			Pkt.Remote
				= AskHostType() == HTYPE_ACOS ? std::format(L"'{}({})'"sv, GetConnectingHost().LsName, Cat)
				: AskHostType() == HTYPE_ACOS_4 ? std::move(Cat)
				: ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + Cat, L'\\', L'/');
#if defined(HAVE_TANDEM)
			Pkt.FileCode = 0;
			Pkt.PriExt = DEF_PRIEXT;
			Pkt.SecExt = DEF_SECEXT;
			Pkt.MaxExt = DEF_MAXEXT;
#endif

			if(f.Node == NODE_DIR)
			{
				// フォルダの場合

				// ホスト側のファイル一覧をRemoteListに登録
				// 同名ファイルチェック用
				RemoteList.clear();

				auto const Tmp = AskRemoteCurDir();
				if(DoCWD(Pkt.Remote, NO, NO, NO) == FTP_COMPLETE)
				{
					if(DoDirList(L""sv, 998, &CancelFlg) == FTP_COMPLETE)
						AddRemoteTreeToFileList(998, {}, RDIR_NONE, RemoteList);
					DoCWD(Tmp, NO, NO, NO);
				}
				else
				{
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						Pkt1.Command = L"SETCUR"s;
						Pkt1.Remote = AskRemoteCurDir();
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					Pkt.Command = L"MKD "s;
					Pkt.Local.clear();
					AddTransFileList(&Pkt);
				}
			}
			else if(f.Node == NODE_FILE)
			{
				// ファイルの場合
				Pkt.Local = cur / f.Name;

				Pkt.Command = L"STOR "s;
				Pkt.Type = AskTransferTypeAssoc(Pkt.Local.native(), AskTransferType());
				Pkt.Size = f.Size;
				Pkt.Time = f.Time;
				Pkt.Attr = AskUploadFileAttr(Pkt.Remote);
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					int const a = f.InfoExist && FINFO_SIZE;
					CalcExtentSize(&Pkt, f.Size);
				}
#endif
				// ディレクトリ自動作成
				auto dir = Pkt.Remote;
				Pkt.Mode = CheckRemoteFile(&Pkt, RemoteList);
				// ミラーリング設定追加
				Pkt.NoTransfer = NO;
				if(Pkt.Mode == EXIST_ABORT)
					break;
				else if(Pkt.Mode != EXIST_IGNORE)
				{
					// ディレクトリ自動作成
					if(MakeAllDir == YES)
					{
						if(MakeDirFromRemotePath(Pkt.Remote, dir, FirstAdd) == YES)
							FirstAdd = NO;
					}
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						Pkt1.Command = L"SETCUR"s;
						Pkt1.Remote = AskRemoteCurDir();
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					AddTransFileList(&Pkt);
				}
			}
		}

		if((FirstAdd == NO) && (AskNoFullPathMode() == YES))
		{
			Pkt.Command = L"BACKCUR"s;
			Pkt.Remote = AskRemoteCurDir();
			AddTransFileList(&Pkt);
		}

		// バグ対策
		AddNullTransFileList();

		GoForwardTransWindow();

		EnableUserOpe();
	}
	return;
}


/*----- ミラーリングアップロードを行う ----------------------------------------
*
*	Parameter
*		int Notify : 確認を行うかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MirrorUploadProc(int Notify)
{
	TRANSPACKET Pkt;
	int Level;
	int Mode;
	SYSTEMTIME TmpStime;
	FILETIME TmpFtimeL;
	FILETIME TmpFtimeR;
	// ファイル一覧バグ修正
	int ListSts;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		std::vector<TRANSPACKET> list;

		Notify = Notify == YES ? MirrorNotify(true) : YES;

		if((Notify == YES) || (Notify == YES_LIST))
		{
			/*===== ファイルリスト取得 =====*/

			std::vector<FILELIST> LocalListBase;
			ListSts = MakeSelectedFileList(WIN_LOCAL, YES, YES, LocalListBase, &CancelFlg);
			std::vector<FILELIST> RemoteListBase;
			if(ListSts == FFFTP_SUCCESS)
				ListSts = MakeSelectedFileList(WIN_REMOTE, YES, YES, RemoteListBase, &CancelFlg);

			for (auto& lf : LocalListBase)
				lf.Attr = YES;		/* LocalPos->Attrは転送するかどうかのフラグに使用 (YES/NO) */

			for (auto RemotePos = begin(RemoteListBase); RemotePos != end(RemoteListBase);) {
				if (AskMirrorNoTrn(RemotePos->Name, 1) == NO) {
					RemotePos->Attr = YES;
					++RemotePos;
				} else {
					RemotePos->Attr = NO;	/* RemotePos->Attrは削除するかどうかのフラグに使用 (YES/NO) */

					if (RemotePos->Node == NODE_DIR) {
						Level = RemotePos->DirLevel();
						++RemotePos;
						while (RemotePos != end(RemoteListBase)) {
							if (RemotePos->Node == NODE_DIR && RemotePos->DirLevel() <= Level)
								break;
							RemotePos->Attr = NO;
							++RemotePos;
						}
					} else
						++RemotePos;
				}
			}

			/*===== ファイルリスト比較 =====*/

			for (auto LocalPos = begin(LocalListBase); LocalPos != end(LocalListBase);) {
				if (AskMirrorNoTrn(LocalPos->Name, 0) == NO) {
					auto const name = ReplaceAll(std::wstring{ LocalPos->Name }, L'\\', L'/');
					Mode = MirrorFnameCnv == YES ? COMP_LOWERMATCH : COMP_STRICT;
					if (LocalPos->Node == NODE_DIR) {
						if (auto RemotePos = SearchFileList(name, RemoteListBase, Mode)) {
							if (RemotePos->Node == NODE_DIR) {
								RemotePos->Attr = NO;
								LocalPos->Attr = NO;
							}
						}
					} else if (LocalPos->Node == NODE_FILE) {
						if (auto RemotePos = SearchFileList(name, RemoteListBase, Mode)) {
							if (RemotePos->Node == NODE_FILE) {
								FileTimeToLocalFileTime(&LocalPos->Time, &TmpFtimeL);
								FileTimeToLocalFileTime(&RemotePos->Time, &TmpFtimeR);
								if ((RemotePos->InfoExist & FINFO_TIME) == 0) {
									if (FileTimeToSystemTime(&TmpFtimeL, &TmpStime)) {
										TmpStime.wHour = 0;
										TmpStime.wMinute = 0;
										TmpStime.wSecond = 0;
										TmpStime.wMilliseconds = 0;
										SystemTimeToFileTime(&TmpStime, &TmpFtimeL);
									}

									if (FileTimeToSystemTime(&TmpFtimeR, &TmpStime)) {
										TmpStime.wHour = 0;
										TmpStime.wMinute = 0;
										TmpStime.wSecond = 0;
										TmpStime.wMilliseconds = 0;
										SystemTimeToFileTime(&TmpStime, &TmpFtimeR);
									}
								}
								RemotePos->Attr = NO;
								if (CompareFileTime(&TmpFtimeL, &TmpFtimeR) <= 0)
									LocalPos->Attr = NO;
							}
						}
					}

					++LocalPos;
				} else {
					if (LocalPos->Node == NODE_FILE) {
						LocalPos->Attr = NO;
						++LocalPos;
					} else {
						LocalPos->Attr = NO;
						Level = LocalPos->DirLevel();
						++LocalPos;
						while (LocalPos != end(LocalListBase)) {
							if (LocalPos->Node == NODE_DIR && LocalPos->DirLevel() <= Level)
								break;
							LocalPos->Attr = NO;
							++LocalPos;
						}
					}
				}
			}

			DispMirrorFiles(LocalListBase, RemoteListBase);

			/*===== 削除／アップロード =====*/

			for (auto const& f : RemoteListBase)
				if (f.Attr == YES && f.Node == NODE_FILE) {
					Pkt.Remote = ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + f.Name, L'\\', L'/');
					Pkt.Local.clear();
					Pkt.Command = L"R-DELE "s;
					list.push_back(Pkt);
				}
			MirrorDeleteAllDir(RemoteListBase, Pkt, list);

			for (auto const& f : LocalListBase) {
				if (f.Attr == YES) {
					auto Cat = MirrorFnameCnv == YES ? lc(std::wstring{ f.Name }) : f.Name;
					Pkt.Remote = ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + Cat, L'\\', L'/');

					if (f.Node == NODE_DIR) {
						Pkt.Local.clear();
						Pkt.Command = L"R-MKD "s;
						list.push_back(Pkt);
					} else if (f.Node == NODE_FILE) {
						Pkt.Local = AskLocalCurDir() / f.Name;

						Pkt.Command = L"STOR "s;
						Pkt.Type = AskTransferTypeAssoc(Pkt.Local.native(), AskTransferType());
						Pkt.Size = f.Size;
						Pkt.Time = f.Time;
						Pkt.Attr = AskUploadFileAttr(Pkt.Remote);
						Pkt.KanjiCode = AskHostKanjiCode();
						// UTF-8対応
						Pkt.KanjiCodeDesired = AskLocalKanjiCode();
						Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
						if (AskHostType() == HTYPE_TANDEM && AskOSS() == NO)
							CalcExtentSize(&Pkt, f.Size);
#endif
						Pkt.Mode = EXIST_OVW;
						// ミラーリング設定追加
						Pkt.NoTransfer = MirrorNoTransferContents;
						list.push_back(Pkt);
					}
				}
			}

			if ((AbortOnListError == NO || ListSts == FFFTP_SUCCESS) && (Notify == YES || Dialog(GetFtpInst(), mirror_notify_dlg, GetMainHwnd(), MirrorList{ list })))
			{
				if(AskNoFullPathMode() == YES)
				{
					Pkt.Command = L"SETCUR"s;
					Pkt.Remote = AskRemoteCurDir();
					AddTransFileList(&Pkt);
				}
				AppendTransFileList(std::move(list));

				if(AskNoFullPathMode() == YES)
				{
					Pkt.Command = L"BACKCUR"s;
					Pkt.Remote = AskRemoteCurDir();
					AddTransFileList(&Pkt);
				}
			}

			// バグ対策
			AddNullTransFileList();

			GoForwardTransWindow();
		}

		EnableUserOpe();
	}
	return;
}


// ミラーリング時のホスト側のフォルダ削除
static void MirrorDeleteAllDir(std::vector<FILELIST> const& Remote, TRANSPACKET& item, std::vector<TRANSPACKET>& list) {
	for (auto it = rbegin(Remote); it != rend(Remote); ++it)
		if (it->Node == NODE_DIR && it->Attr == YES) {
			item.Remote = ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + it->Name, L'\\', L'/');
			item.Local.clear();
			item.Command = L"R-RMD "s;
			list.push_back(item);
		}
}


// ミラーリングアップロード開始確認ウインドウ
static int MirrorNotify(bool upload) noexcept {
	struct Data {
		using result_t = int;
		bool upload;
		Data(bool upload) noexcept : upload{ upload } {}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, YES);
				break;
			case IDCANCEL:
				EndDialog(hDlg, NO);
				break;
			case MIRRORUP_DISP:
				EndDialog(hDlg, YES_LIST);
				break;
			case IDHELP:
				ShowHelp(upload ? IDH_HELP_TOPIC_0000012 : IDH_HELP_TOPIC_0000013);
			}
		}
	};
	return Dialog(GetFtpInst(), upload ? mirror_up_dlg : mirror_down_dlg, GetMainHwnd(), Data{ upload });
}


// ミラーリングで転送／削除するファイルの数を数えダイアログに表示
static void CountMirrorFiles(HWND hDlg, std::vector<TRANSPACKET> const& list) {
	int Del = 0, Make = 0, Copy = 0;
	for (auto const& item : list) {
		if (item.Command.starts_with(L"R-DELE"sv) || item.Command.starts_with(L"R-RMD"sv) || item.Command.starts_with(L"L-DELE"sv) || item.Command.starts_with(L"L-RMD"sv))
			Del++;
		else if (item.Command.starts_with(L"R-MKD"sv) || item.Command.starts_with(L"L-MKD"sv))
			Make++;
		else if (item.Command.starts_with(L"STOR"sv) || item.Command.starts_with(L"RETR"sv))
			Copy++;
	}
	SetText(hDlg, MIRROR_COPYNUM, Copy != 0 ? std::vformat(GetString(IDS_MSGJPN058), std::make_wformat_args(Copy)) : GetString(IDS_MSGJPN059));
	SetText(hDlg, MIRROR_MAKENUM, Make != 0 ? std::vformat(GetString(IDS_MSGJPN060), std::make_wformat_args(Make)) : GetString(IDS_MSGJPN061));
	SetText(hDlg, MIRROR_DELNUM, Del != 0 ? std::vformat(GetString(IDS_MSGJPN062), std::make_wformat_args(Del)) : GetString(IDS_MSGJPN063));
}


// ミラーリングで転送／削除しないファイルかどうかを返す
// Mode : 0=転送しないファイル, 1=削除しないファイル
static int AskMirrorNoTrn(std::wstring_view path, int Mode) {
	if (auto const& patterns = Mode == 1 ? MirrorNoDel : MirrorNoTrn; !empty(patterns))
		for (std::wstring const file{ GetFileName(path) }; auto const& pattern : patterns)
			if (CheckFname(file, pattern))
				return YES;
	return NO;
}


// アップロードするファイルの属性を返す
static int AskUploadFileAttr(std::wstring const& path) {
	std::wstring const file{ GetFileName(path) };
	for (size_t i = 0; i < size(DefAttrList); i += 2)
		if (CheckFname(file, DefAttrList[i]))
			return std::stol(DefAttrList[i + 1], nullptr, 16);
	return -1;
}


/*----- ホストに同じ名前のファイルがないかチェック- ---------------------------a
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		FILELIST *ListList : 
*
*	Return Value
*		int 処理方法
*			EXIST_OVW/EXIST_UNIQUE/EXIST_IGNORE
*
*	Note
*		Pkt.ExistSize, UpExistMode、ExistNotify が変更される
*----------------------------------------------------------------------------*/

static int CheckRemoteFile(TRANSPACKET *Pkt, std::vector<FILELIST> const& ListList) {
	struct UpExistDialog {
		using result_t = bool;
		using UpExistButton = RadioButton<UP_EXIST_OVW, UP_EXIST_NEW, UP_EXIST_RESUME, UP_EXIST_UNIQUE, UP_EXIST_IGNORE, UP_EXIST_LARGE>;
		TRANSPACKET* Pkt;
		UpExistDialog(TRANSPACKET* Pkt) noexcept : Pkt{ Pkt } {}
		INT_PTR OnInit(HWND hDlg) noexcept {
			SendDlgItemMessageW(hDlg, UP_EXIST_NAME, EM_LIMITTEXT, FMAX_PATH, 0);
			SetText(hDlg, UP_EXIST_NAME, Pkt->Remote);
			if (Pkt->Type == TYPE_A || Pkt->ExistSize <= 0)
				EnableWindow(GetDlgItem(hDlg, UP_EXIST_RESUME), FALSE);
			UpExistButton::Set(hDlg, UpExistMode);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK_ALL:
				ExistNotify = NO;
				[[fallthrough]];
			case IDOK:
				UpExistMode = UpExistButton::Get(hDlg);
				Pkt->Remote = GetText(hDlg, UP_EXIST_NAME);
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000011);
				break;
			}
		}
	};
	int Ret;

	Ret = EXIST_OVW;
	Pkt->ExistSize = 0;
	if(SendMode != TRANS_OVW)
	{
		int const Mode =
#if defined(HAVE_TANDEM)
			/* HP NonStop Server は大文字小文字の区別なし(すべて大文字) */
			AskHostType() == HTYPE_TANDEM ? COMP_IGNORE :
#endif
			COMP_STRICT;

		if(auto Exist = SearchFileList(GetFileName(Pkt->Remote), ListList, Mode))
		{
			Pkt->ExistSize = Exist->Size;

			if(ExistNotify == YES)
			{
				Sound::Error.Play();
				Ret = Dialog(GetFtpInst(), up_exist_dlg, GetMainHwnd(), UpExistDialog{ Pkt }) ? UpExistMode : EXIST_ABORT;
			}
			else
				Ret = UpExistMode;

			if(Ret == EXIST_NEW)
			{
				/*ファイル日付チェック */
				if(CompareFileTime(&Exist->Time, &Pkt->Time) < 0)
					Ret = EXIST_OVW;
				else
					Ret = EXIST_IGNORE;
			}
			// 同じ名前のファイルの処理方法追加
			if(Ret == EXIST_LARGE)
			{
				if(Exist->Size < Pkt->Size)
					Ret = EXIST_OVW;
				else
					Ret = EXIST_IGNORE;
			}
		}
	}
	return(Ret);
}


// アップロード／ダウンロードファイル名入力ダイアログ
static bool UpDownAsDialog(std::wstring& name, int win) noexcept {
	struct Data {
		using result_t = bool;
		std::wstring& name;
		int win;
		Data(std::wstring& name, int win) noexcept : name{ name }, win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, GetString(win == WIN_LOCAL ? IDS_MSGJPN064 : IDS_MSGJPN065));
			SendDlgItemMessageW(hDlg, UPDOWNAS_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SetText(hDlg, UPDOWNAS_NEW, name);
			SetText(hDlg, UPDOWNAS_TEXT, name);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				name = GetText(hDlg, UPDOWNAS_NEW);
				EndDialog(hDlg, true);
				break;
			case UPDOWNAS_STOP:
				EndDialog(hDlg, false);
				break;
			}
		}
	};
	return Dialog(GetFtpInst(), updown_as_dlg, GetMainHwnd(), Data{ name, win });
}


/*----- ファイル一覧で指定されたファイルを削除する ----------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteProc(void)
{
	int Win;
	int DelFlg;
	int Sts;

	// 同時接続対応
	CancelFlg = NO;

	Sts = FFFTP_SUCCESS;
	if(GetFocus() == GetLocalHwnd())
		Win = WIN_LOCAL;
	else
	{
		Win = WIN_REMOTE;
		Sts = CheckClosedAndReconnect();
	}

	// デッドロック対策
//	if(Sts == YES)
	if(Sts == FFFTP_SUCCESS)
	{
		// デッドロック対策
		DisableUserOpe();
		auto CurDir = AskRemoteCurDir();
		std::vector<FILELIST> FileListBase;
		if(Win == WIN_LOCAL)
			MakeSelectedFileList(Win, NO, NO, FileListBase, &CancelFlg);
		else
			MakeSelectedFileList(Win, YES, NO, FileListBase, &CancelFlg);

		DelFlg = NO;
		Sts = NO;
		for (auto const& f : FileListBase) {
			if (f.Node == NODE_FILE) {
				DelNotifyAndDo(f, Win, &Sts, &DelFlg, CurDir);
				if (Sts == NO_ALL)
					break;
			}
		}

		if(Sts != NO_ALL)
			DeleteAllDir(FileListBase, Win, &Sts, &DelFlg, CurDir);

		if(Win == WIN_REMOTE)
			if (auto const Tmp = AskRemoteCurDir(); Tmp != CurDir)
				DoCWD(Tmp, NO, NO, NO);

		if(DelFlg == YES)
		{
			if(Win == WIN_LOCAL)
				GetLocalDirForWnd();
			else
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
		}

		// デッドロック対策
		EnableUserOpe();
	}
	return;
}


// サブディレクトリ以下を全て削除する
static void DeleteAllDir(std::vector<FILELIST> const& Dt, int Win, int *Sw, int *Flg, std::wstring& CurDir) {
	for (auto it = rbegin(Dt); it != rend(Dt); ++it)
		if (it->Node == NODE_DIR) {
			DelNotifyAndDo(*it, Win, Sw, Flg, CurDir);
			if (*Sw == NO_ALL)
				break;
		}
}


// 削除するかどうかの確認と削除実行
static void DelNotifyAndDo(FILELIST const& Dt, int Win, int* Sw, int* Flg, std::wstring& CurDir) {
	struct DeleteDialog {
		using result_t = int;
		std::wstring const& path;
		int win;
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, GetString(win == WIN_LOCAL ? IDS_MSGJPN066 : IDS_MSGJPN067));
			SetText(hDlg, DELETE_TEXT, path);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) noexcept {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, YES);
				break;
			case DELETE_NO:
				EndDialog(hDlg, NO);
				break;
			case DELETE_ALL:
				EndDialog(hDlg, YES_ALL);
				break;
			case IDCANCEL:
				EndDialog(hDlg, NO_ALL);
				break;
			}
		}
	};

	auto path = Win == WIN_LOCAL ? (AskLocalCurDir() / Dt.Name).native() : ReplaceAll(SetSlashTail(std::wstring{ AskRemoteCurDir() }) + Dt.Name, L'\\', L'/');
	if (*Sw != YES_ALL) {
		auto path2 = path;
		if (Win == WIN_REMOTE && AskHostType() == HTYPE_VMS)
			path2 = ReformToVMSstylePathName(path2);
		*Sw = Dialog(GetFtpInst(), delete_dlg, GetMainHwnd(), DeleteDialog{ path2, Win });
	}
	if (*Sw == YES || *Sw == YES_ALL) {
		if (Win == WIN_LOCAL) {
			if (Dt.Node == NODE_FILE)
				DoLocalDELE(path);
			else
				DoLocalRMD(path);
			*Flg = YES;
		} else {
			/* フルパスを使わない時のための処理 */
			if (ProcForNonFullpath(AskCmdCtrlSkt(), path, CurDir, GetMainHwnd(), &CancelFlg) == FFFTP_FAIL)
				*Sw = NO_ALL;
			if (*Sw != NO_ALL) {
				if (Dt.Node == NODE_FILE)
					DoDELE(path);
				else
					DoRMD(path);
				*Flg = YES;
			}
		}
	}
}


/* ファイル一覧で指定されたファイルの名前を変更する ----------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RenameProc(void)
{
	struct RenameDialog {
		using result_t = int;
		std::wstring name;
		int win;
		RenameDialog(std::wstring_view name, int win) : name{ name }, win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, GetString(win == WIN_LOCAL ? IDS_MSGJPN068 : IDS_MSGJPN069));
			SendDlgItemMessageW(hDlg, RENAME_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SetText(hDlg, RENAME_NEW, name);
			SetText(hDlg, RENAME_TEXT, name);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				name = GetText(hDlg, RENAME_NEW);
				EndDialog(hDlg, YES);
				break;
			case IDCANCEL:
				EndDialog(hDlg, NO);
				break;
			case RENAME_STOP:
				EndDialog(hDlg, NO_ALL);
				break;
			}
		}
	};
	int Win;
	int RenFlg;
	int Sts;

	// 同時接続対応
	CancelFlg = NO;

	Sts = FFFTP_SUCCESS;
	if(GetFocus() == GetLocalHwnd())
		Win = WIN_LOCAL;
	else
	{
		Win = WIN_REMOTE;
		Sts = CheckClosedAndReconnect();
	}

	if(Sts == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		std::vector<FILELIST> FileListBase;
		MakeSelectedFileList(Win, NO, NO, FileListBase, &CancelFlg);

		RenFlg = NO;
		Sts = NO;
		for (auto const& f : FileListBase)
			if (f.Node == NODE_FILE || f.Node == NODE_DIR) {
				RenameDialog data{ f.Name, Win };
				Sts = Dialog(GetFtpInst(), rename_dlg, GetMainHwnd(), data);
				if (Sts == NO_ALL)
					break;
				if (Sts == YES && !empty(data.name)) {
					if (Win == WIN_LOCAL)
						DoLocalRENAME(f.Name, data.name);
					else
						DoRENAME(f.Name, data.name);
					RenFlg = YES;
				}
			}

		if(RenFlg == YES)
		{
			if(Win == WIN_LOCAL)
				GetLocalDirForWnd();
			else
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
		}

		EnableUserOpe();
	}
	return;
}


//
// リモート側でのファイルの移動（リネーム）を行う
//  
// RenameProc()をベースに改造。(2007.9.5 yutaka)
//
void MoveRemoteFileProc(int drop_index)
{
	struct Data {
		using result_t = bool;
		std::wstring const& file;
		Data(std::wstring const& file) noexcept : file{ file } {}
		INT_PTR OnInit(HWND hDlg) noexcept {
			SetText(hDlg, COMMON_TEXT, file);
			return TRUE;
		}
		static void OnCommand(HWND hDlg, WORD id) noexcept {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			}
		}
		static INT_PTR OnMessage(HWND hDlg, UINT uMsg, WPARAM, LPARAM) noexcept {
			if (uMsg == WM_SHOWWINDOW)
				SendDlgItemMessageW(hDlg, COMMON_TEXT, EM_SETSEL, 0, 0);
			return 0;
		}
	};

	int Win;
	FILELIST Pkt;
	int RenFlg;
	int Sts;

	// 同時接続対応
	CancelFlg = NO;

	if(MoveMode == MOVE_DISABLE)
	{
		return;
	}

	auto const HostDir = AskRemoteCurDir();

	// ドロップ先のフォルダ名を得る
	Pkt.Name = 0 <= drop_index ? GetItem(WIN_REMOTE, drop_index).Name : L".."s;
	if (MoveMode == MOVE_DLG && !Dialog(GetFtpInst(), move_notify_dlg, GetRemoteHwnd(), Data{ Pkt.Name }))
		return;

	Sts = FFFTP_SUCCESS;
#if 0
	if(GetFocus() == GetLocalHwnd())
		Win = WIN_LOCAL;
	else
	{
		Win = WIN_REMOTE;
		Sts = CheckClosedAndReconnect();
	}
#else
		Win = WIN_REMOTE;
		Sts = CheckClosedAndReconnect();
#endif

	if(Sts == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		std::vector<FILELIST> FileListBase;
		MakeSelectedFileList(Win, NO, NO, FileListBase, &CancelFlg);

		RenFlg = NO;
		for (auto const& f : FileListBase)
			if (f.Node == NODE_FILE || f.Node == NODE_DIR) {
				if (!empty(f.Name)) {
					auto const from = std::format(L"{}/{}"sv, HostDir, f.Name);
					auto const to = std::format(L"{}/{}/{}"sv, HostDir, Pkt.Name, f.Name);
					if (Win == WIN_LOCAL)
						DoLocalRENAME(from, to);
					else
						DoRENAME(from, to);
					RenFlg = YES;
				}
			}

		if(RenFlg == YES)
		{
			if(Win == WIN_LOCAL) {
				GetLocalDirForWnd();
			} else {
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
				auto const dir = std::format(L"{}/{}"sv, HostDir, Pkt.Name);
				DoCWD(dir, YES, YES, YES);
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
			}
		}

		EnableUserOpe();
	}
	return;
}


/*----- 新しいディレクトリを作成する ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MkdirProc(void)
{
	CancelFlg = NO;
	auto [Win, titleId] = GetFocus() == GetLocalHwnd() ? std::tuple{ WIN_LOCAL, IDS_MSGJPN070 } : std::tuple{ WIN_REMOTE, IDS_MSGJPN071 };
	if (std::wstring path; InputDialog(mkdir_dlg, GetMainHwnd(), titleId, path, FMAX_PATH + 1) && !empty(path))
	{
		if(Win == WIN_LOCAL)
		{
			DisableUserOpe();
			DoLocalMKD(path);
			GetLocalDirForWnd();
			EnableUserOpe();
		}
		else
		{
			if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
			{
				DisableUserOpe();
				DoMKD(path);
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
				EnableUserOpe();
			}
		}
	}
	return;
}


/*----- ヒストリリストを使ったディレクトリの移動 ------------------------------
*
*	Parameter
*		HWND hWnd : コンボボックスのウインドウハンドル
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
void ChangeDirComboProc(HWND hWnd) {
	CancelFlg = NO;
	if (auto const i = (int)SendMessageW(hWnd, CB_GETCURSEL, 0, 0); i != CB_ERR) {
		auto length = SendMessageW(hWnd, CB_GETLBTEXTLEN, i, 0);
		std::wstring text(length, L'\0');
		length = SendMessageW(hWnd, CB_GETLBTEXT, i, (LPARAM)data(text));
		text.resize(length);
		if (hWnd == GetLocalHistHwnd()) {
			DisableUserOpe();
			DoLocalCWD(text);
			GetLocalDirForWnd();
			EnableUserOpe();
		} else {
			if (CheckClosedAndReconnect() == FFFTP_SUCCESS) {
				DisableUserOpe();
				if(DoCWD(text, YES, NO, YES) < FTP_RETRY)
					GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
				EnableUserOpe();
			}
		}
	}
}


/*----- ブックマークを使ったディレクトリの移動 --------------------------------
*
*	Parameter
*		int MarkID : ブックマークのメニューID
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ChangeDirBmarkProc(int MarkID)
{
	// 同時接続対応
	CancelFlg = NO;

	auto [local, remote] = AskBookMarkText(MarkID);
	if(!empty(local))
	{
		DisableUserOpe();
		if (DoLocalCWD(local))
			GetLocalDirForWnd();
		EnableUserOpe();
	}

	if(!empty(remote))
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			if(DoCWD(remote, YES, NO, YES) < FTP_RETRY)
				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			EnableUserOpe();
		}
	}
	return;
}


// ディレクトリ名を入力してディレクトリの移動
void ChangeDirDirectProc(int Win) {
	CancelFlg = NO;
	if (Win == WIN_LOCAL) {
		if (auto const path = SelectDir(GetMainHwnd()); !path.empty()) {
			DisableUserOpe();
			DoLocalCWD(path);
			GetLocalDirForWnd();
			EnableUserOpe();
		}
	} else {
		if (std::wstring path; InputDialog(chdir_dlg, GetMainHwnd(), IDS_MSGJPN073, path, FMAX_PATH + 1) && !path.empty() && CheckClosedAndReconnect() == FFFTP_SUCCESS) {
			DisableUserOpe();
			if (DoCWD(path, YES, NO, YES) < FTP_RETRY)
				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			EnableUserOpe();
		}
	}
}


// Dropされたファイルによるディレクトリの移動
void ChangeDirDropFileProc(WPARAM wParam) {
	DisableUserOpe();
	DoLocalCWD(MakeDroppedDir(wParam));
	GetLocalDirForWnd();
	EnableUserOpe();
}


// ShellExecute等で使用されるファイル名を修正
// UNCでない場合に末尾の半角スペースは無視されるため拡張子が補完されなくなるまで半角スペースを追加
// 現在UNC対応の予定は無い
fs::path MakeDistinguishableFileName(fs::path&& path) {
	if (path.has_extension())
		return path;
	auto const filename = path.filename().native();
	auto current = path.native();
	WIN32_FIND_DATAW data;
	for (HANDLE handle; (handle = FindFirstFileW((current + L".*"sv).c_str(), &data)) != INVALID_HANDLE_VALUE; current += L' ') {
		bool invalid = false;
		do {
			if (data.cFileName != filename) {
				invalid = true;
				break;
			}
		} while (FindNextFileW(handle, &data));
		FindClose(handle);
		if (!invalid)
			break;
	}
	return current;
}


/*----- ファイルの属性変更 ----------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ChmodProc(void)
{
	int ChmodFlg = NO;

	if(GetFocus() == GetRemoteHwnd())
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			std::vector<FILELIST> FileListBase;
			MakeSelectedFileList(WIN_REMOTE, NO, NO, FileListBase, &CancelFlg);
			if(!empty(FileListBase))
			{
				if(auto newattr = ChmodDialog(std::format(L"{:03X}"sv, FileListBase[0].Attr)))
				{
					ChmodFlg = NO;
					for (auto const& f : FileListBase)
						if (f.Node == NODE_FILE || f.Node == NODE_DIR) {
							DoCHMOD(f.Name, *newattr);
							ChmodFlg = YES;
						}
					if(ChmodFlg == YES)
						GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
				}
			}
			EnableUserOpe();
		}
	}
	else if(GetFocus() == GetLocalHwnd())
	{
		DisableUserOpe();
		std::vector<FILELIST> FileListBase;
		MakeSelectedFileList(WIN_LOCAL, NO, NO, FileListBase, &CancelFlg);
		if (!empty(FileListBase)) {
			// ファイルのプロパティを表示する
			auto path = MakeDistinguishableFileName(FileListBase[0].Name);
			SHELLEXECUTEINFOW info{ sizeof(SHELLEXECUTEINFOW), SEE_MASK_INVOKEIDLIST, 0, L"Properties", path.c_str(), nullptr, nullptr, SW_NORMAL };
			ShellExecuteExW(&info);
		}
		EnableUserOpe();
	}
	return;
}


// 属性変更ダイアログ
std::optional<std::wstring> ChmodDialog(std::wstring const& attr) {
	static constexpr std::tuple<int, int> map[] = {
		{ 0x400, PERM_O_READ },
		{ 0x200, PERM_O_WRITE },
		{ 0x100, PERM_O_EXEC },
		{ 0x040, PERM_G_READ },
		{ 0x020, PERM_G_WRITE },
		{ 0x010, PERM_G_EXEC },
		{ 0x004, PERM_A_READ },
		{ 0x002, PERM_A_WRITE },
		{ 0x001, PERM_A_EXEC },
	};
	struct Data {
		using result_t = bool;
		std::wstring attr;
		Data(std::wstring const& attr) : attr{ attr } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, PERM_NOW, EM_LIMITTEXT, 4, 0);
			SetText(hDlg, PERM_NOW, attr);
			if (!empty(attr) && std::iswdigit(attr[0]))
				for (auto const value = stoi(attr, nullptr, 16); auto [bit, id] : map)
					if (value & bit)
						SendDlgItemMessageW(hDlg, id, BM_SETCHECK, BST_CHECKED, 0);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD cmdId) {
			switch (cmdId) {
			case IDOK:
				attr = GetText(hDlg, PERM_NOW);
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case PERM_O_READ:
			case PERM_O_WRITE:
			case PERM_O_EXEC:
			case PERM_G_READ:
			case PERM_G_WRITE:
			case PERM_G_EXEC:
			case PERM_A_READ:
			case PERM_A_WRITE:
			case PERM_A_EXEC: {
				int value = 0;
				for (auto [bit, id] : map)
					if (SendDlgItemMessageW(hDlg, id, BM_GETCHECK, 0, 0) == BST_CHECKED)
						value |= bit;
				SetText(hDlg, PERM_NOW, std::format(L"{:03X}"sv, value));
				break;
			}
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000017);
				break;
			}
		}
	};
	if (Data data{ attr }; Dialog(GetFtpInst(), chmod_dlg, GetMainHwnd(), data))
		return data.attr;
	return {};
}


/*----- 任意のコマンドを送る --------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SomeCmdProc(void)
{

	// 同時接続対応
	CancelFlg = NO;

	if(GetFocus() == GetRemoteHwnd())
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			std::vector<FILELIST> FileListBase;
			MakeSelectedFileList(WIN_REMOTE, NO, NO, FileListBase, &CancelFlg);
			auto cmd = empty(FileListBase) ? L""s : FileListBase[0].Name;
			if (InputDialog(somecmd_dlg, GetMainHwnd(), 0, cmd, 81, nullptr, IDH_HELP_TOPIC_0000023))
				DoQUOTE(AskCmdCtrlSkt(), cmd, &CancelFlg);
			EnableUserOpe();
		}
	}
	return;
}




// ファイル総容量の計算を行う
void CalcFileSizeProc() {
	struct SizeNotify {
		using result_t = int;
		int win;
		SizeNotify(int win) noexcept : win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, FSNOTIFY_TITLE, GetString(win == WIN_LOCAL ? IDS_MSGJPN074 : IDS_MSGJPN075));
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) noexcept {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, SendDlgItemMessageW(hDlg, FSNOTIFY_SEL_ONLY, BM_GETCHECK, 0, 0) == 1 ? NO : YES);
				break;
			case IDCANCEL:
				EndDialog(hDlg, NO_ALL);
				break;
			}
		}
	};
	struct Size {
		using result_t = int;
		int win;
		uintmax_t size;
		Size(int win, uintmax_t size) noexcept : win{ win }, size{ size } {}
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, FSIZE_TITLE, GetString(win == WIN_LOCAL ? IDS_MSGJPN076 : IDS_MSGJPN077));
			SetText(hDlg, FSIZE_SIZE, MakeSizeString(size));
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) noexcept {
			switch (id) {
			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg, 0);
				break;
			}
		}
	};
	int const Win = GetFocus() == GetLocalHwnd() ? WIN_LOCAL : WIN_REMOTE;
	CancelFlg = NO;
	if (auto const All = Dialog(GetFtpInst(), filesize_notify_dlg, GetMainHwnd(), SizeNotify{ Win }); All != NO_ALL)
		if (Win == WIN_LOCAL || CheckClosedAndReconnect() == FFFTP_SUCCESS) {
			std::vector<FILELIST> ListBase;
			MakeSelectedFileList(Win, YES, All, ListBase, &CancelFlg);
			uintmax_t total = 0;
			for (auto const& f : ListBase)
				if (f.Node != NODE_DIR)
					total += f.Size;
			Dialog(GetFtpInst(), filesize_dlg, GetMainHwnd(), Size{ Win, total });
		}
}


// ディレクトリ移動失敗時のエラーを表示
void DispCWDerror(HWND hWnd) noexcept {
	Dialog(GetFtpInst(), cwderr_dlg, hWnd);
}


// URLをクリップボードにコピー
void CopyURLtoClipBoard() {
	if (GetFocus() != GetRemoteHwnd())
		return;
	std::vector<FILELIST> FileListBase;
	MakeSelectedFileList(WIN_REMOTE, NO, NO, FileListBase, &CancelFlg);
	if (empty(FileListBase))
		return;
	auto const curHost = GetCurHost();
	auto baseAddress = std::vformat(curHost.Port != IPPORT_FTP ? L"ftp://{0}:{1}"sv : L"ftp://{0}"sv, std::make_wformat_args(curHost.HostAdrs, curHost.Port));
	{
		auto dir = SetSlashTail(std::wstring{ AskRemoteCurDir() });
		if (AskHostType() == HTYPE_VMS) {
			baseAddress += L'/';
			dir = ReformToVMSstylePathName(dir);
		}
		baseAddress += dir;
	}
	std::wstring text;
	for (auto const& f : FileListBase)
		text.append(baseAddress).append(f.Name).append(L"\r\n"sv);
	if (OpenClipboard(GetMainHwnd())) {
		if (EmptyClipboard())
			if (auto global = GlobalAlloc(GHND, (size_as<SIZE_T>(text) + 1) * sizeof(wchar_t)); global)
				if (auto buffer = GlobalLock(global); buffer) {
					std::copy(begin(text), end(text), static_cast<wchar_t*>(buffer));
					GlobalUnlock(global);
					SetClipboardData(CF_UNICODETEXT, global);
				}
		CloseClipboard();
	}
}


// 上位ディレクトリのパス名を取得
//   ディレクトリの区切り記号は "\" と "/" の両方が有効
//   最初の "\"や"/"も消す
//   "/pub"   --> ""
//   "C:\DOS" --> "C:"
static std::wstring_view GetUpperDirEraseTopSlash(std::wstring_view path) {
	auto const pos = path.find_first_of(L"/\\"sv);
	return path.substr(0, pos != std::wstring_view::npos ? pos : 0);
}


// 上位ディレクトリのパス名を取得
//    ディレクトリの区切り記号は "\" と "/" の両方が有効
//    最初の "\"や"/"は残す
//    "/pub"   --> "/"
//    "C:\DOS" --> "C:\"
static std::wstring_view GetUpperDir(std::wstring_view path) {
	auto const first = path.find_first_of(L"/\\"sv);
	if (first == std::wstring_view::npos)
		return path;
	auto const last = path.find_last_of(L"/\\"sv);
	return path.substr(0, first == last ? first + 1 : last);
}


// フルパスを使わないファイルアクセスの準備
//   フルパスを使わない時は、このモジュール内で CWD を行ない、Path にファイル名のみ残す。（パス名は消す）
int ProcForNonFullpath(std::shared_ptr<SocketContext> cSkt, std::wstring& Path, std::wstring& CurDir, HWND hWnd, int* CancelCheckWork) {
	int Sts = FFFTP_SUCCESS;
	if (AskNoFullPathMode() == YES) {
		auto Tmp = AskHostType() == HTYPE_VMS ? ReformToVMSstyleDirName(std::wstring{ GetUpperDirEraseTopSlash(Path) }) : AskHostType() == HTYPE_STRATUS ? std::wstring{ GetUpperDirEraseTopSlash(Path) } : std::wstring{ GetUpperDir(Path) };
		if (Tmp != CurDir) {
			if (int const code = std::get<0>(Command(cSkt, CancelCheckWork, L"CWD {}"sv, Tmp)); code / 100 != FTP_COMPLETE) {
				DispCWDerror(hWnd);
				Sts = FFFTP_FAIL;
			} else
				CurDir = std::move(Tmp);
		}
		Path = GetFileName(std::move(Path));
	}
	return Sts;
}


// ディレクトリ名をVAX VMSスタイルに変換する
//   ddd:[xxx.yyy]/rrr/ppp  --> ddd:[xxx.yyy.rrr.ppp]
static std::wstring ReformToVMSstyleDirName(std::wstring&& path) {
	if (auto const btm = path.find(L']'); btm != std::wstring::npos) {
		std::replace(begin(path) + btm, end(path), L'/', L'.');
		path.erase(begin(path) + btm);
		if (path.ends_with(L'.'))
			path.resize(size(path) - 1);
		path += L']';
	}
	return path;
}


// ファイル名をVAX VMSスタイルに変換する
//   ddd:[xxx.yyy]/rrr/ppp  --> ddd:[xxx.yyy.rrr]ppp
static std::wstring ReformToVMSstylePathName(std::wstring_view path) {
	return ReformToVMSstyleDirName(std::wstring{ GetUpperDirEraseTopSlash(path) }) + GetFileName(path);
}


// ファイル名に使えない文字がないかチェックし名前を変更する
static std::wstring RenameUnuseableName(std::wstring&& filename) noexcept {
	for (;;) {
		if (filename.find_first_of(LR"(:*?<>|"\)"sv) == std::wstring::npos)
			return filename;
		if (!InputDialog(forcerename_dlg, GetMainHwnd(), 0, filename, FMAX_PATH + 1))
			return {};
	}
}


// 自動切断対策
// NOOPコマンドでは効果が無いホストが多いためLISTコマンドを使用
void NoopProc(int Force)
{
	if(Force == YES || (AskConnecting() == YES && !AskUserOpeDisabled()))
	{
		if (GetCurHost().ReuseCmdSkt == NO || AskShareProh() == YES && AskTransferNow() == NO) {
			if(Force == NO)
				CancelFlg = NO;
			DisableUserOpe();
			DoDirList(L""sv, 999, &CancelFlg);
			EnableUserOpe();
		}
	}
}

// 同時接続対応
void AbortRecoveryProc(void)
{
	if(AskConnecting() == YES && !AskUserOpeDisabled())
	{
		if (auto const& curHost = GetCurHost(); curHost.ReuseCmdSkt == NO || AskShareProh() == YES || AskTransferNow() == NO) {
			CancelFlg = NO;
			if (curHost.TransferErrorReconnect == YES) {
				DisableUserOpe();
				ReConnectCmdSkt();
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
				EnableUserOpe();
			}
			else {
				if (auto sc = AskCmdCtrlSkt())
					sc->ClearReadBuffer();
			}
		}
	}
	return;
}

void ReconnectProc(void)
{
	if(AskConnecting() == YES && !AskUserOpeDisabled())
	{
		CancelFlg = NO;
		DisableUserOpe();
		ReConnectCmdSkt();
		GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
		EnableUserOpe();
	}
	return;
}

