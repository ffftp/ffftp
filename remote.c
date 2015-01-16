/*=============================================================================
*
*							リモート側のファイル操作
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

/* このソースは一部、WS_FTP Version 93.12.05 のソースを参考にしました。 */

#define	STRICT
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
// IPv6対応
//#include <winsock.h>
#include <winsock2.h>
#include <windowsx.h>
#include <commctrl.h>

#include "common.h"
#include "resource.h"

#define PWD_XPWD		0
#define PWD_PWD			1

/*===== プロトタイプ =====*/

static int DoPWD(char *Buf);
static int ReadOneLine(SOCKET cSkt, char *Buf, int Max, int *CancelCheckWork);
static int DoDirList(HWND hWnd, SOCKET cSkt, char *AddOpt, char *Path, int Num, int *CancelCheckWork);
static void ChangeSepaLocal2Remote(char *Fname);
static void ChangeSepaRemote2Local(char *Fname);

/*===== 外部参照 =====*/

extern TRANSPACKET MainTransPkt;

/* 設定値 */
extern int TimeOut;
extern int SendQuit;

// 同時接続対応
extern int CancelFlg;
// 自動切断対策
extern time_t LastDataConnectionTime;

/*===== ローカルなワーク =====*/

static int PwdCommandType;

// 同時接続対応
//static int CheckCancelFlg = NO;



/*----- リモート側のカレントディレクトリ変更 ----------------------------------
*
*	Parameter
*		char *Path : パス名
*		int Disp : ディレクトリリストにパス名を表示するかどうか(YES/NO)
*		int ForceGet : 失敗してもカレントディレクトリを取得する
*		int ErrorBell : エラー事の音を鳴らすかどうか(YES/NO)
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoCWD(char *Path, int Disp, int ForceGet, int ErrorBell)
{
	int Sts;
	char Buf[FMAX_PATH+1];

	Sts = FTP_COMPLETE * 100;

	if(strcmp(Path, "..") == 0)
		// 同時接続対応
//		Sts = CommandProcCmd(NULL, "CDUP");
		Sts = CommandProcCmd(NULL, &CancelFlg, "CDUP");
	else if(strcmp(Path, "") != 0)
	{
		if((AskHostType() != HTYPE_VMS) || (strchr(Path, '[') != NULL))
			// 同時接続対応
//			Sts = CommandProcCmd(NULL, "CWD %s", Path);
			Sts = CommandProcCmd(NULL, &CancelFlg, "CWD %s", Path);
		else
			// 同時接続対応
//			Sts = CommandProcCmd(NULL, "CWD [.%s]", Path);	/* VMS用 */
			Sts = CommandProcCmd(NULL, &CancelFlg, "CWD [.%s]", Path);	/* VMS用 */
	}

	if((Sts/100 >= FTP_CONTINUE) && (ErrorBell == YES))
		SoundPlay(SND_ERROR);

	if((Sts/100 == FTP_COMPLETE) ||
	   (ForceGet == YES))
	{
		if(Disp == YES)
		{
			if(DoPWD(Buf) != FTP_COMPLETE)
			{
				/*===== PWDが使えなかった場合 =====*/

				if(*Path == '/')
					strcpy(Buf, Path);
				else
				{
					AskRemoteCurDir(Buf, FMAX_PATH);
					if(strlen(Buf) == 0)
						strcpy(Buf, "/");

					while(*Path != NUL)
					{
						if(strcmp(Path, ".") == 0)
							Path++;
						else if(strncmp(Path, "./", 2) == 0)
							Path += 2;
						else if(strcmp(Path, "..") == 0)
						{
							GetUpperDir(Buf);
							Path += 2;
						}
						else if(strncmp(Path, "../", 2) == 0)
						{
							GetUpperDir(Buf);
							Path += 3;
						}
						else
						{
							SetSlashTail(Buf);
							strcat(Buf, Path);
							break;
						}
					}
				}
			}
			SetRemoteDirHist(Buf);
		}
	}
	return(Sts/100);
}




/*----- リモート側のカレントディレクトリ変更（その２）-------------------------
*
*	Parameter
*		char *Path : パス名
*		char *Cur : カレントディレクトリ
*
*	Return Value
*		int 応答コードの１桁目
*
*	Note
*		パス名は "xxx/yyy/zzz" の形式
*		ディレクトリ変更が失敗したら、カレントディレクトリに戻しておく
*----------------------------------------------------------------------------*/

int DoCWDStepByStep(char *Path, char *Cur)
{
	int Sts;
	char *Set;
	char *Set2;
	char Tmp[FMAX_PATH+2];

	Sts = FTP_COMPLETE;

	memset(Tmp, NUL, FMAX_PATH+2);
	strcpy(Tmp, Path);
	Set = Tmp;
	while(*Set != NUL)
	{
		if((Set2 = strchr(Set, '/')) != NULL)
			*Set2 = NUL;
		if((Sts = DoCWD(Set, NO, NO, NO)) != FTP_COMPLETE)
			break;
		if(Set2 == NULL)
			break;
		Set = Set2 + 1;
	}

	if(Sts != FTP_COMPLETE)
		DoCWD(Cur, NO, NO, NO);

	return(Sts);
}


