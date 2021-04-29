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
#include <xmllite.h>
#pragma comment(lib, "xmllite.lib")
const int AES_BLOCK_SIZE = 16;
static int EncryptSettings = NO;

static inline auto a2w(std::string_view text) {
	return convert<wchar_t>([](auto src, auto srclen, auto dst, auto dstlen) { return MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, src, srclen, dst, dstlen); }, text);
}

class Config {
	void Xor(std::string_view name, void* bin, DWORD len, bool preserveZero) const;
protected:
	const std::string KeyName;
	Config(std::string const& keyName) : KeyName{ keyName } {}
	virtual bool ReadInt(std::string_view name, int& value) const = 0;
	virtual bool ReadValue(std::string_view name, std::string& value) const = 0;
	virtual void WriteInt(std::string_view name, int value) = 0;
	virtual void WriteValue(std::string_view name, std::string_view value, DWORD type) = 0;
public:
	Config(Config const&) = delete;
	virtual ~Config() = default;
	virtual std::unique_ptr<Config> OpenSubKey(const char* Name) = 0;
	virtual std::unique_ptr<Config> CreateSubKey(const char* Name) = 0;
	int ReadIntValueFromReg(std::string_view name, int* value) const {
		if (ReadInt(name, *value)) {
			Xor(name, value, sizeof(int), false);
			return FFFTP_SUCCESS;
		}
		return FFFTP_FAIL;
	}
	void WriteIntValueToReg(std::string_view name, int value) {
		Xor(name, &value, sizeof(int), false);
		WriteInt(name, value);
	}
	int ReadStringFromReg(std::string_view name, _Out_writes_z_(size) char* buffer, DWORD size) const {
		if (std::string value; ReadValue(name, value)) {
			Xor(name, data(value), size_as<DWORD>(value), true);
			strncpy_s(buffer, size, value.c_str(), _TRUNCATE);
			return FFFTP_SUCCESS;
		}
		return FFFTP_FAIL;
	}
	void WriteStringToReg(std::string_view name, std::string_view str) {
		std::string value{ str };
		Xor(name, data(value), size_as<DWORD>(value), true);
		WriteValue(name, value, REG_SZ);
	}
	std::optional<std::vector<std::wstring>> ReadStrings(std::string_view name) const {
		if (std::string value; ReadValue(name, value)) {
			std::vector<std::wstring> result;
			if (!empty(value) && value[0] != '\0') {
				Xor(name, data(value), size_as<DWORD>(value), true);
				static boost::regex re{ R"([^\0]+)" };
				for (boost::sregex_iterator it{ value.begin(), value.end(), re }, end; it != end; ++it)
					result.push_back(u8(sv((*it)[0])));
			}
			return result;
		}
		return {};
	}
	void WriteStrings(std::string_view name, std::vector<std::wstring> const& strings) {
		std::string value;
		for (auto const& string : strings) {
			value += u8(string);
			value += '\0';
		}
		Xor(name, data(value), size_as<DWORD>(value), true);
		WriteValue(name, value, REG_MULTI_SZ);
	}
	int ReadBinaryFromReg(std::string_view name, _Out_writes_(size) void* buffer, DWORD size) const {
		if (std::string value; ReadValue(name, value)) {
			Xor(name, data(value), size_as<DWORD>(value), false);
			std::copy_n(begin(value), std::min(value.size(), (size_t)size), reinterpret_cast<char*>(buffer));
			return FFFTP_SUCCESS;
		}
		return FFFTP_FAIL;
	}
	void WriteBinaryToReg(std::string_view name, const void* bin, int len) {
		std::string value{ reinterpret_cast<const char*>(bin), reinterpret_cast<const char*>(bin) + len };
		Xor(name, data(value), size_as<DWORD>(value), false);
		WriteValue(name, value, REG_BINARY);
	}
	virtual bool DeleteSubKey(std::string_view name) {
		return false;
	}
	virtual void DeleteValue(std::string_view name) {
	}
	void SaveIntNum(std::string_view name, int value, int defaultValue) {
		if (value == defaultValue)
			DeleteValue(name);
		else
			WriteIntValueToReg(name, value);
	}
	void SaveStr(std::string_view name, std::string_view str, std::string_view defaultStr) {
		if (str == defaultStr)
			DeleteValue(name);
		else
			WriteStringToReg(name, str);
	}
};

static std::wstring MakeFontData(HFONT hfont, LOGFONTW const& logFont);
static std::optional<LOGFONTW> RestoreFontData(const wchar_t* str);

static void EncodePassword(std::string_view const& Str, char *Buf);

static void DecodePassword(char *Str, char *Buf);
static void DecodePasswordOriginal(char *Str, char *Buf);
static void DecodePassword2(char *Str, char *Buf, const char *Key);
static void DecodePassword3(char *Str, char *Buf);

static std::unique_ptr<Config> OpenReg(int type);
static std::unique_ptr<Config> CreateReg(int type);
static int CheckPasswordValidity(const char* HashStr, int StretchCount);
static void CreatePasswordHash(char* HashStr, int StretchCount);
void SetHashSalt( DWORD salt );
// 全設定暗号化対応
void SetHashSalt1(void* Salt, int Length);

/* 2010.01.30 genta 追加 */
static char SecretKey[FMAX_PATH+1];
static int SecretKeyLength;
static int IsMasterPasswordError = PASSWORD_OK;

static int IsRndSourceInit = 0;
static uint32_t RndSource[9];

// UTF-8対応
static int IniKanjiCode = KANJI_NOCNV;
static int EncryptSettingsError = NO;

/*===== 外部参照 =====*/

/* 設定値 */
extern int WinPosX;
extern int WinPosY;
extern int WinWidth;
extern int WinHeight;
extern int LocalWidth;
extern int TaskHeight;
extern int LocalTabWidthDefault[4];
extern int LocalTabWidth[4];
extern int RemoteTabWidthDefault[6];
extern int RemoteTabWidth[6];
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern char ViewerName[VIEWERS][FMAX_PATH+1];
extern HFONT ListFont;
extern LOGFONTW ListLogFont;
extern int LocalFileSort;
extern int LocalDirSort;
extern int RemoteFileSort;
extern int RemoteDirSort;
extern int TransMode;
extern int ConnectOnStart;
extern int SaveWinPos;
extern std::vector<std::wstring> AsciiExt;
extern int RecvMode;
extern int SendMode;
extern int MoveMode;
extern int ListType;
extern char DefaultLocalPath[FMAX_PATH+1];
extern int SaveTimeStamp;
extern int FindMode;
extern int DotFile;
extern int DclickOpen;
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
extern std::vector<std::wstring> MirrorNoTrn;
extern std::vector<std::wstring> MirrorNoDel;
extern int MirrorFnameCnv;
//extern int MirrorFolderCnv;
extern int RasClose;
extern int RasCloseNotify;
extern int FileHist;
extern std::vector<std::wstring> DefAttrList;
extern SIZE HostDlgSize;
extern SIZE BmarkDlgSize;
extern SIZE MirrorDlgSize;
extern int Sizing;
extern int SortSave;
extern int QuickAnonymous;
extern int PassToHist;
extern int VaxSemicolon;
extern int SendQuit;
extern int NoRasControl;
extern int SuppressSave;

