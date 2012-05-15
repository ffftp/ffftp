/*=============================================================================
*								レジストリ関係
*
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

// VC 8.0(2005)以上でのみ rand_s を利用可能
#if 1400 <= _MSC_VER
//#define _CRT_RAND_S
#endif

#define	STRICT
// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <windowsx.h>
#include <process.h>


#include "common.h"
#include "resource.h"
#include "sample.h"
#include "sha.h"
#include "aes.h"


/*===== プロトタイプ =====*/

static void SaveStr(HKEY hKey, char *Key, char *Str, char *DefaultStr);
static void SaveIntNum(HKEY hKey, char *Key, int Num, int DefaultNum);
static void MakeFontData(LOGFONT Font, HFONT hFont, char *Buf);
static int RestoreFontData(char *Str, LOGFONT *Font);

static void EncodePassword(char *Str, char *Buf);
static void EncodePasswordOriginal(char *Str, char *Buf);
static void EncodePassword2(char *Str, char *Buf, const char *Key);
static void EncodePassword3(char *Str, char *Buf, const char *Key);

static void DecodePassword(char *Str, char *Buf);
static void DecodePasswordOriginal(char *Str, char *Buf);
static void DecodePassword2(char *Str, char *Buf, const char *Key);
static void DecodePassword3(char *Str, char *Buf, const char *Key);
static int CreateAesKey(unsigned char *AesKey, const char* Key);

static void SetRegType(int Type);
static int OpenReg(char *Name, void **Handle);
static int CreateReg(char *Name, void **Handle);
static int CloseReg(void *Handle);
static int OpenSubKey(void *Parent, char *Name, void **Handle);
static int CreateSubKey(void *Parent, char *Name, void **Handle);
static int CloseSubKey(void *Handle);
static int DeleteSubKey(void *Handle, char *Name);
static int DeleteValue(void *Handle, char *Name);
static int ReadIntValueFromReg(void *Handle, char *Name, int *Value);
static int WriteIntValueToReg(void *Handle, char *Name, int Value);
static int ReadStringFromReg(void *Handle, char *Name, char *Str, DWORD Size);
static int WriteStringToReg(void *Handle, char *Name, char *Str);
static int ReadMultiStringFromReg(void *Handle, char *Name, char *Str, DWORD Size);
static int WriteMultiStringToReg(void *Handle, char *Name, char *Str);
static int ReadBinaryFromReg(void *Handle, char *Name, void *Bin, DWORD Size);
static int WriteBinaryToReg(void *Handle, char *Name, void *Bin, int Len);
// 暗号化通信対応
static int StrCatOut(char *Src, int Len, char *Dst);
static int StrReadIn(char *Src, int Max, char *Dst);

int CheckPasswordValidity( char* Password, int length, const char* HashStr );
void CreatePasswordHash( char* Password, int length, char* HashStr );
void SetHashSalt( DWORD salt );

DWORD GetRandamDWRODValue(void);

/* 2010.01.30 genta 追加 */
static char SecretKey[FMAX_PATH+1];
static int SecretKeyLength;
static int IsMasterPasswordError = PASSWORD_OK;

static int IsRndSourceInit = 0;
static ulong RndSource[9];

// UTF-8対応
static int IniKanjiCode = KANJI_NOCNV;

/*===== 外部参照 =====*/

/* 設定値 */
extern int WinPosX;
extern int WinPosY;
extern int WinWidth;
extern int WinHeight;
extern int LocalWidth;
extern int TaskHeight;
extern int LocalTabWidth[4];
extern int RemoteTabWidth[6];
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern char ViewerName[VIEWERS][FMAX_PATH+1];
extern HFONT ListFont;
extern LOGFONT ListLogFont;
extern int LocalFileSort;
extern int LocalDirSort;
extern int RemoteFileSort;
extern int RemoteDirSort;
extern int TransMode;
extern int ConnectOnStart;
extern int DebugConsole;
extern int SaveWinPos;
extern char AsciiExt[ASCII_EXT_LEN+1];
extern int RecvMode;
extern int SendMode;
extern int MoveMode;
extern int ListType;
extern int CacheEntry;
extern int CacheSave;
extern char DefaultLocalPath[FMAX_PATH+1];
extern int SaveTimeStamp;
extern int FindMode;
extern int DotFile;
extern int DclickOpen;
extern SOUNDFILE Sound[SOUND_TYPES];
extern int FnameCnv;
extern int ConnectAndSet;
extern int TimeOut;
extern int RmEOF;
extern int RegType;
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
extern char MirrorNoTrn[MIRROR_LEN+1];
extern char MirrorNoDel[MIRROR_LEN+1];
extern int MirrorFnameCnv;
//extern int MirrorFolderCnv;
extern int RasClose;
extern int RasCloseNotify;
extern int FileHist;
extern char DefAttrList[DEFATTRLIST_LEN+1];
extern SIZE HostDlgSize;
extern SIZE BmarkDlgSize;
extern SIZE MirrorDlgSize;
extern int Sizing;
extern int SortSave;
extern char TmpPath[FMAX_PATH+1];
extern int QuickAnonymous;
extern int PassToHist;
extern int VaxSemicolon;
extern int SendQuit;
extern int NoRasControl;
extern int SuppressSave;

extern int UpExistMode;
extern int ExistMode;
extern int DispIgnoreHide;
extern int DispDrives;
extern int MirUpDelNotify;
extern int MirDownDelNotify;

extern int FolderAttr;
extern int FolderAttrNum;

// 暗号化通信対応
extern BYTE CertificateCacheHash[MAX_CERT_CACHE_HASH][20];
extern BYTE SSLRootCAFileHash[20];
// ファイルアイコン表示対応
extern int DispFileIcon;
// ディレクトリ自動作成
extern int MakeAllDir;

/*----- マスタパスワードの設定 ----------------------------------------------
*
*	Parameter
*		const char* Password : マスターパスワード
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
void SetMasterPassword( const char* Password )
{
	ZeroMemory( SecretKey, MAX_PASSWORD_LEN + 12 );
	if( Password != NULL ){
		strncpy( SecretKey, Password, MAX_PASSWORD_LEN );
	}
	else {
		strcpy( SecretKey, DEFAULT_PASSWORD );
	}
	SecretKeyLength = strlen( SecretKey );
	
	/* 未検証なので，初期状態はOKにする (強制再設定→保存にを可能にする)*/
	IsMasterPasswordError = PASSWORD_OK;
}

// セキュリティ強化
void GetMasterPassword(char* Password)
{
	strcpy(Password, SecretKey);
}

/*----- マスタパスワードの状態取得 ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		PASSWORD_OK : OK
*		PASSWORD_UNMATCH : パスワード不一致
*		BAD_PASSWORD_HASH : パスワード確認失敗
*----------------------------------------------------------------------------*/
int GetMasterPasswordStatus(void)
{
	return IsMasterPasswordError;
}

/*----- レジストリ／INIファイルのマスターパスワードの検証を行う ------------
*
*	Parameter
*		なし
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

int ValidateMasterPassword(void)
{
	void *hKey3;
	int i;

	SetRegType(REGTYPE_INI);
	if((i = OpenReg("FFFTP", &hKey3)) != FFFTP_SUCCESS)
	{
		if(AskForceIni() == NO)
		{
			SetRegType(REGTYPE_REG);
			i = OpenReg("FFFTP", &hKey3);
		}
	}
	if(i == FFFTP_SUCCESS){
		char checkbuf[48];
		int salt = 0;

		if( ReadIntValueFromReg(hKey3, "CredentialSalt", &salt)){
			SetHashSalt( salt );
		}
		if( ReadStringFromReg(hKey3, "CredentialCheck", checkbuf, sizeof( checkbuf )) == FFFTP_SUCCESS ){
			switch( CheckPasswordValidity( SecretKey, SecretKeyLength, checkbuf ) ){
			case 0: /* not match */
				IsMasterPasswordError = PASSWORD_UNMATCH;
				break;
			case 1: /* match */
				IsMasterPasswordError = PASSWORD_OK;
				break;
			case 2: /* invalid hash */
			default:
				IsMasterPasswordError = BAD_PASSWORD_HASH;
				break;
			}
		}
		CloseReg(hKey3);
		return YES;
	}
	return NO;
}