/*----- リモート側のカレントディレクトリ取得 ----------------------------------
*
*	Parameter
*		char *Buf : パス名を返すバッファ
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

static int DoPWD(char *Buf)
{
	char *Pos;
	char Tmp[1024];
	int Sts;

	if(PwdCommandType == PWD_XPWD)
	{
		// 同時接続対応
//		Sts = CommandProcCmd(Tmp, "XPWD");
		Sts = CommandProcCmd(Tmp, &CancelFlg, "XPWD");
		if(Sts/100 != FTP_COMPLETE)
			PwdCommandType = PWD_PWD;
	}
	if(PwdCommandType == PWD_PWD)
		// 同時接続対応
//		Sts = CommandProcCmd(Tmp, "PWD");
		Sts = CommandProcCmd(Tmp, &CancelFlg, "PWD");

	if(Sts/100 == FTP_COMPLETE)
	{
		if((Pos = strchr(Tmp, '"')) != NULL)
		{
			memmove(Tmp, Pos+1, strlen(Pos+1)+1);
			if((Pos = strchr(Tmp, '"')) != NULL)
				*Pos = NUL;
		}
		else
			memmove(Tmp, Tmp+4, strlen(Tmp+4)+1);

		if(strlen(Tmp) < FMAX_PATH)
		{
			strcpy(Buf, Tmp);
			ReplaceAll(Buf, '\\', '/');
			ChangeSepaRemote2Local(Buf);
		}
		else
			Sts = FTP_ERROR*100;
	}
	return(Sts/100);
}


/*----- PWDコマンドのタイプを初期化する ---------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void InitPWDcommand()
{
	PwdCommandType = PWD_XPWD;
}


/*----- リモート側のディレクトリ作成 ----------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoMKD(char *Path)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "MKD %s", Path);
	Sts = CommandProcCmd(NULL, &CancelFlg, "MKD %s", Path);

	if(Sts/100 >= FTP_CONTINUE)
		SoundPlay(SND_ERROR);

	// 自動切断対策
	if(CancelFlg == NO && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
	{
		NoopProc(YES);
		LastDataConnectionTime = time(NULL);
	}

	return(Sts/100);
}


/*----- リモート側のディレクトリ削除 ------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoRMD(char *Path)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "RMD %s", Path);
	Sts = CommandProcCmd(NULL, &CancelFlg, "RMD %s", Path);

	if(Sts/100 >= FTP_CONTINUE)
		SoundPlay(SND_ERROR);

	// 自動切断対策
	if(CancelFlg == NO && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
	{
		NoopProc(YES);
		LastDataConnectionTime = time(NULL);
	}

	return(Sts/100);
}


/*----- リモート側のファイル削除 ----------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoDELE(char *Path)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "DELE %s", Path);
	Sts = CommandProcCmd(NULL, &CancelFlg, "DELE %s", Path);

	if(Sts/100 >= FTP_CONTINUE)
		SoundPlay(SND_ERROR);

	// 自動切断対策
	if(CancelFlg == NO && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
	{
		NoopProc(YES);
		LastDataConnectionTime = time(NULL);
	}

	return(Sts/100);
}


/*----- リモート側のファイル名変更 --------------------------------------------
*
*	Parameter
*		char *Src : 元ファイル名
*		char *Dst : 変更後のファイル名
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoRENAME(char *Src, char *Dst)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "RNFR %s", Src);
	Sts = CommandProcCmd(NULL, &CancelFlg, "RNFR %s", Src);
	if(Sts == 350)
		// 同時接続対応
//		Sts = command(AskCmdCtrlSkt(), NULL, &CheckCancelFlg, "RNTO %s", Dst);
		Sts = command(AskCmdCtrlSkt(), NULL, &CancelFlg, "RNTO %s", Dst);

	if(Sts/100 >= FTP_CONTINUE)
		SoundPlay(SND_ERROR);

	// 自動切断対策
	if(CancelFlg == NO && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
	{
		NoopProc(YES);
		LastDataConnectionTime = time(NULL);
	}

	return(Sts/100);
}


/*----- リモート側のファイルの属性変更 ----------------------------------------
*
*	Parameter
*		char *Path : パス名
*		char *Mode : モード文字列
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoCHMOD(char *Path, char *Mode)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "%s %s %s", AskHostChmodCmd(), Mode, Path);
	Sts = CommandProcCmd(NULL, &CancelFlg, "%s %s %s", AskHostChmodCmd(), Mode, Path);

	if(Sts/100 >= FTP_CONTINUE)
		SoundPlay(SND_ERROR);

	// 自動切断対策
	if(CancelFlg == NO && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
	{
		NoopProc(YES);
		LastDataConnectionTime = time(NULL);
	}

	return(Sts/100);
}


/*----- リモート側のファイルのサイズを取得（転送ソケット使用）-----------------
*
*	Parameter
*		char *Path : パス名
*		LONGLONG *Size : ファイルのサイズを返すワーク
*
*	Return Value
*		int 応答コードの１桁目
*
*	Note
*		★★転送ソケットを使用する★★
*		サイズが選られない時は Size = -1 を返す
*----------------------------------------------------------------------------*/

// 同時接続対応
//int DoSIZE(char *Path, LONGLONG *Size)
int DoSIZE(SOCKET cSkt, char *Path, LONGLONG *Size, int *CancelCheckWork)
{
	int Sts;
	char Tmp[1024];

	// 同時接続対応
//	Sts = CommandProcTrn(Tmp, "SIZE %s", Path);
	Sts = CommandProcTrn(cSkt, Tmp, CancelCheckWork, "SIZE %s", Path);

	*Size = -1;
	if((Sts/100 == FTP_COMPLETE) && (strlen(Tmp) > 4) && IsDigit(Tmp[4]))
		*Size = _atoi64(&Tmp[4]);

	return(Sts/100);
}


