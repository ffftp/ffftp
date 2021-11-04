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
static int EncryptSettings = NO;

static inline auto a2w(std::string_view text) {
	return convert<wchar_t>([](auto src, auto srclen, auto dst, auto dstlen) { return MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, src, srclen, dst, dstlen); }, text);
}

class Config {
	void Xor(std::string_view name, void* bin, DWORD len, bool preserveZero) const;
	std::optional<std::string> ReadStringCore(std::string_view name) const {
		if (std::string value; ReadStringImpl(name, value)) {
			Xor(name, data(value), size_as<DWORD>(value), true);
			if (auto const pos = value.find_last_not_of('\0'); pos != std::string::npos)
				value.erase(pos + 1);
			return std::move(value);
		}
		return {};
	}
	void WriteStringCore(std::string_view name, std::string&& value) {
		Xor(name, data(value), size_as<DWORD>(value), true);
		WriteStringImpl(name, value, REG_SZ);
	}
protected:
	const std::string KeyName;
	Config(std::string const& keyName) : KeyName{ keyName } {}
	virtual bool ReadIntImpl(std::string_view name, int& value) const = 0;
	virtual bool ReadStringImpl(std::string_view name, std::string& value) const = 0;
	virtual void WriteIntImpl(std::string_view name, int value) = 0;
	virtual void WriteStringImpl(std::string_view name, std::string_view value, DWORD type) = 0;
public:
	Config(Config const&) = delete;
	virtual ~Config() = default;
	virtual std::unique_ptr<Config> OpenSubKey(std::string_view name) = 0;
	virtual std::unique_ptr<Config> CreateSubKey(std::string_view name) = 0;

	bool ReadValue(std::string_view name, int& value) const {
		if (ReadIntImpl(name, value)) {
			Xor(name, &value, sizeof(int), false);
			return true;
		}
		return false;
	}
	template<std::integral Integral>
	bool ReadValue(std::string_view name, Integral& value) const {
		static_assert(sizeof(Integral) <= sizeof(int));
		if (int temp; ReadValue(name, temp)) {
			value = gsl::narrow_cast<Integral>(temp);
			return true;
		}
		return false;
	}
	void WriteValue(std::string_view name, int value) {
		Xor(name, &value, sizeof(int), false);
		WriteIntImpl(name, value);
	}
	template<std::integral Integral>
	void WriteValue(std::string_view name, Integral value) {
		WriteValue(name, static_cast<int>(value));
	}
	template<std::integral Integral>
	void WriteValueIf(std::string_view name, Integral value, Integral defaultValue) {
		if (value == defaultValue)
			DeleteValue(name);
		else
			WriteValue(name, value);
	}

	bool ReadValue(std::string_view name, std::string& str) const {
		if (auto const value = ReadStringCore(name)) {
			str = *value;
			return true;
		}
		return false;
	}
	bool ReadValue(std::string_view name, std::wstring& str) const {
		if (auto const value = ReadStringCore(name)) {
			str = u8(*value);
			return true;
		}
		return false;
	}
	void WriteValue(std::string_view name, std::string_view str) {
		WriteStringCore(name, std::string{ str });
	}
	void WriteValue(std::string_view name, std::wstring_view str) {
		WriteStringCore(name, u8(str));
	}
	void WriteValueIf(std::string_view name, std::wstring_view str, std::wstring_view defaultStr) {
		if (str == defaultStr)
			DeleteValue(name);
		else
			WriteValue(name, str);
	}
	bool ReadPassword(std::string_view name, std::wstring& password) const;
	void WritePassword(std::string_view name, std::wstring_view password);
	bool ReadValue(std::string_view name, std::vector<std::wstring>& strings) const {
		if (std::string value; ReadStringImpl(name, value)) {
			strings.clear();
			if (!empty(value) && value[0] != '\0') {
				Xor(name, data(value), size_as<DWORD>(value), true);
				static boost::regex re{ R"([^\0]+)" };
				for (boost::sregex_iterator it{ value.begin(), value.end(), re }, end; it != end; ++it)
					strings.push_back(u8(sv((*it)[0])));
			}
			return true;
		}
		return false;
	}
	void WriteValue(std::string_view name, std::vector<std::wstring> const& strings) {
		std::string value;
		for (auto const& string : strings) {
			value += u8(string);
			value += '\0';
		}
		Xor(name, data(value), size_as<DWORD>(value), true);
		WriteStringImpl(name, value, REG_MULTI_SZ);
	}
	bool ReadBinary(std::string_view name, auto& object) const {
		if (std::string value; ReadStringImpl(name, value)) {
			Xor(name, data(value), size_as<DWORD>(value), false);
			std::copy_n(begin(value), std::min(size(value), sizeof object), reinterpret_cast<char*>(std::addressof(object)));
			return true;
		}
		return false;
	}
	void WriteBinary(std::string_view name, auto const& object) {
		std::string value(reinterpret_cast<const char*>(std::addressof(object)), sizeof object);
		Xor(name, data(value), size_as<DWORD>(value), false);
		WriteStringImpl(name, value, REG_BINARY);
	}
	virtual bool DeleteSubKey(std::string_view name) {
		return false;
	}
	virtual void DeleteValue(std::string_view name) {
	}
	bool ReadFont(std::string_view name, HFONT& hfont, LOGFONTW& logfont) {
		if (std::wstring value; ReadValue(name, value)) {
			int offset;
			auto read = swscanf_s(value.c_str(), L"%ld %ld %ld %ld %ld %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %n",
				&logfont.lfHeight, &logfont.lfWidth, &logfont.lfEscapement, &logfont.lfOrientation, &logfont.lfWeight,
				&logfont.lfItalic, &logfont.lfUnderline, &logfont.lfStrikeOut, &logfont.lfCharSet,
				&logfont.lfOutPrecision, &logfont.lfClipPrecision, &logfont.lfQuality, &logfont.lfPitchAndFamily, &offset
			);
			if (read == 13) {
				wcscpy_s(logfont.lfFaceName, value.c_str() + offset);
				hfont = CreateFontIndirectW(&logfont);
				return true;
			}
		}
		logfont = {};
		return false;
	}
	void WriteFont(std::string_view name, HFONT hfont, LOGFONTW const& logfont) {
		std::wstring value;
		if (hfont)
			value = std::format(L"{} {} {} {} {} {} {} {} {} {} {} {} {} {}"sv,
				logfont.lfHeight, logfont.lfWidth, logfont.lfEscapement, logfont.lfOrientation, logfont.lfWeight,
				logfont.lfItalic, logfont.lfUnderline, logfont.lfStrikeOut, logfont.lfCharSet,
				logfont.lfOutPrecision, logfont.lfClipPrecision, logfont.lfQuality, logfont.lfPitchAndFamily, logfont.lfFaceName
			);
		WriteValue(name, value);
	}
	void ReadHost(Host& host, int version, bool readPassword);
	void WriteHost(Host const& host, Host const& defaultHost, bool writePassword);
};