/*----- レジストリ／INIファイルに設定値を保存 ---------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SaveRegistry(void)
{
	void *hKey3;
	void *hKey4;
	void *hKey5;
	// 暗号化通信対応
//	char Str[FMAX_PATH+1];
	char Str[PRIVATE_KEY_LEN*4+1];
	char Buf[FMAX_PATH+1];
	int i;
	int n;
	HOSTDATA DefaultHost;
	HOSTDATA Host;
	HISTORYDATA Hist;
	HISTORYDATA DefaultHist;
	
	if( GetMasterPasswordStatus() == PASSWORD_UNMATCH ){
		/* 2010.01.30 genta: マスターパスワードが不一致の場合は不用意に上書きしない */
		return;
	}

	SetRegType(RegType);
	if(CreateReg("FFFTP", &hKey3) == FFFTP_SUCCESS)
	{
		char buf[48];
		int salt = GetTickCount();
	
		WriteIntValueToReg(hKey3, "Version", VER_NUM);
		WriteIntValueToReg(hKey3, "CredentialSalt", salt);
		
		SetHashSalt( salt );
		/* save password hash */
		CreatePasswordHash( SecretKey, SecretKeyLength, buf );
		WriteStringToReg(hKey3, "CredentialCheck", buf);

		if(CreateSubKey(hKey3, "Options", &hKey4) == FFFTP_SUCCESS)
		{
			WriteIntValueToReg(hKey4, "NoSave", SuppressSave);

			if(SuppressSave != YES)
			{
				WriteIntValueToReg(hKey4, "WinPosX", WinPosX);
				WriteIntValueToReg(hKey4, "WinPosY", WinPosY);
				WriteIntValueToReg(hKey4, "WinWidth", WinWidth);
				WriteIntValueToReg(hKey4, "WinHeight", WinHeight);
				WriteIntValueToReg(hKey4, "LocalWidth", LocalWidth);
				WriteIntValueToReg(hKey4, "TaskHeight", TaskHeight);
				WriteBinaryToReg(hKey4, "LocalColm", LocalTabWidth, sizeof(LocalTabWidth));
				WriteBinaryToReg(hKey4, "RemoteColm", RemoteTabWidth, sizeof(RemoteTabWidth));
				WriteIntValueToReg(hKey4, "SwCmd", Sizing);

				WriteStringToReg(hKey4, "UserMail", UserMailAdrs);
				WriteStringToReg(hKey4, "Viewer", ViewerName[0]);
				WriteStringToReg(hKey4, "Viewer2", ViewerName[1]);
				WriteStringToReg(hKey4, "Viewer3", ViewerName[2]);

				WriteIntValueToReg(hKey4, "TrType", TransMode);
				WriteIntValueToReg(hKey4, "Recv", RecvMode);
				WriteIntValueToReg(hKey4, "Send", SendMode);
				WriteIntValueToReg(hKey4, "Move", MoveMode);
				WriteStringToReg(hKey4, "Path", DefaultLocalPath);
				WriteIntValueToReg(hKey4, "Time", SaveTimeStamp);
				WriteIntValueToReg(hKey4, "EOF", RmEOF);
				WriteIntValueToReg(hKey4, "Scolon", VaxSemicolon);

				WriteIntValueToReg(hKey4, "RecvEx", ExistMode);
				WriteIntValueToReg(hKey4, "SendEx", UpExistMode);

				WriteIntValueToReg(hKey4, "LFsort", LocalFileSort);
				WriteIntValueToReg(hKey4, "LDsort", LocalDirSort);
				WriteIntValueToReg(hKey4, "RFsort", RemoteFileSort);
				WriteIntValueToReg(hKey4, "RDsort", RemoteDirSort);
				WriteIntValueToReg(hKey4, "SortSave", SortSave);

				WriteIntValueToReg(hKey4, "ListType", ListType);
				WriteIntValueToReg(hKey4, "Cache", CacheEntry);
				WriteIntValueToReg(hKey4, "CacheSave", CacheSave);
				WriteIntValueToReg(hKey4, "DotFile", DotFile);
				WriteIntValueToReg(hKey4, "Dclick", DclickOpen);

				WriteIntValueToReg(hKey4, "ConS", ConnectOnStart);
				WriteIntValueToReg(hKey4, "OldDlg", ConnectAndSet);
				WriteIntValueToReg(hKey4, "RasClose", RasClose);
				WriteIntValueToReg(hKey4, "RasNotify", RasCloseNotify);
				WriteIntValueToReg(hKey4, "Qanony", QuickAnonymous);
				WriteIntValueToReg(hKey4, "PassHist", PassToHist);
				WriteIntValueToReg(hKey4, "SendQuit", SendQuit);
				WriteIntValueToReg(hKey4, "NoRas", NoRasControl);

				WriteIntValueToReg(hKey4, "Debug", DebugConsole);
				WriteIntValueToReg(hKey4, "WinPos", SaveWinPos);
				WriteIntValueToReg(hKey4, "RegExp", FindMode);
				WriteIntValueToReg(hKey4, "Reg", RegType);

				WriteMultiStringToReg(hKey4, "AsciiFile", AsciiExt);
				WriteIntValueToReg(hKey4, "LowUp", FnameCnv);
				WriteIntValueToReg(hKey4, "Tout", TimeOut);

				WriteMultiStringToReg(hKey4, "NoTrn", MirrorNoTrn);
				WriteMultiStringToReg(hKey4, "NoDel", MirrorNoDel);
				WriteIntValueToReg(hKey4, "MirFile", MirrorFnameCnv);
				WriteIntValueToReg(hKey4, "MirUNot", MirUpDelNotify);
				WriteIntValueToReg(hKey4, "MirDNot", MirDownDelNotify);

				MakeFontData(ListLogFont, ListFont, Str);
				WriteStringToReg(hKey4, "ListFont", Str);
				WriteIntValueToReg(hKey4, "ListHide", DispIgnoreHide);
				WriteIntValueToReg(hKey4, "ListDrv", DispDrives);

				WriteStringToReg(hKey4, "FwallHost", FwallHost);
				WriteStringToReg(hKey4, "FwallUser", FwallUser);
				EncodePassword(FwallPass, Str);
				WriteStringToReg(hKey4, "FwallPass", Str);
				WriteIntValueToReg(hKey4, "FwallPort", FwallPort);
				WriteIntValueToReg(hKey4, "FwallType", FwallType);
				WriteIntValueToReg(hKey4, "FwallDef", FwallDefault);
				WriteIntValueToReg(hKey4, "FwallSec", FwallSecurity);
				WriteIntValueToReg(hKey4, "PasvDef", PasvDefault);
				WriteIntValueToReg(hKey4, "FwallRes", FwallResolv);
				WriteIntValueToReg(hKey4, "FwallLow", FwallLower);
				WriteIntValueToReg(hKey4, "FwallDel", FwallDelimiter);

				WriteIntValueToReg(hKey4, "SndConSw", Sound[SND_CONNECT].On);
				WriteIntValueToReg(hKey4, "SndTrnSw", Sound[SND_TRANS].On);
				WriteIntValueToReg(hKey4, "SndErrSw", Sound[SND_ERROR].On);
				WriteStringToReg(hKey4, "SndCon", Sound[SND_CONNECT].Fname);
				WriteStringToReg(hKey4, "SndTrn", Sound[SND_TRANS].Fname);
				WriteStringToReg(hKey4, "SndErr", Sound[SND_ERROR].Fname);

				WriteMultiStringToReg(hKey4, "DefAttr", DefAttrList);

				// 環境依存の不具合対策
//				GetTempPath(FMAX_PATH, Str);
				GetAppTempPath(Str);
				SetYenTail(Str);
				SaveStr(hKey4, "Tmp", TmpPath, Str);

				WriteBinaryToReg(hKey4, "Hdlg", &HostDlgSize, sizeof(SIZE));
				WriteBinaryToReg(hKey4, "Bdlg", &BmarkDlgSize, sizeof(SIZE));
				WriteBinaryToReg(hKey4, "Mdlg", &MirrorDlgSize, sizeof(SIZE));

				WriteIntValueToReg(hKey4, "FAttrSw", FolderAttr);
				WriteIntValueToReg(hKey4, "FAttr", FolderAttrNum);

				WriteIntValueToReg(hKey4, "HistNum", FileHist);

				/* Ver1.54a以前の形式のヒストリデータは削除 */
				DeleteValue(hKey4, "Hist");

				/* ヒストリの設定を保存 */
				CopyDefaultHistory(&DefaultHist);
				n = 0;
				for(i = AskHistoryNum(); i > 0; i--)
				{
					if(GetHistoryByNum(i-1, &Hist) == FFFTP_SUCCESS)
					{
						sprintf(Str, "History%d", n);
						if(CreateSubKey(hKey4, Str, &hKey5) == FFFTP_SUCCESS)
						{
							SaveStr(hKey5, "HostAdrs", Hist.HostAdrs, DefaultHist.HostAdrs);
							SaveStr(hKey5, "UserName", Hist.UserName, DefaultHist.UserName);
							SaveStr(hKey5, "Account", Hist.Account, DefaultHist.Account);
							SaveStr(hKey5, "LocalDir", Hist.LocalInitDir, NULL);
							SaveStr(hKey5, "RemoteDir", Hist.RemoteInitDir, DefaultHist.RemoteInitDir);
							SaveStr(hKey5, "Chmod", Hist.ChmodCmd, DefaultHist.ChmodCmd);
							SaveStr(hKey5, "Nlst", Hist.LsName, DefaultHist.LsName);
							SaveStr(hKey5, "Init", Hist.InitCmd, DefaultHist.InitCmd);
							EncodePassword(Hist.PassWord, Str);
							SaveStr(hKey5, "Password", Str, DefaultHist.PassWord);
							SaveIntNum(hKey5, "Port", Hist.Port, DefaultHist.Port);
							SaveIntNum(hKey5, "Kanji", Hist.KanjiCode, DefaultHist.KanjiCode);
							SaveIntNum(hKey5, "KanaCnv", Hist.KanaCnv, DefaultHist.KanaCnv);
							SaveIntNum(hKey5, "NameKanji", Hist.NameKanjiCode, DefaultHist.NameKanjiCode);
							SaveIntNum(hKey5, "NameKana", Hist.NameKanaCnv, DefaultHist.NameKanaCnv);
							SaveIntNum(hKey5, "Pasv", Hist.Pasv, DefaultHist.Pasv);
							SaveIntNum(hKey5, "Fwall", Hist.FireWall, DefaultHist.FireWall);
							SaveIntNum(hKey5, "List", Hist.ListCmdOnly, DefaultHist.ListCmdOnly);
							SaveIntNum(hKey5, "NLST-R", Hist.UseNLST_R, DefaultHist.UseNLST_R);
							SaveIntNum(hKey5, "Tzone", Hist.TimeZone, DefaultHist.TimeZone);
							SaveIntNum(hKey5, "Type", Hist.HostType, DefaultHist.HostType);
							SaveIntNum(hKey5, "Sync", Hist.SyncMove, DefaultHist.SyncMove);
							SaveIntNum(hKey5, "Fpath", Hist.NoFullPath, DefaultHist.NoFullPath);
							WriteBinaryToReg(hKey5, "Sort", &Hist.Sort, sizeof(Hist.Sort));
							SaveIntNum(hKey5, "Secu", Hist.Security, DefaultHist.Security);
							WriteIntValueToReg(hKey5, "TrType", Hist.Type);
							SaveIntNum(hKey5, "Dial", Hist.Dialup, DefaultHist.Dialup);
							SaveIntNum(hKey5, "UseIt", Hist.DialupAlways, DefaultHist.DialupAlways);
							SaveIntNum(hKey5, "Notify", Hist.DialupNotify, DefaultHist.DialupNotify);
							SaveStr(hKey5, "DialTo", Hist.DialEntry, DefaultHist.DialEntry);
							// 暗号化通信対応
							SaveIntNum(hKey5, "NoEncryption", Hist.UseNoEncryption, DefaultHist.UseNoEncryption);
							SaveIntNum(hKey5, "FTPES", Hist.UseFTPES, DefaultHist.UseFTPES);
							SaveIntNum(hKey5, "FTPIS", Hist.UseFTPIS, DefaultHist.UseFTPIS);
							SaveIntNum(hKey5, "SFTP", Hist.UseSFTP, DefaultHist.UseSFTP);
							EncodePassword(Hist.PrivateKey, Str);
							SaveStr(hKey5, "PKey", Str, DefaultHist.PrivateKey);
							// 同時接続対応
							SaveIntNum(hKey5, "ThreadCount", Hist.MaxThreadCount, DefaultHist.MaxThreadCount);
							SaveIntNum(hKey5, "ReuseCmdSkt", Hist.ReuseCmdSkt, DefaultHist.ReuseCmdSkt);
							// MLSD対応
							SaveIntNum(hKey5, "MLSD", Hist.UseMLSD, DefaultHist.UseMLSD);
							// IPv6対応
							SaveIntNum(hKey5, "NetType", Hist.NetType, DefaultHist.NetType);
							// 自動切断対策
							SaveIntNum(hKey5, "Noop", Hist.NoopInterval, DefaultHist.NoopInterval);
							// 再転送対応
							SaveIntNum(hKey5, "ErrMode", Hist.TransferErrorMode, DefaultHist.TransferErrorMode);
							SaveIntNum(hKey5, "ErrNotify", Hist.TransferErrorNotify, DefaultHist.TransferErrorNotify);

							CloseSubKey(hKey5);
							n++;
						}
					}
				}
				WriteIntValueToReg(hKey4, "SavedHist", n);

				/* 余分なヒストリがあったら削除 */
				for(; n < 999; n++)
				{
					sprintf(Str, "History%d", n);
					if(DeleteSubKey(hKey4, Str) != FFFTP_SUCCESS)
						break;
				}

				/* ホストの設定を保存 */
				CopyDefaultHost(&DefaultHost);
				i = 0;
				while(CopyHostFromList(i, &Host) == FFFTP_SUCCESS)
				{
					sprintf(Str, "Host%d", i);
					if(CreateSubKey(hKey4, Str, &hKey5) == FFFTP_SUCCESS)
					{
//						SaveIntNum(hKey5, "Set", Host.Level, DefaultHost.Level);
						WriteIntValueToReg(hKey5, "Set", Host.Level);
						SaveStr(hKey5, "HostName", Host.HostName, DefaultHost.HostName);
						if((Host.Level & SET_LEVEL_GROUP) == 0)
						{
							SaveStr(hKey5, "HostAdrs", Host.HostAdrs, DefaultHost.HostAdrs);
							SaveStr(hKey5, "UserName", Host.UserName, DefaultHost.UserName);
							SaveStr(hKey5, "Account", Host.Account, DefaultHost.Account);
							SaveStr(hKey5, "LocalDir", Host.LocalInitDir, NULL);
							SaveStr(hKey5, "RemoteDir", Host.RemoteInitDir, DefaultHost.RemoteInitDir);
							SaveStr(hKey5, "Chmod", Host.ChmodCmd, DefaultHost.ChmodCmd);
							SaveStr(hKey5, "Nlst", Host.LsName, DefaultHost.LsName);
							SaveStr(hKey5, "Init", Host.InitCmd, DefaultHost.InitCmd);

							if(Host.Anonymous == NO)
								EncodePassword(Host.PassWord, Str);
							else
								strcpy(Str, DefaultHost.PassWord);
							SaveStr(hKey5, "Password", Str, DefaultHost.PassWord);

							SaveIntNum(hKey5, "Port", Host.Port, DefaultHost.Port);
							SaveIntNum(hKey5, "Anonymous", Host.Anonymous, DefaultHost.Anonymous);
							SaveIntNum(hKey5, "Kanji", Host.KanjiCode, DefaultHost.KanjiCode);
							SaveIntNum(hKey5, "KanaCnv", Host.KanaCnv, DefaultHost.KanaCnv);
							SaveIntNum(hKey5, "NameKanji", Host.NameKanjiCode, DefaultHost.NameKanjiCode);
							SaveIntNum(hKey5, "NameKana", Host.NameKanaCnv, DefaultHost.NameKanaCnv);
							SaveIntNum(hKey5, "Pasv", Host.Pasv, DefaultHost.Pasv);
							SaveIntNum(hKey5, "Fwall", Host.FireWall, DefaultHost.FireWall);
							SaveIntNum(hKey5, "List", Host.ListCmdOnly, DefaultHost.ListCmdOnly);
							SaveIntNum(hKey5, "NLST-R", Host.UseNLST_R, DefaultHost.UseNLST_R);
							SaveIntNum(hKey5, "Last", Host.LastDir, DefaultHost.LastDir);
							SaveIntNum(hKey5, "Tzone", Host.TimeZone, DefaultHost.TimeZone);
							SaveIntNum(hKey5, "Type", Host.HostType, DefaultHost.HostType);
							SaveIntNum(hKey5, "Sync", Host.SyncMove, DefaultHost.SyncMove);
							SaveIntNum(hKey5, "Fpath", Host.NoFullPath, DefaultHost.NoFullPath);
							WriteBinaryToReg(hKey5, "Sort", &Host.Sort, sizeof(Host.Sort));
							SaveIntNum(hKey5, "Secu", Host.Security, DefaultHost.Security);

							WriteMultiStringToReg(hKey5, "Bmarks", Host.BookMark);

							SaveIntNum(hKey5, "Dial", Host.Dialup, DefaultHost.Dialup);
							SaveIntNum(hKey5, "UseIt", Host.DialupAlways, DefaultHost.DialupAlways);
							SaveIntNum(hKey5, "Notify", Host.DialupNotify, DefaultHost.DialupNotify);
							SaveStr(hKey5, "DialTo", Host.DialEntry, DefaultHost.DialEntry);
							// 暗号化通信対応
							SaveIntNum(hKey5, "NoEncryption", Host.UseNoEncryption, DefaultHost.UseNoEncryption);
							SaveIntNum(hKey5, "FTPES", Host.UseFTPES, DefaultHost.UseFTPES);
							SaveIntNum(hKey5, "FTPIS", Host.UseFTPIS, DefaultHost.UseFTPIS);
							SaveIntNum(hKey5, "SFTP", Host.UseSFTP, DefaultHost.UseSFTP);
							EncodePassword(Host.PrivateKey, Str);
							SaveStr(hKey5, "PKey", Str, DefaultHost.PrivateKey);
							// 同時接続対応
							SaveIntNum(hKey5, "ThreadCount", Host.MaxThreadCount, DefaultHost.MaxThreadCount);
							SaveIntNum(hKey5, "ReuseCmdSkt", Host.ReuseCmdSkt, DefaultHost.ReuseCmdSkt);
							// MLSD対応
							SaveIntNum(hKey5, "MLSD", Host.UseMLSD, DefaultHost.UseMLSD);
							// IPv6対応
							SaveIntNum(hKey5, "NetType", Host.NetType, DefaultHost.NetType);
							// 自動切断対策
							SaveIntNum(hKey5, "Noop", Host.NoopInterval, DefaultHost.NoopInterval);
							// 再転送対応
							SaveIntNum(hKey5, "ErrMode", Host.TransferErrorMode, DefaultHost.TransferErrorMode);
							SaveIntNum(hKey5, "ErrNotify", Host.TransferErrorNotify, DefaultHost.TransferErrorNotify);
						}
						CloseSubKey(hKey5);
					}
					i++;
				}
				WriteIntValueToReg(hKey4, "SetNum", i);

				/* 余分なホストの設定があったら削除 */
				for(; i < 998; i++)
				{
					sprintf(Str, "Host%d", i);
					if(DeleteSubKey(hKey4, Str) != FFFTP_SUCCESS)
						break;
				}

				if((i = AskCurrentHost()) == HOSTNUM_NOENTRY)
					i = 0;
				WriteIntValueToReg(hKey4, "CurSet", i);

				// 暗号化通信対応
				WriteBinaryToReg(hKey4, "CertCacheHash", &CertificateCacheHash, sizeof(CertificateCacheHash));
				strcpy(Buf, "");
				StrCatOut((char*)&SSLRootCAFileHash, sizeof(SSLRootCAFileHash), Buf);
				EncodePassword(Buf, Str);
				WriteStringToReg(hKey4, "RootCertHash", Str);
				// ファイルアイコン表示対応
				WriteIntValueToReg(hKey4, "ListIcon", DispFileIcon);
				// ディレクトリ自動作成
				WriteIntValueToReg(hKey4, "MakeDir", MakeAllDir);
			}
			CloseSubKey(hKey4);
		}
		CloseReg(hKey3);
	}
	return;
}