extern int UpExistMode;
extern int ExistMode;
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
		strncpy_s(SecretKey, Password, MAX_PASSWORD_LEN);
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
	std::unique_ptr<Config> hKey3;
	if (hKey3 = OpenReg(REGTYPE_INI); !hKey3 && AskForceIni() == NO)
		hKey3 = OpenReg(REGTYPE_REG);
	if(hKey3){
		char checkbuf[48];
		int salt = 0;
		// 全設定暗号化対応
		int stretch = 0;
		unsigned char salt1[16];

		if(hKey3->ReadStringFromReg("CredentialCheck1", checkbuf, sizeof(checkbuf)) == FFFTP_SUCCESS)
		{
			if(hKey3->ReadBinaryFromReg("CredentialSalt1", &salt1, sizeof(salt1)) == FFFTP_SUCCESS)
				SetHashSalt1(&salt1, 16);
			else
				SetHashSalt1(NULL, 0);
			hKey3->ReadIntValueFromReg("CredentialStretch", &stretch);
			IsMasterPasswordError = CheckPasswordValidity(checkbuf, stretch);
		}
		else if(hKey3->ReadStringFromReg("CredentialCheck", checkbuf, sizeof(checkbuf)) == FFFTP_SUCCESS)
		{
			if(hKey3->ReadIntValueFromReg("CredentialSalt", &salt) == FFFTP_SUCCESS)
				SetHashSalt(salt);
			else
				SetHashSalt1(NULL, 0);
			IsMasterPasswordError = CheckPasswordValidity(checkbuf, 0);
		}
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
	char Str[FMAX_PATH+1];
	char Buf[FMAX_PATH+1];
	int i;
	int n;
	
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

	if(auto hKey3 = CreateReg(RegType))
	{
		char buf[48];
		int salt = GetTickCount();
		// 全設定暗号化対応
		unsigned char salt1[16];
		FILETIME ft[4];
	
		hKey3->WriteIntValueToReg("Version", VER_NUM);
		if(EncryptAllSettings == YES)
		{
			GetProcessTimes(GetCurrentProcess(), &ft[0], &ft[1], &ft[2], &ft[3]);
			memcpy(&salt1[0], &salt, 4);
			memcpy(&salt1[4], &ft[0].dwLowDateTime, 4);
			memcpy(&salt1[8], &ft[2].dwLowDateTime, 4);
			memcpy(&salt1[12], &ft[3].dwLowDateTime, 4);
			SetHashSalt1(&salt1, 16);
			hKey3->WriteBinaryToReg("CredentialSalt1", &salt1, sizeof(salt1));
			hKey3->WriteIntValueToReg("CredentialStretch", 65535);
			CreatePasswordHash(buf, 65535);
			hKey3->WriteStringToReg("CredentialCheck1", buf);
		}
		else
		{
			SetHashSalt( salt );
			hKey3->WriteIntValueToReg("CredentialSalt", salt);
			CreatePasswordHash(buf, 0);
			hKey3->WriteStringToReg("CredentialCheck", buf);
		}

		// 全設定暗号化対応
		hKey3->WriteIntValueToReg("EncryptAll", EncryptAllSettings);
		sprintf(Buf, "%d", EncryptAllSettings);
		EncodePassword(Buf, Str);
		hKey3->WriteStringToReg("EncryptAllDetector", Str);
		EncryptSettings = EncryptAllSettings;

		// 全設定暗号化対応
//		if(CreateSubKey(hKey3, "Options", &hKey4) == FFFTP_SUCCESS)
		if(EncryptAllSettings == YES)
			strcpy(Str, "EncryptedOptions");
		else
			strcpy(Str, "Options");
		if(auto hKey4 = hKey3->CreateSubKey(Str))
		{
			hKey4->WriteIntValueToReg("NoSave", SuppressSave);

			if(SuppressSave != YES)
			{
				hKey4->WriteIntValueToReg("WinPosX", WinPosX);
				hKey4->WriteIntValueToReg("WinPosY", WinPosY);
				hKey4->WriteIntValueToReg("WinWidth", WinWidth);
				hKey4->WriteIntValueToReg("WinHeight", WinHeight);
				hKey4->WriteIntValueToReg("LocalWidth", LocalWidth);
				hKey4->WriteIntValueToReg("TaskHeight", TaskHeight);
				hKey4->WriteBinaryToReg("LocalColm", LocalTabWidth, sizeof(LocalTabWidth));
				hKey4->WriteBinaryToReg("RemoteColm", RemoteTabWidth, sizeof(RemoteTabWidth));
				hKey4->WriteIntValueToReg("SwCmd", Sizing);

				hKey4->WriteStringToReg("UserMail", UserMailAdrs);
				hKey4->WriteStringToReg("Viewer", ViewerName[0]);
				hKey4->WriteStringToReg("Viewer2", ViewerName[1]);
				hKey4->WriteStringToReg("Viewer3", ViewerName[2]);

				hKey4->WriteIntValueToReg("TrType", TransMode);
				hKey4->WriteIntValueToReg("Recv", RecvMode);
				hKey4->WriteIntValueToReg("Send", SendMode);
				hKey4->WriteIntValueToReg("Move", MoveMode);
				hKey4->WriteStringToReg("Path", DefaultLocalPath);
				hKey4->WriteIntValueToReg("Time", SaveTimeStamp);
				hKey4->WriteIntValueToReg("EOF", RmEOF);
				hKey4->WriteIntValueToReg("Scolon", VaxSemicolon);

				hKey4->WriteIntValueToReg("RecvEx", ExistMode);
				hKey4->WriteIntValueToReg("SendEx", UpExistMode);

				hKey4->WriteIntValueToReg("LFsort", LocalFileSort);
				hKey4->WriteIntValueToReg("LDsort", LocalDirSort);
				hKey4->WriteIntValueToReg("RFsort", RemoteFileSort);
				hKey4->WriteIntValueToReg("RDsort", RemoteDirSort);
				hKey4->WriteIntValueToReg("SortSave", SortSave);

				hKey4->WriteIntValueToReg("ListType", ListType);
				hKey4->WriteIntValueToReg("DotFile", DotFile);
				hKey4->WriteIntValueToReg("Dclick", DclickOpen);

				hKey4->WriteIntValueToReg("ConS", ConnectOnStart);
				hKey4->WriteIntValueToReg("OldDlg", ConnectAndSet);
				hKey4->WriteIntValueToReg("RasClose", RasClose);
				hKey4->WriteIntValueToReg("RasNotify", RasCloseNotify);
				hKey4->WriteIntValueToReg("Qanony", QuickAnonymous);
				hKey4->WriteIntValueToReg("PassHist", PassToHist);
				hKey4->WriteIntValueToReg("SendQuit", SendQuit);
				hKey4->WriteIntValueToReg("NoRas", NoRasControl);

				hKey4->WriteIntValueToReg("Debug", DebugConsole);
				hKey4->WriteIntValueToReg("WinPos", SaveWinPos);
				hKey4->WriteIntValueToReg("RegExp", FindMode);
				hKey4->WriteIntValueToReg("Reg", RegType);

				hKey4->WriteStrings("AsciiFile"sv, AsciiExt);
				hKey4->WriteIntValueToReg("LowUp", FnameCnv);
				hKey4->WriteIntValueToReg("Tout", TimeOut);

				hKey4->WriteStrings("NoTrn"sv, MirrorNoTrn);
				hKey4->WriteStrings("NoDel"sv, MirrorNoDel);
				hKey4->WriteIntValueToReg("MirFile", MirrorFnameCnv);
				hKey4->WriteIntValueToReg("MirUNot", MirUpDelNotify);
				hKey4->WriteIntValueToReg("MirDNot", MirDownDelNotify);

				strcpy(Str, u8(MakeFontData(ListFont, ListLogFont)).c_str());
				hKey4->WriteStringToReg("ListFont", Str);
				hKey4->WriteIntValueToReg("ListHide", DispIgnoreHide);
				hKey4->WriteIntValueToReg("ListDrv", DispDrives);

				hKey4->WriteStringToReg("FwallHost", FwallHost);
				if(FwallNoSaveUser == YES)
				{
					hKey4->WriteStringToReg("FwallUser", "");
					EncodePassword("", Str);
				}
				else
				{
					hKey4->WriteStringToReg("FwallUser", FwallUser);
					EncodePassword(FwallPass, Str);
				}
				hKey4->WriteStringToReg("FwallPass", Str);
				hKey4->WriteIntValueToReg("FwallPort", FwallPort);
				hKey4->WriteIntValueToReg("FwallType", FwallType);
				hKey4->WriteIntValueToReg("FwallDef", FwallDefault);
				hKey4->WriteIntValueToReg("FwallSec", FwallSecurity);
				hKey4->WriteIntValueToReg("PasvDef", PasvDefault);
				hKey4->WriteIntValueToReg("FwallRes", FwallResolve);
				hKey4->WriteIntValueToReg("FwallLow", FwallLower);
				hKey4->WriteIntValueToReg("FwallDel", FwallDelimiter);

				hKey4->WriteStrings("DefAttr"sv, DefAttrList);

				hKey4->WriteBinaryToReg("Hdlg", &HostDlgSize, sizeof(SIZE));
				hKey4->WriteBinaryToReg("Bdlg", &BmarkDlgSize, sizeof(SIZE));
				hKey4->WriteBinaryToReg("Mdlg", &MirrorDlgSize, sizeof(SIZE));

				hKey4->WriteIntValueToReg("FAttrSw", FolderAttr);
				hKey4->WriteIntValueToReg("FAttr", FolderAttrNum);

				hKey4->WriteIntValueToReg("HistNum", FileHist);

				/* Ver1.54a以前の形式のヒストリデータは削除 */
				hKey4->DeleteValue("Hist");

				/* ヒストリの設定を保存 */
				HISTORYDATA DefaultHist;
				n = 0;
				auto& histories = GetHistories();
				for (auto it = rbegin(histories); it != rend(histories); it++) {
					auto& Hist = *it;
					sprintf(Str, "History%d", n);
					if(auto hKey5 = hKey4->CreateSubKey(Str))
					{
						hKey5->SaveStr("HostAdrs", Hist.HostAdrs, DefaultHist.HostAdrs);
						hKey5->SaveStr("UserName", Hist.UserName, DefaultHist.UserName);
						hKey5->SaveStr("Account", Hist.Account, DefaultHist.Account);
						hKey5->WriteStringToReg("LocalDir", Hist.LocalInitDir);
						hKey5->SaveStr("RemoteDir", Hist.RemoteInitDir, DefaultHist.RemoteInitDir);
						hKey5->SaveStr("Chmod", Hist.ChmodCmd, DefaultHist.ChmodCmd);
						hKey5->SaveStr("Nlst", Hist.LsName, DefaultHist.LsName);
						hKey5->SaveStr("Init", Hist.InitCmd, DefaultHist.InitCmd);
						EncodePassword(Hist.PassWord, Str);
						hKey5->SaveStr("Password", Str, DefaultHist.PassWord);
						hKey5->SaveIntNum("Port", Hist.Port, DefaultHist.Port);
						hKey5->SaveIntNum("Kanji", Hist.KanjiCode, DefaultHist.KanjiCode);
						hKey5->SaveIntNum("KanaCnv", Hist.KanaCnv, DefaultHist.KanaCnv);
						hKey5->SaveIntNum("NameKanji", Hist.NameKanjiCode, DefaultHist.NameKanjiCode);
						hKey5->SaveIntNum("NameKana", Hist.NameKanaCnv, DefaultHist.NameKanaCnv);
						hKey5->SaveIntNum("Pasv", Hist.Pasv, DefaultHist.Pasv);
						hKey5->SaveIntNum("Fwall", Hist.FireWall, DefaultHist.FireWall);
						hKey5->SaveIntNum("List", Hist.ListCmdOnly, DefaultHist.ListCmdOnly);
						hKey5->SaveIntNum("NLST-R", Hist.UseNLST_R, DefaultHist.UseNLST_R);
						hKey5->SaveIntNum("Tzone", Hist.TimeZone, DefaultHist.TimeZone);
						hKey5->SaveIntNum("Type", Hist.HostType, DefaultHist.HostType);
						hKey5->SaveIntNum("Sync", Hist.SyncMove, DefaultHist.SyncMove);
						hKey5->SaveIntNum("Fpath", Hist.NoFullPath, DefaultHist.NoFullPath);
						hKey5->WriteBinaryToReg("Sort", &Hist.Sort, sizeof(Hist.Sort));
						hKey5->SaveIntNum("Secu", Hist.Security, DefaultHist.Security);
						hKey5->WriteIntValueToReg("TrType", Hist.Type);
						hKey5->SaveIntNum("Dial", Hist.Dialup, DefaultHist.Dialup);
						hKey5->SaveIntNum("UseIt", Hist.DialupAlways, DefaultHist.DialupAlways);
						hKey5->SaveIntNum("Notify", Hist.DialupNotify, DefaultHist.DialupNotify);
						hKey5->SaveStr("DialTo", Hist.DialEntry, DefaultHist.DialEntry);
						// 暗号化通信対応
						hKey5->SaveIntNum("NoEncryption", Hist.UseNoEncryption, DefaultHist.UseNoEncryption);
						hKey5->SaveIntNum("FTPES", Hist.UseFTPES, DefaultHist.UseFTPES);
						hKey5->SaveIntNum("FTPIS", Hist.UseFTPIS, DefaultHist.UseFTPIS);
						hKey5->SaveIntNum("SFTP", Hist.UseSFTP, DefaultHist.UseSFTP);
						// 同時接続対応
						hKey5->SaveIntNum("ThreadCount", Hist.MaxThreadCount, DefaultHist.MaxThreadCount);
						hKey5->SaveIntNum("ReuseCmdSkt", Hist.ReuseCmdSkt, DefaultHist.ReuseCmdSkt);
						// MLSD対応
						hKey5->SaveIntNum("MLSD", Hist.UseMLSD, DefaultHist.UseMLSD);
						// 自動切断対策
						hKey5->SaveIntNum("Noop", Hist.NoopInterval, DefaultHist.NoopInterval);
						// 再転送対応
						hKey5->SaveIntNum("ErrMode", Hist.TransferErrorMode, DefaultHist.TransferErrorMode);
						hKey5->SaveIntNum("ErrNotify", Hist.TransferErrorNotify, DefaultHist.TransferErrorNotify);
						// セッションあたりの転送量制限対策
						hKey5->SaveIntNum("ErrReconnect", Hist.TransferErrorReconnect, DefaultHist.TransferErrorReconnect);
						// ホスト側の設定ミス対策
						hKey5->SaveIntNum("NoPasvAdrs", Hist.NoPasvAdrs, DefaultHist.NoPasvAdrs);
						n++;
					}
				}
				hKey4->WriteIntValueToReg("SavedHist", n);

				/* 余分なヒストリがあったら削除 */
				for(; n < 999; n++)
				{
					sprintf(Str, "History%d", n);
					if(!hKey4->DeleteSubKey(Str))
						break;
				}

				// ホスト共通設定機能
				HOSTDATA DefaultHost;
				if(auto hKey5 = hKey4->CreateSubKey("DefaultHost"))
				{
					HOSTDATA Host;
					CopyDefaultHost(&Host);
					hKey5->WriteIntValueToReg("Set", Host.Level);
					hKey5->SaveStr("HostName", Host.HostName, DefaultHost.HostName);
					hKey5->SaveStr("HostAdrs", Host.HostAdrs, DefaultHost.HostAdrs);
					hKey5->SaveStr("UserName", Host.UserName, DefaultHost.UserName);
					hKey5->SaveStr("Account", Host.Account, DefaultHost.Account);
					hKey5->WriteStringToReg("LocalDir", Host.LocalInitDir);
					hKey5->SaveStr("RemoteDir", Host.RemoteInitDir, DefaultHost.RemoteInitDir);
					hKey5->SaveStr("Chmod", Host.ChmodCmd, DefaultHost.ChmodCmd);
					hKey5->SaveStr("Nlst", Host.LsName, DefaultHost.LsName);
					hKey5->SaveStr("Init", Host.InitCmd, DefaultHost.InitCmd);
					if(Host.Anonymous == NO)
						EncodePassword(Host.PassWord, Str);
					else
						strcpy(Str, DefaultHost.PassWord);
					hKey5->SaveStr("Password", Str, DefaultHost.PassWord);
					hKey5->SaveIntNum("Port", Host.Port, DefaultHost.Port);
					hKey5->SaveIntNum("Anonymous", Host.Anonymous, DefaultHost.Anonymous);
					hKey5->SaveIntNum("Kanji", Host.KanjiCode, DefaultHost.KanjiCode);
					hKey5->SaveIntNum("KanaCnv", Host.KanaCnv, DefaultHost.KanaCnv);
					hKey5->SaveIntNum("NameKanji", Host.NameKanjiCode, DefaultHost.NameKanjiCode);
					hKey5->SaveIntNum("NameKana", Host.NameKanaCnv, DefaultHost.NameKanaCnv);
					hKey5->SaveIntNum("Pasv", Host.Pasv, DefaultHost.Pasv);
					hKey5->SaveIntNum("Fwall", Host.FireWall, DefaultHost.FireWall);
					hKey5->SaveIntNum("List", Host.ListCmdOnly, DefaultHost.ListCmdOnly);
					hKey5->SaveIntNum("NLST-R", Host.UseNLST_R, DefaultHost.UseNLST_R);
					hKey5->SaveIntNum("Last", Host.LastDir, DefaultHost.LastDir);
					hKey5->SaveIntNum("Tzone", Host.TimeZone, DefaultHost.TimeZone);
					hKey5->SaveIntNum("Type", Host.HostType, DefaultHost.HostType);
					hKey5->SaveIntNum("Sync", Host.SyncMove, DefaultHost.SyncMove);
					hKey5->SaveIntNum("Fpath", Host.NoFullPath, DefaultHost.NoFullPath);
					hKey5->WriteBinaryToReg("Sort", &Host.Sort, sizeof(Host.Sort));
					hKey5->SaveIntNum("Secu", Host.Security, DefaultHost.Security);
					hKey5->WriteStrings("Bmarks"sv, Host.BookMark);
					hKey5->SaveIntNum("Dial", Host.Dialup, DefaultHost.Dialup);
					hKey5->SaveIntNum("UseIt", Host.DialupAlways, DefaultHost.DialupAlways);
					hKey5->SaveIntNum("Notify", Host.DialupNotify, DefaultHost.DialupNotify);
					hKey5->SaveStr("DialTo", Host.DialEntry, DefaultHost.DialEntry);
					hKey5->SaveIntNum("NoEncryption", Host.UseNoEncryption, DefaultHost.UseNoEncryption);
					hKey5->SaveIntNum("FTPES", Host.UseFTPES, DefaultHost.UseFTPES);
					hKey5->SaveIntNum("FTPIS", Host.UseFTPIS, DefaultHost.UseFTPIS);
					hKey5->SaveIntNum("SFTP", Host.UseSFTP, DefaultHost.UseSFTP);
					hKey5->SaveIntNum("ThreadCount", Host.MaxThreadCount, DefaultHost.MaxThreadCount);
					hKey5->SaveIntNum("ReuseCmdSkt", Host.ReuseCmdSkt, DefaultHost.ReuseCmdSkt);
					hKey5->SaveIntNum("MLSD", Host.UseMLSD, DefaultHost.UseMLSD);
					hKey5->SaveIntNum("Noop", Host.NoopInterval, DefaultHost.NoopInterval);
					hKey5->SaveIntNum("ErrMode", Host.TransferErrorMode, DefaultHost.TransferErrorMode);
					hKey5->SaveIntNum("ErrNotify", Host.TransferErrorNotify, DefaultHost.TransferErrorNotify);
					hKey5->SaveIntNum("ErrReconnect", Host.TransferErrorReconnect, DefaultHost.TransferErrorReconnect);
					hKey5->SaveIntNum("NoPasvAdrs", Host.NoPasvAdrs, DefaultHost.NoPasvAdrs);
				}

				/* ホストの設定を保存 */
				CopyDefaultHost(&DefaultHost);
				i = 0;
				for (HOSTDATA Host; CopyHostFromList(i, &Host) == FFFTP_SUCCESS; i++) {
					sprintf(Str, "Host%d", i);
					if(auto hKey5 = hKey4->CreateSubKey(Str))
					{
						hKey5->WriteIntValueToReg("Set", Host.Level);
						hKey5->SaveStr("HostName", Host.HostName, DefaultHost.HostName);
						if((Host.Level & SET_LEVEL_GROUP) == 0)
						{
							hKey5->SaveStr("HostAdrs", Host.HostAdrs, DefaultHost.HostAdrs);
							hKey5->SaveStr("UserName", Host.UserName, DefaultHost.UserName);
							hKey5->SaveStr("Account", Host.Account, DefaultHost.Account);
							hKey5->WriteStringToReg("LocalDir", Host.LocalInitDir);
							hKey5->SaveStr("RemoteDir", Host.RemoteInitDir, DefaultHost.RemoteInitDir);
							hKey5->SaveStr("Chmod", Host.ChmodCmd, DefaultHost.ChmodCmd);
							hKey5->SaveStr("Nlst", Host.LsName, DefaultHost.LsName);
							hKey5->SaveStr("Init", Host.InitCmd, DefaultHost.InitCmd);

							if(Host.Anonymous == NO)
								EncodePassword(Host.PassWord, Str);
							else
								strcpy(Str, DefaultHost.PassWord);
							hKey5->SaveStr("Password", Str, DefaultHost.PassWord);

							hKey5->SaveIntNum("Port", Host.Port, DefaultHost.Port);
							hKey5->SaveIntNum("Anonymous", Host.Anonymous, DefaultHost.Anonymous);
							hKey5->SaveIntNum("Kanji", Host.KanjiCode, DefaultHost.KanjiCode);
							hKey5->SaveIntNum("KanaCnv", Host.KanaCnv, DefaultHost.KanaCnv);
							hKey5->SaveIntNum("NameKanji", Host.NameKanjiCode, DefaultHost.NameKanjiCode);
							hKey5->SaveIntNum("NameKana", Host.NameKanaCnv, DefaultHost.NameKanaCnv);
							hKey5->SaveIntNum("Pasv", Host.Pasv, DefaultHost.Pasv);
							hKey5->SaveIntNum("Fwall", Host.FireWall, DefaultHost.FireWall);
							hKey5->SaveIntNum("List", Host.ListCmdOnly, DefaultHost.ListCmdOnly);
							hKey5->SaveIntNum("NLST-R", Host.UseNLST_R, DefaultHost.UseNLST_R);
							hKey5->SaveIntNum("Last", Host.LastDir, DefaultHost.LastDir);
							hKey5->SaveIntNum("Tzone", Host.TimeZone, DefaultHost.TimeZone);
							hKey5->SaveIntNum("Type", Host.HostType, DefaultHost.HostType);
							hKey5->SaveIntNum("Sync", Host.SyncMove, DefaultHost.SyncMove);
							hKey5->SaveIntNum("Fpath", Host.NoFullPath, DefaultHost.NoFullPath);
							hKey5->WriteBinaryToReg("Sort", &Host.Sort, sizeof(Host.Sort));
							hKey5->SaveIntNum("Secu", Host.Security, DefaultHost.Security);

							hKey5->WriteStrings("Bmarks"sv, Host.BookMark);

							hKey5->SaveIntNum("Dial", Host.Dialup, DefaultHost.Dialup);
							hKey5->SaveIntNum("UseIt", Host.DialupAlways, DefaultHost.DialupAlways);
							hKey5->SaveIntNum("Notify", Host.DialupNotify, DefaultHost.DialupNotify);
							hKey5->SaveStr("DialTo", Host.DialEntry, DefaultHost.DialEntry);
							// 暗号化通信対応
							hKey5->SaveIntNum("NoEncryption", Host.UseNoEncryption, DefaultHost.UseNoEncryption);
							hKey5->SaveIntNum("FTPES", Host.UseFTPES, DefaultHost.UseFTPES);
							hKey5->SaveIntNum("FTPIS", Host.UseFTPIS, DefaultHost.UseFTPIS);
							hKey5->SaveIntNum("SFTP", Host.UseSFTP, DefaultHost.UseSFTP);
							// 同時接続対応
							hKey5->SaveIntNum("ThreadCount", Host.MaxThreadCount, DefaultHost.MaxThreadCount);
							hKey5->SaveIntNum("ReuseCmdSkt", Host.ReuseCmdSkt, DefaultHost.ReuseCmdSkt);
							// MLSD対応
							hKey5->SaveIntNum("MLSD", Host.UseMLSD, DefaultHost.UseMLSD);
							// 自動切断対策
							hKey5->SaveIntNum("Noop", Host.NoopInterval, DefaultHost.NoopInterval);
							// 再転送対応
							hKey5->SaveIntNum("ErrMode", Host.TransferErrorMode, DefaultHost.TransferErrorMode);
							hKey5->SaveIntNum("ErrNotify", Host.TransferErrorNotify, DefaultHost.TransferErrorNotify);
							// セッションあたりの転送量制限対策
							hKey5->SaveIntNum("ErrReconnect", Host.TransferErrorReconnect, DefaultHost.TransferErrorReconnect);
							// ホスト側の設定ミス対策
							hKey5->SaveIntNum("NoPasvAdrs", Host.NoPasvAdrs, DefaultHost.NoPasvAdrs);
						}
					}
				}
				hKey4->WriteIntValueToReg("SetNum", i);

				/* 余分なホストの設定があったら削除 */
				for(; i < 998; i++)
				{
					sprintf(Str, "Host%d", i);
					if(!hKey4->DeleteSubKey(Str))
						break;
				}

				if((i = AskCurrentHost()) == HOSTNUM_NOENTRY)
					i = 0;
				hKey4->WriteIntValueToReg("CurSet", i);

				// ファイルアイコン表示対応
				hKey4->WriteIntValueToReg("ListIcon", DispFileIcon);
				// タイムスタンプのバグ修正
				hKey4->WriteIntValueToReg("ListSecond", DispTimeSeconds);
				// ファイルの属性を数字で表示
				hKey4->WriteIntValueToReg("ListPermitNum", DispPermissionsNumber);
				// ディレクトリ自動作成
				hKey4->WriteIntValueToReg("MakeDir", MakeAllDir);
				// UTF-8対応
				hKey4->WriteIntValueToReg("Kanji", LocalKanjiCode);
				// UPnP対応
				hKey4->WriteIntValueToReg("UPnP", UPnPEnabled);
				// ローカル側自動更新
				hKey4->WriteIntValueToReg("ListRefresh", AutoRefreshFileList);
				// 古い処理内容を消去
				hKey4->WriteIntValueToReg("OldLog", RemoveOldLog);
				// ファイル一覧バグ修正
				hKey4->WriteIntValueToReg("AbortListErr", AbortOnListError);
				// ミラーリング設定追加
				hKey4->WriteIntValueToReg("MirNoTransfer", MirrorNoTransferContents);
				// FireWall設定追加
				hKey4->WriteIntValueToReg("FwallShared", FwallNoSaveUser);
				// ゾーンID設定追加
				hKey4->WriteIntValueToReg("MarkDFile", MarkAsInternet);
			}
		}
		// 全設定暗号化対応
		EncryptSettings = NO;
		if(EncryptAllSettings == YES)
		{
			hKey3->DeleteSubKey("Options");
			hKey3->DeleteValue("CredentialSalt");
			hKey3->DeleteValue("CredentialCheck");
		}
		else
		{
			hKey3->DeleteSubKey("EncryptedOptions");
			hKey3->DeleteValue("CredentialSalt1");
			hKey3->DeleteValue("CredentialStretch");
			hKey3->DeleteValue("CredentialCheck1");
		}
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
	std::unique_ptr<Config> hKey3;
	int i;
	int Sets;
	char Str[FMAX_PATH+1];
	char Buf[FMAX_PATH+1];
	// 全設定暗号化対応
	char Buf2[FMAX_PATH+1];
	int Sts;
	int Version;

	Sts = NO;

	if (hKey3 = OpenReg(REGTYPE_INI); !hKey3 && AskForceIni() == NO)
		hKey3 = OpenReg(REGTYPE_REG);
	if(hKey3)
	{
//		char checkbuf[48];
		int salt = 0;
		Sts = YES;

		hKey3->ReadIntValueFromReg("Version", &Version);
		// UTF-8対応
		if(Version < 1980)
			IniKanjiCode = KANJI_SJIS;

		// 全設定暗号化対応
		if(Version >= 1990)
		{
			if(GetMasterPasswordStatus() == PASSWORD_OK)
			{
				hKey3->ReadIntValueFromReg("EncryptAll", &EncryptAllSettings);
				sprintf(Buf, "%d", EncryptAllSettings);
				hKey3->ReadStringFromReg("EncryptAllDetector", Str, 255);
				DecodePassword(Str, Buf2);
				EncryptSettings = EncryptAllSettings;
				if(strcmp(Buf, Buf2) != 0)
				{
					switch (Dialog(GetFtpInst(), corruptsettings_dlg, GetMainHwnd(), Data{}))
					{
					case IDCANCEL:
						Terminate();
						break;
					case IDABORT:
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
		if(auto hKey4 = hKey3->OpenSubKey(Str))
		{
			hKey4->ReadIntValueFromReg("WinPosX", &WinPosX);
			hKey4->ReadIntValueFromReg("WinPosY", &WinPosY);
			hKey4->ReadIntValueFromReg("WinWidth", &WinWidth);
			hKey4->ReadIntValueFromReg("WinHeight", &WinHeight);
			hKey4->ReadIntValueFromReg("LocalWidth", &LocalWidth);
			/* ↓旧バージョンのバグ対策 */
			LocalWidth = std::max(0, LocalWidth);
			hKey4->ReadIntValueFromReg("TaskHeight", &TaskHeight);
			/* ↓旧バージョンのバグ対策 */
			TaskHeight = std::max(0, TaskHeight);
			hKey4->ReadBinaryFromReg("LocalColm", &LocalTabWidth, sizeof(LocalTabWidth));
			if (std::all_of(std::begin(LocalTabWidth), std::end(LocalTabWidth), [](auto width) { return width <= 0; }))
				std::copy(std::begin(LocalTabWidthDefault), std::end(LocalTabWidthDefault), std::begin(LocalTabWidth));
			hKey4->ReadBinaryFromReg("RemoteColm", &RemoteTabWidth, sizeof(RemoteTabWidth));
			if (std::all_of(std::begin(RemoteTabWidth), std::end(RemoteTabWidth), [](auto width) { return width <= 0; }))
				std::copy(std::begin(RemoteTabWidthDefault), std::end(RemoteTabWidthDefault), std::begin(RemoteTabWidth));
			hKey4->ReadIntValueFromReg("SwCmd", &Sizing);

			hKey4->ReadStringFromReg("UserMail", UserMailAdrs, USER_MAIL_LEN+1);
			hKey4->ReadStringFromReg("Viewer", ViewerName[0], FMAX_PATH+1);
			hKey4->ReadStringFromReg("Viewer2", ViewerName[1], FMAX_PATH+1);
			hKey4->ReadStringFromReg("Viewer3", ViewerName[2], FMAX_PATH+1);

			hKey4->ReadIntValueFromReg("TrType", &TransMode);
			hKey4->ReadIntValueFromReg("Recv", &RecvMode);
			hKey4->ReadIntValueFromReg("Send", &SendMode);
			hKey4->ReadIntValueFromReg("Move", &MoveMode);
			hKey4->ReadStringFromReg("Path", DefaultLocalPath, FMAX_PATH+1);
			hKey4->ReadIntValueFromReg("Time", &SaveTimeStamp);
			hKey4->ReadIntValueFromReg("EOF", &RmEOF);
			hKey4->ReadIntValueFromReg("Scolon", &VaxSemicolon);

			hKey4->ReadIntValueFromReg("RecvEx", &ExistMode);
			hKey4->ReadIntValueFromReg("SendEx", &UpExistMode);

			hKey4->ReadIntValueFromReg("LFsort", &LocalFileSort);
			hKey4->ReadIntValueFromReg("LDsort", &LocalDirSort);
			hKey4->ReadIntValueFromReg("RFsort", &RemoteFileSort);
			hKey4->ReadIntValueFromReg("RDsort", &RemoteDirSort);
			hKey4->ReadIntValueFromReg("SortSave", &SortSave);

			hKey4->ReadIntValueFromReg("ListType", &ListType);
			hKey4->ReadIntValueFromReg("DotFile", &DotFile);
			hKey4->ReadIntValueFromReg("Dclick", &DclickOpen);

			hKey4->ReadIntValueFromReg("ConS", &ConnectOnStart);
			hKey4->ReadIntValueFromReg("OldDlg", &ConnectAndSet);
			hKey4->ReadIntValueFromReg("RasClose", &RasClose);
			hKey4->ReadIntValueFromReg("RasNotify", &RasCloseNotify);
			hKey4->ReadIntValueFromReg("Qanony", &QuickAnonymous);
			hKey4->ReadIntValueFromReg("PassHist", &PassToHist);
			hKey4->ReadIntValueFromReg("SendQuit", &SendQuit);
			hKey4->ReadIntValueFromReg("NoRas", &NoRasControl);

			hKey4->ReadIntValueFromReg("Debug", &DebugConsole);
			hKey4->ReadIntValueFromReg("WinPos", &SaveWinPos);
			hKey4->ReadIntValueFromReg("RegExp", &FindMode);
			hKey4->ReadIntValueFromReg("Reg", &RegType);

			if (auto result = hKey4->ReadStrings("AsciiFile"sv))
				AsciiExt = *result;
			else if (hKey4->ReadStringFromReg("Ascii", Str, size_as<DWORD>(Str)) == FFFTP_SUCCESS) {
				/* 旧ASCIIモードの拡張子の設定を新しいものに変換 */
				static boost::wregex re{ LR"([^;]+)" };
				AsciiExt.clear();
				auto wStr = u8(Str);
				for (boost::wsregex_iterator it{ wStr.begin(), wStr.end(), re }, end; it != end; ++it)
					AsciiExt.push_back(L"*."sv + it->str());
			}
			if (Version < 1986) {
				// アスキーモード判別の改良
				for (auto item : { L"*.js"sv, L"*.vbs"sv, L"*.css"sv, L"*.rss"sv, L"*.rdf"sv, L"*.xml"sv, L"*.xhtml"sv, L"*.xht"sv, L"*.shtml"sv, L"*.shtm"sv, L"*.sh"sv, L"*.py"sv, L"*.rb"sv, L"*.properties"sv, L"*.sql"sv, L"*.asp"sv, L"*.aspx"sv, L"*.php"sv, L"*.htaccess"sv })
					if (std::ranges::find(AsciiExt, item) == AsciiExt.end())
						AsciiExt.emplace_back(item);
			}

			hKey4->ReadIntValueFromReg("LowUp", &FnameCnv);
			hKey4->ReadIntValueFromReg("Tout", &TimeOut);

			if (auto result = hKey4->ReadStrings("NoTrn"sv))
				MirrorNoTrn = *result;
			if (auto result = hKey4->ReadStrings("NoDel"sv))
				MirrorNoDel = *result;
			hKey4->ReadIntValueFromReg("MirFile", &MirrorFnameCnv);
			hKey4->ReadIntValueFromReg("MirUNot", &MirUpDelNotify);
			hKey4->ReadIntValueFromReg("MirDNot", &MirDownDelNotify);

			if (hKey4->ReadStringFromReg("ListFont", Str, 256) == FFFTP_SUCCESS) {
				if (auto logfont = RestoreFontData(u8(Str).c_str())) {
					ListLogFont = *logfont;
					ListFont = CreateFontIndirectW(&ListLogFont);
				} else
					ListLogFont = {};
			}
			hKey4->ReadIntValueFromReg("ListHide", &DispIgnoreHide);
			hKey4->ReadIntValueFromReg("ListDrv", &DispDrives);

			hKey4->ReadStringFromReg("FwallHost", FwallHost, HOST_ADRS_LEN+1);
			hKey4->ReadStringFromReg("FwallUser", FwallUser, USER_NAME_LEN+1);
			hKey4->ReadStringFromReg("FwallPass", Str, 255);
			DecodePassword(Str, FwallPass);
			hKey4->ReadIntValueFromReg("FwallPort", &FwallPort);
			hKey4->ReadIntValueFromReg("FwallType", &FwallType);
			hKey4->ReadIntValueFromReg("FwallDef", &FwallDefault);
			hKey4->ReadIntValueFromReg("FwallSec", &FwallSecurity);
			hKey4->ReadIntValueFromReg("PasvDef", &PasvDefault);
			hKey4->ReadIntValueFromReg("FwallRes", &FwallResolve);
			hKey4->ReadIntValueFromReg("FwallLow", &FwallLower);
			hKey4->ReadIntValueFromReg("FwallDel", &FwallDelimiter);

			if (auto result = hKey4->ReadStrings("DefAttr"sv))
				DefAttrList = *result;

			hKey4->ReadBinaryFromReg("Hdlg", &HostDlgSize, sizeof(SIZE));
			hKey4->ReadBinaryFromReg("Bdlg", &BmarkDlgSize, sizeof(SIZE));
			hKey4->ReadBinaryFromReg("Mdlg", &MirrorDlgSize, sizeof(SIZE));

			hKey4->ReadIntValueFromReg("FAttrSw", &FolderAttr);
			hKey4->ReadIntValueFromReg("FAttr", &FolderAttrNum);

			hKey4->ReadIntValueFromReg("NoSave", &SuppressSave);

			hKey4->ReadIntValueFromReg("HistNum", &FileHist);

			/* ヒストリの設定を読み込む */
			Sets = 0;
			hKey4->ReadIntValueFromReg("SavedHist", &Sets);

			for(i = 0; i < Sets; i++)
			{
				sprintf(Str, "History%d", i);
				if(auto hKey5 = hKey4->OpenSubKey(Str))
				{
					HISTORYDATA Hist;

					hKey5->ReadStringFromReg("HostAdrs", Hist.HostAdrs, HOST_ADRS_LEN+1);
					hKey5->ReadStringFromReg("UserName", Hist.UserName, USER_NAME_LEN+1);
					hKey5->ReadStringFromReg("Account", Hist.Account, ACCOUNT_LEN+1);
					hKey5->ReadStringFromReg("LocalDir", Hist.LocalInitDir, INIT_DIR_LEN+1);
					hKey5->ReadStringFromReg("RemoteDir", Hist.RemoteInitDir, INIT_DIR_LEN+1);
					hKey5->ReadStringFromReg("Chmod", Hist.ChmodCmd, CHMOD_CMD_LEN+1);
					hKey5->ReadStringFromReg("Nlst", Hist.LsName, NLST_NAME_LEN+1);
					hKey5->ReadStringFromReg("Init", Hist.InitCmd, INITCMD_LEN+1);
					hKey5->ReadIntValueFromReg("Port", &Hist.Port);
					hKey5->ReadIntValueFromReg("Kanji", &Hist.KanjiCode);
					hKey5->ReadIntValueFromReg("KanaCnv", &Hist.KanaCnv);
					hKey5->ReadIntValueFromReg("NameKanji", &Hist.NameKanjiCode);
					hKey5->ReadIntValueFromReg("NameKana", &Hist.NameKanaCnv);
					hKey5->ReadIntValueFromReg("Pasv", &Hist.Pasv);
					hKey5->ReadIntValueFromReg("Fwall", &Hist.FireWall);
					hKey5->ReadIntValueFromReg("List", &Hist.ListCmdOnly);
					hKey5->ReadIntValueFromReg("NLST-R", &Hist.UseNLST_R);
					hKey5->ReadIntValueFromReg("Tzone", &Hist.TimeZone);
					hKey5->ReadIntValueFromReg("Type", &Hist.HostType);
					hKey5->ReadIntValueFromReg("Sync", &Hist.SyncMove);
					hKey5->ReadIntValueFromReg("Fpath", &Hist.NoFullPath);
					hKey5->ReadBinaryFromReg("Sort", &Hist.Sort, sizeof(Hist.Sort));
					hKey5->ReadIntValueFromReg("Secu", &Hist.Security);
					hKey5->ReadIntValueFromReg("TrType", &Hist.Type);
					strcpy(Str, "");
					hKey5->ReadStringFromReg("Password", Str, 255);
					DecodePassword(Str, Hist.PassWord);
					hKey5->ReadIntValueFromReg("Dial", &Hist.Dialup);
					hKey5->ReadIntValueFromReg("UseIt", &Hist.DialupAlways);
					hKey5->ReadIntValueFromReg("Notify", &Hist.DialupNotify);
					hKey5->ReadStringFromReg("DialTo", Hist.DialEntry, RAS_NAME_LEN+1);
					// 暗号化通信対応
					hKey5->ReadIntValueFromReg("NoEncryption", &Hist.UseNoEncryption);
					hKey5->ReadIntValueFromReg("FTPES", &Hist.UseFTPES);
					hKey5->ReadIntValueFromReg("FTPIS", &Hist.UseFTPIS);
					hKey5->ReadIntValueFromReg("SFTP", &Hist.UseSFTP);
					// 同時接続対応
					hKey5->ReadIntValueFromReg("ThreadCount", &Hist.MaxThreadCount);
					hKey5->ReadIntValueFromReg("ReuseCmdSkt", &Hist.ReuseCmdSkt);
					// MLSD対応
					hKey5->ReadIntValueFromReg("MLSD", &Hist.UseMLSD);
					// 自動切断対策
					hKey5->ReadIntValueFromReg("Noop", &Hist.NoopInterval);
					// 再転送対応
					hKey5->ReadIntValueFromReg("ErrMode", &Hist.TransferErrorMode);
					hKey5->ReadIntValueFromReg("ErrNotify", &Hist.TransferErrorNotify);
					// セッションあたりの転送量制限対策
					hKey5->ReadIntValueFromReg("ErrReconnect", &Hist.TransferErrorReconnect);
					// ホスト側の設定ミス対策
					hKey5->ReadIntValueFromReg("NoPasvAdrs", &Hist.NoPasvAdrs);

					AddHistoryToHistory(Hist);
				}
			}

			// ホスト共通設定機能
			HOSTDATA Host;
			if(auto hKey5 = hKey4->OpenSubKey("DefaultHost"))
			{
				hKey5->ReadIntValueFromReg("Set", &Host.Level);
				hKey5->ReadStringFromReg("HostName", Host.HostName, HOST_NAME_LEN+1);
				hKey5->ReadStringFromReg("HostAdrs", Host.HostAdrs, HOST_ADRS_LEN+1);
				hKey5->ReadStringFromReg("UserName", Host.UserName, USER_NAME_LEN+1);
				hKey5->ReadStringFromReg("Account", Host.Account, ACCOUNT_LEN+1);
				hKey5->ReadStringFromReg("LocalDir", Host.LocalInitDir, INIT_DIR_LEN+1);
				hKey5->ReadStringFromReg("RemoteDir", Host.RemoteInitDir, INIT_DIR_LEN+1);
				hKey5->ReadStringFromReg("Chmod", Host.ChmodCmd, CHMOD_CMD_LEN+1);
				hKey5->ReadStringFromReg("Nlst", Host.LsName, NLST_NAME_LEN+1);
				hKey5->ReadStringFromReg("Init", Host.InitCmd, INITCMD_LEN+1);
				hKey5->ReadIntValueFromReg("Port", &Host.Port);
				hKey5->ReadIntValueFromReg("Anonymous", &Host.Anonymous);
				hKey5->ReadIntValueFromReg("Kanji", &Host.KanjiCode);
				hKey5->ReadIntValueFromReg("KanaCnv", &Host.KanaCnv);
				hKey5->ReadIntValueFromReg("NameKanji", &Host.NameKanjiCode);
				hKey5->ReadIntValueFromReg("NameKana", &Host.NameKanaCnv);
				hKey5->ReadIntValueFromReg("Pasv", &Host.Pasv);
				hKey5->ReadIntValueFromReg("Fwall", &Host.FireWall);
				hKey5->ReadIntValueFromReg("List", &Host.ListCmdOnly);
				hKey5->ReadIntValueFromReg("NLST-R", &Host.UseNLST_R);
				hKey5->ReadIntValueFromReg("Last", &Host.LastDir);
				hKey5->ReadIntValueFromReg("Tzone", &Host.TimeZone);
				hKey5->ReadIntValueFromReg("Type", &Host.HostType);
				hKey5->ReadIntValueFromReg("Sync", &Host.SyncMove);
				hKey5->ReadIntValueFromReg("Fpath", &Host.NoFullPath);
				hKey5->ReadBinaryFromReg("Sort", &Host.Sort, sizeof(Host.Sort));
				hKey5->ReadIntValueFromReg("Secu", &Host.Security);
				if(Host.Anonymous != YES)
				{
					strcpy(Str, "");
					hKey5->ReadStringFromReg("Password", Str, 255);
					DecodePassword(Str, Host.PassWord);
				}
				else
					strcpy(Host.PassWord, UserMailAdrs);
				if (auto result = hKey5->ReadStrings("Bmarks"sv))
					Host.BookMark = *result;
				hKey5->ReadIntValueFromReg("Dial", &Host.Dialup);
				hKey5->ReadIntValueFromReg("UseIt", &Host.DialupAlways);
				hKey5->ReadIntValueFromReg("Notify", &Host.DialupNotify);
				hKey5->ReadStringFromReg("DialTo", Host.DialEntry, RAS_NAME_LEN+1);
				hKey5->ReadIntValueFromReg("NoEncryption", &Host.UseNoEncryption);
				hKey5->ReadIntValueFromReg("FTPES", &Host.UseFTPES);
				hKey5->ReadIntValueFromReg("FTPIS", &Host.UseFTPIS);
				hKey5->ReadIntValueFromReg("SFTP", &Host.UseSFTP);
				hKey5->ReadIntValueFromReg("ThreadCount", &Host.MaxThreadCount);
				hKey5->ReadIntValueFromReg("ReuseCmdSkt", &Host.ReuseCmdSkt);
				hKey5->ReadIntValueFromReg("MLSD", &Host.UseMLSD);
				hKey5->ReadIntValueFromReg("Noop", &Host.NoopInterval);
				hKey5->ReadIntValueFromReg("ErrMode", &Host.TransferErrorMode);
				hKey5->ReadIntValueFromReg("ErrNotify", &Host.TransferErrorNotify);
				hKey5->ReadIntValueFromReg("ErrReconnect", &Host.TransferErrorReconnect);
				hKey5->ReadIntValueFromReg("NoPasvAdrs", &Host.NoPasvAdrs);

				SetDefaultHost(&Host);
			}

			/* ホストの設定を読み込む */
			Sets = 0;
			hKey4->ReadIntValueFromReg("SetNum", &Sets);

			for(i = 0; i < Sets; i++)
			{
				sprintf(Str, "Host%d", i);
				if(auto hKey5 = hKey4->OpenSubKey(Str))
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
					hKey5->ReadIntValueFromReg("Set", &Host.Level);

					hKey5->ReadStringFromReg("HostName", Host.HostName, HOST_NAME_LEN+1);
					hKey5->ReadStringFromReg("HostAdrs", Host.HostAdrs, HOST_ADRS_LEN+1);
					hKey5->ReadStringFromReg("UserName", Host.UserName, USER_NAME_LEN+1);
					hKey5->ReadStringFromReg("Account", Host.Account, ACCOUNT_LEN+1);
					hKey5->ReadStringFromReg("LocalDir", Host.LocalInitDir, INIT_DIR_LEN+1);
					hKey5->ReadStringFromReg("RemoteDir", Host.RemoteInitDir, INIT_DIR_LEN+1);
					hKey5->ReadStringFromReg("Chmod", Host.ChmodCmd, CHMOD_CMD_LEN+1);
					hKey5->ReadStringFromReg("Nlst", Host.LsName, NLST_NAME_LEN+1);
					hKey5->ReadStringFromReg("Init", Host.InitCmd, INITCMD_LEN+1);
					hKey5->ReadIntValueFromReg("Port", &Host.Port);
					hKey5->ReadIntValueFromReg("Anonymous", &Host.Anonymous);
					hKey5->ReadIntValueFromReg("Kanji", &Host.KanjiCode);
					// 1.98b以前のUTF-8はBOMあり
					if(Version < 1983)
					{
						if(Host.KanjiCode == KANJI_UTF8N)
							Host.KanjiCode = KANJI_UTF8BOM;
					}
					hKey5->ReadIntValueFromReg("KanaCnv", &Host.KanaCnv);
					hKey5->ReadIntValueFromReg("NameKanji", &Host.NameKanjiCode);
					hKey5->ReadIntValueFromReg("NameKana", &Host.NameKanaCnv);
					hKey5->ReadIntValueFromReg("Pasv", &Host.Pasv);
					hKey5->ReadIntValueFromReg("Fwall", &Host.FireWall);
					hKey5->ReadIntValueFromReg("List", &Host.ListCmdOnly);
					hKey5->ReadIntValueFromReg("NLST-R", &Host.UseNLST_R);
					hKey5->ReadIntValueFromReg("Last", &Host.LastDir);
					hKey5->ReadIntValueFromReg("Tzone", &Host.TimeZone);
					hKey5->ReadIntValueFromReg("Type", &Host.HostType);
					hKey5->ReadIntValueFromReg("Sync", &Host.SyncMove);
					hKey5->ReadIntValueFromReg("Fpath", &Host.NoFullPath);
					hKey5->ReadBinaryFromReg("Sort", &Host.Sort, sizeof(Host.Sort));
					hKey5->ReadIntValueFromReg("Secu", &Host.Security);
					if(Host.Anonymous != YES)
					{
						strcpy(Str, "");
						hKey5->ReadStringFromReg("Password", Str, 255);
						DecodePassword(Str, Host.PassWord);
					}
					else
						strcpy(Host.PassWord, UserMailAdrs);

					if (auto result = hKey5->ReadStrings("Bmarks"sv))
						Host.BookMark = *result;

					hKey5->ReadIntValueFromReg("Dial", &Host.Dialup);
					hKey5->ReadIntValueFromReg("UseIt", &Host.DialupAlways);
					hKey5->ReadIntValueFromReg("Notify", &Host.DialupNotify);
					hKey5->ReadStringFromReg("DialTo", Host.DialEntry, RAS_NAME_LEN+1);
					// 暗号化通信対応
					hKey5->ReadIntValueFromReg("NoEncryption", &Host.UseNoEncryption);
					hKey5->ReadIntValueFromReg("FTPES", &Host.UseFTPES);
					hKey5->ReadIntValueFromReg("FTPIS", &Host.UseFTPIS);
					hKey5->ReadIntValueFromReg("SFTP", &Host.UseSFTP);
					// 同時接続対応
					hKey5->ReadIntValueFromReg("ThreadCount", &Host.MaxThreadCount);
					hKey5->ReadIntValueFromReg("ReuseCmdSkt", &Host.ReuseCmdSkt);
					// 1.98d以前で同時接続数が1より大きい場合はソケットの再利用なし
					if(Version < 1985)
					{
						if(Host.MaxThreadCount > 1)
							Host.ReuseCmdSkt = NO;
					}
					// MLSD対応
					hKey5->ReadIntValueFromReg("MLSD", &Host.UseMLSD);
					// 自動切断対策
					hKey5->ReadIntValueFromReg("Noop", &Host.NoopInterval);
					// 再転送対応
					hKey5->ReadIntValueFromReg("ErrMode", &Host.TransferErrorMode);
					hKey5->ReadIntValueFromReg("ErrNotify", &Host.TransferErrorNotify);
					// セッションあたりの転送量制限対策
					hKey5->ReadIntValueFromReg("ErrReconnect", &Host.TransferErrorReconnect);
					// ホスト側の設定ミス対策
					hKey5->ReadIntValueFromReg("NoPasvAdrs", &Host.NoPasvAdrs);

					AddHostToList(&Host, -1, Host.Level);
				}
			}

			hKey4->ReadIntValueFromReg("CurSet", &Sets);
			SetCurrentHost(Sets);

			// ファイルアイコン表示対応
			hKey4->ReadIntValueFromReg("ListIcon", &DispFileIcon);
			// タイムスタンプのバグ修正
			hKey4->ReadIntValueFromReg("ListSecond", &DispTimeSeconds);
			// ファイルの属性を数字で表示
			hKey4->ReadIntValueFromReg("ListPermitNum", &DispPermissionsNumber);
			// ディレクトリ自動作成
			hKey4->ReadIntValueFromReg("MakeDir", &MakeAllDir);
			// UTF-8対応
			hKey4->ReadIntValueFromReg("Kanji", &LocalKanjiCode);
			// UPnP対応
			hKey4->ReadIntValueFromReg("UPnP", &UPnPEnabled);
			// ローカル側自動更新
			hKey4->ReadIntValueFromReg("ListRefresh", &AutoRefreshFileList);
			// 古い処理内容を消去
			hKey4->ReadIntValueFromReg("OldLog", &RemoveOldLog);
			// ファイル一覧バグ修正
			hKey4->ReadIntValueFromReg("AbortListErr", &AbortOnListError);
			// ミラーリング設定追加
			hKey4->ReadIntValueFromReg("MirNoTransfer", &MirrorNoTransferContents);
			// FireWall設定追加
			hKey4->ReadIntValueFromReg("FwallShared", &FwallNoSaveUser);
			// ゾーンID設定追加
			hKey4->ReadIntValueFromReg("MarkDFile", &MarkAsInternet);
		}
		// 全設定暗号化対応
		EncryptSettings = NO;
	}
	else
	{
		/*===== 最初の起動時（設定が無い) =====*/
	}
	return(Sts);
}


// レジストリの設定値をクリア
void ClearRegistry() {
	SHDeleteKeyW(HKEY_CURRENT_USER, LR"(Software\Sota\FFFTP)");
}


void ClearIni() {
	fs::remove(AskIniFilePath());
}


// 設定をファイルに保存
void SaveSettingsToFile() {
	if (RegType == REGTYPE_REG) {
		if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"FFFTP.reg", L"reg", { FileType::Reg, FileType::All }); !std::empty(path)) {
			auto commandLine = strprintf(LR"("%s" EXPORT HKCU\Software\sota\FFFTP "%s")", (systemDirectory() / L"reg.exe"sv).c_str(), path.c_str());
			fs::remove(path);
			STARTUPINFOW si{ sizeof(STARTUPINFOW) };
			if (ProcessInformation pi; !CreateProcessW(nullptr, data(commandLine), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, systemDirectory().c_str(), &si, &pi))
				Message(IDS_FAIL_TO_EXEC_REDEDIT, MB_OK | MB_ICONERROR);
		}
	} else {
		if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"FFFTP-Backup.ini", L"ini", { FileType::Ini, FileType::All }); !std::empty(path))
			CopyFileW(AskIniFilePath().c_str(), path.c_str(), FALSE);
	}
}