/*----- リモート側のファイルの日付を取得（転送ソケット使用）-------------------
*
*	Parameter
*		char *Path : パス名
*		FILETIME *Time : 日付を返すワーク
*
*	Return Value
*		int 応答コードの１桁目
*
*	Note
*		★★転送ソケットを使用する★★
*		日付が選られない時は Time = 0 を返す
*----------------------------------------------------------------------------*/

// 同時接続対応
//int DoMDTM(char *Path, FILETIME *Time)
int DoMDTM(SOCKET cSkt, char *Path, FILETIME *Time, int *CancelCheckWork)
{
	int Sts;
	char Tmp[1024];
	SYSTEMTIME sTime;

    Time->dwLowDateTime = 0;
    Time->dwHighDateTime = 0;

	// 同時接続対応
	// ホスト側の日時取得
//	Sts = CommandProcTrn(Tmp, "MDTM %s", Path);
	Sts = 500;
	if(AskHostFeature() & FEATURE_MDTM)
		Sts = CommandProcTrn(cSkt, Tmp, CancelCheckWork, "MDTM %s", Path);
	if(Sts/100 == FTP_COMPLETE)
	{
		sTime.wMilliseconds = 0;
		if(sscanf(Tmp+4, "%04d%02d%02d%02d%02d%02d",
			&sTime.wYear, &sTime.wMonth, &sTime.wDay,
			&sTime.wHour, &sTime.wMinute, &sTime.wSecond) == 6)
		{
			SystemTimeToFileTime(&sTime, Time);
			// 時刻はGMT
//			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

		}
	}
	return(Sts/100);
}


// ホスト側の日時設定
int DoMFMT(SOCKET cSkt, char *Path, FILETIME *Time, int *CancelCheckWork)
{
	int Sts;
	char Tmp[1024];
	SYSTEMTIME sTime;

	FileTimeToSystemTime(Time, &sTime);

	Sts = 500;
	if(AskHostFeature() & FEATURE_MFMT)
		Sts = CommandProcTrn(cSkt, Tmp, CancelCheckWork, "MFMT %04d%02d%02d%02d%02d%02d %s", sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond, Path);
	return(Sts/100);
}


/*----- リモート側のコマンドを実行 --------------------------------------------
*
*	Parameter
*		char *CmdStr : コマンド文字列
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

// 同時接続対応
//int DoQUOTE(char *CmdStr)
int DoQUOTE(SOCKET cSkt, char *CmdStr, int *CancelCheckWork)
{
	int Sts;

//	Sts = CommandProcCmd(NULL, "%s", CmdStr);
	Sts = CommandProcTrn(cSkt, NULL, CancelCheckWork, "%s", CmdStr);

	if(Sts/100 >= FTP_CONTINUE)
		SoundPlay(SND_ERROR);

	return(Sts/100);
}


/*----- ソケットを閉じる ------------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		SOCKET 閉じた後のソケット
*----------------------------------------------------------------------------*/

SOCKET DoClose(SOCKET Sock)
{
	if(Sock != INVALID_SOCKET)
	{
//		if(WSAIsBlocking())
//		{
//			DoPrintf("Skt=%u : Cancelled blocking call", Sock);
//			WSACancelBlockingCall();
//		}
		do_closesocket(Sock);
		DoPrintf("Skt=%u : Socket closed.", Sock);
		Sock = INVALID_SOCKET;
	}
	if(Sock != INVALID_SOCKET)
		DoPrintf("Skt=%u : Failed to close socket.", Sock);

	return(Sock);
}