/*----- レジストリ／INIファイルから設定値を呼び出す ---------------------------
*
*	この関数を複数回呼び出すと，ホスト設定が追加される．
*
*	Parameter
*		なし
*
*	Return Value
*		YES: 読み出し成功
*		NO:  読み出し失敗(設定無し)
*----------------------------------------------------------------------------*/

int LoadRegistry(void)
{
	void *hKey3;
	void *hKey4;
	void *hKey5;
	int i;
	int Sets;
	// 暗号化通信対応
//	char Str[256];	/* ASCII_EXT_LENより大きい事 */
	char Str[PRIVATE_KEY_LEN*4+1];
	char Buf[FMAX_PATH+1];
	char *Pos;
	char *Pos2;
	HOSTDATA Host;
	HISTORYDATA Hist;
	int Sts;
	int Version;

	Sts = NO;

	SetRegType(REGTYPE_INI);
	if((i = OpenReg("FFFTP", &hKey3)) != FFFTP_SUCCESS)
	{
		if(AskForceIni() == NO)
		{
			SetRegType(REGTYPE_REG);
			i = OpenReg("FFFTP", &hKey3);
		}
	}

	if(i == FFFTP_SUCCESS)
	{
//		char checkbuf[48];
		int salt = 0;
		Sts = YES;

		ReadIntValueFromReg(hKey3, "Version", &Version);
		// UTF-8対応
		if(Version < 1980)
			IniKanjiCode = KANJI_SJIS;

		if(OpenSubKey(hKey3, "Options", &hKey4) == FFFTP_SUCCESS)
		{
			ReadIntValueFromReg(hKey4, "WinPosX", &WinPosX);
			ReadIntValueFromReg(hKey4, "WinPosY", &WinPosY);
			ReadIntValueFromReg(hKey4, "WinWidth", &WinWidth);
			ReadIntValueFromReg(hKey4, "WinHeight", &WinHeight);
			ReadIntValueFromReg(hKey4, "LocalWidth", &LocalWidth);
			/* ↓旧バージョンのバグ対策 */
			LocalWidth = max1(0, LocalWidth);
			ReadIntValueFromReg(hKey4, "TaskHeight", &TaskHeight);
			/* ↓旧バージョンのバグ対策 */
			TaskHeight = max1(0, TaskHeight);
			ReadBinaryFromReg(hKey4, "LocalColm", &LocalTabWidth, sizeof(LocalTabWidth));
			ReadBinaryFromReg(hKey4, "RemoteColm", &RemoteTabWidth, sizeof(RemoteTabWidth));
			ReadIntValueFromReg(hKey4, "SwCmd", &Sizing);

			ReadStringFromReg(hKey4, "UserMail", UserMailAdrs, USER_MAIL_LEN+1);
			ReadStringFromReg(hKey4, "Viewer", ViewerName[0], FMAX_PATH+1);
			ReadStringFromReg(hKey4, "Viewer2", ViewerName[1], FMAX_PATH+1);
			ReadStringFromReg(hKey4, "Viewer3", ViewerName[2], FMAX_PATH+1);

			ReadIntValueFromReg(hKey4, "TrType", &TransMode);
			ReadIntValueFromReg(hKey4, "Recv", &RecvMode);
			ReadIntValueFromReg(hKey4, "Send", &SendMode);
			ReadIntValueFromReg(hKey4, "Move", &MoveMode);
			ReadStringFromReg(hKey4, "Path", DefaultLocalPath, FMAX_PATH+1);
			ReadIntValueFromReg(hKey4, "Time", &SaveTimeStamp);
			ReadIntValueFromReg(hKey4, "EOF", &RmEOF);
			ReadIntValueFromReg(hKey4, "Scolon", &VaxSemicolon);

			ReadIntValueFromReg(hKey4, "RecvEx", &ExistMode);
			ReadIntValueFromReg(hKey4, "SendEx", &UpExistMode);

			ReadIntValueFromReg(hKey4, "LFsort", &LocalFileSort);
			ReadIntValueFromReg(hKey4, "LDsort", &LocalDirSort);
			ReadIntValueFromReg(hKey4, "RFsort", &RemoteFileSort);
			ReadIntValueFromReg(hKey4, "RDsort", &RemoteDirSort);
			ReadIntValueFromReg(hKey4, "SortSave", &SortSave);

			ReadIntValueFromReg(hKey4, "ListType", &ListType);
			ReadIntValueFromReg(hKey4, "Cache", &CacheEntry);
			ReadIntValueFromReg(hKey4, "CacheSave", &CacheSave);
			ReadIntValueFromReg(hKey4, "DotFile", &DotFile);
			ReadIntValueFromReg(hKey4, "Dclick", &DclickOpen);

			ReadIntValueFromReg(hKey4, "ConS", &ConnectOnStart);
			ReadIntValueFromReg(hKey4, "OldDlg", &ConnectAndSet);
			ReadIntValueFromReg(hKey4, "RasClose", &RasClose);
			ReadIntValueFromReg(hKey4, "RasNotify", &RasCloseNotify);
			ReadIntValueFromReg(hKey4, "Qanony", &QuickAnonymous);
			ReadIntValueFromReg(hKey4, "PassHist", &PassToHist);
			ReadIntValueFromReg(hKey4, "SendQuit", &SendQuit);
			ReadIntValueFromReg(hKey4, "NoRas", &NoRasControl);

			ReadIntValueFromReg(hKey4, "Debug", &DebugConsole);
			ReadIntValueFromReg(hKey4, "WinPos", &SaveWinPos);
			ReadIntValueFromReg(hKey4, "RegExp", &FindMode);
			ReadIntValueFromReg(hKey4, "Reg", &RegType);

			if(ReadMultiStringFromReg(hKey4, "AsciiFile", AsciiExt, ASCII_EXT_LEN+1) == FFFTP_FAIL)
			{
				/* 旧ASCIIモードの拡張子の設定を新しいものに変換 */
				// アスキーモード判別の改良
//				ReadStringFromReg(hKey4, "Ascii", Str, ASCII_EXT_LEN+1);
//				memset(AsciiExt, NUL, ASCII_EXT_LEN+1);
				Str[0] = NUL;
				if(ReadStringFromReg(hKey4, "Ascii", Str, ASCII_EXT_LEN+1) == FFFTP_SUCCESS)
					memset(AsciiExt, NUL, ASCII_EXT_LEN+1);
				Pos = Str;
				while(*Pos != NUL)
				{
					if((Pos2 = strchr(Pos, ';')) == NULL)
						Pos2 = strchr(Pos, NUL);
					if((Pos2 - Pos) > 0)
					{
						if((StrMultiLen(AsciiExt) + (Pos2 - Pos) + 2) >= ASCII_EXT_LEN)
							break;
						strcpy(AsciiExt + StrMultiLen(AsciiExt), "*.");
						strncpy(AsciiExt + StrMultiLen(AsciiExt) - 1, Pos, (Pos2 - Pos));
					}
					Pos = Pos2;
					if(*Pos == ';')
						Pos++;
				}
			}
			// アスキーモード判別の改良
			if(Version < 1986)
			{
				Pos = "*.js\0*.vbs\0*.css\0*.rss\0*.rdf\0*.xml\0*.xhtml\0*.xht\0*.shtml\0*.shtm\0*.sh\0*.py\0*.rb\0*.properties\0*.sql\0*.asp\0*.aspx\0*.php\0*.htaccess\0";
				while(*Pos != NUL)
				{
					Pos2 = AsciiExt;
					while(*Pos2 != NUL)
					{
						if(_stricmp(Pos2, Pos) == 0)
							break;
						Pos2 = strchr(Pos2, NUL) + 1;
					}
					if(*Pos2 == NUL)
					{
						if((StrMultiLen(AsciiExt) + strlen(Pos) + 2) < ASCII_EXT_LEN)
							strncpy(AsciiExt + StrMultiLen(AsciiExt), Pos, strlen(Pos) + 2);
					}
					Pos = strchr(Pos, NUL) + 1;
				}
			}

			ReadIntValueFromReg(hKey4, "LowUp", &FnameCnv);
			ReadIntValueFromReg(hKey4, "Tout", &TimeOut);

			ReadMultiStringFromReg(hKey4, "NoTrn", MirrorNoTrn, MIRROR_LEN+1);
			ReadMultiStringFromReg(hKey4, "NoDel", MirrorNoDel, MIRROR_LEN+1);
			ReadIntValueFromReg(hKey4, "MirFile", &MirrorFnameCnv);
			ReadIntValueFromReg(hKey4, "MirUNot", &MirUpDelNotify);
			ReadIntValueFromReg(hKey4, "MirDNot", &MirDownDelNotify);

			if(ReadStringFromReg(hKey4, "ListFont", Str, 256) == FFFTP_SUCCESS)
			{
				if(RestoreFontData(Str, &ListLogFont) == FFFTP_SUCCESS)
					ListFont = CreateFontIndirect(&ListLogFont);
			}
			ReadIntValueFromReg(hKey4, "ListHide", &DispIgnoreHide);
			ReadIntValueFromReg(hKey4, "ListDrv", &DispDrives);

			ReadStringFromReg(hKey4, "FwallHost", FwallHost, HOST_ADRS_LEN+1);
			ReadStringFromReg(hKey4, "FwallUser", FwallUser, USER_NAME_LEN+1);
			ReadStringFromReg(hKey4, "FwallPass", Str, 255);
			DecodePassword(Str, FwallPass);
			ReadIntValueFromReg(hKey4, "FwallPort", &FwallPort);
			ReadIntValueFromReg(hKey4, "FwallType", &FwallType);
			ReadIntValueFromReg(hKey4, "FwallDef", &FwallDefault);
			ReadIntValueFromReg(hKey4, "FwallSec", &FwallSecurity);
			ReadIntValueFromReg(hKey4, "PasvDef", &PasvDefault);
			ReadIntValueFromReg(hKey4, "FwallRes", &FwallResolv);
			ReadIntValueFromReg(hKey4, "FwallLow", &FwallLower);
			ReadIntValueFromReg(hKey4, "FwallDel", &FwallDelimiter);

			ReadIntValueFromReg(hKey4, "SndConSw", &Sound[SND_CONNECT].On);
			ReadIntValueFromReg(hKey4, "SndTrnSw", &Sound[SND_TRANS].On);
			ReadIntValueFromReg(hKey4, "SndErrSw", &Sound[SND_ERROR].On);
			ReadStringFromReg(hKey4, "SndCon", Sound[SND_CONNECT].Fname, FMAX_PATH+1);
			ReadStringFromReg(hKey4, "SndTrn", Sound[SND_TRANS].Fname, FMAX_PATH+1);
			ReadStringFromReg(hKey4, "SndErr", Sound[SND_ERROR].Fname, FMAX_PATH+1);

			ReadMultiStringFromReg(hKey4, "DefAttr", DefAttrList, DEFATTRLIST_LEN+1);

			ReadStringFromReg(hKey4, "Tmp", TmpPath, FMAX_PATH+1);

			ReadBinaryFromReg(hKey4, "Hdlg", &HostDlgSize, sizeof(SIZE));
			ReadBinaryFromReg(hKey4, "Bdlg", &BmarkDlgSize, sizeof(SIZE));
			ReadBinaryFromReg(hKey4, "Mdlg", &MirrorDlgSize, sizeof(SIZE));

			ReadIntValueFromReg(hKey4, "FAttrSw", &FolderAttr);
			ReadIntValueFromReg(hKey4, "FAttr", &FolderAttrNum);

			ReadIntValueFromReg(hKey4, "NoSave", &SuppressSave);

			ReadIntValueFromReg(hKey4, "HistNum", &FileHist);
//			ReadMultiStringFromReg(hKey4, "Hist", Hist, (FMAX_PATH+1)*HISTORY_MAX+1);

			/* ヒストリの設定を読み込む */
			Sets = 0;
			ReadIntValueFromReg(hKey4, "SavedHist", &Sets);

			for(i = 0; i < Sets; i++)
			{
				sprintf(Str, "History%d", i);
				if(OpenSubKey(hKey4, Str, &hKey5) == FFFTP_SUCCESS)
				{
					CopyDefaultHistory(&Hist);

					ReadStringFromReg(hKey5, "HostAdrs", Hist.HostAdrs, HOST_ADRS_LEN+1);
					ReadStringFromReg(hKey5, "UserName", Hist.UserName, USER_NAME_LEN+1);
					ReadStringFromReg(hKey5, "Account", Hist.Account, ACCOUNT_LEN+1);
					ReadStringFromReg(hKey5, "LocalDir", Hist.LocalInitDir, INIT_DIR_LEN+1);
					ReadStringFromReg(hKey5, "RemoteDir", Hist.RemoteInitDir, INIT_DIR_LEN+1);
					ReadStringFromReg(hKey5, "Chmod", Hist.ChmodCmd, CHMOD_CMD_LEN+1);
					ReadStringFromReg(hKey5, "Nlst", Hist.LsName, NLST_NAME_LEN+1);
					ReadStringFromReg(hKey5, "Init", Hist.InitCmd, INITCMD_LEN+1);
					ReadIntValueFromReg(hKey5, "Port", &Hist.Port);
					ReadIntValueFromReg(hKey5, "Kanji", &Hist.KanjiCode);
					ReadIntValueFromReg(hKey5, "KanaCnv", &Hist.KanaCnv);
					ReadIntValueFromReg(hKey5, "NameKanji", &Hist.NameKanjiCode);
					ReadIntValueFromReg(hKey5, "NameKana", &Hist.NameKanaCnv);
					ReadIntValueFromReg(hKey5, "Pasv", &Hist.Pasv);
					ReadIntValueFromReg(hKey5, "Fwall", &Hist.FireWall);
					ReadIntValueFromReg(hKey5, "List", &Hist.ListCmdOnly);
					ReadIntValueFromReg(hKey5, "NLST-R", &Hist.UseNLST_R);
					ReadIntValueFromReg(hKey5, "Tzone", &Hist.TimeZone);
					ReadIntValueFromReg(hKey5, "Type", &Hist.HostType);
					ReadIntValueFromReg(hKey5, "Sync", &Hist.SyncMove);
					ReadIntValueFromReg(hKey5, "Fpath", &Hist.NoFullPath);
					ReadBinaryFromReg(hKey5, "Sort", &Hist.Sort, sizeof(Hist.Sort));
					ReadIntValueFromReg(hKey5, "Secu", &Hist.Security);
					ReadIntValueFromReg(hKey5, "TrType", &Hist.Type);
					strcpy(Str, "");
					ReadStringFromReg(hKey5, "Password", Str, 255);
					DecodePassword(Str, Hist.PassWord);
					ReadIntValueFromReg(hKey5, "Dial", &Hist.Dialup);
					ReadIntValueFromReg(hKey5, "UseIt", &Hist.DialupAlways);
					ReadIntValueFromReg(hKey5, "Notify", &Hist.DialupNotify);
					ReadStringFromReg(hKey5, "DialTo", Hist.DialEntry, RAS_NAME_LEN+1);
					// 暗号化通信対応
					ReadIntValueFromReg(hKey5, "NoEncryption", &Hist.UseNoEncryption);
					ReadIntValueFromReg(hKey5, "FTPES", &Hist.UseFTPES);
					ReadIntValueFromReg(hKey5, "FTPIS", &Hist.UseFTPIS);
					ReadIntValueFromReg(hKey5, "SFTP", &Hist.UseSFTP);
					strcpy(Str, "");
					ReadStringFromReg(hKey5, "PKey", Str, PRIVATE_KEY_LEN*4+1);
					DecodePassword(Str, Hist.PrivateKey);
					// 同時接続対応
					ReadIntValueFromReg(hKey5, "ThreadCount", &Hist.MaxThreadCount);
					ReadIntValueFromReg(hKey5, "ReuseCmdSkt", &Hist.ReuseCmdSkt);
					// MLSD対応
					ReadIntValueFromReg(hKey5, "MLSD", &Hist.UseMLSD);
					// IPv6対応
					ReadIntValueFromReg(hKey5, "NetType", &Hist.NetType);
					// 自動切断対策
					ReadIntValueFromReg(hKey5, "Noop", &Hist.NoopInterval);
					// 再転送対応
					ReadIntValueFromReg(hKey5, "ErrMode", &Hist.TransferErrorMode);
					ReadIntValueFromReg(hKey5, "ErrNotify", &Hist.TransferErrorNotify);

					CloseSubKey(hKey5);
					AddHistoryToHistory(&Hist);
				}
			}

			/* ホストの設定を読み込む */
			Sets = 0;
			ReadIntValueFromReg(hKey4, "SetNum", &Sets);

			for(i = 0; i < Sets; i++)
			{
				sprintf(Str, "Host%d", i);
				if(OpenSubKey(hKey4, Str, &hKey5) == FFFTP_SUCCESS)
				{
					CopyDefaultHost(&Host);
					/* 下位互換性のため */
					// SourceForge.JPによるフォーク
//					if(Version < VER_NUM)
					if(Version < 1921)
					{
						Host.Pasv = NO;
						Host.ListCmdOnly = NO;
					}
					// 1.97b以前はデフォルトでShift_JIS
					if(Version < 1980)
						Host.NameKanjiCode = KANJI_SJIS;
					ReadIntValueFromReg(hKey5, "Set", &Host.Level);

					ReadStringFromReg(hKey5, "HostName", Host.HostName, HOST_NAME_LEN+1);
					ReadStringFromReg(hKey5, "HostAdrs", Host.HostAdrs, HOST_ADRS_LEN+1);
					ReadStringFromReg(hKey5, "UserName", Host.UserName, USER_NAME_LEN+1);
					ReadStringFromReg(hKey5, "Account", Host.Account, ACCOUNT_LEN+1);
					ReadStringFromReg(hKey5, "LocalDir", Host.LocalInitDir, INIT_DIR_LEN+1);
					ReadStringFromReg(hKey5, "RemoteDir", Host.RemoteInitDir, INIT_DIR_LEN+1);
					ReadStringFromReg(hKey5, "Chmod", Host.ChmodCmd, CHMOD_CMD_LEN+1);
					ReadStringFromReg(hKey5, "Nlst", Host.LsName, NLST_NAME_LEN+1);
					ReadStringFromReg(hKey5, "Init", Host.InitCmd, INITCMD_LEN+1);
					ReadIntValueFromReg(hKey5, "Port", &Host.Port);
					ReadIntValueFromReg(hKey5, "Anonymous", &Host.Anonymous);
					ReadIntValueFromReg(hKey5, "Kanji", &Host.KanjiCode);
					// 1.98b以前のUTF-8はBOMあり
					if(Version < 1983)
					{
						if(Host.KanjiCode == KANJI_UTF8N)
							Host.KanjiCode = KANJI_UTF8BOM;
					}
					ReadIntValueFromReg(hKey5, "KanaCnv", &Host.KanaCnv);
					ReadIntValueFromReg(hKey5, "NameKanji", &Host.NameKanjiCode);
					ReadIntValueFromReg(hKey5, "NameKana", &Host.NameKanaCnv);
					ReadIntValueFromReg(hKey5, "Pasv", &Host.Pasv);
					ReadIntValueFromReg(hKey5, "Fwall", &Host.FireWall);
					ReadIntValueFromReg(hKey5, "List", &Host.ListCmdOnly);
					ReadIntValueFromReg(hKey5, "NLST-R", &Host.UseNLST_R);
					ReadIntValueFromReg(hKey5, "Last", &Host.LastDir);
					ReadIntValueFromReg(hKey5, "Tzone", &Host.TimeZone);
					ReadIntValueFromReg(hKey5, "Type", &Host.HostType);
					ReadIntValueFromReg(hKey5, "Sync", &Host.SyncMove);
					ReadIntValueFromReg(hKey5, "Fpath", &Host.NoFullPath);
					ReadBinaryFromReg(hKey5, "Sort", &Host.Sort, sizeof(Host.Sort));
					ReadIntValueFromReg(hKey5, "Secu", &Host.Security);
					if(Host.Anonymous != YES)
					{
						strcpy(Str, "");
						ReadStringFromReg(hKey5, "Password", Str, 255);
						DecodePassword(Str, Host.PassWord);
					}
					else
						strcpy(Host.PassWord, UserMailAdrs);

					ReadMultiStringFromReg(hKey5, "Bmarks", Host.BookMark, BOOKMARK_SIZE);

					ReadIntValueFromReg(hKey5, "Dial", &Host.Dialup);
					ReadIntValueFromReg(hKey5, "UseIt", &Host.DialupAlways);
					ReadIntValueFromReg(hKey5, "Notify", &Host.DialupNotify);
					ReadStringFromReg(hKey5, "DialTo", Host.DialEntry, RAS_NAME_LEN+1);
					// 暗号化通信対応
					ReadIntValueFromReg(hKey5, "NoEncryption", &Host.UseNoEncryption);
					ReadIntValueFromReg(hKey5, "FTPES", &Host.UseFTPES);
					ReadIntValueFromReg(hKey5, "FTPIS", &Host.UseFTPIS);
					ReadIntValueFromReg(hKey5, "SFTP", &Host.UseSFTP);
					strcpy(Str, "");
					ReadStringFromReg(hKey5, "PKey", Str, PRIVATE_KEY_LEN*4+1);
					DecodePassword(Str, Host.PrivateKey);
					// 同時接続対応
					ReadIntValueFromReg(hKey5, "ThreadCount", &Host.MaxThreadCount);
					ReadIntValueFromReg(hKey5, "ReuseCmdSkt", &Host.ReuseCmdSkt);
					// 1.98d以前で同時接続数が1より大きい場合はソケットの再利用なし
					if(Version < 1985)
					{
						if(Host.MaxThreadCount > 1)
							Host.ReuseCmdSkt = NO;
					}
					// MLSD対応
					ReadIntValueFromReg(hKey5, "MLSD", &Host.UseMLSD);
					// IPv6対応
					ReadIntValueFromReg(hKey5, "NetType", &Host.NetType);
					// 自動切断対策
					ReadIntValueFromReg(hKey5, "Noop", &Host.NoopInterval);
					// 再転送対応
					ReadIntValueFromReg(hKey5, "ErrMode", &Host.TransferErrorMode);
					ReadIntValueFromReg(hKey5, "ErrNotify", &Host.TransferErrorNotify);

					CloseSubKey(hKey5);

					AddHostToList(&Host, -1, Host.Level);
				}
			}

			ReadIntValueFromReg(hKey4, "CurSet", &Sets);
			SetCurrentHost(Sets);

			// 暗号化通信対応
			ReadBinaryFromReg(hKey4, "CertCacheHash", &CertificateCacheHash, sizeof(CertificateCacheHash));
			ReadStringFromReg(hKey4, "RootCertHash", Str, PRIVATE_KEY_LEN*4+1);
			DecodePassword(Str, Buf);
			StrReadIn(Buf, sizeof(SSLRootCAFileHash), (char*)&SSLRootCAFileHash);
			// ファイルアイコン表示対応
			ReadIntValueFromReg(hKey4, "ListIcon", &DispFileIcon);
			// ディレクトリ自動作成
			ReadIntValueFromReg(hKey4, "MakeDir", &MakeAllDir);

			CloseSubKey(hKey4);
		}
		CloseReg(hKey3);
	}
	else
	{
		/*===== 最初の起動時（設定が無い) =====*/

#if 0
		strcpy(UserMailAdrs, "");
		strcpy(Str, "");
		if(InputDialogBox(mailadrs_dlg, HWND_DESKTOP, NULL, Str, USER_MAIL_LEN+1, &i, IDH_HELP_TOPIC_0000001) == YES)
			strcpy(UserMailAdrs, Str);

		for(i = 0; i < SAMPLE_HOSTS; i++)
		{
			CopyDefaultHost(&Host);
			Host.Level = Sample[i].Level;
			strcpy(Host.PassWord, UserMailAdrs);
			strcpy(Host.HostName, Sample[i].HostName);
			strcpy(Host.HostAdrs, Sample[i].HostAdrs);
			strcpy(Host.UserName, "anonymous");
			AddHostToList(&Host, -1, Host.Level);
		}
#endif
	}
	return(Sts);
}


