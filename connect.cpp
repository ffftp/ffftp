/*=============================================================================
*
*								ホストへの接続／切断
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

static int SendInitCommand(SOCKET Socket, char *Cmd, int *CancelCheckWork);
static void AskUseFireWall(char *Host, int *Fire, int *Pasv, int *List);
static void SaveCurrentSetToHistory(void);
static int ReConnectSkt(SOCKET *Skt);
// 暗号化通信対応
// 同時接続対応
//static SOCKET DoConnect(char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security);
static SOCKET DoConnectCrypt(int CryptMode, HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork);
static SOCKET DoConnect(HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork);
static int CheckOneTimePassword(char *Pass, char *Reply, int Type);

/*===== 外部参照 =====*/

extern char FilterStr[FILTER_EXT_LEN+1];
extern char TitleHostName[HOST_ADRS_LEN+1];
extern int CancelFlg;
// タイトルバーにユーザー名表示対応
extern char TitleUserName[USER_NAME_LEN+1];

/* 設定値 */
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern char FwallHost[HOST_ADRS_LEN+1];
extern char FwallUser[USER_NAME_LEN+1];
extern char FwallPass[PASSWORD_LEN+1];
extern int FwallPort;
extern int FwallType;
extern int FwallDefault;
extern int FwallSecurity;
extern int FwallResolve;
extern int FwallLower;
extern int FwallDelimiter;
extern int PasvDefault;
extern int QuickAnonymous;
// 切断対策
extern int TimeOut;
// UPnP対応
extern int UPnPEnabled;

/*===== ローカルなワーク =====*/

static int Anonymous;
static int TryConnect = NO;
static SOCKET CmdCtrlSocket = INVALID_SOCKET;
static SOCKET TrnCtrlSocket = INVALID_SOCKET;
HOSTDATA CurHost;

#if defined(HAVE_TANDEM)
static int Oss = NO;  /* OSS ファイルシステムへアクセスしている場合は YES */
#endif



/*----- ホスト一覧を使ってホストへ接続 ----------------------------------------
*
*	Parameter
*		int Type : ダイアログのタイプ (DLG_TYPE_xxx)
*		int Num : 接続するホスト番号(0～, -1=ダイアログを出す)

*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ConnectProc(int Type, int Num)
{
	int Save;
	int LFSort;
	int LDSort;
	int RFSort;
	int RDSort;

	SaveBookMark();
	SaveCurrentSetToHost();

	if((Num >= 0) || (SelectHost(Type) == YES))
	{
		if(Num >= 0)
			SetCurrentHost(Num);

		/* 接続中なら切断する */
		if(CmdCtrlSocket != INVALID_SOCKET)
			DisconnectProc();

		SetTaskMsg("----------------------------");

		InitPWDcommand();
		CopyHostFromList(AskCurrentHost(), &CurHost);
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;

		if (ConnectRas(CurHost.Dialup != NO, CurHost.DialupAlways != NO, CurHost.DialupNotify != NO, u8(CurHost.DialEntry))) {
			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			if((AskSaveSortToHost() == YES) && (CurHost.Sort != SORT_NOTSAVED))
			{
				DecomposeSortType(CurHost.Sort, &LFSort, &LDSort, &RFSort, &RDSort);
				SetSortTypeImm(LFSort, LDSort, RFSort, RDSort);
				ReSortDispList(WIN_LOCAL, &CancelFlg);
			}

			Save = NO;
			if(strlen(CurHost.PassWord) > 0)
				Save = YES;

			DisableUserOpe();
			// 暗号化通信対応
			// 同時接続対応
//			CmdCtrlSocket = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, Save, CurHost.Security);
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, Save, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			if(CmdCtrlSocket != INVALID_SOCKET)
			{
				// 暗号化通信対応
				switch(CurHost.CryptMode)
				{
				case CRYPT_NONE:
					if(CurHost.UseFTPIS != NO || CurHost.UseSFTP != NO)
					{
						if(Dialog(GetFtpInst(), savecrypt_dlg, GetMainHwnd()))
							SetHostEncryption(AskCurrentHost(), CurHost.UseNoEncryption, CurHost.UseFTPES, NO, NO);
					}
					break;
				case CRYPT_FTPES:
					if(CurHost.UseNoEncryption != NO || CurHost.UseFTPIS != NO || CurHost.UseSFTP != NO)
					{
						if(Dialog(GetFtpInst(), savecrypt_dlg, GetMainHwnd()))
							SetHostEncryption(AskCurrentHost(), NO, CurHost.UseFTPES, NO, NO);
					}
					break;
				case CRYPT_FTPIS:
					if(CurHost.UseNoEncryption != NO || CurHost.UseFTPES != NO || CurHost.UseSFTP != NO)
					{
						if(Dialog(GetFtpInst(), savecrypt_dlg, GetMainHwnd()))
							SetHostEncryption(AskCurrentHost(), NO, NO, CurHost.UseFTPIS, NO);
					}
					break;
				case CRYPT_SFTP:
					if(CurHost.UseNoEncryption != NO || CurHost.UseFTPES != NO || CurHost.UseFTPIS != NO)
					{
						if(Dialog(GetFtpInst(), savecrypt_dlg, GetMainHwnd()))
							SetHostEncryption(AskCurrentHost(), NO, NO, NO, CurHost.UseSFTP);
					}
					break;
				}

				strcpy(TitleHostName, CurHost.HostName);
				// タイトルバーにユーザー名表示対応
				strcpy(TitleUserName, CurHost.UserName);
				DispWindowTitle();
				UpdateStatusBar();
				Sound::Connected.Play();

				SendInitCommand(CmdCtrlSocket, CurHost.InitCmd, &CancelFlg);

				if(strlen(CurHost.LocalInitDir) > 0)
				{
					DoLocalCWD(CurHost.LocalInitDir);
					GetLocalDirForWnd();
				}
				InitTransCurDir();
				DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

				LoadBookMark();
				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			}
			else
				Sound::Error.Play();

			EnableUserOpe();
		}
		else
			SetTaskMsg(MSGJPN001);
	}
	return;
}


// ホスト名を入力してホストへ接続
void QuickConnectProc() {
	struct QuickCon {
		using result_t = bool;
		std::wstring hostname;
		std::wstring username;
		std::wstring password;
		bool firewall = false;
		bool passive = false;
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, QHOST_HOST, CB_LIMITTEXT, FMAX_PATH, 0);
			SetText(hDlg, QHOST_HOST, L"");
			SendDlgItemMessageW(hDlg, QHOST_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
			if (QuickAnonymous == YES) {
				SetText(hDlg, QHOST_USER, L"anonymous");
				SetText(hDlg, QHOST_PASS, u8(UserMailAdrs));
			} else {
				SetText(hDlg, QHOST_USER, L"");
				SetText(hDlg, QHOST_PASS, L"");
			}
			SendDlgItemMessageW(hDlg, QHOST_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SendDlgItemMessageW(hDlg, QHOST_FWALL, BM_SETCHECK, FwallDefault, 0);
			SendDlgItemMessageW(hDlg, QHOST_PASV, BM_SETCHECK, PasvDefault, 0);
			for (auto const& Tmp : GetHistories())
				SendDlgItemMessageW(hDlg, QHOST_HOST, CB_ADDSTRING, 0, (LPARAM)u8(Tmp.HostAdrs).c_str());
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				hostname = GetText(hDlg, QHOST_HOST);
				username = GetText(hDlg, QHOST_USER);
				password = GetText(hDlg, QHOST_PASS);
				firewall = SendDlgItemMessageW(hDlg, QHOST_FWALL, BM_GETCHECK, 0, 0) == BST_CHECKED;
				passive = SendDlgItemMessageW(hDlg, QHOST_PASV, BM_GETCHECK, 0, 0) == BST_CHECKED;
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			}
		}
	};
	char Tmp[FMAX_PATH+1 + USER_NAME_LEN+1 + PASSWORD_LEN+1 + 2];
	char File[FMAX_PATH+1];

	SaveBookMark();
	SaveCurrentSetToHost();

	if (QuickCon qc; Dialog(GetFtpInst(), hostname_dlg, GetMainHwnd(), qc)) {
		/* 接続中なら切断する */
		if (CmdCtrlSocket != INVALID_SOCKET)
			DisconnectProc();

		SetTaskMsg("----------------------------");

		InitPWDcommand();
		CopyDefaultHost(&CurHost);
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;
		if (strcpy(Tmp, u8(qc.hostname).c_str()); SplitUNCpath(Tmp, CurHost.HostAdrs, CurHost.RemoteInitDir, File, CurHost.UserName, CurHost.PassWord, &CurHost.Port) == FFFTP_SUCCESS) {
			if (strlen(CurHost.UserName) == 0) {
				strcpy(CurHost.UserName, u8(qc.username).c_str());
				strcpy(CurHost.PassWord, u8(qc.password).c_str());
			}

			SetCurrentHost(HOSTNUM_NOENTRY);
			AskUseFireWall(CurHost.HostAdrs, &CurHost.FireWall, &CurHost.Pasv, &CurHost.ListCmdOnly);
			CurHost.FireWall = qc.firewall ? 1 : 0;
			CurHost.Pasv = qc.passive ? 1 : 0;

			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			DisableUserOpe();
			// 暗号化通信対応
			// 同時接続対応
//			CmdCtrlSocket = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security);
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			if (CmdCtrlSocket != INVALID_SOCKET) {
				strcpy(TitleHostName, CurHost.HostAdrs);
				// タイトルバーにユーザー名表示対応
				strcpy(TitleUserName, CurHost.UserName);
				DispWindowTitle();
				UpdateStatusBar();
				Sound::Connected.Play();

				InitTransCurDir();
				DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
				EnableUserOpe();

				if (strlen(File) > 0)
					DirectDownloadProc(File);
			} else {
				Sound::Error.Play();
				EnableUserOpe();
			}
		}
	}
}