/*----- ホストからログアウトする ----------------------------------------------
*
*	Parameter
*		kSOCKET ctrl_skt : ソケット
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

// 同時接続対応
//int DoQUIT(SOCKET ctrl_skt)
int DoQUIT(SOCKET ctrl_skt, int *CancelCheckWork)
{
	int Ret;

	Ret = FTP_COMPLETE;
	if(SendQuit == YES)
		// 同時接続対応
//		Ret = command(ctrl_skt, NULL, &CheckCancelFlg, "QUIT") / 100;
		Ret = command(ctrl_skt, NULL, CancelCheckWork, "QUIT") / 100;

	return(Ret);
}


/*----- リモート側のディレクトリリストを取得（コマンドコントロールソケットを使用)
*
*	Parameter
*		char *AddOpt : 追加のオプション
*		char *Path : パス名
*		int Num : ファイル名番号
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoDirListCmdSkt(char *AddOpt, char *Path, int Num, int *CancelCheckWork)
{
	int Sts;

	if(AskTransferNow() == YES)
		SktShareProh();

//	if((Sts = DoDirList(NULL, AskCmdCtrlSkt(), AddOpt, Path, Num)) == 429)
//	{
//		ReConnectCmdSkt();
		Sts = DoDirList(NULL, AskCmdCtrlSkt(), AddOpt, Path, Num, CancelCheckWork);

		if(Sts/100 >= FTP_CONTINUE)
			SoundPlay(SND_ERROR);
//	}
	return(Sts/100);
}


/*----- リモート側のディレクトリリストを取得 ----------------------------------
*
*	Parameter
*		HWND hWnd : 転送中ダイアログのウインドウハンドル
*		SOCKET cSkt : コントロールソケット
*		char *AddOpt : 追加のオプション
*		char *Path : パス名 (""=カレントディレクトリ)
*		int Num : ファイル名番号
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int DoDirList(HWND hWnd, SOCKET cSkt, char *AddOpt, char *Path, int Num, int *CancelCheckWork)
{
	char Tmp[FMAX_PATH];
	int Sts;

//#pragma aaa
//DoPrintf("===== DoDirList %d = %s", Num, Path);

	MakeCacheFileName(Num, Tmp);
//	MainTransPkt.ctrl_skt = cSkt;

	if(AskListCmdMode() == NO)
	{
		strcpy(MainTransPkt.Cmd, "NLST");
		if(strlen(AskHostLsName()) > 0)
		{
			strcat(MainTransPkt.Cmd, " ");
			if((AskHostType() == HTYPE_ACOS) || (AskHostType() == HTYPE_ACOS_4))
				strcat(MainTransPkt.Cmd, "'");
			strcat(MainTransPkt.Cmd, AskHostLsName());
			if((AskHostType() == HTYPE_ACOS) || (AskHostType() == HTYPE_ACOS_4))
				strcat(MainTransPkt.Cmd, "'");
		}
		if(strlen(AddOpt) > 0)
			strcat(MainTransPkt.Cmd, AddOpt);
	}
	else
	{
		// MLSD対応
//		strcpy(MainTransPkt.Cmd, "LIST");
		if(AskUseMLSD() && (AskHostFeature() & FEATURE_MLSD))
			strcpy(MainTransPkt.Cmd, "MLSD");
		else
			strcpy(MainTransPkt.Cmd, "LIST");
		if(strlen(AddOpt) > 0)
		{
			strcat(MainTransPkt.Cmd, " -");
			strcat(MainTransPkt.Cmd, AddOpt);
		}
	}

	if(strlen(Path) > 0)
		strcat(MainTransPkt.Cmd, " ");

	strcpy(MainTransPkt.RemoteFile, Path);
	strcpy(MainTransPkt.LocalFile, Tmp);
	MainTransPkt.Type = TYPE_A;
	MainTransPkt.Size = -1;
	/* ファイルリストの中の漢字のファイル名は、別途	*/
	/* ChangeFnameRemote2Local で変換する 			*/
	MainTransPkt.KanjiCode = KANJI_NOCNV;
	MainTransPkt.KanaCnv = YES;
	MainTransPkt.Mode = EXIST_OVW;
	// ミラーリング設定追加
	MainTransPkt.NoTransfer = NO;
	MainTransPkt.ExistSize = 0;
	MainTransPkt.hWndTrans = hWnd;
	MainTransPkt.Next = NULL;

	Sts = DoDownload(cSkt, &MainTransPkt, YES, CancelCheckWork);

//#pragma aaa
//DoPrintf("===== DoDirList Done.");

	return(Sts);
}


/*----- リモート側へコマンドを送りリプライを待つ（コマンドソケット）-----------
*
*	Parameter
*		char *Reply : リプライのコピー先 (NULL=コピーしない)
*		char *fmt : フォーマット文字列
*		... : パラメータ
*
*	Return Value
*		int 応答コード
*
*	Note
*		コマンドコントロールソケットを使う
*----------------------------------------------------------------------------*/

// 同時接続対応
//int CommandProcCmd(char *Reply, char *fmt, ...)
int CommandProcCmd(char *Reply, int* CancelCheckWork, char *fmt, ...)
{
	va_list Args;
	char Cmd[1024];
	int Sts;

	va_start(Args, fmt);
	wvsprintf(Cmd, fmt, Args);
	va_end(Args);

	if(AskTransferNow() == YES)
		SktShareProh();

//#pragma aaa
//DoPrintf("**CommandProcCmd : %s", Cmd);

//	if((Sts = command(AskCmdCtrlSkt(), Reply, "%s", Cmd)) == 429)
//	{
//		if(ReConnectCmdSkt() == FFFTP_SUCCESS)
//		{
			// 同時接続対応
//			Sts = command(AskCmdCtrlSkt(), Reply, &CheckCancelFlg, "%s", Cmd);
			Sts = command(AskCmdCtrlSkt(), Reply, CancelCheckWork, "%s", Cmd);
//		}
//	}
	return(Sts);
}


#if defined(HAVE_TANDEM)
/*----- OSS/Guardian ファイルシステムを切り替えるコマンドを送る ---------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SwitchOSSProc(void)
{
	char Buf[MAX_PATH+1];

	/* DoPWD でノード名の \ を保存するために OSSフラグも変更する */
	if(AskOSS() == YES) {
		DoQUOTE(AskCmdCtrlSkt(), "GUARDIAN", &CancelFlg);
		SetOSS(NO);
	} else {
		DoQUOTE(AskCmdCtrlSkt(), "OSS", &CancelFlg);
		SetOSS(YES);
	}
	/* Current Dir 再取得 */
	if (DoPWD(Buf) == FTP_COMPLETE)
		SetRemoteDirHist(Buf);
	/* ファイルリスト再読み込み */
	PostMessage(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(REFRESH_REMOTE, 0), 0);

	return;
}
#endif


/*----- リモート側へコマンドを送りリプライを待つ（転送ソケット）---------------
*
*	Parameter
*		SOCKET cSkt : ソケット
*		char *Reply : リプライのコピー先 (NULL=コピーしない)
*		int *CancelCheckWork :
*		char *fmt : フォーマット文字列
*		... : パラメータ
*
*	Return Value
*		int 応答コード
*
*	Note
*		転送コントロールソケットを使う
*----------------------------------------------------------------------------*/