/*----- 隠しドライブ情報を取得 ------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		DWORD 
*			YES/NO=設定無し
*----------------------------------------------------------------------------*/

DWORD LoadHideDriveListRegistry(void)
{
	HKEY hKey1;
	HKEY hKey2;
	HKEY hKey3;
	HKEY hKey4;
	HKEY hKey5;
	HKEY hKey6;
	DWORD Size;
	DWORD Type;
	DWORD Ret;

	Ret = 0;
	if(RegOpenKeyEx(HKEY_CURRENT_USER, "Software", 0, KEY_READ, &hKey1) == ERROR_SUCCESS)
	{
		if(RegOpenKeyEx(hKey1, "Microsoft", 0, KEY_READ, &hKey2) == ERROR_SUCCESS)
		{
			if(RegOpenKeyEx(hKey2, "Windows", 0, KEY_READ, &hKey3) == ERROR_SUCCESS)
			{
				if(RegOpenKeyEx(hKey3, "CurrentVersion", 0, KEY_READ, &hKey4) == ERROR_SUCCESS)
				{
					if(RegOpenKeyEx(hKey4, "Policies", 0, KEY_READ, &hKey5) == ERROR_SUCCESS)
					{
						if(RegOpenKeyEx(hKey5, "Explorer", 0, KEY_READ, &hKey6) == ERROR_SUCCESS)
						{
							Size = sizeof(DWORD);
							RegQueryValueEx(hKey6, "NoDrives", NULL, &Type, (BYTE *)&Ret, &Size);
							RegCloseKey(hKey6);
						}
						RegCloseKey(hKey5);
					}
					RegCloseKey(hKey4);
				}
				RegCloseKey(hKey3);
			}
			RegCloseKey(hKey2);
		}
		RegCloseKey(hKey1);
	}
	return(Ret);
}


