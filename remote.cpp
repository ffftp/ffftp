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

#include "common.h"


#define PWD_XPWD		0
#define PWD_PWD			1

/*===== プロトタイプ =====*/

static int DoPWD(char *Buf);
static int ReadOneLine(SOCKET cSkt, char *Buf, int Max, int *CancelCheckWork);
static int DoDirList(HWND hWnd, SOCKET cSkt, char *AddOpt, char *Path, int Num, int *CancelCheckWork);
static void ChangeSepaLocal2Remote(char *Fname);
static void ChangeSepaRemote2Local(char *Fname);
#define CommandProcCmd(REPLY, CANCELCHECKWORK, ...) (AskTransferNow() == YES && (SktShareProh(), 0), command(AskCmdCtrlSkt(), REPLY, CANCELCHECKWORK, __VA_ARGS__))

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
	char Tmp[1024] = "";
	int Sts = 0;

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

int DoRENAME(const char *Src, const char *Dst)
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

int DoCHMOD(const char *Path, const char *Mode)
{
	int Sts;

	Sts = CommandProcCmd(NULL, &CancelFlg, "%s %s %s", AskHostChmodCmd().c_str(), Mode, Path);

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
		if(sscanf(Tmp+4, "%04hu%02hu%02hu%02hu%02hu%02hu",
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
		Sts = CommandProcTrn(cSkt, Tmp, CancelCheckWork, "MFMT %04hu%02hu%02hu%02hu%02hu%02hu %s", sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond, Path);
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
		DoPrintf("Skt=%zu : Socket closed.", Sock);
		Sock = INVALID_SOCKET;
	}
	if(Sock != INVALID_SOCKET)
		DoPrintf("Skt=%zu : Failed to close socket.", Sock);

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
	int Sts;
	if(AskListCmdMode() == NO)
	{
		strcpy(MainTransPkt.Cmd, "NLST");
		if(!empty(AskHostLsName()))
		{
			strcat(MainTransPkt.Cmd, " ");
			if((AskHostType() == HTYPE_ACOS) || (AskHostType() == HTYPE_ACOS_4))
				strcat(MainTransPkt.Cmd, "'");
			strcat(MainTransPkt.Cmd, AskHostLsName().c_str());
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
	strcpy(MainTransPkt.LocalFile, MakeCacheFileName(Num).u8string().c_str());
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
	PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(REFRESH_REMOTE, 0), 0);

	return;
}
#endif


// コマンドを送りリプライを待つ
// ホストのファイル名の漢字コードに応じて、ここで漢字コードの変換を行なう
int _command(SOCKET cSkt, char* Reply, int* CancelCheckWork, const char* fmt, ...) {
	if (cSkt == INVALID_SOCKET)
		return 429;
	char Cmd[FMAX_PATH * 2];
	va_list Args;
	va_start(Args, fmt);
	vsprintf(Cmd, fmt, Args);
	va_end(Args);
	if (strncmp(Cmd, "PASS ", 5) == 0)
		SetTaskMsg(">PASS [xxxxxx]");
	else if (strncmp(Cmd, "USER ", 5) == 0 || strncmp(Cmd, "OPEN ", 5) == 0)
		SetTaskMsg(">%s", Cmd);
	else {
		ChangeSepaLocal2Remote(Cmd);
		SetTaskMsg(">%s", Cmd);
	}
	strncpy(Cmd, ConvertTo(Cmd, AskHostNameKanji(), AskHostNameKana()).c_str(), FMAX_PATH * 2);
	strcat(Cmd, "\x0D\x0A");
	if (Reply != NULL)
		strcpy(Reply, "");
	if (SendData(cSkt, Cmd, (int)strlen(Cmd), 0, CancelCheckWork) != FFFTP_SUCCESS)
		return 429;
	char TmpBuf[ONELINE_BUF_SIZE];
	return ReadReplyMessage(cSkt, Reply, 1024, CancelCheckWork, TmpBuf);
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

			strncpy(Tmp, ConvertFrom(Tmp, AskHostNameKanji()).c_str(), ONELINE_BUF_SIZE);
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
				Max = max1(0, Max-(int)strlen(Tmp));

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
			while((i=(int)strlen(Buf))>2 &&
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


// デバッグコンソールにエラーを表示
void ReportWSError(char* Msg, UINT Error) {
	DoPrintf("[[%s : %s]]", Msg, u8(GetErrorMessage(Error)).c_str());
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