// 同時接続対応
//int CommandProcTrn(char *Reply, char *fmt, ...)
int CommandProcTrn(SOCKET cSkt, char *Reply, int* CancelCheckWork, char *fmt, ...)
{
	va_list Args;
	char Cmd[1024];
	int Sts;

	va_start(Args, fmt);
	wvsprintf(Cmd, fmt, Args);
	va_end(Args);

//#pragma aaa
//DoPrintf("**CommandProcTrn : %s", Cmd);

//	if((Sts = command(AskTrnCtrlSkt(), Reply, "%s", Cmd)) == 429)
//	{
//		if(ReConnectTrnSkt() == FFFTP_SUCCESS)
//			Sts = command(AskTrnCtrlSkt(), Reply, &CheckCancelFlg, "%s", Cmd);
			Sts = command(cSkt, Reply, CancelCheckWork, "%s", Cmd);
//	}
	return(Sts);
}


/*----- コマンドを送りリプライを待つ ------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		char *Reply : リプライのコピー先 (NULL=コピーしない)
*		char *fmt : フォーマット文字列
*		... : パラメータ
*
*	Return Value
*		int 応答コード
*
*	Note
*		ホストのファイル名の漢字コードに応じて、ここで漢字コードの変換を行なう
*----------------------------------------------------------------------------*/

//#pragma aaa
//static int cntcnt = 0;

int command(SOCKET cSkt, char *Reply, int *CancelCheckWork, char *fmt, ...)
{
	va_list Args;
	char Cmd[FMAX_PATH*2];
	int Sts;
	char TmpBuf[ONELINE_BUF_SIZE];

	if(cSkt != INVALID_SOCKET)
	{
		va_start(Args, fmt);
		wvsprintf(Cmd, fmt, Args);
		va_end(Args);

		if(strncmp(Cmd, "PASS ", 5) == 0)
			SetTaskMsg(">PASS [xxxxxx]");
		else if((strncmp(Cmd, "USER ", 5) == 0) ||
				(strncmp(Cmd, "OPEN ", 5) == 0))
		{
			SetTaskMsg(">%s", Cmd);
		}
		else
		{
			ChangeSepaLocal2Remote(Cmd);
			SetTaskMsg(">%s", Cmd);
			// UTF-8対応
//			ChangeFnameLocal2Remote(Cmd, FMAX_PATH*2);
		}
		// UTF-8対応
		ChangeFnameLocal2Remote(Cmd, FMAX_PATH*2);

//		DoPrintf("SEND : %s", Cmd);
		strcat(Cmd, "\x0D\x0A");

		if(Reply != NULL)
			strcpy(Reply, "");

		Sts = 429;
		if(SendData(cSkt, Cmd, strlen(Cmd), 0, CancelCheckWork) == FFFTP_SUCCESS)
		{
			Sts = ReadReplyMessage(cSkt, Reply, 1024, CancelCheckWork, TmpBuf);
		}

//#pragma aaa
//if(Reply != NULL)
//	DoPrintf("%x : %x : %s : %s", cSkt, &TmpBuf, Cmd, Reply);
//else
//	DoPrintf("%x : %x : %s : NULL", cSkt, &TmpBuf, Cmd);

//		DoPrintf("command() RET=%d", Sts);
	}
	else
		Sts = 429;

	return(Sts);
}