/*----- レジストリの設定値をクリア --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ClearRegistry(void)
{
	HKEY hKey2;
	HKEY hKey3;
	HKEY hKey4;
	DWORD Dispos;
	char Str[20];
	int i;

	if(RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Sota", 0, "", REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY, NULL, &hKey2, &Dispos) == ERROR_SUCCESS)
	{
		if(RegCreateKeyEx(hKey2, "FFFTP", 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey3, &Dispos) == ERROR_SUCCESS)
		{
			if(RegCreateKeyEx(hKey3, "Options", 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey4, &Dispos) == ERROR_SUCCESS)
			{
				for(i = 0; ; i++)
				{
					sprintf(Str, "Host%d", i);
					if(RegDeleteKey(hKey4, Str) != ERROR_SUCCESS)
						break;
				}
				for(i = 0; ; i++)
				{
					sprintf(Str, "History%d", i);
					if(RegDeleteKey(hKey4, Str) != ERROR_SUCCESS)
						break;
				}
				RegCloseKey(hKey4);
			}
			RegDeleteKey(hKey3, "Options");
			RegCloseKey(hKey3);
		}
		RegDeleteKey(hKey2, "FFFTP");
		RegCloseKey(hKey2);
	}
	return;
}


// ポータブル版判定
void ClearIni(void)
{
//	FILE *Strm;
//	if((Strm = fopen(AskIniFilePath(), "rt")) != NULL)
//	{
//		fclose(Strm);
//		MoveFileToTrashCan(AskIniFilePath());
//	}
	DeleteFile(AskIniFilePath());
	return;
}


/*----- 設定をファイルに保存 --------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SaveSettingsToFile(void)
{
	char Tmp[FMAX_PATH*2];
	char Fname[FMAX_PATH+1];
	// 任意のコードが実行されるバグ修正
	char CurDir[FMAX_PATH+1];
	char SysDir[FMAX_PATH+1];

	if(RegType == REGTYPE_REG)
	{
		strcpy(Fname, "FFFTP.reg");
		if(SelectFile(GetMainHwnd(), Fname, MSGJPN286, MSGJPN287, "reg", OFN_EXTENSIONDIFFERENT | OFN_OVERWRITEPROMPT, 1) == TRUE)
		{
			sprintf(Tmp, "/e \x22%s\x22 HKEY_CURRENT_USER\\Software\\sota\\FFFTP", Fname);
			// 任意のコードが実行されるバグ修正
//			if(ShellExecute(NULL, "open", "regedit", Tmp, ".", SW_SHOW) <= (HINSTANCE)32)
//			{
//				MessageBox(NULL, MSGJPN285, "FFFTP", MB_OK);
//			}
			if(GetCurrentDirectory(FMAX_PATH, CurDir) > 0)
			{
				if(GetSystemDirectory(SysDir, FMAX_PATH) > 0)
				{
					if(SetCurrentDirectory(SysDir))
					{
						if(ShellExecute(NULL, "open", "regedit", Tmp, NULL, SW_SHOW) <= (HINSTANCE)32)
						{
							MessageBox(NULL, MSGJPN285, "FFFTP", MB_OK);
						}
						SetCurrentDirectory(CurDir);
					}
				}
			}
		}
	}
	else
	{
		strcpy(Fname, "FFFTP-Backup.ini");
		if(SelectFile(GetMainHwnd(), Fname, MSGJPN286, MSGJPN288, "ini", OFN_EXTENSIONDIFFERENT | OFN_OVERWRITEPROMPT, 1) == TRUE)
		{
			CopyFile(AskIniFilePath(), Fname, FALSE);
		}
	}
	return;
}


/*----- 設定をファイルから復元 ------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int	ロードしたかどうか (YES/NO)
*----------------------------------------------------------------------------*/

int LoadSettingsFromFile(void)
{
	int Ret;
	char Tmp[FMAX_PATH*2];
	char Fname[FMAX_PATH+1];
	// 任意のコードが実行されるバグ修正
	char CurDir[FMAX_PATH+1];
	char SysDir[FMAX_PATH+1];

	Ret = NO;
	strcpy(Fname, "");
	if(SelectFile(GetMainHwnd(), Fname, MSGJPN291, MSGJPN290, "", OFN_FILEMUSTEXIST, 0) == TRUE)
	{
		if((strlen(Fname) >= 5) && (_stricmp(&Fname[strlen(Fname)-4], ".reg") == 0))
		{
			sprintf(Tmp, "\x22%s\x22", Fname);
			// 任意のコードが実行されるバグ修正
//			if(ShellExecute(NULL, "open", "regedit", Tmp, ".", SW_SHOW) <= (HINSTANCE)32)
//			{
//				MessageBox(NULL, MSGJPN285, "FFFTP", MB_OK);
//			}
//			else
//			{
//				Ret = YES;
//				/* レジストリエディタが終了するのを待つ */
//				WaitForSingleObject(Info.hProcess, INFINITE);
//			}
			if(GetCurrentDirectory(FMAX_PATH, CurDir) > 0)
			{
				if(GetSystemDirectory(SysDir, FMAX_PATH) > 0)
				{
					if(SetCurrentDirectory(SysDir))
					{
						if(ShellExecute(NULL, "open", "regedit", Tmp, NULL, SW_SHOW) <= (HINSTANCE)32)
						{
							MessageBox(NULL, MSGJPN285, "FFFTP", MB_OK);
						}
						else
						{
							Ret = YES;
							/* レジストリエディタが終了するのを待つ */
//							WaitForSingleObject(Info.hProcess, INFINITE);
						}
						SetCurrentDirectory(CurDir);
					}
				}
			}
		}
		else if((strlen(Fname) >= 5) && (_stricmp(&Fname[strlen(Fname)-4], ".ini") == 0))
		{
			CopyFile(Fname, AskIniFilePath(), FALSE);
			Ret = YES;
		}
		else
			MessageBox(NULL, MSGJPN293, "FFFTP", MB_OK);
	}
	return(Ret);
}




/*----- レジストリ/INIファイルに文字列をセーブ --------------------------------
*
*	Parameter
*		HKEY hKey : レジストリキー
*		char *Key : キー名
*		char *Str : セーブする文字列
*		char *DefaultStr : デフォルトの文字列
*
*	Return Value
*		なし
*
*	Note
*		文字列がデフォルトの文字列と同じならセーブしない
*----------------------------------------------------------------------------*/

static void SaveStr(HKEY hKey, char *Key, char *Str, char *DefaultStr)
{
	if((DefaultStr != NULL) && (strcmp(Str, DefaultStr) == 0))
		DeleteValue(hKey, Key);
	else
		WriteStringToReg(hKey, Key, Str);

	return;
}


/*----- レジストリ/INIファイルに数値(INT)をセーブ -----------------------------
*
*	Parameter
*		HKEY hKey : レジストリキー
*		char *Key : キー名
*		int Num : セーブする値
*		int DefaultNum : デフォルトの値
*
*	Return Value
*		なし
*
*	Note
*		数値がデフォルトの値と同じならセーブしない
*----------------------------------------------------------------------------*/

static void SaveIntNum(HKEY hKey, char *Key, int Num, int DefaultNum)
{
	if(Num == DefaultNum)
		DeleteValue(hKey, Key);
	else
		WriteIntValueToReg(hKey, Key, Num);

	return;
}


/*----- LOGFONTデータを文字列に変換する ---------------------------------------
*
*	Parameter
*		LOGFONT Font : フォントデータ
*		HFONT hFont : フォントのハンドル
*			NULL = デフォルトのフォント
*		char *Buf : バッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void MakeFontData(LOGFONT Font, HFONT hFont, char *Buf)
{
	*Buf = NUL;
	if(hFont != NULL)
		sprintf(Buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %s",
			Font.lfHeight, Font.lfWidth, Font.lfEscapement, Font.lfOrientation,
			Font.lfWeight, Font.lfItalic, Font.lfUnderline, Font.lfStrikeOut,
			Font.lfCharSet, Font.lfOutPrecision, Font.lfClipPrecision,
			Font.lfQuality, Font.lfPitchAndFamily, Font.lfFaceName);
	return;
}


/*----- 文字列をLOGFONTデータに変換する ---------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		LOGFONT *Font : フォントデータ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL=変換できない
*----------------------------------------------------------------------------*/

static int RestoreFontData(char *Str, LOGFONT *Font)
{
	int i;
	int Sts;

	Sts = FFFTP_FAIL;
	if(sscanf(Str, "%d %d %d %d %d %d %d %d %d %d %d %d %d",
			&(Font->lfHeight), &(Font->lfWidth), &(Font->lfEscapement), &(Font->lfOrientation),
			&(Font->lfWeight), &(Font->lfItalic), &(Font->lfUnderline), &(Font->lfStrikeOut),
			&(Font->lfCharSet), &(Font->lfOutPrecision), &(Font->lfClipPrecision),
			&(Font->lfQuality), &(Font->lfPitchAndFamily)) == 13)
	{
		for(i = 13; i > 0; i--)
		{
			if((Str = strchr(Str, ' ')) == NULL)
				break;
			Str++;
		}
		if(i == 0)
		{
			strcpy(Font->lfFaceName, Str);
			Sts = FFFTP_SUCCESS;
		}
	}

	if(Sts == FFFTP_FAIL)
		memset(Font, NUL, sizeof(LOGFONT));

	return(Sts);
}