// 設定をファイルから復元
int LoadSettingsFromFile() {
	if (auto const path = SelectFile(true, GetMainHwnd(), IDS_LOAD_SETTING, L"", L"", { FileType::Reg, FileType::Ini, FileType::All }); !std::empty(path)) {
		if (ieq(path.extension().native(), L".reg"s)) {
			auto commandLine = strprintf(LR"("%s" IMPORT "%s")", (systemDirectory() / L"reg.exe"sv).c_str(), path.c_str());
			STARTUPINFOW si{ sizeof(STARTUPINFOW), nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, 0, STARTF_USESHOWWINDOW, SW_HIDE };
			if (ProcessInformation pi; CreateProcessW(nullptr, data(commandLine), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, systemDirectory().c_str(), &si, &pi))
				return YES;
			Message(IDS_FAIL_TO_EXEC_REDEDIT, MB_OK | MB_ICONERROR);
		} else if (ieq(path.extension().native(), L".ini"s)) {
			CopyFileW(path.c_str(), AskIniFilePath().c_str(), FALSE);
			return YES;
		} else
			Message(IDS_MUST_BE_REG_OR_INI, MB_OK | MB_ICONERROR);
	}
	return NO;
}


// LOGFONTデータを文字列に変換する
static std::wstring MakeFontData(HFONT hfont, LOGFONTW const& logfont) {
	if (!hfont)
		return {};
	wchar_t buffer[1024];
	swprintf(buffer, std::size(buffer), L"%ld %ld %ld %ld %ld %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %s",
		logfont.lfHeight, logfont.lfWidth, logfont.lfEscapement, logfont.lfOrientation, logfont.lfWeight,
		logfont.lfItalic, logfont.lfUnderline, logfont.lfStrikeOut, logfont.lfCharSet,
		logfont.lfOutPrecision, logfont.lfClipPrecision, logfont.lfQuality, logfont.lfPitchAndFamily, logfont.lfFaceName
	);
	return buffer;
}


