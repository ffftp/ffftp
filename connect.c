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

#define	STRICT
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
// IPv6対応
//#include <winsock.h>
#include <ws2tcpip.h>
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

static BOOL CALLBACK QuickConDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static int SendInitCommand(char *Cmd);
static void AskUseFireWall(char *Host, int *Fire, int *Pasv, int *List);
static void SaveCurrentSetToHistory(void);
static int ReConnectSkt(SOCKET *Skt);
// 暗号化通信対応
// 同時接続対応
//static SOCKET DoConnect(char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security);
static SOCKET DoConnectCrypt(int CryptMode, HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork);
static SOCKET DoConnect(HOSTDATA* HostData, char *Host, char *User, char *Pass, char *Acct, int Port, int Fwall, int SavePass, int Security, int *CancelCheckWork);
static int CheckOneTimePassword(char *Pass, char *Reply, int Type);
static BOOL CALLBACK BlkHookFnc(void);
static int Socks5MakeCmdPacket(SOCKS5REQUEST *Packet, char Cmd, int ValidIP, ulong IP, char *Host, ushort Port);
// IPv6対応
static int Socks5MakeCmdPacketIPv6(SOCKS5REQUEST *Packet, char Cmd, int ValidIP, char *IP, char *Host, ushort Port);
static int SocksSendCmd(SOCKET Socket, void *Data, int Size, int *CancelCheckWork);
// 同時接続対応
//static int Socks5GetCmdReply(SOCKET Socket, SOCKS5REPLY *Packet);
static int Socks5GetCmdReply(SOCKET Socket, SOCKS5REPLY *Packet, int *CancelCheckWork);
// 同時接続対応
//static int Socks4GetCmdReply(SOCKET Socket, SOCKS4REPLY *Packet);
static int Socks4GetCmdReply(SOCKET Socket, SOCKS4REPLY *Packet, int *CancelCheckWork);
static int Socks5SelMethod(SOCKET Socket, int *CancelCheckWork);

/*===== 外部参照 =====*/

extern char FilterStr[FILTER_EXT_LEN+1];
extern char TitleHostName[HOST_ADRS_LEN+1];
extern int CancelFlg;

/* 設定値 */
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern char FwallHost[HOST_ADRS_LEN+1];
extern char FwallUser[USER_NAME_LEN+1];
extern char FwallPass[PASSWORD_LEN+1];
extern int FwallPort;
extern int FwallType;
extern int FwallDefault;
extern int FwallSecurity;
extern int FwallResolv;
extern int FwallLower;
extern int FwallDelimiter;
extern int PasvDefault;
extern int QuickAnonymous;

/*===== ローカルなワーク =====*/

static int Anonymous;
static int TryConnect = NO;
static SOCKET CmdCtrlSocket = INVALID_SOCKET;
static SOCKET TrnCtrlSocket = INVALID_SOCKET;
static HOSTDATA CurHost;

/* 接続中の接続先、SOCKSサーバのアドレス情報を保存しておく */
/* この情報はlistenソケットを取得する際に用いる */
// IPv6対応
//static struct sockaddr_in SocksSockAddr;	/* SOCKSサーバのアドレス情報 */
//static struct sockaddr_in CurSockAddr;		/* 接続先ホストのアドレス情報 */
static struct sockaddr_in SocksSockAddrIPv4;	/* SOCKSサーバのアドレス情報 */
static struct sockaddr_in CurSockAddrIPv4;		/* 接続先ホストのアドレス情報 */
static struct sockaddr_in6 SocksSockAddrIPv6;	/* SOCKSサーバのアドレス情報 */
static struct sockaddr_in6 CurSockAddrIPv6;		/* 接続先ホストのアドレス情報 */
static const struct in6_addr IN6ADDR_NONE = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

static int UseIPadrs;
static char DomainName[HOST_ADRS_LEN+1];




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

		if(ConnectRas(CurHost.Dialup, CurHost.DialupAlways, CurHost.DialupNotify, CurHost.DialEntry) == FFFTP_SUCCESS)
		{
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

			// UTF-8対応
			if(CurHost.CurNameKanjiCode == KANJI_AUTO)
			{
				if(DoDirListCmdSkt("", "", 999, &CancelFlg) == FTP_COMPLETE)
					CurHost.CurNameKanjiCode = AnalyzeNameKanjiCode(999);
			}

			if(CmdCtrlSocket != INVALID_SOCKET)
			{
				strcpy(TitleHostName, CurHost.HostName);
				DispWindowTitle();
				SoundPlay(SND_CONNECT);

				SendInitCommand(CurHost.InitCmd);

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
				SoundPlay(SND_ERROR);

			EnableUserOpe();
		}
		else
			SetTaskMsg(MSGJPN001);
	}
	return;
}


/*----- ホスト名を入力してホストへ接続 ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void QuickConnectProc(void)
{
	char Tmp[FMAX_PATH+1 + USER_NAME_LEN+1 + PASSWORD_LEN+1 + 2];
	char File[FMAX_PATH+1];

	SaveBookMark();
	SaveCurrentSetToHost();

	if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(hostname_dlg), GetMainHwnd(), QuickConDialogCallBack, (LPARAM)Tmp) == YES)
	{
		/* 接続中なら切断する */
		if(CmdCtrlSocket != INVALID_SOCKET)
			DisconnectProc();

		SetTaskMsg("----------------------------");

		InitPWDcommand();
		CopyDefaultHost(&CurHost);
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;
		if(SplitUNCpath(Tmp, CurHost.HostAdrs, CurHost.RemoteInitDir, File, CurHost.UserName, CurHost.PassWord, &CurHost.Port) == FFFTP_SUCCESS)
		{
			if(strlen(CurHost.UserName) == 0)
			{
				strcpy(CurHost.UserName, Tmp + FMAX_PATH+1);
				strcpy(CurHost.PassWord, Tmp + FMAX_PATH+1 + USER_NAME_LEN+1);
			}

			SetCurrentHost(HOSTNUM_NOENTRY);
			AskUseFireWall(CurHost.HostAdrs, &CurHost.FireWall, &CurHost.Pasv, &CurHost.ListCmdOnly);
			CurHost.FireWall = (int)Tmp[FMAX_PATH+1 + USER_NAME_LEN+1 + PASSWORD_LEN+1];
			CurHost.Pasv = (int)Tmp[FMAX_PATH+1 + USER_NAME_LEN+1 + PASSWORD_LEN+1 + 1];

			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			DisableUserOpe();
			// 暗号化通信対応
			// 同時接続対応
//			CmdCtrlSocket = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security);
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			// UTF-8対応
			if(CurHost.CurNameKanjiCode == KANJI_AUTO)
			{
				if(DoDirListCmdSkt("", "", 999, &CancelFlg) == FTP_COMPLETE)
					CurHost.CurNameKanjiCode = AnalyzeNameKanjiCode(999);
			}

			if(CmdCtrlSocket != INVALID_SOCKET)
			{
				strcpy(TitleHostName, CurHost.HostAdrs);
				DispWindowTitle();
				SoundPlay(SND_CONNECT);

				InitTransCurDir();
				DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
				EnableUserOpe();

				if(strlen(File) > 0)
					DirectDownLoadProc(File);
			}
			else
			{
				SoundPlay(SND_ERROR);
				EnableUserOpe();
			}
		}
	}
	return;
}


