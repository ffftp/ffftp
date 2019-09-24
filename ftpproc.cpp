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
static void MirrorDeleteAllLocalDir(std::vector<FILELIST> const& Local, TRANSPACKET *Pkt, TRANSPACKET **Base);
static int CheckLocalFile(TRANSPACKET *Pkt);
static void RemoveAfterSemicolon(char *Path);
static void MirrorDeleteAllDir(std::vector<FILELIST> const& Remote, TRANSPACKET *Pkt, TRANSPACKET **Base);
static int MirrorNotify(bool upload);
static void CountMirrorFiles(HWND hDlg, TRANSPACKET *Pkt);
static int AskMirrorNoTrn(char *Fname, int Mode);
static int AskUploadFileAttr(char *Fname);
static bool UpDownAsDialog(int win);
static void DeleteAllDir(std::vector<FILELIST> const& Dt, int Win, int *Sw, int *Flg, char *CurDir);
static void DelNotifyAndDo(FILELIST const& Dt, int Win, int *Sw, int *Flg, char *CurDir);
static void SetAttrToDialog(HWND hWnd, int Attr);
static int GetAttrFromDialog(HWND hDlg);
static int RenameUnuseableName(char *Fname);

/* 設定値 */
extern int FnameCnv;
extern int RecvMode;
extern int SendMode;
extern int MoveMode;
extern char MirrorNoTrn[MIRROR_LEN+1];
extern char MirrorNoDel[MIRROR_LEN+1];
extern int MirrorFnameCnv;
extern char DefAttrList[DEFATTRLIST_LEN+1];
extern SIZE MirrorDlgSize;
extern int VaxSemicolon;
extern int DebugConsole;
extern int CancelFlg;
// ディレクトリ自動作成
extern int MakeAllDir;
// ファイル一覧バグ修正
extern int AbortOnListError;
// ミラーリング設定追加
extern int MirrorNoTransferContents;
// タイムスタンプのバグ修正
extern int DispTimeSeconds;

/*===== ローカルなワーク =====*/

static char TmpString[FMAX_PATH+80];		/* テンポラリ */

int UpExistMode = EXIST_OVW;		/* アップロードで同じ名前のファイルがある時の扱い方 EXIST_xxx */
int ExistMode = EXIST_OVW;		/* 同じ名前のファイルがある時の扱い方 EXIST_xxx */
static int ExistNotify;		/* 確認ダイアログを出すかどうか YES/NO */


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

// ディレクトリ自動作成
// ローカル側のパスから必要なフォルダを作成
int MakeDirFromLocalPath(char* LocalFile, char* Old)
{
	TRANSPACKET Pkt;
	char* pDelimiter;
	char* pNext;
	char* Cat;
	int Len;
	int Make;
	pDelimiter = LocalFile;
	Make = NO;
	while(pNext = strchr(pDelimiter, '\\'))
	{
		Len = (int)(pNext - LocalFile);
		strncpy(Pkt.LocalFile, LocalFile, Len);
		Pkt.LocalFile[Len] = '\0';
		if(strncmp(LocalFile, Old, Len + 1) != 0)
		{
			Cat = Pkt.LocalFile + (pDelimiter - LocalFile);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr((unsigned char*)Cat);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr((unsigned char*)Cat);
			ReplaceAll(Pkt.LocalFile, '/', '\\');

			strcpy(Pkt.Cmd, "MKD ");
			strcpy(Pkt.RemoteFile, "");
			AddTransFileList(&Pkt);

			Make = YES;
		}
		pDelimiter = pNext + 1;
	}
	return Make;
}

void DownloadProc(int ChName, int ForceFile, int All)
{
	TRANSPACKET Pkt;
	// ディレクトリ自動作成
	char Tmp[FMAX_PATH+1];
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
			strcpy(Pkt.Cmd, "SETCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}

		for (auto const& f : FileListBase) {
			// ファイル一覧バグ修正
			if (AbortOnListError == YES && ListSts == FFFTP_FAIL)
				break;
			AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
			SetYenTail(Pkt.LocalFile);
			strcpy(TmpString, f.File);
			if (ChName == NO || ForceFile == NO && f.Node == NODE_DIR) {
				if (FnameCnv == FNAME_LOWER)
					_mbslwr((unsigned char*)TmpString);
				else if (FnameCnv == FNAME_UPPER)
					_mbsupr((unsigned char*)TmpString);
				RemoveAfterSemicolon(TmpString);
				if (RenameUnuseableName(TmpString) == FFFTP_FAIL)
					break;
			} else {
				if (!UpDownAsDialog(WIN_REMOTE) || RenameUnuseableName(TmpString) == FFFTP_FAIL)
					break;
			}
			strcat(Pkt.LocalFile, TmpString);
			ReplaceAll(Pkt.LocalFile, '/', '\\');

			if (ForceFile == NO && f.Node == NODE_DIR) {
				strcpy(Pkt.Cmd, "MKD ");
				strcpy(Pkt.RemoteFile, "");
				AddTransFileList(&Pkt);
			} else if (f.Node == NODE_FILE || ForceFile == YES && f.Node == NODE_DIR) {
				if (AskHostType() == HTYPE_ACOS) {
					strcpy(Pkt.RemoteFile, "'");
					strcat(Pkt.RemoteFile, AskHostLsName().c_str());
					strcat(Pkt.RemoteFile, "(");
					strcat(Pkt.RemoteFile, f.File);
					strcat(Pkt.RemoteFile, ")");
					strcat(Pkt.RemoteFile, "'");
				} else if (AskHostType() == HTYPE_ACOS_4) {
					strcpy(Pkt.RemoteFile, f.File);
				} else {
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					strcat(Pkt.RemoteFile, f.File);
					ReplaceAll(Pkt.RemoteFile, '\\', '/');
				}

				strcpy(Pkt.Cmd, "RETR ");
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
					Pkt.Type = AskTransferTypeAssoc(Pkt.RemoteFile, AskTransferType());
				Pkt.Size = f.Size;
				Pkt.Time = f.Time;
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();

				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.LocalFile);
				Pkt.Mode = CheckLocalFile(&Pkt);	/* Pkt.ExistSize がセットされる */
				// ミラーリング設定追加
				Pkt.NoTransfer = NO;
				if (Pkt.Mode == EXIST_ABORT)
					break;
				if (Pkt.Mode != EXIST_IGNORE) {
					if (MakeAllDir == YES)
						MakeDirFromLocalPath(Pkt.LocalFile, Tmp);
					AddTransFileList(&Pkt);
				}
			}
		}

		if(AskNoFullPathMode() == YES)
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
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


/*----- 指定されたファイルを一つダウンロードする ------------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DirectDownloadProc(char *Fname)
{
	TRANSPACKET Pkt;
	// ディレクトリ自動作成
	char Tmp[FMAX_PATH+1];

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		ExistNotify = YES;
//		KeepTransferDialog(YES);

		if(AskNoFullPathMode() == YES)
		{
			strcpy(Pkt.Cmd, "SETCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}

		if(strlen(Fname) > 0)
		{
			AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
			SetYenTail(Pkt.LocalFile);
			strcpy(TmpString, Fname);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr((unsigned char*)TmpString);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr((unsigned char*)TmpString);
			RemoveAfterSemicolon(TmpString);

			if(RenameUnuseableName(TmpString) == FFFTP_SUCCESS)
			{
				strcat(Pkt.LocalFile, TmpString);
				ReplaceAll(Pkt.LocalFile, '/', '\\');

				if(AskHostType() == HTYPE_ACOS)
				{
					strcpy(Pkt.RemoteFile, "'");
					strcat(Pkt.RemoteFile, AskHostLsName().c_str());
					strcat(Pkt.RemoteFile, "(");
					strcat(Pkt.RemoteFile, Fname);
					strcat(Pkt.RemoteFile, ")");
					strcat(Pkt.RemoteFile, "'");
				}
				else if(AskHostType() == HTYPE_ACOS_4)
				{
					strcpy(Pkt.RemoteFile, Fname);
				}
				else
				{
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					strcat(Pkt.RemoteFile, Fname);
					ReplaceAll(Pkt.RemoteFile, '\\', '/');
				}

				strcpy(Pkt.Cmd, "RETR-S ");
				Pkt.Type = AskTransferTypeAssoc(Pkt.RemoteFile, AskTransferType());

				/* サイズと日付は転送側スレッドで取得し、セットする */

				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();

				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.LocalFile);
				Pkt.Mode = CheckLocalFile(&Pkt);	/* Pkt.ExistSize がセットされる */
				// ミラーリング設定追加
				Pkt.NoTransfer = NO;
				if((Pkt.Mode != EXIST_ABORT) && (Pkt.Mode != EXIST_IGNORE))
				// ディレクトリ自動作成
