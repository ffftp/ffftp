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

#define  STRICT
// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
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

static int CheckRemoteFile(TRANSPACKET *Pkt, FILELIST *ListList);
// 64ビット対応
//static BOOL CALLBACK UpExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK UpExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

static void DispMirrorFiles(FILELIST *Local, FILELIST *Remote);
static void MirrorDeleteAllLocalDir(FILELIST *Local, TRANSPACKET *Pkt, TRANSPACKET **Base);
static int CheckLocalFile(TRANSPACKET *Pkt);
// 64ビット対応
//static BOOL CALLBACK DownExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DownExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static void RemoveAfterSemicolon(char *Path);
static void MirrorDeleteAllDir(FILELIST *Remote, TRANSPACKET *Pkt, TRANSPACKET **Base);
// 64ビット対応
//static BOOL CALLBACK MirrorNotifyCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK MirrorDispListCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK MirrorNotifyCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK MirrorDispListCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static void CountMirrorFiles(HWND hDlg, TRANSPACKET *Pkt);
static int AskMirrorNoTrn(char *Fname, int Mode);
static int AskUploadFileAttr(char *Fname);
// 64ビット対応
//static BOOL CALLBACK UpDownAsDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK UpDownAsDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
#if defined(HAVE_TANDEM)
static INT_PTR CALLBACK UpDownAsWithExtDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
#endif
static void DeleteAllDir(FILELIST *Dt, int Win, int *Sw, int *Flg, char *CurDir);
static void DelNotifyAndDo(FILELIST *Dt, int Win, int *Sw, int *Flg, char *CurDir);
// 64ビット対応
//static BOOL CALLBACK DeleteDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK RenameDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DeleteDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK RenameDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static void SetAttrToDialog(HWND hWnd, int Attr);
static int GetAttrFromDialog(HWND hDlg);
static LRESULT CALLBACK SizeNotifyDlgWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK SizeDlgWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int RenameUnuseableName(char *Fname);

/*===== 外部参照 ====*/

extern HWND hHelpWin;

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

/*===== ローカルなワーク =====*/

static char TmpString[FMAX_PATH+80];		/* テンポラリ */
#if defined(HAVE_TANDEM)
static char TmpFileCode[5];		/* テンポラリ */
#endif
static int CurWin;						/* ウインドウ番号 */

int UpExistMode = EXIST_OVW;		/* アップロードで同じ名前のファイルがある時の扱い方 EXIST_xxx */
int ExistMode = EXIST_OVW;		/* 同じ名前のファイルがある時の扱い方 EXIST_xxx */
static int ExistNotify;		/* 確認ダイアログを出すかどうか YES/NO */