static std::unique_ptr<Config> OpenReg(int type);
static std::unique_ptr<Config> CreateReg(int type);
static int CheckPasswordValidity(std::string_view HashSv, int StretchCount);
static std::string CreatePasswordHash(int stretchCount);
void SetHashSalt(DWORD salt);
void SetHashSalt1(void* Salt, int Length);

static char SecretKey[FMAX_PATH+1];
static int SecretKeyLength;
static int IsMasterPasswordError = PASSWORD_OK;
static int IsRndSourceInit = 0;
static uint32_t RndSource[9];
static int IniKanjiCode = KANJI_NOCNV;
static int EncryptSettingsError = NO;


// マスタパスワードの設定
void SetMasterPassword(std::wstring_view password) {
	ZeroMemory(SecretKey, MAX_PASSWORD_LEN + 12);
	strncpy_s(SecretKey, empty(password) ? DEFAULT_PASSWORD : u8(password).c_str(), MAX_PASSWORD_LEN);
	SecretKeyLength = (int)strlen(SecretKey);

	/* 未検証なので，初期状態はOKにする (強制再設定→保存にを可能にする)*/
	IsMasterPasswordError = PASSWORD_OK;
}

std::wstring GetMasterPassword() {
	return u8(SecretKey);
}

// マスタパスワードの状態取得
//   PASSWORD_OK : OK
//   PASSWORD_UNMATCH : パスワード不一致
//   BAD_PASSWORD_HASH : パスワード確認失敗
int GetMasterPasswordStatus() {
	return IsMasterPasswordError;
}

// レジストリ／INIファイルのマスターパスワードの検証を行う
int ValidateMasterPassword() {
	std::unique_ptr<Config> hKey3;
	if (hKey3 = OpenReg(REGTYPE_INI); !hKey3 && AskForceIni() == NO)
		hKey3 = OpenReg(REGTYPE_REG);
	if (hKey3) {
		if (std::string checkbuf; hKey3->ReadValue("CredentialCheck1"sv, checkbuf)) {
			if (unsigned char salt1[16]; hKey3->ReadBinary("CredentialSalt1"sv, salt1))
				SetHashSalt1(&salt1, 16);
			else
				SetHashSalt1(NULL, 0);
			int stretch = 0;
			hKey3->ReadValue("CredentialStretch", stretch);
			IsMasterPasswordError = CheckPasswordValidity(checkbuf, stretch);
		} else if(hKey3->ReadValue("CredentialCheck"sv, checkbuf)) {
			if (int salt = 0; hKey3->ReadValue("CredentialSalt", salt))
				SetHashSalt(salt);
			else
				SetHashSalt1(NULL, 0);
			IsMasterPasswordError = CheckPasswordValidity(checkbuf, 0);
		}
		return YES;
	}
	return NO;
}

static constexpr std::tuple<std::string_view, std::variant<int Host::*, std::wstring Host::*>> hostSettings[] = {
	{ "HostAdrs"sv, &Host::HostAdrs },
	{ "UserName"sv, &Host::UserName },
	{ "Account"sv, &Host::Account },
	{ "RemoteDir"sv, &Host::RemoteInitDir },
	{ "Chmod"sv, &Host::ChmodCmd },
	{ "Nlst"sv, &Host::LsName },
	{ "Init"sv, &Host::InitCmd },
	{ "Port"sv, &Host::Port },
	{ "Kanji"sv, &Host::KanjiCode },
	{ "KanaCnv"sv, &Host::KanaCnv },
	{ "NameKanji"sv, &Host::NameKanjiCode },
	{ "NameKana"sv, &Host::NameKanaCnv },
	{ "Pasv"sv, &Host::Pasv },
	{ "Fwall"sv, &Host::FireWall },
	{ "List"sv, &Host::ListCmdOnly },
	{ "NLST-R"sv, &Host::UseNLST_R },
	{ "Tzone"sv, &Host::TimeZone },
	{ "Type"sv, &Host::HostType },
	{ "Sync"sv, &Host::SyncMove },
	{ "Fpath"sv, &Host::NoFullPath },
	{ "Secu"sv, &Host::Security },
	{ "Dial"sv, &Host::Dialup },
	{ "UseIt"sv, &Host::DialupAlways },
	{ "Notify"sv, &Host::DialupNotify },
	{ "DialTo"sv, &Host::DialEntry },
	{ "NoEncryption"sv, &Host::UseNoEncryption },
	{ "FTPES"sv, &Host::UseFTPES },
	{ "FTPIS"sv, &Host::UseFTPIS },
	{ "SFTP"sv, &Host::UseSFTP },
	{ "ThreadCount"sv, &Host::MaxThreadCount },
	{ "ReuseCmdSkt"sv, &Host::ReuseCmdSkt },
	{ "MLSD"sv, &Host::UseMLSD },
	{ "Noop"sv, &Host::NoopInterval },
	{ "ErrMode"sv, &Host::TransferErrorMode },
	{ "ErrNotify"sv, &Host::TransferErrorNotify },
	{ "ErrReconnect"sv, &Host::TransferErrorReconnect },
	{ "NoPasvAdrs"sv, &Host::NoPasvAdrs },
};

