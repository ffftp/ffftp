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
static std::tuple<int, std::string> ReadOneLine(SOCKET cSkt, int* CancelCheckWork);
static int DoDirList(HWND hWnd, SOCKET cSkt, const char* AddOpt, const char* Path, int Num, int *CancelCheckWork);
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

int DoCWD(std::wstring const& Path, int Disp, int ForceGet, int ErrorBell) {
	int Sts
		= Path == L""sv ? FTP_COMPLETE * 100
		: Path == L".."sv ? CommandProcCmd(NULL, &CancelFlg, L"CDUP"sv)
		: AskHostType() != HTYPE_VMS || Path.starts_with(L'[') ? CommandProcCmd(NULL, &CancelFlg, L"CWD %s", Path.c_str())
		: CommandProcCmd(NULL, &CancelFlg, L"CWD [.%s]", Path.c_str());	/* VMS用 */
	if (Sts / 100 >= FTP_CONTINUE && ErrorBell == YES)
		Sound::Error.Play();
	if ((Sts / 100 == FTP_COMPLETE || ForceGet == YES) && Disp == YES) {
		std::wstring cwd;
		if (char buf[FMAX_PATH + 1]; DoPWD(buf) == FTP_COMPLETE)
			cwd = u8(buf);
		/*===== PWDが使えなかった場合 =====*/
		else if (Path.starts_with(L'/'))
			cwd = Path;
		else {
			cwd = AskRemoteCurDir();
			if (empty(cwd))
				cwd = L'/';
			cwd = (fs::path{ cwd } / Path).lexically_normal().generic_wstring();
		}
		SetRemoteDirHist(cwd);
	}
	return Sts / 100;
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

int DoCWDStepByStep(std::wstring const& Path, std::wstring const& Cur) {
	static boost::wregex re{ L"[^/]+" };
	int Sts = FTP_COMPLETE;
	for (boost::wsregex_iterator it{ begin(Path), end(Path), re }, end; it != end; ++it)
		if ((Sts = DoCWD(it->str(), NO, NO, NO)) != FTP_COMPLETE)
			break;
	if (Sts != FTP_COMPLETE)
		DoCWD(Cur, NO, NO, NO);
	return Sts;
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


// リモート側のディレクトリ作成
int DoMKD(std::wstring const& Path) {
	int Sts = CommandProcCmd(NULL, &CancelFlg, L"MKD %s", Path.c_str());
	if (Sts / 100 >= FTP_CONTINUE)
		Sound::Error.Play();
	if (CancelFlg == NO && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval()) {
		NoopProc(YES);
		LastDataConnectionTime = time(NULL);
	}
	return Sts / 100;
}


/*----- リモート側のディレクトリ削除 ------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int 応答コードの１桁目
*----------------------------------------------------------------------------*/

int DoRMD(const char* Path)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "RMD %s", Path);
	Sts = CommandProcCmd(NULL, &CancelFlg, "RMD %s", Path);

	if(Sts/100 >= FTP_CONTINUE)
		Sound::Error.Play();

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

int DoDELE(const char* Path)
{
	int Sts;

	// 同時接続対応
//	Sts = CommandProcCmd(NULL, "DELE %s", Path);
	Sts = CommandProcCmd(NULL, &CancelFlg, "DELE %s", Path);

	if(Sts/100 >= FTP_CONTINUE)
		Sound::Error.Play();

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
		Sound::Error.Play();

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
		Sound::Error.Play();

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
int DoSIZE(SOCKET cSkt, const char* Path, LONGLONG *Size, int *CancelCheckWork)
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
int DoMDTM(SOCKET cSkt, const char* Path, FILETIME *Time, int *CancelCheckWork)
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
int DoMFMT(SOCKET cSkt, const char* Path, FILETIME *Time, int *CancelCheckWork)
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
int DoQUOTE(SOCKET cSkt, const char* CmdStr, int *CancelCheckWork)
{
	int Sts;

//	Sts = CommandProcCmd(NULL, "%s", CmdStr);
	Sts = CommandProcTrn(cSkt, NULL, CancelCheckWork, "%s", CmdStr);

	if(Sts/100 >= FTP_CONTINUE)
		Sound::Error.Play();

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
		do_closesocket(Sock);
		DoPrintf(L"Skt=%zu : Socket closed.", Sock);
		Sock = INVALID_SOCKET;
	}
	if(Sock != INVALID_SOCKET)
		DoPrintf(L"Skt=%zu : Failed to close socket.", Sock);

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

int DoDirListCmdSkt(const char* AddOpt, const char* Path, int Num, int *CancelCheckWork)
{
	int Sts;

	if(AskTransferNow() == YES)
		SktShareProh();

//	if((Sts = DoDirList(NULL, AskCmdCtrlSkt(), AddOpt, Path, Num)) == 429)
//	{
//		ReConnectCmdSkt();
		Sts = DoDirList(NULL, AskCmdCtrlSkt(), AddOpt, Path, Num, CancelCheckWork);

		if(Sts/100 >= FTP_CONTINUE)
			Sound::Error.Play();
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

static int DoDirList(HWND hWnd, SOCKET cSkt, const char* AddOpt, const char* Path, int Num, int *CancelCheckWork)
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

	Sts = DoDownload(cSkt, MainTransPkt, YES, CancelCheckWork);
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
		SetRemoteDirHist(u8(Buf));
	/* ファイルリスト再読み込み */
	PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(REFRESH_REMOTE, 0), 0);

	return;
}
#endif


// コマンドを送りリプライを待つ
// ホストのファイル名の漢字コードに応じて、ここで漢字コードの変換を行なう
int detail::command(SOCKET cSkt, char* Reply, int* CancelCheckWork, std::wstring&& cmd) {
	if (cmd.starts_with(L"PASS "sv))
		SetTaskMsg(">PASS [xxxxxx]");
	else {
		if (!cmd.starts_with(L"USER "sv) && !cmd.starts_with(L"OPEN "sv)) {
			// パスの区切り文字をホストに合わせて変更する
			if (AskHostType() == HTYPE_STRATUS)
				std::ranges::replace(cmd, L'/', L'>');
		}
		SetTaskMsg(">%s", u8(cmd).c_str());
	}
	auto native = ConvertTo(cmd, AskHostNameKanji(), AskHostNameKana());
	native += "\r\n"sv;
	if (Reply != NULL)
		strcpy(Reply, "");
	if (SendData(cSkt, data(native), size_as<int>(native), 0, CancelCheckWork) != FFFTP_SUCCESS)
		return 429;
	auto [code, text] = ReadReplyMessage(cSkt, CancelCheckWork);
	if (Reply)
		strncpy_s(Reply, 1024, text.c_str(), _TRUNCATE);
	return code;
}


// 応答メッセージを受け取る
std::tuple<int, std::string> ReadReplyMessage(SOCKET cSkt, int* CancelCheckWork) {
	int firstCode = 0;
	std::string text;
	if (cSkt != INVALID_SOCKET)
		for (int Lines = 0;; Lines++) {
			auto [code, line] = ReadOneLine(cSkt, CancelCheckWork);
			line = u8(ConvertFrom(line, AskHostNameKanji()));
			SetTaskMsg("%s", line.c_str());

			// ２行目以降の応答コードは消す
			if (Lines > 0)
				for (int i = 0; i < size_as<int>(line) && IsDigit(line[i]); i++)
					line[i] = ' ';
			text += line;

			if (code == 421 || code == 429) {
				firstCode = code;
				break;
			}
			if (100 <= code && code < 600) {
				if (firstCode == 0)
					firstCode = code;
				if (firstCode == code && (size(line) <= 3 || line[3] != '-'))
					break;
			}
		}
	return { firstCode, text };
}


// １行分のデータを受け取る
static std::tuple<int, std::string> ReadOneLine(SOCKET cSkt, int* CancelCheckWork) {
	if (cSkt == INVALID_SOCKET)
		return { 0, {} };

	int read;
	std::string line;
	for (char buffer[1024];;) {
		int TimeOutErr;
		/* LFまでを受信するために、最初はPEEKで受信 */
		if ((read = do_recv(cSkt, buffer, size_as<int>(buffer), MSG_PEEK, &TimeOutErr, CancelCheckWork)) <= 0) {
			if (TimeOutErr == YES) {
				SetTaskMsg(IDS_MSGJPN242);
				read = -2;
			} else if (read == SOCKET_ERROR)
				read = -1;
			break;
		}
		/* LFを探して、あったらそこまでの長さをセット */
		assert(std::find(buffer, buffer + read, '\0') == buffer + read);
		if (auto lf = std::find(buffer, buffer + read, '\n'); lf != buffer + read)
			read = (int)(lf - buffer + 1);
		/* 本受信 */
		if ((read = do_recv(cSkt, buffer, read, 0, &TimeOutErr, CancelCheckWork)) <= 0)
			break;
		line.append(buffer, buffer + read);
		/* データがLFで終わっていたら１行終わり */
		if (line.back() == '\n')
			break;
	}
	if (read <= 0) {
		if (read == -2 || AskTransferNow() == YES)
			cSkt = DoClose(cSkt);
		return { 429, {} };
	}

	int replyCode = 0;
	if (IsDigit(line[0]) && IsDigit(line[1]) && IsDigit(line[2]))
		std::from_chars(data(line), data(line) + 3, replyCode);

	/* 末尾の CR,LF,スペースを取り除く */
	static boost::regex re{ R"([\r\n ]+$)" };
	line = boost::regex_replace(line, re, "");
	return { replyCode, std::move(line) };
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
			if((SizeOnce = do_recv(cSkt, Buf, Size, 0, &TimeOutErr, CancelCheckWork)) <= 0)
			{
				if(TimeOutErr == YES)
					SetTaskMsg(IDS_MSGJPN243);
				Sts = FFFTP_FAIL;
				break;
			}

			Buf += SizeOnce;
			Size -= SizeOnce;
		}
	}

	if(Sts == FFFTP_FAIL)
		SetTaskMsg(IDS_MSGJPN244);

	return(Sts);
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







