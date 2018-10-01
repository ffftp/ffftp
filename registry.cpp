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

#include "common.h"
const int AES_BLOCK_SIZE = 16;

/*===== プロトタイプ =====*/

// バグ修正
//static void SaveStr(HKEY hKey, char *Key, char *Str, char *DefaultStr);
//static void SaveIntNum(HKEY hKey, char *Key, int Num, int DefaultNum);
static void SaveStr(void *Handle, char *Key, char *Str, char *DefaultStr);
static void SaveIntNum(void *Handle, char *Key, int Num, int DefaultNum);
static void MakeFontData(LOGFONT Font, HFONT hFont, char *Buf);
static int RestoreFontData(char *Str, LOGFONT *Font);

static void EncodePassword(std::string_view const& Str, char *Buf);

static void DecodePassword(char *Str, char *Buf);
static void DecodePasswordOriginal(char *Str, char *Buf);
static void DecodePassword2(char *Str, char *Buf, const char *Key);
static void DecodePassword3(char *Str, char *Buf);
static bool CreateAesKey(unsigned char *AesKey);

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

// 全設定暗号化対応
//int CheckPasswordValidity( char* Password, int length, const char* HashStr );
//void CreatePasswordHash( char* Password, int length, char* HashStr );
int CheckPasswordValidity( char* Password, int length, const char* HashStr, int StretchCount );
void CreatePasswordHash( char* Password, int length, char* HashStr, int StretchCount );
void SetHashSalt( DWORD salt );
// 全設定暗号化対応
void SetHashSalt1(void* Salt, int Length);

// 全設定暗号化対応
void GetMaskWithHMACSHA1(DWORD IV, const char* Salt, int SaltLength, void* pHash);
void MaskSettingsData(const char* Salt, int SaltLength, void* Data, DWORD Size, int EscapeZero);
void UnmaskSettingsData(const char* Salt, int SaltLength, void* Data, DWORD Size, int EscapeZero);
void CalculateSettingsDataChecksum(void* Data, DWORD Size);

/* 2010.01.30 genta 追加 */
static char SecretKey[FMAX_PATH+1];
static int SecretKeyLength;
static int IsMasterPasswordError = PASSWORD_OK;

static int IsRndSourceInit = 0;
static ulong RndSource[9];

// UTF-8対応
static int IniKanjiCode = KANJI_NOCNV;

// 全設定暗号化対応
static int EncryptSettings = NO;
static BYTE EncryptSettingsChecksum[20];
static int EncryptSettingsError = NO;

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
extern int FwallResolve;
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

// ファイルアイコン表示対応
extern int DispFileIcon;
// タイムスタンプのバグ修正
extern int DispTimeSeconds;
// ファイルの属性を数字で表示
extern int DispPermissionsNumber;
// ディレクトリ自動作成
extern int MakeAllDir;
// UTF-8対応
extern int LocalKanjiCode;
// UPnP対応
extern int UPnPEnabled;
// 全設定暗号化対応
extern int EncryptAllSettings;
// ローカル側自動更新
extern int AutoRefreshFileList;
// 古い処理内容を消去
extern int RemoveOldLog;
// バージョン確認
extern int ReadOnlySettings;
// ファイル一覧バグ修正
extern int AbortOnListError;
// ミラーリング設定追加
extern int MirrorNoTransferContents;
// FireWall設定追加
extern int FwallNoSaveUser;
// ゾーンID設定追加
extern int MarkAsInternet;


void sha_memory(const char* mem, DWORD length, uint32_t* buffer) {
	// ビット反転の必要がある
	if (HCRYPTHASH hash; CryptCreateHash(HCryptProv, CALG_SHA1, 0, 0, &hash)) {
		if (CryptHashData(hash, reinterpret_cast<const BYTE*>(mem), length, 0))
			if (DWORD hashlen = 20; CryptGetHashParam(hash, HP_HASHVAL, reinterpret_cast<BYTE*>(buffer), &hashlen, 0))
				for (DWORD i = 0, end = hashlen / sizeof uint32_t; i < end; i++)
					buffer[i] = _byteswap_ulong(buffer[i]);
		CryptDestroyHash(hash);
	}
}


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
	SecretKeyLength = (int)strlen( SecretKey );
	
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
		// 全設定暗号化対応
		int stretch = 0;
		unsigned char salt1[16];

		// 全設定暗号化対応