// 文字列をLOGFONTデータに変換する
static std::optional<LOGFONTW> RestoreFontData(const wchar_t* str) {
	LOGFONTW logfont;
	int offset;
	__pragma(warning(suppress:6328)) auto read = swscanf(str, L"%ld %ld %ld %ld %ld %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %n",
		&logfont.lfHeight, &logfont.lfWidth, &logfont.lfEscapement, &logfont.lfOrientation, &logfont.lfWeight,
		&logfont.lfItalic, &logfont.lfUnderline, &logfont.lfStrikeOut, &logfont.lfCharSet,
		&logfont.lfOutPrecision, &logfont.lfClipPrecision, &logfont.lfQuality, &logfont.lfPitchAndFamily, &offset);
	if (read != 13)
		return {};
	wcscpy(logfont.lfFaceName, str + offset);
	return logfont;
}

static auto AesData(std::span<UCHAR, 16> iv, std::vector<UCHAR>& text, bool encrypt) {
	return BCrypt(BCRYPT_AES_ALGORITHM, [&iv, &text, encrypt](BCRYPT_ALG_HANDLE alg) {
		auto result = false;
		NTSTATUS status;
		if (DWORD objlen, resultlen; (status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objlen), sizeof objlen, &resultlen, 0)) == STATUS_SUCCESS && resultlen == sizeof objlen) {
			if ((status = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, const_cast<PUCHAR>(reinterpret_cast<const UCHAR*>(BCRYPT_CHAIN_MODE_CBC)), sizeof BCRYPT_CHAIN_MODE_CBC, 0)) == STATUS_SUCCESS) {
				std::vector<UCHAR> obj(objlen);
				// AES用固定長キーを作成
				// SHA-1をもちいて32Byte鍵を生成する
				auto blob = HashOpen(BCRYPT_SHA1_ALGORITHM, [](auto alg, auto obj, auto hash) {
					assert(hash.size() == 20);
					struct {
						BCRYPT_KEY_DATA_BLOB_HEADER header = { BCRYPT_KEY_DATA_BLOB_MAGIC, BCRYPT_KEY_DATA_BLOB_VERSION1, 32/*うしろ8バイトは使用しない*/ };
						uint32_t data[10] = {};
					} blob;
					auto result = HashData(alg, obj, hash, std::string_view{ SecretKey }, ">g^r=@N7=//z<[`:"sv);
					assert(result);
					memcpy(blob.data, hash.data(), hash.size());
					result = HashData(alg, obj, hash, std::string_view{ SecretKey }, "VG77dO1#EyC]$|C@"sv);
					assert(result);
					memcpy(blob.data + 5, hash.data(), hash.size());
					// ビット反転の必要がある
					for (auto& x : blob.data)
						x = _byteswap_ulong(x);
					return blob;
				});
				if (BCRYPT_KEY_HANDLE key; (status = BCryptImportKey(alg, 0, BCRYPT_KEY_DATA_BLOB, &key, data(obj), size_as<ULONG>(obj), reinterpret_cast<PUCHAR>(&blob), sizeof blob, 0)) == STATUS_SUCCESS) {
					if ((status = (encrypt ? BCryptEncrypt : BCryptDecrypt)(key, data(text), size_as<ULONG>(text), nullptr, data(iv), size_as<ULONG>(iv), data(text), size_as<ULONG>(text), &resultlen, 0)) == STATUS_SUCCESS && resultlen == size_as<ULONG>(text))
						result = true;
					else
						DoPrintf(L"%s() failed: 0x%08X or bad length: %d.", encrypt ? L"BCryptEncrypt" : L"BCryptDecrypt", status, resultlen);
					BCryptDestroyKey(key);
				} else
					DoPrintf(L"BCryptImportKey() failed: 0x%08X.", status);
			} else
				DoPrintf(L"BCryptSetProperty(%s) failed: 0x%08X.", BCRYPT_CHAINING_MODE, status);
		} else
			DoPrintf(L"BCryptGetProperty(%s) failed: 0x%08X or invalid length: %d.", BCRYPT_OBJECT_LENGTH, status, resultlen);
		return result;
	});
}