/*----- 指定したホスト名でホストへ接続 ----------------------------------------
*
*	Parameter
*		char *unc : UNC文字列
*		int Kanji : ホストの漢字コード (KANJI_xxx)
*		int Kana : 半角かな→全角変換モード (YES/NO)
*		int Fkanji : ファイル名の漢字コード (KANJI_xxx)
*		int TrMode : 転送モード (TYPE_xx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DirectConnectProc(char *unc, int Kanji, int Kana, int Fkanji, int TrMode)
{
	char Host[HOST_ADRS_LEN+1];
	char Path[FMAX_PATH+1];
	char File[FMAX_PATH+1];
	char User[USER_NAME_LEN+1];
	char Pass[PASSWORD_LEN+1];
	int Port;

	SaveBookMark();
	SaveCurrentSetToHost();

	/* 接続中なら切断する */
	if(CmdCtrlSocket != INVALID_SOCKET)
		DisconnectProc();

	SetTaskMsg("----------------------------");

	InitPWDcommand();
	if(SplitUNCpath(unc, Host, Path, File, User, Pass, &Port) == FFFTP_SUCCESS)
	{
		if(strlen(User) == 0)
		{
			strcpy(User, "anonymous");
			strcpy(Pass, UserMailAdrs);
		}

		CopyDefaultHost(&CurHost);

		SetCurrentHost(HOSTNUM_NOENTRY);
		strcpy(CurHost.HostAdrs, Host);
		strcpy(CurHost.UserName, User);
		strcpy(CurHost.PassWord, Pass);
		strcpy(CurHost.RemoteInitDir, Path);
		AskUseFireWall(CurHost.HostAdrs, &CurHost.FireWall, &CurHost.Pasv, &CurHost.ListCmdOnly);
		CurHost.Port = Port;
		CurHost.KanjiCode = Kanji;
		CurHost.KanaCnv = Kana;
		CurHost.NameKanjiCode = Fkanji;
		CurHost.KanaCnv = YES;			/* とりあえず */
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;

		SetHostKanaCnvImm(CurHost.KanaCnv);
		SetHostKanjiCodeImm(CurHost.KanjiCode);
		SetSyncMoveMode(CurHost.SyncMove);

		if(TrMode != TYPE_DEFAULT)
		{
			SetTransferTypeImm(TrMode);
			DispTransferType();
		}

		DisableUserOpe();
		// 暗号化通信対応
		// 同時接続対応
//		CmdCtrlSocket = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security);
		CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
		TrnCtrlSocket = CmdCtrlSocket;

		if(CmdCtrlSocket != INVALID_SOCKET)
		{
			strcpy(TitleHostName, CurHost.HostAdrs);
			// タイトルバーにユーザー名表示対応
			strcpy(TitleUserName, CurHost.UserName);
			DispWindowTitle();
			UpdateStatusBar();
			Sound::Connected.Play();

			InitTransCurDir();
			DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

			GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			EnableUserOpe();

			if(strlen(File) > 0)
				DirectDownloadProc(File);
			else
				ResetAutoExitFlg();
		}
		else
		{
			Sound::Error.Play();
			EnableUserOpe();
		}
	}
	return;
}


/*----- ホストのヒストリで指定されたホストへ接続 ------------------------------
*
*	Parameter
*		int MenuCmd : 取り出すヒストリに割り当てられたメニューコマンド
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void HistoryConnectProc(int MenuCmd)
{
	int LFSort;
	int LDSort;
	int RFSort;
	int RDSort;

	if (auto history = GetHistoryByCmd(MenuCmd)) {
		SaveBookMark();
		SaveCurrentSetToHost();

		/* 接続中なら切断する */
		if(CmdCtrlSocket != INVALID_SOCKET)
			DisconnectProc();

		SetTaskMsg("----------------------------");

		InitPWDcommand();
		CurHost = *history;
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;

		if (ConnectRas(CurHost.Dialup != NO, CurHost.DialupAlways != NO, CurHost.DialupNotify != NO, u8(CurHost.DialEntry))) {
			SetCurrentHost(HOSTNUM_NOENTRY);
			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			DecomposeSortType(CurHost.Sort, &LFSort, &LDSort, &RFSort, &RDSort);
			SetSortTypeImm(LFSort, LDSort, RFSort, RDSort);
			ReSortDispList(WIN_LOCAL, &CancelFlg);

			SetTransferTypeImm(history->Type);
			DispTransferType();

			DisableUserOpe();
			// 暗号化通信対応
			// 同時接続対応
//			CmdCtrlSocket = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security);
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			if(CmdCtrlSocket != INVALID_SOCKET)
			{
				strcpy(TitleHostName, CurHost.HostAdrs);
				// タイトルバーにユーザー名表示対応
				strcpy(TitleUserName, CurHost.UserName);
				DispWindowTitle();
				UpdateStatusBar();
				Sound::Connected.Play();

				SendInitCommand(CmdCtrlSocket, CurHost.InitCmd, &CancelFlg);

				DoLocalCWD(CurHost.LocalInitDir);
				GetLocalDirForWnd();

				InitTransCurDir();
				DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			}
			else
				Sound::Error.Play();

			EnableUserOpe();
		}
		else
			SetTaskMsg(MSGJPN002);
	}
	else
		Sound::Error.Play();

	return;
}


/*----- ホストの初期化コマンドを送る ------------------------------------------
*
*	Parameter
*		int Cmd : 初期化コマンドス
*
*	Return Value
*		なし
*
*	NOte
*		初期化コマンドは以下のようなフォーマットであること
*			cmd1\0
*			cmd1\r\ncmd2\r\n\0
*----------------------------------------------------------------------------*/

// 同時接続対応
//static int SendInitCommand(char *Cmd)
static int SendInitCommand(SOCKET Socket, char *Cmd, int *CancelCheckWork)
{
	char Tmp[INITCMD_LEN+1];
	char *Pos;

	while(strlen(Cmd) > 0)
	{
		strcpy(Tmp, Cmd);
		if((Pos = strchr(Tmp, '\r')) != NULL)
			*Pos = NUL;
		if(strlen(Tmp) > 0)
//			DoQUOTE(Tmp);
			DoQUOTE(Socket, Tmp, CancelCheckWork);

		if((Cmd = strchr(Cmd, '\n')) != NULL)
			Cmd++;
		else
			break;
	}
	return(0);
}


/*----- 指定のホストはFireWallを使う設定かどうかを返す ------------------------
*
*	Parameter
*		char *Hots : ホスト名
*		int *Fire : FireWallを使うかどうかを返すワーク
*		int *Pasv : PASVモードを返すワーク
*		int *List : LISTコマンドのみ使用フラグ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AskUseFireWall(char *Host, int *Fire, int *Pasv, int *List)
{
	int i;
	HOSTDATA Tmp;

	*Fire = FwallDefault;
	*Pasv = PasvDefault;
	// NLSTを送ってしまうバグ修正（ただしNLSTを使うべきホストへクイック接続できなくなる）
//	*List = NO;

	i = 0;
	while(CopyHostFromList(i, &Tmp) == FFFTP_SUCCESS)
	{
		if(strcmp(Host, Tmp.HostAdrs) == 0)
		{
			*Fire = Tmp.FireWall;
			*Pasv = Tmp.Pasv;
			*List = Tmp.ListCmdOnly;
			break;
		}
		i++;
	}
	return;
}


/*----- 接続しているホストのアドレスを返す ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *ホストのアドレス
*----------------------------------------------------------------------------*/

char *AskHostAdrs(void)
{
	return(CurHost.HostAdrs);
}


/*----- 接続しているホストのポートを返す --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ホストのポート
*----------------------------------------------------------------------------*/

int AskHostPort(void)
{
	return(CurHost.Port);
}

/*----- 接続しているホストのファイル名の漢字コードを返す ----------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 漢字コード (KANJI_xxx)
*----------------------------------------------------------------------------*/

int AskHostNameKanji(void)
{
	// UTF-8対応
//	if(AskCurrentHost() != HOSTNUM_NOENTRY)
//		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
//
//	return(CurHost.NameKanjiCode);
	return(CurHost.CurNameKanjiCode);
}


/*----- 接続しているホストのファイル名の半角カナ変換フラグを返す --------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 半角カナを全角に変換するかどうか (YES/NO)
*----------------------------------------------------------------------------*/