/*----- データを送る ----------------------------------------------------------
*
*	Parameter
*		SOCKET Skt : ソケット
*		char *Data : データ
*		int Size : 送るデータのサイズ
*		int Mode : コールモード
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int SendData(SOCKET Skt, char *Data, int Size, int Mode, int *CancelCheckWork)
{
	int Sts;
	int Tmp;
//	fd_set SendFds;
//	struct timeval Tout;
//	struct timeval *ToutPtr;
	int TimeOutErr;

	Sts = FFFTP_FAIL;
	if(Skt != INVALID_SOCKET)
	{
		Sts = FFFTP_SUCCESS;
		while(Size > 0)
		{
//			FD_ZERO(&SendFds);
//			FD_SET(Skt, &SendFds);
//			ToutPtr = NULL;
//			if(TimeOut != 0)
//			{
//				Tout.tv_sec = TimeOut;
//				Tout.tv_usec = 0;
//				ToutPtr = &Tout;
//			}
//			Tmp = select(0, NULL, &SendFds, NULL, ToutPtr);
//			if(Tmp == SOCKET_ERROR)
//			{
//				Sts = FFFTP_FAIL;
//				ReportWSError("select", WSAGetLastError());
//				break;
//			}
//			else if(Tmp == 0)
//			{
//				Sts = FFFTP_FAIL;
//				SetTaskMsg(MSGJPN241);
//				break;
//			}

			Tmp = do_send(Skt, Data, Size, Mode, &TimeOutErr, CancelCheckWork);
			if(TimeOutErr == YES)
			{
				Sts = FFFTP_FAIL;
				SetTaskMsg(MSGJPN241);
				break;
			}
			else if(Tmp == SOCKET_ERROR)
			{
				Sts = FFFTP_FAIL;
				ReportWSError("send", WSAGetLastError());
				break;
			}

			Size -= Tmp;
			Data += Tmp;
		}
	}
	return(Sts);
}


/*----- 応答メッセージを受け取る ----------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		char *Buf : メッセージを受け取るバッファ (NULL=コピーしない)
*		int Max : バッファのサイズ
*		int *CancelCheckWork :
*		char *Tmp : テンポラリワーク
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

int ReadReplyMessage(SOCKET cSkt, char *Buf, int Max, int *CancelCheckWork, char *Tmp)
{
	int iRetCode;
	int iContinue;
	int FirstCode;
	int Lines;
	int i;

	if(Buf != NULL)
		memset(Buf, NUL, Max);
	Max--;

	FirstCode = 0;
	if(cSkt != INVALID_SOCKET)
	{
		Lines = 0;
		do
		{
			iContinue = NO;
			iRetCode = ReadOneLine(cSkt, Tmp, ONELINE_BUF_SIZE, CancelCheckWork);

			// 文字化け対策
			ChangeFnameRemote2Local(Tmp, ONELINE_BUF_SIZE);
			SetTaskMsg("%s", Tmp);

			if(Buf != NULL)
			{
				// ２行目以降の応答コードは消す
				if(Lines > 0)
				{
					for(i = 0; ; i++)
					{
						if(IsDigit(Tmp[i]) == 0)
							break;
						Tmp[i] = ' ';
					}
				}
				strncat(Buf, Tmp, Max);
				Max = max1(0, Max-strlen(Tmp));

//				strncpy(Buf, Tmp, Max);
			}

			if((iRetCode != 421) && (iRetCode != 429))
			{
				if((FirstCode == 0) &&
				   (iRetCode >= 100) && (iRetCode <= 599))
				{
					FirstCode = iRetCode;
				}

				if((iRetCode < 100) || (iRetCode > 599) ||
				   (*(Tmp + 3) == '-') ||
				   ((FirstCode > 0) && (iRetCode != FirstCode)))
				{
					iContinue = YES;
				}
			}
			else
				FirstCode = iRetCode;

			Lines++;
		}
		while(iContinue == YES);
	}
	return(FirstCode);
}


/*----- １行分のデータを受け取る ----------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		char *Buf : メッセージを受け取るバッファ
*		int Max : バッファのサイズ
*		int *CancelCheckWork : 
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int ReadOneLine(SOCKET cSkt, char *Buf, int Max, int *CancelCheckWork)
{
	char *Pos;
	int SizeOnce;
	int CopySize;
	int ResCode;
	int i;
//	fd_set ReadFds;
//	struct timeval Tout;
//	struct timeval *ToutPtr;
	char Tmp[1024];
	int TimeOutErr;

	ResCode = 0;
	if(cSkt != INVALID_SOCKET)
	{
		memset(Buf, NUL, Max);
		Max--;					/* 末尾のNULLのぶん */
		Pos = Buf;

		for(;;)
		{
//			FD_ZERO(&ReadFds);
//			FD_SET(cSkt, &ReadFds);
//			ToutPtr = NULL;
//			if(TimeOut != 0)
//			{
//				Tout.tv_sec = TimeOut;
//				Tout.tv_usec = 0;
//				ToutPtr = &Tout;
//			}
//			i = select(0, &ReadFds, NULL, NULL, ToutPtr);
//			if(i == SOCKET_ERROR)
//			{
//				ReportWSError("select", WSAGetLastError());
//				SizeOnce = -1;
//				break;
//			}
//			else if(i == 0)
//			{
//				SetTaskMsg(MSGJPN242);
//				SizeOnce = -2;
//				break;
//			}

			/* LFまでを受信するために、最初はPEEKで受信 */
			if((SizeOnce = do_recv(cSkt, (LPSTR)Tmp, 1024, MSG_PEEK, &TimeOutErr, CancelCheckWork)) <= 0)
			{
				if(TimeOutErr == YES)
				{
					SetTaskMsg(MSGJPN242);
					SizeOnce = -2;
				}
				else if(SizeOnce == SOCKET_ERROR)
				{
					SizeOnce = -1;
				}
				break;
			}

			/* LFを探して、あったらそこまでの長さをセット */
			for(i = 0; i < SizeOnce ; i++)
			{
				if(*(Tmp + i) == NUL || *(Tmp + i) == 0x0A)
				{
					SizeOnce = i + 1;
					break;
				}
			}

			/* 本受信 */
			if((SizeOnce = do_recv(cSkt, Tmp, SizeOnce, 0, &TimeOutErr, CancelCheckWork)) <= 0)
				break;

			CopySize = min1(Max, SizeOnce);
			memcpy(Pos, Tmp, CopySize);
			Pos += CopySize;
			Max -= CopySize;

			/* データがLFで終わっていたら１行終わり */
			if(*(Tmp + SizeOnce - 1) == 0x0A)
				break;
		}
		*Pos = NUL;

		if(SizeOnce <= 0)
		{
			ResCode = 429;
			memset(Buf, 0, Max);

			if((SizeOnce == -2) || (AskTransferNow() == YES))
			// 転送中に全て中止を行うと不正なデータが得られる場合のバグ修正
//				DisconnectSet();
				cSkt = DoClose(cSkt);
		}
		else
		{
			if(IsDigit(*Buf) && IsDigit(*(Buf+1)) && IsDigit(*(Buf+2)))
			{
				memset(Tmp, NUL, 4);
				strncpy(Tmp, Buf, 3);
				ResCode = atoi(Tmp);
			}

			/* 末尾の CR,LF,スペースを取り除く */
			while((i=strlen(Buf))>2 &&
				  (Buf[i-1]==0x0a || Buf[i-1]==0x0d || Buf[i-1]==' '))
				Buf[i-1]=0;
		}
	}
	return(ResCode);
}