/*----- パスワードを暗号化する ------------------------------------------------
*
*	Parameter
*		char *Str : パスワード
*		char *Buf : 暗号化したパスワードを格納するバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
static void EncodePassword(char *Str, char *Buf)
{
	EncodePassword3( Str, Buf, SecretKey );
}

/*----- パスワードを暗号化する(オリジナルアルゴリズム)  ------------------
*
*	Parameter
*		char *Str : パスワード
*		char *Buf : 暗号化したパスワードを格納するバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void EncodePasswordOriginal(char *Str, char *Buf)
{
	unsigned char *Get;
	unsigned char *Put;
	int Rnd;
	int Ch;

	srand((unsigned)time(NULL));

	Get = (unsigned char *)Str;
	Put = (unsigned char *)Buf;
	
	if( *Get == NUL ){
		*Put = NUL;
		return;
	}

	/* 識別子を先頭に置く */
	Put[0] = '0';
	Put[1] = 'A';
	Put += 2;

	while(*Get != NUL)
	{
		Rnd = rand() % 3;
		Ch = ((int)*Get++) << Rnd;
		Ch = (unsigned char)Ch | (unsigned char)(Ch >> 8);
		*Put++ = 0x40 | ((Rnd & 0x3) << 4) | (Ch & 0xF);
		*Put++ = 0x40 | ((Ch >> 4) & 0xF);
		if((*(Put-2) & 0x1) != 0)
			*Put++ = (rand() % 62) + 0x40;
	}
	*Put = NUL;
	return;
}

/*----- パスワードを暗号化する(オリジナルアルゴリズム＾Key)  ----------------
*
*	Parameter
*		char *Str : パスワード
*		char *Buf : 暗号化したパスワードを格納するバッファ
*		const char *Key : 暗号化キー
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void EncodePassword2(char *Str, char *Buf, const char* Key)
{
	unsigned char *Get;
	unsigned char *Put;
	int Rnd;
	int Ch;

	/* 2010.01.31 genta Key */
	unsigned char *KeyHead = (unsigned char *)Key;
	unsigned char *KeyEnd = KeyHead + strlen(KeyHead);
	unsigned char *KeyCurrent = KeyHead;

	srand((unsigned)time(NULL));

	Get = (unsigned char *)Str;
	Put = (unsigned char *)Buf;
	
	if( *Get == NUL ){
		*Put = NUL;
		return;
	}

	/* 識別子を先頭に置く */
	Put[0] = '0';
	Put[1] = 'B';
	Put += 2;

	while(*Get != NUL)
	{
		Rnd = rand() % 3;
		Ch = ((int)(*Get++ ^ *KeyCurrent)) << Rnd;
		Ch = (unsigned char)Ch | (unsigned char)(Ch >> 8);
		*Put++ = 0x40 | ((Rnd & 0x3) << 4) | (Ch & 0xF);
		*Put++ = 0x40 | ((Ch >> 4) & 0xF);
		if((*(Put-2) & 0x1) != 0)
			*Put++ = (rand() % 62) + 0x40;

		/* 2010.01.31 genta Key */
		if( ++KeyCurrent == KeyEnd ){
			KeyCurrent = KeyHead;
		}
	}
	*Put = NUL;
	return;
}

/*----- パスワードを暗号化する(AES) ------------------------------------------
*
*	Parameter
*		char *Str : パスワード
*		char *Buf : 暗号化したパスワードを格納するバッファ
*		const char *Key : 暗号化キー
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void EncodePassword3(char *Str, char *Buf, const char *Key)
{
	unsigned char *Put;
	unsigned char *AesEncBuf;
	unsigned char *StrPadBuf;
	size_t StrLen;
	size_t StrPadLen;
	size_t StrPadIndex;
	size_t IvIndex;
	size_t EncBufIndex;
	DWORD RandValue;
	int RandByteCount;
	unsigned char AesKey[32];
	unsigned char AesCbcIv[AES_BLOCK_SIZE];
	aes_encrypt_ctx Ctx;

	Buf[0] = NUL;

	Put = (unsigned char *)Buf;
	StrLen = strlen(Str);
	StrPadLen = ((StrLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;

	/* 最低長を32文字とする */
//	StrPadLen = min1(StrPadLen, AES_BLOCK_SIZE * 2);
	StrPadLen = max1(StrPadLen, AES_BLOCK_SIZE * 2);

	if((StrPadBuf = malloc(StrPadLen)) != NULL)
	{
		if((AesEncBuf = malloc(StrPadLen)) != NULL)
		{
			BOOL PutState;

			PutState = FALSE;
			strncpy(StrPadBuf, Str, StrPadLen);
			
			/* PAD部分を乱数で埋める StrPad[StrLen](が有効な場合) は NUL */
			for(StrPadIndex = StrLen + 1; StrPadIndex < StrPadLen;)
			{
				RandValue = GetRandamDWRODValue();
				for(RandByteCount = 0; RandByteCount < 4; RandByteCount++)
				{
					if(StrPadIndex < StrPadLen)
					{
						StrPadBuf[StrPadIndex++] = (unsigned char)(RandValue & 0xff);
						RandValue >>= 8;
					}
				}
			}
			
			// IVの初期化
			for(IvIndex = 0; IvIndex < AES_BLOCK_SIZE;)
			{
				RandValue = GetRandamDWRODValue();
				for(RandByteCount = 0; RandByteCount < 4; RandByteCount++)
				{
					if(IvIndex < AES_BLOCK_SIZE)
					{
						AesCbcIv[IvIndex++] = (unsigned char)(RandValue & 0xff);
						RandValue >>= 8;
					}
				}
			}
			Put[0] = '0';
			Put[1] = 'C';
			Put += 2;
			for(IvIndex = 0; IvIndex < AES_BLOCK_SIZE; IvIndex++)
			{
				sprintf(Put, "%02x", AesCbcIv[IvIndex]);
				Put += 2;
			}
			*Put++ = ':';

			if(CreateAesKey(AesKey, Key) == FFFTP_SUCCESS)
			{
				aes_encrypt_key(AesKey, 32, &Ctx);

				if(aes_cbc_encrypt(StrPadBuf, AesEncBuf, StrPadLen, AesCbcIv, &Ctx) == EXIT_SUCCESS)
				{
					for(EncBufIndex = 0; EncBufIndex < StrPadLen; EncBufIndex++)
					{
						sprintf(Put, "%02x", AesEncBuf[EncBufIndex]);
						Put += 2;
					}
					*Put = NUL;
					PutState = TRUE;
				}
			}
			if(FALSE == PutState)
			{
				Buf[0] = NUL;
			}
			free(StrPadBuf);
		}
		free(AesEncBuf);
	}
	return;
}


/*----- パスワードの暗号化を解く ----------------------------------------------
*
*	Parameter
*		char *Str : 暗号化したパスワード
*		char *Buf : パスワードを格納するバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DecodePassword(char *Str, char *Buf)
{
	unsigned char *Get;
	unsigned char *Put;

	Get = (unsigned char *)Str;
	Put = (unsigned char *)Buf;
	
	if( *Get == NUL ){
		*Put = NUL;
	}
	else if( 0x40 <= *Get && *Get < 0x80 ){
		/* Original algorithm */
		DecodePasswordOriginal( Str, Buf );
	}
	else if( strncmp( Get, "0A", 2 ) == 0 ){
		DecodePasswordOriginal( Str + 2, Buf );
	}
	else if( strncmp( Get, "0B", 2 ) == 0 ){
		DecodePassword2( Str + 2, Buf, SecretKey );
	}
	else if( strncmp( Get, "0C", 2 ) == 0 ){
		DecodePassword3( Str + 2, Buf, SecretKey );
	}
	else {
		//	unknown encoding
		*Put = NUL;
		return;
	}
}

/*----- パスワードの暗号化を解く(オリジナルアルゴリズム) -------------------
*
*	Parameter
*		char *Str : 暗号化したパスワード
*		char *Buf : パスワードを格納するバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
static void DecodePasswordOriginal(char *Str, char *Buf)
{
	unsigned char *Get;
	unsigned char *Put;
	int Rnd;
	int Ch;

	Get = (unsigned char *)Str;
	Put = (unsigned char *)Buf;

	while(*Get != NUL)
	{
		Rnd = ((unsigned int)*Get >> 4) & 0x3;
		Ch = (*Get & 0xF) | ((*(Get+1) & 0xF) << 4);
		Ch <<= 8;
		if((*Get & 0x1) != 0)
			Get++;
		Get += 2;
		Ch >>= Rnd;
		Ch = (Ch & 0xFF) | ((Ch >> 8) & 0xFF);
		*Put++ = Ch;
	}
	*Put = NUL;
	return;
}

/*----- パスワードの暗号化を解く(オリジナルアルゴリズム＾Key) -------------------
*
*	Parameter
*		char *Str : 暗号化したパスワード
*		char *Buf : パスワードを格納するバッファ
*		const char *Key : 暗号化キー
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
static void DecodePassword2(char *Str, char *Buf, const char* Key)
{
	int Rnd;
	int Ch;
	unsigned char *Get = (unsigned char *)Str;
	unsigned char *Put = (unsigned char *)Buf;

	/* 2010.01.31 genta Key */
	unsigned char *KeyHead = (unsigned char *)Key;
	unsigned char *KeyEnd = KeyHead + strlen(KeyHead);
	unsigned char *KeyCurrent = KeyHead;

	while(*Get != NUL)
	{
		Rnd = ((unsigned int)*Get >> 4) & 0x3;
		Ch = (*Get & 0xF) | ((*(Get+1) & 0xF) << 4);
		Ch <<= 8;
		if((*Get & 0x1) != 0)
			Get++;
		Get += 2;
		Ch >>= Rnd;
		Ch = (Ch & 0xFF) | ((Ch >> 8) & 0xFF);
		*Put++ = Ch ^ *KeyCurrent;
		
		/* 2010.01.31 genta Key */
		if( ++KeyCurrent == KeyEnd ){
			KeyCurrent = KeyHead;
		}
	}
	*Put = NUL;
	return;
}

/*----- パスワードの暗号化を解く(AES) ---------------------------------------
*
*	Parameter
*		char *Str : 暗号化したパスワード
*		char *Buf : パスワードを格納するバッファ
*		const char *Key : 暗号化キー
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DecodePassword3(char *Str, char *Buf, const char *Key)
{
	char *Get;
	unsigned char *EncBuf;
	size_t StrLen;
	size_t IvIndex;
	size_t EncBufIndex;
	size_t EncBufLen;
	unsigned char AesKey[32];
	unsigned char AesCbcIv[AES_BLOCK_SIZE];
	aes_decrypt_ctx Ctx;

	Buf[0] = NUL;

	Get = Str;
	StrLen = strlen(Str);

	if(AES_BLOCK_SIZE * 2 + 1 < StrLen)
	{

		EncBufLen = (StrLen - 1 ) / 2 - AES_BLOCK_SIZE;
		if((EncBuf = malloc(EncBufLen)) != NULL)
		{
			for(IvIndex = 0; IvIndex < AES_BLOCK_SIZE; IvIndex++)
			{
				AesCbcIv[IvIndex]  = hex2bin(*Get++) << 4;
				AesCbcIv[IvIndex] |= hex2bin(*Get++);
			}

			if(*Get++ == ':')
			{
				if(CreateAesKey(AesKey, Key) == FFFTP_SUCCESS)
				{
					aes_decrypt_key(AesKey, 32, &Ctx);

					for(EncBufIndex = 0; EncBufIndex < EncBufLen; EncBufIndex++)
					{
						EncBuf[EncBufIndex]  = hex2bin(*Get++) << 4;
						EncBuf[EncBufIndex] |= hex2bin(*Get++);
					}
					if(aes_cbc_decrypt(EncBuf, Buf, EncBufLen, AesCbcIv, &Ctx) == EXIT_SUCCESS)
					{
						Buf[EncBufLen] = NUL;
					}
				}
			}
			free(EncBuf);
		}
	}
	return;
}

/*----- AES用固定長キーを作成 ----------------------------------------------
*
*	Parameter
*		unsigned char *AesKey : AES暗号鍵
*		const char *Key : 暗号化キー
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*	Note
*		SHA-1をもちいて32Byte鍵を生成する
*----------------------------------------------------------------------------*/