int AskHostNameKana(void)
{
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		// 同時接続対応
//		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);

	// 同時接続対応
//	return(CurHost.NameKanaCnv);
	return(TmpHost.NameKanaCnv);
}


/*----- 接続しているホストのLISTコマンドモードを返す --------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int LISTコマンドモード (YES/NO)
*----------------------------------------------------------------------------*/

int AskListCmdMode(void)
{
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(CurHost.HostType == HTYPE_VMS)
		return(YES);
	else
	{
		if(AskCurrentHost() != HOSTNUM_NOENTRY)
			// 同時接続対応
//			CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
			CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);
		// 同時接続対応
//		return(CurHost.ListCmdOnly);
		return(TmpHost.ListCmdOnly);
	}
}


/*----- 接続しているホストでNLST -Rを使うかどうかを返す ------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int NLST -Rを使うかどうか (YES/NO)
*----------------------------------------------------------------------------*/

int AskUseNLST_R(void)
{
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		// 同時接続対応
//		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);

	// 同時接続対応
//	return(CurHost.UseNLST_R);
	return(TmpHost.UseNLST_R);
}


std::string AskHostChmodCmd() {
	HOSTDATA TmpHost = CurHost;
	if (AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);
	return TmpHost.ChmodCmd;
}


/*----- 接続しているホストのタイムゾーンを返す --------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int タイムゾーン
*----------------------------------------------------------------------------*/

int AskHostTimeZone(void)
{
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		// 同時接続対応
//		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);

	// 同時接続対応
//	return(CurHost.TimeZone);
	return(TmpHost.TimeZone);
}


/*----- 接続しているホストのPASVモードを返す ----------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int PASVモードかどうか (YES/NO)
*----------------------------------------------------------------------------*/

int AskPasvMode(void)
{
	return(CurHost.Pasv);
}


std::string AskHostLsName() {
	HOSTDATA TmpHost = CurHost;
	if (AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);
	return TmpHost.LsName;
}


/*----- 接続しているホストのホストタイプを返す --------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *ファイル名／オプション
*----------------------------------------------------------------------------*/

int AskHostType(void)
{
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		// 同時接続対応
//		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);

#if defined(HAVE_TANDEM)
	/* OSS ファイルシステムは UNIX ファイルシステムと同じでいいので AUTO を返す
	   ただし、Guardian ファイルシステムに戻ったときにおかしくならないように
	   CurHost.HostType 変数は更新しない */
	if(CurHost.HostType == HTYPE_TANDEM && Oss == YES)
		return(HTYPE_AUTO);
#endif

	// 同時接続対応
//	return(CurHost.HostType);
	return(TmpHost.HostType);
}


/*----- 接続しているホストはFireWallを使うホストかどうかを返す ----------------
*
*	Parameter
*		なし
*
*	Return Value
*		int FireWallを使うかどうか (YES/NO)
*----------------------------------------------------------------------------*/

int AskHostFireWall(void)
{
	return(CurHost.FireWall);
}


/*----- 接続しているホストでフルパスでファイルアクセスしないかどうかを返す ----
*
*	Parameter
*		なし
*
*	Return Value
*		int フルパスでアクセスしない (YES=フルパス禁止/NO)
*----------------------------------------------------------------------------*/

int AskNoFullPathMode(void)
{
	if(CurHost.HostType == HTYPE_VMS)
		return(YES);
	else
		return(CurHost.NoFullPath);
}


/*----- 接続しているユーザ名を返す --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *ユーザ名
*----------------------------------------------------------------------------*/

char *AskHostUserName(void)
{
	return(CurHost.UserName);
}


/*----- 現在の設定をホストの設定にセットする ----------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*
*	Note
*		カレントディレクトリ、ソート方法をホストの設定にセットする
*----------------------------------------------------------------------------*/

void SaveCurrentSetToHost(void)
{
	int Host;
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(TrnCtrlSocket != INVALID_SOCKET)
	{
		if((Host = AskCurrentHost()) != HOSTNUM_NOENTRY)
		{
			// 同時接続対応
//			CopyHostFromListInConnect(Host, &CurHost);
//			if(CurHost.LastDir == YES)
			CopyHostFromListInConnect(Host, &TmpHost);
			if(TmpHost.LastDir == YES)
				SetHostDir(AskCurrentHost(), AskLocalCurDir().u8string().c_str(), u8(AskRemoteCurDir()).c_str());
			SetHostSort(AskCurrentHost(), AskSortType(ITEM_LFILE), AskSortType(ITEM_LDIR), AskSortType(ITEM_RFILE), AskSortType(ITEM_RDIR));
		}
	}
	return;
}


/*----- 現在の設定をヒストリにセットする --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void SaveCurrentSetToHistory(void)
{
	strcpy(CurHost.LocalInitDir, AskLocalCurDir().u8string().c_str());
	strcpy(CurHost.RemoteInitDir, u8(AskRemoteCurDir()).c_str());

	CurHost.Sort = AskSortType(ITEM_LFILE) * 0x1000000 | AskSortType(ITEM_LDIR) * 0x10000 | AskSortType(ITEM_RFILE) * 0x100 | AskSortType(ITEM_RDIR);

	CurHost.KanjiCode = AskHostKanjiCode();
	CurHost.KanaCnv = AskHostKanaCnv();

	CurHost.SyncMove = AskSyncMoveMode();

	AddHostToHistory(CurHost);
	SetAllHistoryToMenu();

	return;
}


// コマンドコントロールソケットの再接続
int ReConnectCmdSkt() {
	int result;
	if (AskShareProh() == YES && AskTransferNow() == YES) {
		SktShareProh();
		result = FFFTP_SUCCESS;
	} else {
		if (CmdCtrlSocket == TrnCtrlSocket)
			TrnCtrlSocket = INVALID_SOCKET;
		result = ReConnectSkt(&CmdCtrlSocket);
	}
	if (TrnCtrlSocket == INVALID_SOCKET)
		TrnCtrlSocket = CmdCtrlSocket;
	return result;
}


/*----- 転送コントロールソケットの再接続 --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

//int ReConnectTrnSkt(void)
//{
//	return(ReConnectSkt(&TrnCtrlSocket));
//}
// 同時接続対応
int ReConnectTrnSkt(SOCKET *Skt, int *CancelCheckWork)
{
//	char Path[FMAX_PATH+1];
	int Sts;
	// 暗号化通信対応
	HOSTDATA HostData;

	Sts = FFFTP_FAIL;

	SetTaskMsg(MSGJPN003);

//	DisableUserOpe();
	/* 現在のソケットは切断 */
	if(*Skt != INVALID_SOCKET)
		do_closesocket(*Skt);
	/* 再接続 */
	// 暗号化通信対応
	HostData = CurHost;
	if(HostData.CryptMode != CRYPT_NONE)
		HostData.UseNoEncryption = NO;
	if(HostData.CryptMode != CRYPT_FTPES)
		HostData.UseFTPES = NO;
	if(HostData.CryptMode != CRYPT_FTPIS)
		HostData.UseFTPIS = NO;
	if(HostData.CryptMode != CRYPT_SFTP)
		HostData.UseSFTP = NO;
	// UTF-8対応
	HostData.CurNameKanjiCode = HostData.NameKanjiCode;
	// 同時接続対応
	HostData.NoDisplayUI = YES;
	// 暗号化通信対応
	// 同時接続対応
//	if((*Skt = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security)) != INVALID_SOCKET)
	if((*Skt = DoConnect(&HostData, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, CancelCheckWork)) != INVALID_SOCKET)
	{
		SendInitCommand(*Skt, CurHost.InitCmd, CancelCheckWork);
//		AskRemoteCurDir(Path, FMAX_PATH);
//		DoCWD(Path, YES, YES, YES);
		Sts = FFFTP_SUCCESS;
	}
	else
		Sound::Error.Play();

//	EnableUserOpe();
	return(Sts);
}


/*----- 回線の再接続 ----------------------------------------------------------
*
*	Parameter
*		SOCKET *Skt : 接続したソケットを返すワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int ReConnectSkt(SOCKET *Skt)
{
	int Sts;

	Sts = FFFTP_FAIL;

	SetTaskMsg(MSGJPN003);

	DisableUserOpe();
	/* 現在のソケットは切断 */
	if(*Skt != INVALID_SOCKET)
		do_closesocket(*Skt);
	/* 再接続 */
	// 暗号化通信対応
	// 同時接続対応
//	if((*Skt = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security)) != INVALID_SOCKET)
	if((*Skt = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg)) != INVALID_SOCKET)
	{
		SendInitCommand(*Skt, CurHost.InitCmd, &CancelFlg);
		DoCWD(u8(AskRemoteCurDir()).c_str(), YES, YES, YES);
		Sts = FFFTP_SUCCESS;
	}
	else
		Sound::Error.Play();

	EnableUserOpe();
	return(Sts);
}


/*----- コマンドコントロールソケットを返す ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		SOCKET コマンドコントロールソケット
*----------------------------------------------------------------------------*/

SOCKET AskCmdCtrlSkt(void)
{
	return(CmdCtrlSocket);
}


/*----- 転送コントロールソケットを返す ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		SOCKET 転送コントロールソケット
*----------------------------------------------------------------------------*/