void Config::ReadHost(Host& host, int version, bool readPassword) {
	for (auto& [name, variant] : hostSettings)
		std::visit([this, name, &host](auto&& ptr) { ReadValue(name, host.*ptr); }, variant);
	// 1.98b以前のUTF-8はBOMあり
	if (version < 1983 && host.KanjiCode == KANJI_UTF8N)
		host.KanjiCode = KANJI_UTF8BOM;
	ReadValue("LocalDir"sv, host.LocalInitDir);
	ReadBinary("Sort"sv, host.Sort);
	if (readPassword)
		ReadPassword("Password"sv, host.PassWord);
	else
		host.PassWord = UserMailAdrs;
	// 1.98d以前で同時接続数が1より大きい場合はソケットの再利用なし
	if (version < 1985 && 1 < host.MaxThreadCount)
		host.ReuseCmdSkt = NO;
}

void Config::WriteHost(Host const& host, Host const& defaultHost, bool writePassword) {
	for (auto& [name, variant] : hostSettings)
		std::visit([this, name, &host, &defaultHost](auto&& ptr) { WriteValueIf(name, host.*ptr, defaultHost.*ptr); }, variant);
	WriteValue("LocalDir"sv, host.LocalInitDir);
	if (writePassword)
		WritePassword("Password"sv, host.PassWord);
	else
		DeleteValue("Password"sv);
	WriteBinary("Sort"sv, host.Sort);
}

static constexpr std::tuple<std::string_view, std::variant<int*, uint8_t*, std::wstring*, std::vector<std::wstring>*>> settings[] = {
	{ "UserMail"sv, &UserMailAdrs },
	{ "Viewer"sv, &ViewerName[0] },
	{ "Viewer2"sv, &ViewerName[1] },
	{ "Viewer3"sv, &ViewerName[2] },
	{ "TrType"sv, &TransMode },
	{ "Recv"sv, &RecvMode },
	{ "Send"sv, &SendMode },
	{ "Move"sv, &MoveMode },
	{ "Path"sv, &DefaultLocalPath },
	{ "Time"sv, &SaveTimeStamp },
	{ "EOF"sv, &RmEOF },
	{ "Scolon"sv, &VaxSemicolon },
	{ "RecvEx"sv, &ExistMode },
	{ "SendEx"sv, &UpExistMode },
	{ "LFsort"sv, &Sort.LocalFile },
	{ "LDsort"sv, &Sort.LocalDirectory },
	{ "RFsort"sv, &Sort.RemoteFile },
	{ "RDsort"sv, &Sort.RemoteDirectory },
	{ "SortSave"sv, &SortSave },
	{ "ListType"sv, &ListType },
	{ "DotFile"sv, &DotFile },
	{ "Dclick"sv, &DclickOpen },
	{ "ConS"sv, &ConnectOnStart },
	{ "OldDlg"sv, &ConnectAndSet },
	{ "RasClose"sv, &RasClose },
	{ "RasNotify"sv, &RasCloseNotify },
	{ "Qanony"sv, &QuickAnonymous },
	{ "PassHist"sv, &PassToHist },
	{ "SendQuit"sv, &SendQuit },
	{ "NoRas"sv, &NoRasControl },
	{ "Debug"sv, &DebugConsole },
	{ "WinPos"sv, &SaveWinPos },
	{ "RegExp"sv, &FindMode },
	{ "Reg"sv, &RegType },
	{ "LowUp"sv, &FnameCnv },
	{ "Tout"sv, &TimeOut },
	{ "NoTrn"sv, &MirrorNoTrn },
	{ "NoDel"sv, &MirrorNoDel },
	{ "MirFile"sv, &MirrorFnameCnv },
	{ "MirUNot"sv, &MirUpDelNotify },
	{ "MirDNot"sv, &MirDownDelNotify },
	{ "ListHide"sv, &DispIgnoreHide },
	{ "ListDrv"sv, &DispDrives },
	{ "FwallHost"sv, &FwallHost },
	{ "FwallPort"sv, &FwallPort },
	{ "FwallType"sv, &FwallType },
	{ "FwallDef"sv, &FwallDefault },
	{ "FwallSec"sv, &FwallSecurity },
	{ "PasvDef"sv, &PasvDefault },
	{ "FwallRes"sv, &FwallResolve },
	{ "FwallLow"sv, &FwallLower },
	{ "FwallDel"sv, &FwallDelimiter },
	{ "DefAttr"sv, &DefAttrList },
	{ "FAttrSw"sv, &FolderAttr },
	{ "FAttr"sv, &FolderAttrNum },
	{ "HistNum"sv, &FileHist },
	{ "ListIcon"sv, &DispFileIcon },
	{ "ListSecond"sv, &DispTimeSeconds },
	{ "ListPermitNum"sv, &DispPermissionsNumber },
	{ "MakeDir"sv, &MakeAllDir },
	{ "Kanji"sv, &LocalKanjiCode },
	{ "UPnP"sv, &UPnPEnabled },
	{ "ListRefresh"sv, &AutoRefreshFileList },
	{ "OldLog"sv, &RemoveOldLog },
	{ "AbortListErr"sv, &AbortOnListError },
	{ "MirNoTransfer"sv, &MirrorNoTransferContents },
	{ "FwallShared"sv, &FwallNoSaveUser },
	{ "MarkDFile"sv, &MarkAsInternet },
};