/*----- クイック接続ダイアログのコールバック ----------------------------------
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

static BOOL CALLBACK QuickConDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static char *Buf;
	int i;
	HISTORYDATA Tmp;

//char Str[HOST_ADRS_LEN+USER_NAME_LEN+INIT_DIR_LEN+5+1];

	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, QHOST_HOST, CB_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, QHOST_HOST, WM_SETTEXT, 0, (LPARAM)"");
			SendDlgItemMessage(hDlg, QHOST_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
			if(QuickAnonymous == YES)
			{
				SendDlgItemMessage(hDlg, QHOST_USER, WM_SETTEXT, 0, (LPARAM)"anonymous");
				SendDlgItemMessage(hDlg, QHOST_PASS, WM_SETTEXT, 0, (LPARAM)UserMailAdrs);
			}
			else
			{
				SendDlgItemMessage(hDlg, QHOST_USER, WM_SETTEXT, 0, (LPARAM)"");
				SendDlgItemMessage(hDlg, QHOST_PASS, WM_SETTEXT, 0, (LPARAM)"");
			}
			SendDlgItemMessage(hDlg, QHOST_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SendDlgItemMessage(hDlg, QHOST_FWALL, BM_SETCHECK, FwallDefault, 0);
			SendDlgItemMessage(hDlg, QHOST_PASV, BM_SETCHECK, PasvDefault, 0);
			for(i = 0; i < HISTORY_MAX; i++)
			{
				if(GetHistoryByNum(i, &Tmp) == FFFTP_SUCCESS)
				{
//sprintf(Str, "%s (%s) %s", Tmp.HostAdrs, Tmp.UserName, Tmp.RemoteInitDir);
//SendDlgItemMessage(hDlg, QHOST_HOST, CB_ADDSTRING, 0, (LPARAM)Str);
					SendDlgItemMessage(hDlg, QHOST_HOST, CB_ADDSTRING, 0, (LPARAM)Tmp.HostAdrs);
				}
			}
			Buf = (char *)lParam;
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, QHOST_HOST, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Buf);
					SendDlgItemMessage(hDlg, QHOST_USER, WM_GETTEXT, USER_NAME_LEN+1, (LPARAM)Buf + FMAX_PATH+1);
					SendDlgItemMessage(hDlg, QHOST_PASS, WM_GETTEXT, PASSWORD_LEN+1, (LPARAM)Buf + FMAX_PATH+1 + USER_NAME_LEN+1);
					*(Buf + FMAX_PATH+1 + USER_NAME_LEN+1 + PASSWORD_LEN+1) = (char)SendDlgItemMessage(hDlg, QHOST_FWALL, BM_GETCHECK, 0, 0);
					*(Buf + FMAX_PATH+1 + USER_NAME_LEN+1 + PASSWORD_LEN+1+1) = (char)SendDlgItemMessage(hDlg, QHOST_PASV, BM_GETCHECK, 0, 0);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

//				case QHOST_HOST :
//					if(HIWORD(wParam) == CBN_EDITCHANGE)
//						DoPrintf("EDIT");
//					break;
			}
            return(TRUE);
	}
	return(FALSE);
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

		// UTF-8対応
		if(CurHost.CurNameKanjiCode == KANJI_AUTO)
		{
			if(DoDirListCmdSkt("", "", 999, &CancelFlg) == FTP_COMPLETE)
				CurHost.CurNameKanjiCode = AnalyzeNameKanjiCode(999);
		}

		if(CmdCtrlSocket != INVALID_SOCKET)
		{
			strcpy(TitleHostName, CurHost.HostAdrs);
			DispWindowTitle();
			SoundPlay(SND_CONNECT);

			InitTransCurDir();
			DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

			GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			EnableUserOpe();

			if(strlen(File) > 0)
				DirectDownLoadProc(File);
			else
				ResetAutoExitFlg();
		}
		else
		{
			SoundPlay(SND_ERROR);
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
	HISTORYDATA Hist;
	int LFSort;
	int LDSort;
	int RFSort;
	int RDSort;

	if(GetHistoryByCmd(MenuCmd, &Hist) == FFFTP_SUCCESS)
	{
		SaveBookMark();
		SaveCurrentSetToHost();

		/* 接続中なら切断する */
		if(CmdCtrlSocket != INVALID_SOCKET)
			DisconnectProc();

		SetTaskMsg("----------------------------");

		InitPWDcommand();
		CopyHistoryToHost(&Hist, &CurHost);
		// UTF-8対応
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;

		if(ConnectRas(CurHost.Dialup, CurHost.DialupAlways, CurHost.DialupNotify, CurHost.DialEntry) == FFFTP_SUCCESS)
		{
			SetCurrentHost(HOSTNUM_NOENTRY);
			SetHostKanaCnvImm(CurHost.KanaCnv);
			SetHostKanjiCodeImm(CurHost.KanjiCode);
			SetSyncMoveMode(CurHost.SyncMove);

			DecomposeSortType(CurHost.Sort, &LFSort, &LDSort, &RFSort, &RDSort);
			SetSortTypeImm(LFSort, LDSort, RFSort, RDSort);
			ReSortDispList(WIN_LOCAL, &CancelFlg);

			SetTransferTypeImm(Hist.Type);
			DispTransferType();

			DisableUserOpe();
			// 暗号化通信対応
			// 同時接続対応
//			CmdCtrlSocket = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security);
			CmdCtrlSocket = DoConnect(&CurHost, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, &CancelFlg);
			TrnCtrlSocket = CmdCtrlSocket;

			// UTF-8対応
			if(CurHost.CurNameKanjiCode == KANJI_AUTO)
			{
				if(DoDirListCmdSkt("", "", 999, &CancelFlg) == FTP_COMPLETE)
					CurHost.CurNameKanjiCode = AnalyzeNameKanjiCode(999);
			}

			if(CmdCtrlSocket != INVALID_SOCKET)
			{
				strcpy(TitleHostName, CurHost.HostAdrs);
				DispWindowTitle();
				SoundPlay(SND_CONNECT);

				SendInitCommand(CurHost.InitCmd);

				DoLocalCWD(CurHost.LocalInitDir);
				GetLocalDirForWnd();

				InitTransCurDir();
				DoCWD(CurHost.RemoteInitDir, YES, YES, YES);

				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
			}
			else
				SoundPlay(SND_ERROR);

			EnableUserOpe();
		}
		else
			SetTaskMsg(MSGJPN002);
	}
	else
		SoundPlay(SND_ERROR);

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

static int SendInitCommand(char *Cmd)
{
	char Tmp[INITCMD_LEN+1];
	char *Pos;

	while(strlen(Cmd) > 0)
	{
		strcpy(Tmp, Cmd);
		if((Pos = strchr(Tmp, '\r')) != NULL)
			*Pos = NUL;
		if(strlen(Tmp) > 0)
			DoQUOTE(Tmp);

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
	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);

	return(CurHost.NameKanaCnv);
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
	if(CurHost.HostType == HTYPE_VMS)
		return(YES);
	else
	{
		if(AskCurrentHost() != HOSTNUM_NOENTRY)
			CopyHostFromListInConnect(AskCurrentHost(), &CurHost);
		return(CurHost.ListCmdOnly);
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
	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);

	return(CurHost.UseNLST_R);
}


/*----- 接続しているホストのChmodコマンドを返す -------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *Chmodコマンド
*----------------------------------------------------------------------------*/

char *AskHostChmodCmd(void)
{
	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);

	return(CurHost.ChmodCmd);
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
	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);

	return(CurHost.TimeZone);
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


/*----- 接続しているホストのLNSTファイル名を返す ------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *ファイル名／オプション
*----------------------------------------------------------------------------*/