// パスワードを暗号化する(AES)
static void EncodePassword(std::string_view const& Str, char* Buf) {
	auto result = BCrypt(BCRYPT_RNG_ALGORITHM, [Str, Buf](BCRYPT_ALG_HANDLE alg) {
		auto result = false;
		auto p = Buf;
		auto length = size_as<DWORD>(Str);
		auto paddedLength = (length + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE * AES_BLOCK_SIZE;
		paddedLength = std::max(paddedLength, DWORD(AES_BLOCK_SIZE * 2));	/* 最低長を32文字とする */
		std::vector<BYTE> buffer(paddedLength, 0);
		std::copy(begin(Str), end(Str), begin(buffer));
		/* PAD部分を乱数で埋める StrPad[StrLen](が有効な場合) は NUL */
		NTSTATUS status = STATUS_SUCCESS;
		if (paddedLength <= length + 1 || (status = BCryptGenRandom(alg, &buffer[(size_t)length + 1], paddedLength - length - 1, 0)) == STATUS_SUCCESS) {
			// IVの初期化
			if (std::array<UCHAR, 16> iv; (status = BCryptGenRandom(alg, data(iv), size_as<DWORD>(iv), 0)) == STATUS_SUCCESS) {
				*p++ = '0';
				*p++ = 'C';
				for (auto const& item : iv) {
					sprintf(p, "%02x", item);
					p += 2;
				}
				*p++ = ':';

				if (AesData(iv, buffer, true)) {
					for (auto const& item : buffer) {
						sprintf(p, "%02x", item);
						p += 2;
					}
					*p = NUL;
					result = true;
				}
			} else
				DoPrintf(L"BCryptGenRandom() failed: 0x%08X.", status);
		} else
			DoPrintf(L"BCryptGenRandom() failed: 0x%08X.", status);
		return result;
	});
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

static void DecodePassword3(char* Str, char* Buf) {
	Buf[0] = NUL;
	if (auto length = DWORD(strlen(Str)); AES_BLOCK_SIZE * 2 + 1 < length && Str[AES_BLOCK_SIZE * 2] == ':') {
		DWORD encodedLength = (length - 1) / 2 - AES_BLOCK_SIZE;
		std::array<UCHAR, 16> iv;
		for (auto& item : iv) {
			std::from_chars(Str, Str + 2, item, 16);
			Str += 2;
		}
		Str++;
		std::vector<UCHAR> buffer(static_cast<size_t>(encodedLength), 0);
		for (auto& item : buffer) {
			std::from_chars(Str, Str + 2, item, 16);
			Str += 2;
		}
		if (AesData(iv, buffer, false)) {
			buffer.push_back(0);	// NUL終端用に１バイト追加
			strcpy(Buf, data_as<char>(buffer));
		}
	}
}


/*===== レジストリとINIファイルのアクセス処理 ============*/

struct IniConfig : Config {
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> map;
	bool const update;
	IniConfig(std::string const& keyName, bool update) : Config{ keyName }, map{ new std::map<std::string, std::vector<std::string>>{} }, update{ update } {}
	IniConfig(std::string const& keyName, IniConfig& parent) : Config{ keyName }, map{ parent.map }, update{ false } {}
	~IniConfig() override {
		if (update) {
			std::ofstream of{ AskIniFilePath() };
			if (!of) {
				Message(IDS_CANT_SAVE_TO_INI, MB_OK | MB_ICONERROR);
				return;
			}
			of << u8(GetString(IDS_MSGJPN239));
			for (auto const& [key, lines] : *map) {
				of << "\n[" << key << "]\n";
				for (auto const& line : lines)
					of << line << "\n";
			}
		}
	}
	std::unique_ptr<Config> OpenSubKey(const char* Name) override {
		if (auto const keyName = KeyName + '\\' + Name; map->contains(keyName))
			return std::make_unique<IniConfig>(keyName, *this);
		return {};
	}
	std::unique_ptr<Config> CreateSubKey(const char* Name) override {
		return std::make_unique<IniConfig>(KeyName + '\\' + Name, *this);
	}
	const char* Scan(std::string_view name) const {
		for (auto const& line : (*map)[KeyName])
			if (size(name) + 1 < size(line) && line.starts_with(name) && line[size(name)] == '=')
				return data(line) + size(name) + 1;
		return nullptr;
	}
	bool ReadInt(std::string_view name, int& value) const override {
		if (auto const p = Scan(name)) {
			value = atoi(p);
			return true;
		}
		return false;
	}
	bool ReadValue(std::string_view name, std::string& value) const override {
		static boost::regex re{ R"(\\([0-9A-F]{2})|\\\\)" };
		if (auto const p = Scan(name)) {
			value = replace({ p }, re, [](auto const& m) { return m[1].matched ? std::stoi(m[1], nullptr, 16) : '\\'; });
			if (IniKanjiCode == KANJI_SJIS)
				value = u8(a2w(value));
			return true;
		}
		return false;
	}
	void WriteInt(std::string_view name, int value) override {
		(*map)[KeyName].push_back(std::string{ name } + '=' + std::to_string(value));
	}
	void WriteValue(std::string_view name, std::string_view value, DWORD) override {
		auto line = std::string{ name } +'=';
		for (auto it = begin(value); it != end(value); ++it)
			if (0x20 <= *it && *it < 0x7F) {
				if (*it == '\\')
					line += '\\';
				line += *it;
			} else {
				char buffer[4];
				sprintf(buffer, "\\%02X", (unsigned char)*it);
				line += buffer;
			}
		(*map)[KeyName].push_back(std::move(line));
	}
};

struct RegConfig : Config {
	HKEY hKey;
	RegConfig(std::string const& keyName, HKEY hkey) : Config{ keyName }, hKey{ hkey } {}
	~RegConfig() override {
		RegCloseKey(hKey);
	}
	std::unique_ptr<Config> OpenSubKey(const char* Name) override {
		if (HKEY key; RegOpenKeyExW(hKey, u8(Name).c_str(), 0, KEY_READ, &key) == ERROR_SUCCESS)
			return std::make_unique<RegConfig>(KeyName + '\\' + Name, key);
		return {};
	}
	std::unique_ptr<Config> CreateSubKey(const char* Name) override {
		if (HKEY key; RegCreateKeyExW(hKey, u8(Name).c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
			return std::make_unique<RegConfig>(KeyName + '\\' + Name, key);
		return {};
	}
	bool ReadInt(std::string_view name, int& value) const override {
		DWORD size = sizeof(int);
		return RegQueryValueExW(hKey, u8(name).c_str(), nullptr, nullptr, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS;
	}
	bool ReadValue(std::string_view name, std::string& value) const override {
		auto const wName = u8(name);
		if (DWORD type, count; RegQueryValueExW(hKey, wName.c_str(), nullptr, &type, nullptr, &count) == ERROR_SUCCESS) {
			if (type == REG_BINARY) {
				// TODO: EncryptSettings == YESの時、末尾に\0を含むが削除していない。
				value.resize(count);
				if (RegQueryValueExW(hKey, wName.c_str(), nullptr, nullptr, data_as<BYTE>(value), &count) == ERROR_SUCCESS)
					return true;
			} else {
				// TODO: 末尾に\0が含まれているが削除していない。
				assert(EncryptSettings != YES && (type == REG_SZ || type == REG_MULTI_SZ));
				if (std::wstring wvalue(count / sizeof(wchar_t), L'\0'); RegQueryValueExW(hKey, wName.c_str(), nullptr, nullptr, data_as<BYTE>(wvalue), &count) == ERROR_SUCCESS) {
					value = u8(wvalue);
					return true;
				}
			}
		}
		return false;
	}
	void WriteInt(std::string_view name, int value) override {
		RegSetValueExW(hKey, u8(name).c_str(), 0, REG_DWORD, reinterpret_cast<CONST BYTE*>(&value), sizeof(int));
	}
	void WriteValue(std::string_view name, std::string_view value, DWORD type) override {
		if (EncryptSettings == YES || type == REG_BINARY)
			RegSetValueExW(hKey, u8(name).c_str(), 0, REG_BINARY, data_as<const BYTE>(value), type == REG_BINARY ? size_as<DWORD>(value) : size_as<DWORD>(value) + 1);
		else {
			auto const wvalue = u8(value);
			RegSetValueExW(hKey, u8(name).c_str(), 0, type, data_as<const BYTE>(wvalue), (size_as<DWORD>(wvalue) + 1) * sizeof(wchar_t));
		}
	}
	bool DeleteSubKey(std::string_view name) override {
		return SHDeleteKeyW(hKey, u8(name).c_str()) == ERROR_SUCCESS;
	}
	void DeleteValue(std::string_view name) override {
		RegDeleteValueW(hKey, u8(name).c_str());
	}
};


// レジストリ/INIファイルをオープンする（読み込み）
static std::unique_ptr<Config> OpenReg(int type) {
	auto name = "FFFTP"s;
	if (type == REGTYPE_REG) {
		if (HKEY key; RegOpenKeyExW(HKEY_CURRENT_USER, LR"(Software\Sota\FFFTP)", 0, KEY_READ, &key) == ERROR_SUCCESS)
			return std::make_unique<RegConfig>(name, key);
	} else {
		if (std::ifstream is{ AskIniFilePath() }) {
			auto root = std::make_unique<IniConfig>(name, false);
			for (std::string line; getline(is, line);) {
				if (empty(line) || line[0] == '#')
					continue;
				if (line[0] == '[') {
					if (auto pos = line.find(']'); pos != std::string::npos)
						line.resize(pos);
					name = line.substr(1);
				} else
					(*root->map)[name].push_back(line);
			}
			return root;
		}
	}
	return {};
}


// レジストリ/INIファイルを作成する（書き込み）
static std::unique_ptr<Config> CreateReg(int type) {
	auto name = "FFFTP"s;
	if (type == REGTYPE_REG) {
		if (HKEY key; RegCreateKeyExW(HKEY_CURRENT_USER, LR"(Software\Sota\FFFTP)", 0, nullptr, 0, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
			return std::make_unique<RegConfig>(name, key);
	} else {
		return std::make_unique<IniConfig>(name, true);
	}
	return {};
}


static auto HashPassword(int stretchCount) {
	return HashOpen(BCRYPT_SHA1_ALGORITHM, [stretchCount](auto alg, auto obj, auto hash) {
		assert(hash.size() == 20);
		std::string_view password{ SecretKey, (size_t)SecretKeyLength };
		auto result = HashData(alg, obj, hash, password);
		assert(result);
		for (int j = 0; j < 5; j++)
			data_as<uint32_t>(hash)[j] = _byteswap_ulong(data_as<uint32_t>(hash)[j]);
		for (int i = 0; i < stretchCount; i++) {
			result = HashData(alg, obj, hash, hash, password);
			assert(result);
			for (int j = 0; j < 5; j++)
				data_as<uint32_t>(hash)[j] = _byteswap_ulong(data_as<uint32_t>(hash)[j]);
		}
		return *data_as<std::array<uint32_t, 5>>(hash);
	});
}

// パスワードの妥当性を確認する
static int CheckPasswordValidity(const char* HashStr, int StretchCount) {
	std::string_view HashSv{ HashStr };
	/* 空文字列は一致したことにする */
	if (empty(HashSv))
		return PASSWORD_OK;
	if (size(HashSv) != 40)
		return BAD_PASSWORD_HASH;

	/* Hashをデコードする*/
	std::array<uint32_t, 5> hash1 = {};
	for (auto& decode : hash1)
		for (int j = 0; j < 8; j++) {
			if (*HashStr < 0x40 || 0x40 + 15 < *HashStr)
				return BAD_PASSWORD_HASH;
			decode = decode << 4 | *HashStr++ & 0x0F;
		}

	/* Password をハッシュする */
	auto hash2 = HashPassword(StretchCount);
	return hash1 == hash2 ? PASSWORD_OK : PASSWORD_UNMATCH;
}

// パスワードの妥当性チェックのための文字列を作成する
static void CreatePasswordHash(char* HashStr, int StretchCount) {
	for (auto rest : HashPassword(StretchCount))
		for (int j = 0; j < 8; j++)
			*HashStr++ = 0x40 | (rest = _rotl(rest, 4)) & 0x0F;
	*HashStr = '\0';
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

void Config::Xor(std::string_view name, void* bin, DWORD len, bool preserveZero) const {
	if (EncryptSettings != YES)
		return;
	auto result = HashOpen(BCRYPT_SHA1_ALGORITHM, [bin, len, preserveZero, salt = KeyName + '\\' + name](auto alg, auto obj, auto hash) {
		assert(hash.size() == 20);
		auto p = reinterpret_cast<BYTE*>(bin);
		for (DWORD i = 0; i < len; i++) {
			if (i % 20 == 0) {
				std::array<DWORD, 16> buffer;
				for (DWORD nonce = i; auto & x : buffer)
					x = nonce = _byteswap_ulong(~nonce * 1566083941);
				if (!HashData(alg, obj, hash, buffer, salt, std::span{ SecretKey, (size_t)SecretKeyLength }))
					return false;
				memcpy(data(buffer), hash.data(), hash.size());
				for (int j = 0; j < 5; j++)
					buffer[j] ^= 0x36363636;
				for (int j = 5; j < 16; j++)
					buffer[j] = 0x36363636;
				if (!HashData(alg, obj, hash, buffer))
					return false;
				for (auto& x : buffer)
					x ^= 0x6A6A6A6A;
				if (!HashData(alg, obj, hash, buffer, hash))
					return false;
			}
			if (!preserveZero || p[i] != 0 && p[i] != hash[i % 20])
				p[i] ^= hash[i % 20];
		}
		return true;
	});
	assert(result);
}

// ポータブル版判定
int IsRegAvailable() {
	return OpenReg(REGTYPE_REG) ? YES : NO;
}

int IsIniAvailable() {
	return OpenReg(REGTYPE_INI) ? YES : NO;
}

// バージョン確認
int ReadSettingsVersion() {
	std::unique_ptr<Config> hKey3;
	if (hKey3 = OpenReg(REGTYPE_INI); !hKey3 && AskForceIni() == NO)
		hKey3 = OpenReg(REGTYPE_REG);
	int Version = INT_MAX;
	if (hKey3)
		hKey3->ReadIntValueFromReg("Version", &Version);
	return Version;
}

// FileZilla XML形式エクスポート対応
void SaveSettingsToFileZillaXml() {
	static boost::wregex unix{ LR"([^/]+)" }, dos{ LR"([^/\\]+)" };
	if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"FileZilla.xml", L"xml", { FileType::Xml,FileType::All }); !std::empty(path)) {
		if (ComPtr<IStream> stream; SHCreateStreamOnFileEx(path.c_str(), STGM_WRITE | STGM_CREATE, FILE_ATTRIBUTE_NORMAL, 0, nullptr, &stream) == S_OK)
			if (ComPtr<IXmlWriter> writer; CreateXmlWriter(IID_PPV_ARGS(&writer), nullptr) == S_OK) {
				TIME_ZONE_INFORMATION tzi;
				GetTimeZoneInformation(&tzi);
				writer->SetOutput(stream.Get());
				writer->SetProperty(XmlWriterProperty_Indent, TRUE);
				writer->WriteStartDocument(XmlStandalone_Yes);
				writer->WriteStartElement(nullptr, L"FileZilla3", nullptr);
				writer->WriteStartElement(nullptr, L"Servers", nullptr);
				int level = 0;
				HOSTDATA host;
				for (int i = 0; CopyHostFromList(i, &host) == FFFTP_SUCCESS; i++) {
					while ((host.Level & SET_LEVEL_MASK) < level--)
						writer->WriteEndElement();
					if (host.Level & SET_LEVEL_GROUP) {
						writer->WriteStartElement(nullptr, L"Folder", nullptr);
						writer->WriteAttributeString(nullptr, L"expanded", nullptr, L"1");
						writer->WriteString(u8(host.HostName).c_str());
						writer->WriteCharEntity(L'\n');
						level++;
					} else {
						writer->WriteStartElement(nullptr, L"Server", nullptr);
						writer->WriteElementString(nullptr, L"Host", nullptr, u8(host.HostAdrs).c_str());
						writer->WriteElementString(nullptr, L"Port", nullptr, std::to_wstring(host.Port).c_str());
						writer->WriteElementString(nullptr, L"Protocol", nullptr, host.UseNoEncryption == YES ? L"0" : host.UseFTPES == YES ? L"4" : host.UseFTPIS == YES ? L"3" : L"0");
						writer->WriteElementString(nullptr, L"Type", nullptr, L"0");
						writer->WriteElementString(nullptr, L"User", nullptr, u8(host.UserName).c_str());
						writer->WriteElementString(nullptr, L"Pass", nullptr, u8(host.PassWord).c_str());
						writer->WriteElementString(nullptr, L"Account", nullptr, u8(host.Account).c_str());
						writer->WriteElementString(nullptr, L"Logontype", nullptr, host.Anonymous == YES || strlen(host.UserName) == 0 ? L"0" : L"1");
						writer->WriteElementString(nullptr, L"TimezoneOffset", nullptr, std::to_wstring(tzi.Bias + host.TimeZone * 60).c_str());
						writer->WriteElementString(nullptr, L"PasvMode", nullptr, host.Pasv == YES ? L"MODE_PASSIVE" : L"MODE_ACTIVE");
						writer->WriteElementString(nullptr, L"MaximumMultipleConnections", nullptr, std::to_wstring(host.MaxThreadCount).c_str());
						switch (host.NameKanjiCode) {
						case KANJI_SJIS:
							writer->WriteElementString(nullptr, L"EncodingType", nullptr, L"Custom");
							writer->WriteElementString(nullptr, L"CustomEncoding", nullptr, L"Shift_JIS");
							break;
						case KANJI_EUC:
							writer->WriteElementString(nullptr, L"EncodingType", nullptr, L"Custom");
							writer->WriteElementString(nullptr, L"CustomEncoding", nullptr, L"EUC-JP");
							break;
						case KANJI_UTF8N:
							writer->WriteElementString(nullptr, L"EncodingType", nullptr, L"UTF-8");
							break;
						case KANJI_JIS:
						case KANJI_SMB_HEX:
						case KANJI_SMB_CAP:
						case KANJI_UTF8HFSX:
						default:
							writer->WriteElementString(nullptr, L"EncodingType", nullptr, L"Auto");
							break;
						}
						writer->WriteElementString(nullptr, L"BypassProxy", nullptr, host.FireWall == YES ? L"0" : L"1");
						writer->WriteElementString(nullptr, L"Name", nullptr, u8(host.HostName).c_str());
						writer->WriteElementString(nullptr, L"LocalDir", nullptr, u8(host.LocalInitDir).c_str());
						auto remoteDir = u8(host.RemoteInitDir);
						for (auto& [ch, prefix, re] : std::initializer_list<std::tuple<wchar_t, std::wstring_view, boost::wregex>>{ { L'/', L"1 0"sv, unix }, { L'\\', L"8 0"sv, dos } })
							if (remoteDir.find(ch) != std::wstring::npos) {
								std::wstring encoded{ prefix };
								for (boost::wcregex_iterator it{ data(remoteDir), data(remoteDir) + size(remoteDir), re }, end; it != end; ++it) {
									encoded += L' ';
									encoded += std::to_wstring(it->length(0));
									encoded += L' ';
									encoded += { (*it)[0].first, (size_t)it->length(0) };
								}
								remoteDir = encoded;
								break;
							}
						writer->WriteElementString(nullptr, L"RemoteDir", nullptr, remoteDir.c_str());
						writer->WriteElementString(nullptr, L"SyncBrowsing", nullptr, host.SyncMove == YES ? L"1" : L"0");
						writer->WriteString(u8(host.HostName).c_str());
						writer->WriteCharEntity(L'\n');
						writer->WriteEndElement();	// </Server>
					}
				}
				writer->WriteEndDocument();	// Closes any open elements or attributes, then closes the current document.
				return;
			}
		Message(IDS_FAIL_TO_EXPORT, MB_OK | MB_ICONERROR);
	}
}

void SaveSettingsToWinSCPIni() {
	auto escape = [](std::string_view str) {
		std::string result;
		if (std::ranges::any_of(str, [](auto ch) { return ch & 0x80; }))
			result += "%EF%BB%BF"sv;
		for (auto ch : str)
			if ((ch & 0x80) || "\t\n\r %*?\\"sv.find(ch) != std::string_view::npos)
				result += strprintf<3>("%%%02hhX", ch);
			else
				result += ch;
		return result;
	};
	auto encode = [](std::string_view user, std::string_view host, std::string_view password) {
		auto str = concat(user, host, password);
		static_assert((char)~0xA3 == 0x5C);
		auto result = strprintf<8>("A35C%02hhX5C", size_as<unsigned char>(str) ^ 0x5C);
		for (auto ch : str)
			result += strprintf<2>("%02hhX", ch ^ 0x5C);
		return result;
	};
	Message(IDS_NEED_EXSITING_WINSCP_INI, MB_OK);
	if (auto const path = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"WinSCP.ini", L"ini", { FileType::Ini, FileType::All }); !path.empty()) {
		if (std::ofstream f{ path, std::ofstream::in | std::ofstream::ate }) {
			std::vector<std::string> names;
			HOSTDATA Host;
			for (int i = 0; CopyHostFromList(i, &Host) == FFFTP_SUCCESS; i++) {
				assert((Host.Level & SET_LEVEL_MASK) <= size_as<int>(names));
				names.resize(Host.Level & SET_LEVEL_MASK);
				if (Host.Level & SET_LEVEL_GROUP) {
					names.push_back(Host.HostName);
					continue;
				}
				std::string path;
				for (auto name : names) {
					path += name;
					path += '/';
				}
				f << "[Sessions\\"sv << escape(path + Host.HostName) << "]\n"sv;
				f << "HostName="sv << escape(Host.HostAdrs) << '\n';
				f << "PortNumber="sv << Host.Port << '\n';
				f << "UserName="sv << escape(Host.UserName) << '\n';
				f << "FSProtocol=5\n"sv;
				f << "LocalDirectory="sv << escape(Host.LocalInitDir) << '\n';
				f << "RemoteDirectory="sv << escape(Host.RemoteInitDir) << '\n';
				f << "SynchronizeBrowsing="sv << (Host.SyncMove == YES ? 1 : 0) << '\n';
				f << "PostLoginCommands="sv << escape(Host.InitCmd) << '\n';
				if (Host.FireWall == YES) {
					if (auto method = FwallType == FWALL_SOCKS4 ? 1 : FwallType == FWALL_SOCKS5_USER ? 2 : -1; method != -1)
						f << "ProxyMethod="sv << method << '\n';
					f << "ProxyHost="sv << escape(FwallHost) << '\n';
					f << "ProxyPort="sv << FwallPort << '\n';
					f << "ProxyUsername="sv << escape(FwallUser) << '\n';
				}
				if (auto utf = Host.NameKanjiCode == KANJI_SJIS ? 0 : Host.NameKanjiCode == KANJI_UTF8N ? 1 : -1; utf != -1)
					f << "Utf="sv << utf << '\n';
				f << "FtpPasvMode="sv << (Host.Pasv == YES ? 1 : 0) << '\n';
				if (Host.ListCmdOnly == YES && Host.UseMLSD == NO)
					f << "FtpUseMlsd=0\n"sv;
				f << "FtpAccount="sv << escape(Host.Account) << '\n';
				if (Host.NoopInterval > 0)
					f << "FtpPingInterval="sv << Host.NoopInterval << '\n';
				else
					f << "FtpPingType=0\n"sv;
				f << "Ftps="sv << (Host.UseNoEncryption == YES ? 0 : Host.UseFTPES == YES ? 3 : Host.UseFTPIS == YES ? 1 : 0) << '\n';
				if (Host.FireWall == YES)
					if (auto type = FwallType == FWALL_FU_FP_SITE ? 1 : FwallType == FWALL_FU_FP_USER ? 2 : FwallType == FWALL_USER ? 5 : FwallType == FWALL_OPEN ? 3 : -1; type != -1)
						f << "FtpProxyLogonType="sv << type << '\n';
				f << "Password="sv << encode(Host.UserName, Host.HostAdrs, Host.PassWord) << '\n';
				if (Host.FireWall == YES)
					f << "ProxyPasswordEnc="sv << encode(FwallUser, FwallHost, FwallPass) << '\n';
				f << '\n';
			}
		} else
			Message(IDS_FAIL_TO_EXPORT, MB_OK | MB_ICONERROR);
	}
}