// レジストリ／INIファイルに設定値を保存
void SaveRegistry() {
	/* 2010.01.30 genta: マスターパスワードが不一致の場合は不用意に上書きしない */
	if (GetMasterPasswordStatus() == PASSWORD_UNMATCH)
		return;

	if (EncryptSettingsError == YES)
		return;

	if (ReadOnlySettings == YES)
		return;

	auto hKey3 = CreateReg(RegType);
	if (!hKey3)
		return;

	hKey3->WriteValue("Version", VER_NUM);
	union {
		unsigned char salt1[16];
		int salt;
	} u;
	auto result = BCrypt(BCRYPT_RNG_ALGORITHM, [&arr = u.salt1](BCRYPT_ALG_HANDLE alg) {
		auto const status = BCryptGenRandom(alg, arr, size_as<ULONG>(arr), 0);
		if (status != STATUS_SUCCESS)
			Debug(L"BCryptGenRandom() failed: 0x{:08X}.", status);
		return status == STATUS_SUCCESS;
	});
	if (!result)
		return;
	if (EncryptAllSettings == YES) {
		SetHashSalt1(&u.salt1, 16);
		hKey3->WriteBinary("CredentialSalt1"sv, u.salt1);
		hKey3->WriteValue("CredentialStretch", 65535);
		hKey3->WriteValue("CredentialCheck1", CreatePasswordHash(65535));
	} else {
		SetHashSalt(u.salt);
		hKey3->WriteValue("CredentialSalt", u.salt);
		hKey3->WriteValue("CredentialCheck", CreatePasswordHash(0));
	}

	hKey3->WriteValue("EncryptAll", EncryptAllSettings);
	hKey3->WritePassword("EncryptAllDetector", std::to_wstring(EncryptAllSettings));
	EncryptSettings = EncryptAllSettings;

	if (auto hKey4 = hKey3->CreateSubKey(EncryptAllSettings == YES ? "EncryptedOptions"sv : "Options"sv)) {
		hKey4->WriteValue("NoSave", SuppressSave);

		if (SuppressSave != YES) {
			for (auto& [name, variant] : settings)
				std::visit([&hKey4, name](auto&& ptr) { hKey4->WriteValue(name, *ptr); }, variant);

			hKey4->WriteBinary("LocalColm"sv, LocalTabWidth);
			hKey4->WriteBinary("RemoteColm"sv, RemoteTabWidth);
			hKey4->WriteValue("AsciiFile"sv, AsciiExt);
			hKey4->WriteFont("ListFont"sv, ListFont, ListLogFont);
			hKey4->WriteValue("FwallUser"sv, FwallNoSaveUser == YES ? L""sv : FwallUser);
			hKey4->WritePassword("FwallPass"sv, FwallNoSaveUser == YES ? L""sv : FwallPass);
			hKey4->WriteBinary("Hdlg"sv, HostDlgSize);
			hKey4->WriteBinary("Bdlg"sv, BmarkDlgSize);
			hKey4->WriteBinary("Mdlg"sv, MirrorDlgSize);
			/* Ver1.54a以前の形式のヒストリデータは削除 */
			hKey4->DeleteValue("Hist");

			/* ヒストリの設定を保存 */
			HISTORYDATA DefaultHist;
			int i = 0;
			for (auto const& history : GetHistories())
				if (auto hKey5 = hKey4->CreateSubKey(std::format("History{}"sv, i))) {
					hKey5->WriteHost(history, DefaultHist, true);
					hKey5->WriteValue("TrType", history.Type);
					i++;
				}
			hKey4->WriteValue("SavedHist", i);

			/* 余分なヒストリがあったら削除 */
			while (hKey4->DeleteSubKey(std::format("History{}"sv, i)))
				i++;

			// ホスト共通設定機能
			HOSTDATA DefaultHost;
			if (auto hKey5 = hKey4->CreateSubKey("DefaultHost")) {
				HOSTDATA Host;
				CopyDefaultHost(&Host);
				hKey5->WriteValue("Set", Host.Level);
				hKey5->WriteValueIf("HostName"sv, Host.HostName, DefaultHost.HostName);
				hKey5->WriteHost(Host, DefaultHost, Host.Anonymous == NO);
				hKey5->WriteValueIf("Anonymous", Host.Anonymous, DefaultHost.Anonymous);
				hKey5->WriteValueIf("Last", Host.LastDir, DefaultHost.LastDir);
				hKey5->WriteValue("Bmarks"sv, Host.BookMark);
			}

			/* ホストの設定を保存 */
			CopyDefaultHost(&DefaultHost);
			i = 0;
			for (HOSTDATA Host; CopyHostFromList(i, &Host) == FFFTP_SUCCESS; i++)
				if (auto hKey5 = hKey4->CreateSubKey(std::format("Host{}"sv, i))) {
					hKey5->WriteValue("Set", Host.Level);
					hKey5->WriteValueIf("HostName"sv, Host.HostName, DefaultHost.HostName);
					if ((Host.Level & SET_LEVEL_GROUP) == 0) {
						hKey5->WriteHost(Host, DefaultHost, Host.Anonymous == NO);
						hKey5->WriteValueIf("Anonymous", Host.Anonymous, DefaultHost.Anonymous);
						hKey5->WriteValueIf("Last", Host.LastDir, DefaultHost.LastDir);
						hKey5->WriteValue("Bmarks"sv, Host.BookMark);
					}
				}
			hKey4->WriteValue("SetNum", i);

			/* 余分なホストの設定があったら削除 */
			while (hKey4->DeleteSubKey(std::format("Host{}"sv, i)))
				i++;

			if ((i = AskCurrentHost()) == HOSTNUM_NOENTRY)
				i = 0;
			hKey4->WriteValue("CurSet", i);
		}
	}

	EncryptSettings = NO;
	if (EncryptAllSettings == YES) {
		hKey3->DeleteSubKey("Options");
		hKey3->DeleteValue("CredentialSalt");
		hKey3->DeleteValue("CredentialCheck");
	} else {
		hKey3->DeleteSubKey("EncryptedOptions");
		hKey3->DeleteValue("CredentialSalt1");
		hKey3->DeleteValue("CredentialStretch");
		hKey3->DeleteValue("CredentialCheck1");
	}
}