SOCKET AskTrnCtrlSkt(void)
{
	return(TrnCtrlSocket);
}


/*----- コマンド／転送コントロールソケットの共有を解除 ------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SktShareProh(void)
{
	if(CmdCtrlSocket == TrnCtrlSocket)
	{

//SetTaskMsg("############### SktShareProh");

		// 同時接続対応
//		CmdCtrlSocket = INVALID_SOCKET;
//		ReConnectSkt(&CmdCtrlSocket);
		if(CurHost.ReuseCmdSkt == YES)
		{
			CurHost.ReuseCmdSkt = NO;
			CmdCtrlSocket = INVALID_SOCKET;
			ReConnectSkt(&CmdCtrlSocket);
		}
	}
	return;
}


/*----- コマンド／転送コントロールソケットの共有が解除されているかチェック ----
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス
*			YES=共有解除/NO=共有
*----------------------------------------------------------------------------*/

int AskShareProh(void)
{
	int Sts;

	Sts = YES;
	// 同時接続対応
//	if(CmdCtrlSocket == TrnCtrlSocket)
	if(CmdCtrlSocket == TrnCtrlSocket || TrnCtrlSocket == INVALID_SOCKET)
		Sts = NO;

	return(Sts);
}


/*----- ホストから切断 --------------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DisconnectProc(void)
{

//SetTaskMsg("############### Disconnect Cmd=%x, Trn=%x", CmdCtrlSocket,TrnCtrlSocket);

	// 同時接続対応
	AbortAllTransfer();

	if((CmdCtrlSocket != INVALID_SOCKET) && (CmdCtrlSocket != TrnCtrlSocket))
	{
		// 同時接続対応
//		DoQUIT(CmdCtrlSocket);
		DoQUIT(CmdCtrlSocket, &CancelFlg);
		DoClose(CmdCtrlSocket);
	}

	if(TrnCtrlSocket != INVALID_SOCKET)
	{
		// 同時接続対応
//		DoQUIT(TrnCtrlSocket);
		DoQUIT(TrnCtrlSocket, &CancelFlg);
		DoClose(TrnCtrlSocket);

		SaveCurrentSetToHistory();

		EraseRemoteDirForWnd();
		SetTaskMsg(MSGJPN004);
	}

	TrnCtrlSocket = INVALID_SOCKET;
	CmdCtrlSocket = INVALID_SOCKET;

	DispWindowTitle();
	UpdateStatusBar();
	MakeButtonsFocus();
	ClearBookMark();

	return;
}


/*----- ソケットが強制切断されたときの処理 ------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DisconnectSet(void)
{
	CmdCtrlSocket = INVALID_SOCKET;
	TrnCtrlSocket = INVALID_SOCKET;

	EraseRemoteDirForWnd();
	DispWindowTitle();
	UpdateStatusBar();
	MakeButtonsFocus();
	SetTaskMsg(MSGJPN005);
	return;
}


/*----- ホストに接続中かどうかを返す ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス (YES/NO)
*----------------------------------------------------------------------------*/

int AskConnecting(void)
{
	int Sts;

	Sts = NO;
	if(TrnCtrlSocket != INVALID_SOCKET)
		Sts = YES;

	return(Sts);
}


#if defined(HAVE_TANDEM)
/*----- 接続している本当のホストのホストタイプを返す --------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *ファイル名／オプション
*----------------------------------------------------------------------------*/

int AskRealHostType(void)
{
	// 同時接続対応
	HOSTDATA TmpHost;
	TmpHost = CurHost;

	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		// 同時接続対応
//		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
		CopyHostFromListInConnect(AskCurrentHost(), &TmpHost);

	// 同時接続対応
//	return(CurHost.HostType);
	return(TmpHost.HostType);
}

/*----- OSS ファイルシステムにアクセスしているかどうかのフラグを変更する ------
*
*	Parameter
*		int ステータス (YES/NO)
*
*	Return Value
*		int ステータス (YES/NO)
*----------------------------------------------------------------------------*/

int SetOSS(int wkOss)
{
	if(Oss != wkOss) {
		if (wkOss == YES) {
			strcpy(CurHost.InitCmd, "OSS");
		} else {
			strcpy(CurHost.InitCmd, "GUARDIAN");
		}
	}
	Oss = wkOss;
	return(Oss);
}

/*----- OSS ファイルシステムにアクセスしているかどうかを返す ------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス (YES/NO)
*----------------------------------------------------------------------------*/

int AskOSS(void)
{
	return(Oss);
}
#endif /* HAVE_TANDEM */


/*----- ホストへ接続する ------------------------------------------------------
*
*	Parameter
*		char *Host : ホスト名
*		char *User : ユーザ名
*		char *Pass : パスワード
*		char *Acct : アカウント
*		int Port : ポート
*		int Fwall : FireWallを使うかどうか (YES/NO)
*		int SavePass : パスワードを再入力した時に保存するかどうか (YES/NO)
*		int Security : セキュリティ (SECURITY_xxx, MDx)
*
*	Return Value
*		SOCKET ソケット
*
*	Note
*		ホスト名、ユーザ名、パスワードが指定されていなかったときは、接続に使用
*		したものをコピーしてかえす
*			char *Host : ホスト名
*			char *User : ユーザ名
*			char *Pass : パスワード
*			char *Acct : アカウント
*
*		FireWallは次のように動作する
*			TYPE1	Connect fire → USER user(f) → PASS pass(f) → SITE host → USER user(h) →      PASS pass(h) → ACCT acct
*			TYPE2	Connect fire → USER user(f) → PASS pass(f) →              USER user(h)@host → PASS pass(h) → ACCT acct
*			TYPE3	Connect fire →                                              USER user(h)@host → PASS pass(h) → ACCT acct
*			TYPE4	Connect fire →                                 OPEN host → USER user(h) →      PASS pass(h) → ACCT acct
*			TYPE5	SOCKS4
*			none	Connect host →                                              USER user(h) →      PASS pass(h) → ACCT acct
*----------------------------------------------------------------------------*/