static double FileSize;		/* ファイル総容量 */



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
		Len = pNext - LocalFile;
		strncpy(Pkt.LocalFile, LocalFile, Len);
		Pkt.LocalFile[Len] = '\0';
		if(strncmp(LocalFile, Old, Len + 1) != 0)
		{
			Cat = Pkt.LocalFile + (pDelimiter - LocalFile);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr(Cat);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr(Cat);
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
	FILELIST *FileListBase;
	FILELIST *Pos;
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
//		KeepTransferDialog(YES);

		FileListBase = NULL;
		// ファイル一覧バグ修正
//		MakeSelectedFileList(WIN_REMOTE, (ForceFile == YES ? NO : YES), All, &FileListBase, &CancelFlg);
		ListSts = MakeSelectedFileList(WIN_REMOTE, (ForceFile == YES ? NO : YES), All, &FileListBase, &CancelFlg);

		if(AskNoFullPathMode() == YES)
		{
			strcpy(Pkt.Cmd, "SETCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}

		Pos = FileListBase;
		while(Pos != NULL)
		{
			// ファイル一覧バグ修正
			if((AbortOnListError == YES) && (ListSts == FFFTP_FAIL))
				break;
			AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
			SetYenTail(Pkt.LocalFile);
			strcpy(TmpString, Pos->File);
			if((ChName == NO) || ((ForceFile == NO) && (Pos->Node == NODE_DIR)))
			{
				if(FnameCnv == FNAME_LOWER)
					_mbslwr(TmpString);
				else if(FnameCnv == FNAME_UPPER)
					_mbsupr(TmpString);
				RemoveAfterSemicolon(TmpString);
				if(RenameUnuseableName(TmpString) == FFFTP_FAIL)
					break;
			}
			else
			{
				CurWin = WIN_REMOTE;
				if(DialogBox(GetFtpInst(), MAKEINTRESOURCE(updown_as_dlg), GetMainHwnd(), UpDownAsDialogCallBack) == YES)
				{
					if(RenameUnuseableName(TmpString) == FFFTP_FAIL)
						break;
				}
				else
					break;
			}
			strcat(Pkt.LocalFile, TmpString);
			ReplaceAll(Pkt.LocalFile, '/', '\\');

			if((ForceFile == NO) && (Pos->Node == NODE_DIR))
			{
				strcpy(Pkt.Cmd, "MKD ");
				strcpy(Pkt.RemoteFile, "");
				AddTransFileList(&Pkt);
			}
			else if((Pos->Node == NODE_FILE) ||
					((ForceFile == YES) && (Pos->Node == NODE_DIR)))
			{
				if(AskHostType() == HTYPE_ACOS)
				{
					strcpy(Pkt.RemoteFile, "'");
					strcat(Pkt.RemoteFile, AskHostLsName());
					strcat(Pkt.RemoteFile, "(");
					strcat(Pkt.RemoteFile, Pos->File);
					strcat(Pkt.RemoteFile, ")");
					strcat(Pkt.RemoteFile, "'");
				}
				else if(AskHostType() == HTYPE_ACOS_4)
				{
					strcpy(Pkt.RemoteFile, Pos->File);
				}
				else
				{
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					strcat(Pkt.RemoteFile, Pos->File);
					ReplaceAll(Pkt.RemoteFile, '\\', '/');
				}

				strcpy(Pkt.Cmd, "RETR ");
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM) {
					if(AskTransferType() != TYPE_X) {
						Pkt.Type = AskTransferType();
					} else {
						Pkt.Attr = Pos->Attr;
						if (Pkt.Attr == 101)
							Pkt.Type = TYPE_A;
						else
							Pkt.Type = TYPE_I;
					}
				} else
#endif
				Pkt.Type = AskTransferTypeAssoc(Pkt.RemoteFile, AskTransferType());
				Pkt.Size = Pos->Size;
				Pkt.Time = Pos->Time;
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();

				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.LocalFile);
				Pkt.Mode = CheckLocalFile(&Pkt);	/* Pkt.ExistSize がセットされる */
				if(Pkt.Mode == EXIST_ABORT)
					break;
				else if(Pkt.Mode != EXIST_IGNORE)
				// ディレクトリ自動作成
//					AddTransFileList(&Pkt);
				{
					if(MakeAllDir == YES)
						MakeDirFromLocalPath(Pkt.LocalFile, Tmp);
					AddTransFileList(&Pkt);
				}
			}
			Pos = Pos->Next;
		}

		if(AskNoFullPathMode() == YES)
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}
		DeleteFileList(&FileListBase);

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
				_mbslwr(TmpString);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr(TmpString);
			RemoveAfterSemicolon(TmpString);

			if(RenameUnuseableName(TmpString) == FFFTP_SUCCESS)
			{
				strcat(Pkt.LocalFile, TmpString);
				ReplaceAll(Pkt.LocalFile, '/', '\\');

				if(AskHostType() == HTYPE_ACOS)
				{
					strcpy(Pkt.RemoteFile, "'");
					strcat(Pkt.RemoteFile, AskHostLsName());
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


/*----- 入力されたファイル名のファイルを一つダウンロードする ------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void InputDownloadProc(void)
{
	char Path[FMAX_PATH+1];
	int Tmp;

//	DisableUserOpe();

	strcpy(Path, "");
	if(InputDialogBox(downname_dlg, GetMainHwnd(), NULL, Path, FMAX_PATH, &Tmp, IDH_HELP_TOPIC_0000001) == YES)
	{
		DirectDownloadProc(Path);
	}

//	EnableUserOpe();

	return;
}


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
	FILELIST *LocalListBase;
	FILELIST *RemoteListBase;
	FILELIST *LocalPos;
	FILELIST *RemotePos;
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

		if(Notify == YES)
			Notify = DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(mirror_down_dlg), GetMainHwnd(), MirrorNotifyCallBack, 0);
		else
			Notify = YES;

		if((Notify == YES) || (Notify == YES_LIST))
		{
			/*===== ファイルリスト取得 =====*/

			LocalListBase = NULL;
			// ファイル一覧バグ修正
//			MakeSelectedFileList(WIN_LOCAL, YES, YES, &LocalListBase, &CancelFlg);
			ListSts = MakeSelectedFileList(WIN_LOCAL, YES, YES, &LocalListBase, &CancelFlg);
			RemoteListBase = NULL;
			// ファイル一覧バグ修正
//			MakeSelectedFileList(WIN_REMOTE, YES, YES, &RemoteListBase, &CancelFlg);
			if(ListSts == FFFTP_SUCCESS)
				ListSts = MakeSelectedFileList(WIN_REMOTE, YES, YES, &RemoteListBase, &CancelFlg);

			RemotePos = RemoteListBase;
			while(RemotePos != NULL)
			{
				RemotePos->Attr = YES;		/* RemotePos->Attrは転送するかどうかのフラグに使用 (YES/NO) */
				RemotePos = RemotePos->Next;
			}

			LocalPos = LocalListBase;
			while(LocalPos != NULL)
			{
				if(AskMirrorNoTrn(LocalPos->File, 1) == NO)
				{
					LocalPos->Attr = YES;
					LocalPos = LocalPos->Next;
				}
				else
				{
					LocalPos->Attr = NO;	/* LocalPos->Attrは削除するかどうかのフラグに使用 (YES/NO) */

					if(LocalPos->Node == NODE_DIR)
					{
						Level = AskDirLevel(LocalPos->File);
						LocalPos = LocalPos->Next;
						while(LocalPos != NULL)
						{
							if((LocalPos->Node == NODE_DIR) &&
							   (AskDirLevel(LocalPos->File) <= Level))
							{
								break;
							}
							LocalPos->Attr = NO;
							LocalPos = LocalPos->Next;
						}
					}
					else
						LocalPos = LocalPos->Next;
				}
			}

			/*===== ファイルリスト比較 =====*/

			RemotePos = RemoteListBase;
			while(RemotePos != NULL)
			{
				if(AskMirrorNoTrn(RemotePos->File, 0) == NO)
				{
					strcpy(Name, RemotePos->File);
//					ReplaceAll(Name, '/', '\\');

					if(MirrorFnameCnv == YES)
						Mode = COMP_LOWERMATCH;
					else
						Mode = COMP_STRICT;

					if((LocalPos = SearchFileList(Name, LocalListBase, Mode)) != NULL)
					{
						if((RemotePos->Node == NODE_DIR) && (LocalPos->Node == NODE_DIR))
						{
							LocalPos->Attr = NO;
							RemotePos->Attr = NO;
						}
						else if((RemotePos->Node == NODE_FILE) && (LocalPos->Node == NODE_FILE))
						{
							LocalPos->Attr = NO;
							if(CompareFileTime(&RemotePos->Time, &LocalPos->Time) <= 0)
								RemotePos->Attr = NO;
						}
					}
					RemotePos = RemotePos->Next;
				}
				else
				{
					if(RemotePos->Node == NODE_FILE)
					{
						RemotePos->Attr = NO;
						RemotePos = RemotePos->Next;
					}
					else
					{
						RemotePos->Attr = NO;
						Level = AskDirLevel(RemotePos->File);
						RemotePos = RemotePos->Next;
						while(RemotePos != NULL)
						{
							if((RemotePos->Node == NODE_DIR) &&
							   (AskDirLevel(RemotePos->File) <= Level))
							{
								break;
							}
							RemotePos->Attr = NO;
							RemotePos = RemotePos->Next;
						}
					}
				}
			}

			DispMirrorFiles(LocalListBase, RemoteListBase);

			/*===== 削除／アップロード =====*/

			LocalPos = LocalListBase;
			while(LocalPos != NULL)
			{
				if((LocalPos->Attr == YES) && (LocalPos->Node == NODE_FILE))
				{
					AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
					SetYenTail(Pkt.LocalFile);
					strcat(Pkt.LocalFile, LocalPos->File);
					ReplaceAll(Pkt.LocalFile, '/', '\\');
					strcpy(Pkt.RemoteFile, "");
					strcpy(Pkt.Cmd, "L-DELE ");
					AddTmpTransFileList(&Pkt, &Base);
				}
				LocalPos = LocalPos->Next;
			}
			MirrorDeleteAllLocalDir(LocalListBase, &Pkt, &Base);


			RemotePos = RemoteListBase;
			while(RemotePos != NULL)
			{
				if(RemotePos->Attr == YES)
				{
					AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
					SetYenTail(Pkt.LocalFile);
					Cat = strchr(Pkt.LocalFile, NUL);
					strcat(Pkt.LocalFile, RemotePos->File);

					if(MirrorFnameCnv == YES)
						_mbslwr(Cat);

					RemoveAfterSemicolon(Cat);
					ReplaceAll(Pkt.LocalFile, '/', '\\');

					if(RemotePos->Node == NODE_DIR)
					{
						strcpy(Pkt.RemoteFile, "");
						strcpy(Pkt.Cmd, "L-MKD ");
						AddTmpTransFileList(&Pkt, &Base);
					}
					else if(RemotePos->Node == NODE_FILE)
					{
						AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
						SetSlashTail(Pkt.RemoteFile);
						strcat(Pkt.RemoteFile, RemotePos->File);
						ReplaceAll(Pkt.RemoteFile, '\\', '/');

						strcpy(Pkt.Cmd, "RETR ");
						Pkt.Type = AskTransferTypeAssoc(Pkt.RemoteFile, AskTransferType());
						Pkt.Size = RemotePos->Size;
						Pkt.Time = RemotePos->Time;
//						Pkt.Attr = 0;
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
				RemotePos = RemotePos->Next;
			}

			// ファイル一覧バグ修正
//			if((Notify == YES) ||
//			   (DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(mirrordown_notify_dlg), GetMainHwnd(), MirrorDispListCallBack, (LPARAM)&Base) == YES))
			if(((AbortOnListError == NO) || (ListSts == FFFTP_SUCCESS)) && ((Notify == YES) ||
			   (DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(mirrordown_notify_dlg), GetMainHwnd(), MirrorDispListCallBack, (LPARAM)&Base) == YES)))
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

			DeleteFileList(&LocalListBase);
			DeleteFileList(&RemoteListBase);

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

static void DispMirrorFiles(FILELIST *Local, FILELIST *Remote)
{
	char Date[80];
	SYSTEMTIME sTime;
	FILETIME fTime;

	if(DebugConsole == YES)
	{
		DoPrintf("---- MIRROR FILE LIST ----");
		while(Local != NULL)
		{
			FileTimeToLocalFileTime(&Local->Time, &fTime);
			FileTimeToSystemTime(&fTime, &sTime);
			sprintf(Date, "%04d/%02d/%02d %02d:%02d:%02d.%04d", 
				sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);
			DoPrintf("LOCAL  : %s %s [%s] %s", Local->Attr==1?"YES":"NO ", Local->Node==NODE_DIR?"DIR ":"FILE", Date, Local->File);
			Local = Local->Next;
		}
		while(Remote != NULL)
		{
			FileTimeToLocalFileTime(&Remote->Time, &fTime);
			FileTimeToSystemTime(&fTime, &sTime);
			sprintf(Date, "%04d/%02d/%02d %02d:%02d:%02d.%04d", 
				sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);
			DoPrintf("REMOTE : %s %s [%s] %s", Remote->Attr==1?"YES":"NO ", Remote->Node==NODE_DIR?"DIR ":"FILE", Date, Remote->File);
			Remote = Remote->Next;
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

static void MirrorDeleteAllLocalDir(FILELIST *Local, TRANSPACKET *Pkt, TRANSPACKET **Base)
{
	while(Local != NULL)
	{
		if(Local->Node == NODE_DIR)
		{
			MirrorDeleteAllLocalDir(Local->Next, Pkt, Base);

			if(Local->Attr == YES)
			{
				AskLocalCurDir(Pkt->LocalFile, FMAX_PATH);
				SetYenTail(Pkt->LocalFile);
				strcat(Pkt->LocalFile, Local->File);
				ReplaceAll(Pkt->LocalFile, '/', '\\');
				strcpy(Pkt->RemoteFile, "");
				strcpy(Pkt->Cmd, "L-RMD ");
				AddTmpTransFileList(Pkt, Base);
			}
			break;
		}
		Local = Local->Next;
	}
	return;
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
	HANDLE fHnd;
	WIN32_FIND_DATA Find;
	int Ret;

	Ret = EXIST_OVW;
	Pkt->ExistSize = 0;
	if(RecvMode != TRANS_OVW)
	{
		if((fHnd = FindFirstFile(Pkt->LocalFile, &Find)) != INVALID_HANDLE_VALUE)
		{
			FindClose(fHnd);

			Pkt->ExistSize = MakeLongLong(Find.nFileSizeHigh, Find.nFileSizeLow);

			if(ExistNotify == YES)
			{
				SoundPlay(SND_ERROR);
				if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(down_exist_dlg), GetMainHwnd(), DownExistDialogCallBack, (LPARAM)Pkt) == NO)
					Ret = EXIST_ABORT;
				else
					Ret = ExistMode;
			}
			else
				Ret = ExistMode;

			if(Ret == EXIST_NEW)
			{
				/*ファイル日付チェック */
				if(CompareFileTime(&Find.ftLastWriteTime, &Pkt->Time) < 0)
					Ret = EXIST_OVW;
				else
					Ret = EXIST_IGNORE;
			}
			// 同じ名前のファイルの処理方法追加
			if(Ret == EXIST_LARGE)
			{
				if(MakeLongLong(Find.nFileSizeHigh, Find.nFileSizeLow) < Pkt->Size)
					Ret = EXIST_OVW;
				else
					Ret = EXIST_IGNORE;
			}
		}
	}
	return(Ret);
}


/*----- ローカルに同じ名前のファイルがある時の確認ダイアログのコールバック ----
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
//static BOOL CALLBACK DownExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK DownExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static TRANSPACKET *Pkt;
	// 同じ名前のファイルの処理方法追加
//	static const RADIOBUTTON DownExistButton[] = {
//		{ DOWN_EXIST_OVW, EXIST_OVW },
//		{ DOWN_EXIST_NEW, EXIST_NEW },
//		{ DOWN_EXIST_RESUME, EXIST_RESUME },
//		{ DOWN_EXIST_IGNORE, EXIST_IGNORE }
//	};
	static const RADIOBUTTON DownExistButton[] = {
		{ DOWN_EXIST_OVW, EXIST_OVW },
		{ DOWN_EXIST_NEW, EXIST_NEW },
		{ DOWN_EXIST_RESUME, EXIST_RESUME },
		{ DOWN_EXIST_IGNORE, EXIST_IGNORE },
		{ DOWN_EXIST_LARGE, EXIST_LARGE }
	};
	#define DOWNEXISTBUTTONS	(sizeof(DownExistButton)/sizeof(RADIOBUTTON))

	switch (iMessage)
	{
		case WM_INITDIALOG :
			Pkt = (TRANSPACKET *)lParam;
			SendDlgItemMessage(hDlg, DOWN_EXIST_NAME, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, DOWN_EXIST_NAME, WM_SETTEXT, 0, (LPARAM)Pkt->LocalFile);

			if((Pkt->Type == TYPE_A) || (Pkt->ExistSize <= 0))
				EnableWindow(GetDlgItem(hDlg, DOWN_EXIST_RESUME), FALSE);

			SetRadioButtonByValue(hDlg, ExistMode, DownExistButton, DOWNEXISTBUTTONS);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK_ALL :
					ExistNotify = NO;
					/* ここに break はない */

				case IDOK :
					ExistMode = AskRadioButtonValue(hDlg, DownExistButton, DOWNEXISTBUTTONS);
					SendDlgItemMessage(hDlg, DOWN_EXIST_NAME, WM_GETTEXT, FMAX_PATH, (LPARAM)Pkt->LocalFile);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
//					ExistMode = EXIST_ABORT;
					EndDialog(hDlg, NO);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000009);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
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
		Len = pNext - RemoteFile;
		strncpy(Pkt.RemoteFile, RemoteFile, Len);
		Pkt.RemoteFile[Len] = '\0';
		if(strncmp(RemoteFile, Old, Len + 1) != 0)
		{
			Cat = Pkt.RemoteFile + (pDelimiter - RemoteFile);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr(Cat);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr(Cat);
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
				strcat(Pkt.RemoteFile, AskHostLsName());
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
	FILELIST *FileListBase;
	FILELIST *Pos;
	TRANSPACKET Pkt;
	TRANSPACKET Pkt1;
	char *Cat;
	FILELIST *RemoteList;
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
		FileListBase = NULL;
		// ファイル一覧バグ修正
//		MakeSelectedFileList(WIN_LOCAL, YES, All, &FileListBase, &CancelFlg);
		ListSts = MakeSelectedFileList(WIN_LOCAL, YES, All, &FileListBase, &CancelFlg);

		// 現在ホスト側のファイル一覧に表示されているものをRemoteListに登録
		// 同名ファイルチェック用
		RemoteList = NULL;
		AddRemoteTreeToFileList(AskCurrentFileListNum(), "", RDIR_NONE, &RemoteList);

		FirstAdd = YES;
		ExistNotify = YES;

		Pos = FileListBase;
		while(Pos != NULL)
		{
			// ファイル一覧バグ修正
			if((AbortOnListError == YES) && (ListSts == FFFTP_FAIL))
				break;
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			SetSlashTail(Pkt.RemoteFile);
			Cat = strchr(Pkt.RemoteFile, NUL);
			if((ChName == NO) || (Pos->Node == NODE_DIR))
			{
				strcat(Pkt.RemoteFile, Pos->File);
				if(FnameCnv == FNAME_LOWER)
					_mbslwr(Cat);
				else if(FnameCnv == FNAME_UPPER)
					_mbsupr(Cat);
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
				strcpy(TmpString, Pos->File);
				CurWin = WIN_LOCAL;
#if defined(HAVE_TANDEM)
				strcpy(TmpFileCode, "0"); /* ASCII モードの場合は無視される */
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					if(DialogBox(GetFtpInst(), MAKEINTRESOURCE(updown_as_with_ext_dlg), GetMainHwnd(), UpDownAsWithExtDialogCallBack) == YES) {
						strcat(Pkt.RemoteFile, TmpString);
						Pkt.FileCode = atoi(TmpFileCode);
					} else {
						break;
					}
				} else
#endif
				if(DialogBox(GetFtpInst(), MAKEINTRESOURCE(updown_as_dlg), GetMainHwnd(), UpDownAsDialogCallBack) == YES)
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
				strcat(Pkt.RemoteFile, AskHostLsName());
				strcat(Pkt.RemoteFile, "(");
				strcat(Pkt.RemoteFile, Cat);
				strcat(Pkt.RemoteFile, ")");
				strcat(Pkt.RemoteFile, "'");
			}
			else if(AskHostType() == HTYPE_ACOS_4)
				strcpy(Pkt.RemoteFile, Cat);

			if(Pos->Node == NODE_DIR)
			{
				// フォルダの場合

				// ホスト側のファイル一覧をRemoteListに登録
				// 同名ファイルチェック用
				if(RemoteList != NULL)
					DeleteFileList(&RemoteList);
				RemoteList = NULL;

				AskRemoteCurDir(Tmp, FMAX_PATH);
				if(DoCWD(Pkt.RemoteFile, NO, NO, NO) == FTP_COMPLETE)
				{
					if(DoDirListCmdSkt("", "", 998, &CancelFlg) == FTP_COMPLETE)
						AddRemoteTreeToFileList(998, "", RDIR_NONE, &RemoteList);
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
			else if(Pos->Node == NODE_FILE)
			{
				// ファイルの場合
				AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
				SetYenTail(Pkt.LocalFile);
				strcat(Pkt.LocalFile, Pos->File);
				ReplaceAll(Pkt.LocalFile, '/', '\\');

				strcpy(Pkt.Cmd, "STOR ");
				Pkt.Type = AskTransferTypeAssoc(Pkt.LocalFile, AskTransferType());
				// バグ修正
//				Pkt.Size = 0;
				Pkt.Size = Pos->Size;
				Pkt.Time = Pos->Time;
				Pkt.Attr = AskUploadFileAttr(Pkt.RemoteFile);
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					CalcExtentSize(&Pkt, Pos->Size);
				}
#endif
				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.RemoteFile);
				Pkt.Mode = CheckRemoteFile(&Pkt, RemoteList);
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
			Pos = Pos->Next;
		}

		if((FirstAdd == NO) && (AskNoFullPathMode() == YES))
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}

		if(RemoteList != NULL)
			DeleteFileList(&RemoteList);

		DeleteFileList(&FileListBase);

		// 同時接続対応
//		strcpy(Pkt.Cmd, "GOQUIT");
//		AddTransFileList(&Pkt);

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
	FILELIST *FileListBase;
	FILELIST *Pos;
	TRANSPACKET Pkt;
	TRANSPACKET Pkt1;
	char *Cat;
	FILELIST *RemoteList;
	char Tmp[FMAX_PATH+1];
	int FirstAdd;
	char Cur[FMAX_PATH+1];

	// 同時接続対応
	CancelFlg = NO;

	if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
	{
		DisableUserOpe();

		// ローカル側で選ばれているファイルをFileListBaseに登録
		FileListBase = NULL;
		MakeDroppedFileList(wParam, Cur, &FileListBase);

		// 現在ホスト側のファイル一覧に表示されているものをRemoteListに登録
		// 同名ファイルチェック用
		RemoteList = NULL;
		AddRemoteTreeToFileList(AskCurrentFileListNum(), "", RDIR_NONE, &RemoteList);

		FirstAdd = YES;
		ExistNotify = YES;

		Pos = FileListBase;
		while(Pos != NULL)
		{
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			SetSlashTail(Pkt.RemoteFile);
			Cat = strchr(Pkt.RemoteFile, NUL);

			strcat(Pkt.RemoteFile, Pos->File);
			if(FnameCnv == FNAME_LOWER)
				_mbslwr(Cat);
			else if(FnameCnv == FNAME_UPPER)
				_mbsupr(Cat);
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
				strcat(Pkt.RemoteFile, AskHostLsName());
				strcat(Pkt.RemoteFile, "(");
				strcat(Pkt.RemoteFile, Cat);
				strcat(Pkt.RemoteFile, ")");
				strcat(Pkt.RemoteFile, "'");
			}
			else if(AskHostType() == HTYPE_ACOS_4)
				strcpy(Pkt.RemoteFile, Cat);

			if(Pos->Node == NODE_DIR)
			{
				// フォルダの場合

				// ホスト側のファイル一覧をRemoteListに登録
				// 同名ファイルチェック用
				if(RemoteList != NULL)
					DeleteFileList(&RemoteList);
				RemoteList = NULL;

				AskRemoteCurDir(Tmp, FMAX_PATH);
				if(DoCWD(Pkt.RemoteFile, NO, NO, NO) == FTP_COMPLETE)
				{
					if(DoDirListCmdSkt("", "", 998, &CancelFlg) == FTP_COMPLETE)
						AddRemoteTreeToFileList(998, "", RDIR_NONE, &RemoteList);
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
			else if(Pos->Node == NODE_FILE)
			{
				// ファイルの場合
				strcpy(Pkt.LocalFile, Cur);
				SetYenTail(Pkt.LocalFile);
				strcat(Pkt.LocalFile, Pos->File);
				ReplaceAll(Pkt.LocalFile, '/', '\\');

				strcpy(Pkt.Cmd, "STOR ");
				Pkt.Type = AskTransferTypeAssoc(Pkt.LocalFile, AskTransferType());
				// バグ修正
//				Pkt.Size = 0;
				Pkt.Size = Pos->Size;
				Pkt.Time = Pos->Time;
				Pkt.Attr = AskUploadFileAttr(Pkt.RemoteFile);
				Pkt.KanjiCode = AskHostKanjiCode();
				// UTF-8対応
				Pkt.KanjiCodeDesired = AskLocalKanjiCode();
				Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
				if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
					int a = Pos->InfoExist && FINFO_SIZE;
					CalcExtentSize(&Pkt, Pos->Size);
				}
#endif
				// ディレクトリ自動作成
				strcpy(Tmp, Pkt.RemoteFile);
				Pkt.Mode = CheckRemoteFile(&Pkt, RemoteList);
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
			Pos = Pos->Next;
		}

		if((FirstAdd == NO) && (AskNoFullPathMode() == YES))
		{
			strcpy(Pkt.Cmd, "BACKCUR");
			AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
			AddTransFileList(&Pkt);
		}

		if(RemoteList != NULL)
			DeleteFileList(&RemoteList);

		DeleteFileList(&FileListBase);

		// 同時接続対応
//		strcpy(Pkt.Cmd, "GOQUIT");
//		AddTransFileList(&Pkt);

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
	FILELIST *LocalListBase;
	FILELIST *RemoteListBase;
	FILELIST *LocalPos;
	FILELIST *RemotePos;
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

		if(Notify == YES)
			Notify = DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(mirror_up_dlg), GetMainHwnd(), MirrorNotifyCallBack, 1);
		else
			Notify = YES;

		if((Notify == YES) || (Notify == YES_LIST))
		{
			/*===== ファイルリスト取得 =====*/

			LocalListBase = NULL;
			// ファイル一覧バグ修正
//			MakeSelectedFileList(WIN_LOCAL, YES, YES, &LocalListBase, &CancelFlg);
			ListSts = MakeSelectedFileList(WIN_LOCAL, YES, YES, &LocalListBase, &CancelFlg);
			RemoteListBase = NULL;
			// ファイル一覧バグ修正
//			MakeSelectedFileList(WIN_REMOTE, YES, YES, &RemoteListBase, &CancelFlg);
			if(ListSts == FFFTP_SUCCESS)
				ListSts = MakeSelectedFileList(WIN_REMOTE, YES, YES, &RemoteListBase, &CancelFlg);

			LocalPos = LocalListBase;
			while(LocalPos != NULL)
			{
				LocalPos->Attr = YES;		/* LocalPos->Attrは転送するかどうかのフラグに使用 (YES/NO) */
				LocalPos = LocalPos->Next;
			}

			RemotePos = RemoteListBase;
			while(RemotePos != NULL)
			{
				if(AskMirrorNoTrn(RemotePos->File, 1) == NO)
				{
					RemotePos->Attr = YES;
					RemotePos = RemotePos->Next;
				}
				else
				{
					RemotePos->Attr = NO;	/* RemotePos->Attrは削除するかどうかのフラグに使用 (YES/NO) */

					if(RemotePos->Node == NODE_DIR)
					{
						Level = AskDirLevel(RemotePos->File);
						RemotePos = RemotePos->Next;
						while(RemotePos != NULL)
						{
							if((RemotePos->Node == NODE_DIR) &&
							   (AskDirLevel(RemotePos->File) <= Level))
							{
								break;
							}
							RemotePos->Attr = NO;
							RemotePos = RemotePos->Next;
						}
					}
					else
						RemotePos = RemotePos->Next;
				}
			}

			/*===== ファイルリスト比較 =====*/

			LocalPos = LocalListBase;
			while(LocalPos != NULL)
			{
				if(AskMirrorNoTrn(LocalPos->File, 0) == NO)
				{
					strcpy(Name, LocalPos->File);
					ReplaceAll(Name, '\\', '/');

					if(MirrorFnameCnv == YES)
						Mode = COMP_LOWERMATCH;
					else
						Mode = COMP_STRICT;

					if(LocalPos->Node == NODE_DIR)
					{
						if((RemotePos = SearchFileList(Name, RemoteListBase, Mode)) != NULL)
						{
							if(RemotePos->Node == NODE_DIR)
							{
								RemotePos->Attr = NO;
								LocalPos->Attr = NO;
							}
						}
					}
					else if(LocalPos->Node == NODE_FILE)
					{
						if((RemotePos = SearchFileList(Name, RemoteListBase, Mode)) != NULL)
						{
							if(RemotePos->Node == NODE_FILE)
							{
								FileTimeToLocalFileTime(&LocalPos->Time, &TmpFtimeL);
								FileTimeToLocalFileTime(&RemotePos->Time, &TmpFtimeR);
								if((RemotePos->InfoExist & FINFO_TIME) == 0)
								{
									FileTimeToSystemTime(&TmpFtimeL, &TmpStime);
									TmpStime.wHour = 0;
									TmpStime.wMinute = 0;
									TmpStime.wSecond = 0;
									TmpStime.wMilliseconds = 0;
									SystemTimeToFileTime(&TmpStime, &TmpFtimeL);

									FileTimeToSystemTime(&TmpFtimeR, &TmpStime);
									TmpStime.wHour = 0;
									TmpStime.wMinute = 0;
									TmpStime.wSecond = 0;
									TmpStime.wMilliseconds = 0;
									SystemTimeToFileTime(&TmpStime, &TmpFtimeR);
								}
								RemotePos->Attr = NO;
								if(CompareFileTime(&TmpFtimeL, &TmpFtimeR) <= 0)
									LocalPos->Attr = NO;
							}
						}
					}

					LocalPos = LocalPos->Next;
				}
				else
				{
					if(LocalPos->Node == NODE_FILE)
					{
						LocalPos->Attr = NO;
						LocalPos = LocalPos->Next;
					}
					else
					{
						LocalPos->Attr = NO;
						Level = AskDirLevel(LocalPos->File);
						LocalPos = LocalPos->Next;
						while(LocalPos != NULL)
						{
							if((LocalPos->Node == NODE_DIR) &&
							   (AskDirLevel(LocalPos->File) <= Level))
							{
								break;
							}
							LocalPos->Attr = NO;
							LocalPos = LocalPos->Next;
						}
					}
				}
			}

			DispMirrorFiles(LocalListBase, RemoteListBase);

			/*===== 削除／アップロード =====*/

			RemotePos = RemoteListBase;
			while(RemotePos != NULL)
			{
				if((RemotePos->Attr == YES) && (RemotePos->Node == NODE_FILE))
				{
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					strcat(Pkt.RemoteFile, RemotePos->File);
					ReplaceAll(Pkt.RemoteFile, '\\', '/');
					strcpy(Pkt.LocalFile, "");
					strcpy(Pkt.Cmd, "R-DELE ");
					AddTmpTransFileList(&Pkt, &Base);
				}
				RemotePos = RemotePos->Next;
			}
			MirrorDeleteAllDir(RemoteListBase, &Pkt, &Base);

			LocalPos = LocalListBase;
			while(LocalPos != NULL)
			{
				if(LocalPos->Attr == YES)
				{
					AskRemoteCurDir(Pkt.RemoteFile, FMAX_PATH);
					SetSlashTail(Pkt.RemoteFile);
					Cat = strchr(Pkt.RemoteFile, NUL);
					strcat(Pkt.RemoteFile, LocalPos->File);

					if(MirrorFnameCnv == YES)
						_mbslwr(Cat);

					ReplaceAll(Pkt.RemoteFile, '\\', '/');

					if(LocalPos->Node == NODE_DIR)
					{
						strcpy(Pkt.LocalFile, "");
						strcpy(Pkt.Cmd, "R-MKD ");
						AddTmpTransFileList(&Pkt, &Base);
					}
					else if(LocalPos->Node == NODE_FILE)
					{
						AskLocalCurDir(Pkt.LocalFile, FMAX_PATH);
						SetYenTail(Pkt.LocalFile);
						strcat(Pkt.LocalFile, LocalPos->File);
						ReplaceAll(Pkt.LocalFile, '/', '\\');

						strcpy(Pkt.Cmd, "STOR ");
						Pkt.Type = AskTransferTypeAssoc(Pkt.LocalFile, AskTransferType());
						// バグ修正
//						Pkt.Size = 0;
						Pkt.Size = LocalPos->Size;
						Pkt.Time = LocalPos->Time;
						Pkt.Attr = AskUploadFileAttr(Pkt.RemoteFile);
						Pkt.KanjiCode = AskHostKanjiCode();
						// UTF-8対応
						Pkt.KanjiCodeDesired = AskLocalKanjiCode();
						Pkt.KanaCnv = AskHostKanaCnv();
#if defined(HAVE_TANDEM)
						if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO) {
							CalcExtentSize(&Pkt, LocalPos->Size);
						}
#endif
						Pkt.Mode = EXIST_OVW;
						// ミラーリング設定追加
						Pkt.NoTransfer = MirrorNoTransferContents;
						AddTmpTransFileList(&Pkt, &Base);
					}
				}
				LocalPos = LocalPos->Next;
			}

			// ファイル一覧バグ修正
//			if((Notify == YES) ||
//			   (DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(mirror_notify_dlg), GetMainHwnd(), MirrorDispListCallBack, (LPARAM)&Base) == YES))
			if(((AbortOnListError == NO) || (ListSts == FFFTP_SUCCESS)) && ((Notify == YES) ||
			   (DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(mirror_notify_dlg), GetMainHwnd(), MirrorDispListCallBack, (LPARAM)&Base) == YES)))
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

			DeleteFileList(&LocalListBase);
			DeleteFileList(&RemoteListBase);

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

static void MirrorDeleteAllDir(FILELIST *Remote, TRANSPACKET *Pkt, TRANSPACKET **Base)
{
	while(Remote != NULL)
	{
		if(Remote->Node == NODE_DIR)
		{
			MirrorDeleteAllDir(Remote->Next, Pkt, Base);

			if(Remote->Attr == YES)
			{
				AskRemoteCurDir(Pkt->RemoteFile, FMAX_PATH);
				SetSlashTail(Pkt->RemoteFile);
				strcat(Pkt->RemoteFile, Remote->File);
				ReplaceAll(Pkt->RemoteFile, '\\', '/');
				strcpy(Pkt->LocalFile, "");
				strcpy(Pkt->Cmd, "R-RMD ");
				AddTmpTransFileList(Pkt, Base);
			}
			break;
		}
		Remote = Remote->Next;
	}
	return;
}


/*----- ミラーリングアップロード開始確認ウインドウのコールバック --------------
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
//static BOOL CALLBACK MirrorNotifyCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK MirrorNotifyCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static int Mode;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			Mode = lParam;
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case MIRRORUP_DISP :
					EndDialog(hDlg, YES_LIST);
					break;

				case IDHELP :
					if(Mode == 0)
						hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000013);
					else
						hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000012);
			}
            return(TRUE);
	}
	return(FALSE);
}


/*----- ミラーリングアップロード処理内容確認ウインドウのコールバック ----------
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
//static BOOL CALLBACK MirrorDispListCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK MirrorDispListCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static DIALOGSIZE DlgSize = {
		{ MIRROR_DEL, MIRROR_SIZEGRIP, -1 },
		{ IDOK, IDCANCEL, IDHELP, MIRROR_DEL, MIRROR_COPYNUM, MIRROR_MAKENUM, MIRROR_DELNUM, MIRROR_SIZEGRIP, -1 },
		{ MIRROR_LIST, -1 },
		{ 0, 0 },
		{ 0, 0 }
	};

	static TRANSPACKET **Base;
	TRANSPACKET *Pos;
	char Tmp[FMAX_PATH+1+6];
	int Num;
	int *List;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			Base = (TRANSPACKET **)lParam;
			Pos = *Base;
			while(Pos != NULL)
			{
				strcpy(Tmp, "");
				if((strncmp(Pos->Cmd, "R-DELE", 6) == 0) ||
				   (strncmp(Pos->Cmd, "R-RMD", 5) == 0))
					sprintf(Tmp, MSGJPN052, Pos->RemoteFile);
				else if(strncmp(Pos->Cmd, "R-MKD", 5) == 0)
					sprintf(Tmp, MSGJPN053, Pos->RemoteFile);
				else if(strncmp(Pos->Cmd, "STOR", 4) == 0)
					sprintf(Tmp, MSGJPN054, Pos->RemoteFile);
				else if((strncmp(Pos->Cmd, "L-DELE", 6) == 0) ||
						(strncmp(Pos->Cmd, "L-RMD", 5) == 0))
					sprintf(Tmp, MSGJPN055, Pos->LocalFile);
				else if(strncmp(Pos->Cmd, "L-MKD", 5) == 0)
					sprintf(Tmp, MSGJPN056, Pos->LocalFile);
				else if(strncmp(Pos->Cmd, "RETR", 4) == 0)
					sprintf(Tmp, MSGJPN057, Pos->LocalFile);

				if(strlen(Tmp) > 0)
					SendDlgItemMessage(hDlg, MIRROR_LIST, LB_ADDSTRING, 0, (LPARAM)Tmp);
				Pos = Pos->Next;
			}
			CountMirrorFiles(hDlg, *Base);
			DlgSizeInit(hDlg, &DlgSize, &MirrorDlgSize);
			EnableWindow(GetDlgItem(hDlg, MIRROR_DEL), FALSE);
			// ミラーリング設定追加
			SendDlgItemMessage(hDlg, MIRROR_NO_TRANSFER, BM_SETCHECK, MirrorNoTransferContents, 0);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					AskDlgSize(hDlg, &DlgSize, &MirrorDlgSize);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					AskDlgSize(hDlg, &DlgSize, &MirrorDlgSize);
					EndDialog(hDlg, NO);
					break;

				case MIRROR_DEL :
					Num = SendDlgItemMessage(hDlg, MIRROR_LIST, LB_GETSELCOUNT, 0, 0);
					if((List = malloc(Num * sizeof(int))) != NULL)
					{
						Num = SendDlgItemMessage(hDlg, MIRROR_LIST, LB_GETSELITEMS, Num, (LPARAM)List);
						for(Num--; Num >= 0; Num--)
						{
							if(RemoveTmpTransFileListItem(Base, List[Num]) == FFFTP_SUCCESS)
								SendDlgItemMessage(hDlg, MIRROR_LIST, LB_DELETESTRING, List[Num], 0);
							else
								MessageBeep(-1);
						}
						free(List);
						CountMirrorFiles(hDlg, *Base);
					}
					break;

				case MIRROR_LIST :
					switch(GET_WM_COMMAND_CMD(wParam, lParam))
					{
						case LBN_SELCHANGE :
							if(SendDlgItemMessage(hDlg, MIRROR_LIST, LB_GETSELCOUNT, 0, 0) > 0)
								EnableWindow(GetDlgItem(hDlg, MIRROR_DEL), TRUE);
							else
								EnableWindow(GetDlgItem(hDlg, MIRROR_DEL), FALSE);
							break;
					}
					break;

				// ミラーリング設定追加
				case MIRROR_NO_TRANSFER :
					Pos = *Base;
					while(Pos != NULL)
					{
						if(strncmp(Pos->Cmd, "STOR", 4) == 0 || strncmp(Pos->Cmd, "RETR", 4) == 0)
							Pos->NoTransfer = SendDlgItemMessage(hDlg, MIRROR_NO_TRANSFER, BM_GETCHECK, 0, 0);
						Pos = Pos->Next;
					}
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000012);
			}
            return(TRUE);

		case WM_SIZING :
			DlgSizeChange(hDlg, &DlgSize, (RECT *)lParam, (int)wParam);
		    return(TRUE);
	}
	return(FALSE);
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




/*----- ミラーリングで転送／削除しないファイルかどうかを返す ------------------
*
*	Parameter
*		char Fname : ファイル名
*		int Mode : モード
*			0=転送しないファイル, 1=削除しないファイル
*
*	Return Value
*		int ステータス
*			YES=転送・削除しない/NO
*----------------------------------------------------------------------------*/

static int AskMirrorNoTrn(char *Fname, int Mode)
{
	int Ret;
	char *Tbl;

	Tbl = MirrorNoTrn;
	if(Mode == 1)
		Tbl = MirrorNoDel;

	Ret = NO;
	if(StrMultiLen(Tbl) > 0)
	{
		Fname = GetFileName(Fname);
		while(*Tbl != NUL)
		{
			if(CheckFname(Fname, Tbl) == FFFTP_SUCCESS)
			{
				Ret = YES;
				break;
			}
			Tbl += strlen(Tbl) + 1;
		}
	}
	return(Ret);
}


/*----- アップロードするファイルの属性を返す ----------------------------------
*
*	Parameter
*		char Fname : ファイル名
*
*	Return Value
*		int 属性 (-1=設定なし)
*----------------------------------------------------------------------------*/

static int AskUploadFileAttr(char *Fname)
{
	int Ret;
	int Sts;
	char *Tbl;

	Tbl = DefAttrList;
	Fname = GetFileName(Fname);
	Ret = -1;
	while(*Tbl != NUL)
	{
		Sts = CheckFname(Fname, Tbl);
		Tbl += strlen(Tbl) + 1;

		if((Sts == FFFTP_SUCCESS) && (*Tbl != NUL))
		{
			Ret = xtoi(Tbl);
			break;
		}
		Tbl += strlen(Tbl) + 1;
	}
	return(Ret);
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

static int CheckRemoteFile(TRANSPACKET *Pkt, FILELIST *ListList)
{
	int Ret;
#if defined(HAVE_TANDEM)
	int Mode;
#endif
	FILELIST *Exist;

	Ret = EXIST_OVW;
	Pkt->ExistSize = 0;
	if(SendMode != TRANS_OVW)
	{
#if defined(HAVE_TANDEM)
		/* HP NonStop Server は大文字小文字の区別なし(すべて大文字) */
		if(AskHostType() == HTYPE_TANDEM)
			Mode = COMP_IGNORE;
		else
			Mode = COMP_STRICT;

		if((Exist = SearchFileList(GetFileName(Pkt->RemoteFile), ListList, Mode)) != NULL)
#else
		if((Exist = SearchFileList(GetFileName(Pkt->RemoteFile), ListList, COMP_STRICT)) != NULL)
#endif
		{
			Pkt->ExistSize = Exist->Size;

			if(ExistNotify == YES)
			{
				SoundPlay(SND_ERROR);
				if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(up_exist_dlg), GetMainHwnd(), UpExistDialogCallBack, (LPARAM)Pkt) == NO)
					Ret = EXIST_ABORT;
				else
					Ret = UpExistMode;
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


/*----- ホストに同じ名前のファイルがある時の確認ダイアログのコールバック ------
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
//static BOOL CALLBACK UpExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK UpExistDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static TRANSPACKET *Pkt;
	// 同じ名前のファイルの処理方法追加
//	static const RADIOBUTTON UpExistButton[] = {
//		{ UP_EXIST_OVW, EXIST_OVW },
//		{ UP_EXIST_NEW, EXIST_NEW },
//		{ UP_EXIST_RESUME, EXIST_RESUME },
//		{ UP_EXIST_UNIQUE, EXIST_UNIQUE },
//		{ UP_EXIST_IGNORE, EXIST_IGNORE }
//	};
	static const RADIOBUTTON UpExistButton[] = {
		{ UP_EXIST_OVW, EXIST_OVW },
		{ UP_EXIST_NEW, EXIST_NEW },
		{ UP_EXIST_RESUME, EXIST_RESUME },
		{ UP_EXIST_UNIQUE, EXIST_UNIQUE },
		{ UP_EXIST_IGNORE, EXIST_IGNORE },
		{ UP_EXIST_LARGE, EXIST_LARGE }
	};
	#define UPEXISTBUTTONS	(sizeof(UpExistButton)/sizeof(RADIOBUTTON))

	switch (iMessage)
	{
		case WM_INITDIALOG :
			Pkt = (TRANSPACKET *)lParam;
			SendDlgItemMessage(hDlg, UP_EXIST_NAME, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, UP_EXIST_NAME, WM_SETTEXT, 0, (LPARAM)Pkt->RemoteFile);

			if((Pkt->Type == TYPE_A) || (Pkt->ExistSize <= 0))
				EnableWindow(GetDlgItem(hDlg, UP_EXIST_RESUME), FALSE);

			SetRadioButtonByValue(hDlg, UpExistMode, UpExistButton, UPEXISTBUTTONS);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK_ALL :
					ExistNotify = NO;
					/* ここに break はない */

				case IDOK :
					UpExistMode = AskRadioButtonValue(hDlg, UpExistButton, UPEXISTBUTTONS);
					SendDlgItemMessage(hDlg, UP_EXIST_NAME, WM_GETTEXT, FMAX_PATH, (LPARAM)Pkt->RemoteFile);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
//					Pkt->Abort = ABORT_USER;
//					UpExistMode = EXIST_IGNORE;
					EndDialog(hDlg, NO);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000011);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


/*----- アップロード／ダウンロードファイル名入力ダイアログのコールバック ------
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
//static BOOL CALLBACK UpDownAsDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK UpDownAsDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			if(CurWin == WIN_LOCAL)
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN064);
			else
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN065);

			SendDlgItemMessage(hDlg, UPDOWNAS_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, UPDOWNAS_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpString);
					EndDialog(hDlg, YES);
					break;

				case UPDOWNAS_STOP :
					EndDialog(hDlg, NO_ALL);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


#if defined(HAVE_TANDEM)
/*----- アップロード／ダウンロードファイル名入力ダイアログのコールバック ------
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

static INT_PTR CALLBACK UpDownAsWithExtDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			if(CurWin == WIN_LOCAL)
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN064);
			else
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN065);

			SendDlgItemMessage(hDlg, UPDOWNAS_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, UPDOWNAS_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, UPDOWNAS_FILECODE, EM_LIMITTEXT, 4, 0);
			SendDlgItemMessage(hDlg, UPDOWNAS_FILECODE, WM_SETTEXT, 0, (LPARAM)TmpFileCode);

			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, UPDOWNAS_NEW, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpString);
					SendDlgItemMessage(hDlg, UPDOWNAS_FILECODE, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpFileCode);
					EndDialog(hDlg, YES);
					break;

				case UPDOWNAS_STOP :
					EndDialog(hDlg, NO_ALL);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}
#endif


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
	FILELIST *FileListBase;
	FILELIST *Pos;
	int DelFlg;
	int Sts;
	char CurDir[FMAX_PATH+1];
	char Tmp[FMAX_PATH+1];

	// 同時接続対応
	CancelFlg = NO;

	// デッドロック対策
	DisableUserOpe();
	Sts = YES;
	AskRemoteCurDir(CurDir, FMAX_PATH);
	FileListBase = NULL;
	if(GetFocus() == GetLocalHwnd())
	{
		Win = WIN_LOCAL;
		MakeSelectedFileList(Win, NO, NO, &FileListBase, &CancelFlg);
	}
	else
	{
		Win = WIN_REMOTE;
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
			MakeSelectedFileList(Win, YES, NO, &FileListBase, &CancelFlg);
		else
			Sts = NO;
	}

	if(Sts == YES)
	{
		// デッドロック対策
//		DisableUserOpe();

		DelFlg = NO;
		Sts = NO;
		Pos = FileListBase;
		while(Pos != NULL)
		{
			if(Pos->Node == NODE_FILE)
			{
				DelNotifyAndDo(Pos, Win, &Sts, &DelFlg, CurDir);
				if(Sts == NO_ALL)
					break;
			}
			Pos = Pos->Next;
		}

		if(Sts != NO_ALL)
			DeleteAllDir(FileListBase, Win, &Sts, &DelFlg, CurDir);

		if(Win == WIN_REMOTE)
		{
			AskRemoteCurDir(Tmp, FMAX_PATH);
			if(strcmp(Tmp, CurDir) != 0)
				DoCWD(Tmp, NO, NO, NO);
		}

		DeleteFileList(&FileListBase);

		if(DelFlg == YES)
		{
			if(Win == WIN_LOCAL)
				GetLocalDirForWnd();
			else
				GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
		}

		// デッドロック対策
//		EnableUserOpe();
	}
	// デッドロック対策
	EnableUserOpe();
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

static void DeleteAllDir(FILELIST *Dt, int Win, int *Sw, int *Flg, char *CurDir)
{
	while(Dt != NULL)
	{
		if(Dt->Node == NODE_DIR)
		{
			DeleteAllDir(Dt->Next, Win, Sw, Flg, CurDir);
			if(*Sw == NO_ALL)
				break;

			DelNotifyAndDo(Dt, Win, Sw, Flg, CurDir);
			break;
		}
		Dt = Dt->Next;
	}
	return;
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

static void DelNotifyAndDo(FILELIST *Dt, int Win, int *Sw, int *Flg, char *CurDir)
{
	char Path[FMAX_PATH+1];

	if(Win == WIN_LOCAL)
	{
		AskLocalCurDir(Path, FMAX_PATH);
		SetYenTail(Path);
		strcat(Path, Dt->File);
		ReplaceAll(Path, '/', '\\');
	}
	else
	{
		AskRemoteCurDir(Path, FMAX_PATH);
		SetSlashTail(Path);
		strcat(Path, Dt->File);
		ReplaceAll(Path, '\\', '/');
	}

	if(*Sw != YES_ALL)
	{
		sprintf(TmpString, "%s", Path);

		// ローカルのファイルのパスの最後の'\\'が消えるバグ修正
//		if(AskHostType() == HTYPE_VMS)
		if(Win == WIN_REMOTE && AskHostType() == HTYPE_VMS)
			ReformToVMSstylePathName(TmpString);

		CurWin = Win;
		*Sw = DialogBox(GetFtpInst(), MAKEINTRESOURCE(delete_dlg), GetMainHwnd(), DeleteDialogCallBack);
	}

	if((*Sw == YES) || (*Sw == YES_ALL))
	{
		if(Win == WIN_LOCAL)
		{
			if(Dt->Node == NODE_FILE)
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
				if(Dt->Node == NODE_FILE)
					DoDELE(Path);
				else
					DoRMD(Path);
				*Flg = YES;
			}
		}
	}
	return;
}


/*----- ファイル削除ダイアログのコールバック ----------------------------------
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
//static BOOL CALLBACK DeleteDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK DeleteDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			if(CurWin == WIN_LOCAL)
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN066);
			else
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN067);
			SendDlgItemMessage(hDlg, DELETE_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					EndDialog(hDlg, YES);
					break;

				case DELETE_NO :
					EndDialog(hDlg, NO);
					break;

				case DELETE_ALL :
					EndDialog(hDlg, YES_ALL);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO_ALL);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


/*----- ファイル一覧で指定されたファイルの名前を変更する ----------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RenameProc(void)
{
	int Win;
	FILELIST *FileListBase;
	FILELIST *Pos;
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

		FileListBase = NULL;
		MakeSelectedFileList(Win, NO, NO, &FileListBase, &CancelFlg);

		RenFlg = NO;
		Sts = NO;
		Pos = FileListBase;
		while(Pos != NULL)
		{
			if((Pos->Node == NODE_FILE) || (Pos->Node == NODE_DIR))
			{
				strcpy(TmpString, Pos->File);
				CurWin = Win;
				Sts = DialogBox(GetFtpInst(), MAKEINTRESOURCE(rename_dlg), GetMainHwnd(), RenameDialogCallBack);

				if(Sts == NO_ALL)
					break;

				if((Sts == YES) && (strlen(TmpString) != 0))
				{
					strcpy(New, TmpString);
					if(Win == WIN_LOCAL)
						DoLocalRENAME(Pos->File, New);
					else
						DoRENAME(Pos->File, New);
					RenFlg = YES;
				}
			}
			Pos = Pos->Next;
		}
		DeleteFileList(&FileListBase);

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
	int Win;
	FILELIST *FileListBase;
	FILELIST *Pos;
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

	if(MoveMode == MOVE_DLG)
	{
		if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(move_notify_dlg), GetRemoteHwnd(), ExeEscTextDialogProc, (LPARAM)Pkt.File) == NO)
		{
			return;
		}
	}

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

		FileListBase = NULL;
		MakeSelectedFileList(Win, NO, NO, &FileListBase, &CancelFlg);

		RenFlg = NO;
		Sts = NO;
		Pos = FileListBase;
		while(Pos != NULL)
		{
			if((Pos->Node == NODE_FILE) || (Pos->Node == NODE_DIR))
			{
				strcpy(TmpString, Pos->File);
				CurWin = Win;
#if 0
				Sts = DialogBox(GetFtpInst(), MAKEINTRESOURCE(rename_dlg), GetMainHwnd(), RenameDialogCallBack);

				if(Sts == NO_ALL)
					break;
#else
				Sts = YES;
#endif

				if((Sts == YES) && (strlen(TmpString) != 0))
				{
					// パスの設定(local)
					strncpy_s(Old, sizeof(Old), HostDir, _TRUNCATE);
					strncat_s(Old, sizeof(Old), "/", _TRUNCATE);
					strncat_s(Old, sizeof(Old), Pos->File, _TRUNCATE);

					// パスの設定(remote)
					strncpy_s(New, sizeof(New), HostDir, _TRUNCATE);
					strncat_s(New, sizeof(New), "/", _TRUNCATE);
					strncat_s(New, sizeof(New), Pkt.File, _TRUNCATE);
					strncat_s(New, sizeof(New), "/", _TRUNCATE);
					strncat_s(New, sizeof(New), Pos->File, _TRUNCATE);

					if(Win == WIN_LOCAL)
						DoLocalRENAME(Old, New);
					else
						DoRENAME(Old, New);
					RenFlg = YES;
				}
			}
			Pos = Pos->Next;
		}
		DeleteFileList(&FileListBase);

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



/*----- 新ファイル名入力ダイアログのコールバック ------------------------------
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
//static BOOL CALLBACK RenameDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK RenameDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			if(CurWin == WIN_LOCAL)
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN068);
			else
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN069);
			SendDlgItemMessage(hDlg, RENAME_NEW, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, RENAME_NEW, WM_SETTEXT, 0, (LPARAM)TmpString);
			SendDlgItemMessage(hDlg, RENAME_TEXT, WM_SETTEXT, 0, (LPARAM)TmpString);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, RENAME_NEW, WM_GETTEXT, FMAX_PATH, (LPARAM)TmpString);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case RENAME_STOP :
					EndDialog(hDlg, NO_ALL);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
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
	int Sts;
	int Win;
	char Path[FMAX_PATH+1];
	char *Title;
	int Tmp;

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

	strcpy(Path, "");
	Sts = InputDialogBox(mkdir_dlg, GetMainHwnd(), Title, Path, FMAX_PATH+1, &Tmp, IDH_HELP_TOPIC_0000001);

	if((Sts == YES) && (strlen(Path) != 0))
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

	if((i = SendMessage(hWnd, CB_GETCURSEL, 0, 0)) != CB_ERR)
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
	char Local[FMAX_PATH+1];
	char Remote[FMAX_PATH+1];
	int Sts;

	// 同時接続対応
	CancelFlg = NO;

	Sts = AskBookMarkText(MarkID, Local, Remote, FMAX_PATH+1);
	if((Sts == BMARK_TYPE_LOCAL) || (Sts == BMARK_TYPE_BOTH))
	{
		DisableUserOpe();
		if(DoLocalCWD(Local) == FFFTP_SUCCESS)
			GetLocalDirForWnd();
		EnableUserOpe();
	}

	if((Sts == BMARK_TYPE_REMOTE) || (Sts == BMARK_TYPE_BOTH))
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			if(DoCWD(Remote, YES, NO, YES) < FTP_RETRY)
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
	int Sts;
	char Path[FMAX_PATH+1];
	char *Title;
	int Tmp;

	// 同時接続対応
	CancelFlg = NO;

	if(Win == WIN_LOCAL)
		Title = MSGJPN072;
	else
		Title = MSGJPN073;

	strcpy(Path, "");
	if(Win == WIN_LOCAL)
	// フォルダ選択ダイアログを直接表示
//		Sts = InputDialogBox(chdir_br_dlg, GetMainHwnd(), Title, Path, FMAX_PATH+1, &Tmp, IDH_HELP_TOPIC_0000001);
	{
		if(SelectDir(GetMainHwnd(), Path, FMAX_PATH) == TRUE)
			Sts = YES;
	}
	else
		Sts = InputDialogBox(chdir_dlg, GetMainHwnd(), Title, Path, FMAX_PATH+1, &Tmp, IDH_HELP_TOPIC_0000001);

	if((Sts == YES) && (strlen(Path) != 0))
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
	FILELIST *FileListBase;
	FILELIST *Pos;
	char Tmp[5];
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
			FileListBase = NULL;
			MakeSelectedFileList(WIN_REMOTE, NO, NO, &FileListBase, &CancelFlg);
			if(FileListBase != NULL)
			{
				sprintf(Tmp, "%03X", FileListBase->Attr);
				if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(chmod_dlg), GetMainHwnd(), ChmodDialogCallBack, (LPARAM)Tmp) == YES)
				{
					ChmodFlg = NO;
					Pos = FileListBase;
					while(Pos != NULL)
					{
						if((Pos->Node == NODE_FILE) || (Pos->Node == NODE_DIR))
						{
							DoCHMOD(Pos->File, Tmp);
							ChmodFlg = YES;
						}
						Pos = Pos->Next;
					}
					if(ChmodFlg == YES)
						GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
				}
			}
			DeleteFileList(&FileListBase);
			EnableUserOpe();
		}
	}
	else if(GetFocus() == GetLocalHwnd())
	{
		DisableUserOpe();
		FileListBase = NULL;
		MakeSelectedFileList(WIN_LOCAL, NO, NO, &FileListBase, &CancelFlg);
		if(FileListBase != NULL)
		{
			if((Buf = malloc(1)) != NULL)
			{
				*Buf = NUL;
				BufLen = 0;
				Pos = FileListBase;
				while(Pos != NULL)
				{
					if((BufTmp = realloc(Buf, BufLen + strlen(Pos->File) + 2)) != NULL)
					{
						Buf = BufTmp;
						strcpy(Buf+BufLen, Pos->File);
						BufLen += strlen(Pos->File) + 1;
					}
					Pos = Pos->Next;
				}

				memset(Buf+BufLen, NUL, 1);
				DispFileProperty(Buf);
				free(Buf);
			}
		}
		DeleteFileList(&FileListBase);
		EnableUserOpe();
	}
	return;
}


/*----- 属性変更ダイアログのコールバック --------------------------------------
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
//BOOL CALLBACK ChmodDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
INT_PTR CALLBACK ChmodDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	char Str[5];
	static char *Buf;
	int Tmp;

	switch (iMessage)
	{
		case WM_INITDIALOG :
			Buf = (char *)lParam;
			SendDlgItemMessage(hDlg, PERM_NOW, EM_LIMITTEXT, 4, 0);
			SendDlgItemMessage(hDlg, PERM_NOW, WM_SETTEXT, 0, (LPARAM)Buf);
			SetAttrToDialog(hDlg, xtoi(Buf));
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, PERM_NOW, WM_GETTEXT, 5, (LPARAM)Buf);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case PERM_O_READ :
				case PERM_O_WRITE :
				case PERM_O_EXEC :
				case PERM_G_READ :
				case PERM_G_WRITE :
				case PERM_G_EXEC :
				case PERM_A_READ :
				case PERM_A_WRITE :
				case PERM_A_EXEC :
					Tmp = GetAttrFromDialog(hDlg);
					sprintf(Str, "%03X", Tmp);
					SendDlgItemMessage(hDlg, PERM_NOW, WM_SETTEXT, 0, (LPARAM)Str);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000017);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
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
	int Tmp;
	FILELIST *FileListBase;

	// 同時接続対応
	CancelFlg = NO;

	if(GetFocus() == GetRemoteHwnd())
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
			DisableUserOpe();
			FileListBase = NULL;
			MakeSelectedFileList(WIN_REMOTE, NO, NO, &FileListBase, &CancelFlg);
			memset(Cmd, NUL, 81);
			if(FileListBase != NULL)
			{
				strncpy(Cmd, FileListBase->File, 80);
			}
			DeleteFileList(&FileListBase);

			if(InputDialogBox(somecmd_dlg, GetMainHwnd(), NULL, Cmd, 81, &Tmp, IDH_HELP_TOPIC_0000023) == YES)
			{
				// 同時接続対応
				//DoQUOTE(Cmd);
				DoQUOTE(AskCmdCtrlSkt(), Cmd, &CancelFlg);
			}
			EnableUserOpe();
		}
	}
	return;
}




/*----- ファイル総容量の計算を行う --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void CalcFileSizeProc(void)
{
	FILELIST *ListBase;
	FILELIST *Pos;
	int Win;
	int All;
	int Sts;

	// 同時接続対応
	CancelFlg = NO;

	if((All = DialogBox(GetFtpInst(), MAKEINTRESOURCE(filesize_notify_dlg), GetMainHwnd(), SizeNotifyDlgWndProc)) != NO_ALL)
	{
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
			ListBase = NULL;
			MakeSelectedFileList(Win, YES, All, &ListBase, &CancelFlg);

			FileSize = 0;
			Pos = ListBase;
			while(Pos != NULL)
			{
				if(Pos->Node != NODE_DIR)
					FileSize += Pos->Size;
				Pos = Pos->Next;
			}
			DeleteFileList(&ListBase);
			DialogBox(GetFtpInst(), MAKEINTRESOURCE(filesize_dlg), GetMainHwnd(), SizeDlgWndProc);
		}
	}
	return;
}


/*----- ファイル容量検索確認ダイアログのコールバック --------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		UINT message  : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		メッセージに対応する戻り値
*----------------------------------------------------------------------------*/

static LRESULT CALLBACK SizeNotifyDlgWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
			if(GetFocus() == GetLocalHwnd())
				SendDlgItemMessage(hDlg, FSNOTIFY_TITLE, WM_SETTEXT, 0, (LPARAM)MSGJPN074);
			else
				SendDlgItemMessage(hDlg, FSNOTIFY_TITLE, WM_SETTEXT, 0, (LPARAM)MSGJPN075);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					if(SendDlgItemMessage(hDlg, FSNOTIFY_SEL_ONLY, BM_GETCHECK, 0, 0) == 1)
						EndDialog(hDlg, NO);
					else
						EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO_ALL);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ファイル容量検索ダイアログのコールバック ------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		UINT message  : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		メッセージに対応する戻り値
*----------------------------------------------------------------------------*/

static LRESULT CALLBACK SizeDlgWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tmp[FMAX_PATH+1];

	switch (message)
	{
		case WM_INITDIALOG :
			if(GetFocus() == GetLocalHwnd())
				SendDlgItemMessage(hDlg, FSIZE_TITLE, WM_SETTEXT, 0, (LPARAM)MSGJPN076);
			else
				SendDlgItemMessage(hDlg, FSIZE_TITLE, WM_SETTEXT, 0, (LPARAM)MSGJPN077);

			MakeSizeString(FileSize, Tmp);
			SendDlgItemMessage(hDlg, FSIZE_SIZE, WM_SETTEXT, 0, (LPARAM)Tmp);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
				case IDCANCEL :
					EndDialog(hDlg, YES);
					break;

			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ディレクトリ移動失敗時のエラーを表示 ----------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispCWDerror(HWND hWnd)
{
	DialogBox(GetFtpInst(), MAKEINTRESOURCE(cwderr_dlg), hWnd, ExeEscDialogProc);
	return;
}


/*----- URLをクリップボードにコピー -------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void CopyURLtoClipBoard(void)
{
	FILELIST *FileListBase;
	FILELIST *Pos;
	char *Buf;
	char Path[FMAX_PATH+1];
	char Host[HOST_ADRS_LEN+1];
	char Port[10];
	int Set;
	int Total;

	if(GetFocus() == GetRemoteHwnd())
	{
		FileListBase = NULL;
		MakeSelectedFileList(WIN_REMOTE, NO, NO, &FileListBase, &CancelFlg);
		if(FileListBase != NULL)
		{
			strcpy(Host, AskHostAdrs());
			Total = 0;
			Buf = NULL;
			Pos = FileListBase;
			while(Pos != NULL)
			{
				AskRemoteCurDir(Path, FMAX_PATH);
				SetSlashTail(Path);
				strcat(Path, Pos->File);

				if(AskHostType() == HTYPE_VMS)
					ReformToVMSstylePathName(Path);

				strcpy(Port, "");
				if(AskHostPort() != PORT_NOR)
					sprintf(Port, ":%d", AskHostPort());

				Set = Total;
				Total += strlen(Path) + strlen(Host) + strlen(Port) + 8;	/* 8は "ftp://\r\n" のぶん */
				if(AskHostType() == HTYPE_VMS)
					Total++;

				if((Buf = realloc(Buf, Total+1)) == NULL)
					break;

				if(AskHostType() != HTYPE_VMS)
					sprintf(Buf + Set, "ftp://%s%s%s\r\n", Host, Port, Path);
				else
					sprintf(Buf + Set, "ftp://%s%s/%s\r\n", Host, Port, Path);

				Pos = Pos->Next;
			}

			if(Buf != NULL)
			{
				CopyStrToClipBoard(Buf);
				free(Buf);
			}
		}
		DeleteFileList(&FileListBase);
	}
	return;
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
			// 同時接続対応
//			if(Type == 0)
//				Cmd = CommandProcCmd(NULL, "CWD %s", Tmp);
//			else
//				Cmd = CommandProcTrn(NULL, "CWD %s", Tmp);
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
	int Tmp;
	int Ret;

	Ret = FFFTP_SUCCESS;
	while(1)
	{
		if((_mbschr(Fname, ':') != NULL) ||
		   (_mbschr(Fname, '*') != NULL) ||
		   (_mbschr(Fname, '?') != NULL) ||
		   (_mbschr(Fname, '<') != NULL) ||
		   (_mbschr(Fname, '>') != NULL) ||
		   (_mbschr(Fname, '|') != NULL) ||
		   (_mbschr(Fname, '\x22') != NULL) ||
		   (_mbschr(Fname, '\\') != NULL))
		{
			if(InputDialogBox(forcerename_dlg, GetMainHwnd(), NULL, Fname, FMAX_PATH+1, &Tmp, IDH_HELP_TOPIC_0000001) == NO)
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
	int CancelCheckWork;
	CancelCheckWork = NO;
	if(Force == YES || (AskConnecting() == YES && AskUserOpeDisabled() == NO))
	{
		if(AskReuseCmdSkt() == NO || AskShareProh() == YES || AskTransferNow() == NO)
		{
			DisableUserOpe();
			DoDirListCmdSkt("", "", 999, &CancelCheckWork);
			EnableUserOpe();
		}
	}
}

// 同時接続対応
void AbortRecoveryProc(void)
{
	CancelFlg = NO;
	if(AskConnecting() == YES && AskUserOpeDisabled() == NO)
	{
		if(AskReuseCmdSkt() == NO || AskShareProh() == YES || AskTransferNow() == NO)
		{
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