// レジストリ／INIファイルから設定値を呼び出す
//   この関数を複数回呼び出すと，ホスト設定が追加される．
bool LoadRegistry() {
	struct Data {
		using result_t = int;
		static void OnCommand(HWND hDlg, WORD cmd, WORD id) {
			if (cmd == BN_CLICKED)
				EndDialog(hDlg, id);
		}
	};

	auto hKey3 = OpenReg(REGTYPE_INI);
	if (!hKey3 && AskForceIni() == NO)
		hKey3 = OpenReg(REGTYPE_REG);
	if (!hKey3)
		return false;

	int Version;
	hKey3->ReadValue("Version", Version);

	if (Version < 1980)
		IniKanjiCode = KANJI_SJIS;

	if (1990 <= Version && GetMasterPasswordStatus() == PASSWORD_OK) {
		hKey3->ReadValue("EncryptAll", EncryptAllSettings);
		std::wstring encryptAllDetector;
		hKey3->ReadPassword("EncryptAllDetector"sv, encryptAllDetector);
		EncryptSettings = EncryptAllSettings;
		if (std::to_wstring(EncryptAllSettings) != encryptAllDetector)
			switch (Dialog(GetFtpInst(), corruptsettings_dlg, GetMainHwnd(), Data{})) {
			case IDABORT:
				ClearRegistry();
				ClearIni();
				Restart();
				[[fallthrough]];
			case IDCANCEL:
				Terminate();
				break;
			case IDRETRY:
				EncryptSettingsError = YES;
				break;
			case IDIGNORE:
				break;
			}
	}

	if (auto hKey4 = hKey3->OpenSubKey(EncryptAllSettings == YES? "EncryptedOptions"sv : "Options"sv)) {
		for (auto& [name, variant] : settings)
			std::visit([&hKey4, name](auto&& ptr) { hKey4->ReadValue(name, *ptr); }, variant);

		if (5600 <= Version) {		// HighDPI廃止のため、古いサイズは読み込まない。
			hKey4->ReadValue("WinPosX"sv, WinPosX);
			hKey4->ReadValue("WinPosY"sv, WinPosY);
			hKey4->ReadValue("WinWidth"sv, WinWidth);
			hKey4->ReadValue("WinHeight"sv, WinHeight);
			hKey4->ReadValue("LocalWidth"sv, LocalWidth);
			/* ↓旧バージョンのバグ対策 */
			LocalWidth = std::max(0, LocalWidth);
			hKey4->ReadValue("TaskHeight"sv, TaskHeight);
			/* ↓旧バージョンのバグ対策 */
			TaskHeight = std::max(0, TaskHeight);
			hKey4->ReadBinary("LocalColm"sv, LocalTabWidth);
			if (std::all_of(std::begin(LocalTabWidth), std::end(LocalTabWidth), [](auto width) { return width <= 0; }))
				std::copy(std::begin(LocalTabWidthDefault), std::end(LocalTabWidthDefault), std::begin(LocalTabWidth));
			hKey4->ReadBinary("RemoteColm"sv, RemoteTabWidth);
			if (std::all_of(std::begin(RemoteTabWidth), std::end(RemoteTabWidth), [](auto width) { return width <= 0; }))
				std::copy(std::begin(RemoteTabWidthDefault), std::end(RemoteTabWidthDefault), std::begin(RemoteTabWidth));
			hKey4->ReadValue("SwCmd"sv, Sizing);
			hKey4->ReadBinary("Hdlg"sv, HostDlgSize);
			hKey4->ReadBinary("Bdlg"sv, BmarkDlgSize);
			hKey4->ReadBinary("Mdlg"sv, MirrorDlgSize);
		}

		if (!hKey4->ReadValue("AsciiFile"sv, AsciiExt))
			if (std::wstring value; hKey4->ReadValue ("Ascii"sv, value)) {
				/* 旧ASCIIモードの拡張子の設定を新しいものに変換 */
				static boost::wregex re{ LR"([^;]+)" };
				AsciiExt.clear();
				for (boost::wsregex_iterator it{ value.begin(), value.end(), re }, end; it != end; ++it)
					AsciiExt.push_back(L"*."sv + it->str());
			}
		if (Version < 1986) {
			// アスキーモード判別の改良
			for (auto item : { L"*.js"sv, L"*.vbs"sv, L"*.css"sv, L"*.rss"sv, L"*.rdf"sv, L"*.xml"sv, L"*.xhtml"sv, L"*.xht"sv, L"*.shtml"sv, L"*.shtm"sv, L"*.sh"sv, L"*.py"sv, L"*.rb"sv, L"*.properties"sv, L"*.sql"sv, L"*.asp"sv, L"*.aspx"sv, L"*.php"sv, L"*.htaccess"sv })
				if (std::ranges::find(AsciiExt, item) == AsciiExt.end())
					AsciiExt.emplace_back(item);
		}

		hKey4->ReadFont("ListFont"sv, ListFont, ListLogFont);
		hKey4->ReadValue("FwallUser"sv, FwallUser);
		hKey4->ReadPassword("FwallPass"sv, FwallPass);
		hKey4->ReadValue("NoSave", SuppressSave);

		/* ヒストリの設定を読み込む */
		int Sets = 0;
		hKey4->ReadValue("SavedHist", Sets);
		for (int i = 0; i < Sets; i++)
			if (auto hKey5 = hKey4->OpenSubKey(std::format("History{}"sv, i))) {
				HISTORYDATA Hist;
				hKey5->ReadHost(Hist, Version, true);
				hKey5->ReadValue("TrType", Hist.Type);
				AddHistoryToHistory(Hist);
			}

		// ホスト共通設定機能
		HOSTDATA Host;
		if (auto hKey5 = hKey4->OpenSubKey("DefaultHost")) {
			hKey5->ReadValue("Set", Host.Level);
			hKey5->ReadValue("HostName"sv, Host.HostName);
			hKey5->ReadValue("Anonymous", Host.Anonymous);
			hKey5->ReadHost(Host, Version, Host.Anonymous != YES);
			hKey5->ReadValue("Bmarks"sv, Host.BookMark);
			SetDefaultHost(&Host);
		}

		/* ホストの設定を読み込む */
		Sets = 0;
		hKey4->ReadValue("SetNum", Sets);
		for (int i = 0; i < Sets; i++)
			if (auto hKey5 = hKey4->OpenSubKey(std::format("Host{}"sv, i))) {
				CopyDefaultHost(&Host);
				if (Version < 1921) {
					Host.Pasv = NO;
					Host.ListCmdOnly = NO;
				}
				// 1.97b以前はデフォルトでShift_JIS
				if (Version < 1980)
					Host.NameKanjiCode = KANJI_SJIS;
				hKey5->ReadValue("Set", Host.Level);
				hKey5->ReadValue("HostName"sv, Host.HostName);
				hKey5->ReadValue("Anonymous", Host.Anonymous);
				hKey5->ReadHost(Host, Version, Host.Anonymous != YES);
				hKey5->ReadValue("Last", Host.LastDir);
				hKey5->ReadValue("Bmarks"sv, Host.BookMark);
				AddHostToList(&Host, -1, Host.Level);
			}

		hKey4->ReadValue("CurSet", Sets);
		SetCurrentHost(Sets);
	}
	EncryptSettings = NO;
	return true;
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
			auto commandLine = std::format(LR"("{}" EXPORT HKCU\Software\sota\FFFTP "{}")"sv, (systemDirectory() / L"reg.exe"sv).native(), path.native());
			fs::remove(path);
			STARTUPINFOW si{ sizeof(STARTUPINFOW) };
			if (ProcessInformation pi; __pragma(warning(suppress:6335)) !CreateProcessW(nullptr, data(commandLine), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, systemDirectory().c_str(), &si, &pi))
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
			auto commandLine = std::format(LR"("{}" IMPORT "{}")"sv, (systemDirectory() / L"reg.exe"sv).native(), path.native());
			STARTUPINFOW si{ sizeof(STARTUPINFOW), nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, 0, STARTF_USESHOWWINDOW, SW_HIDE };
			if (ProcessInformation pi; __pragma(warning(suppress:6335)) CreateProcessW(nullptr, data(commandLine), nullptr, nullptr, false, CREATE_NO_WINDOW, nullptr, systemDirectory().c_str(), &si, &pi))
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


static auto AesData(std::span<UCHAR> iv, std::span<UCHAR> text, bool encrypt) {
	return BCrypt(BCRYPT_AES_ALGORITHM, [iv, text, encrypt](BCRYPT_ALG_HANDLE alg) {
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
						Debug(L"{}() failed: 0x{:08X} or bad length: {}."sv, encrypt ? L"BCryptEncrypt"sv : L"BCryptDecrypt"sv, status, resultlen);
					BCryptDestroyKey(key);
				} else
					Debug(L"BCryptImportKey() failed: 0x{:08X}."sv, status);
			} else
				Debug(L"BCryptSetProperty({}) failed: 0x{:08X}."sv, BCRYPT_CHAINING_MODE, status);
		} else
			Debug(L"BCryptGetProperty({}) failed: 0x{:08X} or invalid length: {}."sv, BCRYPT_OBJECT_LENGTH, status, resultlen);
		return result;
	});
}

