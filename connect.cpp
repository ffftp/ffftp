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

static int SendInitCommand(std::shared_ptr<SocketContext> Socket, std::wstring_view Cmd, int *CancelCheckWork);
static void AskUseFireWall(std::wstring_view Host, int *Fire, int *Pasv, int *List);
static void SaveCurrentSetToHistory(void);
static int ReConnectSkt(std::shared_ptr<SocketContext>& Skt);
static std::shared_ptr<SocketContext> DoConnect(HOSTDATA* HostData, std::wstring const& Host, std::wstring& User, std::wstring& Pass, std::wstring& Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork);
static std::wstring CheckOneTimePassword(std::wstring&& pass, std::wstring const& reply, int Type);

/*===== 外部参照 =====*/

extern std::wstring TitleHostName;
extern int CancelFlg;

/* 設定値 */
std::wstring UserMailAdrs = L"who@example.com"s;
std::wstring FwallHost;
std::wstring FwallUser;
std::wstring FwallPass;
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
static std::shared_ptr<SocketContext> CmdCtrlSocket;
static std::shared_ptr<SocketContext> TrnCtrlSocket;
HOSTDATA CurHost;

#if defined(HAVE_TANDEM)
static int Oss = NO;  /* OSS ファイルシステムへアクセスしている場合は YES */
#endif