/*----- 固定長データを受け取る ------------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		char *Buf : メッセージを受け取るバッファ
*		int Size : バイト数
*		int *CancelCheckWork : 
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int ReadNchar(SOCKET cSkt, char *Buf, int Size, int *CancelCheckWork)
{
//	struct timeval Tout;
//	struct timeval *ToutPtr;
//	fd_set ReadFds;
//	int i;
	int SizeOnce;
	int Sts;
	int TimeOutErr;

	Sts = FFFTP_FAIL;
	if(cSkt != INVALID_SOCKET)
	{
		Sts = FFFTP_SUCCESS;
		while(Size > 0)
		{
//			FD_ZERO(&ReadFds);
//			FD_SET(cSkt, &ReadFds);
//			ToutPtr = NULL;
//			if(TimeOut != 0)
//			{
//				Tout.tv_sec = TimeOut;
//				Tout.tv_usec = 0;
//				ToutPtr = &Tout;
//			}
//			i = select(0, &ReadFds, NULL, NULL, ToutPtr);
//			if(i == SOCKET_ERROR)
//			{
//				ReportWSError("select", WSAGetLastError());
//				Sts = FFFTP_FAIL;
//				break;
//			}
//			else if(i == 0)
//			{
//				SetTaskMsg(MSGJPN243);
//				Sts = FFFTP_FAIL;
//				break;
//			}

			if((SizeOnce = do_recv(cSkt, Buf, Size, 0, &TimeOutErr, CancelCheckWork)) <= 0)
			{
				if(TimeOutErr == YES)
					SetTaskMsg(MSGJPN243);
				Sts = FFFTP_FAIL;
				break;
			}

			Buf += SizeOnce;
			Size -= SizeOnce;
		}
	}

	if(Sts == FFFTP_FAIL)
		SetTaskMsg(MSGJPN244);

	return(Sts);
}


/*----- エラー文字列を取得 ----------------------------------------------------
*
*	Parameter
*		UINT Error : エラー番号
*
*	Return Value
*		char *エラー文字列
*----------------------------------------------------------------------------*/

char *ReturnWSError(UINT Error)
{
	static char Msg[128];
	char *Str;

	switch(Error)
	{
		case WSAVERNOTSUPPORTED:
			Str = "version of WinSock not supported";
			break;

		case WSASYSNOTREADY:
			Str = "WinSock not present or not responding";
			break;

		case WSAEINVAL:
			Str = "app version not supported by DLL";
			break;

		case WSAHOST_NOT_FOUND:
			Str = "Authoritive: Host not found";
			break;

		case WSATRY_AGAIN:
			Str = "Non-authoritive: host not found or server failure";
			break;

		case WSANO_RECOVERY:
			Str = "Non-recoverable: refused or not implemented";
			break;

		case WSANO_DATA:
			Str = "Valid name, no data record for type";
			break;

#if 0
		case WSANO_ADDRESS:
			Str = "Valid name, no MX record";
			break;
#endif

		case WSANOTINITIALISED:
			Str = "WSA Startup not initialized";
			break;

		case WSAENETDOWN:
			Str = "Network subsystem failed";
			break;

		case WSAEINPROGRESS:
			Str = "Blocking operation in progress";
			break;

		case WSAEINTR:
			Str = "Blocking call cancelled";
			break;

		case WSAEAFNOSUPPORT:
			Str = "address family not supported";
			break;

		case WSAEMFILE:
			Str = "no file descriptors available";
			break;

		case WSAENOBUFS:
			Str = "no buffer space available";
			break;

		case WSAEPROTONOSUPPORT:
			Str = "specified protocol not supported";
			break;

		case WSAEPROTOTYPE:
			Str = "protocol wrong type for this socket";
			break;

		case WSAESOCKTNOSUPPORT:
			Str = "socket type not supported for address family";
			break;

		case WSAENOTSOCK:
			Str = "descriptor is not a socket";
			break;

		case WSAEWOULDBLOCK:
			Str = "socket marked as non-blocking and SO_LINGER set not 0";
			break;

		case WSAEADDRINUSE:
			Str = "address already in use";
			break;

		case WSAECONNABORTED:
			Str = "connection aborted";
			break;

		case WSAECONNRESET:
			Str = "connection reset";
			break;

		case WSAENOTCONN:
			Str = "not connected";
			break;

		case WSAETIMEDOUT:
			Str = "connection timed out";
			break;

		case WSAECONNREFUSED:
			Str = "connection refused";
			break;

		case WSAEHOSTDOWN:
			Str = "host down";
			break;

		case WSAEHOSTUNREACH:
			Str = "host unreachable";
			break;

		case WSAEADDRNOTAVAIL:
			Str = "address not available";
			break;

		default:
			sprintf(Msg, "error %u", Error);
			return(Msg);
	}
	return(Str);
}