// パスワードの暗号化を解く(オリジナルアルゴリズム)
static std::optional<std::string> DecodePasswordOriginal(std::string_view encrypted) {
	std::string plain;
	for (size_t i = 0; i < size(encrypted); i += encrypted[i] & 0x01 ? 3 : 2)
		plain += (char)_rotr8(encrypted[i] & 0x0F | encrypted[i + 1] << 4, encrypted[i] >> 4 & 0x03);
	return plain;
}

// パスワードの暗号化を解く(オリジナルアルゴリズム＾Key)
static std::optional<std::string> DecodePassword2(std::string_view encrypted) {
	std::string plain;
	auto kit = (unsigned char*)SecretKey, kend = kit + strlen(SecretKey);
	for (size_t i = 0; i < size(encrypted); i += encrypted[i] & 0x01 ? 3 : 2){
		plain += (char)(_rotr8(encrypted[i] & 0x0F | encrypted[i + 1] << 4, encrypted[i] >> 4 & 0x03) ^ *kit);
		if (++kit == kend)
			kit = (unsigned char*)SecretKey;
	}
	return plain;
}

constexpr ULONG AesBlockSize = 16;
static_assert(std::popcount(AesBlockSize) == 1);

// パスワードの暗号化を解く(AES)
static std::optional<std::string> DecodePassword3(std::string_view encrypted) {
	if (AesBlockSize * 2 + 1 < size_as<ULONG>(encrypted) && encrypted[AesBlockSize * 2] == ':') {
		auto encodedLength = (size_as<ULONG>(encrypted) - 1) / 2 - AesBlockSize;
		std::array<UCHAR, AesBlockSize> iv;
		for (size_t i = 0; i < size(iv); i++)
			std::from_chars(data(encrypted) + i * 2, data(encrypted) + i * 2 + 2, iv[i], 16);
		std::string plain(encodedLength, '\0');
		auto temp = encrypted.substr(size(iv) * 2 + 1);
		for (size_t i = 0; i < encodedLength; i++)
			std::from_chars(data(temp) + i * 2, data(temp) + i * 2 + 2, reinterpret_cast<unsigned char&>(plain[i]), 16);
		if (AesData(iv, { data_as<UCHAR>(plain), size(plain) }, false)) {
			if (auto const pos = plain.find('\0'); pos != std::string::npos)
				plain.resize(pos);
			return std::move(plain);
		}
	}
	return {};
}