static int CreateAesKey(unsigned char *AesKey, const char* Key)
{
	char* HashKey;
	uint32 HashKeyLen;
	uint32 results[10];
	int ByteOffset;
	int KeyIndex;
	int ResIndex;

	HashKeyLen = strlen(Key) + 16;
	if((HashKey = malloc(HashKeyLen + 1)) == NULL){
		return (FFFTP_FAIL);
	}

	strcpy(HashKey, Key);
	strcat(HashKey, ">g^r=@N7=//z<[`:");
    sha_memory((uchar *)HashKey, HashKeyLen, results);

	strcpy(HashKey, Key);
	strcat(HashKey, "VG77dO1#EyC]$|C@");
    sha_memory((uchar *)HashKey, HashKeyLen, results + 5);

	KeyIndex = 0;
	ResIndex = 0;
	while(ResIndex < 8){
		for(ByteOffset = 0; ByteOffset < 4; ByteOffset++){
			AesKey[KeyIndex++] = (results[ResIndex] >> ByteOffset * 8) & 0xff;
		}
		ResIndex++;
	}
	free(HashKey);

	return (FFFTP_SUCCESS);
}


/*===== レジストリとINIファイルのアクセス処理 ============*/


/*===== INIファイル用のレジストリデータ =====*/

typedef struct regdatatbl {
	char KeyName[80+1];			/* キー名 */
	char ValTbl[REG_SECT_MAX];	/* 値のテーブル */
	int ValLen;					/* 値データのバイト数 */
	int Mode;					/* キーのモード */
	struct regdatatbl *Next;
} REGDATATBL;

/*===== プロトタイプ =====*/

static BOOL WriteOutRegToFile(REGDATATBL *Pos);
static int ReadInReg(char *Name, REGDATATBL **Handle);
// 暗号化通信対応
//static int StrCatOut(char *Src, int Len, char *Dst);
//static int StrReadIn(char *Src, int Max, char *Dst);
static char *ScanValue(void *Handle, char *Name);


/*===== ローカルなワーク =====*/

static int TmpRegType;



/*----- レジストリのタイプを設定する ------------------------------------------
*
*	Parameter
*		int Type : タイプ (REGTYPE_xxx)
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static void SetRegType(int Type)
{
	TmpRegType = Type;
	return;
}


/*----- レジストリ/INIファイルをオープンする（読み込み）-----------------------
*
*	Parameter
*		char *Name : レジストリ名
*		void **Handle : ハンドルを返すワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int OpenReg(char *Name, void **Handle)
{
	int Sts;
	char Tmp[FMAX_PATH+1];

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		strcpy(Tmp, "Software\\Sota\\");
		strcat(Tmp, Name);
		if(RegOpenKeyEx(HKEY_CURRENT_USER, Tmp, 0, KEY_READ, (HKEY *)Handle) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		if((Sts = ReadInReg(Name, (REGDATATBL **)Handle)) == FFFTP_SUCCESS)
			((REGDATATBL *)(*Handle))->Mode = 0;
	}
	return(Sts);
}


/*----- レジストリ/INIファイルを作成する（書き込み）---------------------------
*
*	Parameter
*		char *Name : レジストリ名
*		void **Handle : ハンドルを返すワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int CreateReg(char *Name, void **Handle)
{
	int Sts;
	char Tmp[FMAX_PATH+1];
	DWORD Dispos;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		strcpy(Tmp, "Software\\Sota\\");
		strcat(Tmp, Name);
		if(RegCreateKeyEx(HKEY_CURRENT_USER, Tmp, 0, "", REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, (HKEY *)Handle, &Dispos) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		if((*Handle = malloc(sizeof(REGDATATBL))) != NULL)
		{
			strcpy(((REGDATATBL *)(*Handle))->KeyName, Name);
			((REGDATATBL *)(*Handle))->ValLen = 0;
			((REGDATATBL *)(*Handle))->Next = NULL;
			((REGDATATBL *)(*Handle))->Mode = 1;
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- レジストリ/INIファイルをクローズする ----------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int CloseReg(void *Handle)
{
	REGDATATBL *Pos;
	REGDATATBL *Next;
	// ポータブル版判定
//	FILE *Strm;

	if(TmpRegType == REGTYPE_REG)
	{
		RegCloseKey(Handle);

		/* INIファイルを削除 */
		// ポータブル版判定
//		if((Strm = fopen(AskIniFilePath(), "rt")) != NULL)
//		{
//			fclose(Strm);
//			MoveFileToTrashCan(AskIniFilePath());
//		}
	}
	else
	{
		if(((REGDATATBL *)Handle)->Mode == 1)
		{
			if(WriteOutRegToFile(Handle) == TRUE)
			{
//				/* レジストリをクリア */
//				ClearRegistry();
			}
		}
		/* テーブルを削除 */
		Pos = Handle;
		while(Pos != NULL)
		{
			Next = Pos->Next;
			free(Pos);
			Pos = Next;
		}
	}
	return(FFFTP_SUCCESS);
}


/*----- レジストリ情報をINIファイルに書き込む ---------------------------------
*
*	Parameter
*		REGDATATBL *Pos : レジストリデータ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static BOOL WriteOutRegToFile(REGDATATBL *Pos)
{
	FILE *Strm;
	char *Disp;
	BOOL Ret;

	Ret = FALSE;
	if((Strm = fopen(AskIniFilePath(), "wt")) != NULL)
	{
		fprintf(Strm, MSGJPN239);
		while(Pos != NULL)
		{
			fprintf(Strm, "\n[%s]\n", Pos->KeyName);

			Disp = Pos->ValTbl;
			while(Disp < (Pos->ValTbl + Pos->ValLen))
			{
				fprintf(Strm, "%s\n", Disp);
				Disp = Disp + strlen(Disp) + 1;
			}
			Pos = Pos->Next;
		}
		fclose(Strm);
		Ret = TRUE;
	}
	else
		MessageBox(NULL, MSGJPN240, "FFFTP", MB_OK);

	return(Ret);
}


/*----- INIファイルからレジストリ情報を読み込む -------------------------------
*
*	Parameter
*		char *Name : 名前
*		void *Handle : ハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int ReadInReg(char *Name, REGDATATBL **Handle)
{
	FILE *Strm;
	char *Buf;
	char *Tmp;
	char *Data;
	REGDATATBL *New;
	REGDATATBL *Pos;
	int Sts;

	Sts = FFFTP_FAIL;
	*Handle = NULL;

	if((Strm = fopen(AskIniFilePath(), "rt")) != NULL)
	{
		if((Buf = malloc(REG_SECT_MAX)) != NULL)
		{
			while(fgets(Buf, REG_SECT_MAX, Strm) != NULL)
			{
				if(*Buf != '#')
				{
					if((Tmp = strchr(Buf, '\n')) != NULL)
						*Tmp = NUL;

					if(*Buf == '[')
					{
						if((New = malloc(sizeof(REGDATATBL))) != NULL)
						{
							if((Tmp = strchr(Buf, ']')) != NULL)
								*Tmp = NUL;
							strcpy(New->KeyName, Buf+1);
							New->ValLen = 0;
							New->Next = NULL;
							Data = New->ValTbl;
						}
						if(*Handle == NULL)
							*Handle = New;
						else
						{
							Pos = *Handle;
							while(Pos->Next != NULL)
								Pos = Pos->Next;
							Pos->Next = New;
						}
					}
					else if(strlen(Buf) > 0)
					{
						strcpy(Data, Buf);
						Data += strlen(Buf) + 1;
						New->ValLen += strlen(Buf) + 1;
					}
				}
			}
			Sts = FFFTP_SUCCESS;
			free(Buf);
		}
		fclose(Strm);
	}
	return(Sts);
}


/*----- サブキーをオープンする ------------------------------------------------
*
*	Parameter
*		void *Parent : 親のハンドル
*		char *Name : 名前
*		void **Handle : ハンドルを返すワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int OpenSubKey(void *Parent, char *Name, void **Handle)
{
	int Sts;
	char Key[80];
	REGDATATBL *Pos;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegOpenKeyEx(Parent, Name, 0, KEY_READ, (HKEY *)Handle) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		strcpy(Key, ((REGDATATBL *)Parent)->KeyName);
		strcat(Key, "\\");
		strcat(Key, Name);
		Pos = Parent;
		while(Pos != NULL)
		{
			if(strcmp(Pos->KeyName, Key) == 0)
			{
				*Handle = Pos;
				Sts = FFFTP_SUCCESS;
				break;
			}
			Pos = Pos->Next;
		}
	}
	return(Sts);
}


/*----- サブキーを作成する ----------------------------------------------------
*
*	Parameter
*		void *Parent : 親のハンドル
*		char *Name : 名前
*		void **Handle : ハンドルを返すワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int CreateSubKey(void *Parent, char *Name, void **Handle)
{
	int Sts;
	DWORD Dispos;
	REGDATATBL *Pos;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegCreateKeyEx(Parent, Name, 0, "", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, (HKEY *)Handle, &Dispos) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		if((*Handle = malloc(sizeof(REGDATATBL))) != NULL)
		{
			strcpy(((REGDATATBL *)(*Handle))->KeyName, ((REGDATATBL *)Parent)->KeyName);
			strcat(((REGDATATBL *)(*Handle))->KeyName, "\\");
			strcat(((REGDATATBL *)(*Handle))->KeyName, Name);

			((REGDATATBL *)(*Handle))->ValLen = 0;
			((REGDATATBL *)(*Handle))->Next = NULL;

			Pos = (REGDATATBL *)Parent;
			while(Pos->Next != NULL)
				Pos = Pos->Next;
			Pos->Next = *Handle;
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- サブキーをクローズする ------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int CloseSubKey(void *Handle)
{
	if(TmpRegType == REGTYPE_REG)
		RegCloseKey(Handle);
	else
	{
		/* Nothing */
	}
	return(FFFTP_SUCCESS);
}