//		if( ReadIntValueFromReg(hKey3, "CredentialSalt", &salt)){
//			SetHashSalt( salt );
//		}
//		if( ReadStringFromReg(hKey3, "CredentialCheck", checkbuf, sizeof( checkbuf )) == FFFTP_SUCCESS ){
//			switch( CheckPasswordValidity( SecretKey, SecretKeyLength, checkbuf ) ){
//			case 0: /* not match */
//				IsMasterPasswordError = PASSWORD_UNMATCH;
//				break;
//			case 1: /* match */
//				IsMasterPasswordError = PASSWORD_OK;
//				break;
//			case 2: /* invalid hash */
//			default:
//				IsMasterPasswordError = BAD_PASSWORD_HASH;
//				break;
//			}
//		}
		if(ReadStringFromReg(hKey3, "CredentialCheck1", checkbuf, sizeof(checkbuf)) == FFFTP_SUCCESS)
		{
			if(ReadBinaryFromReg(hKey3, "CredentialSalt1", &salt1, sizeof(salt1)) == FFFTP_SUCCESS)
				SetHashSalt1(&salt1, 16);
			else
				SetHashSalt1(NULL, 0);
			ReadIntValueFromReg(hKey3, "CredentialStretch", &stretch);
			switch(CheckPasswordValidity(SecretKey, SecretKeyLength, checkbuf, stretch))
			{
			case 0:
				IsMasterPasswordError = PASSWORD_UNMATCH;
				break;
			case 1:
				IsMasterPasswordError = PASSWORD_OK;
				break;
			default:
				IsMasterPasswordError = BAD_PASSWORD_HASH;
				break;
			}
		}
		else if(ReadStringFromReg(hKey3, "CredentialCheck", checkbuf, sizeof(checkbuf)) == FFFTP_SUCCESS)
		{
			if(ReadIntValueFromReg(hKey3, "CredentialSalt", &salt) == FFFTP_SUCCESS)
				SetHashSalt(salt);
			else
				SetHashSalt1(NULL, 0);
			switch(CheckPasswordValidity(SecretKey, SecretKeyLength, checkbuf, 0))
			{
			case 0:
				IsMasterPasswordError = PASSWORD_UNMATCH;
				break;
			case 1:
				IsMasterPasswordError = PASSWORD_OK;
				break;
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

	// 全設定暗号化対応
	if(EncryptSettingsError == YES)
		return;

	// バージョン確認
	if(ReadOnlySettings == YES)
		return;

	SetRegType(RegType);
	if(CreateReg("FFFTP", &hKey3) == FFFTP_SUCCESS)
	{
		char buf[48];
		int salt = GetTickCount();
		// 全設定暗号化対応
		unsigned char salt1[16];
		FILETIME ft[4];
	
		WriteIntValueToReg(hKey3, "Version", VER_NUM);
		// 全設定暗号化対応
//		WriteIntValueToReg(hKey3, "CredentialSalt", salt);
//		
//		SetHashSalt( salt );
//		/* save password hash */
//		CreatePasswordHash( SecretKey, SecretKeyLength, buf );
//		WriteStringToReg(hKey3, "CredentialCheck", buf);
		if(EncryptAllSettings == YES)
		{
			GetProcessTimes(GetCurrentProcess(), &ft[0], &ft[1], &ft[2], &ft[3]);
			memcpy(&salt1[0], &salt, 4);
			memcpy(&salt1[4], &ft[0].dwLowDateTime, 4);
			memcpy(&salt1[8], &ft[2].dwLowDateTime, 4);
			memcpy(&salt1[12], &ft[3].dwLowDateTime, 4);
			SetHashSalt1(&salt1, 16);
			WriteBinaryToReg(hKey3, "CredentialSalt1", &salt1, sizeof(salt1));
			WriteIntValueToReg(hKey3, "CredentialStretch", 65535);
			CreatePasswordHash(SecretKey, SecretKeyLength, buf, 65535);
			WriteStringToReg(hKey3, "CredentialCheck1", buf);
		}
		else
		{
			SetHashSalt( salt );
			WriteIntValueToReg(hKey3, "CredentialSalt", salt);
			CreatePasswordHash(SecretKey, SecretKeyLength, buf, 0);
			WriteStringToReg(hKey3, "CredentialCheck", buf);
		}

		// 全設定暗号化対応
		WriteIntValueToReg(hKey3, "EncryptAll", EncryptAllSettings);
		sprintf(Buf, "%d", EncryptAllSettings);
		EncodePassword(Buf, Str);
		WriteStringToReg(hKey3, "EncryptAllDetector", Str);
		EncryptSettings = EncryptAllSettings;
		memset(&EncryptSettingsChecksum, 0, 20);

		// 全設定暗号化対応
//		if(CreateSubKey(hKey3, "Options", &hKey4) == FFFTP_SUCCESS)
		if(EncryptAllSettings == YES)
			strcpy(Str, "EncryptedOptions");
		else
			strcpy(Str, "Options");
		if(CreateSubKey(hKey3, Str, &hKey4) == FFFTP_SUCCESS)
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
				// FireWall設定追加
//				WriteStringToReg(hKey4, "FwallUser", FwallUser);
//				EncodePassword(FwallPass, Str);
				if(FwallNoSaveUser == YES)
				{
					WriteStringToReg(hKey4, "FwallUser", "");
					EncodePassword("", Str);
				}
				else
				{
					WriteStringToReg(hKey4, "FwallUser", FwallUser);
					EncodePassword(FwallPass, Str);
				}
				WriteStringToReg(hKey4, "FwallPass", Str);
				WriteIntValueToReg(hKey4, "FwallPort", FwallPort);
				WriteIntValueToReg(hKey4, "FwallType", FwallType);
				WriteIntValueToReg(hKey4, "FwallDef", FwallDefault);
				WriteIntValueToReg(hKey4, "FwallSec", FwallSecurity);
				WriteIntValueToReg(hKey4, "PasvDef", PasvDefault);
				WriteIntValueToReg(hKey4, "FwallRes", FwallResolve);
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
							// 自動切断対策
							SaveIntNum(hKey5, "Noop", Hist.NoopInterval, DefaultHist.NoopInterval);
							// 再転送対応
							SaveIntNum(hKey5, "ErrMode", Hist.TransferErrorMode, DefaultHist.TransferErrorMode);
							SaveIntNum(hKey5, "ErrNotify", Hist.TransferErrorNotify, DefaultHist.TransferErrorNotify);
							// セッションあたりの転送量制限対策
							SaveIntNum(hKey5, "ErrReconnect", Hist.TransferErrorReconnect, DefaultHist.TransferErrorReconnect);
							// ホスト側の設定ミス対策
							SaveIntNum(hKey5, "NoPasvAdrs", Hist.NoPasvAdrs, DefaultHist.NoPasvAdrs);

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

				// ホスト共通設定機能
				if(CreateSubKey(hKey4, "DefaultHost", &hKey5) == FFFTP_SUCCESS)
				{
					CopyDefaultDefaultHost(&DefaultHost);
					CopyDefaultHost(&Host);
					WriteIntValueToReg(hKey5, "Set", Host.Level);
					SaveStr(hKey5, "HostName", Host.HostName, DefaultHost.HostName);
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
					SaveIntNum(hKey5, "NoEncryption", Host.UseNoEncryption, DefaultHost.UseNoEncryption);
					SaveIntNum(hKey5, "FTPES", Host.UseFTPES, DefaultHost.UseFTPES);
					SaveIntNum(hKey5, "FTPIS", Host.UseFTPIS, DefaultHost.UseFTPIS);
					SaveIntNum(hKey5, "SFTP", Host.UseSFTP, DefaultHost.UseSFTP);
					EncodePassword(Host.PrivateKey, Str);
					SaveStr(hKey5, "PKey", Str, DefaultHost.PrivateKey);
					SaveIntNum(hKey5, "ThreadCount", Host.MaxThreadCount, DefaultHost.MaxThreadCount);
					SaveIntNum(hKey5, "ReuseCmdSkt", Host.ReuseCmdSkt, DefaultHost.ReuseCmdSkt);
					SaveIntNum(hKey5, "MLSD", Host.UseMLSD, DefaultHost.UseMLSD);
					SaveIntNum(hKey5, "Noop", Host.NoopInterval, DefaultHost.NoopInterval);
					SaveIntNum(hKey5, "ErrMode", Host.TransferErrorMode, DefaultHost.TransferErrorMode);
					SaveIntNum(hKey5, "ErrNotify", Host.TransferErrorNotify, DefaultHost.TransferErrorNotify);
					SaveIntNum(hKey5, "ErrReconnect", Host.TransferErrorReconnect, DefaultHost.TransferErrorReconnect);
					SaveIntNum(hKey5, "NoPasvAdrs", Host.NoPasvAdrs, DefaultHost.NoPasvAdrs);
					CloseSubKey(hKey5);
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
							// 自動切断対策
							SaveIntNum(hKey5, "Noop", Host.NoopInterval, DefaultHost.NoopInterval);
							// 再転送対応
							SaveIntNum(hKey5, "ErrMode", Host.TransferErrorMode, DefaultHost.TransferErrorMode);
							SaveIntNum(hKey5, "ErrNotify", Host.TransferErrorNotify, DefaultHost.TransferErrorNotify);
							// セッションあたりの転送量制限対策
							SaveIntNum(hKey5, "ErrReconnect", Host.TransferErrorReconnect, DefaultHost.TransferErrorReconnect);
							// ホスト側の設定ミス対策
							SaveIntNum(hKey5, "NoPasvAdrs", Host.NoPasvAdrs, DefaultHost.NoPasvAdrs);
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

				// ファイルアイコン表示対応
				WriteIntValueToReg(hKey4, "ListIcon", DispFileIcon);
				// タイムスタンプのバグ修正
				WriteIntValueToReg(hKey4, "ListSecond", DispTimeSeconds);
				// ファイルの属性を数字で表示
				WriteIntValueToReg(hKey4, "ListPermitNum", DispPermissionsNumber);
				// ディレクトリ自動作成
				WriteIntValueToReg(hKey4, "MakeDir", MakeAllDir);
				// UTF-8対応
				WriteIntValueToReg(hKey4, "Kanji", LocalKanjiCode);
				// UPnP対応
				WriteIntValueToReg(hKey4, "UPnP", UPnPEnabled);
				// ローカル側自動更新
				WriteIntValueToReg(hKey4, "ListRefresh", AutoRefreshFileList);
				// 古い処理内容を消去
				WriteIntValueToReg(hKey4, "OldLog", RemoveOldLog);
				// ファイル一覧バグ修正
				WriteIntValueToReg(hKey4, "AbortListErr", AbortOnListError);
				// ミラーリング設定追加
				WriteIntValueToReg(hKey4, "MirNoTransfer", MirrorNoTransferContents);
				// FireWall設定追加
				WriteIntValueToReg(hKey4, "FwallShared", FwallNoSaveUser);
				// ゾーンID設定追加
				WriteIntValueToReg(hKey4, "MarkDFile", MarkAsInternet);
			}
			CloseSubKey(hKey4);
		}
		// 全設定暗号化対応
		EncryptSettings = NO;
		WriteBinaryToReg(hKey3, "EncryptAllChecksum", &EncryptSettingsChecksum, 20);
		if(EncryptAllSettings == YES)
		{
			if(OpenSubKey(hKey3, "Options", &hKey4) == FFFTP_SUCCESS)
			{
				for(i = 0; ; i++)
				{
					sprintf(Str, "Host%d", i);
					if(DeleteSubKey(hKey4, Str) != FFFTP_SUCCESS)
						break;
				}
				for(i = 0; ; i++)
				{
					sprintf(Str, "History%d", i);
					if(DeleteSubKey(hKey4, Str) != FFFTP_SUCCESS)
						break;
				}
				CloseSubKey(hKey4);
			}
			DeleteSubKey(hKey3, "Options");
			DeleteValue(hKey3, "CredentialSalt");
			DeleteValue(hKey3, "CredentialCheck");
		}
		else
		{
			if(OpenSubKey(hKey3, "EncryptedOptions", &hKey4) == FFFTP_SUCCESS)
			{
				for(i = 0; ; i++)
				{
					sprintf(Str, "Host%d", i);
					if(DeleteSubKey(hKey4, Str) != FFFTP_SUCCESS)
						break;
				}
				for(i = 0; ; i++)
				{
					sprintf(Str, "History%d", i);
					if(DeleteSubKey(hKey4, Str) != FFFTP_SUCCESS)
						break;
				}
				CloseSubKey(hKey4);
			}
			DeleteSubKey(hKey3, "EncryptedOptions");
			DeleteValue(hKey3, "CredentialSalt1");
			DeleteValue(hKey3, "CredentialStretch");
			DeleteValue(hKey3, "CredentialCheck1");
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
	struct Data {
		using result_t = int;
		static void OnCommand(HWND hDlg, WORD cmd, WORD id) {
			if (cmd == BN_CLICKED)
				EndDialog(hDlg, id);
		}
	};
	void *hKey3;
	void *hKey4;
	void *hKey5;
	int i;
	int Sets;
	// 暗号化通信対応
//	char Str[256];	/* ASCII_EXT_LENより大きい事 */
	char Str[PRIVATE_KEY_LEN*4+1];
	char Buf[FMAX_PATH+1];
	// 全設定暗号化対応
	char Buf2[FMAX_PATH+1];
	char *Pos;
	char *Pos2;
	HOSTDATA Host;
	HISTORYDATA Hist;
	int Sts;
	int Version;
	// 全設定暗号化対応
	BYTE Checksum[20];

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

		// 全設定暗号化対応
		if(Version >= 1990)
		{
			if(GetMasterPasswordStatus() == PASSWORD_OK)
			{
				ReadIntValueFromReg(hKey3, "EncryptAll", &EncryptAllSettings);
				sprintf(Buf, "%d", EncryptAllSettings);
				ReadStringFromReg(hKey3, "EncryptAllDetector", Str, 255);
				DecodePassword(Str, Buf2);
				EncryptSettings = EncryptAllSettings;
				memset(&EncryptSettingsChecksum, 0, 20);
				if(strcmp(Buf, Buf2) != 0)
				{
					switch (Dialog(GetFtpInst(), corruptsettings_dlg, GetMainHwnd(), Data{}))
					{
					case IDCANCEL:
						Terminate();
						break;
					case IDABORT:
						CloseReg(hKey3);
						ClearRegistry();
						ClearIni();
						Restart();
						Terminate();
						break;
					case IDRETRY:
						EncryptSettingsError = YES;
						break;
					case IDIGNORE:
						break;
					}
				}
			}
		}

		// 全設定暗号化対応
//		if(OpenSubKey(hKey3, "Options", &hKey4) == FFFTP_SUCCESS)
		if(EncryptAllSettings == YES)
			strcpy(Str, "EncryptedOptions");
		else
			strcpy(Str, "Options");
		if(OpenSubKey(hKey3, Str, &hKey4) == FFFTP_SUCCESS)
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
			ReadIntValueFromReg(hKey4, "FwallRes", &FwallResolve);
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
					// 自動切断対策
					ReadIntValueFromReg(hKey5, "Noop", &Hist.NoopInterval);
					// 再転送対応
					ReadIntValueFromReg(hKey5, "ErrMode", &Hist.TransferErrorMode);
					ReadIntValueFromReg(hKey5, "ErrNotify", &Hist.TransferErrorNotify);
					// セッションあたりの転送量制限対策
					ReadIntValueFromReg(hKey5, "ErrReconnect", &Hist.TransferErrorReconnect);
					// ホスト側の設定ミス対策
					ReadIntValueFromReg(hKey5, "NoPasvAdrs", &Hist.NoPasvAdrs);

					CloseSubKey(hKey5);
					AddHistoryToHistory(&Hist);
				}
			}

			// ホスト共通設定機能
			if(OpenSubKey(hKey4, "DefaultHost", &hKey5) == FFFTP_SUCCESS)
			{
				CopyDefaultDefaultHost(&Host);
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
				ReadIntValueFromReg(hKey5, "NoEncryption", &Host.UseNoEncryption);
				ReadIntValueFromReg(hKey5, "FTPES", &Host.UseFTPES);
				ReadIntValueFromReg(hKey5, "FTPIS", &Host.UseFTPIS);
				ReadIntValueFromReg(hKey5, "SFTP", &Host.UseSFTP);
				strcpy(Str, "");
				ReadStringFromReg(hKey5, "PKey", Str, PRIVATE_KEY_LEN*4+1);
				DecodePassword(Str, Host.PrivateKey);
				ReadIntValueFromReg(hKey5, "ThreadCount", &Host.MaxThreadCount);
				ReadIntValueFromReg(hKey5, "ReuseCmdSkt", &Host.ReuseCmdSkt);
				ReadIntValueFromReg(hKey5, "MLSD", &Host.UseMLSD);
				ReadIntValueFromReg(hKey5, "Noop", &Host.NoopInterval);
				ReadIntValueFromReg(hKey5, "ErrMode", &Host.TransferErrorMode);
				ReadIntValueFromReg(hKey5, "ErrNotify", &Host.TransferErrorNotify);
				ReadIntValueFromReg(hKey5, "ErrReconnect", &Host.TransferErrorReconnect);
				ReadIntValueFromReg(hKey5, "NoPasvAdrs", &Host.NoPasvAdrs);

				CloseSubKey(hKey5);

				SetDefaultHost(&Host);
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
					// 自動切断対策
					ReadIntValueFromReg(hKey5, "Noop", &Host.NoopInterval);
					// 再転送対応
					ReadIntValueFromReg(hKey5, "ErrMode", &Host.TransferErrorMode);
					ReadIntValueFromReg(hKey5, "ErrNotify", &Host.TransferErrorNotify);
					// セッションあたりの転送量制限対策
					ReadIntValueFromReg(hKey5, "ErrReconnect", &Host.TransferErrorReconnect);
					// ホスト側の設定ミス対策
					ReadIntValueFromReg(hKey5, "NoPasvAdrs", &Host.NoPasvAdrs);

					CloseSubKey(hKey5);

					AddHostToList(&Host, -1, Host.Level);
				}
			}

			ReadIntValueFromReg(hKey4, "CurSet", &Sets);
			SetCurrentHost(Sets);

			// ファイルアイコン表示対応
			ReadIntValueFromReg(hKey4, "ListIcon", &DispFileIcon);
			// タイムスタンプのバグ修正
			ReadIntValueFromReg(hKey4, "ListSecond", &DispTimeSeconds);
			// ファイルの属性を数字で表示
			ReadIntValueFromReg(hKey4, "ListPermitNum", &DispPermissionsNumber);
			// ディレクトリ自動作成
			ReadIntValueFromReg(hKey4, "MakeDir", &MakeAllDir);
			// UTF-8対応
			ReadIntValueFromReg(hKey4, "Kanji", &LocalKanjiCode);
			// UPnP対応
			ReadIntValueFromReg(hKey4, "UPnP", &UPnPEnabled);
			// ローカル側自動更新
			ReadIntValueFromReg(hKey4, "ListRefresh", &AutoRefreshFileList);
			// 古い処理内容を消去
			ReadIntValueFromReg(hKey4, "OldLog", &RemoveOldLog);
			// ファイル一覧バグ修正
			ReadIntValueFromReg(hKey4, "AbortListErr", &AbortOnListError);
			// ミラーリング設定追加
			ReadIntValueFromReg(hKey4, "MirNoTransfer", &MirrorNoTransferContents);
			// FireWall設定追加
			ReadIntValueFromReg(hKey4, "FwallShared", &FwallNoSaveUser);
			// ゾーンID設定追加
			ReadIntValueFromReg(hKey4, "MarkDFile", &MarkAsInternet);

			CloseSubKey(hKey4);
		}
		// 全設定暗号化対応
		EncryptSettings = NO;
		if(Version >= 1990)
		{
			if(GetMasterPasswordStatus() == PASSWORD_OK)
			{
				memset(&Checksum, 0, 20);
				ReadBinaryFromReg(hKey3, "EncryptAllChecksum", &Checksum, 20);
				if(memcmp(&Checksum, &EncryptSettingsChecksum, 20) != 0)
				{
					switch (Dialog(GetFtpInst(), corruptsettings_dlg, GetMainHwnd(), Data{}))
					{
					case IDCANCEL:
						Terminate();
						break;
					case IDABORT:
						CloseReg(hKey3);
						ClearRegistry();
						ClearIni();
						Restart();
						Terminate();
						break;
					case IDRETRY:
						EncryptSettingsError = YES;
						break;
					case IDIGNORE:
						break;
					}
				}
			}
		}
		CloseReg(hKey3);
	}
	else
	{
		/*===== 最初の起動時（設定が無い) =====*/
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
			// 全設定暗号化対応
			if(RegCreateKeyEx(hKey3, "EncryptedOptions", 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey4, &Dispos) == ERROR_SUCCESS)
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
			RegDeleteKey(hKey3, "EncryptedOptions");
			RegCloseKey(hKey3);
		}
		RegDeleteKey(hKey2, "FFFTP");
		RegCloseKey(hKey2);
	}
	return;
}