// ＵＮＣ文字列を分解する
//   "\"は全て"/"に置き換える
static bool SplitUNCpath(std::wstring&& unc, std::wstring& Host, std::wstring& Path, std::wstring& File, std::wstring& User, std::wstring& Pass, int& Port) {
	static boost::wregex re{ LR"(^(?:([^:@]*)(?::([^@]*))?@)?(?:\[([^\]]*)\]|([^:/]*))(?::([0-9]*))?(.*?)([^/]*)$)" };
	unc = ReplaceAll(std::move(unc), L'\\', L'/');
	if (auto const pos = unc.find(L"//"sv); pos != std::wstring::npos)
		unc.erase(0, pos + 2);
	if (boost::wsmatch m; boost::regex_search(unc, m, re)) {
		User = m[1];
		Pass = m[2].matched ? m[2] : L""s;
		Host = m[m[3].matched ? 3 : 4];
		Port = m[5].matched ? std::stoi(m[5]) : IPPORT_FTP;
		Path = m[6];
		File = m[7];
		return true;
	}
	Host = Path = File = User = Pass = L""s;
	Port = IPPORT_FTP;
	return false;
}


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
		if (CmdCtrlSocket)
			DisconnectProc();

		Notice(IDS_SEPARATOR);

		InitPWDcommand();
		CopyHostFromList(AskCurrentHost(), &CurHost);
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;

		if (ConnectRas(CurHost.Dialup != NO, CurHost.DialupAlways != NO, CurHost.DialupNotify != NO, CurHost.DialEntry)) {
			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			if((AskSaveSortToHost() == YES) && (CurHost.Sort != SORT_NOTSAVED))
			{
				DecomposeSortType(CurHost.Sort, &LFSort, &LDSort, &RFSort, &RDSort);
				SetSortTypeImm(LFSort, LDSort, RFSort, RDSort);
				ReSortDispList(WIN_LOCAL, &CancelFlg);
			}

			int Save = empty(CurHost.PassWord) ? NO : YES;

			DisableUserOpe();
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, Save, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			if (CmdCtrlSocket) {
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

				TitleHostName = CurHost.HostName;
				DispWindowTitle();
				UpdateStatusBar();
				Sound::Connected.Play();

				SendInitCommand(CmdCtrlSocket, CurHost.InitCmd, &CancelFlg);

				if (!empty(CurHost.LocalInitDir)) {
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
			Notice(IDS_MSGJPN001);
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
				SetText(hDlg, QHOST_PASS, UserMailAdrs);
			} else {
				SetText(hDlg, QHOST_USER, L"");
				SetText(hDlg, QHOST_PASS, L"");
			}
			SendDlgItemMessageW(hDlg, QHOST_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SendDlgItemMessageW(hDlg, QHOST_FWALL, BM_SETCHECK, FwallDefault, 0);
			SendDlgItemMessageW(hDlg, QHOST_PASV, BM_SETCHECK, PasvDefault, 0);
			for (auto const& Tmp : GetHistories())
				SendDlgItemMessageW(hDlg, QHOST_HOST, CB_ADDSTRING, 0, (LPARAM)Tmp.HostAdrs.c_str());
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

	SaveBookMark();
	SaveCurrentSetToHost();

	if (QuickCon qc; Dialog(GetFtpInst(), hostname_dlg, GetMainHwnd(), qc)) {
		/* 接続中なら切断する */
		if (CmdCtrlSocket)
			DisconnectProc();

		Notice(IDS_SEPARATOR);

		InitPWDcommand();
		CopyDefaultHost(&CurHost);
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;
		std::wstring File;
		if (SplitUNCpath(std::move(qc.hostname), CurHost.HostAdrs, CurHost.RemoteInitDir, File, CurHost.UserName, CurHost.PassWord, CurHost.Port)) {
			if (empty(CurHost.UserName)) {
				CurHost.UserName = qc.username;
				CurHost.PassWord = qc.password;
			}

			SetCurrentHost(HOSTNUM_NOENTRY);
			AskUseFireWall(CurHost.HostAdrs, &CurHost.FireWall, &CurHost.Pasv, &CurHost.ListCmdOnly);
			CurHost.FireWall = qc.firewall ? 1 : 0;
			CurHost.Pasv = qc.passive ? 1 : 0;

			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			DisableUserOpe();
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			if (CmdCtrlSocket) {
				TitleHostName = CurHost.HostAdrs;
				DispWindowTitle();
				UpdateStatusBar();
				Sound::Connected.Play();

				InitTransCurDir();
				DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
				EnableUserOpe();

				if (!empty(File))
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

void DirectConnectProc(std::wstring&& unc, int Kanji, int Kana, int Fkanji, int TrMode)
{
	std::wstring Host;
	std::wstring Path;
	std::wstring File;
	std::wstring User;
	std::wstring Pass;
	int Port;

	SaveBookMark();
	SaveCurrentSetToHost();

	/* 接続中なら切断する */
	if (CmdCtrlSocket)
		DisconnectProc();

	Notice(IDS_SEPARATOR);

	InitPWDcommand();
	if (SplitUNCpath(std::move(unc), Host, Path, File, User, Pass, Port)) {
		if (empty(User)) {
			User = L"anonymous"s;
			Pass = UserMailAdrs;
		}

		CopyDefaultHost(&CurHost);

		SetCurrentHost(HOSTNUM_NOENTRY);
		CurHost.HostAdrs = std::move(Host);
		CurHost.UserName = std::move(User);
		CurHost.PassWord = std::move(Pass);
		CurHost.RemoteInitDir = std::move(Path);
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
		CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
		TrnCtrlSocket = CmdCtrlSocket;

		if (CmdCtrlSocket) {
			TitleHostName = CurHost.HostAdrs;
			DispWindowTitle();
			UpdateStatusBar();
			Sound::Connected.Play();

			InitTransCurDir();
			DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

			GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			EnableUserOpe();

			if (!empty(File))
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
		if (CmdCtrlSocket)
			DisconnectProc();

		Notice(IDS_SEPARATOR);

		InitPWDcommand();
		CurHost = *history;
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;

		if (ConnectRas(CurHost.Dialup != NO, CurHost.DialupAlways != NO, CurHost.DialupNotify != NO, CurHost.DialEntry)) {
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
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			if (CmdCtrlSocket) {
				TitleHostName = CurHost.HostAdrs;
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
			Notice(IDS_MSGJPN002);
	}
	else
		Sound::Error.Play();

	return;
}


// ホストの初期化コマンドを送る
//   初期化コマンドは以下のようなフォーマットであること
//     cmd1\0
//     cmd1\r\ncmd2\r\n\0
static int SendInitCommand(std::shared_ptr<SocketContext> Socket, std::wstring_view Cmd, int* CancelCheckWork) {
	static boost::wregex re{ LR"([^\r\n]+)" };
	for (boost::regex_iterator<std::wstring_view::const_iterator> it{ begin(Cmd), end(Cmd), re }, end; it != end; ++it)
		DoQUOTE(Socket, sv((*it)[0]), CancelCheckWork);
	return 0;
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

static void AskUseFireWall(std::wstring_view Host, int *Fire, int *Pasv, int *List)
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
		if (Host == Tmp.HostAdrs) {
			*Fire = Tmp.FireWall;
			*Pasv = Tmp.Pasv;
			*List = Tmp.ListCmdOnly;
			break;
		}
		i++;
	}
	return;
}


// 接続しているホストのアドレスを返す
std::wstring_view AskHostAdrs() {
	return CurHost.HostAdrs;
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


std::wstring AskHostChmodCmd() {
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


std::wstring AskHostLsName() {
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

	if (TrnCtrlSocket) {
		if((Host = AskCurrentHost()) != HOSTNUM_NOENTRY)
		{
			// 同時接続対応
//			CopyHostFromListInConnect(Host, &CurHost);
//			if(CurHost.LastDir == YES)
			CopyHostFromListInConnect(Host, &TmpHost);
			if(TmpHost.LastDir == YES)
				SetHostDir(AskCurrentHost(), AskLocalCurDir().native(), AskRemoteCurDir());
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
	CurHost.LocalInitDir = AskLocalCurDir();
	CurHost.RemoteInitDir = AskRemoteCurDir();

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
			TrnCtrlSocket.reset();
		result = ReConnectSkt(CmdCtrlSocket);
	}
	if (!TrnCtrlSocket)
		TrnCtrlSocket = CmdCtrlSocket;
	return result;
}


// 転送コントロールソケットの再接続
int ReConnectTrnSkt(std::shared_ptr<SocketContext>& Skt, int *CancelCheckWork) {
//	char Path[FMAX_PATH+1];
	int Sts;
	// 暗号化通信対応
	HOSTDATA HostData;

	Sts = FFFTP_FAIL;

	Notice(IDS_MSGJPN003);

//	DisableUserOpe();
	/* 現在のソケットは切断 */
	if (Skt)
		Skt->Close();
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
	if (Skt = DoConnect(&HostData, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, CancelCheckWork)) {
		SendInitCommand(Skt, CurHost.InitCmd, CancelCheckWork);
		Sts = FFFTP_SUCCESS;
	}
	else
		Sound::Error.Play();

//	EnableUserOpe();
	return(Sts);
}


// 回線の再接続
static int ReConnectSkt(std::shared_ptr<SocketContext>& Skt) {
	int Sts;

	Sts = FFFTP_FAIL;

	Notice(IDS_MSGJPN003);

	DisableUserOpe();
	/* 現在のソケットは切断 */
	if (Skt)
		Skt->Close();
	/* 再接続 */
	if (Skt = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg)) {
		SendInitCommand(Skt, CurHost.InitCmd, &CancelFlg);
		DoCWD(AskRemoteCurDir(), YES, YES, YES);
		Sts = FFFTP_SUCCESS;
	}
	else
		Sound::Error.Play();

	EnableUserOpe();
	return(Sts);
}


// コマンドコントロールソケットを返す
std::shared_ptr<SocketContext> AskCmdCtrlSkt() {
	return CmdCtrlSocket;
}


// 転送コントロールソケットを返す
std::shared_ptr<SocketContext> AskTrnCtrlSkt() {
	return TrnCtrlSocket;
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
		if(CurHost.ReuseCmdSkt == YES)
		{
			CurHost.ReuseCmdSkt = NO;
			CmdCtrlSocket.reset();
			ReConnectSkt(CmdCtrlSocket);
		}
	}
	return;
}


// コマンド／転送コントロールソケットの共有が解除されているかチェック
//   YES=共有解除/NO=共有
int AskShareProh() {
	return CmdCtrlSocket == TrnCtrlSocket || !TrnCtrlSocket ? NO : YES;
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
	AbortAllTransfer();

	if (CmdCtrlSocket && CmdCtrlSocket != TrnCtrlSocket) {
		DoQUIT(CmdCtrlSocket, &CancelFlg);
		DoClose(CmdCtrlSocket);
	}

	if (TrnCtrlSocket) {
		DoQUIT(TrnCtrlSocket, &CancelFlg);
		DoClose(TrnCtrlSocket);

		SaveCurrentSetToHistory();

		EraseRemoteDirForWnd();
		Notice(IDS_MSGJPN004);
	}

	TrnCtrlSocket.reset();
	CmdCtrlSocket.reset();

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
	CmdCtrlSocket.reset();
	TrnCtrlSocket.reset();

	EraseRemoteDirForWnd();
	DispWindowTitle();
	UpdateStatusBar();
	MakeButtonsFocus();
	Notice(IDS_MSGJPN005);
	return;
}


// ホストに接続中かどうかを返す
int AskConnecting() {
	return TrnCtrlSocket ? YES : NO;
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
	if(Oss != wkOss)
		CurHost.InitCmd = wkOss == YES ? L"OSS"sv : L"GUARDIAN"sv;
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
static std::shared_ptr<SocketContext> DoConnectCrypt(int CryptMode, HOSTDATA* HostData, std::wstring const& Host, std::wstring& User, std::wstring& Pass, std::wstring& Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork)
{
	int Sts;
	int Flg;
	int Anony;
	std::shared_ptr<SocketContext> ContSock;
	int Continue;
	int ReInPass;
	constexpr std::wstring_view SiteTbl[] = { L"SITE"sv, L"site"sv, L"OPEN"sv, L"open"sv };
	struct linger LingerOpt;
	struct tcp_keepalive KeepAlive;
	DWORD dwTmp;

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

		auto [tempHost, tempPort] = FWALL_FU_FP_SITE <= Fwall && Fwall <= FWALL_OPEN || Fwall == FWALL_SIDEWINDER || Fwall == FWALL_FU_FP ? std::tuple{ FwallHost, FwallPort } : std::tuple{ Host, Port };
		if (!empty(tempHost)) {
			if (ContSock = connectsock(std::move(tempHost), tempPort, 0, CancelCheckWork)) {
				// バッファを無効
#ifdef DISABLE_CONTROL_NETWORK_BUFFERS
				int BufferSize = 0;
				setsockopt(ContSock->handle, SOL_SOCKET, SO_SNDBUF, (char*)&BufferSize, sizeof(int));
				setsockopt(ContSock->handle, SOL_SOCKET, SO_RCVBUF, (char*)&BufferSize, sizeof(int));
#endif
				if(CryptMode == CRYPT_FTPIS)
				{
					if(AttachSSL(ContSock->handle, INVALID_SOCKET, CancelCheckWork, Host))
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
					if(setsockopt(ContSock->handle, SOL_SOCKET, SO_OOBINLINE, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
						WSAError(L"setsockopt(SOL_SOCKET, SO_OOBINLINE)"sv);
					// データ転送用ソケットのTCP遅延転送が無効されているので念のため
					if(setsockopt(ContSock->handle, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
						WSAError(L"setsockopt(IPPROTO_TCP, TCP_NODELAY)"sv);
//#pragma aaa
					Flg = 1;
					if(setsockopt(ContSock->handle, SOL_SOCKET, SO_KEEPALIVE, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
						WSAError(L"setsockopt(SOL_SOCKET, SO_KEEPALIVE)"sv);
					// 切断対策
					if(TimeOut > 0)
					{
						KeepAlive.onoff = 1;
						KeepAlive.keepalivetime = TimeOut * 1000;
						KeepAlive.keepaliveinterval = 1000;
						if(WSAIoctl(ContSock->handle, SIO_KEEPALIVE_VALS, &KeepAlive, sizeof(struct tcp_keepalive), NULL, 0, &dwTmp, NULL, NULL) == SOCKET_ERROR)
							WSAError(L"WSAIoctl(SIO_KEEPALIVE_VALS)"sv);
					}
					LingerOpt.l_onoff = 1;
					LingerOpt.l_linger = 90;
					if(setsockopt(ContSock->handle, SOL_SOCKET, SO_LINGER, (LPSTR)&LingerOpt, sizeof(LingerOpt)) == SOCKET_ERROR)
						WSAError(L"setsockopt(SOL_SOCKET, SO_LINGER)"sv);
///////


					/*===== 認証を行なう =====*/

					Sts = FTP_COMPLETE;
					if((Fwall == FWALL_FU_FP_SITE) ||
					   (Fwall == FWALL_FU_FP_USER) ||
					   (Fwall == FWALL_FU_FP))
					{
						if (auto [code, text] = Command(ContSock, CancelCheckWork, L"USER {}"sv, FwallUser); (Sts = code / 100) == FTP_CONTINUE) {
							auto const pass = CheckOneTimePassword(std::wstring{ FwallPass }, text, FwallSecurity);
							Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"PASS {}"sv, pass)) / 100;
						}
					}
					else if(Fwall == FWALL_SIDEWINDER)
					{
						Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"USER {}:{}{}{}"sv, FwallUser, FwallPass, (wchar_t)FwallDelimiter, Host)) / 100;
					}
					if((Sts != FTP_COMPLETE) && (Sts != FTP_CONTINUE))
					{
						Notice(IDS_MSGJPN006);
						DoClose(ContSock);
						ContSock.reset();
					}
					else
					{
						if((Fwall == FWALL_FU_FP_SITE) || (Fwall == FWALL_OPEN))
						{
							int index = (Fwall == FWALL_OPEN ? 2 : 0) | (FwallLower == YES ? 1 : 0);
							auto format = Port == IPPORT_FTP ? L"{} {}"sv : L"{} {} {}"sv;
							Sts = std::get<0>(Command(ContSock, CancelCheckWork, format, SiteTbl[index], Host)) / 100;
						}

						if((Sts != FTP_COMPLETE) && (Sts != FTP_CONTINUE))
						{
							Notice(IDS_MSGJPN007, Host);
							DoClose(ContSock);
							ContSock.reset();
						}
						else
						{
							Anony = NO;
							if (empty(User) && HostData->NoDisplayUI == NO)
								InputDialog(username_dlg, GetMainHwnd(), 0, User, USER_NAME_LEN + 1, &Anony);
							if (!empty(User)) {
								if(Anony == YES)
								{
									User = L"anonymous"s;
									Pass = UserMailAdrs;
								}

								auto const user = Fwall != FWALL_FU_FP_USER && Fwall != FWALL_USER ? User : std::format(Port == IPPORT_FTP ? L"{}{}{}"sv : L"{}{}{} {}"sv, User, (wchar_t)FwallDelimiter, Host, Port);

								// FTPES対応
								if (CryptMode == CRYPT_FTPES) {
									if ((Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"AUTH TLS"sv))) != 234 && (Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"AUTH SSL"sv))) != 234)
										Sts = FTP_ERROR;
									else if (!AttachSSL(ContSock->handle, INVALID_SOCKET, CancelCheckWork, Host))
										Sts = FTP_ERROR;
									else if ((Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"PBSZ 0"sv))) != 200)
										Sts = FTP_ERROR;
									else if ((Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"PROT P"sv))) != 200)
										Sts = FTP_ERROR;
								}

								// FTPIS対応
								if (CryptMode == CRYPT_FTPIS) {
									// "PBSZ 0"と"PROT P"は黙示的に設定されているはずだが念のため
									if ((Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"PBSZ 0"sv))) == 200)
										Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"PROT P"sv));
								}

								ReInPass = NO;
								do
								{
									// FTPES対応
									if(Sts == FTP_ERROR)
										break;
									Continue = NO;
									if (auto [code, text] = Command(ContSock, CancelCheckWork, L"USER {}"sv, user); (Sts = code / 100) == FTP_CONTINUE)
									{
										if (empty(Pass) && HostData->NoDisplayUI == NO)
											InputDialog(passwd_dlg, GetMainHwnd(), 0, Pass, PASSWORD_LEN + 1);
										if (!empty(Pass)) {
											auto pass = CheckOneTimePassword(std::wstring{ Pass }, text, Security);

											/* パスワードがスペース1個の時はパスワードの実体なしとする */
											if (pass == L" "sv)
												pass = L""s;
											Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"PASS {}"sv, pass)) / 100;
											if(Sts == FTP_ERROR)
											{
												if (HostData->NoDisplayUI == NO && InputDialog(re_passwd_dlg, GetMainHwnd(), 0, Pass, PASSWORD_LEN + 1))
													Continue = YES;
												else {
													Pass.clear();
													Debug(L"No password specified."sv);
												}
												ReInPass = YES;
											}
											else if(Sts == FTP_CONTINUE)
											{
												if (empty(Acct) && HostData->NoDisplayUI == NO)
													InputDialog(account_dlg, GetMainHwnd(), 0, Acct, ACCOUNT_LEN + 1);
												if (!empty(Acct))
													Sts = std::get<0>(Command(ContSock, CancelCheckWork, L"ACCT {}", Acct)) / 100;
												else
													Debug(L"No account specified."sv);
											}
										}
										else
										{
											Sts = FTP_ERROR;
											Debug(L"No password specified."sv);
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
								Debug(L"No user name specified."sv);
							}

							if(Sts != FTP_COMPLETE)
							{
								Notice(IDS_MSGJPN008);
								DoClose(ContSock);
								ContSock.reset();
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
				Notice(IDS_MSGJPN009);
					DoClose(ContSock);
					ContSock.reset();
				}
			}
		}
		else
		{

			if(((Fwall >= FWALL_FU_FP_SITE) && (Fwall <= FWALL_OPEN)) ||
			   (Fwall == FWALL_FU_FP))
				Notice(IDS_MSGJPN010);
			else
				Notice(IDS_MSGJPN011);
		}

#if 0
//		WSAUnhookBlockingHook();
#endif
		TryConnect = NO;

		// FEAT対応
		// ホストの機能を確認
		if (ContSock) {
			if (auto [code, text] = Command(ContSock, CancelCheckWork, L"FEAT"sv); code == 211) {
				// 改行文字はReadReplyMessageで消去されるため区切り文字に空白を使用
				if (text.find(L" UTF8 "sv) != std::wstring::npos)
					HostData->Feature |= FEATURE_UTF8;
				if (text.find(L" MLST "sv) != std::wstring::npos || text.find(L" MLSD "sv) != std::wstring::npos)
					HostData->Feature |= FEATURE_MLSD;
				if (text.find(L" EPRT "sv) != std::wstring::npos || text.find(L" EPSV "sv) != std::wstring::npos)
					HostData->Feature |= FEATURE_EPRT | FEATURE_EPSV;
				if (text.find(L" MDTM "sv) != std::wstring::npos)
					HostData->Feature |= FEATURE_MDTM;
				if (text.find(L" MFMT "sv) != std::wstring::npos)
					HostData->Feature |= FEATURE_MFMT;
			}
			if (HostData->CurNameKanjiCode == KANJI_AUTO && (HostData->Feature & FEATURE_UTF8))
				Command(ContSock, CancelCheckWork, L"OPTS UTF8 ON"sv);
		}
	}

	return(ContSock);
}

static std::shared_ptr<SocketContext> DoConnect(HOSTDATA* HostData, std::wstring const& Host, std::wstring& User, std::wstring& Pass, std::wstring& Acct, int Port, int Fwall, int SavePass, int Security, int* CancelCheckWork) {
	constexpr struct {
		int HOSTDATA::* ptr;
		int msgid;
		int cryptMode;
	} pattern[] = {
		{ &HOSTDATA::UseFTPIS, IDS_MSGJPN316, CRYPT_FTPIS },
		{ &HOSTDATA::UseFTPES, IDS_MSGJPN315, CRYPT_FTPES },
		{ &HOSTDATA::UseNoEncryption, IDS_MSGJPN314, CRYPT_NONE },
	};
	std::shared_ptr<SocketContext> ContSock;
	*CancelCheckWork = NO;
	for (auto [ptr, msgid, cryptMode] : pattern)
		if (*CancelCheckWork == NO && !ContSock && HostData->*ptr == YES) {
			Notice(msgid);
			if (ContSock = DoConnectCrypt(cryptMode, HostData, Host, User, Pass, Acct, Port, Fwall, SavePass, Security, CancelCheckWork))
				HostData->CryptMode = cryptMode;
		}
	return ContSock;
}


// ワンタイムパスワードのチェック
//   Reply : USERコマンドを送ったあとのリプライ文字列／PASSコマンドで送るパスワードを返すバッファ
//   ワンタイムパスワードでない時はPassをそのままReplyにコピー
static std::wstring CheckOneTimePassword(std::wstring&& pass, std::wstring const& reply, int Type) {
	Debug(L"OTP: {}"sv, reply);

	auto pos = begin(reply);
	if (Type == SECURITY_AUTO) {
		static boost::wregex re{ LR"((otp-md5)|(otp-sha1)|otp-md4|s/key)", boost::regex_constants::icase };
		if (boost::wsmatch m; boost::regex_search(reply, m, re)) {
			if (m[1].matched) {
				Type = MD5;
				Notice(IDS_MSGJPN012);
			} else if (m[2].matched) {
				Type = SHA1;
				Notice(IDS_MSGJPN013);
			} else {
				Type = MD4;
				Notice(IDS_MSGJPN014);
			}
			pos = m[0].first;
		}
	} else {
		static boost::wregex re{ LR"([^ ]* +)" };
		if (boost::wsmatch m; boost::regex_search(reply, m, re))
			pos = m[0].second;
	}

	if (Type != MD4 && Type != MD5 && Type != SHA1) {
		Debug(L"No OTP used.");
		return std::move(pass);
	}

	static boost::wregex re{ LR"( +(\d+)[^ ]* +([A-Za-z0-9]*))" };
	if (boost::wsmatch m; boost::regex_search(pos, end(reply), m, re) && 0 < m[2].length()) {
		auto seq = std::stoi(m[1]);
		auto seed = u8(sv(m[2]));
		pass =  u8(Make6WordPass(seq, seed, u8(pass), Type));
		Debug(L"OPT Reponse={}"sv, pass);
		/* シーケンス番号のチェックと警告 */
		if (seq <= 10)
			Dialog(GetFtpInst(), otp_notify_dlg, GetMainHwnd());
		return std::move(pass);
	}
	Notice(IDS_MSGJPN015);
	return L""s;
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


static bool SocksSend(std::shared_ptr<SocketContext> s, std::vector<uint8_t> const& buffer, int* CancelCheckWork) {
	if (SendData(s->handle, reinterpret_cast<const char*>(data(buffer)), size_as<int>(buffer), 0, CancelCheckWork) != FFFTP_SUCCESS) {
		Notice(IDS_MSGJPN033, *reinterpret_cast<const unsigned short*>(&buffer[0]));
		return false;
	}
	return true;
}


template<class T>
static inline bool SocksRecv(std::shared_ptr<SocketContext> s, T& buffer, int* CancelCheckWork) {
	return ReadNchar(s, reinterpret_cast<char*>(&buffer), sizeof(T), CancelCheckWork) == FFFTP_SUCCESS;
}


// SOCKS5の認証を行う
static bool Socks5Authenticate(std::shared_ptr<SocketContext> s, int* CancelCheckWork) {
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
		Notice(IDS_MSGJPN036);
		return false;
	}
	if (reply.METHOD == NO_AUTHENTICATION_REQUIRED)
		Debug(L"SOCKS5 No Authentication."sv);
	else if (reply.METHOD == USERNAME_PASSWORD) {
		// RFC 1929 Username/Password Authentication for SOCKS V5
		Debug(L"SOCKS5 User/Pass Authentication."sv);
		auto const uname = u8(FwallUser);
		auto const passwd = u8(FwallPass);
		buffer = { 1 };												// VER
		buffer.push_back(size_as<uint8_t>(uname));					// ULEN
		buffer.insert(end(buffer), begin(uname), end(uname));		// UNAME
		buffer.push_back(size_as<uint8_t>(passwd));					// PLEN
		buffer.insert(end(buffer), begin(passwd), end(passwd));		// PASSWD
		#pragma pack(push, 1)
		struct {
			uint8_t VER;
			uint8_t STATUS;
		} reply;
		static_assert(sizeof reply == 2);
		#pragma pack(pop)
		if (!SocksSend(s, buffer, CancelCheckWork) || !SocksRecv(s, reply, CancelCheckWork) || reply.STATUS != 0) {
			Notice(IDS_MSGJPN037);
			return false;
		}
	} else {
		Notice(IDS_MSGJPN036);
		return false;
	}
	return true;
}


// SOCKSのコマンドに対するリプライパケットを受信する
std::optional<sockaddr_storage> SocksReceiveReply(std::shared_ptr<SocketContext> s, int* CancelCheckWork) {
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
			Notice(IDS_MSGJPN035);
			return {};
		}
		// from "SOCKS: A protocol for TCP proxy across firewalls"
		// If the DSTIP in the reply is 0 (the value of constant INADDR_ANY), then the client should replace it by the IP address of the SOCKS server to which the cleint is connected.
		if (reply.DSTIP.s_addr == 0) {
			int namelen = sizeof ss;
			getpeername(s->handle, reinterpret_cast<sockaddr*>(&ss), &namelen);
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
	Notice(IDS_MSGJPN034);
	return {};
}


template<class T>
static void append(std::vector<uint8_t>& buffer, T const& data) {
	buffer.insert(end(buffer), reinterpret_cast<const uint8_t*>(&data), reinterpret_cast<const uint8_t*>(&data + 1));
}

static std::optional<sockaddr_storage> SocksRequest(std::shared_ptr<SocketContext> s, SocksCommand cmd, std::variant<sockaddr_storage, std::tuple<std::wstring, int>> const& target, int* CancelCheckWork) {
	assert(AskHostFireWall() == YES);
	std::vector<uint8_t> buffer;
	if (FwallType == FWALL_SOCKS4) {
		auto ss = std::get_if<sockaddr_storage>(&target);
		assert(ss && ss->ss_family == AF_INET);
		auto sin = reinterpret_cast<const sockaddr_in*>(ss);
		auto const userid = u8(FwallUser);
		buffer = { 4, static_cast<uint8_t>(cmd) };									// VN, CD
		append(buffer, sin->sin_port);												// DSTPORT
		append(buffer, sin->sin_addr);												// DSTIP
		buffer.insert(end(buffer), data(userid), data(userid) + size(userid) + 1);	// USERID, NULL
	} else if (FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER) {
		if (!Socks5Authenticate(s, CancelCheckWork))
			return {};
		buffer = { 5, static_cast<uint8_t>(cmd), 0 };								// VER, CMD, RSV
		if (target.index() == 0) {
			if (auto const& addr = std::get<0>(target); addr.ss_family == AF_INET) {
				auto const& sin = reinterpret_cast<sockaddr_in const&>(addr);
				buffer.push_back(1);												// ATYP
				append(buffer, sin.sin_addr);										// DST.ADDR
				append(buffer, sin.sin_port);										// DST.PORT
			} else {
				assert(addr.ss_family == AF_INET6);
				auto const& sin6 = reinterpret_cast<sockaddr_in6 const&>(addr);
				buffer.push_back(4);												// ATYP
				append(buffer, sin6.sin6_addr);										// DST.ADDR
				append(buffer, sin6.sin6_port);										// DST.PORT
			}
		} else {
			auto [host, hport] = std::get<1>(target);
			auto u8host = u8(host);
			auto nsport = htons(hport);
			buffer.push_back(3);													// ATYP
			buffer.push_back(size_as<uint8_t>(u8host));								// DST.ADDR
			buffer.insert(end(buffer), begin(u8host), end(u8host));					// DST.ADDR
			append(buffer, nsport);													// DST.PORT
		}
	}
	if (!SocksSend(s, buffer, CancelCheckWork))
		return {};
	return SocksReceiveReply(s, CancelCheckWork);
}


std::shared_ptr<SocketContext> connectsock(std::wstring&& host, int port, UINT prefixId, int *CancelCheckWork) {
	std::variant<sockaddr_storage, std::tuple<std::wstring, int>> target;
	int Fwall = AskHostFireWall() == YES ? FwallType : FWALL_NONE;
	if (auto ai = getaddrinfo(host, port, Fwall == FWALL_SOCKS4 ? AF_INET : AF_UNSPEC)) {
		// ホスト名がIPアドレスだった
		Notice(IDS_MSGJPN017, prefixId ? GetString(prefixId) : L""s, host, AddressPortToString(ai->ai_addr, ai->ai_addrlen));
		memcpy(&std::get<sockaddr_storage>(target), ai->ai_addr, ai->ai_addrlen);
	} else if ((Fwall == FWALL_SOCKS5_NOAUTH || Fwall == FWALL_SOCKS5_USER) && FwallResolve == YES) {
		// SOCKS5で名前解決する
		target = std::tuple{ std::move(host), port };
	} else if (ai = getaddrinfo(host, port, Fwall == FWALL_SOCKS4 ? AF_INET : AF_UNSPEC, CancelCheckWork)) {
		// 名前解決に成功
		Notice(IDS_MSGJPN017, prefixId ? GetString(prefixId) : L""s, host, AddressPortToString(ai->ai_addr, ai->ai_addrlen));
		memcpy(&std::get<sockaddr_storage>(target), ai->ai_addr, ai->ai_addrlen);
	} else {
		// 名前解決に失敗
		Notice(IDS_MSGJPN019, host);
		return {};
	}

	sockaddr_storage saConnect;
	if (Fwall == FWALL_SOCKS4 || Fwall == FWALL_SOCKS5_NOAUTH || Fwall == FWALL_SOCKS5_USER) {
		// connectで接続する先はSOCKSサーバ
		auto ai = getaddrinfo(FwallHost, FwallPort);
		if (!ai)
			ai = getaddrinfo(FwallHost, FwallPort, AF_UNSPEC, CancelCheckWork);
		if (!ai) {
			Notice(IDS_MSGJPN021, FwallHost);
			return {};
		}
		memcpy(&saConnect, ai->ai_addr, ai->ai_addrlen);
		Notice(IDS_MSGJPN022, AddressPortToString(ai->ai_addr, ai->ai_addrlen));
	} else {
		// connectで接続するのは接続先のホスト
		saConnect = std::get<sockaddr_storage>(target);
	}

	auto s = SocketContext::Create(saConnect.ss_family, SOCK_STREAM, IPPROTO_TCP);
	if (!s) {
		Notice(IDS_MSGJPN027);
		return {};
	}
	s->target = target;
	if (do_connect(s->handle, reinterpret_cast<const sockaddr*>(&saConnect), sizeof saConnect, CancelCheckWork) == SOCKET_ERROR) {
		Notice(IDS_MSGJPN026);
		DoClose(s);
		return {};
	}
	if (Fwall == FWALL_SOCKS4 || Fwall == FWALL_SOCKS5_NOAUTH || Fwall == FWALL_SOCKS5_USER) {
		auto result = SocksRequest(s, SocksCommand::Connect, target, CancelCheckWork);
		if (!result) {
			Notice(IDS_MSGJPN023);
			DoClose(s);
			return {};
		}
		CurHost.CurNetType = result->ss_family == AF_INET ? NTYPE_IPV4 : NTYPE_IPV6;
	} else
		CurHost.CurNetType = saConnect.ss_family == AF_INET ? NTYPE_IPV4 : NTYPE_IPV6;
	Notice(IDS_MSGJPN025);
	return s;
}


// リッスンソケットを取得
std::shared_ptr<SocketContext> GetFTPListenSocket(std::shared_ptr<SocketContext> ctrl_skt, int *CancelCheckWork) {
	sockaddr_storage saListen;
	int salen = sizeof saListen;
	if (getsockname(ctrl_skt->handle, reinterpret_cast<sockaddr*>(&saListen), &salen) == SOCKET_ERROR) {
		WSAError(L"getsockname()"sv);
		return {};
	}
	auto listen_skt = SocketContext::Create(saListen.ss_family, SOCK_STREAM, IPPROTO_TCP);
	if (!listen_skt)
		return {};
	if (AskHostFireWall() == YES && (FwallType == FWALL_SOCKS4 || FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER)) {
		Debug(L"Use SOCKS BIND."sv);
		// Control接続と同じアドレスに接続する
		salen = sizeof saListen;
		if (getpeername(ctrl_skt->handle, reinterpret_cast<sockaddr*>(&saListen), &salen) == SOCKET_ERROR) {
			WSAError(L"getpeername()"sv);
			return {};
		}
		if (do_connect(listen_skt->handle, reinterpret_cast<const sockaddr*>(&saListen), salen, CancelCheckWork) == SOCKET_ERROR) {
			return {};
		}
		if (auto result = SocksRequest(listen_skt, SocksCommand::Bind, ctrl_skt->target, CancelCheckWork)) {
			saListen = *result;
		} else {
			Notice(IDS_MSGJPN023);
			DoClose(listen_skt);
			return {};
		}
	} else {
		Debug(L"Use normal BIND."sv);
		// Control接続と同じアドレス（ただしport=0）でlistenする
		if (saListen.ss_family == AF_INET)
			reinterpret_cast<sockaddr_in&>(saListen).sin_port = 0;
		else
			reinterpret_cast<sockaddr_in6&>(saListen).sin6_port = 0;
		if (bind(listen_skt->handle, reinterpret_cast<const sockaddr*>(&saListen), salen) == SOCKET_ERROR) {
			WSAError(L"bind()"sv);
			listen_skt->Close();
			Notice(IDS_MSGJPN027);
			return {};
		}
		salen = sizeof saListen;
		if (getsockname(listen_skt->handle, reinterpret_cast<sockaddr*>(&saListen), &salen) == SOCKET_ERROR) {
			WSAError(L"getsockname()"sv);
			listen_skt->Close();
			Notice(IDS_MSGJPN027);
			return {};
		}
		if (do_listen(listen_skt->handle, 1) != 0) {
			WSAError(L"listen()"sv);
			listen_skt->Close();
			Notice(IDS_MSGJPN027);
			return {};
		}
		// TODO: IPv6にUPnP NATは無意味なのでは？
		if (IsUPnPLoaded() == YES && UPnPEnabled == YES) {
			auto port = ntohs(saListen.ss_family == AF_INET ? reinterpret_cast<sockaddr_in const&>(saListen).sin_port : reinterpret_cast<sockaddr_in6 const&>(saListen).sin6_port);
			// TODO: UPnP NATで外部アドレスだけ参照しているが、外部ポートが内部ポートと異なる可能性が十分にあるのでは？
			if (auto const ExtAdrs = AddPortMapping(AddressToString(saListen), port))
				if (auto ai = getaddrinfo(*ExtAdrs, port)) {
					memcpy(&saListen, ai->ai_addr, ai->ai_addrlen);
					listen_skt->mapPort = port;
				}
		}
	}
	int status;
	if (saListen.ss_family == AF_INET) {
		auto const& sin = reinterpret_cast<sockaddr_in const&>(saListen);
		auto a = reinterpret_cast<const uint8_t*>(&sin.sin_addr), p = reinterpret_cast<const uint8_t*>(&sin.sin_port);
		status = std::get<0>(Command(ctrl_skt, CancelCheckWork, L"PORT {},{},{},{},{},{}"sv, a[0], a[1], a[2], a[3], p[0], p[1]));
	} else {
		auto a = AddressToString(saListen);
		auto p = reinterpret_cast<sockaddr_in6 const&>(saListen).sin6_port;
		status = std::get<0>(Command(ctrl_skt, CancelCheckWork, L"EPRT |2|{}|{}|"sv, a, ntohs(p)));
	}
	if (status / 100 != FTP_COMPLETE) {
		Notice(IDS_MSGJPN031, saListen.ss_family == AF_INET ? L"PORT"sv : L"EPRT"sv);
		if (IsUPnPLoaded() == YES)
			RemovePortMapping(listen_skt->mapPort);
		listen_skt->Close();
		return {};
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