bool Config::ReadPassword(std::string_view name, std::wstring& password) const {
	if (auto value = ReadStringCore(name); value && !value->empty()) {
		value = 0x40 <= value->at(0) && value->at(0) < 0x80 ? DecodePasswordOriginal(*value)
			: value->starts_with("0A"sv) ? DecodePasswordOriginal(value->substr(2))
			: value->starts_with("0B"sv) ? DecodePassword2(value->substr(2))
			: value->starts_with("0C"sv) ? DecodePassword3(value->substr(2))
			: std::nullopt;
		if (value) {
			password.assign(u8(*value));
			return true;
		}
	}
	return false;
}

void Config::WritePassword(std::string_view name, std::wstring_view password) {
	BCrypt(BCRYPT_RNG_ALGORITHM, [this, name, plain = u8(password)](BCRYPT_ALG_HANDLE alg) {
		auto length = size_as<ULONG>(plain);
		auto paddedLength = std::max(length + AesBlockSize - 1 & ~(AesBlockSize - 1), AesBlockSize * 2);	/* 最低長を32文字とする */
		std::vector<UCHAR> buffer((size_t)paddedLength + AesBlockSize, 0);
		std::copy(begin(plain), end(plain), begin(buffer));
		/* PADとIV部分を乱数で埋める StrPad[StrLen](が有効な場合) は NUL */
		if (auto status = BCryptGenRandom(alg, &buffer[(size_t)length + 1], size_as<ULONG>(buffer) - length - 1, 0); status == STATUS_SUCCESS) {
			auto encrypted = "0C"s;
			for (auto i = paddedLength; i < size_as<ULONG>(buffer); i++)
				encrypted += std::format("{:02x}"sv, buffer[i]);
			encrypted += ':';
			if (AesData({ &buffer[paddedLength], AesBlockSize }, { &buffer[0], paddedLength }, true)) {
				for (auto i = 0u; i < paddedLength; i++)
					encrypted += std::format("{:02x}"sv, buffer[i]);
				WriteValue(name, encrypted);
			}
		} else
			Debug(L"BCryptGenRandom() failed: 0x{:08X}."sv, status);
		return 0;
	});
}


/*===== レジストリとINIファイルのアクセス処理 ============*/

struct IniConfig : Config {
	std::shared_ptr<std::map<std::string, std::vector<std::string>>> map;
	bool const update;
	IniConfig(std::string const& keyName, bool update) : Config{ keyName }, map{ std::make_shared<std::map<std::string, std::vector<std::string>>>() }, update{ update } {}
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
	std::unique_ptr<Config> OpenSubKey(std::string_view name) override {
		if (auto const keyName = KeyName + '\\' + name; map->contains(keyName))
			return std::make_unique<IniConfig>(keyName, *this);
		return {};
	}
	std::unique_ptr<Config> CreateSubKey(std::string_view name) override {
		return std::make_unique<IniConfig>(KeyName + '\\' + name, *this);
	}
	const char* Scan(std::string_view name) const {
		for (auto const& line : (*map)[KeyName])
			if (size(name) + 1 < size(line) && line.starts_with(name) && line[size(name)] == '=')
				return data(line) + size(name) + 1;
		return nullptr;
	}
	bool ReadIntImpl(std::string_view name, int& value) const override {
		if (auto const p = Scan(name)) {
			value = atoi(p);
			return true;
		}
		return false;
	}
	bool ReadStringImpl(std::string_view name, std::string& value) const override {
		static boost::regex re{ R"(\\([0-9A-F]{2})|\\\\)" };
		if (auto const p = Scan(name)) {
			value = replace({ p }, re, [](auto const& m) { return m[1].matched ? std::stoi(m[1], nullptr, 16) : '\\'; });
			if (IniKanjiCode == KANJI_SJIS)
				value = u8(a2w(value));
			return true;
		}
		return false;
	}
	void WriteIntImpl(std::string_view name, int value) override {
		(*map)[KeyName].push_back(std::string{ name } + '=' + std::to_string(value));
	}
	void WriteStringImpl(std::string_view name, std::string_view value, DWORD) override {
		auto line = std::string{ name } +'=';
		for (auto it = begin(value); it != end(value); ++it)
			if (0x20 <= *it && *it < 0x7F) {
				if (*it == '\\')
					line += '\\';
				line += *it;
			} else
				line += std::format("\\{:02X}"sv, (unsigned char)*it);
		(*map)[KeyName].push_back(std::move(line));
	}
};