/*----- サブキーを削除する ----------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int DeleteSubKey(void *Handle, char *Name)
{
	int Sts;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegDeleteKey(Handle, Name) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		Sts = FFFTP_FAIL;
	}
	return(Sts);
}


/*----- 値を削除する ----------------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int DeleteValue(void *Handle, char *Name)
{
	int Sts;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegDeleteValue(Handle, Name) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		Sts = FFFTP_FAIL;
	}
	return(Sts);
}


/*----- INT値を読み込む -------------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		int *Value : INT値を返すワーク
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int ReadIntValueFromReg(void *Handle, char *Name, int *Value)
{
	int Sts;
	DWORD Size;
	char *Pos;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		Size = sizeof(int);
		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Value, &Size) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		if((Pos = ScanValue(Handle, Name)) != NULL)
		{
			*Value = atoi(Pos);
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- INT値を書き込む -------------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		int Value : INT値
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int WriteIntValueToReg(void *Handle, char *Name, int Value)
{
	REGDATATBL *Pos;
	char *Data;
	char Tmp[20];

	if(TmpRegType == REGTYPE_REG)
		RegSetValueEx(Handle, Name, 0, REG_DWORD, (CONST BYTE *)&Value, sizeof(int));
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		sprintf(Tmp, "%d", Value);
		strcat(Data, Tmp);
		Pos->ValLen += strlen(Data) + 1;
	}
	return(FFFTP_SUCCESS);
}


/*----- 文字列を読み込む ------------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		char *Str : 文字列を返すワーク
*		DWORD Size : 最大サイズ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int ReadStringFromReg(void *Handle, char *Name, char *Str, DWORD Size)
{
	int Sts;
	char *Pos;
	// UTF-8対応
	DWORD TempSize;
	char* pa0;
	wchar_t* pw0;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Str, &Size) == ERROR_SUCCESS)
		{
			if(*(Str + Size - 1) != NUL)
				*(Str + Size) = NUL;
			Sts = FFFTP_SUCCESS;
		}
	}
	else
	{
		if((Pos = ScanValue(Handle, Name)) != NULL)
		{
			// UTF-8対応
//			Size = min1(Size-1, strlen(Pos));
//			Size = StrReadIn(Pos, Size, Str);
//			*(Str + Size) = NUL;
//			Sts = FFFTP_SUCCESS;
			switch(IniKanjiCode)
			{
			case KANJI_NOCNV:
				TempSize = min1(Size-1, strlen(Pos));
				TempSize = StrReadIn(Pos, TempSize, Str);
				*(Str + TempSize) = NUL;
				Sts = FFFTP_SUCCESS;
				if(!CheckStringM(Str))
					break;
				// UTF-8ではない可能性がある
				// Shift_JISとみなす
			case KANJI_SJIS:
				if(pa0 = AllocateStringA(Size * 4))
				{
					if(pw0 = AllocateStringW(Size * 4 * 4))
					{
						TempSize = min1((Size * 4) - 1, strlen(Pos));
						TempSize = StrReadIn(Pos, TempSize, pa0);
						*(pa0 + TempSize) = NUL;
						AtoW(pw0, Size * 4 * 4, pa0, -1);
						WtoM(Str, Size, pw0, -1);
						TerminateStringM(Str, Size);
						Sts = FFFTP_SUCCESS;
						FreeDuplicatedString(pw0);
					}
					FreeDuplicatedString(pa0);
				}
				break;
			}
		}
	}
	return(Sts);
}


/*----- 文字列を書き込む ------------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		char *Str :文字列
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int WriteStringToReg(void *Handle, char *Name, char *Str)
{
	REGDATATBL *Pos;
	char *Data;

	if(TmpRegType == REGTYPE_REG)
		RegSetValueEx(Handle, Name, 0, REG_SZ, (CONST BYTE *)Str, strlen(Str)+1);
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		Pos->ValLen += strlen(Data);
		Data = Pos->ValTbl + Pos->ValLen;
		Pos->ValLen += StrCatOut(Str, strlen(Str), Data) + 1;
	}
	return(FFFTP_SUCCESS);
}


/*----- マルチ文字列を読み込む ------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		char *Str : 文字列を返すワーク
*		DWORD Size : 最大サイズ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int ReadMultiStringFromReg(void *Handle, char *Name, char *Str, DWORD Size)
{
	int Sts;
	char *Pos;
	// UTF-8対応
	DWORD TempSize;
	char* pa0;
	wchar_t* pw0;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Str, &Size) == ERROR_SUCCESS)
		{
			if(*(Str + Size - 1) != NUL)
				*(Str + Size) = NUL;
			Sts = FFFTP_SUCCESS;
		}
	}
	else
	{
		if((Pos = ScanValue(Handle, Name)) != NULL)
		{
			// UTF-8対応
//			Size = min1(Size-1, strlen(Pos));
//			Size = StrReadIn(Pos, Size, Str);
//			*(Str + Size) = NUL;
//			Sts = FFFTP_SUCCESS;
			switch(IniKanjiCode)
			{
			case KANJI_NOCNV:
				TempSize = min1(Size - 2, strlen(Pos));
				TempSize = StrReadIn(Pos, TempSize, Str);
				*(Str + TempSize) = NUL;
				*(Str + TempSize + 1) = NUL;
				Sts = FFFTP_SUCCESS;
				if(!CheckMultiStringM(Str))
					break;
				// UTF-8ではない可能性がある
				// Shift_JISとみなす
			case KANJI_SJIS:
				if(pa0 = AllocateStringA(Size * 4))
				{
					if(pw0 = AllocateStringW(Size * 4 * 4))
					{
						TempSize = min1((Size * 4) - 2, strlen(Pos));
						TempSize = StrReadIn(Pos, TempSize, pa0);
						*(pa0 + TempSize) = NUL;
						*(pa0 + TempSize + 1) = NUL;
						AtoWMultiString(pw0, Size * 4 * 4, pa0);
						WtoMMultiString(Str, Size, pw0);
						TerminateStringM(Str, Size);
						TerminateStringM(Str, Size - 1);
						Sts = FFFTP_SUCCESS;
						FreeDuplicatedString(pw0);
					}
					FreeDuplicatedString(pa0);
				}
				break;
			}
		}
	}
	return(Sts);
}


/*----- マルチ文字列を書き込む ------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		char *Str : 文字列
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int WriteMultiStringToReg(void *Handle, char *Name, char *Str)
{
	REGDATATBL *Pos;
	char *Data;

	if(TmpRegType == REGTYPE_REG)
		RegSetValueEx(Handle, Name, 0, REG_MULTI_SZ, (CONST BYTE *)Str, StrMultiLen(Str)+1);
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		Pos->ValLen += strlen(Data);
		Data = Pos->ValTbl + Pos->ValLen;
		Pos->ValLen += StrCatOut(Str, StrMultiLen(Str), Data) + 1;
	}
	return(FFFTP_SUCCESS);
}


/*----- バイナリを読み込む-----------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		void *Bin : バイナリを返すワーク
*		DWORD Size : 最大サイズ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int ReadBinaryFromReg(void *Handle, char *Name, void *Bin, DWORD Size)
{
	int Sts;
	char *Pos;

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Bin, &Size) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		if((Pos = ScanValue(Handle, Name)) != NULL)
		{
			Size = min1(Size, strlen(Pos));
			Size = StrReadIn(Pos, Size, Bin);
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- バイナリを書き込む ----------------------------------------------------
*
*	Parameter
*		void *Handle : ハンドル
*		char *Name : 名前
*		void *Bin : バイナリ
*		int Len : 長さ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int WriteBinaryToReg(void *Handle, char *Name, void *Bin, int Len)
{
	REGDATATBL *Pos;
	char *Data;

	if(TmpRegType == REGTYPE_REG)
		RegSetValueEx(Handle, Name, 0, REG_BINARY, (CONST BYTE *)Bin, Len);
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		Pos->ValLen += strlen(Data);
		Data = Pos->ValTbl + Pos->ValLen;
		Pos->ValLen += StrCatOut(Bin, Len, Data) + 1;
	}
	return(FFFTP_SUCCESS);
}


/*----- 文字列をバッファに追加書き込みする ------------------------------------
*
*	Parameter
*		char *Src : 文字列
*		int len : 文字列の長さ
*		char *Dst : 書き込みするバッファ
*
*	Return Value
*		int 追加したバイト数
*----------------------------------------------------------------------------*/

static int StrCatOut(char *Src, int Len, char *Dst)
{
	int Count;

	Dst += strlen(Dst);
	Count = 0;
	for(; Len > 0; Len--)
	{
		if(*Src == '\\')
		{
			*Dst++ = '\\';
			*Dst++ = '\\';
			Count += 2;
		}
		else if((*Src >= 0x20) && (*Src <= 0x7E))
		{
			*Dst++ = *Src;
			Count++;
		}
		else
		{
			sprintf(Dst, "\\%02X", *(unsigned char *)Src);
			Dst += 3;
			Count += 3;
		}
		Src++;
	}
	*Dst = NUL;
	return(Count);
}


/*----- 文字列をバッファに読み込む --------------------------------------------
*
*	Parameter
*		char *Src : 文字列
*		int Max : 最大サイズ
*		char *Dst : 書き込みするバッファ
*
*	Return Value
*		int 読み込んだバイト数
*----------------------------------------------------------------------------*/

static int StrReadIn(char *Src, int Max, char *Dst)
{
	int Count;
	int Tmp;

	Count = 0;
	while(*Src != NUL)
	{
		if(Count >= Max)
			break;

		if(*Src == '\\')
		{
			Src++;
			if(*Src == '\\')
				*Dst = '\\';
			else
			{
				sscanf(Src, "%02x", &Tmp);
				*Dst = Tmp;
				Src++;
			}
		}
		else
			*Dst = *Src;

		Count++;
		Dst++;
		Src++;
	}
	return(Count);
}


/*----- 値を検索する ----------------------------------------------------------
*
*	Parameter
*		char *Handle : ハンドル
*		char *Name : 名前
*
*	Return Value
*		char *値データの先頭
*			NULL=指定の名前の値が見つからない
*----------------------------------------------------------------------------*/

static char *ScanValue(void *Handle, char *Name)
{
	REGDATATBL *Cur;
	char *Pos;
	char *Ret;

	Ret = NULL;
	Cur = Handle;
	Pos = Cur->ValTbl;
	while(Pos < (Cur->ValTbl + Cur->ValLen))
	{
		if((strncmp(Name, Pos, strlen(Name)) == 0) &&
		   (*(Pos + strlen(Name)) == '='))
		{
			Ret = Pos + strlen(Name) + 1;
			break;
		}
		Pos += strlen(Pos) + 1;
	}
	return(Ret);
}


/*----------- パスワードの妥当性を確認する ------------------------------------
*
*	Parameter
*		char *Password: パスワード文字列
*		char *HashStr: SHA-1ハッシュ文字列
*
*	Return Value
*		int 0 不一致
*			1: 一致
*			2: 異常
*----------------------------------------------------------------------------*/
int CheckPasswordValidity( char* Password, int length, const char* HashStr )
{
	ulong hash1[5];
	ulong hash2[5];
	
	int i, j;
	
	const char* p = HashStr;
	
	/* 空文字列は一致したことにする */
	if( HashStr[0] == NUL )	return 1;

	/* Hashをチェックするする*/
	if( strlen(HashStr) != 40 )	return 2;

	/* Hashをデコードする*/
	for( i = 0; i < 5; i++ ){
		ulong decode = 0;
		for( j = 0; j < 8; j++ ){
			if( *p < 0x40 || 0x40 + 15 < *p ){
				return 2;
			}
			decode = (decode << 4 ) + (*p - 0x40);
			++p;
		}
		hash1[i] = decode;
	}
	
	/* Password をハッシュする */
	sha_memory( Password, length, hash2 );
	
	if( memcmp( (char*)hash1, (char*)hash2, sizeof( hash1 )) == 0 ){
		return 1;
	}
	return 0;
}

/*----------- パスワードの妥当性チェックのための文字列を作成する ------------
*
*	Parameter
*		char *Password: パスワード文字列
*		char *Str: SHA-1ハッシュ文字列格納場所 (41bytes以上)
*
*	Return Value
*		None
*----------------------------------------------------------------------------*/
void CreatePasswordHash( char* Password, int length, char* HashStr )
{
	ulong hash[5];
	int i, j;
	unsigned char *p = (unsigned char *)HashStr;

	sha_memory( Password, length, hash );

	for( i = 0; i < 5; i++ ){
		ulong rest = hash[i];
		for( j = 0; j < 8; j++ ){
			*p++ = (unsigned char)((rest & 0xf0000000) >> 28) + '@';
			rest <<= 4;
		}
	}
	*p = NUL;
}

void SetHashSalt( DWORD salt )
{
	unsigned char* pos = &SecretKey[strlen(SecretKey) + 1];
	*pos++ = ( salt >> 24 ) & 0xff;
	*pos++ = ( salt >> 16 ) & 0xff;
	*pos++ = ( salt >>  8 ) & 0xff;
	*pos++ = ( salt       ) & 0xff;
	
	SecretKeyLength = strlen( SecretKey ) + 5;
}

/*----------- 乱数生成をする -------------------------------------------------
*
*	Parameter
*
*	Return Value
*		ランダムな値：コンパイラVS2005/動作環境WinXP以上では rand_s から取得する
*----------------------------------------------------------------------------*/
DWORD GetRandamDWRODValue(void)
{
	DWORD rndValue;
	int errorCode;
#ifdef _CRT_RAND_S
	errno_t errnoRand_s;
	errnoRand_s = rand_s(&rndValue);
	errorCode = (0 != errnoRand_s ? 1 : 0);
#else
	errorCode = 1;
#endif
	if(0 != errorCode){
#ifdef USE_RANDAM_C_RAND
		rndValue = rand() | (rand() << 16);
#else
		/* rand() のかわりに、SHA-1とパフォーマンスカウンタを用いる */
		ulong shaValue[5];
		LARGE_INTEGER Counter;
		
		if(0 == IsRndSourceInit){
			/* 初回取得時 */
			HANDLE CurProcHandle;
			HANDLE CurThreadHandle;
			
			if(DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
				&CurProcHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
			{
				CloseHandle(CurProcHandle);
			}
			if(DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
				&CurThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
			{
				CloseHandle(CurThreadHandle);
			}
			
			/* _WIN64 では64bitだが、その場合はrand_sが大抵利用可能なのでここでは32bitのみ用いる */
			RndSource[0] = (ulong)CurProcHandle;
			RndSource[1] = (ulong)CurThreadHandle;
			RndSource[2] = (ulong)GetTickCount();
			RndSource[3] = (ulong)timeGetTime();
			RndSource[4] = 0; /* カウントアップ */
			RndSource[5] = RndSource[3] + 1;
			IsRndSourceInit = 1;
		}
		RndSource[4]++;
		RndSource[5] += 0x00010010;
		if(QueryPerformanceCounter(&Counter)){
			RndSource[6] = Counter.LowPart;
			RndSource[7] = Counter.HighPart;
			RndSource[8] = (ulong)rand();
		}else{
			RndSource[6] = (ulong)timeGetTime();
			RndSource[7] = (ulong)rand();
			RndSource[8] = (ulong)rand();
		}
		
		sha_memory((char *)RndSource, sizeof(RndSource), shaValue);
		rndValue = shaValue[0] ^ shaValue[1] ^ shaValue[2] ^ shaValue[3] ^ shaValue[4];
#endif
	}
	return rndValue;
}

// ポータブル版判定
int IsRegAvailable()
{
	int Sts;
	void* h;
	Sts = NO;
	SetRegType(REGTYPE_REG);
	if(OpenReg("FFFTP", &h) == FFFTP_SUCCESS)
	{
		CloseReg(h);
		Sts = YES;
	}
	return Sts;
}

int IsIniAvailable()
{
	int Sts;
	void* h;
	Sts = NO;
	SetRegType(REGTYPE_INI);
	if(OpenReg("FFFTP", &h) == FFFTP_SUCCESS)
	{
		CloseReg(h);
		Sts = YES;
	}
	return Sts;
}