//					AddTransFileList(&Pkt);
				{
					if(MakeAllDir == YES)
						MakeDirFromLocalPath(Pkt.LocalFile, Tmp);
					AddTransFileList(&Pkt);
				}
			}
		}

		if(AskNoFullPathMode() == YES)
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}

		// 同時接続対応
//		strcpy(Pkt.Cmd, "GOQUIT");
//		AddTransFileList(&Pkt);

		// バグ対策
		AddNullTransFileList();

		GoForwardTransWindow();
//		KeepTransferDialog(NO);

		EnableUserOpe();
	}
	return;
}


// 入力されたファイル名のファイルを一つダウンロードする
void InputDownloadProc() {
	if (char Path[FMAX_PATH + 1] = ""; InputDialog(downname_dlg, GetMainHwnd(), NULL, Path, FMAX_PATH))
		DirectDownloadProc(Path);
}

struct MirrorList {
	using result_t = bool;
	Resizable<Controls<MIRROR_DEL, MIRROR_SIZEGRIP>, Controls<IDOK, IDCANCEL, IDHELP, MIRROR_DEL, MIRROR_COPYNUM, MIRROR_MAKENUM, MIRROR_DELNUM, MIRROR_SIZEGRIP, MIRROR_NO_TRANSFER>, Controls<MIRROR_LIST>> resizable{ MirrorDlgSize };
	TRANSPACKET** Base;
	MirrorList(TRANSPACKET** Base) : Base{ Base } {}
	INT_PTR OnInit(HWND hDlg) {
		for (auto Pos = *Base; Pos; Pos = Pos->Next) {
			char Tmp[FMAX_PATH + 1 + 6] = "";
			if (strncmp(Pos->Cmd, "R-DELE", 6) == 0 || strncmp(Pos->Cmd, "R-RMD", 5) == 0)
				sprintf(Tmp, MSGJPN052, Pos->RemoteFile);
			else if (strncmp(Pos->Cmd, "R-MKD", 5) == 0)
				sprintf(Tmp, MSGJPN053, Pos->RemoteFile);
			else if (strncmp(Pos->Cmd, "STOR", 4) == 0)
				sprintf(Tmp, MSGJPN054, Pos->RemoteFile);
			else if (strncmp(Pos->Cmd, "L-DELE", 6) == 0 || strncmp(Pos->Cmd, "L-RMD", 5) == 0)
				sprintf(Tmp, MSGJPN055, Pos->LocalFile);
			else if (strncmp(Pos->Cmd, "L-MKD", 5) == 0)
				sprintf(Tmp, MSGJPN056, Pos->LocalFile);
			else if (strncmp(Pos->Cmd, "RETR", 4) == 0)
				sprintf(Tmp, MSGJPN057, Pos->LocalFile);
			if (strlen(Tmp) > 0)
				SendDlgItemMessage(hDlg, MIRROR_LIST, LB_ADDSTRING, 0, (LPARAM)Tmp);
		}
		CountMirrorFiles(hDlg, *Base);
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
			std::vector<int> List((size_t)SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_GETSELCOUNT, 0, 0));
			auto Num = (int)SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_GETSELITEMS, size_as<WPARAM>(List), (LPARAM)data(List));
			for (Num--; Num >= 0; Num--)
				if (RemoveTmpTransFileListItem(Base, List[Num]) == FFFTP_SUCCESS)
					SendDlgItemMessage(hDlg, MIRROR_LIST, LB_DELETESTRING, List[Num], 0);
				else
					MessageBeep(-1);
			CountMirrorFiles(hDlg, *Base);
			break;
		}
		case MIRROR_LIST:
			if (cmd == LBN_SELCHANGE)
				EnableWindow(GetDlgItem(hDlg, MIRROR_DEL), 0 < SendDlgItemMessageW(hDlg, MIRROR_LIST, LB_GETSELCOUNT, 0, 0));
			break;
		case MIRROR_NO_TRANSFER:
			for (auto Pos = *Base; Pos; Pos = Pos->Next)
				if (strncmp(Pos->Cmd, "STOR", 4) == 0 || strncmp(Pos->Cmd, "RETR", 4) == 0)
					Pos->NoTransfer = (int)SendDlgItemMessageW(hDlg, MIRROR_NO_TRANSFER, BM_GETCHECK, 0, 0);
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
	TRANSPACKET *Base;
	char Name[FMAX_PATH+1];
	char *Cat;
	int Level;
	int Mode;
	// ファイル一覧バグ修正
	int ListSts;

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		Base = NULL;

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
				if (AskMirrorNoTrn(LocalPos->File, 1) == NO) {
					LocalPos->Attr = YES;
					++LocalPos;
				} else {
					LocalPos->Attr = NO;	/* LocalPos->Attrは削除するかどうかのフラグに使用 (YES/NO) */

					if (LocalPos->Node == NODE_DIR) {
						Level = AskDirLevel(LocalPos->File);
						++LocalPos;
						while (LocalPos != end(LocalListBase)) {
							if (LocalPos->Node == NODE_DIR && AskDirLevel(LocalPos->File) <= Level)
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
				if (AskMirrorNoTrn(RemotePos->File, 0) == NO) {
					strcpy(Name, RemotePos->File);

					if (MirrorFnameCnv == YES)
						Mode = COMP_LOWERMATCH;
					else
						Mode = COMP_STRICT;

					if (auto LocalPos = SearchFileList(Name, LocalListBase, Mode)) {
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
						Level = AskDirLevel(RemotePos->File);
						++RemotePos;
						while (RemotePos != end(RemoteListBase)) {
							if (RemotePos->Node == NODE_DIR && AskDirLevel(RemotePos->File) <= Level)
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
					AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
					SetYenTail(Pkt.LocalFile);
					strcat(Pkt.LocalFile, f.File);
					ReplaceAll(Pkt.LocalFile, '/', '\\');
					strcpy(Pkt.RemoteFile, "");
					strcpy(Pkt.Cmd, "L-DELE ");
					AddTmpTransFileList(&Pkt, &Base);
				}
			MirrorDeleteAllLocalDir(LocalListBase, &Pkt, &Base);


			for (auto const& f : RemoteListBase)
				if (f.Attr == YES) {
					AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
					SetYenTail(Pkt.LocalFile);
					Cat = strchr(Pkt.LocalFile, NUL);
					strcat(Pkt.LocalFile, f.File);

					if (MirrorFnameCnv == YES)
						_mbslwr((unsigned char*)Cat);

					RemoveAfterSemicolon(Cat);
					ReplaceAll(Pkt.LocalFile, '/', '\\');

					if (f.Node == NODE_DIR) {
						strcpy(Pkt.RemoteFile, "");
						strcpy(Pkt.Cmd, "L-MKD ");
						AddTmpTransFileList(&Pkt, &Base);
					} else if (f.Node == NODE_FILE) {
						AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
						SetSlashTail(Pkt.RemoteFile);
						strcat(Pkt.RemoteFile, f.File);
						ReplaceAll(Pkt.RemoteFile, '\\', '/');

						strcpy(Pkt.Cmd, "RETR ");
						Pkt.Type = AskTransferTypeAssoc(Pkt.RemoteFile, AskTransferType());
						Pkt.Size = f.Size;
						Pkt.Time = f.Time;
						Pkt.KanjiCode = AskHostKanjiCode();
						// UTF-8対応
						Pkt.KanjiCodeDesired = AskLocalKanjiCode();
						Pkt.KanaCnv = AskHostKanaCnv();
						Pkt.Mode = EXIST_OVW;
						// ミラーリング設定追加
						Pkt.NoTransfer = MirrorNoTransferContents;
						AddTmpTransFileList(&Pkt, &Base);
					}
				}

			if ((AbortOnListError == NO || ListSts == FFFTP_SUCCESS) && (Notify == YES || Dialog(GetFtpInst(), mirrordown_notify_dlg, GetMainHwnd(), MirrorList{ &Base })))
			{
				if(AskNoFullPathMode() == YES)
				{
					strcpy(Pkt.Cmd, "SETCUR");
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					AddTransFileList(&Pkt);
				}
				AppendTransFileList(Base);

				if(AskNoFullPathMode() == YES)
				{
					strcpy(Pkt.Cmd, "BACKCUR");
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					AddTransFileList(&Pkt);
				}

				// 同時接続対応
//				strcpy(Pkt.Cmd, "GOQUIT");
//				AddTransFileList(&Pkt);
			}
			else
				EraseTmpTransFileList(&Base);

			// バグ対策
			AddNullTransFileList();

			GoForwardTransWindow();
		}

		EnableUserOpe();
	}
	return;
}


/*----- ミラーリングのファイル一覧を表示 --------------------------------------
*
*	Parameter
*		FILELIST *Local : ローカル側
*		FILELIST *Remote : リモート側
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispMirrorFiles(std::vector<FILELIST> const& Local, std::vector<FILELIST> const& Remote)
{
	char Date[80];
	SYSTEMTIME sTime;
	FILETIME fTime;

	if(DebugConsole == YES)
	{
		DoPrintf("---- MIRROR FILE LIST ----");
		for (auto const& f : Local) {
			FileTimeToLocalFileTime(&f.Time, &fTime);
			if (FileTimeToSystemTime(&fTime, &sTime))
				sprintf(Date, "%04d/%02d/%02d %02d:%02d:%02d.%04d", sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);
			else
				strcpy(Date, "");
			DoPrintf("LOCAL  : %s %s [%s] %s", f.Attr == 1 ? "YES" : "NO ", f.Node == NODE_DIR ? "DIR " : "FILE", Date, f.File);
		}
		for (auto const& f : Remote) {
			FileTimeToLocalFileTime(&f.Time, &fTime);
			if (FileTimeToSystemTime(&fTime, &sTime))
				sprintf(Date, "%04d/%02d/%02d %02d:%02d:%02d.%04d", sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);
			else
				strcpy(Date, "");
			DoPrintf("REMOTE : %s %s [%s] %s", f.Attr == 1 ? "YES" : "NO ", f.Node == NODE_DIR ? "DIR " : "FILE", Date, f.File);
		}
		DoPrintf("---- END ----");
	}
	return;
}


/*----- ミラーリング時のローカル側のフォルダ削除 ------------------------------
*
*	Parameter
*		FILELIST *Local : ファイルリスト
*		TRANSPACKET *Pkt : 
*		TRANSPACKET **Base : 
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void MirrorDeleteAllLocalDir(std::vector<FILELIST> const& Local, TRANSPACKET *Pkt, TRANSPACKET **Base) {
	for (auto it = rbegin(Local); it != rend(Local); ++it)
		if (it->Node == NODE_DIR && it->Attr == YES) {
			AskLocalCurDir(Pkt->LocalFile, FMAX_PATH);
			SetYenTail(Pkt->LocalFile);
			strcat(Pkt->LocalFile, it->File);
			ReplaceAll(Pkt->LocalFile, '/', '\\');
			strcpy(Pkt->RemoteFile, "");
			strcpy(Pkt->Cmd, "L-RMD ");
			AddTmpTransFileList(Pkt, Base);
		}
}


/*----- ファイル名のセミコロン以降を取り除く ----------------------------------
*
*	Parameter
*		char *Path : ファイル名
*
*	Return Value
*		なし
*
*	Note
*		Pathの内容が書き換えられる
*		オプション設定によって処理を切替える
*----------------------------------------------------------------------------*/

static void RemoveAfterSemicolon(char *Path)
{
	char *Pos;

	if(VaxSemicolon == YES)
	{
		if((Pos = strchr(Path, ';')) != NULL)
			*Pos = NUL;
	}
	return;
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

static int CheckLocalFile(TRANSPACKET *Pkt)
{
	struct DownExistDialog {
		using result_t = bool;
		using DownExistButton = RadioButton<DOWN_EXIST_OVW, DOWN_EXIST_NEW, DOWN_EXIST_RESUME, DOWN_EXIST_IGNORE, DOWN_EXIST_LARGE>;
		TRANSPACKET* Pkt;
		DownExistDialog(TRANSPACKET* Pkt) : Pkt{ Pkt } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, DOWN_EXIST_NAME, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, DOWN_EXIST_NAME, WM_SETTEXT, 0, (LPARAM)Pkt->LocalFile);
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
				SendDlgItemMessage(hDlg, DOWN_EXIST_NAME, WM_GETTEXT, FMAX_PATH, (LPARAM)Pkt->LocalFile);
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
		if (WIN32_FILE_ATTRIBUTE_DATA attr; GetFileAttributesExW(fs::u8path(Pkt->LocalFile).c_str(), GetFileExInfoStandard, &attr))
		{
			Pkt->ExistSize = LONGLONG(attr.nFileSizeHigh) << 32 | attr.nFileSizeLow;

			if(ExistNotify == YES)
			{
				SoundPlay(SND_ERROR);
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
int MakeDirFromRemotePath(char* RemoteFile, char* Old, int FirstAdd)
{
	TRANSPACKET Pkt;
	TRANSPACKET Pkt1;
	char* pDelimiter;
	char* pNext;
	char* Cat;
	int Len;
	int Make;
	pDelimiter = RemoteFile;
	Make = NO;
	while(pNext = strchr(pDelimiter, '/'))
	{
		Len = (int)(pNext - RemoteFile);
		strncpy(Pkt.RemoteFile, RemoteFile, Len);
		Pkt.RemoteFile[Len] = '\0';
		if(strncmp(RemoteFile, Old, Len + 1) != 0)
		{
			Cat = Pkt.RemoteFile + (pDelimiter - RemoteFile);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr((unsigned char*)Cat);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr((unsigned char*)Cat);
#if defined(HAVE_TANDEM)
			Pkt.FileCode = 0;
			Pkt.PriExt = DEF_PRIEXT;
			Pkt.SecExt = DEF_SECEXT;
			Pkt.MaxExt = DEF_MAXEXT;
#endif
			ReplaceAll(Pkt.RemoteFile, '\\', '/');

			if(AskHostType() == HTYPE_ACOS)
			{
				strcpy(Pkt.RemoteFile, "'");
				strcat(Pkt.RemoteFile, AskHostLsName().c_str());
				strcat(Pkt.RemoteFile, "(");
				strcat(Pkt.RemoteFile, Cat);
				strcat(Pkt.RemoteFile, ")");
				strcat(Pkt.RemoteFile, "'");
			}
			else if(AskHostType() == HTYPE_ACOS_4)
				strcpy(Pkt.RemoteFile, Cat);

			if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
			{
				strcpy(Pkt1.Cmd, "SETCUR");
				AskRemoteCurDir(Pkt1.RemoteFile, FMAX_PATH);
				AddTransFileList(&Pkt1);
			}
			FirstAdd = NO;
			strcpy(Pkt.Cmd, "MKD ");
			strcpy(Pkt.LocalFile, "");
			AddTransFileList(&Pkt);

			Make = YES;
		}
		pDelimiter = pNext + 1;
	}
	return Make;
}

void UploadListProc(int ChName, int All)
{
#if defined(HAVE_TANDEM)
	struct UpDownAsWithExt {
		using result_t = bool;
		int win;
		int filecode;
		UpDownAsWithExt(int win) : win{ win }, filecode{} {}
		INT_PTR OnInit(HWND hDlg) {
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)(win == WIN_LOCAL ? MSGJPN064 : MSGJPN065));
			SendDlgItemMessageW(hDlg, UPDOWNAS_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, UPDOWNAS_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessageW(hDlg, UPDOWNAS_FILECODE, EM_LIMITTEXT, 4, 0);
			SendDlgItemMessageW(hDlg, UPDOWNAS_FILECODE, WM_SETTEXT, 0, (LPARAM)L"0");
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpString);
				filecode = std::stoi(GetText(hDlg, UPDOWNAS_FILECODE));
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
	char *Cat;
	char Tmp[FMAX_PATH+1];
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
		AddRemoteTreeToFileList(0, "", RDIR_NONE, RemoteList);

		FirstAdd = YES;
		ExistNotify = YES;

		for (auto const& f : FileListBase) {
			// ファイル一覧バグ修正
			if((AbortOnListError == YES) && (ListSts == FFFTP_FAIL))
				break;
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			SetSlashTail(Pkt.RemoteFile);
			Cat = strchr(Pkt.RemoteFile, NUL);
			if (ChName == NO || f.Node == NODE_DIR) {
				strcat(Pkt.RemoteFile, f.File);
				if(FnameCnv == FNAME_LOWER)
					_mbslwr((unsigned char*)Cat);
				else if(FnameCnv == FNAME_UPPER)
					_mbsupr((unsigned char*)Cat);
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
				strcpy(TmpString, f.File);
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					UpDownAsWithExt data{ WIN_LOCAL };
					if (!Dialog(GetFtpInst(), updown_as_with_ext_dlg, GetMainHwnd(), data))
						break;
					strcat(Pkt.RemoteFile, TmpString);
					Pkt.FileCode = data.filecode;
				} else
#endif
				if (UpDownAsDialog(WIN_LOCAL))
					strcat(Pkt.RemoteFile, TmpString);
				else
					break;
			}
			// バグ修正
			AskRemoteCurDir(Tmp, FMAX_PATH);
			SetSlashTail(Tmp);
			if(strncmp(Pkt.RemoteFile, Tmp, strlen(Tmp)) != 0)
			{
				if((Cat = strrchr(Pkt.RemoteFile, '/')) != NULL)
					Cat++;
				else
					Cat = Pkt.RemoteFile;
			}
			ReplaceAll(Pkt.RemoteFile, '\\', '/');

			if(AskHostType() == HTYPE_ACOS)
			{
				strcpy(Pkt.RemoteFile, "'");
				strcat(Pkt.RemoteFile, AskHostLsName().c_str());
				strcat(Pkt.RemoteFile, "(");
				strcat(Pkt.RemoteFile, Cat);
				strcat(Pkt.RemoteFile, ")");
				strcat(Pkt.RemoteFile, "'");
			}
			else if(AskHostType() == HTYPE_ACOS_4)
				strcpy(Pkt.RemoteFile, Cat);

			if(f.Node == NODE_DIR)
			{
				// フォルダの場合

				// ホスト側のファイル一覧をRemoteListに登録
				// 同名ファイルチェック用
				RemoteList.clear();

				AskRemoteCurDir(Tmp, FMAX_PATH);
				if(DoCWD(Pkt.RemoteFile, NO, NO, NO) == FTP_COMPLETE)
				{
					if(DoDirListCmdSkt("", "", 998, &CancelFlg) == FTP_COMPLETE)
						AddRemoteTreeToFileList(998, "", RDIR_NONE, RemoteList);
					DoCWD(Tmp, NO, NO, NO);
				}
				else
				{
					// フォルダを作成
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						strcpy(Pkt1.Cmd, "SETCUR");
						AskRemoteCurDir(Pkt1.RemoteFile, FMAX_PATH);
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					strcpy(Pkt.Cmd, "MKD ");
					strcpy(Pkt.LocalFile, "");
					AddTransFileList(&Pkt);
				}
			}
			else if(f.Node == NODE_FILE)
			{
				// ファイルの場合
				AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
				SetYenTail(Pkt.LocalFile);
				strcat(Pkt.LocalFile, f.File);
				ReplaceAll(Pkt.LocalFile, '/', '\\');

				strcpy(Pkt.Cmd, "STOR ");
				Pkt.Type = AskTransferTypeAssoc(Pkt.LocalFile, AskTransferType());
				Pkt.Size = f.Size;
				Pkt.Time = f.Time;
				Pkt.Attr = AskUploadFileAttr(Pkt.RemoteFile);
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					CalcExtentSize(&Pkt, f.Size);
				}
#endif
				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.RemoteFile);
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
						if(MakeDirFromRemotePath(Pkt.RemoteFile, Tmp, FirstAdd) == YES)
							FirstAdd = NO;
					}
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						strcpy(Pkt1.Cmd, "SETCUR");
						AskRemoteCurDir(Pkt1.RemoteFile, FMAX_PATH);
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					AddTransFileList(&Pkt);
				}
			}
		}

		if((FirstAdd == NO) && (AskNoFullPathMode() == YES))
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
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
	char *Cat;
	char Tmp[FMAX_PATH+1];
	int FirstAdd;
	char Cur[FMAX_PATH+1];

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		// ローカル側で選ばれているファイルをFileListBaseに登録
		std::vector<FILELIST> FileListBase;
		MakeDroppedFileList(wParam, Cur, FileListBase);

		// 現在ホスト側のファイル一覧に表示されているものをRemoteListに登録
		// 同名ファイルチェック用
		std::vector<FILELIST> RemoteList;
		AddRemoteTreeToFileList(0, "", RDIR_NONE, RemoteList);

		FirstAdd = YES;
		ExistNotify = YES;

		for (auto const& f : FileListBase) {
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			SetSlashTail(Pkt.RemoteFile);
			Cat = strchr(Pkt.RemoteFile, NUL);

			strcat(Pkt.RemoteFile, f.File);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr((unsigned char*)Cat);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr((unsigned char*)Cat);
			ReplaceAll(Pkt.RemoteFile, '\\', '/');
#if defined(HAVE_TANDEM)
			Pkt.FileCode = 0;
			Pkt.PriExt = DEF_PRIEXT;
			Pkt.SecExt = DEF_SECEXT;
			Pkt.MaxExt = DEF_MAXEXT;
#endif

			if(AskHostType() == HTYPE_ACOS)
			{
				strcpy(Pkt.RemoteFile, "'");
				strcat(Pkt.RemoteFile, AskHostLsName().c_str());
				strcat(Pkt.RemoteFile, "(");
				strcat(Pkt.RemoteFile, Cat);
				strcat(Pkt.RemoteFile, ")");
				strcat(Pkt.RemoteFile, "'");
			}
			else if(AskHostType() == HTYPE_ACOS_4)
				strcpy(Pkt.RemoteFile, Cat);

			if(f.Node == NODE_DIR)
			{
				// フォルダの場合

				// ホスト側のファイル一覧をRemoteListに登録
				// 同名ファイルチェック用
				RemoteList.clear();

				AskRemoteCurDir(Tmp, FMAX_PATH);
				if(DoCWD(Pkt.RemoteFile, NO, NO, NO) == FTP_COMPLETE)
				{
					if(DoDirListCmdSkt("", "", 998, &CancelFlg) == FTP_COMPLETE)
						AddRemoteTreeToFileList(998, "", RDIR_NONE, RemoteList);
					DoCWD(Tmp, NO, NO, NO);
				}
				else
				{
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						strcpy(Pkt1.Cmd, "SETCUR");
						AskRemoteCurDir(Pkt1.RemoteFile, FMAX_PATH);
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					strcpy(Pkt.Cmd, "MKD ");
					strcpy(Pkt.LocalFile, "");
					AddTransFileList(&Pkt);
				}
			}
			else if(f.Node == NODE_FILE)
			{
				// ファイルの場合
				strcpy(Pkt.LocalFile, Cur);
				SetYenTail(Pkt.LocalFile);
				strcat(Pkt.LocalFile, f.File);
				ReplaceAll(Pkt.LocalFile, '/', '\\');

				strcpy(Pkt.Cmd, "STOR ");
				Pkt.Type = AskTransferTypeAssoc(Pkt.LocalFile, AskTransferType());
				Pkt.Size = f.Size;
				Pkt.Time = f.Time;
				Pkt.Attr = AskUploadFileAttr(Pkt.RemoteFile);
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					int a = f.InfoExist && FINFO_SIZE;
					CalcExtentSize(&Pkt, f.Size);
				}
#endif
				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.RemoteFile);
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
						if(MakeDirFromRemotePath(Pkt.RemoteFile, Tmp, FirstAdd) == YES)
							FirstAdd = NO;
					}
					if((FirstAdd == YES) && (AskNoFullPathMode() == YES))
					{
						strcpy(Pkt1.Cmd, "SETCUR");
						AskRemoteCurDir(Pkt1.RemoteFile, FMAX_PATH);
						AddTransFileList(&Pkt1);
					}
					FirstAdd = NO;
					AddTransFileList(&Pkt);
				}
			}
		}

		if((FirstAdd == NO) && (AskNoFullPathMode() == YES))
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
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
	TRANSPACKET *Base;
	char Name[FMAX_PATH+1];
	char *Cat;
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

		Base = NULL;

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
				if (AskMirrorNoTrn(RemotePos->File, 1) == NO) {
					RemotePos->Attr = YES;
					++RemotePos;
				} else {
					RemotePos->Attr = NO;	/* RemotePos->Attrは削除するかどうかのフラグに使用 (YES/NO) */

					if (RemotePos->Node == NODE_DIR) {
						Level = AskDirLevel(RemotePos->File);
						++RemotePos;
						while (RemotePos != end(RemoteListBase)) {
							if (RemotePos->Node == NODE_DIR && AskDirLevel(RemotePos->File) <= Level)
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
				if (AskMirrorNoTrn(LocalPos->File, 0) == NO) {
					strcpy(Name, LocalPos->File);
					ReplaceAll(Name, '\\', '/');

					if (MirrorFnameCnv == YES)
						Mode = COMP_LOWERMATCH;
					else
						Mode = COMP_STRICT;

					if (LocalPos->Node == NODE_DIR) {
						if (auto RemotePos = SearchFileList(Name, RemoteListBase, Mode)) {
							if (RemotePos->Node == NODE_DIR) {
								RemotePos->Attr = NO;
								LocalPos->Attr = NO;
							}
						}
					} else if (LocalPos->Node == NODE_FILE) {
						if (auto RemotePos = SearchFileList(Name, RemoteListBase, Mode)) {
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
						Level = AskDirLevel(LocalPos->File);
						++LocalPos;
						while (LocalPos != end(LocalListBase)) {
							if (LocalPos->Node == NODE_DIR && AskDirLevel(LocalPos->File) <= Level)
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
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					strcat(Pkt.RemoteFile, f.File);
					ReplaceAll(Pkt.RemoteFile, '\\', '/');
					strcpy(Pkt.LocalFile, "");
					strcpy(Pkt.Cmd, "R-DELE ");
					AddTmpTransFileList(&Pkt, &Base);
				}
			MirrorDeleteAllDir(RemoteListBase, &Pkt, &Base);

			for (auto const& f : LocalListBase) {
				if (f.Attr == YES) {
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					Cat = strchr(Pkt.RemoteFile, NUL);
					strcat(Pkt.RemoteFile, f.File);

					if (MirrorFnameCnv == YES)
						_mbslwr((unsigned char*)Cat);

					ReplaceAll(Pkt.RemoteFile, '\\', '/');

					if (f.Node == NODE_DIR) {
						strcpy(Pkt.LocalFile, "");
						strcpy(Pkt.Cmd, "R-MKD ");
						AddTmpTransFileList(&Pkt, &Base);
					} else if (f.Node == NODE_FILE) {
						AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
						SetYenTail(Pkt.LocalFile);
						strcat(Pkt.LocalFile, f.File);
						ReplaceAll(Pkt.LocalFile, '/', '\\');

						strcpy(Pkt.Cmd, "STOR ");
						Pkt.Type = AskTransferTypeAssoc(Pkt.LocalFile, AskTransferType());
						Pkt.Size = f.Size;
						Pkt.Time = f.Time;
						Pkt.Attr = AskUploadFileAttr(Pkt.RemoteFile);
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
						AddTmpTransFileList(&Pkt, &Base);
					}
				}
			}

			if ((AbortOnListError == NO || ListSts == FFFTP_SUCCESS) && (Notify == YES || Dialog(GetFtpInst(), mirror_notify_dlg, GetMainHwnd(), MirrorList{ &Base })))
			{
				if(AskNoFullPathMode() == YES)
				{
					strcpy(Pkt.Cmd, "SETCUR");
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					AddTransFileList(&Pkt);
				}
				AppendTransFileList(Base);

				if(AskNoFullPathMode() == YES)
				{
					strcpy(Pkt.Cmd, "BACKCUR");
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					AddTransFileList(&Pkt);
				}

				// 同時接続対応
//				strcpy(Pkt.Cmd, "GOQUIT");
//				AddTransFileList(&Pkt);
			}
			else
				EraseTmpTransFileList(&Base);

			// バグ対策
			AddNullTransFileList();

			GoForwardTransWindow();
		}

		EnableUserOpe();
	}
	return;
}


/*----- ミラーリング時のホスト側のフォルダ削除 --------------------------------
*
*	Parameter
*		FILELIST *Base : ファイルリスト
*		TRANSPACKET *Pkt : 
*		TRANSPACKET **Base : 
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void MirrorDeleteAllDir(std::vector<FILELIST> const& Remote, TRANSPACKET *Pkt, TRANSPACKET **Base) {
	for (auto it = rbegin(Remote); it != rend(Remote); ++it)
		if (it->Node == NODE_DIR && it->Attr == YES) {
			AskRemoteCurDir(Pkt->RemoteFile, FMAX_PATH);
			SetSlashTail(Pkt->RemoteFile);
			strcat(Pkt->RemoteFile, it->File);
			ReplaceAll(Pkt->RemoteFile, '\\', '/');
			strcpy(Pkt->LocalFile, "");
			strcpy(Pkt->Cmd, "R-RMD ");
			AddTmpTransFileList(Pkt, Base);
		}
}


// ミラーリングアップロード開始確認ウインドウ
static int MirrorNotify(bool upload) {
	struct Data {
		using result_t = int;
		bool upload;
		Data(bool upload) : upload{ upload } {}
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


/*----- ミラーリングで転送／削除するファイルの数を数えダイアログに表示---------
*
*	Parameter
*		HWND hWnd : 
*		TRANSPACKET *Pkt : 
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void CountMirrorFiles(HWND hDlg, TRANSPACKET *Pkt)
{
	char Tmp[80];
	int Del;
	int Make;
	int Copy;

	Del = 0;
	Make = 0;
	Copy = 0;
	while(Pkt != NULL)
	{
		if((strncmp(Pkt->Cmd, "R-DELE", 6) == 0) ||
		   (strncmp(Pkt->Cmd, "R-RMD", 5) == 0) ||
		   (strncmp(Pkt->Cmd, "L-DELE", 6) == 0) ||
		   (strncmp(Pkt->Cmd, "L-RMD", 5) == 0))
		{
			Del += 1;
		}
		else if((strncmp(Pkt->Cmd, "R-MKD", 5) == 0) ||
				(strncmp(Pkt->Cmd, "L-MKD", 5) == 0))
		{
			Make += 1;
		}
		else if((strncmp(Pkt->Cmd, "STOR", 4) == 0) ||
				(strncmp(Pkt->Cmd, "RETR", 4) == 0))
		{
			Copy += 1;
		}
		Pkt = Pkt->Next;
	}

	if(Copy != 0)
		sprintf(Tmp, MSGJPN058, Copy);
	else
		sprintf(Tmp, MSGJPN059);
	SendDlgItemMessage(hDlg, MIRROR_COPYNUM, WM_SETTEXT, 0, (LPARAM)Tmp);

	if(Make != 0)
		sprintf(Tmp, MSGJPN060, Make);
	else
		sprintf(Tmp, MSGJPN061);
	SendDlgItemMessage(hDlg, MIRROR_MAKENUM, WM_SETTEXT, 0, (LPARAM)Tmp);

	if(Del != 0)
		sprintf(Tmp, MSGJPN062, Del);
	else
		sprintf(Tmp, MSGJPN063);
	SendDlgItemMessage(hDlg, MIRROR_DELNUM, WM_SETTEXT, 0, (LPARAM)Tmp);

	return;
}


// ミラーリングで転送／削除しないファイルかどうかを返す
// Mode : 0=転送しないファイル, 1=削除しないファイル
static int AskMirrorNoTrn(char *Fname, int Mode) {
	if (auto Tbl = Mode == 1 ? MirrorNoDel : MirrorNoTrn; 0 < StrMultiLen(Tbl))
		for (auto wFname = u8(GetFileName(Fname)); *Tbl != NUL; Tbl += strlen(Tbl) + 1)
			if (CheckFname(wFname, u8(Tbl)))
				return YES;
	return NO;
}


// アップロードするファイルの属性を返す
static int AskUploadFileAttr(char *Fname) {
	auto wFname = u8(GetFileName(Fname));
	for (char* Tbl = DefAttrList; *Tbl != NUL; Tbl += strlen(Tbl) + 1) {
		auto match = CheckFname(wFname, u8(Tbl));
		Tbl += strlen(Tbl) + 1;
		if (match && *Tbl != NUL)
			return std::strtol(Tbl, nullptr, 16);
	}
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
		UpExistDialog(TRANSPACKET* Pkt) : Pkt{ Pkt } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, UP_EXIST_NAME, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, UP_EXIST_NAME, WM_SETTEXT, 0, (LPARAM)Pkt->RemoteFile);
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
				SendDlgItemMessage(hDlg, UP_EXIST_NAME, WM_GETTEXT, FMAX_PATH, (LPARAM)Pkt->RemoteFile);
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
		int Mode =
#if defined(HAVE_TANDEM)
			/* HP NonStop Server は大文字小文字の区別なし(すべて大文字) */
			AskHostType() == HTYPE_TANDEM ? COMP_IGNORE :
#endif
			COMP_STRICT;

		if(auto Exist = SearchFileList(GetFileName(Pkt->RemoteFile), ListList, Mode))
		{
			Pkt->ExistSize = Exist->Size;

			if(ExistNotify == YES)
			{
				SoundPlay(SND_ERROR);
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
static bool UpDownAsDialog(int win) {
	struct Data {
		using result_t = bool;
		int win;
		Data(int win) : win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)(win == WIN_LOCAL ? MSGJPN064 : MSGJPN065));
			SendDlgItemMessageW(hDlg, UPDOWNAS_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, UPDOWNAS_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpString);
				EndDialog(hDlg, true);
				break;
			case UPDOWNAS_STOP:
				EndDialog(hDlg, false);
				break;
			}
		}
	};
	return Dialog(GetFtpInst(), updown_as_dlg, GetMainHwnd(), Data{ win });
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
	char CurDir[FMAX_PATH+1];
	char Tmp[FMAX_PATH+1];

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
		AskRemoteCurDir(CurDir, FMAX_PATH);
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
		{
			AskRemoteCurDir(Tmp, FMAX_PATH);
			if(strcmp(Tmp, CurDir) != 0)
				DoCWD(Tmp, NO, NO, NO);
		}

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


/*----- サブディレクトリ以下を全て削除する ------------------------------------
*
*	Parameter
*		FILELIST *Dt : 削除するファイルのリスト
*		int Win : ウインドウ番号 (WIN_xxx)
*		int *Sw : 操作方法 (YES/NO/YES_ALL/NO_ALL)
*		int *Flg : ファイルを削除したかどうかのフラグ (YES/NO)
*		char *CurDir : カレントディレクトリ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DeleteAllDir(std::vector<FILELIST> const& Dt, int Win, int *Sw, int *Flg, char *CurDir) {
	for (auto it = rbegin(Dt); it != rend(Dt); ++it)
		if (it->Node == NODE_DIR) {
			DelNotifyAndDo(*it, Win, Sw, Flg, CurDir);
			if (*Sw == NO_ALL)
				break;
		}
}


/*----- 削除するかどうかの確認と削除実行 --------------------------------------
*
*	Parameter
*		FILELIST *Dt : 削除するファイルのリスト
*		int Win : ウインドウ番号 (WIN_xxx)
*		int *Sw : 操作方法 (YES/NO/YES_ALL/NO_ALL)
*		int *Flg : ファイルを削除したかどうかのフラグ (YES/NO)
*		char *CurDir : カレントディレクトリ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DelNotifyAndDo(FILELIST const& Dt, int Win, int *Sw, int *Flg, char *CurDir) {
	struct DeleteDialog {
		using result_t = int;
		int win;
		DeleteDialog(int win) : win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)(win == WIN_LOCAL ? MSGJPN066 : MSGJPN067));
			SendDlgItemMessage(hDlg, DELETE_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
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
	char Path[FMAX_PATH+1];

	if(Win == WIN_LOCAL)
	{
		AskLocalCurDir(Path, FMAX_PATH);
		SetYenTail(Path);
		strcat(Path, Dt.File);
		ReplaceAll(Path, '/', '\\');
	}
	else
	{
		AskRemoteCurDir(Path, FMAX_PATH);
		SetSlashTail(Path);
		strcat(Path, Dt.File);
		ReplaceAll(Path, '\\', '/');
	}

	if(*Sw != YES_ALL)
	{
		sprintf(TmpString, "%s", Path);

		// ローカルのファイルのパスの最後の'\\'が消えるバグ修正
//		if(AskHostType() == HTYPE_VMS)
		if(Win == WIN_REMOTE && AskHostType() == HTYPE_VMS)
			ReformToVMSstylePathName(TmpString);
		*Sw = Dialog(GetFtpInst(), delete_dlg, GetMainHwnd(), DeleteDialog{ Win });
	}

	if((*Sw == YES) || (*Sw == YES_ALL))
	{
		if(Win == WIN_LOCAL)
		{
			if(Dt.Node == NODE_FILE)
				DoLocalDELE(Path);
			else
				DoLocalRMD(Path);
			*Flg = YES;
		}
		else
		{
			/* フルパスを使わない時のための処理 */
			// 同時接続対応
//			if(ProcForNonFullpath(Path, CurDir, GetMainHwnd(), 0) == FFFTP_FAIL)
			if(ProcForNonFullpath(AskCmdCtrlSkt(), Path, CurDir, GetMainHwnd(), &CancelFlg) == FFFTP_FAIL)
				*Sw = NO_ALL;

			if(*Sw != NO_ALL)
			{
				if(Dt.Node == NODE_FILE)
					DoDELE(Path);
				else
					DoRMD(Path);
				*Flg = YES;
			}
		}
	}
	return;
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
		int win;
		RenameDialog(int win) : win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)(win == WIN_LOCAL ? MSGJPN068 : MSGJPN069));
			SendDlgItemMessageW(hDlg, RENAME_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, RENAME_NEW, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, RENAME_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				SendDlgItemMessage(hDlg, RENAME_NEW, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpString);
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
	char New[FMAX_PATH+1];
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
				strcpy(TmpString, f.File);
				Sts = Dialog(GetFtpInst(), rename_dlg, GetMainHwnd(), RenameDialog{ Win });

				if (Sts == NO_ALL)
					break;

				if ((Sts == YES) && (strlen(TmpString) != 0)) {
					strcpy(New, TmpString);
					if (Win == WIN_LOCAL)
						DoLocalRENAME(f.File, New);
					else
						DoRENAME(f.File, New);
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
		Data(std::wstring const& file) : file{ file } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, COMMON_TEXT, WM_SETTEXT, 0, (LPARAM)file.c_str());
			return TRUE;
		}
		static void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			}
		}
		static INT_PTR OnMessage(HWND hDlg, UINT uMsg, WPARAM, LPARAM) {
			if (uMsg == WM_SHOWWINDOW)
				SendDlgItemMessageW(hDlg, COMMON_TEXT, EM_SETSEL, 0, 0);
			return 0;
		}
	};

	int Win;
	FILELIST Pkt;
	char New[FMAX_PATH+1];
	char Old[FMAX_PATH+1];
	char HostDir[FMAX_PATH+1];
	int RenFlg;
	int Sts;

	// 同時接続対応
	CancelFlg = NO;

	if(MoveMode == MOVE_DISABLE)
	{
		return;
	}

	AskRemoteCurDir(HostDir, FMAX_PATH);

	// ドロップ先のフォルダ名を得る
	// 上位のディレクトリへ移動対応