/*----- デバッグコンソールにエラーを表示 --------------------------------------
*
*	Parameter
*		char *Msg : エラーの前に表示するメッセージ
*		UINT Error : エラー番号
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ReportWSError(char *Msg, UINT Error)
{
	if(Msg != NULL)
		DoPrintf("[[%s : %s]]", Msg, ReturnWSError(Error));
	else
		DoPrintf("[[%s]]", ReturnWSError(Error));
}


/*----- ファイル名をローカル側で扱えるように変換する --------------------------
*
*	Parameter
*		char *Fname : ファイル名
*		int Max : 最大長
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int ChangeFnameRemote2Local(char *Fname, int Max)
{
	int Sts;
	char *Buf;
	char *Pos;
	CODECONVINFO cInfo;
	// バッファ上書きバグ対策
	char *Buf2;

	Sts = FFFTP_FAIL;
	if((Buf = malloc(Max)) != NULL)
	{
	// バッファ上書きバグ対策
	if((Buf2 = malloc(strlen(Fname) + 1)) != NULL)
	{
		InitCodeConvInfo(&cInfo);
		cInfo.KanaCnv = NO;			//AskHostNameKana();
		// バッファ上書きバグ対策
//		cInfo.Str = Fname;
		strcpy(Buf2, Fname);
		cInfo.Str = Buf2;
		cInfo.StrLen = strlen(Fname);
		cInfo.Buf = Buf;
		cInfo.BufSize = Max - 1;

		// ここで全てUTF-8へ変換する
		// TODO: SJIS以外も直接UTF-8へ変換
		switch(AskHostNameKanji())
		{
			case KANJI_SJIS :
				ConvSJIStoUTF8N(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_JIS :
				ConvJIStoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoUTF8N(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_EUC :
				ConvEUCtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoUTF8N(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_SMB_HEX :
			case KANJI_SMB_CAP :
				ConvSMBtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoUTF8N(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

//			case KANJI_UTF8N :
//				ConvUTF8NtoSJIS(&cInfo);
//				*(Buf + cInfo.OutLen) = NUL;
//				strcpy(Fname, Buf);
//				Pos = strchr(Fname, NUL);
//				FlushRestData(&cInfo);
//				*(Buf + cInfo.OutLen) = NUL;
//				strcpy(Pos, Buf);
//				break;

			// UTF-8 HFS+対応
			case KANJI_UTF8HFSX :
				ConvUTF8HFSXtoUTF8N(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;
		}
		// バッファ上書きバグ対策
		free(Buf2);
		Sts = FFFTP_SUCCESS;
		}
		free(Buf);
		// バッファ上書きバグ対策
//		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- ファイル名をリモート側で扱えるように変換する --------------------------
*
*	Parameter
*		char *Fname : ファイル名
*		int Max : 最大長
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int ChangeFnameLocal2Remote(char *Fname, int Max)
{
	int Sts;
	char *Buf;
	char *Pos;
	CODECONVINFO cInfo;
	// バッファ上書きバグ対策
	char *Buf2;

	Sts = FFFTP_FAIL;
	if((Buf = malloc(Max)) != NULL)
	{
	// バッファ上書きバグ対策
	if((Buf2 = malloc(strlen(Fname) + 1)) != NULL)
	{
		InitCodeConvInfo(&cInfo);
		cInfo.KanaCnv = AskHostNameKana();
		// バッファ上書きバグ対策
//		cInfo.Str = Fname;
		strcpy(Buf2, Fname);
		cInfo.Str = Buf2;
		cInfo.StrLen = strlen(Fname);
		cInfo.Buf = Buf;
		cInfo.BufSize = Max - 1;

		// ここで全てUTF-8から変換する
		// TODO: SJIS以外も直接UTF-8から変換
		switch(AskHostNameKanji())
		{
			case KANJI_SJIS :
				ConvUTF8NtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_JIS :
				ConvUTF8NtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_EUC :
				ConvUTF8NtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoEUC(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_SMB_HEX :
				ConvUTF8NtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoSMB_HEX(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

			case KANJI_SMB_CAP :
				ConvUTF8NtoSJIS(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				// TODO
				InitCodeConvInfo(&cInfo);
				cInfo.KanaCnv = NO;
				cInfo.Str = Fname;
				cInfo.StrLen = strlen(Fname);
				cInfo.Buf = Buf;
				cInfo.BufSize = Max - 1;
				ConvSJIStoSMB_CAP(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;

//			case KANJI_UTF8N :
//				ConvSJIStoUTF8N(&cInfo);
//				*(Buf + cInfo.OutLen) = NUL;
//				strcpy(Fname, Buf);
//				Pos = strchr(Fname, NUL);
//				FlushRestData(&cInfo);
//				*(Buf + cInfo.OutLen) = NUL;
//				strcpy(Pos, Buf);
//				break;

			// UTF-8 HFS+対応
			case KANJI_UTF8HFSX :
				ConvUTF8NtoUTF8HFSX(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Fname, Buf);
				Pos = strchr(Fname, NUL);
				FlushRestData(&cInfo);
				*(Buf + cInfo.OutLen) = NUL;
				strcpy(Pos, Buf);
				break;
		}
		// バッファ上書きバグ対策
		free(Buf2);
		Sts = FFFTP_SUCCESS;
		}
		free(Buf);
		// バッファ上書きバグ対策
//		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- パスの区切り文字をホストに合わせて変更する ----------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
static void ChangeSepaLocal2Remote(char *Fname)
{
	if(AskHostType() == HTYPE_STRATUS)
	{
		ReplaceAll(Fname, '/', '>');
	}
	return;
}


/*----- パスの区切り文字をローカルに合わせて変更する --------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
static void ChangeSepaRemote2Local(char *Fname)
{
	if(AskHostType() == HTYPE_STRATUS)
	{
		ReplaceAll(Fname, '>', '/');
	}
	return;
}