void ClearIni() {
	fs::remove(fs::u8path(AskIniFilePath()));
}

/*----- ウィンドウ表示無しでプロセス実行 --------------------------------------------
*
* Parameter
*   const char *ApplicationName アプリケーション名
*   char *Commandline 実行パラメーター
*   const char *Directory 実行ディレクトリ
* Return Value
*   プロセスが正常に起動できた場合は0、それ以外の場合はエラーコード
*/
bool ExecuteProcessNoWindow(const WCHAR *ApplicationName, WCHAR *Commandline, const WCHAR *Directory)
{
	STARTUPINFOW si{};
	//ZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFOW); si.wShowWindow = SW_HIDE; si.dwFlags |= STARTF_USESHOWWINDOW;
	PROCESS_INFORMATION pi{};
	if (CreateProcessW(ApplicationName, Commandline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, Directory, &si, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return true;
	}
	else
	{
		return false;
	}
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
	WCHAR Tmp[FMAX_PATH*2];
	// 任意のコードが実行されるバグ修正
	WCHAR SysDir[FMAX_PATH+1];

	if(RegType == REGTYPE_REG)
	{
		if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"FFFTP.reg", L"reg", { FileType::Reg, FileType::All }); !std::empty(path))
		{
			if (std::experimental::filesystem::exists(path))
			{
				std::error_code er;
				if (!std::experimental::filesystem::remove(path, er))
				{
					WCHAR msgtemplate[128];
					WCHAR msg[128];
					MtoW(msgtemplate, 128, MSGJPN366, (int)strlen(MSGJPN366));
					_snwprintf(msg, 128, msgtemplate, er.value());
					MessageBoxW(GetMainHwnd(), msg, L"FFFTP", MB_OK | MB_ICONERROR);
					return;
				}
			}
			// 任意のコードが実行されるバグ修正
//			if(ShellExecute(NULL, "open", "regedit", Tmp, ".", SW_SHOW) <= (HINSTANCE)32)
//			{
//				MessageBox(NULL, MSGJPN285, "FFFTP", MB_OK);
//			}
			if (GetSystemDirectoryW(SysDir, FMAX_PATH) > 0)
			{
				_snwprintf(Tmp, sizeof(Tmp)/sizeof(WCHAR), LR"("%s\reg.exe" export HKEY_CURRENT_USER\Software\sota\FFFTP "%s")", SysDir, path.c_str());
				if (!ExecuteProcessNoWindow(NULL, Tmp, SysDir))
				{
					MessageBox(GetMainHwnd(), MSGJPN285, "FFFTP", MB_OK | MB_ICONERROR);
				}
			}
		}
	}
	else
	{
		if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"FFFTP-Backup.ini", L"ini", { FileType::Ini, FileType::All }); !std::empty(path))
		{
			CopyFileW(u8(AskIniFilePath()).c_str(), path.c_str(), FALSE);
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
	WCHAR Tmp[FMAX_PATH*2];
	// 任意のコードが実行されるバグ修正
	WCHAR SysDir[FMAX_PATH+1];

	Ret = NO;
	if (auto const path = SelectFile(true, GetMainHwnd(), IDS_LOAD_SETTING, L"", L"", { FileType::Reg, FileType::Ini, FileType::All }); !std::empty(path))
	{
		if (ieq(path.extension(), L".reg"s))
		{
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
			if (GetSystemDirectoryW(SysDir, FMAX_PATH) > 0)
			{
				_snwprintf(Tmp, sizeof(Tmp)/sizeof(WCHAR), LR"("%s\reg.exe" import "%s")", SysDir, path.c_str());
				if (!ExecuteProcessNoWindow(NULL, Tmp, SysDir))
				{
					MessageBox(GetMainHwnd(), MSGJPN285, "FFFTP", MB_OK | MB_ICONERROR);
				}
				else
				{
					Ret = YES;
				}
			}
		}
		else if (ieq(path.extension(), L".ini"s))
		{
			CopyFileW(path.c_str(), u8(AskIniFilePath()).c_str(), FALSE);
			Ret = YES;
		}
		else
			// バグ修正
//			MessageBox(NULL, MSGJPN293, "FFFTP", MB_OK);
			MessageBox(GetMainHwnd(), MSGJPN293, "FFFTP", MB_OK | MB_ICONERROR);
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

// バグ修正
//static void SaveStr(HKEY hKey, char *Key, char *Str, char *DefaultStr)
static void SaveStr(void *Handle, char *Key, char *Str, char *DefaultStr)
{
	if((DefaultStr != NULL) && (strcmp(Str, DefaultStr) == 0))
//		DeleteValue(hKey, Key);
		DeleteValue(Handle, Key);
	else
//		WriteStringToReg(hKey, Key, Str);
		WriteStringToReg(Handle, Key, Str);

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

// バグ修正
//static void SaveIntNum(HKEY hKey, char *Key, int Num, int DefaultNum)
static void SaveIntNum(void *Handle, char *Key, int Num, int DefaultNum)
{
	if(Num == DefaultNum)
//		DeleteValue(hKey, Key);
		DeleteValue(Handle, Key);
	else
//		WriteIntValueToReg(hKey, Key, Num);
		WriteIntValueToReg(Handle, Key, Num);

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
		sprintf(Buf, "%ld %ld %ld %ld %ld %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %s",
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
	if(sscanf(Str, "%ld %ld %ld %ld %ld %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu",
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

// パスワードを暗号化する(AES)
static void EncodePassword(std::string_view const& Str, char *Buf) {
	auto result = false;
	try {
		auto p = Buf;
		auto length = size_as<DWORD>(Str);
		auto paddedLength = (length + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;
		paddedLength = std::max(paddedLength, DWORD(AES_BLOCK_SIZE * 2));	/* 最低長を32文字とする */
		std::vector<BYTE> buffer(paddedLength, 0);
		std::copy(begin(Str), end(Str), begin(buffer));

		/* PAD部分を乱数で埋める StrPad[StrLen](が有効な場合) は NUL */
		if (paddedLength <= length + 1 || CryptGenRandom(HCryptProv, paddedLength - length - 1, &buffer[length + 1]))
			// IVの初期化
			if (unsigned char iv[AES_BLOCK_SIZE]; CryptGenRandom(HCryptProv, size_as<DWORD>(iv), iv)) {
				*p++ = '0';
				*p++ = 'C';
				for (auto const& item : iv) {
					sprintf(p, "%02x", item);
					p += 2;
				}
				*p++ = ':';

				// PLAINTEXTKEYBLOB structure https://msdn.microsoft.com/en-us/library/jj650836(v=vs.85).aspx
				struct _PLAINTEXTKEYBLOB {
					BLOBHEADER hdr;
					DWORD dwKeySize;
					BYTE rgbKeyData[32];
				} keyBlob{ { PLAINTEXTKEYBLOB, CUR_BLOB_VERSION, 0, CALG_AES_256 }, 32 };
				if (CreateAesKey(keyBlob.rgbKeyData))
					if (HCRYPTKEY hkey; CryptImportKey(HCryptProv, reinterpret_cast<const BYTE*>(&keyBlob), DWORD(sizeof keyBlob), 0, 0, &hkey)) {
						if (DWORD mode = CRYPT_MODE_CBC; CryptSetKeyParam(hkey, KP_MODE, reinterpret_cast<const BYTE*>(&mode), 0))
							if (CryptSetKeyParam(hkey, KP_IV, iv, 0))
								if (CryptEncrypt(hkey, 0, false, 0, data(buffer), &paddedLength, paddedLength)) {
									for (auto const& item : buffer) {
										sprintf(p, "%02x", item);
										p += 2;
									}
									*p = NUL;
									result = true;
								}
						CryptDestroyKey(hkey);
					}
			}
	}
	catch (std::bad_alloc&) {}
	if (!result)
		Buf[0] = NUL;
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
	else if( strncmp( (const char*)Get, "0A", 2 ) == 0 ){
		DecodePasswordOriginal( Str + 2, Buf );
	}
	else if( strncmp( (const char*)Get, "0B", 2 ) == 0 ){
		DecodePassword2( Str + 2, Buf, SecretKey );
	}
	else if( strncmp( (const char*)Get, "0C", 2 ) == 0 ){
		DecodePassword3( Str + 2, Buf );
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
	unsigned char *KeyEnd = KeyHead + strlen((const char*)KeyHead);
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

static void DecodePassword3(char *Str, char *Buf) {
	try {
		Buf[0] = NUL;
		if (auto length = DWORD(strlen(Str)); AES_BLOCK_SIZE * 2 + 1 < length) {
			DWORD encodedLength = (length - 1) / 2 - AES_BLOCK_SIZE;
			std::vector<unsigned char> buffer(encodedLength + 1, 0);	// NUL終端用に１バイト追加
			unsigned char iv[AES_BLOCK_SIZE];
			for (auto& item : iv) {
				item = hex2bin(*Str++) << 4;
				item |= hex2bin(*Str++);
			}
			if (*Str++ == ':') {
				// PLAINTEXTKEYBLOB structure https://msdn.microsoft.com/en-us/library/jj650836(v=vs.85).aspx
				struct _PLAINTEXTKEYBLOB {
					BLOBHEADER hdr;
					DWORD dwKeySize;
					BYTE rgbKeyData[32];
				} keyBlob{ { PLAINTEXTKEYBLOB, CUR_BLOB_VERSION, 0, CALG_AES_256 }, 32 };
				if (CreateAesKey(keyBlob.rgbKeyData)) {
					for (DWORD i = 0; i < encodedLength; i++) {
						auto item = hex2bin(*Str++) << 4;
						item |= hex2bin(*Str++);
						buffer[i] = static_cast<unsigned char>(item);
					}
					if (HCRYPTKEY hkey; CryptImportKey(HCryptProv, reinterpret_cast<const BYTE*>(&keyBlob), sizeof keyBlob, 0, 0, &hkey)) {
						if (DWORD mode = CRYPT_MODE_CBC; CryptSetKeyParam(hkey, KP_MODE, reinterpret_cast<const BYTE*>(&mode), 0))
							if (CryptSetKeyParam(hkey, KP_IV, iv, 0))
								if (CryptDecrypt(hkey, 0, false, 0, data(buffer), &encodedLength))
									strcpy(Buf, reinterpret_cast<const char*>(data(buffer)));
						CryptDestroyKey(hkey);
					}
				}
			}
		}
	}
	catch (std::bad_alloc&) {}
}

// AES用固定長キーを作成
// SHA-1をもちいて32Byte鍵を生成する
static bool CreateAesKey(unsigned char *AesKey) {
	char* HashKey;
	uint32_t HashKeyLen;
	uint32_t results[10];
	int ByteOffset;
	int KeyIndex;
	int ResIndex;

	HashKeyLen = (uint32_t)strlen(SecretKey) + 16;
	if((HashKey = (char*)malloc(HashKeyLen + 1)) == NULL){
		return false;
	}

	strcpy(HashKey, SecretKey);
	strcat(HashKey, ">g^r=@N7=//z<[`:");
	sha_memory(HashKey, HashKeyLen, results);

	strcpy(HashKey, SecretKey);
	strcat(HashKey, "VG77dO1#EyC]$|C@");
	sha_memory(HashKey, HashKeyLen, results + 5);

	KeyIndex = 0;
	ResIndex = 0;
	while (ResIndex < 8) {
		for (ByteOffset = 0; ByteOffset < 4; ByteOffset++) {
			AesKey[KeyIndex++] = (results[ResIndex] >> ByteOffset * 8) & 0xff;
		}
		ResIndex++;
	}
	free(HashKey);

	return true;
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

// 全設定暗号化対応
typedef struct regdatatbl_reg {
	char KeyName[80+1];			/* キー名 */
	HKEY hKey;
} REGDATATBL_REG;

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
		// 全設定暗号化対応
//		strcpy(Tmp, "Software\\Sota\\");
//		strcat(Tmp, Name);
//		if(RegOpenKeyEx(HKEY_CURRENT_USER, Tmp, 0, KEY_READ, (HKEY *)Handle) == ERROR_SUCCESS)
//			Sts = FFFTP_SUCCESS;
		if((*Handle = malloc(sizeof(REGDATATBL_REG))) != NULL)
		{
			strcpy(((REGDATATBL_REG *)(*Handle))->KeyName, Name);
			strcpy(Tmp, "Software\\Sota\\");
			strcat(Tmp, Name);
			if(RegOpenKeyEx(HKEY_CURRENT_USER, Tmp, 0, KEY_READ, &(((REGDATATBL_REG *)(*Handle))->hKey)) == ERROR_SUCCESS)
				Sts = FFFTP_SUCCESS;
			if(Sts != FFFTP_SUCCESS)
				free(*Handle);
		}
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
		// 全設定暗号化対応
//		strcpy(Tmp, "Software\\Sota\\");
//		strcat(Tmp, Name);
//		if(RegCreateKeyEx(HKEY_CURRENT_USER, Tmp, 0, "", REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, (HKEY *)Handle, &Dispos) == ERROR_SUCCESS)
//			Sts = FFFTP_SUCCESS;
		if((*Handle = malloc(sizeof(REGDATATBL_REG))) != NULL)
		{
			strcpy(((REGDATATBL_REG *)(*Handle))->KeyName, Name);
			strcpy(Tmp, "Software\\Sota\\");
			strcat(Tmp, Name);
			if(RegCreateKeyEx(HKEY_CURRENT_USER, Tmp, 0, "", REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &(((REGDATATBL_REG *)(*Handle))->hKey), &Dispos) == ERROR_SUCCESS)
				Sts = FFFTP_SUCCESS;
			if(Sts != FFFTP_SUCCESS)
				free(*Handle);
		}
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
		// 全設定暗号化対応
//		RegCloseKey(Handle);
		RegCloseKey(((REGDATATBL_REG *)Handle)->hKey);
		free(Handle);

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
			if(WriteOutRegToFile((REGDATATBL*)Handle) == TRUE)
			{
//				/* レジストリをクリア */
//				ClearRegistry();
			}
		}
		/* テーブルを削除 */
		Pos = (REGDATATBL*)Handle;
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
		// バグ修正
//		MessageBox(NULL, MSGJPN240, "FFFTP", MB_OK);
		MessageBox(GetMainHwnd(), MSGJPN240, "FFFTP", MB_OK | MB_ICONERROR);

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
	// バグ修正
	New = NULL;

	if((Strm = fopen(AskIniFilePath(), "rt")) != NULL)
	{
		if((Buf = (char*)malloc(REG_SECT_MAX)) != NULL)
		{
			while(fgets(Buf, REG_SECT_MAX, Strm) != NULL)
			{
				if(*Buf != '#')
				{
					if((Tmp = strchr(Buf, '\n')) != NULL)
						*Tmp = NUL;

					if(*Buf == '[')
					{
						if((New = (REGDATATBL*)malloc(sizeof(REGDATATBL))) != NULL)
						{
							if((Tmp = strchr(Buf, ']')) != NULL)
								*Tmp = NUL;
							// バグ修正
//							strcpy(New->KeyName, Buf+1);
							strncpy(New->KeyName, Buf+1, 80);
							New->KeyName[80] = NUL;
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
						// バグ修正
//						strcpy(Data, Buf);
//						Data += strlen(Buf) + 1;
//						New->ValLen += strlen(Buf) + 1;
						if(Data != NULL && New != NULL)
						{
							if(New->ValLen + strlen(Buf) + 1 <= REG_SECT_MAX)
							{
								strcpy(Data, Buf);
								Data += strlen(Buf) + 1;
								New->ValLen += (int)strlen(Buf) + 1;
							}
						}
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
		// 全設定暗号化対応
//		if(RegOpenKeyEx(Parent, Name, 0, KEY_READ, (HKEY *)Handle) == ERROR_SUCCESS)
//			Sts = FFFTP_SUCCESS;
		if((*Handle = malloc(sizeof(REGDATATBL_REG))) != NULL)
		{
			strcpy(((REGDATATBL_REG *)(*Handle))->KeyName, ((REGDATATBL_REG *)Parent)->KeyName);
			strcat(((REGDATATBL_REG *)(*Handle))->KeyName, "\\");
			strcat(((REGDATATBL_REG *)(*Handle))->KeyName, Name);
			if(RegOpenKeyEx(((REGDATATBL_REG *)Parent)->hKey, Name, 0, KEY_READ, &(((REGDATATBL_REG *)(*Handle))->hKey)) == ERROR_SUCCESS)
				Sts = FFFTP_SUCCESS;
			if(Sts != FFFTP_SUCCESS)
				free(*Handle);
		}
	}
	else
	{
		strcpy(Key, ((REGDATATBL *)Parent)->KeyName);
		strcat(Key, "\\");
		strcat(Key, Name);
		Pos = (REGDATATBL*)Parent;
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
		// 全設定暗号化対応
//		if(RegCreateKeyEx(Parent, Name, 0, "", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, (HKEY *)Handle, &Dispos) == ERROR_SUCCESS)
//			Sts = FFFTP_SUCCESS;
		if((*Handle = malloc(sizeof(REGDATATBL_REG))) != NULL)
		{
			strcpy(((REGDATATBL_REG *)(*Handle))->KeyName, ((REGDATATBL_REG *)Parent)->KeyName);
			strcat(((REGDATATBL_REG *)(*Handle))->KeyName, "\\");
			strcat(((REGDATATBL_REG *)(*Handle))->KeyName, Name);
			if(RegCreateKeyEx(((REGDATATBL_REG *)Parent)->hKey, Name, 0, "", REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &(((REGDATATBL_REG *)(*Handle))->hKey), &Dispos) == ERROR_SUCCESS)
				Sts = FFFTP_SUCCESS;
			if(Sts != FFFTP_SUCCESS)
				free(*Handle);
		}
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
			Pos->Next = (regdatatbl*)*Handle;
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
	// 全設定暗号化対応
//		RegCloseKey(Handle);
	{
		RegCloseKey(((REGDATATBL_REG *)Handle)->hKey);
		free(Handle);
	}
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
		// 全設定暗号化対応
//		if(RegDeleteKey(Handle, Name) == ERROR_SUCCESS)
		if(RegDeleteKey(((REGDATATBL_REG *)Handle)->hKey, Name) == ERROR_SUCCESS)
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
		// 全設定暗号化対応
//		if(RegDeleteValue(Handle, Name) == ERROR_SUCCESS)
		if(RegDeleteValue(((REGDATATBL_REG *)Handle)->hKey, Name) == ERROR_SUCCESS)
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
	// 全設定暗号化対応
	char Path[80];

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		Size = sizeof(int);
		// 全設定暗号化対応
//		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Value, &Size) == ERROR_SUCCESS)
		if(RegQueryValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, NULL, NULL, (BYTE *)Value, &Size) == ERROR_SUCCESS)
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
	// 全設定暗号化対応
	if(Sts == FFFTP_SUCCESS)
	{
		if(EncryptSettings == YES)
		{
			if(TmpRegType == REGTYPE_REG)
				strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
			else
				strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
			strcat(Path, "\\");
			strcat(Path, Name);
			UnmaskSettingsData(Path, (int)strlen(Path), Value, sizeof(int), NO);
			CalculateSettingsDataChecksum(Value, sizeof(int));
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
	// 全設定暗号化対応
	char Path[80];

	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		if(TmpRegType == REGTYPE_REG)
			strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
		else
			strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
		strcat(Path, "\\");
		strcat(Path, Name);
		MaskSettingsData(Path, (int)strlen(Path), &Value, sizeof(int), NO);
	}
	if(TmpRegType == REGTYPE_REG)
		// 全設定暗号化対応
//		RegSetValueEx(Handle, Name, 0, REG_DWORD, (CONST BYTE *)&Value, sizeof(int));
		RegSetValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, 0, REG_DWORD, (CONST BYTE *)&Value, sizeof(int));
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		sprintf(Tmp, "%d", Value);
		strcat(Data, Tmp);
		Pos->ValLen += (int)strlen(Data) + 1;
	}
	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		UnmaskSettingsData(Path, (int)strlen(Path), &Value, sizeof(int), NO);
		CalculateSettingsDataChecksum(&Value, sizeof(int));
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
	// 全設定暗号化対応
	char Path[80];

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		// 全設定暗号化対応
//		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Str, &Size) == ERROR_SUCCESS)
		if(RegQueryValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, NULL, NULL, (BYTE *)Str, &Size) == ERROR_SUCCESS)
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
				TempSize = min1(Size-1, (int)strlen(Pos));
				TempSize = StrReadIn(Pos, TempSize, Str);
				*(Str + TempSize) = NUL;
				Sts = FFFTP_SUCCESS;
				break;
			case KANJI_SJIS:
				if(pa0 = AllocateStringA(Size * 4))
				{
					if(pw0 = AllocateStringW(Size * 4 * 4))
					{
						TempSize = min1((Size * 4) - 1, (int)strlen(Pos));
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
	// 全設定暗号化対応
	if(Sts == FFFTP_SUCCESS)
	{
		if(EncryptSettings == YES)
		{
			if(TmpRegType == REGTYPE_REG)
				strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
			else
				strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
			strcat(Path, "\\");
			strcat(Path, Name);
			UnmaskSettingsData(Path, (int)strlen(Path), Str, (DWORD)strlen(Str) + 1, YES);
			CalculateSettingsDataChecksum(Str, (DWORD)strlen(Str) + 1);
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
	// 全設定暗号化対応
	char Path[80];

	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		if(TmpRegType == REGTYPE_REG)
			strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
		else
			strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
		strcat(Path, "\\");
		strcat(Path, Name);
		MaskSettingsData(Path, (int)strlen(Path), Str, (DWORD)strlen(Str) + 1, YES);
	}
	if(TmpRegType == REGTYPE_REG)
	// 全設定暗号化対応
//		RegSetValueEx(Handle, Name, 0, REG_SZ, (CONST BYTE *)Str, strlen(Str)+1);
		RegSetValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, 0, EncryptSettings == YES ? REG_BINARY : REG_SZ, (CONST BYTE *)Str, (DWORD)strlen(Str)+1);
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		Pos->ValLen += (int)strlen(Data);
		Data = Pos->ValTbl + Pos->ValLen;
		Pos->ValLen += StrCatOut(Str, (int)strlen(Str), Data) + 1;
	}
	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		UnmaskSettingsData(Path, (int)strlen(Path), Str, (DWORD)strlen(Str) + 1, YES);
		CalculateSettingsDataChecksum(Str, (DWORD)strlen(Str) + 1);
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
	// 全設定暗号化対応
	char Path[80];

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		// 全設定暗号化対応
//		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Str, &Size) == ERROR_SUCCESS)
		if(RegQueryValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, NULL, NULL, (BYTE *)Str, &Size) == ERROR_SUCCESS)
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
				TempSize = min1(Size - 2, (int)strlen(Pos));
				TempSize = StrReadIn(Pos, TempSize, Str);
				*(Str + TempSize) = NUL;
				*(Str + TempSize + 1) = NUL;
				Sts = FFFTP_SUCCESS;
				break;
			case KANJI_SJIS:
				if(pa0 = AllocateStringA(Size * 4))
				{
					if(pw0 = AllocateStringW(Size * 4 * 4))
					{
						TempSize = min1((Size * 4) - 2, (int)strlen(Pos));
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
	// 全設定暗号化対応
	if(Sts == FFFTP_SUCCESS)
	{
		if(EncryptSettings == YES)
		{
			if(TmpRegType == REGTYPE_REG)
				strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
			else
				strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
			strcat(Path, "\\");
			strcat(Path, Name);
			UnmaskSettingsData(Path, (int)strlen(Path), Str, StrMultiLen(Str) + 1, YES);
			CalculateSettingsDataChecksum(Str, StrMultiLen(Str) + 1);
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
	// 全設定暗号化対応
	char Path[80];

	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		if(TmpRegType == REGTYPE_REG)
			strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
		else
			strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
		strcat(Path, "\\");
		strcat(Path, Name);
		MaskSettingsData(Path, (int)strlen(Path), Str, StrMultiLen(Str) + 1, YES);
	}
	if(TmpRegType == REGTYPE_REG)
	// 全設定暗号化対応
//		RegSetValueEx(Handle, Name, 0, REG_MULTI_SZ, (CONST BYTE *)Str, StrMultiLen(Str)+1);
		RegSetValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, 0, EncryptSettings == YES ? REG_BINARY : REG_MULTI_SZ, (CONST BYTE *)Str, StrMultiLen(Str)+1);
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		Pos->ValLen += (int)strlen(Data);
		Data = Pos->ValTbl + Pos->ValLen;
		Pos->ValLen += StrCatOut(Str, StrMultiLen(Str), Data) + 1;
	}
	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		UnmaskSettingsData(Path, (int)strlen(Path), Str, StrMultiLen(Str) + 1, YES);
		CalculateSettingsDataChecksum(Str, StrMultiLen(Str) + 1);
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
	// 全設定暗号化対応
	char Path[80];

	Sts = FFFTP_FAIL;
	if(TmpRegType == REGTYPE_REG)
	{
		// 全設定暗号化対応
//		if(RegQueryValueEx(Handle, Name, NULL, NULL, (BYTE *)Bin, &Size) == ERROR_SUCCESS)
		if(RegQueryValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, NULL, NULL, (BYTE *)Bin, &Size) == ERROR_SUCCESS)
			Sts = FFFTP_SUCCESS;
	}
	else
	{
		if((Pos = ScanValue(Handle, Name)) != NULL)
		{
			Size = min1(Size, (int)strlen(Pos));
			Size = StrReadIn(Pos, Size, (char*)Bin);
			Sts = FFFTP_SUCCESS;
		}
	}
	// 全設定暗号化対応
	if(Sts == FFFTP_SUCCESS)
	{
		if(EncryptSettings == YES)
		{
			if(TmpRegType == REGTYPE_REG)
				strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
			else
				strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
			strcat(Path, "\\");
			strcat(Path, Name);
			UnmaskSettingsData(Path, (int)strlen(Path), Bin, Size, NO);
			CalculateSettingsDataChecksum(Bin, Size);
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
	// 全設定暗号化対応
	char Path[80];

	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		if(TmpRegType == REGTYPE_REG)
			strcpy(Path, ((REGDATATBL_REG *)Handle)->KeyName);
		else
			strcpy(Path, ((REGDATATBL *)Handle)->KeyName);
		strcat(Path, "\\");
		strcat(Path, Name);
		MaskSettingsData(Path, (int)strlen(Path), Bin, Len, NO);
	}
	if(TmpRegType == REGTYPE_REG)
	// 全設定暗号化対応
//		RegSetValueEx(Handle, Name, 0, REG_BINARY, (CONST BYTE *)Bin, Len);
		RegSetValueEx(((REGDATATBL_REG *)Handle)->hKey, Name, 0, REG_BINARY, (CONST BYTE *)Bin, Len);
	else
	{
		Pos = (REGDATATBL *)Handle;
		Data = Pos->ValTbl + Pos->ValLen;
		strcpy(Data, Name);
		strcat(Data, "=");
		Pos->ValLen += (int)strlen(Data);
		Data = Pos->ValTbl + Pos->ValLen;
		Pos->ValLen += StrCatOut((char*)Bin, Len, Data) + 1;
	}
	// 全設定暗号化対応
	if(EncryptSettings == YES)
	{
		UnmaskSettingsData(Path, (int)strlen(Path), Bin, Len, NO);
		CalculateSettingsDataChecksum(Bin, Len);
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
	Cur = (REGDATATBL*)Handle;
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
// 全設定暗号化対応
//int CheckPasswordValidity( char* Password, int length, const char* HashStr )
int CheckPasswordValidity( char* Password, int length, const char* HashStr, int StretchCount )
{
	char Buf[MAX_PASSWORD_LEN + 32];
	ulong hash1[5];
	uint32_t hash2[5];
	
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
	for(i = 0; i < StretchCount; i++)
	{
		memcpy(&Buf[0], &hash2, 20);
		memcpy(&Buf[20], Password, length);
		sha_memory(Buf, 20 + length, hash2);
	}
	
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
// 全設定暗号化対応
//void CreatePasswordHash( char* Password, int length, char* HashStr )
void CreatePasswordHash( char* Password, int length, char* HashStr, int StretchCount )
{
	char Buf[MAX_PASSWORD_LEN + 32];
	uint32_t hash[5];
	int i, j;
	unsigned char *p = (unsigned char *)HashStr;

	sha_memory( Password, length, hash );
	for(i = 0; i < StretchCount; i++)
	{
		memcpy(&Buf[0], &hash, 20);
		memcpy(&Buf[20], Password, length);
		sha_memory(Buf, 20 + length, hash);
	}

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
	// 全設定暗号化対応
//	unsigned char* pos = &SecretKey[strlen(SecretKey) + 1];
	unsigned char c[4];
	unsigned char* pos = &c[0];
	*pos++ = ( salt >> 24 ) & 0xff;
	*pos++ = ( salt >> 16 ) & 0xff;
	*pos++ = ( salt >>  8 ) & 0xff;
	*pos++ = ( salt       ) & 0xff;
	
//	SecretKeyLength = strlen( SecretKey ) + 5;
	SetHashSalt1(&c, 4);
}

// 全設定暗号化対応
void SetHashSalt1(void* Salt, int Length)
{
	void* p;
	if(Salt != NULL)
	{
		p = &SecretKey[strlen(SecretKey) + 1];
		memcpy(p, Salt, Length);
		SecretKeyLength = (int)strlen(SecretKey) + 1 + Length;
	}
	else
		SecretKeyLength = (int)strlen(SecretKey) + 1;
}


// 全設定暗号化対応
void GetMaskWithHMACSHA1(DWORD Nonce, const char* Salt, int SaltLength, void* pHash)
{
	BYTE Key[FMAX_PATH*2+1];
	uint32_t Hash[5];
	DWORD i;
	for(i = 0; i < 16; i++)
	{
		Nonce = ~Nonce;
		Nonce *= 1566083941;
		Nonce = _byteswap_ulong(Nonce);
		memcpy(&Key[i * 4], &Nonce, 4);
	}
	memcpy(&Key[64], Salt, SaltLength);
	memcpy(&Key[64 + SaltLength], SecretKey, SecretKeyLength);
	sha_memory((const char*)Key, 64 + SaltLength + SecretKeyLength, Hash);
	// sha.cはビッグエンディアンのため
	for(i = 0; i < 5; i++)
		Hash[i] = _byteswap_ulong(Hash[i]);
	memcpy(&Key[0], &Hash, 20);
	memset(&Key[20], 0, 44);
	for(i = 0; i < 64; i++)
		Key[i] ^= 0x36;
	sha_memory((const char*)Key, 64, Hash);
	// sha.cはビッグエンディアンのため
	for(i = 0; i < 5; i++)
		Hash[i] = _byteswap_ulong(Hash[i]);
	memcpy(&Key[64], &Hash, 20);
	for(i = 0; i < 64; i++)
		Key[i] ^= 0x6a;
	sha_memory((const char*)Key, 84, Hash);
	// sha.cはビッグエンディアンのため
	for(i = 0; i < 5; i++)
		Hash[i] = _byteswap_ulong(Hash[i]);
	memcpy(pHash, &Hash, 20);
}

void MaskSettingsData(const char* Salt, int SaltLength, void* Data, DWORD Size, int EscapeZero)
{
	BYTE* p;
	DWORD i;
	BYTE Mask[20];
	p = (BYTE*)Data;
	for(i = 0; i < Size; i++)
	{
		if(i % 20 == 0)
			GetMaskWithHMACSHA1(i, Salt, SaltLength, &Mask);
		if(EscapeZero == NO || (p[i] != 0 && p[i] != Mask[i % 20]))
			p[i] ^= Mask[i % 20];
	}
}

void UnmaskSettingsData(const char* Salt, int SaltLength, void* Data, DWORD Size, int EscapeZero)
{
	MaskSettingsData(Salt, SaltLength, Data, Size, EscapeZero);
}

void CalculateSettingsDataChecksum(void* Data, DWORD Size)
{
	uint32_t Hash[5];
	DWORD i;
	BYTE Mask[20];
	sha_memory((const char*)Data, Size, Hash);
	// sha.cはビッグエンディアンのため
	for(i = 0; i < 5; i++)
		Hash[i] = _byteswap_ulong(Hash[i]);
	memcpy(&Mask, &Hash, 20);
	for(i = 0; i < 20; i++)
		EncryptSettingsChecksum[i] ^= Mask[i];
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

// バージョン確認
int ReadSettingsVersion()
{
	void *hKey3;
	int i;
	int Version;

	SetRegType(REGTYPE_INI);
	if((i = OpenReg("FFFTP", &hKey3)) != FFFTP_SUCCESS)
	{
		if(AskForceIni() == NO)
		{
			SetRegType(REGTYPE_REG);
			i = OpenReg("FFFTP", &hKey3);
		}
	}
	Version = INT_MAX;
	if(i == FFFTP_SUCCESS)
	{
		ReadIntValueFromReg(hKey3, "Version", &Version);
		CloseReg(hKey3);
	}
	return Version;
}

// FileZilla XML形式エクスポート対応
void SaveSettingsToFileZillaXml()
{
	FILE* f;
	TIME_ZONE_INFORMATION tzi;
	int Level;
	int i;
	HOSTDATA Host;
	char Tmp[FMAX_PATH+1];
	char* p1;
	char* p2;
	if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"FileZilla.xml", L"xml", { FileType::Xml,FileType::All }); !std::empty(path))
	{
		if((f = _wfopen(path.c_str(), L"wt")) != NULL)
		{
			fputs("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n", f);
			fputs("<FileZilla3>\n", f);
			fputs("<Servers>\n", f);
			GetTimeZoneInformation(&tzi);
			Level = 0;
			i = 0;
			while(CopyHostFromList(i, &Host) == FFFTP_SUCCESS)
			{
				while((Host.Level & SET_LEVEL_MASK) < Level)
				{
					fputs("</Folder>\n", f);
					Level--;
				}
				if(Host.Level & SET_LEVEL_GROUP)
				{
					fputs("<Folder expanded=\"1\">\n", f);
					fprintf(f, "%s&#x0A;\n", Host.HostName);
					Level++;
				}
				else
				{
					fputs("<Server>\n", f);
					fprintf(f, "<Host>%s</Host>\n", Host.HostAdrs);
					fprintf(f, "<Port>%d</Port>\n", Host.Port);
					if(Host.UseNoEncryption == YES)
						fprintf(f, "<Protocol>%s</Protocol>\n", "0");
					else if(Host.UseFTPES == YES)
						fprintf(f, "<Protocol>%s</Protocol>\n", "4");
					else if(Host.UseFTPIS == YES)
						fprintf(f, "<Protocol>%s</Protocol>\n", "3");
					else
						fprintf(f, "<Protocol>%s</Protocol>\n", "0");
					fprintf(f, "<Type>%s</Type>\n", "0");
					fprintf(f, "<User>%s</User>\n", Host.UserName);
					fprintf(f, "<Pass>%s</Pass>\n", Host.PassWord);
					fprintf(f, "<Account>%s</Account>\n", Host.Account);
					if(Host.Anonymous == YES || strlen(Host.UserName) == 0)
						fprintf(f, "<Logontype>%s</Logontype>\n", "0");
					else
						fprintf(f, "<Logontype>%s</Logontype>\n", "1");
					fprintf(f, "<TimezoneOffset>%d</TimezoneOffset>\n", tzi.Bias + Host.TimeZone * 60);
					if(Host.Pasv == YES)
						fprintf(f, "<PasvMode>%s</PasvMode>\n", "MODE_PASSIVE");
					else
						fprintf(f, "<PasvMode>%s</PasvMode>\n", "MODE_ACTIVE");
					fprintf(f, "<MaximumMultipleConnections>%d</MaximumMultipleConnections>\n", Host.MaxThreadCount);
					switch(Host.NameKanjiCode)
					{
					case KANJI_SJIS:
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Custom");
						fprintf(f, "<CustomEncoding>%s</CustomEncoding>\n", "Shift_JIS");
						break;
					case KANJI_JIS:
						// 非対応
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Auto");
						break;
					case KANJI_EUC:
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Custom");
						fprintf(f, "<CustomEncoding>%s</CustomEncoding>\n", "EUC-JP");
						break;
					case KANJI_SMB_HEX:
						// 非対応
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Auto");
						break;
					case KANJI_SMB_CAP:
						// 非対応
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Auto");
						break;
					case KANJI_UTF8N:
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "UTF-8");
						break;
					case KANJI_UTF8HFSX:
						// 非対応
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Auto");
						break;
					default:
						fprintf(f, "<EncodingType>%s</EncodingType>\n", "Auto");
						break;
					}
					if(Host.FireWall == YES)
						fprintf(f, "<BypassProxy>%s</BypassProxy>\n", "0");
					else
						fprintf(f, "<BypassProxy>%s</BypassProxy>\n", "1");
					fprintf(f, "<Name>%s</Name>\n", Host.HostName);
					fprintf(f, "<LocalDir>%s</LocalDir>\n", Host.LocalInitDir);
					if(strchr(Host.RemoteInitDir, '\\') != NULL)
					{
						fputs("<RemoteDir>", f);
						fputs("8 0", f);
						strcpy(Tmp, Host.RemoteInitDir);
						p1 = Tmp;
						while(*p1 != '\0')
						{
							while(*p1 == '\\')
							{
								p1++;
							}
							if((p2 = strchr(p1, '\\')) != NULL)
							{
								*p2 = '\0';
								p2++;
							}
							else
								p2 = strchr(p1, '\0');
							if(*p1 != '\0')
								fprintf(f, " %zu %s", _mbslen((const unsigned char*)p1), p1);
							p1 = p2;
						}
						fputs("</RemoteDir>\n", f);
					}
					else if(strchr(Host.RemoteInitDir, '/') != NULL)
					{
						fputs("<RemoteDir>", f);
						fputs("1 0", f);
						strcpy(Tmp, Host.RemoteInitDir);
						p1 = Tmp;
						while(*p1 != '\0')
						{
							while(*p1 == '/')
							{
								p1++;
							}
							if((p2 = strchr(p1, '/')) != NULL)
							{
								*p2 = '\0';
								p2++;
							}
							else
								p2 = strchr(p1, '\0');
							if(*p1 != '\0')
								fprintf(f, " %zu %s", _mbslen((const unsigned char*)p1), p1);
							p1 = p2;
						}
						fputs("</RemoteDir>\n", f);
					}
					else
						fprintf(f, "<RemoteDir>%s</RemoteDir>\n", Host.RemoteInitDir);
					if(Host.SyncMove == YES)
						fprintf(f, "<SyncBrowsing>%s</SyncBrowsing>\n", "1");
					else
						fprintf(f, "<SyncBrowsing>%s</SyncBrowsing>\n", "0");
					fprintf(f, "%s&#x0A;\n", Host.HostName);
					fputs("</Server>\n", f);
				}
				i++;
			}
			while(Level > 0)
			{
				fputs("</Folder>\n", f);
				Level--;
			}
			fputs("</Servers>\n", f);
			// TODO: 環境設定
//			fputs("<Settings>\n", f);
//			fputs("</Settings>\n", f);
			fputs("</FileZilla3>\n", f);
			fclose(f);
		}
		else
			MessageBox(GetMainHwnd(), MSGJPN357, "FFFTP", MB_OK | MB_ICONERROR);
	}
}

// WinSCP INI形式エクスポート対応
void WriteWinSCPString(FILE* f, const char* s)
{
	const char* p;
	p = s;
	while(*p != '\0')
	{
		if(*p & 0x80)
		{
			p = NULL;
			break;
		}
		p++;
	}
	if(!p)
		fputs("%EF%BB%BF", f);
	while(*s != '\0')
	{
		switch(*s)
		{
		case '\t':
		case '\n':
		case '\r':
		case ' ':
		case '%':
		case '*':
		case '?':
		case '\\':
			fprintf(f, "%%%02X", *s & 0xff);
			break;
		default:
			if(*s & 0x80)
				fprintf(f, "%%%02X", *s & 0xff);
			else
				fputc(*s, f);
			break;
		}
		s++;
	}
}

void WriteWinSCPPassword(FILE* f, const char* UserName, const char* HostName, const char* Password)
{
	char Tmp[256];
	strcpy(Tmp, UserName);
	strcat(Tmp, HostName);
	strcat(Tmp, Password);
	fprintf(f, "%02X", ~(0xff ^ 0xa3) & 0xff);
	fprintf(f, "%02X", ~(0x00 ^ 0xa3) & 0xff);
	fprintf(f, "%02X", ~((unsigned char)strlen(Tmp) ^ 0xa3) & 0xff);
	fprintf(f, "%02X", ~(0x00 ^ 0xa3) & 0xff);
	Password = Tmp;
	while(*Password != '\0')
	{
		fprintf(f, "%02X", ~(*Password ^ 0xa3) & 0xff);
		Password++;
	}
}

void SaveSettingsToWinSCPIni()
{
	FILE* f;
	char HostPath[FMAX_PATH+1];
	int Level;
	int i;
	HOSTDATA Host;
	char Tmp[FMAX_PATH+1];
	char* p1;
	MessageBox(GetMainHwnd(), MSGJPN365, "FFFTP", MB_OK);
	if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"WinSCP.ini", L"ini", { FileType::Ini, FileType::All }); !std::empty(path))
	{
		if((f = _wfopen(path.c_str(), L"at")) != NULL)
		{
			strcpy(HostPath, "");
			Level = 0;
			i = 0;
			while(CopyHostFromList(i, &Host) == FFFTP_SUCCESS)
			{
				while((Host.Level & SET_LEVEL_MASK) < Level)
				{
					if((p1 = strrchr(HostPath, '/')) != NULL)
						*p1 = '\0';
					if((p1 = strrchr(HostPath, '/')) != NULL)
						p1++;
					else
						p1 = HostPath;
					*p1 = '\0';
					Level--;
				}
				if(Host.Level & SET_LEVEL_GROUP)
				{
					strcat(HostPath, Host.HostName);
					strcat(HostPath, "/");
					Level++;
				}
				else
				{
					fputs("[Sessions\\", f);
					strcpy(Tmp, HostPath);
					strcat(Tmp, Host.HostName);
					WriteWinSCPString(f, Tmp);
					fputs("]\n", f);
					fputs("HostName=", f);
					WriteWinSCPString(f, Host.HostAdrs);
					fputs("\n", f);
					fprintf(f, "PortNumber=%d\n", Host.Port);
					fputs("UserName=", f);
					WriteWinSCPString(f, Host.UserName);
					fputs("\n", f);
					fprintf(f, "FSProtocol=%s\n", "5");
					fputs("LocalDirectory=", f);
					WriteWinSCPString(f, Host.LocalInitDir);
					fputs("\n", f);
					fputs("RemoteDirectory=", f);
					WriteWinSCPString(f, Host.RemoteInitDir);
					fputs("\n", f);
					if(Host.SyncMove == YES)
						fprintf(f, "SynchronizeBrowsing=%s\n", "1");
					else
						fprintf(f, "SynchronizeBrowsing=%s\n", "0");
					fputs("PostLoginCommands=", f);
					WriteWinSCPString(f, Host.InitCmd);
					fputs("\n", f);
					if(Host.FireWall == YES)
					{
						switch(FwallType)
						{
						case FWALL_NONE:
							break;
						case FWALL_FU_FP_SITE:
							break;
						case FWALL_FU_FP_USER:
							break;
						case FWALL_USER:
							break;
						case FWALL_OPEN:
							break;
						case FWALL_SOCKS4:
							fprintf(f, "ProxyMethod=%s\n", "1");
							break;
						case FWALL_SOCKS5_NOAUTH:
							break;
						case FWALL_SOCKS5_USER:
							fprintf(f, "ProxyMethod=%s\n", "2");
							break;
						case FWALL_FU_FP:
							break;
						case FWALL_SIDEWINDER:
							break;
						default:
							break;
						}
						fputs("ProxyHost=", f);
						WriteWinSCPString(f, FwallHost);
						fputs("\n", f);
						fprintf(f, "ProxyPort=%d\n", FwallPort);
						fputs("ProxyUsername=", f);
						WriteWinSCPString(f, FwallUser);
						fputs("\n", f);
					}
					switch(Host.NameKanjiCode)
					{
					case KANJI_SJIS:
						fprintf(f, "Utf=%s\n", "0");
						break;
					case KANJI_JIS:
						// 非対応
						break;
					case KANJI_EUC:
						// 非対応
						break;
					case KANJI_SMB_HEX:
						// 非対応
						break;
					case KANJI_SMB_CAP:
						// 非対応
						break;
					case KANJI_UTF8N:
						fprintf(f, "Utf=%s\n", "1");
						break;
					case KANJI_UTF8HFSX:
						// 非対応
						break;
					default:
						break;
					}
					if(Host.Pasv == YES)
						fprintf(f, "FtpPasvMode=%s\n", "1");
					else
						fprintf(f, "FtpPasvMode=%s\n", "0");
					if(Host.ListCmdOnly == YES && Host.UseMLSD == NO)
						fprintf(f, "FtpUseMlsd=%s\n", "0");
					fputs("FtpAccount=", f);
					WriteWinSCPString(f, Host.Account);
					fputs("\n", f);
					if(Host.NoopInterval > 0)
						fprintf(f, "FtpPingInterval=%d\n", Host.NoopInterval);
					else
						fprintf(f, "FtpPingType=%s\n", "0");
					if(Host.UseNoEncryption == YES)
						fprintf(f, "Ftps=%s\n", "0");
					else if(Host.UseFTPES == YES)
						fprintf(f, "Ftps=%s\n", "3");
					else if(Host.UseFTPIS == YES)
						fprintf(f, "Ftps=%s\n", "1");
					else
						fprintf(f, "Ftps=%s\n", "0");
					if(Host.FireWall == YES)
					{
						switch(FwallType)
						{
						case FWALL_NONE:
							break;
						case FWALL_FU_FP_SITE:
							fprintf(f, "FtpProxyLogonType=%s\n", "1");
							break;
						case FWALL_FU_FP_USER:
							fprintf(f, "FtpProxyLogonType=%s\n", "2");
							break;
						case FWALL_USER:
							fprintf(f, "FtpProxyLogonType=%s\n", "5");
							break;
						case FWALL_OPEN:
							fprintf(f, "FtpProxyLogonType=%s\n", "3");
							break;
						case FWALL_SOCKS4:
							break;
						case FWALL_SOCKS5_NOAUTH:
							break;
						case FWALL_SOCKS5_USER:
							break;
						case FWALL_FU_FP:
							break;
						case FWALL_SIDEWINDER:
							break;
						default:
							break;
						}
					}
					fputs("Password=", f);
					WriteWinSCPPassword(f, Host.UserName, Host.HostAdrs, Host.PassWord);
					fputs("\n", f);
					if(Host.FireWall == YES)
					{
						fputs("ProxyPasswordEnc=", f);
						WriteWinSCPPassword(f, FwallUser, FwallHost, FwallPass);
						fputs("\n", f);
					}
					fputs("\n", f);
				}
				i++;
			}
			fclose(f);
		}
		else
			MessageBox(GetMainHwnd(), MSGJPN357, "FFFTP", MB_OK | MB_ICONERROR);
	}
}