//	GetNodeName(WIN_REMOTE, drop_index, Pkt.File, FMAX_PATH);
	if(drop_index >= 0)
		GetNodeName(WIN_REMOTE, drop_index, Pkt.File, FMAX_PATH);
	else
		strcpy(Pkt.File, "..");

	if (MoveMode == MOVE_DLG && !Dialog(GetFtpInst(), move_notify_dlg, GetRemoteHwnd(), Data{ u8(Pkt.File) }))
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
		Sts = NO;
		for (auto const& f : FileListBase)
			if (f.Node == NODE_FILE || f.Node == NODE_DIR) {
				strcpy(TmpString, f.File);
				Sts = YES;
				if (strlen(TmpString) != 0) {
					// パスの設定(local)
					strncpy_s(Old, sizeof(Old), HostDir, _TRUNCATE);
					strncat_s(Old, sizeof(Old), "/", _TRUNCATE);
					strncat_s(Old, sizeof(Old), f.File, _TRUNCATE);

					// パスの設定(remote)
					strncpy_s(New, sizeof(New), HostDir, _TRUNCATE);
					strncat_s(New, sizeof(New), "/", _TRUNCATE);
					strncat_s(New, sizeof(New), Pkt.File, _TRUNCATE);
					strncat_s(New, sizeof(New), "/", _TRUNCATE);
					strncat_s(New, sizeof(New), f.File, _TRUNCATE);

					if (Win == WIN_LOCAL)
						DoLocalRENAME(Old, New);
					else
						DoRENAME(Old, New);
					RenFlg = YES;
				}
			}

		if(RenFlg == YES)
		{
			if(Win == WIN_LOCAL) {
				GetLocalDirForWnd();
			} else {
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);

				strncpy_s(New, sizeof(New), HostDir, _TRUNCATE);
				strncat_s(New, sizeof(New), "/", _TRUNCATE);
				strncat_s(New, sizeof(New), Pkt.File, _TRUNCATE);
				DoCWD(New, YES, YES, YES);
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
	int Win;
	char *Title;

	// 同時接続対応
	CancelFlg = NO;

	if(GetFocus() == GetLocalHwnd())
	{
		Win = WIN_LOCAL;
		Title = MSGJPN070;
	}
	else
	{
		Win = WIN_REMOTE;
		Title = MSGJPN071;
	}

	if (char Path[FMAX_PATH + 1] = ""; InputDialog(mkdir_dlg, GetMainHwnd(), Title, Path, FMAX_PATH + 1) && strlen(Path) != 0)
	{
		if(Win == WIN_LOCAL)
		{
			DisableUserOpe();
			DoLocalMKD(Path);
			GetLocalDirForWnd();
			EnableUserOpe();
		}
		else
		{
			if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
			{
				DisableUserOpe();
				DoMKD(Path);
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

void ChangeDirComboProc(HWND hWnd)
{
	char Tmp[FMAX_PATH+1];
	int i;

	// 同時接続対応
	CancelFlg = NO;

	if((i = (int)SendMessage(hWnd, CB_GETCURSEL, 0, 0)) != CB_ERR)
	{
		SendMessage(hWnd, CB_GETLBTEXT, i, (LPARAM)Tmp);

		if(hWnd == GetLocalHistHwnd())
		{
			DisableUserOpe();
			DoLocalCWD(Tmp);
			GetLocalDirForWnd();
			EnableUserOpe();
		}
		else
		{
			if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
			{
				DisableUserOpe();
				if(DoCWD(Tmp, YES, NO, YES) < FTP_RETRY)
					GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
				EnableUserOpe();
			}
		}
	}
	return;
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
		if(DoLocalCWD(u8(local).data()) == FFFTP_SUCCESS)
			GetLocalDirForWnd();
		EnableUserOpe();
	}

	if(!empty(remote))
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			if(DoCWD(u8(remote).data(), YES, NO, YES) < FTP_RETRY)
				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			EnableUserOpe();
		}
	}
	return;
}


/*----- ディレクトリ名を入力してディレクトリの移動 ----------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ChangeDirDirectProc(int Win)
{
	bool result = false;
	char Path[FMAX_PATH+1];
	char *Title;

	// 同時接続対応
	CancelFlg = NO;

	if(Win == WIN_LOCAL)
		Title = MSGJPN072;
	else
		Title = MSGJPN073;

	strcpy(Path, "");
	if(Win == WIN_LOCAL)
	{
		if(SelectDir(GetMainHwnd(), Path, FMAX_PATH) == TRUE)
			result = true;
	}
	else
		result = InputDialog(chdir_dlg, GetMainHwnd(), Title, Path, FMAX_PATH+1);

	if(result && strlen(Path) != 0)
	{
		if(Win == WIN_LOCAL)
		{
			DisableUserOpe();
			DoLocalCWD(Path);
			GetLocalDirForWnd();
			EnableUserOpe();
		}
		else
		{
			if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
			{
				DisableUserOpe();
				if(DoCWD(Path, YES, NO, YES) < FTP_RETRY)
					GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
				EnableUserOpe();
			}
		}
	}
	return;
}


/*----- Dropされたファイルによるディレクトリの移動 ----------------------------
*
*	Parameter
*		WPARAM wParam : ドロップされたファイルの情報
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ChangeDirDropFileProc(WPARAM wParam)
{
	char Path[FMAX_PATH+1];

	DisableUserOpe();
	MakeDroppedDir(wParam, Path);
	DoLocalCWD(Path);
	GetLocalDirForWnd();
	EnableUserOpe();
	return;
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
	int ChmodFlg;
	char *Buf;
	char *BufTmp;
	int BufLen;

	// 同時接続対応
	CancelFlg = NO;

	if(GetFocus() == GetRemoteHwnd())
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			std::vector<FILELIST> FileListBase;
			MakeSelectedFileList(WIN_REMOTE, NO, NO, FileListBase, &CancelFlg);
			if(!empty(FileListBase))
			{
				wchar_t attr[5];
				swprintf(attr, std::size(attr), L"%03X", FileListBase[0].Attr);
				if(auto newattr = ChmodDialog(attr))
				{
					ChmodFlg = NO;
					for (auto const& f : FileListBase)
						if (f.Node == NODE_FILE || f.Node == NODE_DIR) {
							DoCHMOD(f.File, u8(*newattr).c_str());
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
		if(!empty(FileListBase))
		{
			if((Buf = (char*)malloc(1)) != NULL)
			{
				*Buf = NUL;
				BufLen = 0;
				for (auto const& f : FileListBase)
					if ((BufTmp = (char*)realloc(Buf, BufLen + strlen(f.File) + 2)) != NULL) {
						Buf = BufTmp;
						strcpy(Buf + BufLen, f.File);
						BufLen += (int)strlen(f.File) + 1;
					}

				memset(Buf+BufLen, NUL, 1);
				DispFileProperty(Buf);
				free(Buf);
			}
		}
		EnableUserOpe();
	}
	return;
}


// 属性変更ダイアログ
std::optional<std::wstring> ChmodDialog(std::wstring const& attr) {
	struct Data {
		using result_t = bool;
		std::wstring attr;
		Data(std::wstring const& attr) : attr{ attr } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, PERM_NOW, EM_LIMITTEXT, 4, 0);
			SendDlgItemMessageW(hDlg, PERM_NOW, WM_SETTEXT, 0, (LPARAM)attr.c_str());
			SetAttrToDialog(hDlg, std::stoi(attr, nullptr, 16));
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
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
				auto Tmp = GetAttrFromDialog(hDlg);
				wchar_t buffer[5];
				swprintf(buffer, std::size(buffer), L"%03X", Tmp);
				SendDlgItemMessageW(hDlg, PERM_NOW, WM_SETTEXT, 0, (LPARAM)buffer);
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


/*----- 属性をダイアログボックスに設定 ----------------------------------------
*
*	Parameter
*		HWND hWnd : ダイアログボックスのウインドウハンドル
*		int Attr : 属性
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void SetAttrToDialog(HWND hDlg, int Attr)
{
	if(Attr & 0x400)
		SendDlgItemMessage(hDlg, PERM_O_READ, BM_SETCHECK, 1, 0);
	if(Attr & 0x200)
		SendDlgItemMessage(hDlg, PERM_O_WRITE, BM_SETCHECK, 1, 0);
	if(Attr & 0x100)
		SendDlgItemMessage(hDlg, PERM_O_EXEC, BM_SETCHECK, 1, 0);

	if(Attr & 0x40)
		SendDlgItemMessage(hDlg, PERM_G_READ, BM_SETCHECK, 1, 0);
	if(Attr & 0x20)
		SendDlgItemMessage(hDlg, PERM_G_WRITE, BM_SETCHECK, 1, 0);
	if(Attr & 0x10)
		SendDlgItemMessage(hDlg, PERM_G_EXEC, BM_SETCHECK, 1, 0);

	if(Attr & 0x4)
		SendDlgItemMessage(hDlg, PERM_A_READ, BM_SETCHECK, 1, 0);
	if(Attr & 0x2)
		SendDlgItemMessage(hDlg, PERM_A_WRITE, BM_SETCHECK, 1, 0);
	if(Attr & 0x1)
		SendDlgItemMessage(hDlg, PERM_A_EXEC, BM_SETCHECK, 1, 0);

	return;
}


/*----- ダイアログボックスの内容から属性を取得 --------------------------------
*
*	Parameter
*		HWND hWnd : ダイアログボックスのウインドウハンドル
*
*	Return Value
*		int 属性
*----------------------------------------------------------------------------*/

static int GetAttrFromDialog(HWND hDlg)
{
	int Ret;

	Ret = 0;

	if(SendDlgItemMessage(hDlg, PERM_O_READ, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x400;
	if(SendDlgItemMessage(hDlg, PERM_O_WRITE, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x200;
	if(SendDlgItemMessage(hDlg, PERM_O_EXEC, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x100;

	if(SendDlgItemMessage(hDlg, PERM_G_READ, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x40;
	if(SendDlgItemMessage(hDlg, PERM_G_WRITE, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x20;
	if(SendDlgItemMessage(hDlg, PERM_G_EXEC, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x10;

	if(SendDlgItemMessage(hDlg, PERM_A_READ, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x4;
	if(SendDlgItemMessage(hDlg, PERM_A_WRITE, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x2;
	if(SendDlgItemMessage(hDlg, PERM_A_EXEC, BM_GETCHECK, 0, 0) == 1)
		Ret |= 0x1;

	return(Ret);
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
	char Cmd[81];

	// 同時接続対応
	CancelFlg = NO;

	if(GetFocus() == GetRemoteHwnd())
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			std::vector<FILELIST> FileListBase;
			MakeSelectedFileList(WIN_REMOTE, NO, NO, FileListBase, &CancelFlg);
			memset(Cmd, NUL, 81);
			if (!empty(FileListBase))
				strncpy(Cmd, FileListBase[0].File, 80);
			if (InputDialog(somecmd_dlg, GetMainHwnd(), NULL, Cmd, 81, nullptr, IDH_HELP_TOPIC_0000023))
				DoQUOTE(AskCmdCtrlSkt(), Cmd, &CancelFlg);
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
		SizeNotify(int win) : win{ win } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessage(hDlg, FSNOTIFY_TITLE, WM_SETTEXT, 0, (LPARAM)(win == WIN_LOCAL ? MSGJPN074 : MSGJPN075));
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, SendDlgItemMessage(hDlg, FSNOTIFY_SEL_ONLY, BM_GETCHECK, 0, 0) == 1 ? NO : YES);
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
		const char* size;
		Size(int win, const char* size) : win{ win }, size{ size } {}
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessage(hDlg, FSIZE_TITLE, WM_SETTEXT, 0, (LPARAM)(win == WIN_LOCAL ? MSGJPN076 : MSGJPN077));
			SendDlgItemMessage(hDlg, FSIZE_SIZE, WM_SETTEXT, 0, (LPARAM)size);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg, 0);
				break;
			}
		}
	};
	int Win = GetFocus() == GetLocalHwnd() ? WIN_LOCAL : WIN_REMOTE;
	CancelFlg = NO;
	if (auto All = Dialog(GetFtpInst(), filesize_notify_dlg, GetMainHwnd(), SizeNotify{ Win }); All != NO_ALL)
		if (Win == WIN_LOCAL || CheckClosedAndReconnect() == FFFTP_SUCCESS) {
			std::vector<FILELIST> ListBase;
			MakeSelectedFileList(Win, YES, All, ListBase, &CancelFlg);
			double total = 0;
			for (auto const& f : ListBase)
				if (f.Node != NODE_DIR)
					total += f.Size;
			char size[FMAX_PATH + 1];
			MakeSizeString(total, size);
			Dialog(GetFtpInst(), filesize_dlg, GetMainHwnd(), Size{ Win, size });
		}
}


// ディレクトリ移動失敗時のエラーを表示
void DispCWDerror(HWND hWnd) {
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
	auto baseAddress = L"ftp://"s + u8(AskHostAdrs());
	if (auto port = AskHostPort(); port != PORT_NOR)
		baseAddress.append(L":").append(std::to_wstring(port));
	{
		char dir[FMAX_PATH + 1];
		AskRemoteCurDir(dir, FMAX_PATH);
		SetSlashTail(dir);
		if (AskHostType() == HTYPE_VMS) {
			baseAddress += L"/"sv;
			ReformToVMSstylePathName(dir);
		}
		baseAddress += u8(dir);
	}
	std::wstring text;
	for (auto const& f : FileListBase)
		text.append(baseAddress).append(u8(f.File)).append(L"\r\n"sv);
	if (OpenClipboard(GetMainHwnd())) {
		if (EmptyClipboard())
			if (auto global = GlobalAlloc(GHND, (size_as<SIZE_T>(text) + 1) * sizeof(wchar_t)); global)
				if (auto buffer = GlobalLock(global); buffer) {
					std::copy(begin(text), end(text), reinterpret_cast<wchar_t*>(buffer));
					GlobalUnlock(global);
					SetClipboardData(CF_UNICODETEXT, global);
				}
		CloseClipboard();
	}
}


/*----- フルパスを使わないファイルアクセスの準備 ------------------------------
*
*	Parameter
*		char *Path : パス名
*		char *CurDir : カレントディレクトリ
*		HWND hWnd : エラーウインドウを表示する際の親ウインドウ
*		int Type : 使用するソケットの種類
*			0=コマンドソケット, 1=転送ソケット
*
*	Return Value
*		int ステータス(FFFTP_SUCCESS/FFFTP_FAIL)
*
*	Note
*		フルパスを使わない時は、
*			このモジュール内で CWD を行ない、
*			Path にファイル名のみ残す。（パス名は消す）
*----------------------------------------------------------------------------*/

// 同時接続対応
//int ProcForNonFullpath(char *Path, char *CurDir, HWND hWnd, int Type)
int ProcForNonFullpath(SOCKET cSkt, char *Path, char *CurDir, HWND hWnd, int *CancelCheckWork)
{
	int Sts;
	int Cmd;
	char Tmp[FMAX_PATH+1];

	Sts = FFFTP_SUCCESS;
	if(AskNoFullPathMode() == YES)
	{
		strcpy(Tmp, Path);
		if(AskHostType() == HTYPE_VMS)
		{
			GetUpperDirEraseTopSlash(Tmp);
			ReformToVMSstyleDirName(Tmp);
		}
		else if(AskHostType() == HTYPE_STRATUS)
			GetUpperDirEraseTopSlash(Tmp);
		else
			GetUpperDir(Tmp);

		if(strcmp(Tmp, CurDir) != 0)
		{
			Cmd = CommandProcTrn(cSkt, NULL, CancelCheckWork, "CWD %s", Tmp);

			if(Cmd/100 != FTP_COMPLETE)
			{
				DispCWDerror(hWnd);
				Sts = FFFTP_FAIL;
			}
			else
				strcpy(CurDir, Tmp);
		}
		strcpy(Path, GetFileName(Path));
	}
	return(Sts);
}


/*----- ディレクトリ名をVAX VMSスタイルに変換する -----------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*
*	Note
*		ddd:[xxx.yyy]/rrr/ppp  --> ddd:[xxx.yyy.rrr.ppp]
*----------------------------------------------------------------------------*/

void ReformToVMSstyleDirName(char *Path)
{
	char *Pos;
	char *Btm;

	if((Btm = strchr(Path, ']')) != NULL)
	{
		Pos = Btm;
		while((Pos = strchr(Pos, '/')) != NULL)
			*Pos = '.';

		memmove(Btm, Btm+1, strlen(Btm+1)+1);
		Pos = strchr(Path, NUL);
		if(*(Pos-1) == '.')
		{
			Pos--;
			*Pos = NUL;
		}
		strcpy(Pos, "]");
	}
	return;
}


/*----- ファイル名をVAX VMSスタイルに変換する ---------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*
*	Note
*		ddd:[xxx.yyy]/rrr/ppp  --> ddd:[xxx.yyy.rrr]ppp
*----------------------------------------------------------------------------*/

void ReformToVMSstylePathName(char *Path)
{
	char Fname[FMAX_PATH+1];

	strcpy(Fname, GetFileName(Path));

	GetUpperDirEraseTopSlash(Path);
	ReformToVMSstyleDirName(Path);

	strcat(Path, Fname);

	return;
}


#if defined(HAVE_OPENVMS)
/*----- VMSの"HOGE.DIR;?"というディレクトリ名から"HOGE"を取り出す ---------------
*
*	Parameter
*		char *DirName : "HOGE.DIR;?"形式のディレクトリ名
*		int Flg       : ";"のチェックをする(TRUE)かしない(FALSE)か
*
*	Return Value
*		なし
*
*	Note
*		DirNameを直接書きかえる
*----------------------------------------------------------------------------*/

void ReformVMSDirName(char *DirName, int Flg)
{
	char *p;

	if (Flg == TRUE) {
		/* ';'がない場合はVMS形式じゃなさそうなので何もしない */
		if ((p = strrchr(DirName, ';')) == NULL)
			return;
	}

	/* ".DIR"があったらつぶす */
	if ((p = strrchr(DirName, '.'))) {
		if (memcmp(p + 1, "DIR", 3) == 0)
			*p = '\0';
	}
}
#endif


/*----- ファイル名に使えない文字がないかチェックし名前を変更する --------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL=中止する
*
*	Note
*		Fnameを直接書きかえる
*----------------------------------------------------------------------------*/

static int RenameUnuseableName(char *Fname)
{
	int Ret;

	Ret = FFFTP_SUCCESS;
	while(1)
	{
		if((_mbschr((const unsigned char *)Fname, ':') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '*') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '?') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '<') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '>') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '|') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '\x22') != NULL) ||
		   (_mbschr((const unsigned char *)Fname, '\\') != NULL))
		{
			if (!InputDialog(forcerename_dlg, GetMainHwnd(), NULL, Fname, FMAX_PATH+1))
			{
				Ret = FFFTP_FAIL;
				break;
			}
		}
		else
			break;
	}
	return(Ret);
}


// 自動切断対策
// NOOPコマンドでは効果が無いホストが多いためLISTコマンドを使用
void NoopProc(int Force)
{
	if(Force == YES || (AskConnecting() == YES && AskUserOpeDisabled() == NO))
	{
		if(AskReuseCmdSkt() == NO || (AskShareProh() == YES && AskTransferNow() == NO))
		{
			if(Force == NO)
				CancelFlg = NO;
			DisableUserOpe();
			DoDirListCmdSkt("", "", 999, &CancelFlg);
			EnableUserOpe();
		}
	}
}

// 同時接続対応
void AbortRecoveryProc(void)
{
	if(AskConnecting() == YES && AskUserOpeDisabled() == NO)
	{
		if(AskReuseCmdSkt() == NO || AskShareProh() == YES || AskTransferNow() == NO)
		{
			CancelFlg = NO;
			if(AskErrorReconnect() == YES)
			{
				DisableUserOpe();
				ReConnectCmdSkt();
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
				EnableUserOpe();
			}
			else
				RemoveReceivedData(AskCmdCtrlSkt());
		}
	}
	return;
}

void ReconnectProc(void)
{
	if(AskConnecting() == YES && AskUserOpeDisabled() == NO)
	{
		CancelFlg = NO;
		DisableUserOpe();
		ReConnectCmdSkt();
		GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
		EnableUserOpe();
	}
	return;
}