// 暗号化通信対応
static SOCKET DoConnectCrypt(int CryptMode, HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork)
{
	int Sts;
	int Flg;
	int Anony;
	SOCKET ContSock;
	char Reply[1024];
	int Continue;
	int ReInPass;
	char *Tmp;
	int HostPort;
	static const char *SiteTbl[4] = { "SITE", "site", "OPEN", "open" };
	struct linger LingerOpt;
	struct tcp_keepalive KeepAlive;
	DWORD dwTmp;

	// 暗号化通信対応
	ContSock = INVALID_SOCKET;

	if(CryptMode == CRYPT_NONE || CryptMode == CRYPT_FTPES || CryptMode == CRYPT_FTPIS)
	{
		if(Fwall == YES)
			Fwall = FwallType;
		else
			Fwall = FWALL_NONE;

		TryConnect = YES;
		// 暗号化通信対応
//		CancelFlg = NO;
#if 0
//		WSASetBlockingHook(BlkHookFnc);
#endif

		ContSock = INVALID_SOCKET;

		HostPort = Port;
		Tmp = Host;
		if(((Fwall >= FWALL_FU_FP_SITE) && (Fwall <= FWALL_OPEN)) ||
		   (Fwall == FWALL_SIDEWINDER) ||
		   (Fwall == FWALL_FU_FP))
		{
			Tmp = FwallHost;
			Port = FwallPort;
		}

		if(strlen(Tmp) != 0)
		{
			// 同時接続対応
//			if((ContSock = connectsock(Tmp, Port, "", &CancelFlg)) != INVALID_SOCKET)
			if((ContSock = connectsock(Tmp, Port, "", CancelCheckWork)) != INVALID_SOCKET)
			{
				// バッファを無効
#ifdef DISABLE_CONTROL_NETWORK_BUFFERS
				int BufferSize = 0;
				setsockopt(ContSock, SOL_SOCKET, SO_SNDBUF, (char*)&BufferSize, sizeof(int));
				setsockopt(ContSock, SOL_SOCKET, SO_RCVBUF, (char*)&BufferSize, sizeof(int));
#endif
				if(CryptMode == CRYPT_FTPIS)
				{
					if(AttachSSL(ContSock, INVALID_SOCKET, CancelCheckWork, Host))
					{
						while((Sts = std::get<0>(ReadReplyMessage(ContSock, CancelCheckWork)) / 100) == FTP_PRELIM)
							;
					}
					else
						Sts = FTP_ERROR;
				}
				else
				{
					while((Sts = std::get<0>(ReadReplyMessage(ContSock, CancelCheckWork)) / 100) == FTP_PRELIM)
						;
				}

				if(Sts == FTP_COMPLETE)
				{
					Flg = 1;
					if(setsockopt(ContSock, SOL_SOCKET, SO_OOBINLINE, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
						ReportWSError("setsockopt", WSAGetLastError());
					// データ転送用ソケットのTCP遅延転送が無効されているので念のため
					if(setsockopt(ContSock, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
						ReportWSError("setsockopt", WSAGetLastError());
//#pragma aaa
					Flg = 1;
					if(setsockopt(ContSock, SOL_SOCKET, SO_KEEPALIVE, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
						ReportWSError("setsockopt", WSAGetLastError());
					// 切断対策
					if(TimeOut > 0)
					{
						KeepAlive.onoff = 1;
						KeepAlive.keepalivetime = TimeOut * 1000;
						KeepAlive.keepaliveinterval = 1000;
						if(WSAIoctl(ContSock, SIO_KEEPALIVE_VALS, &KeepAlive, sizeof(struct tcp_keepalive), NULL, 0, &dwTmp, NULL, NULL) == SOCKET_ERROR)
							ReportWSError("WSAIoctl", WSAGetLastError());
					}
					LingerOpt.l_onoff = 1;
					LingerOpt.l_linger = 90;
					if(setsockopt(ContSock, SOL_SOCKET, SO_LINGER, (LPSTR)&LingerOpt, sizeof(LingerOpt)) == SOCKET_ERROR)
						ReportWSError("setsockopt", WSAGetLastError());
///////


					/*===== 認証を行なう =====*/

					Sts = FTP_COMPLETE;
					if((Fwall == FWALL_FU_FP_SITE) ||
					   (Fwall == FWALL_FU_FP_USER) ||
					   (Fwall == FWALL_FU_FP))
					{
						// 同時接続対応
//						if((Sts = command(ContSock, Reply, &CancelFlg, "USER %s", FwallUser) / 100) == FTP_CONTINUE)
						if((Sts = command(ContSock, Reply, CancelCheckWork, "USER %s", FwallUser) / 100) == FTP_CONTINUE)
						{
							CheckOneTimePassword(FwallPass, Reply, FwallSecurity);
							// 同時接続対応
//							Sts = command(ContSock, NULL, &CancelFlg, "PASS %s", Reply) / 100;
							Sts = command(ContSock, NULL, CancelCheckWork, "PASS %s", Reply) / 100;
						}
					}
					else if(Fwall == FWALL_SIDEWINDER)
					{
						// 同時接続対応
//						Sts = command(ContSock, Reply, &CancelFlg, "USER %s:%s%c%s", FwallUser, FwallPass, FwallDelimiter, Host) / 100;
						Sts = command(ContSock, Reply, CancelCheckWork, "USER %s:%s%c%s", FwallUser, FwallPass, FwallDelimiter, Host) / 100;
					}
					if((Sts != FTP_COMPLETE) && (Sts != FTP_CONTINUE))
					{
						SetTaskMsg(MSGJPN006);
						DoClose(ContSock);
						ContSock = INVALID_SOCKET;
					}
					else
					{
						if((Fwall == FWALL_FU_FP_SITE) || (Fwall == FWALL_OPEN))
						{
							Flg = 0;
							if(Fwall == FWALL_OPEN)
								Flg = 2;
							if(FwallLower == YES)
								Flg++;

							if(HostPort == IPPORT_FTP)
								// 同時接続対応
//								Sts = command(ContSock, NULL, &CancelFlg, "%s %s", SiteTbl[Flg], Host) / 100;
								Sts = command(ContSock, NULL, CancelCheckWork, "%s %s", SiteTbl[Flg], Host) / 100;
							else
								// 同時接続対応
//								Sts = command(ContSock, NULL, &CancelFlg, "%s %s %d", SiteTbl[Flg], Host, HostPort) / 100;
								Sts = command(ContSock, NULL, CancelCheckWork, "%s %s %d", SiteTbl[Flg], Host, HostPort) / 100;
						}

						if((Sts != FTP_COMPLETE) && (Sts != FTP_CONTINUE))
						{
							SetTaskMsg(MSGJPN007, Host);
							DoClose(ContSock);
							ContSock = INVALID_SOCKET;
						}
						else
						{
							Anony = NO;
							if (strlen(User) != 0 || HostData->NoDisplayUI == NO && InputDialog(username_dlg, GetMainHwnd(), NULL, User, USER_NAME_LEN+1, &Anony))
							{
								if(Anony == YES)
								{
									strcpy(User, "anonymous");
									strcpy(Pass, UserMailAdrs);
								}

								char Buf[1024];
								if((Fwall == FWALL_FU_FP_USER) || (Fwall == FWALL_USER))
								{
									if(HostPort == IPPORT_FTP)
										sprintf(Buf, "%s%c%s", User, FwallDelimiter, Host);
									else
										sprintf(Buf, "%s%c%s %d", User, FwallDelimiter, Host, HostPort);
								}
								else
									strcpy(Buf, User);

								// FTPES対応
								if(CryptMode == CRYPT_FTPES)
								{
									if(((Sts = command(ContSock, Reply, CancelCheckWork, "AUTH TLS")) == 234 || (Sts = command(ContSock, Reply, CancelCheckWork, "AUTH SSL")) == 234))
									{
										if(AttachSSL(ContSock, INVALID_SOCKET, CancelCheckWork, Host))
										{
											if((Sts = command(ContSock, Reply, CancelCheckWork, "PBSZ 0")) == 200)
											{
												if((Sts = command(ContSock, Reply, CancelCheckWork, "PROT P")) == 200)
												{
												}
												else
													Sts = FTP_ERROR;
											}
											else
												Sts = FTP_ERROR;
										}
										else
											Sts = FTP_ERROR;
									}
									else
										Sts = FTP_ERROR;
								}

								// FTPIS対応
								// "PBSZ 0"と"PROT P"は黙示的に設定されているはずだが念のため
								if(CryptMode == CRYPT_FTPIS)
								{
									if((Sts = command(ContSock, Reply, CancelCheckWork, "PBSZ 0")) == 200)
									{
										if((Sts = command(ContSock, Reply, CancelCheckWork, "PROT P")) == 200)
										{
										}
									}
								}

								ReInPass = NO;
								do
								{
									// FTPES対応
									if(Sts == FTP_ERROR)
										break;
									Continue = NO;
									// 同時接続対応
//									if((Sts = command(ContSock, Reply, &CancelFlg, "USER %s", Buf) / 100) == FTP_CONTINUE)
									if((Sts = command(ContSock, Reply, CancelCheckWork, "USER %s", Buf) / 100) == FTP_CONTINUE)
									{
										if (strlen(Pass) != 0 || HostData->NoDisplayUI == NO && InputDialog(passwd_dlg, GetMainHwnd(), NULL, Pass, PASSWORD_LEN+1))
										{
											CheckOneTimePassword(Pass, Reply, Security);

											/* パスワードがスペース1個の時はパスワードの実体なしとする */
											if(strcmp(Reply, " ") == 0)
												strcpy(Reply, "");

											// 同時接続対応
//											Sts = command(ContSock, NULL, &CancelFlg, "PASS %s", Reply) / 100;
											Sts = command(ContSock, NULL, CancelCheckWork, "PASS %s", Reply) / 100;
											if(Sts == FTP_ERROR)
											{
												strcpy(Pass, "");
												if (HostData->NoDisplayUI == NO && InputDialog(re_passwd_dlg, GetMainHwnd(), NULL, Pass, PASSWORD_LEN+1))
													Continue = YES;
												else
													DoPrintf(L"No password specified.");
												ReInPass = YES;
											}
											else if(Sts == FTP_CONTINUE)
											{
												if (strlen(Acct) != 0 || HostData->NoDisplayUI == NO && InputDialog(account_dlg, GetMainHwnd(), NULL, Acct, ACCOUNT_LEN+1))
												{
													// 同時接続対応
//													Sts = command(ContSock, NULL, &CancelFlg, "ACCT %s", Acct) / 100;
													Sts = command(ContSock, NULL, CancelCheckWork, "ACCT %s", Acct) / 100;
												}
												else
													DoPrintf(L"No account specified");
											}
										}
										else
										{
											Sts = FTP_ERROR;
											DoPrintf(L"No password specified.");
										}
									}
									// FTPES対応
									if(Continue == YES)
										Sts = FTP_COMPLETE;
								}
								while(Continue == YES);
							}
							else
							{
								Sts = FTP_ERROR;
								DoPrintf(L"No user name specified");
							}

							if(Sts != FTP_COMPLETE)
							{
								SetTaskMsg(MSGJPN008);
								DoClose(ContSock);
								ContSock = INVALID_SOCKET;
							}
							else if((SavePass == YES) && (ReInPass == YES))
							{
								if (HostData->NoDisplayUI == NO && Dialog(GetFtpInst(), savepass_dlg, GetMainHwnd()))
									SetHostPassword(AskCurrentHost(), Pass);
							}
						}
					}
				}
				else
				{
//#pragma aaa
					SetTaskMsg(MSGJPN009/*"接続できません(1) %x", ContSock*/);
					DoClose(ContSock);
					ContSock = INVALID_SOCKET;
				}
			}
		}
		else
		{

			if(((Fwall >= FWALL_FU_FP_SITE) && (Fwall <= FWALL_OPEN)) ||
			   (Fwall == FWALL_FU_FP))
				SetTaskMsg(MSGJPN010);
			else
				SetTaskMsg(MSGJPN011);
		}

#if 0
//		WSAUnhookBlockingHook();
#endif
		TryConnect = NO;

		// FEAT対応
		// ホストの機能を確認
		if(ContSock != INVALID_SOCKET)
		{
			if((Sts = command(ContSock, Reply, CancelCheckWork, "FEAT")) == 211)
			{
				// 改行文字はReadReplyMessageで消去されるため区切り文字に空白を使用
				// UTF-8対応
				if(strstr(Reply, " UTF8 "))
					HostData->Feature |= FEATURE_UTF8;
				// MLST対応
				if(strstr(Reply, " MLST ") || strstr(Reply, " MLSD "))
					HostData->Feature |= FEATURE_MLSD;
				// IPv6対応
				if(strstr(Reply, " EPRT ") || strstr(Reply, " EPSV "))
					HostData->Feature |= FEATURE_EPRT | FEATURE_EPSV;
				// ホスト側の日時取得
				if(strstr(Reply, " MDTM "))
					HostData->Feature |= FEATURE_MDTM;
				// ホスト側の日時設定
				if(strstr(Reply, " MFMT "))
					HostData->Feature |= FEATURE_MFMT;
			}
			// UTF-8対応
			if(HostData->CurNameKanjiCode == KANJI_AUTO && (HostData->Feature & FEATURE_UTF8))
			{
				// UTF-8を指定した場合も自動判別を行う
//				if((Sts = command(ContSock, Reply, CancelCheckWork, "OPTS UTF8 ON")) == 200)
//					HostData->CurNameKanjiCode = KANJI_UTF8N;
				command(ContSock, Reply, CancelCheckWork, "OPTS UTF8 ON");
			}
		}
	}

	return(ContSock);
}

// 同時接続対応
//static SOCKET DoConnect(HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security)
static SOCKET DoConnect(HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork)
{
	SOCKET ContSock;
	ContSock = INVALID_SOCKET;
	*CancelCheckWork = NO;
	if(*CancelCheckWork == NO && ContSock == INVALID_SOCKET && HostData->UseFTPIS == YES)
	{
		SetTaskMsg(MSGJPN316);
		if((ContSock = DoConnectCrypt(CRYPT_FTPIS, HostData, Host, User, Pass, Acct, Port, Fwall, SavePass, Security, CancelCheckWork)) != INVALID_SOCKET)
			HostData->CryptMode = CRYPT_FTPIS;
	}
	if(*CancelCheckWork == NO && ContSock == INVALID_SOCKET && HostData->UseFTPES == YES)
	{
		SetTaskMsg(MSGJPN315);
		if((ContSock = DoConnectCrypt(CRYPT_FTPES, HostData, Host, User, Pass, Acct, Port, Fwall, SavePass, Security, CancelCheckWork)) != INVALID_SOCKET)
			HostData->CryptMode = CRYPT_FTPES;
	}
	if(*CancelCheckWork == NO && ContSock == INVALID_SOCKET && HostData->UseNoEncryption == YES)
	{
		SetTaskMsg(MSGJPN314);
		if((ContSock = DoConnectCrypt(CRYPT_NONE, HostData, Host, User, Pass, Acct, Port, Fwall, SavePass, Security, CancelCheckWork)) != INVALID_SOCKET)
			HostData->CryptMode = CRYPT_NONE;
	}
	return ContSock;
}


/*----- ワンタイムパスワードのチェック ----------------------------------------
*
*	Parameter
*		chat *Pass : パスワード／パスフレーズ
*		char *Reply : USERコマンドを送ったあとのリプライ文字列
*						／PASSコマンドで送るパスワードを返すバッファ
*		int Type : タイプ (SECURITY_xxx, MDx)
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*
*	Note
*		ワンタイムパスワードでない時はPassをそのままReplyにコピー
*----------------------------------------------------------------------------*/

static int CheckOneTimePassword(char *Pass, char *Reply, int Type)
{
	int Sts;
	const char* Pos;
	int Seq;
	char Seed[MAX_SEED_LEN+1];
	int i;

	Sts = FFFTP_SUCCESS;
	Pos = NULL;

	if(Type == SECURITY_AUTO)
	{
		if((Pos = stristr(Reply, "otp-md5")) != NULL)
		{
			Type = MD5;
			SetTaskMsg(MSGJPN012);
		}
		else if((Pos = stristr(Reply, "otp-sha1")) != NULL)
		{
			Type = SHA1;
			SetTaskMsg(MSGJPN013);
		}
		else if(((Pos = stristr(Reply, "otp-md4")) != NULL) || ((Pos = stristr(Reply, "s/key")) != NULL))
		{
			Type = MD4;
			SetTaskMsg(MSGJPN014);
		}
	}
	else
		Pos = GetNextField(Reply);

	if((Type == MD4) || (Type == MD5) || (Type == SHA1))
	{
		/* シーケンス番号を見つけるループ */
		DoPrintf(L"Analyze OTP");
		DoPrintf("%s", Pos);
		Sts = FFFTP_FAIL;
		while((Pos = GetNextField(Pos)) != NULL)
		{
			if(IsDigit(*Pos))
			{
				Seq = atoi(Pos);
				DoPrintf(L"Sequence=%d", Seq);

				/* Seed */
				if((Pos = GetNextField(Pos)) != NULL)
				{
					if(GetOneField(Pos, Seed, MAX_SEED_LEN) == FFFTP_SUCCESS)
					{
						/* Seedは英数字のみ有効とする */
						for(i = (int)strlen(Seed)-1; i >= 0; i--)
						{
							if((IsAlpha(Seed[i]) == 0) && (IsDigit(Seed[i]) == 0))
								Seed[i] = NUL;
						}
						if(strlen(Seed) > 0)
						{
							DoPrintf("Seed=%s", Seed);
							Make6WordPass(Seq, Seed, Pass, Type, Reply);
							DoPrintf("Response=%s", Reply);

							/* シーケンス番号のチェックと警告 */
							if(Seq <= 10)
								Dialog(GetFtpInst(), otp_notify_dlg, GetMainHwnd());

							Sts = FFFTP_SUCCESS;
						}
					}
				}
				break;
			}
		}

		if(Sts == FFFTP_FAIL)
			SetTaskMsg(MSGJPN015);
	}
	else
	{
		strcpy(Reply, Pass);
		DoPrintf(L"No OTP used.");
	}
	return(Sts);
}


namespace std {
	template<>
	struct default_delete<addrinfoW> {
		void operator()(addrinfoW* ai) const noexcept {
			FreeAddrInfoW(ai);
		}
	};
}

static inline auto getaddrinfo(std::wstring const& host, std::wstring const& port, int family = AF_UNSPEC, int flags = AI_NUMERICHOST | AI_NUMERICSERV) {
	if (addrinfoW hint{ flags, family }, *ai; GetAddrInfoW(host.c_str(), port.c_str(), &hint, &ai) == 0) {
		assert(family == AF_INET || family == AF_INET6 ? ai->ai_family == family : true);
		assert(ai->ai_family == AF_INET && ai->ai_addrlen == sizeof(sockaddr_in) || ai->ai_family == AF_INET6 && ai->ai_addrlen == sizeof(sockaddr_in6));
		assert(ai->ai_addr->sa_family == ai->ai_family);
		return std::unique_ptr<addrinfoW>{ ai };
	}
	return std::unique_ptr<addrinfoW>{};
}

static inline auto getaddrinfo(std::wstring const& host, int port, int family = AF_UNSPEC, int flags = AI_NUMERICHOST | AI_NUMERICSERV) {
	return getaddrinfo(host, std::to_wstring(port), family, flags);
}

static std::unique_ptr<addrinfoW> getaddrinfo(std::wstring const& host, std::wstring const& port, int family, int* CancelCheckWork) {
	auto future = std::async(std::launch::async, [host, port, family] { return getaddrinfo(IdnToAscii(host), port, family, AI_NUMERICSERV);	});
	while (*CancelCheckWork == NO && future.wait_for(1ms) == std::future_status::timeout)
		if (BackgrndMessageProc() == YES)
			*CancelCheckWork = YES;
	if (*CancelCheckWork == YES)
		return {};
	return future.get();
}


static inline auto getaddrinfo(std::wstring const& host, int port, int family, int* CancelCheckWork) {
	return getaddrinfo(host, std::to_wstring(port), family, CancelCheckWork);
}


enum class SocksCommand : uint8_t {
	Connect = 1,
	Bind = 2,
};


static bool SocksSend(SOCKET s, std::vector<uint8_t> const& buffer, int* CancelCheckWork) {
	if (SendData(s, reinterpret_cast<const char*>(data(buffer)), size_as<int>(buffer), 0, CancelCheckWork) != FFFTP_SUCCESS) {
		SetTaskMsg(MSGJPN033, *reinterpret_cast<const short*>(&buffer[0]));
		return false;
	}
	return true;
}


template<class T>
static inline bool SocksRecv(SOCKET s, T& buffer, int* CancelCheckWork) {
	return ReadNchar(s, reinterpret_cast<char*>(&buffer), sizeof(T), CancelCheckWork) == FFFTP_SUCCESS;
}


// SOCKS5の認証を行う
static bool Socks5Authenticate(SOCKET s, int* CancelCheckWork) {
	// RFC 1928 SOCKS Protocol Version 5
	constexpr uint8_t NO_AUTHENTICATION_REQUIRED = 0;
	constexpr uint8_t USERNAME_PASSWORD = 2;

	std::vector<uint8_t> buffer;
	if (FwallType == FWALL_SOCKS5_NOAUTH)
		buffer = { 5, 1, NO_AUTHENTICATION_REQUIRED };						// VER, NMETHODS, METHODS
	else
		buffer = { 5, 2, NO_AUTHENTICATION_REQUIRED, USERNAME_PASSWORD };	// VER, NMETHODS, METHODS
	#pragma pack(push, 1)
	struct {
		uint8_t VER;
		uint8_t METHOD;
	} reply;
	static_assert(sizeof reply == 2);
	#pragma pack(pop)
	if (!SocksSend(s, buffer, CancelCheckWork) || !SocksRecv(s, reply, CancelCheckWork)) {
		SetTaskMsg(MSGJPN036);
		return false;
	}
	if (reply.METHOD == NO_AUTHENTICATION_REQUIRED)
		DoPrintf(L"SOCKS5 No Authentication");
	else if (reply.METHOD == USERNAME_PASSWORD) {
		// RFC 1929 Username/Password Authentication for SOCKS V5
		DoPrintf(L"SOCKS5 User/Pass Authentication");
		auto ulen = (uint8_t)strlen(FwallUser);
		auto plen = (uint8_t)strlen(FwallPass);
		buffer = { 1 };												// VER
		buffer.push_back(ulen);										// ULEN
		buffer.insert(end(buffer), FwallUser, FwallUser + ulen);	// UNAME
		buffer.push_back(plen);										// PLEN
		buffer.insert(end(buffer), FwallPass, FwallPass + plen);	// PASSWD
		#pragma pack(push, 1)
		struct {
			uint8_t VER;
			uint8_t STATUS;
		} reply;
		static_assert(sizeof reply == 2);
		#pragma pack(pop)
		if (!SocksSend(s, buffer, CancelCheckWork) || !SocksRecv(s, reply, CancelCheckWork) || reply.STATUS != 0) {
			SetTaskMsg(MSGJPN037);
			return false;
		}
	} else {
		SetTaskMsg(MSGJPN036);
		return false;
	}
	return true;
}


// SOCKSのコマンドに対するリプライパケットを受信する
std::optional<sockaddr_storage> SocksReceiveReply(SOCKET s, int* CancelCheckWork) {
	assert(AskHostFireWall() == YES);
	sockaddr_storage ss;
	if (FwallType == FWALL_SOCKS4) {
		#pragma pack(push, 1)
		struct {
			uint8_t VN;
			uint8_t CD;
			USHORT DSTPORT;
			IN_ADDR DSTIP;
		} reply;
		static_assert(sizeof reply == 8);
		#pragma pack(pop)
		if (!SocksRecv(s, reply, CancelCheckWork) || reply.VN != 0 || reply.CD != 90) {
			SetTaskMsg(MSGJPN035);
			return {};
		}
		// from "SOCKS: A protocol for TCP proxy across firewalls"
		// If the DSTIP in the reply is 0 (the value of constant INADDR_ANY), then the client should replace it by the IP address of the SOCKS server to which the cleint is connected.
		if (reply.DSTIP.s_addr == 0) {
			int namelen = sizeof ss;
			getpeername(s, reinterpret_cast<sockaddr*>(&ss), &namelen);
			assert(ss.ss_family == AF_INET && namelen == sizeof(sockaddr_in));
			reinterpret_cast<sockaddr_in&>(ss).sin_port = reply.DSTPORT;
		} else
			reinterpret_cast<sockaddr_in&>(ss) = { AF_INET, reply.DSTPORT, reply.DSTIP };
		return ss;
	} else if (FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER) {
		#pragma pack(push, 1)
		struct {
			uint8_t VER;
			uint8_t REP;
			uint8_t RSV;
			uint8_t ATYP;
		} reply;
		static_assert(sizeof reply == 4);
		#pragma pack(pop)
		if (SocksRecv(s, reply, CancelCheckWork) && reply.VER == 5 && reply.REP == 0) {
			if (reply.ATYP == 1) {
				#pragma pack(push, 1)
				struct {
					IN_ADDR BND_ADDR;
					USHORT BND_PORT;
				} reply;
				static_assert(sizeof reply == 6);
				#pragma pack(pop)
				if (SocksRecv(s, reply, CancelCheckWork)) {
					reinterpret_cast<sockaddr_in&>(ss) = { AF_INET, reply.BND_PORT, reply.BND_ADDR };
					return ss;
				}
			} else if (reply.ATYP == 4) {
				#pragma pack(push, 1)
				struct {
					IN6_ADDR BND_ADDR;
					USHORT BND_PORT;
				} reply;
				static_assert(sizeof reply == 18);
				#pragma pack(pop)
				if (SocksRecv(s, reply, CancelCheckWork)) {
					reinterpret_cast<sockaddr_in6&>(ss) = { AF_INET6, reply.BND_PORT, 0, reply.BND_ADDR };
					return ss;
				}
			}
		}
	}
	SetTaskMsg(MSGJPN034);
	return {};
}


template<class T>
static void append(std::vector<uint8_t>& buffer, T const& data) {
	buffer.insert(end(buffer), reinterpret_cast<const uint8_t*>(&data), reinterpret_cast<const uint8_t*>(&data + 1));
}

static std::optional<sockaddr_storage> SocksRequest(SOCKET s, SocksCommand cmd, std::variant<sockaddr_storage, std::tuple<std::string, int>> const& target, int* CancelCheckWork) {
	assert(AskHostFireWall() == YES);
	std::vector<uint8_t> buffer;
	if (FwallType == FWALL_SOCKS4) {
		auto ss = std::get_if<sockaddr_storage>(&target);
		assert(ss && ss->ss_family == AF_INET);
		auto sin = reinterpret_cast<const sockaddr_in*>(ss);
		buffer = { 4, static_cast<uint8_t>(cmd) };									// VN, CD
		append(buffer, sin->sin_port);												// DSTPORT
		append(buffer, sin->sin_addr);												// DSTIP
		buffer.insert(end(buffer), FwallUser, FwallUser + strlen(FwallUser) + 1);	// USERID, NULL
	} else if (FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER) {
		if (!Socks5Authenticate(s, CancelCheckWork))
			return {};
		buffer = { 5, static_cast<uint8_t>(cmd), 0 };								// VER, CMD, RSV
		std::visit([&buffer](auto const& addr) {
			using type = std::decay_t<decltype(addr)>;
			if constexpr (std::is_same_v<type, sockaddr_storage>) {
				if (addr.ss_family == AF_INET) {
					auto const& sin = reinterpret_cast<sockaddr_in const&>(addr);
					buffer.push_back(1);											// ATYP
					append(buffer, sin.sin_addr);									// DST.ADDR
					append(buffer, sin.sin_port);									// DST.PORT
				} else {
					assert(addr.ss_family == AF_INET6);
					auto const& sin6 = reinterpret_cast<sockaddr_in6 const&>(addr);
					buffer.push_back(4);											// ATYP
					append(buffer, sin6.sin6_addr);									// DST.ADDR
					append(buffer, sin6.sin6_port);									// DST.PORT
				}
			} else if constexpr (std::is_same_v<type, std::tuple<std::string, int>>) {
				auto [host, hport] = addr;
				auto nsport = htons(hport);
				buffer.push_back(3);												// ATYP
				buffer.push_back(size_as<uint8_t>(host));							// DST.ADDR
				buffer.insert(end(buffer), begin(host), end(host));					// DST.ADDR
				append(buffer, nsport);												// DST.PORT
			} else
				static_assert(false_v<type>);
		}, target);
	}
	if (!SocksSend(s, buffer, CancelCheckWork))
		return {};
	return SocksReceiveReply(s, CancelCheckWork);
}


SOCKET connectsock(char *host, int port, char *PreMsg, int *CancelCheckWork) {
	std::variant<sockaddr_storage, std::tuple<std::string, int>> target;
	int Fwall = AskHostFireWall() == YES ? FwallType : FWALL_NONE;
	if (auto ai = getaddrinfo(u8(host), port, Fwall == FWALL_SOCKS4 ? AF_INET : AF_UNSPEC)) {
		// ホスト名がIPアドレスだった
		SetTaskMsg(MSGJPN017, PreMsg, host, u8(AddressPortToString(ai->ai_addr, ai->ai_addrlen)).c_str());
		memcpy(&std::get<sockaddr_storage>(target), ai->ai_addr, ai->ai_addrlen);
	} else if ((Fwall == FWALL_SOCKS5_NOAUTH || Fwall == FWALL_SOCKS5_USER) && FwallResolve == YES) {
		// SOCKS5で名前解決する
		target = std::tuple{ std::string(host), port };
	} else if (ai = getaddrinfo(u8(host), port, Fwall == FWALL_SOCKS4 ? AF_INET : AF_UNSPEC, CancelCheckWork)) {
		// 名前解決に成功
		SetTaskMsg(MSGJPN017, PreMsg, host, u8(AddressPortToString(ai->ai_addr, ai->ai_addrlen)).c_str());
		memcpy(&std::get<sockaddr_storage>(target), ai->ai_addr, ai->ai_addrlen);
	} else {
		// 名前解決に失敗
		SetTaskMsg(MSGJPN019, host);
		return INVALID_SOCKET;
	}

	sockaddr_storage saConnect;
	if (Fwall == FWALL_SOCKS4 || Fwall == FWALL_SOCKS5_NOAUTH || Fwall == FWALL_SOCKS5_USER) {
		// connectで接続する先はSOCKSサーバ
		auto ai = getaddrinfo(u8(FwallHost), FwallPort);
		if (!ai)
			ai = getaddrinfo(u8(FwallHost), FwallPort, AF_UNSPEC, CancelCheckWork);
		if (!ai) {
			SetTaskMsg(MSGJPN021, FwallHost);
			return INVALID_SOCKET;
		}
		memcpy(&saConnect, ai->ai_addr, ai->ai_addrlen);
		SetTaskMsg(MSGJPN022, u8(AddressPortToString(ai->ai_addr, ai->ai_addrlen)).c_str());
	} else {
		// connectで接続するのは接続先のホスト
		saConnect = std::get<sockaddr_storage>(target);
	}

	auto s = do_socket(saConnect.ss_family, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		SetTaskMsg(MSGJPN027);
		return INVALID_SOCKET;
	}
	SetAsyncTableData(s, target);
	if (do_connect(s, reinterpret_cast<const sockaddr*>(&saConnect), sizeof saConnect, CancelCheckWork) == SOCKET_ERROR) {
		SetTaskMsg(MSGJPN026);
		DoClose(s);
		return INVALID_SOCKET;
	}
	if (Fwall == FWALL_SOCKS4 || Fwall == FWALL_SOCKS5_NOAUTH || Fwall == FWALL_SOCKS5_USER) {
		auto result = SocksRequest(s, SocksCommand::Connect, target, CancelCheckWork);
		if (!result) {
			SetTaskMsg(MSGJPN023, -1);
			DoClose(s);
			return INVALID_SOCKET;
		}
		CurHost.CurNetType = result->ss_family == AF_INET ? NTYPE_IPV4 : NTYPE_IPV6;
	} else
		CurHost.CurNetType = saConnect.ss_family == AF_INET ? NTYPE_IPV4 : NTYPE_IPV6;
	SetTaskMsg(MSGJPN025);
	return s;
}


// リッスンソケットを取得
SOCKET GetFTPListenSocket(SOCKET ctrl_skt, int *CancelCheckWork) {
	sockaddr_storage saListen;
	int salen = sizeof saListen;
	if (getsockname(ctrl_skt, reinterpret_cast<sockaddr*>(&saListen), &salen) == SOCKET_ERROR) {
		ReportWSError("getsockname", WSAGetLastError());
		return INVALID_SOCKET;
	}
	auto listen_skt = do_socket(saListen.ss_family, SOCK_STREAM, IPPROTO_TCP);
	if (listen_skt == INVALID_SOCKET) {
		ReportWSError("socket create", WSAGetLastError());
		return INVALID_SOCKET;
	}
	if (AskHostFireWall() == YES && (FwallType == FWALL_SOCKS4 || FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER)) {
		DoPrintf(L"Use SOCKS BIND");
		// Control接続と同じアドレスに接続する
		salen = sizeof saListen;
		if (getpeername(ctrl_skt, reinterpret_cast<sockaddr*>(&saListen), &salen) == SOCKET_ERROR) {
			ReportWSError("getpeername", WSAGetLastError());
			return INVALID_SOCKET;
		}
		if (do_connect(listen_skt, reinterpret_cast<const sockaddr*>(&saListen), salen, CancelCheckWork) == SOCKET_ERROR) {
			return INVALID_SOCKET;
		}
		std::variant<sockaddr_storage, std::tuple<std::string, int>> target;
		GetAsyncTableData(ctrl_skt, target);
		if (auto result = SocksRequest(listen_skt, SocksCommand::Bind, target, CancelCheckWork)) {
			saListen = *result;
		} else {
			SetTaskMsg(MSGJPN023, -1);
			DoClose(listen_skt);
			return INVALID_SOCKET;
		}
	} else {
		DoPrintf(L"Use normal BIND");
		// Control接続と同じアドレス（ただしport=0）でlistenする
		if (saListen.ss_family == AF_INET)
			reinterpret_cast<sockaddr_in&>(saListen).sin_port = 0;
		else
			reinterpret_cast<sockaddr_in6&>(saListen).sin6_port = 0;
		if (bind(listen_skt, reinterpret_cast<const sockaddr*>(&saListen), salen) == SOCKET_ERROR) {
			ReportWSError("bind", WSAGetLastError());
			do_closesocket(listen_skt);
			SetTaskMsg(MSGJPN027);
			return INVALID_SOCKET;
		}
		salen = sizeof saListen;
		if (getsockname(listen_skt, reinterpret_cast<sockaddr*>(&saListen), &salen) == SOCKET_ERROR) {
			ReportWSError("getsockname", WSAGetLastError());
			do_closesocket(listen_skt);
			SetTaskMsg(MSGJPN027);
			return INVALID_SOCKET;
		}
		if (do_listen(listen_skt, 1) != 0) {
			ReportWSError("listen", WSAGetLastError());
			do_closesocket(listen_skt);
			SetTaskMsg(MSGJPN027);
			return INVALID_SOCKET;
		}
		// TODO: IPv6にUPnP NATは無意味なのでは？
		if (IsUPnPLoaded() == YES && UPnPEnabled == YES) {
			auto port = ntohs(saListen.ss_family == AF_INET ? reinterpret_cast<sockaddr_in const&>(saListen).sin_port : reinterpret_cast<sockaddr_in6 const&>(saListen).sin6_port);
			// TODO: UPnP NATで外部アドレスだけ参照しているが、外部ポートが内部ポートと異なる可能性が十分にあるのでは？
			if (char ExtAdrs[40]; AddPortMapping(u8(AddressToString(saListen)).c_str(), port, ExtAdrs) == FFFTP_SUCCESS)
				if (auto ai = getaddrinfo(u8(ExtAdrs), port)) {
					memcpy(&saListen, ai->ai_addr, ai->ai_addrlen);
					SetAsyncTableDataMapPort(listen_skt, port);
				}
		}
	}
	int status;
	if (saListen.ss_family == AF_INET) {
		auto const& sin = reinterpret_cast<sockaddr_in const&>(saListen);
		auto a = reinterpret_cast<const uint8_t*>(&sin.sin_addr), p = reinterpret_cast<const uint8_t*>(&sin.sin_port);
		status = command(ctrl_skt, NULL, CancelCheckWork, "PORT %hhu,%hhu,%hhu,%hhu,%hhu,%hhu", a[0], a[1], a[2], a[3], p[0], p[1]);
	} else {
		auto a = u8(AddressToString(saListen));
		auto p = reinterpret_cast<sockaddr_in6 const&>(saListen).sin6_port;
		status = command(ctrl_skt, NULL, CancelCheckWork, "EPRT |2|%s|%d|", a.c_str(), ntohs(p));
	}
	if (status / 100 != FTP_COMPLETE) {
		SetTaskMsg(MSGJPN031, saListen.ss_family == AF_INET ? "PORT" : "EPRT");
		if (IsUPnPLoaded() == YES)
			if (int port; GetAsyncTableDataMapPort(listen_skt, &port) == YES)
				RemovePortMapping(port);
		do_closesocket(listen_skt);
		return INVALID_SOCKET;
	}
	return listen_skt;
}


/*----- ホストへ接続処理中かどうかを返す---------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス
*			YES/NO
*----------------------------------------------------------------------------*/

int AskTryingConnect(void)
{
	return(TryConnect);
}


int AskUseNoEncryption(void)
{
	return(CurHost.UseNoEncryption);
}

int AskUseFTPES(void)
{
	return(CurHost.UseFTPES);
}

int AskUseFTPIS(void)
{
	return(CurHost.UseFTPIS);
}

int AskUseSFTP(void)
{
	return(CurHost.UseSFTP);
}

char *AskPrivateKey(void)
{
	return(CurHost.PrivateKey);
}

// 同時接続対応
int AskMaxThreadCount(void)
{
	return(CurHost.MaxThreadCount);
}

int AskReuseCmdSkt(void)
{
	return(CurHost.ReuseCmdSkt);
}

// FEAT対応
int AskHostFeature(void)
{
	return(CurHost.Feature);
}

// MLSD対応
int AskUseMLSD(void)
{
	return(CurHost.UseMLSD);
}

// IPv6対応
int AskCurNetType(void)
{
	return(CurHost.CurNetType);
}

// 自動切断対策
int AskNoopInterval(void)
{
	return(CurHost.NoopInterval);
}

// 再転送対応
int AskTransferErrorMode(void)
{
	return(CurHost.TransferErrorMode);
}

int AskTransferErrorNotify(void)
{
	return(CurHost.TransferErrorNotify);
}

// セッションあたりの転送量制限対策
int AskErrorReconnect(void)
{
	return(CurHost.TransferErrorReconnect);
}

// ホスト側の設定ミス対策
int AskNoPasvAdrs(void)
{
	return(CurHost.NoPasvAdrs);
}