struct RegConfig : Config {
	HKEY hKey;
	RegConfig(std::string const& keyName, HKEY hkey) : Config{ keyName }, hKey{ hkey } {}
	~RegConfig() override {
		RegCloseKey(hKey);
	}
	std::unique_ptr<Config> OpenSubKey(std::string_view name) override {
		if (HKEY key; RegOpenKeyExW(hKey, u8(name).c_str(), 0, KEY_READ, &key) == ERROR_SUCCESS)
			return std::make_unique<RegConfig>(KeyName + '\\' + name, key);
		return {};
	}
	std::unique_ptr<Config> CreateSubKey(std::string_view name) override {
		if (HKEY key; RegCreateKeyExW(hKey, u8(name).c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS)
			return std::make_unique<RegConfig>(KeyName + '\\' + name, key);
		return {};
	}
	bool ReadIntImpl(std::string_view name, int& value) const override {
		DWORD size = sizeof(int);
		return RegQueryValueExW(hKey, u8(name).c_str(), nullptr, nullptr, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS;
	}
	bool ReadStringImpl(std::string_view name, std::string& value) const override {
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
	void WriteIntImpl(std::string_view name, int value) override {
		RegSetValueExW(hKey, u8(name).c_str(), 0, REG_DWORD, reinterpret_cast<CONST BYTE*>(&value), sizeof(int));
	}
	void WriteStringImpl(std::string_view name, std::string_view value, DWORD type) override {
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
static int CheckPasswordValidity(std::string_view HashSv, int StretchCount) {
	/* 空文字列は一致したことにする */
	if (empty(HashSv))
		return PASSWORD_OK;
	if (size(HashSv) != 40)
		return BAD_PASSWORD_HASH;

	/* Hashをデコードする*/
	std::array<uint32_t, 5> hash1 = {};
	for (auto it = begin(HashSv); auto& decode : hash1)
		for (int j = 0; j < 8; j++) {
			if (*it < 0x40 || 0x40 + 15 < *it)
				return BAD_PASSWORD_HASH;
			decode = decode << 4 | *it++ & 0x0F;
		}

	/* Password をハッシュする */
	auto hash2 = HashPassword(StretchCount);
	return hash1 == hash2 ? PASSWORD_OK : PASSWORD_UNMATCH;
}

// パスワードの妥当性チェックのための文字列を作成する
static std::string CreatePasswordHash(int stretchCount) {
	std::string hash;
	for (auto rest : HashPassword(stretchCount))
		for (int j = 0; j < 8; j++)
			hash += (char)(0x40 | (rest = _rotl(rest, 4)) & 0x0F);
	return hash;
}

void SetHashSalt(DWORD salt) {
	if constexpr (std::endian::native == std::endian::little)
		salt = _byteswap_ulong(salt);
	SetHashSalt1(&salt, 4);
}

void SetHashSalt1(void* Salt, int Length) {
	if (Salt && 0 < Length)
		memcpy(SecretKey + strlen(SecretKey) + 1, Salt, Length);
	SecretKeyLength = (int)strlen(SecretKey) + 1 + Length;
}

void Config::Xor(std::string_view name, void* bin, DWORD len, bool preserveZero) const {
	if (EncryptSettings != YES)
		return;
	auto result = HashOpen(BCRYPT_SHA1_ALGORITHM, [bin, len, preserveZero, salt = KeyName + '\\' + name](auto alg, auto obj, auto hash) {
		assert(hash.size() == 20);
		auto p = static_cast<BYTE*>(bin);
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
		hKey3->ReadValue("Version", Version);
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
						writer->WriteString(host.HostName.c_str());
						writer->WriteCharEntity(L'\n');
						level++;
					} else {
						writer->WriteStartElement(nullptr, L"Server", nullptr);
						writer->WriteElementString(nullptr, L"Host", nullptr, host.HostAdrs.c_str());
						writer->WriteElementString(nullptr, L"Port", nullptr, std::to_wstring(host.Port).c_str());
						writer->WriteElementString(nullptr, L"Protocol", nullptr, host.UseNoEncryption == YES ? L"0" : host.UseFTPES == YES ? L"4" : host.UseFTPIS == YES ? L"3" : L"0");
						writer->WriteElementString(nullptr, L"Type", nullptr, L"0");
						writer->WriteElementString(nullptr, L"User", nullptr, host.UserName.c_str());
						writer->WriteElementString(nullptr, L"Pass", nullptr, host.PassWord.c_str());
						writer->WriteElementString(nullptr, L"Account", nullptr, host.Account.c_str());
						writer->WriteElementString(nullptr, L"Logontype", nullptr, host.Anonymous == YES || empty(host.UserName) ? L"0" : L"1");
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
						writer->WriteElementString(nullptr, L"Name", nullptr, host.HostName.c_str());
						writer->WriteElementString(nullptr, L"LocalDir", nullptr, host.LocalInitDir.c_str());
						auto remoteDir = host.RemoteInitDir;
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
						writer->WriteString(host.HostName.c_str());
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
	auto escape = [](std::wstring_view wstr) {
		auto const str = u8(wstr);
		std::string result;
		if (std::ranges::any_of(str, [](auto ch) { return ch & 0x80; }))
			result += "%EF%BB%BF"sv;
		for (auto ch : str)
			if ((ch & 0x80) || "\t\n\r %*?\\"sv.find(ch) != std::string_view::npos)
				result += std::format("%{:02X}"sv, (unsigned char)ch);
			else
				result += ch;
		return result;
	};
	auto encode = [](std::wstring_view user, std::wstring_view host, std::wstring_view password) {
		auto str = u8(concat(user, host, password));
		static_assert((char)~0xA3 == 0x5C);
		auto result = std::format("A35C{:02X}5C"sv, size_as<unsigned char>(str) ^ 0x5C);
		for (unsigned char ch : str)
			result += std::format("{:02X}"sv, ch ^ 0x5C);
		return result;
	};
	Message(IDS_NEED_EXSITING_WINSCP_INI, MB_OK);
	if (auto const file = SelectFile(false, GetMainHwnd(), IDS_SAVE_SETTING, L"WinSCP.ini", L"ini", { FileType::Ini, FileType::All }); !file.empty()) {
		if (std::ofstream f{ file, std::ofstream::in | std::ofstream::ate }) {
			std::vector<std::wstring> names;
			HOSTDATA Host;
			for (int i = 0; CopyHostFromList(i, &Host) == FFFTP_SUCCESS; i++) {
				assert((Host.Level & SET_LEVEL_MASK) <= size_as<int>(names));
				names.resize(Host.Level & SET_LEVEL_MASK);
				if (Host.Level & SET_LEVEL_GROUP) {
					names.push_back(Host.HostName);
					continue;
				}
				std::wstring path;
				for (auto name : names) {
					path += name;
					path += L'/';
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