char *AskHostLsName(void)
{
	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);

	return(CurHost.LsName);
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
	if(AskCurrentHost() != HOSTNUM_NOENTRY)
		CopyHostFromListInConnect(AskCurrentHost(), &CurHost);

	return(CurHost.HostType);
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
	char LocDir[FMAX_PATH+1];
	char HostDir[FMAX_PATH+1];

	if(TrnCtrlSocket != INVALID_SOCKET)
	{
		if((Host = AskCurrentHost()) != HOSTNUM_NOENTRY)
		{
			CopyHostFromListInConnect(Host, &CurHost);
			if(CurHost.LastDir == YES)
			{
				AskLocalCurDir(LocDir, FMAX_PATH);
				AskRemoteCurDir(HostDir, FMAX_PATH);
				SetHostDir(AskCurrentHost(), LocDir, HostDir);
			}
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
	char LocDir[FMAX_PATH+1];
	char HostDir[FMAX_PATH+1];

	AskLocalCurDir(LocDir, FMAX_PATH);
	AskRemoteCurDir(HostDir, FMAX_PATH);
	strcpy(CurHost.LocalInitDir, LocDir);
	strcpy(CurHost.RemoteInitDir, HostDir);

	CurHost.Sort = AskSortType(ITEM_LFILE) * 0x1000000 | AskSortType(ITEM_LDIR) * 0x10000 | AskSortType(ITEM_RFILE) * 0x100 | AskSortType(ITEM_RDIR);

	CurHost.KanjiCode = AskHostKanjiCode();
	CurHost.KanaCnv = AskHostKanaCnv();

	CurHost.SyncMove = AskSyncMoveMode();

	AddHostToHistory(&CurHost, AskTransferType());
	SetAllHistoryToMenu();

	return;
}


/*----- コマンドコントロールソケットの再接続 ----------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int ReConnectCmdSkt(void)
{
	int Sts;


	if(CmdCtrlSocket != TrnCtrlSocket)
		do_closesocket(TrnCtrlSocket);
	TrnCtrlSocket = INVALID_SOCKET;

	Sts = ReConnectSkt(&CmdCtrlSocket);

	TrnCtrlSocket = CmdCtrlSocket;

	return(Sts);
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
	// 暗号化通信対応
	// 同時接続対応
//	if((*Skt = DoConnect(CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security)) != INVALID_SOCKET)
	if((*Skt = DoConnect(&HostData, CurHost.HostAdrs, CurHost.UserName, CurHost.PassWord, CurHost.Account, CurHost.Port, CurHost.FireWall, NO, CurHost.Security, CancelCheckWork)) != INVALID_SOCKET)
	{
//		AskRemoteCurDir(Path, FMAX_PATH);
//		DoCWD(Path, YES, YES, YES);
		Sts = FFFTP_SUCCESS;
	}
	else
		SoundPlay(SND_ERROR);

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
	char Path[FMAX_PATH+1];
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
		AskRemoteCurDir(Path, FMAX_PATH);
		DoCWD(Path, YES, YES, YES);
		Sts = FFFTP_SUCCESS;
	}
	else
		SoundPlay(SND_ERROR);

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

	if((CmdCtrlSocket != INVALID_SOCKET) && (CmdCtrlSocket != TrnCtrlSocket))
	{
		DoQUIT(CmdCtrlSocket);
		DoClose(CmdCtrlSocket);
	}

	if(TrnCtrlSocket != INVALID_SOCKET)
	{
		DoQUIT(TrnCtrlSocket);
		DoClose(TrnCtrlSocket);

		SaveCurrentSetToHistory();

		EraseRemoteDirForWnd();
		SetTaskMsg(MSGJPN004);
	}

	TrnCtrlSocket = INVALID_SOCKET;
	CmdCtrlSocket = INVALID_SOCKET;

	DispWindowTitle();
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
	char Buf[1024];
	char Reply[1024];
	int Continue;
	int ReInPass;
	char *Tmp;
	int HostPort;
	static const char *SiteTbl[4] = { "SITE", "site", "OPEN", "open" };
	char TmpBuf[ONELINE_BUF_SIZE];
	struct linger LingerOpt;

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
				// FTPIS対応
//				while((Sts = ReadReplyMessage(ContSock, Buf, 1024, &CancelFlg, TmpBuf) / 100) == FTP_PRELIM)
//					;
				if(CryptMode == CRYPT_FTPIS)
				{
					if(AttachSSL(ContSock, INVALID_SOCKET, CancelCheckWork))
					{
						while((Sts = ReadReplyMessage(ContSock, Buf, 1024, CancelCheckWork, TmpBuf) / 100) == FTP_PRELIM)
							;
					}
					else
						Sts = FTP_ERROR;
				}
				else
				{
					while((Sts = ReadReplyMessage(ContSock, Buf, 1024, CancelCheckWork, TmpBuf) / 100) == FTP_PRELIM)
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

							if(HostPort == PORT_NOR)
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
							if((strlen(User) != 0) || 
							   (InputDialogBox(username_dlg, GetMainHwnd(), NULL, User, USER_NAME_LEN+1, &Anony, IDH_HELP_TOPIC_0000001) == YES))
							{
								if(Anony == YES)
								{
									strcpy(User, "anonymous");
									strcpy(Pass, UserMailAdrs);
								}

								if((Fwall == FWALL_FU_FP_USER) || (Fwall == FWALL_USER))
								{
									if(HostPort == PORT_NOR)
										sprintf(Buf, "%s%c%s", User, FwallDelimiter, Host);
									else
										sprintf(Buf, "%s%c%s %d", User, FwallDelimiter, Host, HostPort);
								}
								else
									strcpy(Buf, User);

								// FTPES対応
								if(CryptMode == CRYPT_FTPES)
								{
									if(IsOpenSSLLoaded() && (Sts = command(ContSock, Reply, CancelCheckWork, "AUTH TLS")) == 234)
									{
										if(AttachSSL(ContSock, INVALID_SOCKET, CancelCheckWork))
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
										if((strlen(Pass) != 0) || 
										   (InputDialogBox(passwd_dlg, GetMainHwnd(), NULL, Pass, PASSWORD_LEN+1, &Anony, IDH_HELP_TOPIC_0000001) == YES))
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
												if(InputDialogBox(re_passwd_dlg, GetMainHwnd(), NULL, Pass, PASSWORD_LEN+1, &Anony, IDH_HELP_TOPIC_0000001) == YES)
													Continue = YES;
												else
													DoPrintf("No password specified.");
												ReInPass = YES;
											}
											else if(Sts == FTP_CONTINUE)
											{
												if((strlen(Acct) != 0) || 
												   (InputDialogBox(account_dlg, GetMainHwnd(), NULL, Acct, ACCOUNT_LEN+1, &Anony, IDH_HELP_TOPIC_0000001) == YES))
												{
													// 同時接続対応
//													Sts = command(ContSock, NULL, &CancelFlg, "ACCT %s", Acct) / 100;
													Sts = command(ContSock, NULL, CancelCheckWork, "ACCT %s", Acct) / 100;
												}
												else
													DoPrintf("No account specified");
											}
										}
										else
										{
											Sts = FTP_ERROR;
											DoPrintf("No password specified.");
										}
									}
								}
								while(Continue == YES);
							}
							else
							{
								Sts = FTP_ERROR;
								DoPrintf("No user name specified");
							}

							if(Sts != FTP_COMPLETE)
							{
								SetTaskMsg(MSGJPN008, Host);
								DoClose(ContSock);
								ContSock = INVALID_SOCKET;
							}
							else if((SavePass == YES) && (ReInPass == YES))
							{
								if(DialogBox(GetFtpInst(), MAKEINTRESOURCE(savepass_dlg), GetMainHwnd(), ExeEscDialogProc) == YES)
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
			}
			// UTF-8対応
			if(HostData->CurNameKanjiCode == KANJI_AUTO && (HostData->Feature & FEATURE_UTF8))
			{
				if((Sts = command(ContSock, Reply, CancelCheckWork, "OPTS UTF8 ON")) == 200)
					HostData->CurNameKanjiCode = KANJI_UTF8N;
			}
		}
	}
	else if(CryptMode == CRYPT_SFTP)
	{
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
	if(*CancelCheckWork == NO && ContSock == INVALID_SOCKET && HostData->UseSFTP == YES)
	{
		SetTaskMsg(MSGJPN317);
		if((ContSock = DoConnectCrypt(CRYPT_SFTP, HostData, Host, User, Pass, Acct, Port, Fwall, SavePass, Security, CancelCheckWork)) != INVALID_SOCKET)
			HostData->CryptMode = CRYPT_SFTP;
	}
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
	char *Pos;
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
		DoPrintf("Analize OTP");
		DoPrintf("%s", Pos);
		Sts = FFFTP_FAIL;
		while((Pos = GetNextField(Pos)) != NULL)
		{
			if(IsDigit(*Pos))
			{
				Seq = atoi(Pos);
				DoPrintf("Sequence=%d", Seq);

				/* Seed */
				if((Pos = GetNextField(Pos)) != NULL)
				{
					if(GetOneField(Pos, Seed, MAX_SEED_LEN) == FFFTP_SUCCESS)
					{
						/* Seedは英数字のみ有効とする */
						for(i = strlen(Seed)-1; i >= 0; i--)
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
								DialogBox(GetFtpInst(), MAKEINTRESOURCE(otp_notify_dlg), GetMainHwnd(), ExeEscDialogProc);

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
		DoPrintf("No OTP used.");
	}
	return(Sts);
}














/*----- ソケットを接続する ----------------------------------------------------
*
*	Parameter
*		char *host : ホスト名
*		int port : ポート番号
*		char *PreMsg : メッセージの前半部分
*
*	Return Value
*		SOCKET ソケット
*----------------------------------------------------------------------------*/

// IPv6対応
SOCKET connectsock(char *host, int port, char *PreMsg, int *CancelCheckWork)
{
	SOCKET Result;
	Result = INVALID_SOCKET;
	switch(CurHost.InetFamily)
	{
	case AF_UNSPEC:
		if((Result = connectsockIPv4(host, port, PreMsg, CancelCheckWork)) != INVALID_SOCKET)
			CurHost.InetFamily = AF_INET;
		else if(CurHost.UseIPv6 == YES && (Result = connectsockIPv6(host, port, PreMsg, CancelCheckWork)) != INVALID_SOCKET)
			CurHost.InetFamily = AF_INET6;
		break;
	case AF_INET:
		Result = connectsockIPv4(host, port, PreMsg, CancelCheckWork);
		break;
	case AF_INET6:
		Result = connectsockIPv6(host, port, PreMsg, CancelCheckWork);
		break;
	}
	return Result;
}


// IPv6対応
//SOCKET connectsock(char *host, int port, char *PreMsg, int *CancelCheckWork)
SOCKET connectsockIPv4(char *host, int port, char *PreMsg, int *CancelCheckWork)
{
	struct sockaddr_in saSockAddr;
	char HostEntry[MAXGETHOSTSTRUCT];
	struct hostent *pHostEntry;
	SOCKET sSocket;
	int Len;
	int Fwall;
	SOCKS4CMD Socks4Cmd;
	SOCKS4REPLY Socks4Reply;
	SOCKS5REQUEST Socks5Cmd;
	SOCKS5REPLY Socks5Reply;

	//////////////////////////////
	// ホスト名解決と接続の準備
	//////////////////////////////

	Fwall = FWALL_NONE;
	if(AskHostFireWall() == YES)
		Fwall = FwallType;

	sSocket = INVALID_SOCKET;

	UseIPadrs = YES;
	strcpy(DomainName, host);
	// IPv6対応
//	memset(&CurSockAddr, 0, sizeof(CurSockAddr));
//	CurSockAddr.sin_port = htons((u_short)port);
//	CurSockAddr.sin_family = AF_INET;
//	if((CurSockAddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
	memset(&CurSockAddrIPv4, 0, sizeof(CurSockAddrIPv4));
	CurSockAddrIPv4.sin_port = htons((u_short)port);
	CurSockAddrIPv4.sin_family = AF_INET;
	if((CurSockAddrIPv4.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
	{
		// ホスト名が指定された
		// ホスト名からアドレスを求める
		if(((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER)) &&
		   (FwallResolv == YES))
		{
			// ホスト名解決はSOCKSサーバに任せる
			pHostEntry = NULL;
		}
		else
		{
			// アドレスを取得
			SetTaskMsg(MSGJPN016, DomainName);
			// IPv6対応
//			pHostEntry = do_gethostbyname(host, HostEntry, MAXGETHOSTSTRUCT, CancelCheckWork);
			pHostEntry = do_gethostbynameIPv4(host, HostEntry, MAXGETHOSTSTRUCT, CancelCheckWork);
		}

		if(pHostEntry != NULL)
		{
			// IPv6対応
//			memcpy((char *)&CurSockAddr.sin_addr, pHostEntry->h_addr, pHostEntry->h_length);
//			SetTaskMsg(MSGJPN017, PreMsg, DomainName, inet_ntoa(CurSockAddr.sin_addr), ntohs(CurSockAddr.sin_port));
			memcpy((char *)&CurSockAddrIPv4.sin_addr, pHostEntry->h_addr, pHostEntry->h_length);
			SetTaskMsg(MSGJPN017, PreMsg, DomainName, inet_ntoa(CurSockAddrIPv4.sin_addr), ntohs(CurSockAddrIPv4.sin_port));
		}
		else
		{
			if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
			{
				UseIPadrs = NO;
				// IPv6対応
//				SetTaskMsg(MSGJPN018, PreMsg, DomainName, ntohs(CurSockAddr.sin_port));
				SetTaskMsg(MSGJPN018, PreMsg, DomainName, ntohs(CurSockAddrIPv4.sin_port));
			}
			else
			{
				SetTaskMsg(MSGJPN019, host);
				return(INVALID_SOCKET);
			}
		}
	}
	else
		// IPv6対応
//		SetTaskMsg(MSGJPN020, PreMsg, inet_ntoa(CurSockAddr.sin_addr), ntohs(CurSockAddr.sin_port));
		SetTaskMsg(MSGJPN020, PreMsg, inet_ntoa(CurSockAddrIPv4.sin_addr), ntohs(CurSockAddrIPv4.sin_port));

	if((Fwall == FWALL_SOCKS4) || (Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
	{
		// SOCKSを使う
		// SOCKSに接続する準備
		if(Fwall == FWALL_SOCKS4)
		{
			Socks4Cmd.Ver = SOCKS4_VER;
			Socks4Cmd.Cmd = SOCKS4_CMD_CONNECT;
			// IPv6対応
//			Socks4Cmd.Port = CurSockAddr.sin_port;
//			Socks4Cmd.AdrsInt = CurSockAddr.sin_addr.s_addr;
			Socks4Cmd.Port = CurSockAddrIPv4.sin_port;
			Socks4Cmd.AdrsInt = CurSockAddrIPv4.sin_addr.s_addr;
			strcpy(Socks4Cmd.UserID, FwallUser);
			Len = offsetof(SOCKS4CMD, UserID) + strlen(FwallUser) + 1;
		}
		else
		{
			// IPv6対応
//			Len = Socks5MakeCmdPacket(&Socks5Cmd, SOCKS5_CMD_CONNECT, UseIPadrs, CurSockAddr.sin_addr.s_addr, DomainName, CurSockAddr.sin_port);
			Len = Socks5MakeCmdPacket(&Socks5Cmd, SOCKS5_CMD_CONNECT, UseIPadrs, CurSockAddrIPv4.sin_addr.s_addr, DomainName, CurSockAddrIPv4.sin_port);
		}

		// IPv6対応
//		memset(&SocksSockAddr, 0, sizeof(SocksSockAddr));
//		if((SocksSockAddr.sin_addr.s_addr = inet_addr(FwallHost)) == INADDR_NONE)
		memset(&SocksSockAddrIPv4, 0, sizeof(SocksSockAddrIPv4));
		if((SocksSockAddrIPv4.sin_addr.s_addr = inet_addr(FwallHost)) == INADDR_NONE)
		{
			// IPv6対応
//			if((pHostEntry = do_gethostbyname(FwallHost, HostEntry, MAXGETHOSTSTRUCT, CancelCheckWork)) != NULL)
//				memcpy((char *)&SocksSockAddr.sin_addr, pHostEntry->h_addr, pHostEntry->h_length);
			if((pHostEntry = do_gethostbynameIPv4(FwallHost, HostEntry, MAXGETHOSTSTRUCT, CancelCheckWork)) != NULL)
				memcpy((char *)&SocksSockAddrIPv4.sin_addr, pHostEntry->h_addr, pHostEntry->h_length);
			else
			{
				SetTaskMsg(MSGJPN021, FwallHost);
				return INVALID_SOCKET;
			}
		}
		// IPv6対応
//		SocksSockAddr.sin_port = htons((u_short)FwallPort);
//		SocksSockAddr.sin_family = AF_INET;
//		SetTaskMsg(MSGJPN022, inet_ntoa(SocksSockAddr.sin_addr), ntohs(SocksSockAddr.sin_port));
		SocksSockAddrIPv4.sin_port = htons((u_short)FwallPort);
		SocksSockAddrIPv4.sin_family = AF_INET;
		SetTaskMsg(MSGJPN022, inet_ntoa(SocksSockAddrIPv4.sin_addr), ntohs(SocksSockAddrIPv4.sin_port));
		// connectで接続する先はSOCKSサーバ
		// IPv6対応
//		memcpy(&saSockAddr, &SocksSockAddr, sizeof(SocksSockAddr));
		memcpy(&saSockAddr, &SocksSockAddrIPv4, sizeof(SocksSockAddrIPv4));
	}
	else
	{
		// connectで接続するのは接続先のホスト
		// IPv6対応
//		memcpy(&saSockAddr, &CurSockAddr, sizeof(CurSockAddr));
		memcpy(&saSockAddr, &CurSockAddrIPv4, sizeof(CurSockAddrIPv4));
	}

	/////////////
	// 接続実行
	/////////////

	if((sSocket = do_socket(AF_INET, SOCK_STREAM, TCP_PORT)) != INVALID_SOCKET)
	{
		if(do_connect(sSocket, (struct sockaddr *)&saSockAddr, sizeof(saSockAddr), CancelCheckWork) != SOCKET_ERROR)
		{
			if(Fwall == FWALL_SOCKS4)
			{
				Socks4Reply.Result = -1;
				// 同時接続対応
//				if((SocksSendCmd(sSocket, &Socks4Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
//				   (Socks4GetCmdReply(sSocket, &Socks4Reply) != FFFTP_SUCCESS) || 
//				   (Socks4Reply.Result != SOCKS4_RES_OK))
				if((SocksSendCmd(sSocket, &Socks4Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
				   (Socks4GetCmdReply(sSocket, &Socks4Reply, CancelCheckWork) != FFFTP_SUCCESS) || 
				   (Socks4Reply.Result != SOCKS4_RES_OK))
				{
					SetTaskMsg(MSGJPN023, Socks4Reply.Result);
					DoClose(sSocket);
					sSocket = INVALID_SOCKET;
				}
			}
			else if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
			{
				if(Socks5SelMethod(sSocket, CancelCheckWork) == FFFTP_FAIL)
				{
					DoClose(sSocket);
					sSocket = INVALID_SOCKET;
				}

				Socks5Reply.Result = -1;
				// 同時接続対応
//				if((SocksSendCmd(sSocket, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
//				   (Socks5GetCmdReply(sSocket, &Socks5Reply) != FFFTP_SUCCESS) || 
//				   (Socks5Reply.Result != SOCKS5_RES_OK))
				if((SocksSendCmd(sSocket, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
				   (Socks5GetCmdReply(sSocket, &Socks5Reply, CancelCheckWork) != FFFTP_SUCCESS) || 
				   (Socks5Reply.Result != SOCKS5_RES_OK))
				{
					SetTaskMsg(MSGJPN024, Socks5Reply.Result);
					DoClose(sSocket);
					sSocket = INVALID_SOCKET;
				}

			}

			if(sSocket != INVALID_SOCKET)
				SetTaskMsg(MSGJPN025);
		}
		else
		{
//#pragma aaa
			SetTaskMsg(MSGJPN026/*"接続できません(2) %x", sSocket*/);
			DoClose(sSocket);
			sSocket = INVALID_SOCKET;
		}
	}
	else
		SetTaskMsg(MSGJPN027);

	return(sSocket);
}


SOCKET connectsockIPv6(char *host, int port, char *PreMsg, int *CancelCheckWork)
{
	struct sockaddr_in6 saSockAddr;
	char HostEntry[MAXGETHOSTSTRUCT];
	struct hostent *pHostEntry;
	SOCKET sSocket;
	int Len;
	int Fwall;
	SOCKS5REQUEST Socks5Cmd;
	SOCKS5REPLY Socks5Reply;

	//////////////////////////////
	// ホスト名解決と接続の準備
	//////////////////////////////

	Fwall = FWALL_NONE;
	if(AskHostFireWall() == YES)
		Fwall = FwallType;

	sSocket = INVALID_SOCKET;

	UseIPadrs = YES;
	strcpy(DomainName, host);
	memset(&CurSockAddrIPv6, 0, sizeof(CurSockAddrIPv6));
	CurSockAddrIPv6.sin6_port = htons((u_short)port);
	CurSockAddrIPv6.sin6_family = AF_INET6;
	CurSockAddrIPv6.sin6_addr = inet6_addr(host);
	if(memcmp(&CurSockAddrIPv6.sin6_addr, &IN6ADDR_NONE, sizeof(struct in6_addr)) == 0)
	{
		// ホスト名が指定された
		// ホスト名からアドレスを求める
		if(((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER)) &&
		   (FwallResolv == YES))
		{
			// ホスト名解決はSOCKSサーバに任せる
			pHostEntry = NULL;
		}
		else
		{
			// アドレスを取得
			SetTaskMsg(MSGJPN016, DomainName);
			pHostEntry = do_gethostbynameIPv6(host, HostEntry, MAXGETHOSTSTRUCT, CancelCheckWork);
		}

		if(pHostEntry != NULL)
		{
			memcpy((char *)&CurSockAddrIPv6.sin6_addr, pHostEntry->h_addr, pHostEntry->h_length);
			SetTaskMsg(MSGJPN017, PreMsg, DomainName, inet6_ntoa(CurSockAddrIPv6.sin6_addr), ntohs(CurSockAddrIPv6.sin6_port));
		}
		else
		{
			if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
			{
				UseIPadrs = NO;
				SetTaskMsg(MSGJPN018, PreMsg, DomainName, ntohs(CurSockAddrIPv6.sin6_port));
			}
			else
			{
				SetTaskMsg(MSGJPN019, host);
				return(INVALID_SOCKET);
			}
		}
	}
	else
		SetTaskMsg(MSGJPN020, PreMsg, inet6_ntoa(CurSockAddrIPv6.sin6_addr), ntohs(CurSockAddrIPv6.sin6_port));

	if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
	{
		// SOCKSを使う
		// SOCKSに接続する準備
		{
			Len = Socks5MakeCmdPacketIPv6(&Socks5Cmd, SOCKS5_CMD_CONNECT, UseIPadrs, (char*)&CurSockAddrIPv6.sin6_addr, DomainName, CurSockAddrIPv6.sin6_port);
		}

		memset(&SocksSockAddrIPv6, 0, sizeof(SocksSockAddrIPv6));
		SocksSockAddrIPv6.sin6_addr = inet6_addr(FwallHost);
		if(memcmp(&SocksSockAddrIPv6.sin6_addr, &IN6ADDR_NONE, sizeof(struct in6_addr)) == 0)
		{
			if((pHostEntry = do_gethostbynameIPv6(FwallHost, HostEntry, MAXGETHOSTSTRUCT, CancelCheckWork)) != NULL)
				memcpy((char *)&SocksSockAddrIPv6.sin6_addr, pHostEntry->h_addr, pHostEntry->h_length);
			else
			{
				SetTaskMsg(MSGJPN021, FwallHost);
				return INVALID_SOCKET;
			}
		}
		SocksSockAddrIPv6.sin6_port = htons((u_short)FwallPort);
		SocksSockAddrIPv6.sin6_family = AF_INET6;
		SetTaskMsg(MSGJPN022, inet6_ntoa(SocksSockAddrIPv6.sin6_addr), ntohs(SocksSockAddrIPv6.sin6_port));
		// connectで接続する先はSOCKSサーバ
		memcpy(&saSockAddr, &SocksSockAddrIPv6, sizeof(SocksSockAddrIPv6));
	}
	else
	{
		// connectで接続するのは接続先のホスト
		memcpy(&saSockAddr, &CurSockAddrIPv6, sizeof(CurSockAddrIPv6));
	}

	/////////////
	// 接続実行
	/////////////

	inet6_ntoa(saSockAddr.sin6_addr);
	if((sSocket = do_socket(AF_INET6, SOCK_STREAM, TCP_PORT)) != INVALID_SOCKET)
	{
		if(do_connect(sSocket, (struct sockaddr *)&saSockAddr, sizeof(saSockAddr), CancelCheckWork) != SOCKET_ERROR)
		{
			if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
			{
				if(Socks5SelMethod(sSocket, CancelCheckWork) == FFFTP_FAIL)
				{
					DoClose(sSocket);
					sSocket = INVALID_SOCKET;
				}

				Socks5Reply.Result = -1;
				// 同時接続対応
//				if((SocksSendCmd(sSocket, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
//				   (Socks5GetCmdReply(sSocket, &Socks5Reply) != FFFTP_SUCCESS) || 
//				   (Socks5Reply.Result != SOCKS5_RES_OK))
				if((SocksSendCmd(sSocket, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
				   (Socks5GetCmdReply(sSocket, &Socks5Reply, CancelCheckWork) != FFFTP_SUCCESS) || 
				   (Socks5Reply.Result != SOCKS5_RES_OK))
				{
					SetTaskMsg(MSGJPN024, Socks5Reply.Result);
					DoClose(sSocket);
					sSocket = INVALID_SOCKET;
				}

			}

			if(sSocket != INVALID_SOCKET)
				SetTaskMsg(MSGJPN025);
		}
		else
		{
//#pragma aaa
			SetTaskMsg(MSGJPN026/*"接続できません(2) %x", sSocket*/);
			DoClose(sSocket);
			sSocket = INVALID_SOCKET;
		}
	}
	else
		SetTaskMsg(MSGJPN027);

	return(sSocket);
}


/*----- リッスンソケットを取得 ------------------------------------------------
*
*	Parameter
*		SOCKET ctrl_skt : コントロールソケット
*
*	Return Value
*		SOCKET リッスンソケット
*----------------------------------------------------------------------------*/

// IPv6対応
SOCKET GetFTPListenSocket(SOCKET ctrl_skt, int *CancelCheckWork)
{
	SOCKET Result;
	Result = INVALID_SOCKET;
	switch(CurHost.InetFamily)
	{
	case AF_UNSPEC:
		break;
	case AF_INET:
		Result = GetFTPListenSocketIPv4(ctrl_skt, CancelCheckWork);
		break;
	case AF_INET6:
		Result = GetFTPListenSocketIPv6(ctrl_skt, CancelCheckWork);
		break;
	}
	return Result;
}


// IPv6対応
//SOCKET GetFTPListenSocket(SOCKET ctrl_skt, int *CancelCheckWork)
SOCKET GetFTPListenSocketIPv4(SOCKET ctrl_skt, int *CancelCheckWork)
{
    SOCKET listen_skt;
    int iLength;
    char *a,*p;
	struct sockaddr_in saCtrlAddr;
	struct sockaddr_in saTmpAddr;
	SOCKS4CMD Socks4Cmd;
	SOCKS4REPLY Socks4Reply;
	SOCKS5REQUEST Socks5Cmd;
	SOCKS5REPLY Socks5Reply;

	int Len;
	int Fwall;

	Fwall = FWALL_NONE;
	if(AskHostFireWall() == YES)
		Fwall = FwallType;

	if((listen_skt = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET)
	{
		if(Fwall == FWALL_SOCKS4)
		{
			/*===== SOCKS4を使う =====*/
			DoPrintf("Use SOCKS4 BIND");
			// IPv6対応
//			if(do_connect(listen_skt, (struct sockaddr *)&SocksSockAddr, sizeof(SocksSockAddr), CancelCheckWork) != SOCKET_ERROR)
			if(do_connect(listen_skt, (struct sockaddr *)&SocksSockAddrIPv4, sizeof(SocksSockAddrIPv4), CancelCheckWork) != SOCKET_ERROR)
			{
				Socks4Cmd.Ver = SOCKS4_VER;
				Socks4Cmd.Cmd = SOCKS4_CMD_BIND;
				// IPv6対応
//				Socks4Cmd.Port = CurSockAddr.sin_port;
//				Socks4Cmd.AdrsInt = CurSockAddr.sin_addr.s_addr;
				Socks4Cmd.Port = CurSockAddrIPv4.sin_port;
				Socks4Cmd.AdrsInt = CurSockAddrIPv4.sin_addr.s_addr;
				strcpy(Socks4Cmd.UserID, FwallUser);
				Len = offsetof(SOCKS4CMD, UserID) + strlen(FwallUser) + 1;

				Socks4Reply.Result = -1;
				// 同時接続対応
//				if((SocksSendCmd(listen_skt, &Socks4Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
//				   (Socks4GetCmdReply(listen_skt, &Socks4Reply) != FFFTP_SUCCESS) || 
//				   (Socks4Reply.Result != SOCKS4_RES_OK))
				if((SocksSendCmd(listen_skt, &Socks4Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
				   (Socks4GetCmdReply(listen_skt, &Socks4Reply, CancelCheckWork) != FFFTP_SUCCESS) || 
				   (Socks4Reply.Result != SOCKS4_RES_OK))
				{
					SetTaskMsg(MSGJPN028, Socks4Reply.Result);
					DoClose(listen_skt);
					listen_skt = INVALID_SOCKET;
				}

				if(Socks4Reply.AdrsInt == 0)
					// IPv6対応
//					Socks4Reply.AdrsInt = SocksSockAddr.sin_addr.s_addr;
					Socks4Reply.AdrsInt = SocksSockAddrIPv4.sin_addr.s_addr;

				a = (char *)&Socks4Reply.AdrsInt;
				p = (char *)&Socks4Reply.Port;
			}
		}
		else if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
		{
			/*===== SOCKS5を使う =====*/
			DoPrintf("Use SOCKS5 BIND");
			// IPv6対応
//			if(do_connect(listen_skt, (struct sockaddr *)&SocksSockAddr, sizeof(SocksSockAddr), CancelCheckWork) != SOCKET_ERROR)
			if(do_connect(listen_skt, (struct sockaddr *)&SocksSockAddrIPv4, sizeof(SocksSockAddrIPv4), CancelCheckWork) != SOCKET_ERROR)
			{
				if(Socks5SelMethod(listen_skt, CancelCheckWork) == FFFTP_FAIL)
				{
					DoClose(listen_skt);
					listen_skt = INVALID_SOCKET;
					return(listen_skt);
				}

				// IPv6対応
//				Len = Socks5MakeCmdPacket(&Socks5Cmd, SOCKS5_CMD_BIND, UseIPadrs, CurSockAddr.sin_addr.s_addr, DomainName, CurSockAddr.sin_port);
				Len = Socks5MakeCmdPacket(&Socks5Cmd, SOCKS5_CMD_BIND, UseIPadrs, CurSockAddrIPv4.sin_addr.s_addr, DomainName, CurSockAddrIPv4.sin_port);

				Socks5Reply.Result = -1;
				// 同時接続対応
//				if((SocksSendCmd(listen_skt, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
//				   (Socks5GetCmdReply(listen_skt, &Socks5Reply) != FFFTP_SUCCESS) || 
//				   (Socks5Reply.Result != SOCKS5_RES_OK))
				if((SocksSendCmd(listen_skt, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
				   (Socks5GetCmdReply(listen_skt, &Socks5Reply, CancelCheckWork) != FFFTP_SUCCESS) || 
				   (Socks5Reply.Result != SOCKS5_RES_OK))
				{
					SetTaskMsg(MSGJPN029, Socks5Reply.Result);
					DoClose(listen_skt);
					listen_skt = INVALID_SOCKET;
				}

				// IPv6対応
//				if(Socks5Reply.AdrsInt == 0)
//					Socks5Reply.AdrsInt = SocksSockAddr.sin_addr.s_addr;

				// IPv6対応
//				a = (char *)&Socks5Reply.AdrsInt;
//				p = (char *)&Socks5Reply.Port;
				a = (char *)&Socks5Reply._dummy[0];
				p = (char *)&Socks5Reply._dummy[4];
			}
		}
		else
		{
			/*===== SOCKSを使わない =====*/
			DoPrintf("Use normal BIND");
			saCtrlAddr.sin_port = htons(0);
			saCtrlAddr.sin_family = AF_INET;
			saCtrlAddr.sin_addr.s_addr = 0;

			if(bind(listen_skt, (struct sockaddr *)&saCtrlAddr, sizeof(struct sockaddr)) != SOCKET_ERROR)
			{
				iLength = sizeof(saCtrlAddr);
				if(getsockname(listen_skt, (struct sockaddr *)&saCtrlAddr, &iLength) != SOCKET_ERROR)
				{
					if(do_listen(listen_skt, 1) == 0)
					{
						iLength = sizeof(saTmpAddr);
						if(getsockname(ctrl_skt, (struct sockaddr *)&saTmpAddr, &iLength) == SOCKET_ERROR)
							ReportWSError("getsockname", WSAGetLastError());

						a = (char *)&saTmpAddr.sin_addr;
						p = (char *)&saCtrlAddr.sin_port;
					}
					else
					{
						ReportWSError("listen", WSAGetLastError());
						do_closesocket(listen_skt);
						listen_skt = INVALID_SOCKET;
					}
				}
				else
				{
					ReportWSError("getsockname", WSAGetLastError());
					do_closesocket(listen_skt);
					listen_skt = INVALID_SOCKET;
				}
			}
			else
			{
				ReportWSError("bind", WSAGetLastError());
				do_closesocket(listen_skt);
				listen_skt = INVALID_SOCKET;
			}

			if(listen_skt == INVALID_SOCKET)
				SetTaskMsg(MSGJPN030);
		}
	}
	else
		ReportWSError("socket create", WSAGetLastError());

	if(listen_skt != INVALID_SOCKET)
	{
#define  UC(b)  (((int)b)&0xff)
		// 同時接続対応
//		if((command(ctrl_skt,NULL, &CancelFlg, "PORT %d,%d,%d,%d,%d,%d",
//				UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
//				UC(p[0]), UC(p[1])) / 100) != FTP_COMPLETE)
		if((command(ctrl_skt,NULL, CancelCheckWork, "PORT %d,%d,%d,%d,%d,%d",
				UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
				UC(p[0]), UC(p[1])) / 100) != FTP_COMPLETE)
		{
			SetTaskMsg(MSGJPN031);
			do_closesocket(listen_skt);
			listen_skt = INVALID_SOCKET;
		}
//		else
//			DoPrintf("Skt=%u : listener %s port %u",listen_skt,inet_ntoa(saCtrlAddr.sin_addr),ntohs(saCtrlAddr.sin_port));
	}

	return(listen_skt);
}


SOCKET GetFTPListenSocketIPv6(SOCKET ctrl_skt, int *CancelCheckWork)
{
    SOCKET listen_skt;
    int iLength;
    char *a,*p;
	struct sockaddr_in6 saCtrlAddr;
	struct sockaddr_in6 saTmpAddr;
	SOCKS5REQUEST Socks5Cmd;
	SOCKS5REPLY Socks5Reply;

	int Len;
	int Fwall;

	char Adrs[40];

	Fwall = FWALL_NONE;
	if(AskHostFireWall() == YES)
		Fwall = FwallType;

	if((listen_skt = do_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET)
	{
		if((Fwall == FWALL_SOCKS5_NOAUTH) || (Fwall == FWALL_SOCKS5_USER))
		{
			/*===== SOCKS5を使う =====*/
			DoPrintf("Use SOCKS5 BIND");
			if(do_connect(listen_skt, (struct sockaddr *)&SocksSockAddrIPv6, sizeof(SocksSockAddrIPv6), CancelCheckWork) != SOCKET_ERROR)
			{
				if(Socks5SelMethod(listen_skt, CancelCheckWork) == FFFTP_FAIL)
				{
					DoClose(listen_skt);
					listen_skt = INVALID_SOCKET;
					return(listen_skt);
				}

				Len = Socks5MakeCmdPacketIPv6(&Socks5Cmd, SOCKS5_CMD_BIND, UseIPadrs, (char*)&CurSockAddrIPv6.sin6_addr, DomainName, CurSockAddrIPv6.sin6_port);

				Socks5Reply.Result = -1;
				// 同時接続対応
//				if((SocksSendCmd(listen_skt, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
//				   (Socks5GetCmdReply(listen_skt, &Socks5Reply) != FFFTP_SUCCESS) || 
//				   (Socks5Reply.Result != SOCKS5_RES_OK))
				if((SocksSendCmd(listen_skt, &Socks5Cmd, Len, CancelCheckWork) != FFFTP_SUCCESS) ||
				   (Socks5GetCmdReply(listen_skt, &Socks5Reply, CancelCheckWork) != FFFTP_SUCCESS) || 
				   (Socks5Reply.Result != SOCKS5_RES_OK))
				{
					SetTaskMsg(MSGJPN029, Socks5Reply.Result);
					DoClose(listen_skt);
					listen_skt = INVALID_SOCKET;
				}

				// IPv6対応
//				if(Socks5Reply.AdrsInt == 0)
//					Socks5Reply.AdrsInt = SocksSockAddr.sin_addr.s_addr;

				// IPv6対応
//				a = (char *)&Socks5Reply.AdrsInt;
//				p = (char *)&Socks5Reply.Port;
				a = (char *)&Socks5Reply._dummy[0];
				p = (char *)&Socks5Reply._dummy[16];
			}
		}
		else
		{
			/*===== SOCKSを使わない =====*/
			DoPrintf("Use normal BIND");
			saCtrlAddr.sin6_port = htons(0);
			saCtrlAddr.sin6_family = AF_INET6;
			memset(&saCtrlAddr.sin6_addr, 0, 16);

			if(bind(listen_skt, (struct sockaddr *)&saCtrlAddr, sizeof(struct sockaddr_in6)) != SOCKET_ERROR)
			{
				iLength = sizeof(saCtrlAddr);
				if(getsockname(listen_skt, (struct sockaddr *)&saCtrlAddr, &iLength) != SOCKET_ERROR)
				{
					if(do_listen(listen_skt, 1) == 0)
					{
						iLength = sizeof(saTmpAddr);
						if(getsockname(ctrl_skt, (struct sockaddr *)&saTmpAddr, &iLength) == SOCKET_ERROR)
							ReportWSError("getsockname", WSAGetLastError());

						a = (char *)&saTmpAddr.sin6_addr;
						p = (char *)&saCtrlAddr.sin6_port;
					}
					else
					{
						ReportWSError("listen", WSAGetLastError());
						do_closesocket(listen_skt);
						listen_skt = INVALID_SOCKET;
					}
				}
				else
				{
					ReportWSError("getsockname", WSAGetLastError());
					do_closesocket(listen_skt);
					listen_skt = INVALID_SOCKET;
				}
			}
			else
			{
				ReportWSError("bind", WSAGetLastError());
				do_closesocket(listen_skt);
				listen_skt = INVALID_SOCKET;
			}

			if(listen_skt == INVALID_SOCKET)
				SetTaskMsg(MSGJPN030);
		}
	}
	else
		ReportWSError("socket create", WSAGetLastError());

	if(listen_skt != INVALID_SOCKET)
	{
#define  US(w)  (((int)w)&0xffff)
		// 同時接続対応
//		if((command(ctrl_skt,NULL, &CancelFlg, "PORT %d,%d,%d,%d,%d,%d",
//				UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
//				UC(p[0]), UC(p[1])) / 100) != FTP_COMPLETE)
		if((command(ctrl_skt,NULL, CancelCheckWork, "EPRT |2|%s|%d|",
				AddressToStringIPv6(Adrs, a),
				US(p[0])) / 100) != FTP_COMPLETE)
		{
			SetTaskMsg(MSGJPN031);
			do_closesocket(listen_skt);
			listen_skt = INVALID_SOCKET;
		}
//		else
//			DoPrintf("Skt=%u : listener %s port %u",listen_skt,inet_ntoa(saCtrlAddr.sin_addr),ntohs(saCtrlAddr.sin_port));
	}

	return(listen_skt);
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


#if 0
///*----- ブロッキングコールのフックコールバック --------------------------------
//*
//*	Parameter
//*		なし
//*
//*	Return Value
//*		BOOL FALSE
//*----------------------------------------------------------------------------*/
//
//static BOOL CALLBACK BlkHookFnc(void)
//{
//	BackgrndMessageProc();
//
//	if(CancelFlg == YES)
//	{
//		SetTaskMsg(MSGJPN032);
//		WSACancelBlockingCall();
//		CancelFlg = NO;
//	}
//	return(FALSE);
//}
#endif



/*----- SOCKS5のコマンドパケットを作成する ------------------------------------
*
*	Parameter
*		SOCKS5REQUEST *Packet : パケットを作成するワーク
*		char Cmd : コマンド
*		int ValidIP : IPアドレスを使うかどうか(YES/NO)
*		ulong IP : IPアドレス
*		char *Host : ホスト名
*		ushort Port : ポート
*
*	Return Value
*		int コマンドパケットの長さ
*----------------------------------------------------------------------------*/

static int Socks5MakeCmdPacket(SOCKS5REQUEST *Packet, char Cmd, int ValidIP, ulong IP, char *Host, ushort Port)
{
	uchar *Pos;
	int Len;
	int TotalLen;

	Pos = (uchar *)Packet;
	Pos += SOCKS5REQUEST_SIZE;
	TotalLen = SOCKS5REQUEST_SIZE + 2;	/* +2はポートの分 */

	Packet->Ver = SOCKS5_VER;
	Packet->Cmd = Cmd;
	Packet->Rsv = 0;
	if(ValidIP == YES)
	{
		/* IPアドレスを指定 */
		Packet->Type = SOCKS5_ADRS_IPV4;
		*((ulong *)Pos) = IP;
		Pos += 4;
		TotalLen += 4;
	}
	else
	{
		/* ホスト名を指定 */
		Packet->Type = SOCKS5_ADRS_NAME;
		Len = strlen(Host);
		*Pos++ = Len;
		strcpy(Pos, Host);
		Pos += Len;
		TotalLen += Len + 1;
	}
	*((ushort *)Pos) = Port;

	return(TotalLen);
}


// IPv6対応
static int Socks5MakeCmdPacketIPv6(SOCKS5REQUEST *Packet, char Cmd, int ValidIP, char *IP, char *Host, ushort Port)
{
	uchar *Pos;
	int Len;
	int TotalLen;

	Pos = (uchar *)Packet;
	Pos += SOCKS5REQUEST_SIZE;
	TotalLen = SOCKS5REQUEST_SIZE + 2;	/* +2はポートの分 */

	Packet->Ver = SOCKS5_VER;
	Packet->Cmd = Cmd;
	Packet->Rsv = 0;
	if(ValidIP == YES)
	{
		/* IPアドレスを指定 */
		Packet->Type = SOCKS5_ADRS_IPV6;
		memcpy(Pos, IP, 16);
		Pos += 16;
		TotalLen += 16;
	}
	else
	{
		/* ホスト名を指定 */
		Packet->Type = SOCKS5_ADRS_NAME;
		Len = strlen(Host);
		*Pos++ = Len;
		strcpy(Pos, Host);
		Pos += Len;
		TotalLen += Len + 1;
	}
	*((ushort *)Pos) = Port;

	return(TotalLen);
}


/*----- SOCKSのコマンドを送る -------------------------------------------------
*
*	Parameter
*		SOCKET Socket : ソケット
*		void *Data : 送るデータ
*		int Size : サイズ
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

static int SocksSendCmd(SOCKET Socket, void *Data, int Size, int *CancelCheckWork)
{
	int Ret;

	Ret = SendData(Socket, (char *)Data, Size, 0, CancelCheckWork);

	if(Ret != FFFTP_SUCCESS)
		SetTaskMsg(MSGJPN033, *((short *)Data));

	return(Ret);
}


/*----- SOCKS5のコマンドに対するリプライパケットを受信する --------------------
*
*	Parameter
*		SOCKET Socket : ソケット
*		SOCKS5REPLY *Packet : パケット
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

// 同時接続対応
//static int Socks5GetCmdReply(SOCKET Socket, SOCKS5REPLY *Packet)
static int Socks5GetCmdReply(SOCKET Socket, SOCKS5REPLY *Packet, int *CancelCheckWork)
{
	uchar *Pos;
	int Len;
	int Ret;

	Pos = (uchar *)Packet;
	Pos += SOCKS5REPLY_SIZE;

	// 同時接続対応
//	if((Ret = ReadNchar(Socket, (char *)Packet, SOCKS5REPLY_SIZE, &CancelFlg)) == FFFTP_SUCCESS)
	if((Ret = ReadNchar(Socket, (char *)Packet, SOCKS5REPLY_SIZE, CancelCheckWork)) == FFFTP_SUCCESS)
	{
		if(Packet->Type == SOCKS5_ADRS_IPV4)
			Len = 4 + 2;
		else if(Packet->Type == SOCKS5_ADRS_IPV6)
			Len = 16 + 2;
		else
		{
			// 同時接続対応
//			if((Ret = ReadNchar(Socket, (char *)Pos, 1, &CancelFlg)) == FFFTP_SUCCESS)
			if((Ret = ReadNchar(Socket, (char *)Pos, 1, CancelCheckWork)) == FFFTP_SUCCESS)
			{
				Len = *Pos + 2;
				Pos++;
			}
		}

		if(Ret == FFFTP_SUCCESS)
			// 同時接続対応
//			Ret = ReadNchar(Socket, (char *)Pos, Len, &CancelFlg);
			Ret = ReadNchar(Socket, (char *)Pos, Len, CancelCheckWork);
	}

	if(Ret != FFFTP_SUCCESS)
		SetTaskMsg(MSGJPN034);

	return(Ret);
}


/*----- SOCKS4のコマンドに対するリプライパケットを受信する --------------------
*
*	Parameter
*		SOCKET Socket : ソケット
*		SOCKS5REPLY *Packet : パケット
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

// 同時接続対応
//static int Socks4GetCmdReply(SOCKET Socket, SOCKS4REPLY *Packet)
static int Socks4GetCmdReply(SOCKET Socket, SOCKS4REPLY *Packet, int *CancelCheckWork)
{
	int Ret;

	// 同時接続対応
//	Ret = ReadNchar(Socket, (char *)Packet, SOCKS4REPLY_SIZE, &CancelFlg);
	Ret = ReadNchar(Socket, (char *)Packet, SOCKS4REPLY_SIZE, CancelCheckWork);

	if(Ret != FFFTP_SUCCESS)
		DoPrintf(MSGJPN035);

	return(Ret);
}


/*----- SOCKS5の認証を行う ----------------------------------------------------
*
*	Parameter
*		SOCKET Socket : ソケット
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

static int Socks5SelMethod(SOCKET Socket, int *CancelCheckWork)
{
	int Ret;
	SOCKS5METHODREQUEST Socks5Method;
	SOCKS5METHODREPLY Socks5MethodReply;
	SOCKS5USERPASSSTATUS Socks5Status;
	char Buf[USER_NAME_LEN + PASSWORD_LEN + 4];
	int Len;
	int Len2;

	Ret = FFFTP_SUCCESS;
	Socks5Method.Ver = SOCKS5_VER;
	Socks5Method.Num = 1;
	if(FwallType == FWALL_SOCKS5_NOAUTH)
		Socks5Method.Methods[0] = SOCKS5_AUTH_NONE;
	else
		Socks5Method.Methods[0] = SOCKS5_AUTH_USER;

	// 同時接続対応
//	if((SocksSendCmd(Socket, &Socks5Method, SOCKS5METHODREQUEST_SIZE, CancelCheckWork) != FFFTP_SUCCESS) ||
//	   (ReadNchar(Socket, (char *)&Socks5MethodReply, SOCKS5METHODREPLY_SIZE, &CancelFlg) != FFFTP_SUCCESS) ||
//	   (Socks5MethodReply.Method == (uchar)0xFF))
	if((SocksSendCmd(Socket, &Socks5Method, SOCKS5METHODREQUEST_SIZE, CancelCheckWork) != FFFTP_SUCCESS) ||
	   (ReadNchar(Socket, (char *)&Socks5MethodReply, SOCKS5METHODREPLY_SIZE, CancelCheckWork) != FFFTP_SUCCESS) ||
	   (Socks5MethodReply.Method == (uchar)0xFF))
	{
		SetTaskMsg(MSGJPN036);
		Ret = FFFTP_FAIL;
	}
	else if(Socks5MethodReply.Method == SOCKS5_AUTH_USER)
	{
		DoPrintf("SOCKS5 User/Pass Authentication");
		Buf[0] = SOCKS5_USERAUTH_VER;
		Len = strlen(FwallUser);
		Len2 = strlen(FwallPass);
		Buf[1] = Len;
		strcpy(Buf+2, FwallUser);
		Buf[2 + Len] = Len2;
		strcpy(Buf+3+Len, FwallPass);

		// 同時接続対応
//		if((SocksSendCmd(Socket, &Buf, Len+Len2+3, CancelCheckWork) != FFFTP_SUCCESS) ||
//		   (ReadNchar(Socket, (char *)&Socks5Status, SOCKS5USERPASSSTATUS_SIZE, &CancelFlg) != FFFTP_SUCCESS) ||
//		   (Socks5Status.Status != 0))
		if((SocksSendCmd(Socket, &Buf, Len+Len2+3, CancelCheckWork) != FFFTP_SUCCESS) ||
		   (ReadNchar(Socket, (char *)&Socks5Status, SOCKS5USERPASSSTATUS_SIZE, CancelCheckWork) != FFFTP_SUCCESS) ||
		   (Socks5Status.Status != 0))
		{
			SetTaskMsg(MSGJPN037);
			Ret = FFFTP_FAIL;
		}
	}
	else
		DoPrintf("SOCKS5 No Authentication");

	return(Ret);
}


/*----- SOCKSのBINDの第２リプライメッセージを受け取る -------------------------
*
*	Parameter
*		SOCKET Socket : ソケット
*		SOCKET *Data : データソケットを返すワーク
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

// 同時接続対応
//int SocksGet2ndBindReply(SOCKET Socket, SOCKET *Data)
int SocksGet2ndBindReply(SOCKET Socket, SOCKET *Data, int *CancelCheckWork)
{
	int Ret;
	char Buf[300];

	Ret = FFFTP_FAIL;
	if((AskHostFireWall() == YES) && (FwallType == FWALL_SOCKS4))
	{
		// 同時接続対応
//		Socks4GetCmdReply(Socket, (SOCKS4REPLY *)Buf);
		Socks4GetCmdReply(Socket, (SOCKS4REPLY *)Buf, CancelCheckWork);
		*Data = Socket;
		Ret = FFFTP_SUCCESS;
	}
	else if((AskHostFireWall() == YES) &&
			((FwallType == FWALL_SOCKS5_NOAUTH) || (FwallType == FWALL_SOCKS5_USER)))
	{
		// 同時接続対応
//		Socks5GetCmdReply(Socket, (SOCKS5REPLY *)Buf);
		Socks5GetCmdReply(Socket, (SOCKS5REPLY *)Buf, CancelCheckWork);
		*Data = Socket;
		Ret = FFFTP_SUCCESS;
	}
	return(Ret);
}



// 暗号化通信対応
int AskCryptMode(void)
{
	return(CurHost.CryptMode);
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
int AskInetFamily(void)
{
	return(CurHost.InetFamily);
}

int AskUseIPv6(void)
{
	return(CurHost.UseIPv6);
}

