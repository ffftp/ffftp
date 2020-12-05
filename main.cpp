/*=============================================================================
*
*									ＦＦＦＴＰ
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
#pragma hdrstop
#include <delayimp.h>
#include <HtmlHelp.h>
#pragma comment(lib, "HtmlHelp.lib")

#define RESIZE_OFF		0		/* ウインドウの区切り位置変更していない */
#define RESIZE_ON		1		/* ウインドウの区切り位置変更中 */
#define RESIZE_PREPARE	2		/* ウインドウの区切り位置変更の準備 */

#define RESIZE_HPOS		0		/* ローカル－ホスト間の区切り位置変更 */
#define RESIZE_VPOS		1		/* リスト－タスク間の区切り位置の変更 */

/*===== コマンドラインオプション =====*/

#define OPT_MIRROR		0x00000001	/* ミラーリングアップロードを行う */
#define OPT_FORCE		0x00000002	/* ミラーリング開始の確認をしない */
#define OPT_QUIT		0x00000004	/* 終了後プログラム終了 */
#define OPT_EUC			0x00000008	/* 漢字コードはEUC */
#define OPT_JIS			0x00000010	/* 漢字コードはJIS */
#define OPT_ASCII		0x00000020	/* アスキー転送モード */
#define OPT_BINARY		0x00000040	/* バイナリ転送モード */
#define OPT_AUTO		0x00000080	/* 自動判別 */
#define OPT_KANA		0x00000100	/* 半角かなをそのまま通す */
#define OPT_EUC_NAME	0x00000200	/* ファイル名はEUC */
#define OPT_JIS_NAME	0x00000400	/* ファイル名はJIS */
#define OPT_MIRRORDOWN	0x00000800	/* ミラーリングダウンロードを行う */
#define OPT_SAVEOFF		0x00001000	/* 設定の保存を中止する */
#define OPT_SAVEON		0x00002000	/* 設定の保存を再開する */
#define OPT_SJIS		0x00004000	/* 漢字コードはShift_JIS */
#define OPT_UTF8N		0x00008000	/* 漢字コードはUTF-8 */
#define OPT_UTF8BOM		0x00010000	/* 漢字コードはUTF-8 BOM */
#define OPT_SJIS_NAME	0x00020000	/* ファイル名はShift_JIS */
#define OPT_UTF8N_NAME	0x00040000	/* ファイル名はUTF-8 */


/*===== プロトタイプ =====*/

static int InitApp(int cmdShow);
static bool MakeAllWindows(int cmdShow);
static void DeleteAllObject(void);
static LRESULT CALLBACK FtpWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void StartupProc(std::vector<std::wstring_view> const& args);
static std::optional<int> AnalyzeComLine(std::vector<std::wstring_view> const& args, std::wstring& hostname, std::wstring& unc);
static void ExitProc(HWND hWnd);
static void ChangeDir(int Win, char *Path);
static void ResizeWindowProc(void);
static void CalcWinSize(void);
static void CheckResizeFrame(WPARAM Keys, int x, int y);
static void DispDirInfo(void);
static void DeleteAlltempFile();
static void AboutDialog(HWND hWnd);
static int EnterMasterPasswordAndSet(bool newpassword, HWND hWnd);

/*===== ローカルなワーク =====*/

static const wchar_t FtpClass[] = L"FFFTPWin";
static const wchar_t WebURL[] = L"https://github.com/ffftp/ffftp";

static HINSTANCE hInstFtp;
static HWND hWndFtp;
static HWND hWndCurFocus = NULL;

static HACCEL Accel;

static int Resizing = RESIZE_OFF;
static int ResizePos;
static HCURSOR hCursor;

int ClientWidth;
static int ClientHeight;
int SepaWidth;
int RemoteWidth;
int ListHeight;

static std::vector<fs::path> TempFiles;

static int SaveExit = YES;
static int AutoExit = NO;

static fs::path IniPath;
static int ForceIni = NO;

TRANSPACKET MainTransPkt;		/* ファイル転送用パケット */
								/* これを使って転送を行うと、ツールバーの転送 */
								/* 中止ボタンで中止できる */

char TitleHostName[HOST_ADRS_LEN+1];
char FilterStr[FILTER_EXT_LEN+1] = { "*" };
char TitleUserName[USER_NAME_LEN+1];

int CancelFlg;

int SuppressRefresh = 0;

static DWORD dwCookie;

// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
static DWORD MainThreadId;
HANDLE ChangeNotification = INVALID_HANDLE_VALUE;
static int ToolWinHeight;


/*===== グローバルなワーク =====*/

static HWND hHelpWin = NULL;
std::map<int, std::string> msgs;
HCRYPTPROV HCryptProv;

/* 設定値 */
int WinPosX = CW_USEDEFAULT;
int WinPosY = 0;
int WinWidth = 790;
int WinHeight = 513;
int LocalWidth = 389;
int TaskHeight = 100;
int LocalTabWidth[4] = { 150, 120, 60, 37 };
int RemoteTabWidth[6] = { 150, 120, 60, 37, 60, 60 };
char UserMailAdrs[USER_MAIL_LEN+1] = { "who@example.com" };
char ViewerName[VIEWERS][FMAX_PATH+1] = { { "notepad" }, { "" }, { "" } };
HFONT ListFont = NULL;
int LocalFileSort = SORT_NAME;
int LocalDirSort = SORT_NAME;
int RemoteFileSort = SORT_NAME;
int RemoteDirSort = SORT_NAME;
int TransMode = TYPE_X;
int ConnectOnStart = YES;
int SaveWinPos = NO;
char AsciiExt[ASCII_EXT_LEN+1] = { "*.txt\0*.html\0*.htm\0*.cgi\0*.pl\0*.js\0*.vbs\0*.css\0*.rss\0*.rdf\0*.xml\0*.xhtml\0*.xht\0*.shtml\0*.shtm\0*.sh\0*.py\0*.rb\0*.properties\0*.sql\0*.asp\0*.aspx\0*.php\0*.htaccess\0" };
int RecvMode = TRANS_DLG;
int SendMode = TRANS_DLG;
int MoveMode = MOVE_DLG;
int ListType = LVS_REPORT;
char DefaultLocalPath[FMAX_PATH+1] = { "" };
int SaveTimeStamp = YES;
int FindMode = 0;
int DotFile = YES;
int DclickOpen = YES;
int ConnectAndSet = YES;
int FnameCnv = FNAME_NOCNV;
int TimeOut = 90;
int RmEOF = NO;
int RegType = REGTYPE_REG;
char FwallHost[HOST_ADRS_LEN+1] = { "" };
char FwallUser[USER_NAME_LEN+1] = { "" };
char FwallPass[PASSWORD_LEN+1] = { "" };
int FwallPort = IPPORT_FTP;
int FwallType = 1;
int FwallDefault = NO;
int FwallSecurity = SECURITY_AUTO;
int FwallResolve = NO;
int FwallLower = NO;
int FwallDelimiter = '@';
int PasvDefault = YES;
char MirrorNoTrn[MIRROR_LEN+1] = { "*.bak\0" };
char MirrorNoDel[MIRROR_LEN+1] = { "" };
int MirrorFnameCnv = NO;
int SplitVertical = YES;
int RasClose = NO;
int RasCloseNotify = YES;
char DefAttrList[DEFATTRLIST_LEN+1] = { "" };
SIZE HostDlgSize = { -1, -1 };
SIZE BmarkDlgSize = { -1, -1 };
SIZE MirrorDlgSize = { -1, -1 };
int Sizing = SW_RESTORE;
int SortSave = NO;
int QuickAnonymous = YES;
int VaxSemicolon = NO;
int SendQuit = NO;
int NoRasControl = NO;
int SuppressSave = NO;
int DispIgnoreHide = NO;
int DispDrives = NO;
int MirUpDelNotify = YES; 
int MirDownDelNotify = YES; 
int FolderAttr = NO;
int FolderAttrNum = 777;
int DispFileIcon = NO;
int DispTimeSeconds = NO;
int DispPermissionsNumber = NO;
int MakeAllDir = YES;
int LocalKanjiCode = KANJI_SJIS;
int NoopEnable = NO;
int UPnPEnabled = NO;
time_t LastDataConnectionTime = 0;
int EncryptAllSettings = NO;
int AutoRefreshFileList = YES;
int RemoveOldLog = NO;
int ReadOnlySettings = NO;
int AbortOnListError = YES;
int MirrorNoTransferContents = NO; 
int FwallNoSaveUser = NO; 
int MarkAsInternet = YES; 


fs::path const& systemDirectory() {
	static auto const path = [] {
		wchar_t directory[FMAX_PATH];
		auto length = GetSystemDirectoryW(directory, size_as<UINT>(directory));
		assert(0 < length);
		return fs::path{ directory, directory + length };
	}();
	return path;
}


fs::path const& moduleDirectory() {
	static auto const directory = [] {
		wchar_t directory[FMAX_PATH];
		const auto length = GetModuleFileNameW(nullptr, directory, size_as<DWORD>(directory));
		assert(0 < length);
		return fs::path{ directory, directory + length }.remove_filename();
	}();
	return directory;
}


fs::path const& tempDirectory() {
	static const auto directory = [] {
		wchar_t subdir[16];
		swprintf(subdir, std::size(subdir), L"ffftp%08x", GetCurrentProcessId());
		const auto path = fs::temp_directory_path() / subdir;
		fs::create_directory(path);
		return path;
	}();
	return directory;
}


static auto isPortable() {
	static const auto isPortable = fs::is_regular_file(moduleDirectory() / L"portable");
	return isPortable;
}


static auto const& helpPath() {
	static const auto path = moduleDirectory() / L"ffftp.chm";
	return path;
}

Sound Sound::Connected{ L"FFFTP_Connected", L"Connected", IDS_SOUNDCONNECTED };
Sound Sound::Transferred{ L"FFFTP_Transferred", L"Transferred", IDS_SOUNDTRANSFERRED };
Sound Sound::Error{ L"FFFTP_Error", L"Error", IDS_SOUNDERROR };
void Sound::Register() {
	if (HKEY eventlabels; RegCreateKeyExW(HKEY_CURRENT_USER, LR"(AppEvents\EventLabels)", 0, nullptr, 0, KEY_WRITE, nullptr, &eventlabels, nullptr) == ERROR_SUCCESS) {
		if (HKEY apps; RegCreateKeyExW(HKEY_CURRENT_USER, LR"(AppEvents\Schemes\Apps\ffftp)", 0, nullptr, 0, KEY_WRITE, nullptr, &apps, nullptr) == ERROR_SUCCESS) {
			RegSetValueExW(apps, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(L"FFFTP"), 12);
			for (auto [keyName, name, id] : { Connected, Transferred, Error }) {
				if (HKEY key; RegCreateKeyExW(eventlabels, keyName, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
					RegSetValueExW(key, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(name), ((DWORD)wcslen(name) + 1) * sizeof(wchar_t));
					auto value = strprintf(L"@%s,%d", (moduleDirectory() / L"ffftp.exe"sv).c_str(), -id);
					RegSetValueExW(key, L"DispFileName", 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), (size_as<DWORD>(value) + 1) * sizeof(wchar_t));
				}
				if (HKEY key; RegCreateKeyExW(apps, keyName, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
					if (HKEY _current; RegCreateKeyExW(key, L".current", 0, nullptr, 0, KEY_WRITE, nullptr, &_current, nullptr) == ERROR_SUCCESS)
						RegCloseKey(_current);
					RegCloseKey(key);
				}
			}
			RegCloseKey(apps);
		}
		RegCloseKey(eventlabels);
	}
}


// メインルーチン
int WINAPI wWinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPWSTR lpCmdLine, __in int nShowCmd) {
	hInstFtp = hInstance;
	EnumResourceNames(GetFtpInst(), RT_STRING, [](auto hModule, auto lpType, auto lpName, auto lParam) -> BOOL {
		wchar_t buffer[1024];
		if (IS_INTRESOURCE(lpName))
			for (int id = (PtrToInt(lpName) - 1) * 16, end = id + 16; id < end; id++)
				if (auto length = LoadStringW(hModule, id, buffer, size_as<int>(buffer)); 0 < length)
					msgs.emplace(id, u8(buffer, length));
		return true;
	}, 0);

	Sound::Register();

	// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
#ifdef DISABLE_MULTI_CPUS
	SetProcessAffinityMask(GetCurrentProcess(), 1);
#endif
	MainThreadId = GetCurrentThreadId();

	if (OleInitialize(nullptr) != S_OK) {
		Message(IDS_FAIL_TO_INIT_OLE, MB_OK | MB_ICONERROR);
		return 0;
	}

	LoadUPnP();
	LoadTaskbarList3();
	LoadZoneID();

	if (!CryptAcquireContextW(&HCryptProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		Message(IDS_ERR_CRYPTO, MB_OK | MB_ICONERROR);
		return 0;
	}

	if (!LoadSSL()) {
		Message(IDS_ERR_SSL, MB_OK | MB_ICONERROR);
		return 0;
	}

	int exitCode = FALSE;
	if (InitApp(nShowCmd) == FFFTP_SUCCESS) {
		MSG msg;
		while (GetMessageW(&msg, NULL, 0, 0)) {
			if (__pragma(warning(suppress:6387)) HtmlHelpW(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)&msg))
				continue;
			/* ディレクトリ名の表示コンボボックスでBSやRETが効くように */
			/* コンボボックス内ではアクセラレータを無効にする */
			if (msg.hwnd == GetLocalHistEditHwnd() || msg.hwnd == GetRemoteHistEditHwnd() || hHelpWin && GetAncestor(msg.hwnd, GA_ROOT) == hHelpWin || AskUserOpeDisabled() || TranslateAcceleratorW(GetMainHwnd(), Accel, &msg) == 0) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
		exitCode = (int)msg.wParam;
	}
	UnregisterClassW(FtpClass, GetFtpInst());
	FreeSSL();
	CryptReleaseContext(HCryptProv, 0);
	FreeZoneID();
	FreeTaskbarList3();
	FreeUPnP();
	OleUninitialize();
	return exitCode;
}


// アプリケーションの初期設定
static int InitApp(int cmdShow)
{
	int sts;
	int Err;
	WSADATA WSAData;
	int useDefautPassword = 0; /* 警告文表示用 */
	int masterpass;
	// ポータブル版判定
	int ImportPortable;
	// 高DPI対応
	int i;

	sts = FFFTP_FAIL;
	
	__pragma(warning(suppress:6387)) HtmlHelpW(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&dwCookie);

	if((Err = WSAStartup((WORD)0x0202, &WSAData)) != 0)
		MessageBoxW(GetMainHwnd(), GetErrorMessage(Err).c_str(), GetString(IDS_APP).c_str(), MB_OK);
	else
	{
		Accel = LoadAcceleratorsW(GetFtpInst(), MAKEINTRESOURCEW(ffftp_accel));

		// 高DPI対応
		WinWidth = CalcPixelX(WinWidth);
		WinHeight = CalcPixelY(WinHeight);
		LocalWidth = CalcPixelX(LocalWidth);
		TaskHeight = CalcPixelY(TaskHeight);
		for(i = 0; i < sizeof(LocalTabWidth) / sizeof(int); i++)
			LocalTabWidth[i] = CalcPixelX(LocalTabWidth[i]);
		for(i = 0; i < sizeof(RemoteTabWidth) / sizeof(int); i++)
			RemoteTabWidth[i] = CalcPixelX(RemoteTabWidth[i]);

		std::vector<std::wstring_view> args{ __wargv + 1, __wargv + __argc };
		if (auto it = std::find_if(begin(args), end(args), [](auto const& arg) { return ieq(arg, L"-n"sv) || ieq(arg, L"--ini"sv); }); it != end(args) && ++it != end(args)) {
			ForceIni = YES;
			RegType = REGTYPE_INI;
			IniPath = *it;
		} else
			IniPath = moduleDirectory() / L"ffftp.ini"sv;
		ImportPortable = NO;
		if (isPortable()) {
			ForceIni = YES;
			RegType = REGTYPE_INI;
			if(IsRegAvailable() == YES && IsIniAvailable() == NO)
			{
				if (Dialog(GetFtpInst(), ini_from_reg_dlg, GetMainHwnd()))
					ImportPortable = YES;
			}
		} else {
			if(ReadSettingsVersion() > VER_NUM)
			{
				if(IsRegAvailable() == YES && IsIniAvailable() == NO)
				{
					switch(Message(IDS_FOUND_NEW_VERSION_INI, MB_YESNOCANCEL | MB_DEFBUTTON2))
					{
						case IDCANCEL:
							ReadOnlySettings = YES;
							break;
						case IDYES:
							break;
						case IDNO:
							ImportPortable = YES;
							break;
					}
				}
			}
		}
		// ポータブル版判定
		if(ImportPortable == YES)
		{
			ForceIni = NO;
			RegType = REGTYPE_REG;
		}

		/* 2010.02.01 genta マスターパスワードを入力させる
		  -z オプションがあるときは最初だけスキップ
		  -z オプションがないときは，デフォルトパスワードをまず試す
		  LoadRegistry()する
		  パスワードが不一致なら再入力するか尋ねる．
		  (破損していた場合はさせない)
		*/
		if(auto it = std::find_if(begin(args), end(args), [](auto const& arg) { return ieq(arg, L"-z"sv) || ieq(arg, L"--mpasswd"sv); }); it != end(args) && ++it != end(args))
		{
			SetMasterPassword(u8(*it).c_str());
			useDefautPassword = 0;
		}
		else {
			/* パスワード指定無し */
			SetMasterPassword( NULL );
			/* この場では表示できないのでフラグだけ立てておく*/
			useDefautPassword = 2;
		}

		/* パスワードチェックのみ実施 */
		masterpass = 1;
		while( ValidateMasterPassword() == YES &&
				GetMasterPasswordStatus() == PASSWORD_UNMATCH ){
			
			if( useDefautPassword != 2 ){
				/* 再トライするか確認 */
				if( Message(IDS_MASTER_PASSWORD_INCORRECT, MB_YESNO | MB_ICONEXCLAMATION) == IDNO ){
					useDefautPassword = 0; /* 不一致なので，もはやデフォルトかどうかは分からない */
					break;
				}
			}
			
			/* 再入力させる*/
			masterpass = EnterMasterPasswordAndSet(false, NULL);
			if( masterpass == 2 ){
				useDefautPassword = 1;
			}
			else if( masterpass == 0 ){
				SaveExit = NO;
				break;
			}
			else {
				useDefautPassword = 0;
			}
		}
		
		if(masterpass != 0)
		{
			// ホスト共通設定機能
			ResetDefaultHost();

			LoadRegistry();

			// ポータブル版判定
			if(ImportPortable == YES)
			{
				ForceIni = YES;
				RegType = REGTYPE_INI;
			}

			//タイマの精度を改善
			timeBeginPeriod(1);

			if(MakeAllWindows(cmdShow))
			{
				hWndCurFocus = GetLocalHwnd();

				if (std::error_code ec; strlen(DefaultLocalPath) > 0)
					fs::current_path(fs::u8path(DefaultLocalPath), ec);

				SetSortTypeImm(LocalFileSort, LocalDirSort, RemoteFileSort, RemoteDirSort);
				SetTransferTypeImm(TransMode);
				DispTransferType();
				SetHostKanaCnvImm(YES);
				SetHostKanjiCodeImm(KANJI_NOCNV);
				// UTF-8対応
				SetLocalKanjiCodeImm(LocalKanjiCode);
				DispListType();
				DispDotFileMode();
				DispSyncMoveMode();

				if(MakeTransferThread() == FFFTP_SUCCESS)
				{
					DoPrintf("DEBUG MESSAGE ON ! ##");

					DispWindowTitle();
					UpdateStatusBar();
					SetTaskMsg("FFFTP Ver." VER_STR " Copyright(C) 1997-2010 Sota & cooperators.\r\n"
						"Copyright (C) 2011-2018 FFFTP Project (Hiromichi Matsushima, Suguru Kawamoto, IWAMOTO Kouichi, vitamin0x, unarist, Asami, fortran90, tomo1192, Yuji Tanaka, Moriguchi Hirokazu, Fu-sen, potato).\r\n"
						"Copyright (C) 2018-2021, Kurata Sayuri."
					);

					if(ForceIni)
						SetTaskMsg("%s%s", MSGJPN283, IniPath.u8string().c_str());

					DoPrintf("Help=%s", helpPath().u8string().c_str());

					DragAcceptFiles(GetRemoteHwnd(), TRUE);
					DragAcceptFiles(GetLocalHwnd(), TRUE);

					SetAllHistoryToMenu();
					GetLocalDirForWnd();
					MakeButtonsFocus();
					DispTransferFiles();

					StartupProc(args);
					sts = FFFTP_SUCCESS;

					/* セキュリティ警告文の表示 */
					if( useDefautPassword ){
						SetTaskMsg(MSGJPN300);
					}
					
					/* パスワード不一致警告文の表示 */
					switch( GetMasterPasswordStatus() ){
					case PASSWORD_UNMATCH:
						SetTaskMsg(MSGJPN301);
						break;
					case BAD_PASSWORD_HASH:
						SetTaskMsg(MSGJPN302);
						break;
					default:
						break;
					}
				}
			}
		}
	}

	if(sts == FFFTP_FAIL)
		DeleteAllObject();

	return(sts);
}


// ウインドウを作成する
static bool MakeAllWindows(int cmdShow) {
	WNDCLASSEXW classEx{ sizeof(WNDCLASSEXW), 0, FtpWndProc, 0, 0, GetFtpInst(), LoadIconW(GetFtpInst(), MAKEINTRESOURCEW(ffftp)), 0, GetSysColorBrush(COLOR_3DFACE), MAKEINTRESOURCEW(main_menu), FtpClass };
	RegisterClassExW(&classEx);

	ToolWinHeight = CalcPixelY(16) + 12;

	if (SaveWinPos == NO) {
		WinPosX = CW_USEDEFAULT;
		WinPosY = 0;
	}
	hWndFtp = CreateWindowExW(0, FtpClass, L"FFFTP", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WinPosX, WinPosY, WinWidth, WinHeight, HWND_DESKTOP, 0, GetFtpInst(), nullptr);
	if (!hWndFtp)
		return false;

	RECT workArea;
	SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
	RECT windowRect;
	GetWindowRect(GetMainHwnd(), &windowRect);
	if (workArea.bottom < windowRect.bottom)
		MoveWindow(GetMainHwnd(), windowRect.left, std::max(0L, windowRect.top - windowRect.bottom + workArea.bottom), WinWidth, WinHeight, FALSE);

	if (MakeStatusBarWindow() == FFFTP_FAIL)
		return false;

	CalcWinSize();

	if (!MakeToolBarWindow())
		return false;

	if (MakeListWin() == FFFTP_FAIL)
		return false;

	if (MakeTaskWindow() == FFFTP_FAIL)
		return false;

	ShowWindow(GetMainHwnd(), cmdShow != SW_MINIMIZE && cmdShow != SW_SHOWMINIMIZED && cmdShow != SW_SHOWMINNOACTIVE && Sizing == SW_MAXIMIZE ? SW_MAXIMIZE : cmdShow);

	if (MakeSocketWin() == FFFTP_FAIL)
		return false;

	if (InitListViewTips() == FFFTP_FAIL)
		return false;

	SetListViewType();

	return true;
}


// ウインドウのタイトルを表示する
void DispWindowTitle() {
	char text[HOST_ADRS_LEN + FILTER_EXT_LEN + 20];
	if (AskConnecting() == YES)
		sprintf(text, "%s (%s) - FFFTP", TitleHostName, FilterStr);
	else
		sprintf(text, "FFFTP (%s)", FilterStr);
	SetWindowTextW(GetMainHwnd(), u8(text).c_str());
}


// 全てのオブジェクトを削除
static void DeleteAllObject() {
	WSACleanup();
	if (hWndFtp != NULL)
		DestroyWindow(hWndFtp);
}


// メインウインドウのウインドウハンドルを返す
HWND GetMainHwnd() {
	return hWndFtp;
}


/*----- 現在フォーカスがあるウインドウのウインドウハンドルを返す --------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetFocusHwnd(void)
{
	return(hWndCurFocus);
}


/*----- 現在フォーカスがあるウインドウのをセットする --------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetFocusHwnd(HWND hWnd)
{
	hWndCurFocus = hWnd;
	return;
}


// プログラムのインスタンスを返す
HINSTANCE GetFtpInst() {
	return hInstFtp;
}


/*----- メインウインドウのメッセージ処理 --------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		UINT message  : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		メッセージに対応する戻り値
*----------------------------------------------------------------------------*/

static LRESULT CALLBACK FtpWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	RECT Rect;

	int TmpTransType;

	switch (message)
	{
		// ローカル側自動更新
		// タスクバー進捗表示
		case WM_CREATE :
			SetTimer(hWnd, 1, 1000, NULL);
			SetTimer(hWnd, 2, 100, NULL);
			break;

		// ローカル側自動更新
		// 自動切断対策
		// タスクバー進捗表示
		case WM_TIMER :
			switch(wParam)
			{
			case 1:
				if(WaitForSingleObject(ChangeNotification, 0) == WAIT_OBJECT_0)
				{
					if (!AskUserOpeDisabled())
					{
						FindNextChangeNotification(ChangeNotification);
						if(AutoRefreshFileList == YES)
						{
							char Name[FMAX_PATH+1];
							int Pos;
							std::vector<FILELIST> Base;
							MakeSelectedFileList(WIN_LOCAL, NO, NO, Base, &CancelFlg);
							GetHotSelected(WIN_LOCAL, Name);
							Pos = (int)SendMessageW(GetLocalHwnd(), LVM_GETTOPINDEX, 0, 0);
							GetLocalDirForWnd();
							SelectFileInList(GetLocalHwnd(), SELECT_LIST, Base);
							SetHotSelected(WIN_LOCAL, Name);
							SendMessageW(GetLocalHwnd(), LVM_ENSUREVISIBLE, (WPARAM)(SendMessageW(GetLocalHwnd(), LVM_GETITEMCOUNT, 0, 0) - 1), true);
							SendMessageW(GetLocalHwnd(), LVM_ENSUREVISIBLE, (WPARAM)Pos, true);
						}
					}
				}
				if(CancelFlg == YES)
					AbortRecoveryProc();
				if(NoopEnable == YES && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
				{
					NoopProc(NO);
					LastDataConnectionTime = time(NULL);
				}
				break;
			case 2:
				if(IsTaskbarList3Loaded() == YES)
					UpdateTaskbarProgress();
				break;
			}
			break;

		case WM_COMMAND :
			// 同時接続対応
			// 中断後に受信バッファに応答が残っていると次のコマンドの応答が正しく処理できない
			if(CancelFlg == YES)
				AbortRecoveryProc();
			switch(LOWORD(wParam))
			{
				case MENU_CONNECT :
					// 自動切断対策
					NoopEnable = NO;
					ConnectProc(DLG_TYPE_CON, -1);
					// 自動切断対策
					NoopEnable = YES;
					break;

				case MENU_CONNECT_NUM :
					// 自動切断対策
					NoopEnable = NO;
					ConnectProc(DLG_TYPE_CON, (int)lParam);
					// 自動切断対策
					NoopEnable = YES;
					if(AskConnecting() == YES)
					{
						if(HIWORD(wParam) & OPT_MIRROR)
						{
							if(HIWORD(wParam) & OPT_FORCE)
								MirrorUploadProc(NO);
							else
								MirrorUploadProc(YES);
						}
						else if(HIWORD(wParam) & OPT_MIRRORDOWN)
						{
							if(HIWORD(wParam) & OPT_FORCE)
								MirrorDownloadProc(NO);
							else
								MirrorDownloadProc(YES);
						}
					}
					break;

				case MENU_SET_CONNECT :
					// 自動切断対策
					NoopEnable = NO;
					ConnectProc(DLG_TYPE_SET, -1);
					// 自動切断対策
					NoopEnable = YES;
					break;

				case MENU_QUICK :
					// 自動切断対策
					NoopEnable = NO;
					QuickConnectProc();
					// 自動切断対策
					NoopEnable = YES;
					break;

				case MENU_DISCONNECT :
					if(AskTryingConnect() == YES)
						CancelFlg = YES;
					else if(AskConnecting() == YES)
					{
						SaveBookMark();
						SaveCurrentSetToHost();
						DisconnectProc();
					}
					break;

				case MENU_HIST_1 :
				case MENU_HIST_2 :
				case MENU_HIST_3 :
				case MENU_HIST_4 :
				case MENU_HIST_5 :
				case MENU_HIST_6 :
				case MENU_HIST_7 :
				case MENU_HIST_8 :
				case MENU_HIST_9 :
				case MENU_HIST_10 :
				case MENU_HIST_11 :
				case MENU_HIST_12 :
				case MENU_HIST_13 :
				case MENU_HIST_14 :
				case MENU_HIST_15 :
				case MENU_HIST_16 :
				case MENU_HIST_17 :
				case MENU_HIST_18 :
				case MENU_HIST_19 :
				case MENU_HIST_20 :
					// 自動切断対策
					NoopEnable = NO;
					HistoryConnectProc(LOWORD(wParam));
					// 自動切断対策
					NoopEnable = YES;
					break;

				case MENU_UPDIR :
					if(hWndCurFocus == GetLocalHwnd())
						PostMessageW(hWnd, WM_COMMAND, MAKEWPARAM(MENU_LOCAL_UPDIR, 0), 0);
					else
						PostMessageW(hWnd, WM_COMMAND, MAKEWPARAM(MENU_REMOTE_UPDIR, 0), 0);
					break;

				case MENU_DCLICK :
					if(hWndCurFocus == GetLocalHwnd())
						// ローカルフォルダを開く
//						DoubleClickProc(WIN_LOCAL, YES, -1);
						DoubleClickProc(WIN_LOCAL, NO, -1);
					else
					{
						SuppressRefresh = 1;
						// ローカルフォルダを開く
//						DoubleClickProc(WIN_REMOTE, YES, -1);
						DoubleClickProc(WIN_REMOTE, NO, -1);
						SuppressRefresh = 0;
					}
					break;

				// ローカルフォルダを開く
				case MENU_OPEN :
					if(hWndCurFocus == GetLocalHwnd())
						DoubleClickProc(WIN_LOCAL, YES, -1);
					else
					{
						SuppressRefresh = 1;
						DoubleClickProc(WIN_REMOTE, YES, -1);
						SuppressRefresh = 0;
					}
					break;

				case MENU_OPEN1 :
					if(hWndCurFocus == GetLocalHwnd())
						DoubleClickProc(WIN_LOCAL, YES, 0);
					else
					{
						SuppressRefresh = 1;
						DoubleClickProc(WIN_REMOTE, YES, 0);
						SuppressRefresh = 0;
					}
					break;

				case MENU_OPEN2 :
					if(hWndCurFocus == GetLocalHwnd())
						DoubleClickProc(WIN_LOCAL, YES, 1);
					else
					{
						SuppressRefresh = 1;
						DoubleClickProc(WIN_REMOTE, YES, 1);
						SuppressRefresh = 0;
					}
					break;

				case MENU_OPEN3 :
					if(hWndCurFocus == GetLocalHwnd())
						DoubleClickProc(WIN_LOCAL, YES, 2);
					else
					{
						SuppressRefresh = 1;
						DoubleClickProc(WIN_REMOTE, YES, 2);
						SuppressRefresh = 0;
					}
					break;

				case MENU_REMOTE_UPDIR :
					if (AskUserOpeDisabled())
						break;
					SuppressRefresh = 1;
					SetCurrentDirAsDirHist();
					ChangeDir(WIN_REMOTE, "..");
					SuppressRefresh = 0;
					break;

				case MENU_LOCAL_UPDIR :
					if (AskUserOpeDisabled())
						break;
					SetCurrentDirAsDirHist();
					ChangeDir(WIN_LOCAL, "..");
					break;

				case MENU_REMOTE_CHDIR :
					SuppressRefresh = 1;
					SetCurrentDirAsDirHist();
					ChangeDirDirectProc(WIN_REMOTE);
					SuppressRefresh = 0;
					break;

				case MENU_LOCAL_CHDIR :
					SetCurrentDirAsDirHist();
					ChangeDirDirectProc(WIN_LOCAL);
					break;

				case MENU_DOWNLOAD :
					SetCurrentDirAsDirHist();
					DownloadProc(NO, NO, NO);
					break;

				case MENU_DOWNLOAD_AS :
					SetCurrentDirAsDirHist();
					DownloadProc(YES, NO, NO);
					break;

				case MENU_DOWNLOAD_AS_FILE :
					SetCurrentDirAsDirHist();
					DownloadProc(NO, YES, NO);
					break;

				case MENU_DOWNLOAD_ALL :
					SetCurrentDirAsDirHist();
					DownloadProc(NO, NO, YES);
					break;

				case MENU_DOWNLOAD_NAME :
					SetCurrentDirAsDirHist();
					InputDownloadProc();
					break;

				case MENU_UPLOAD :
					SetCurrentDirAsDirHist();
					UploadListProc(NO, NO);
					break;

				case MENU_UPLOAD_AS :
					SetCurrentDirAsDirHist();
					UploadListProc(YES, NO);
					break;

				case MENU_UPLOAD_ALL :
					SetCurrentDirAsDirHist();
					UploadListProc(NO, YES);
					break;

				case MENU_MIRROR_UPLOAD :
					SetCurrentDirAsDirHist();
					MirrorUploadProc(YES);
					break;

				case MENU_MIRROR_DOWNLOAD :
					SetCurrentDirAsDirHist();
					MirrorDownloadProc(YES);
					break;

				case MENU_FILESIZE :
					SetCurrentDirAsDirHist();
					CalcFileSizeProc();
					break;

				case MENU_DELETE :
					SuppressRefresh = 1;
					SetCurrentDirAsDirHist();
					DeleteProc();
					SuppressRefresh = 0;
					break;

				case MENU_RENAME :
					SuppressRefresh = 1;
					SetCurrentDirAsDirHist();
					RenameProc();
					SuppressRefresh = 0;
					break;

				case MENU_MKDIR :
					SuppressRefresh = 1;
					SetCurrentDirAsDirHist();
					MkdirProc();
					SuppressRefresh = 0;
					break;

				case MENU_CHMOD :
					SuppressRefresh = 1;
					ChmodProc();
					SuppressRefresh = 0;
					break;

				case MENU_SOMECMD :
					SuppressRefresh = 1;
					SomeCmdProc();
					SuppressRefresh = 0;
					break;

				case MENU_OPTION :
					SetOption();
					if(ListFont != NULL)
					{
						SendMessageW(GetLocalHwnd(), WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
						SendMessageW(GetRemoteHwnd(), WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
						SendMessageW(GetTaskWnd(), WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
					}
					GetLocalDirForWnd();
					DispTransferType();
					SetAllHistoryToMenu();
					break;

				case MENU_FILTER :
					// 同時接続対応
					CancelFlg = NO;
					SetFilter(&CancelFlg);
					break;

				case MENU_SORT :
					if(SortSetting() == YES)
					{
						// 同時接続対応
						CancelFlg = NO;
						LocalFileSort = AskSortType(ITEM_LFILE);
						LocalDirSort = AskSortType(ITEM_LDIR);
						RemoteFileSort = AskSortType(ITEM_RFILE);
						RemoteDirSort = AskSortType(ITEM_RDIR);
						ReSortDispList(WIN_LOCAL, &CancelFlg);
						ReSortDispList(WIN_REMOTE, &CancelFlg);
					}
					break;

				case MENU_EXIT :
					PostMessageW(hWnd, WM_CLOSE, 0, 0L);
					break;

				case MENU_AUTO_EXIT :
					if(AutoExit == YES)
						PostMessageW(hWnd, WM_CLOSE, 0, 0L);
					break;

				case MENU_ABOUT :
					AboutDialog(hWnd);
					break;

				case MENU_TEXT :
				case MENU_BINARY :
				case MENU_AUTO :
					SetTransferType(LOWORD(wParam));
					DispTransferType();
					break;

				case MENU_XFRMODE :
					switch(AskTransferType())
					{
						case TYPE_A :
							TmpTransType = MENU_BINARY;
							break;

						case TYPE_I :
							TmpTransType = MENU_AUTO;
							break;

						default :
							TmpTransType = MENU_TEXT;
							break;
					}
					SetTransferType(TmpTransType);
					DispTransferType();
					break;

				// UTF-8対応
				case MENU_KNJ_SJIS :
				case MENU_KNJ_EUC :
				case MENU_KNJ_JIS :
				case MENU_KNJ_UTF8N :
				case MENU_KNJ_UTF8BOM :
				case MENU_KNJ_NONE :
					SetHostKanjiCode(LOWORD(wParam));
					break;

				case MENU_L_KNJ_SJIS :
				case MENU_L_KNJ_EUC :
				case MENU_L_KNJ_JIS :
				case MENU_L_KNJ_UTF8N :
				case MENU_L_KNJ_UTF8BOM :
					SetLocalKanjiCode(LOWORD(wParam));
					break;

				case MENU_KANACNV :
					SetHostKanaCnv();
					break;

				case MENU_REFRESH :
					if (AskUserOpeDisabled())
						break;
					// 同時接続対応
					CancelFlg = NO;
					SuppressRefresh = 1;
					GetLocalDirForWnd();
					if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
						GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
					SuppressRefresh = 0;
					break;

				case MENU_LIST :
					ListType = LVS_LIST;
					DispListType();
					SetListViewType();
					break;

				case MENU_REPORT :
					ListType = LVS_REPORT;
					DispListType();
					SetListViewType();
					break;

				case REFRESH_LOCAL :
					if (AskUserOpeDisabled())
						break;
					GetLocalDirForWnd();
					break;

				case REFRESH_REMOTE :
					if (AskUserOpeDisabled())
						break;
					// 同時接続対応
					CancelFlg = NO;
					SuppressRefresh = 1;
					if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
						GetRemoteDirForWnd(CACHE_REFRESH, &CancelFlg);
					SuppressRefresh = 0;
					break;

				case COMBO_LOCAL :
				case COMBO_REMOTE :
					SuppressRefresh = 1;
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						SetCurrentDirAsDirHist();
						ChangeDirComboProc((HWND)lParam);
					}
					else if(HIWORD(wParam) != CBN_CLOSEUP)
					{
						MakeButtonsFocus();
						SuppressRefresh = 0;
						return(0);
					}
					SuppressRefresh = 0;
					break;

				case MENU_HELP :
					ShowHelp(IDH_HELP_TOPIC_0000001);
					break;

				case MENU_HELP_TROUBLE :
					ShellExecuteW(0, L"open", WebURL, nullptr, nullptr, SW_SHOW);
					break;

				case MENU_BMARK_ADD :
					AddCurDirToBookMark(WIN_REMOTE);
					break;

				case MENU_BMARK_ADD_LOCAL :
					AddCurDirToBookMark(WIN_LOCAL);
					break;

				case MENU_BMARK_ADD_BOTH :
					AddCurDirToBookMark(WIN_BOTH);
					break;

				case MENU_BMARK_EDIT :
					EditBookMark();
					break;

				case MENU_SELECT_ALL :
					SelectFileInList(hWndCurFocus, SELECT_ALL, {});
					break;

				case MENU_SELECT :
					SelectFileInList(hWndCurFocus, SELECT_REGEXP, {});
					break;

				case MENU_FIND :
					FindFileInList(hWndCurFocus, FIND_FIRST);
					break;

				case MENU_FINDNEXT :
					FindFileInList(hWndCurFocus, FIND_NEXT);
					break;

				case MENU_DOTFILE :
					if (AskUserOpeDisabled())
						break;
					// 同時接続対応
					CancelFlg = NO;
					DotFile ^= 1;
					DispDotFileMode();
					GetLocalDirForWnd();
					GetRemoteDirForWnd(CACHE_LASTREAD, &CancelFlg);
					break;

				case MENU_SYNC :
					ToggleSyncMoveMode();
					break;

				case MENU_IMPORT_WS :
					ImportFromWSFTP();
					break;

				case MENU_REGSAVE :
					GetListTabWidth();
					SaveRegistry();
					SaveSettingsToFile();
					break;

				case MENU_REGLOAD :
					if(LoadSettingsFromFile() == YES)
					{
						Message(IDS_NEED_RESTART, MB_OK);
						SaveExit = NO;
						PostMessageW(hWnd, WM_CLOSE, 0, 0L);
					}
					break;

				case MENU_REGINIT :
					if(Dialog(GetFtpInst(), reginit_dlg, hWnd))
					{
						ClearRegistry();
						// ポータブル版判定
						ClearIni();
						SaveExit = NO;
						PostMessageW(hWnd, WM_CLOSE, 0, 0L);
					}
					break;
				case MENU_CHANGEPASSWD:	/* 2010.01.31 genta */
					if( GetMasterPasswordStatus() != PASSWORD_OK )
					{
						/* 強制的に設定するか確認 */
						if (!Dialog(GetFtpInst(), forcepasschange_dlg, hWnd))
							break;
						if(EnterMasterPasswordAndSet(true, hWnd) != 0)
							SetTaskMsg(MSGJPN303);
					}
					else if(GetMasterPasswordStatus() == PASSWORD_OK)
					{
						char Password[MAX_PASSWORD_LEN + 1];
						GetMasterPassword(Password);
						SetMasterPassword(NULL);
						while(ValidateMasterPassword() == YES && GetMasterPasswordStatus() == PASSWORD_UNMATCH)
						{
							if(EnterMasterPasswordAndSet(false, hWnd) == 0)
								break;
						}
						if(GetMasterPasswordStatus() == PASSWORD_OK && EnterMasterPasswordAndSet(true, hWnd) != 0)
						{
							SetTaskMsg(MSGJPN303);
							SaveRegistry();
						}
						else
						{
							SetMasterPassword(Password);
							ValidateMasterPassword();
						}
					}
					break;

				case MENU_DIRINFO :
					DispDirInfo();
					break;

				case MENU_TASKINFO :
					DispTaskMsg();
					break;

				case MENU_ABORT :
					CancelFlg = YES;
					if(AskTryingConnect() == NO)
						MainTransPkt.Abort = ABORT_USER;
					break;

				case MENU_OTPCALC :
					OtpCalcTool();
					break;

				// FTPS対応
				case MENU_FW_FTP_FILTER :
					TurnStatefulFTPFilter();
					break;

				case MENU_URL_COPY :
					CopyURLtoClipBoard();
					break;

				case MENU_APPKEY :
					EraseListViewTips();
					if(hWndCurFocus == GetRemoteHwnd())
						ShowPopupMenu(WIN_REMOTE, 1);
					else if(hWndCurFocus == GetLocalHwnd())
						ShowPopupMenu(WIN_LOCAL, 1);
					break;

#if defined(HAVE_TANDEM)
				case MENU_SWITCH_OSS :
					SwitchOSSProc();
					break;
#endif

				// 上位のディレクトリへ移動対応
				case MENU_REMOTE_MOVE_UPDIR :
					MoveRemoteFileProc(-1);
					break;

				// FileZilla XML形式エクスポート対応
				case MENU_EXPORT_FILEZILLA_XML :
					// 平文で出力するためマスターパスワードを再確認
					if(GetMasterPasswordStatus() == PASSWORD_OK)
					{
						char Password[MAX_PASSWORD_LEN + 1];
						GetMasterPassword(Password);
						SetMasterPassword(NULL);
						while(ValidateMasterPassword() == YES && GetMasterPasswordStatus() == PASSWORD_UNMATCH)
						{
							if(EnterMasterPasswordAndSet(false, hWnd) == 0)
								break;
						}
						if(GetMasterPasswordStatus() == PASSWORD_OK)
							SaveSettingsToFileZillaXml();
						else
						{
							SetMasterPassword(Password);
							ValidateMasterPassword();
						}
					}
					break;

				// WinSCP INI形式エクスポート対応
				case MENU_EXPORT_WINSCP_INI :
					// 平文で出力するためマスターパスワードを再確認
					if(GetMasterPasswordStatus() == PASSWORD_OK)
					{
						char Password[MAX_PASSWORD_LEN + 1];
						GetMasterPassword(Password);
						SetMasterPassword(NULL);
						while(ValidateMasterPassword() == YES && GetMasterPasswordStatus() == PASSWORD_UNMATCH)
						{
							if(EnterMasterPasswordAndSet(false, hWnd) == 0)
								break;
						}
						if(GetMasterPasswordStatus() == PASSWORD_OK)
							SaveSettingsToWinSCPIni();
						else
						{
							SetMasterPassword(Password);
							ValidateMasterPassword();
						}
					}
					break;

				default :
					if((LOWORD(wParam) >= MENU_BMARK_TOP) &&
					   (LOWORD(wParam) < MENU_BMARK_TOP+100))
					{
						ChangeDirBmarkProc(LOWORD(wParam));
					}
					break;
			}
// 常にホストかローカルへフォーカスを移動
//			SetFocus(hWndCurFocus);
			MakeButtonsFocus();
			break;

		case WM_NOTIFY :
			if (NotifyStatusBar(reinterpret_cast<const NMHDR*>(lParam)))
				break;
			switch(((LPNMHDR)lParam)->code)
			{
				/* ツールチップコントロールメッセージの処理 */
				case TTN_GETDISPINFOW:
				{
					static constexpr std::tuple<int, int> map[] = {
						{ MENU_CONNECT, IDS_MSGJPN154 },
						{ MENU_QUICK, IDS_MSGJPN155 },
						{ MENU_DISCONNECT, IDS_MSGJPN156 },
						{ MENU_DOWNLOAD, IDS_MSGJPN157 },
#if defined(HAVE_TANDEM)
						{ MENU_DOWNLOAD_AS, IDS_MSGJPN065 },
						{ MENU_UPLOAD_AS, IDS_MSGJPN064 },
#endif
						{ MENU_UPLOAD, IDS_MSGJPN158 },
						{ MENU_MIRROR_UPLOAD, IDS_MSGJPN159 },
						{ MENU_DELETE, IDS_MSGJPN160 },
						{ MENU_RENAME, IDS_MSGJPN161 },
						{ MENU_MKDIR, IDS_MSGJPN162 },
						{ MENU_LOCAL_UPDIR, IDS_MSGJPN163 },
						{ MENU_REMOTE_UPDIR, IDS_MSGJPN163 },
						{ MENU_LOCAL_CHDIR, IDS_MSGJPN164 },
						{ MENU_REMOTE_CHDIR, IDS_MSGJPN164 },
						{ MENU_TEXT, IDS_MSGJPN165 },
						{ MENU_BINARY, IDS_MSGJPN166 },
						{ MENU_AUTO, IDS_MSGJPN167 },
						{ MENU_REFRESH, IDS_MSGJPN168 },
						{ MENU_LIST, IDS_MSGJPN169 },
						{ MENU_REPORT, IDS_MSGJPN170 },
						{ MENU_KNJ_SJIS, IDS_MSGJPN307 },
						{ MENU_KNJ_EUC, IDS_MSGJPN171 },
						{ MENU_KNJ_JIS, IDS_MSGJPN172 },
						{ MENU_KNJ_UTF8N, IDS_MSGJPN308 },
						{ MENU_KNJ_UTF8BOM, IDS_MSGJPN330 },
						{ MENU_KNJ_NONE, IDS_MSGJPN173 },
						{ MENU_L_KNJ_SJIS, IDS_MSGJPN309 },
						{ MENU_L_KNJ_EUC, IDS_MSGJPN310 },
						{ MENU_L_KNJ_JIS, IDS_MSGJPN311 },
						{ MENU_L_KNJ_UTF8N, IDS_MSGJPN312 },
						{ MENU_L_KNJ_UTF8BOM, IDS_MSGJPN331 },
						{ MENU_KANACNV, IDS_MSGJPN174 },
						{ MENU_SYNC, IDS_MSGJPN175 },
						{ MENU_ABORT, IDS_MSGJPN176 },
					};
					auto di = reinterpret_cast<NMTTDISPINFOW*>(lParam);
					for (auto [menuId, resourceId] : map) {
						if (di->hdr.idFrom == menuId) {
							di->lpszText = MAKEINTRESOURCEW(resourceId);
							di->hinst = GetFtpInst();
							break;
						}
					}
					break;
				}
				case LVN_COLUMNCLICK :
					if(((NMHDR *)lParam)->hwndFrom == GetLocalHwnd())
					{
						// 同時接続対応
						CancelFlg = NO;
						SetSortTypeByColumn(WIN_LOCAL, ((NM_LISTVIEW *)lParam)->iSubItem);
						ReSortDispList(WIN_LOCAL, &CancelFlg);
					}
					else if(((NMHDR *)lParam)->hwndFrom == GetRemoteHwnd())
					{
						if(((NM_LISTVIEW *)lParam)->iSubItem != 4)
						{
							// 同時接続対応
							CancelFlg = NO;
							SetSortTypeByColumn(WIN_REMOTE, ((NM_LISTVIEW *)lParam)->iSubItem);
							ReSortDispList(WIN_REMOTE, &CancelFlg);
						}
					}
					SetFocus(hWndCurFocus);
					break;

				case LVN_ITEMCHANGED :
					{
						// SetTimerによるとnIDEventが一致する既存のタイマーを置き換えるとのこと <https://msdn.microsoft.com/en-us/library/ms644906(v=vs.85).aspx>
						// この通りであれば問題ない。ただしCWnd::SetTimerによると新たなタイマーが作成される（＝既存のタイマーを置き換えない）とのこと
						// <https://docs.microsoft.com/ja-jp/cpp/mfc/reference/cwnd-class#settimer>
						auto id = SetTimer(hWnd, 3, USER_TIMER_MINIMUM, [](auto hWnd, auto, auto, auto) {
							DispSelectedSpace();
							MakeButtonsFocus();
							auto result = KillTimer(hWnd, 3);
							assert(result);
						});
						assert(id == 3);
					}
					break;
			}
			break;

		case WM_SIZE :
			Sizing = SW_RESTORE;
			if(wParam == SIZE_RESTORED)
			{
				ResizeWindowProc();
				GetWindowRect(hWnd, &Rect);
				WinPosX = Rect.left;
				WinPosY = Rect.top;
			}
			else if(wParam == SIZE_MAXIMIZED)
			{
				Sizing = SW_MAXIMIZE;
				ResizeWindowProc();
			}
			else
				return DefWindowProcW(hWnd, message, wParam, lParam);
			break;

		case WM_MOVING :
			WinPosX = ((RECT *)lParam)->left;
			WinPosY = ((RECT *)lParam)->top;
			return DefWindowProcW(hWnd, message, wParam, lParam);

		case WM_SETFOCUS :
			SetFocus(hWndCurFocus);
			break;

		case WM_LBUTTONDOWN :
		case WM_LBUTTONUP :
		case WM_MOUSEMOVE :
			CheckResizeFrame(wParam, LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_CHANGE_COND :
			DispTransferFiles();
			break;

		case WM_REFRESH_LOCAL_FLG :
			// 外部アプリケーションへドロップ後にローカル側のファイル一覧に作業フォルダが表示されるバグ対策
			if(SuppressRefresh == 0)
				PostMessageW(hWnd,  WM_COMMAND, MAKEWPARAM(REFRESH_LOCAL, 0), 0);
			break;

		case WM_REFRESH_REMOTE_FLG :
			if(SuppressRefresh == 0)
				PostMessageW(hWnd,  WM_COMMAND, MAKEWPARAM(REFRESH_REMOTE, 0), 0);
			break;

		// UPnP対応
		case WM_ADDPORTMAPPING :
			((ADDPORTMAPPINGDATA*)lParam)->r = AddPortMapping(((ADDPORTMAPPINGDATA*)lParam)->Adrs, ((ADDPORTMAPPINGDATA*)lParam)->Port, ((ADDPORTMAPPINGDATA*)lParam)->ExtAdrs);
			SetEvent(((ADDPORTMAPPINGDATA*)lParam)->h);
			break;

		case WM_REMOVEPORTMAPPING :
			((REMOVEPORTMAPPINGDATA*)lParam)->r = RemovePortMapping(((REMOVEPORTMAPPINGDATA*)lParam)->Port);
			SetEvent(((REMOVEPORTMAPPINGDATA*)lParam)->h);
			break;

		// 同時接続対応
		case WM_RECONNECTSOCKET :
			ReconnectProc();
			break;

		// ゾーンID設定追加
		case WM_MARKFILEASDOWNLOADEDFROMINTERNET :
			((MARKFILEASDOWNLOADEDFROMINTERNETDATA*)lParam)->r = MarkFileAsDownloadedFromInternet(((MARKFILEASDOWNLOADEDFROMINTERNETDATA*)lParam)->Fname);
			SetEvent(((MARKFILEASDOWNLOADEDFROMINTERNETDATA*)lParam)->h);
			break;

		case WM_PAINT :
			BeginPaint(hWnd, (LPPAINTSTRUCT) &ps);
			EndPaint(hWnd, (LPPAINTSTRUCT) &ps);
			break;

		case WM_DESTROY :
			// ローカル側自動更新
			KillTimer(hWnd, 1);
			if(ChangeNotification != INVALID_HANDLE_VALUE)
				FindCloseChangeNotification(ChangeNotification);
			// タスクバー進捗表示
			KillTimer(hWnd, 2);
			PostQuitMessage(0);
			break;

		case WM_QUERYENDSESSION :
			ExitProc(hWnd);
			return(TRUE);

		case WM_CLOSE :
			if (AskTransferNow() == NO || Dialog(GetFtpInst(), exit_dlg, hWnd)) {
				ExitProc(hWnd);
				return DefWindowProcW(hWnd, message, wParam, lParam);
			}
			break;

		default :
			return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return(0L);
}


// プログラム開始時の処理
static void StartupProc(std::vector<std::wstring_view> const& args) {
	std::wstring hostname;
	std::wstring unc;
	if (auto result = AnalyzeComLine(args, hostname, unc)) {
		int opt = *result;
		int Kanji = opt & OPT_UTF8BOM ? KANJI_UTF8BOM : opt & OPT_UTF8N ? KANJI_UTF8N : opt & OPT_SJIS ? KANJI_SJIS : opt & OPT_JIS ? KANJI_JIS : opt & OPT_EUC ? KANJI_EUC : KANJI_NOCNV;
		int Kana = opt & OPT_KANA ? NO : YES;
		int FnameKanji = opt & OPT_UTF8N_NAME ? KANJI_UTF8N : opt & OPT_SJIS_NAME ? KANJI_SJIS : opt & OPT_JIS_NAME ? KANJI_JIS : opt & OPT_EUC_NAME ? KANJI_EUC : KANJI_NOCNV;
		int TrMode = opt & OPT_BINARY ? TYPE_I : opt & OPT_ASCII ? TYPE_A : TYPE_DEFAULT;
		if (opt & OPT_QUIT)
			AutoExit = YES;
		if (opt & OPT_SAVEOFF)
			SuppressSave = YES;
		if (opt & OPT_SAVEON)
			SuppressSave = NO;
		if (empty(hostname) && empty(unc)) {
			if (ConnectOnStart == YES)
				PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_CONNECT, 0), 0);
		} else if (empty(hostname) && !empty(unc)) {
			auto u8unc = u8(unc);
			DirectConnectProc(data(u8unc), Kanji, Kana, FnameKanji, TrMode);
		} else if (!empty(hostname) && empty(unc)) {
			auto u8hostname = u8(hostname);
			if (int AutoConnect = SearchHostName(data(u8hostname)); AutoConnect == -1)
				__pragma(warning(suppress:4474)) SetTaskMsg(MSGJPN177, u8hostname.c_str());
			else
				PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_CONNECT_NUM, opt), (LPARAM)AutoConnect);
		} else {
			SetTaskMsg(MSGJPN179);
		}
	}
}


// コマンドラインを解析
static std::optional<int> AnalyzeComLine(std::vector<std::wstring_view> const& args, std::wstring& hostname, std::wstring& unc) {
	const std::map<std::wstring_view, int> map{
		{ L"-m"sv, OPT_MIRROR }, { L"--mirror"sv, OPT_MIRROR },
		{ L"-d"sv, OPT_MIRRORDOWN }, { L"--mirrordown"sv, OPT_MIRRORDOWN },
		{ L"-eu"sv, OPT_EUC }, { L"-e"sv, OPT_EUC }, { L"--euc"sv, OPT_EUC },
		{ L"-ji"sv, OPT_JIS }, { L"-j"sv, OPT_JIS }, { L"--jis"sv, OPT_JIS },
		{ L"-a"sv, OPT_ASCII }, { L"--ascii"sv, OPT_ASCII },
		{ L"-b"sv, OPT_BINARY }, { L"--binary"sv, OPT_BINARY },
		{ L"-x"sv, OPT_AUTO }, { L"--auto"sv, OPT_AUTO },
		{ L"-f"sv, OPT_FORCE }, { L"--force"sv, OPT_FORCE },
		{ L"-q"sv, OPT_QUIT }, { L"--quit"sv, OPT_QUIT },
		{ L"-k"sv, OPT_KANA }, { L"--kana"sv, OPT_KANA },
		{ L"-eun"sv, OPT_EUC_NAME }, { L"-u"sv, OPT_EUC_NAME }, { L"--eucname"sv, OPT_EUC_NAME },
		{ L"-jin"sv, OPT_JIS_NAME }, { L"-i"sv, OPT_JIS_NAME }, { L"--jisname"sv, OPT_JIS_NAME },
		{ L"--saveoff"sv, OPT_SAVEOFF },
		{ L"--saveon"sv, OPT_SAVEON },
		{ L"-sj"sv, OPT_SJIS }, { L"--sjis"sv, OPT_SJIS },
		{ L"-u8"sv, OPT_UTF8N }, { L"--utf8"sv, OPT_UTF8N },
		{ L"-8b"sv, OPT_UTF8BOM }, { L"--utf8bom"sv, OPT_UTF8BOM },
		{ L"-sjn"sv, OPT_SJIS_NAME }, { L"--sjisname"sv, OPT_SJIS_NAME },
		{ L"-u8n"sv, OPT_UTF8N_NAME }, { L"--utf8name"sv, OPT_UTF8N_NAME },
	};
	int option = 0;
	for (auto it = begin(args); it != end(args); ++it) {
		if ((*it)[0] == L'-') {
			auto key = lc(*it);
			if (auto mapit = map.find(key); mapit != end(map)) {
				option |= mapit->second;
			} else if (key == L"-n"sv || key == L"--ini"sv) {
				if (++it == end(args)) {
					SetTaskMsg(MSGJPN282);
					return {};
				}
			} else if (key == L"-z"sv || key == L"--mpasswd"sv) {
				if (++it == end(args)) {
					SetTaskMsg(MSGJPN299);
					return {};
				}
			} else if (key == L"-s"sv || key == L"--set"sv) {
				if (++it == end(args)) {
					SetTaskMsg(MSGJPN178);
					return {};
				}
				hostname = *it;
			} else if (key == L"-h"sv || key == L"--help"sv) {
				ShowHelp(IDH_HELP_TOPIC_0000024);
			} else {
				__pragma(warning(suppress:4474)) SetTaskMsg(MSGJPN180, u8(*it).c_str());
				return {};
			}
		} else
			unc = *it;
	}
	return option;
}


/*----- プログラム終了時の処理 ------------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void ExitProc(HWND hWnd)
{
	CancelFlg = YES;

	// バグ対策
	DisableUserOpe();

	CloseTransferThread();

	if(SaveExit == YES)
	{
		SaveBookMark();
		SaveCurrentSetToHost();
	}
	DeleteAlltempFile();

//	WSACancelBlockingCall();
	DisconnectProc();
//	CloseTransferThread();

	if(SaveExit == YES)
	{
		GetListTabWidth();
		SaveRegistry();
		// ポータブル版判定
		if(RegType == REGTYPE_REG)
			ClearIni();
	}

	fs::remove_all(tempDirectory());

	if(RasClose == YES)
	{
		DisconnectRas(RasCloseNotify != NO);
	}
	DeleteAllObject();
	__pragma(warning(suppress:6387)) HtmlHelpW(NULL, NULL, HH_UNINITIALIZE, dwCookie);
	return;
}


/*----- ファイル名をダブルクリックしたときの処理 ------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Mode : 常に「開く」動作をするかどうか (YES/NO)
*		int App : アプリケーション番号（-1=関連づけ優先）
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoubleClickProc(int Win, int Mode, int App)
{
	int Pos;
	int Type;
	char Local[FMAX_PATH+1];
	char Tmp[FMAX_PATH+1];
	int UseDiffViewer;

	if (!AskUserOpeDisabled())
	{
		SetCurrentDirAsDirHist();
		if(GetSelectedCount(Win) == 1)
		{
			if((Pos = GetFirstSelected(Win, NO)) != -1)
			{
				GetNodeName(Win, Pos, Tmp, FMAX_PATH);
				Type = GetNodeType(Win, Pos);

				if(Win == WIN_LOCAL)
				{
					// ローカルフォルダを開く
//					if((App != -1) || (Type == NODE_FILE))
					if((App != -1) || (Type == NODE_FILE) || (Mode == YES))
					{
						if((DclickOpen == YES) || (Mode == YES))
						{
							strcpy(Local, (AskLocalCurDir() / fs::u8path(Tmp)).u8string().c_str());
							ExecViewer(Local, App);
						}
						else
							PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_UPLOAD, 0), 0);
					}
					else
						ChangeDir(WIN_LOCAL, Tmp);
				}
				else if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
				{
					if((App != -1) || (Type == NODE_FILE))
					{
						if((DclickOpen == YES) || (Mode == YES))
						{
							// ビューワ２、３のパスが "d " で始まっていたら差分ビューア使用
							if ((App == 1 || App == 2) && strncmp(ViewerName[App], "d ", 2) == 0)
								UseDiffViewer = YES;
							else
								UseDiffViewer = NO;

							auto remoteDir = tempDirectory() / L"file";
							fs::create_directory(remoteDir);
							auto remotePath = (remoteDir / (UseDiffViewer == YES ? L"remote." + u8(Tmp) : u8(Tmp))).u8string();

							if(AskTransferNow() == YES)
								SktShareProh();

	//						MainTransPkt.ctrl_skt = AskCmdCtrlSkt();
							strcpy(MainTransPkt.Cmd, "RETR ");
							if(AskHostType() == HTYPE_ACOS)
							{
								strcpy(MainTransPkt.RemoteFile, "'");
								strcat(MainTransPkt.RemoteFile, AskHostLsName().c_str());
								strcat(MainTransPkt.RemoteFile, "(");
								strcat(MainTransPkt.RemoteFile, Tmp);
								strcat(MainTransPkt.RemoteFile, ")");
								strcat(MainTransPkt.RemoteFile, "'");
							}
							else if(AskHostType() == HTYPE_ACOS_4)
							{
								strcpy(MainTransPkt.RemoteFile, Tmp);
							}
							else
							{
								strcpy(MainTransPkt.RemoteFile, Tmp);
							}
							strcpy(MainTransPkt.LocalFile, data(remotePath));
							MainTransPkt.Type = AskTransferTypeAssoc(MainTransPkt.RemoteFile, AskTransferType());
							MainTransPkt.Size = 1;
							MainTransPkt.KanjiCode = AskHostKanjiCode();
							MainTransPkt.KanjiCodeDesired = AskLocalKanjiCode();
							MainTransPkt.KanaCnv = AskHostKanaCnv();
							MainTransPkt.Mode = EXIST_OVW;
							// ミラーリング設定追加
							MainTransPkt.NoTransfer = NO;
							MainTransPkt.ExistSize = 0;
							MainTransPkt.hWndTrans = NULL;

							/* 不正なパスを検出 */
							int Sts = 0;
							DisableUserOpe();
							if (CheckPathViolation(MainTransPkt) == NO) {
								CancelFlg = NO;
								Sts = DoDownload(AskCmdCtrlSkt(), MainTransPkt, NO, &CancelFlg);
								if (MarkAsInternet == YES && IsZoneIDLoaded() == YES)
									MarkFileAsDownloadedFromInternet(data(remotePath));
							}
							EnableUserOpe();

							AddTempFileList(data(remotePath));
							if(Sts/100 == FTP_COMPLETE) {
								if (UseDiffViewer == YES) {
									strcpy(Local, (AskLocalCurDir() / fs::u8path(Tmp)).u8string().c_str());
									ExecViewer2(Local, data(remotePath), App);
								} else {
									ExecViewer(data(remotePath), App);
								}
							}
						}
						else
							PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_DOWNLOAD, 0), 0);
					}
					else
						ChangeDir(WIN_REMOTE, Tmp);
				}
			}
		}
		MakeButtonsFocus();
	}
	return;
}


/*----- フォルダの移動 --------------------------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		char *Path : 移動先のパス名
*
*	Return Value
*		なし
*
*	Note
*		フォルダ同時移動の処理も行う
*----------------------------------------------------------------------------*/

static void ChangeDir(int Win, char *Path)
{
	int Sync;
	char Remote[FMAX_PATH+1];

	// 同時接続対応
	CancelFlg = NO;

	// デッドロック対策
	DisableUserOpe();
	Sync = AskSyncMoveMode();
	if(Sync == YES)
	{
		if(strcmp(Path, "..") == 0)
		{
			strcpy(Remote, u8(AskRemoteCurDir()).c_str());
			if (AskLocalCurDir().filename() != u8(GetFileName(Remote)))
				Sync = NO;
		}
	}

	if((Win == WIN_LOCAL) || (Sync == YES))
	{
		if(DoLocalCWD(Path) == FFFTP_SUCCESS)
			GetLocalDirForWnd();
	}

	if((Win == WIN_REMOTE) || (Sync == YES))
	{
		if(CheckClosedAndReconnect() == FFFTP_SUCCESS)
		{
#if defined(HAVE_OPENVMS)
			/* OpenVMSの場合、".DIR;?"を取る */
			if (AskHostType() == HTYPE_VMS)
				ReformVMSDirName(Path, TRUE);
#endif
			if(DoCWD(Path, YES, NO, YES) < FTP_RETRY)
				GetRemoteDirForWnd(CACHE_NORMAL, &CancelFlg);
		}
	}
	// デッドロック対策
	EnableUserOpe();
	return;
}


/*----- ウインドウのサイズ変更の処理 ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void ResizeWindowProc(void)
{
	RECT Rect;

	GetClientRect(GetMainHwnd(), &Rect);
	SendMessageW(GetSbarWnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(Rect.right, Rect.bottom));

	CalcWinSize();
	SetWindowPos(GetMainTbarWnd(), 0, 0, 0, Rect.right, AskToolWinHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetLocalTbarWnd(), 0, 0, AskToolWinHeight(), LocalWidth, AskToolWinHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetRemoteTbarWnd(), 0, LocalWidth + SepaWidth, AskToolWinHeight(), RemoteWidth, AskToolWinHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessageW(GetLocalTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetLocalHistHwnd(), 0, Rect.right, Rect.top, LocalWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessageW(GetRemoteTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetRemoteHistHwnd(), 0, Rect.right, Rect.top, RemoteWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetLocalHwnd(), 0, 0, AskToolWinHeight()*2, LocalWidth, ListHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetRemoteHwnd(), 0, LocalWidth + SepaWidth, AskToolWinHeight()*2, RemoteWidth, ListHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetTaskWnd(), 0, 0, AskToolWinHeight()*2+ListHeight+SepaWidth, ClientWidth, TaskHeight, SWP_NOACTIVATE | SWP_NOZORDER);
}


/*----- ウインドウの各部分のサイズを計算する ----------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void CalcWinSize(void)
{
	RECT Rect;

	GetWindowRect(GetMainHwnd(), &Rect);

	if(Sizing != SW_MAXIMIZE)
	{
		WinWidth = Rect.right - Rect.left;
		WinHeight = Rect.bottom - Rect.top;
	}

	GetClientRect(GetMainHwnd(), &Rect);

	ClientWidth = Rect.right;
	ClientHeight = Rect.bottom;

	SepaWidth = 4;
	LocalWidth = std::clamp(LocalWidth, 0, ClientWidth - SepaWidth);
	RemoteWidth = std::max(0, ClientWidth - LocalWidth - SepaWidth);

	GetClientRect(GetSbarWnd(), &Rect);

	ListHeight = std::max(0L, ClientHeight - AskToolWinHeight() * 2 - TaskHeight - SepaWidth - Rect.bottom);
}


/*----- ディレクトリリストとファイルリストの境界変更処理 ----------------------
*
*	Parameter
*		WPARAM Keys : WM_MOUSEMOVEなどのWPARAMの値
*		int x : マウスカーソルのＸ座標
*		int y : マウスカーソルのＹ座標
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void CheckResizeFrame(WPARAM Keys, int x, int y)
{
	RECT Rect;
	RECT Rect1;

	if((Resizing == RESIZE_OFF) && (Keys == 0))
	{
		if((x >= LocalWidth) && (x <= LocalWidth + SepaWidth) &&
		   (y > AskToolWinHeight()) && (y < (AskToolWinHeight() * 2 + ListHeight)))
		{
			/* 境界位置変更用カーソルに変更 */
			SetCapture(GetMainHwnd());
			hCursor = LoadCursor(GetFtpInst(), MAKEINTRESOURCE(resize_lr_csr));
			SetCursor(hCursor);
			Resizing = RESIZE_PREPARE;
			ResizePos = RESIZE_HPOS;
		}
		else if((y >= AskToolWinHeight()*2+ListHeight) && (y <= AskToolWinHeight()*2+ListHeight+SepaWidth))
		{
			/* 境界位置変更用カーソルに変更 */
			SetCapture(GetMainHwnd());
			hCursor = LoadCursor(GetFtpInst(), MAKEINTRESOURCE(resize_ud_csr));
			SetCursor(hCursor);
			Resizing = RESIZE_PREPARE;
			ResizePos = RESIZE_VPOS;
		}
	}
	else if(Resizing == RESIZE_PREPARE)
	{
		if(Keys & MK_LBUTTON)
		{
			/* 境界位置変更開始 */
			Resizing = RESIZE_ON;
			GetWindowRect(GetMainHwnd(), &Rect);
			GetClientRect(GetSbarWnd(), &Rect1);
			Rect.left += GetSystemMetrics(SM_CXFRAME);
			Rect.right -= GetSystemMetrics(SM_CXFRAME);
			Rect.top += AskToolWinHeight()*2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
			Rect.bottom -= GetSystemMetrics(SM_CYFRAME) + Rect1.bottom;
			ClipCursor(&Rect);
		}
		else
		{
			if(((ResizePos == RESIZE_HPOS) &&
				((x < LocalWidth) || (x > LocalWidth + SepaWidth) ||
				 (y <= AskToolWinHeight()) || (y >= (AskToolWinHeight() * 2 + ListHeight)))) ||
			   ((ResizePos == RESIZE_VPOS) &&
				((y < AskToolWinHeight()*2+ListHeight) || (y > AskToolWinHeight()*2+ListHeight+SepaWidth))))
			{
				/* 元のカーソルに戻す */
				ReleaseCapture();
				hCursor = LoadCursor(NULL, IDC_ARROW);
				SetCursor(hCursor);
				Resizing = RESIZE_OFF;
			}
		}
	}
	else if(Resizing == RESIZE_ON)
	{
		if(ResizePos == RESIZE_HPOS)
			LocalWidth = x;
		else
		{
			GetClientRect(GetMainHwnd(), &Rect);
			GetClientRect(GetSbarWnd(), &Rect1);
			TaskHeight = std::max(0L, Rect.bottom - y - Rect1.bottom);
		}
		ResizeWindowProc();

		if((Keys & MK_LBUTTON) == 0)
		{
			/* 境界位置変更終了 */
			ReleaseCapture();
			ClipCursor(NULL);
			hCursor = LoadCursor(NULL, IDC_ARROW);
			SetCursor(hCursor);
			Resizing = RESIZE_OFF;
		}
	}
	return;
}


/*----- ファイル一覧情報をビューワで表示 --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispDirInfo(void)
{
	char Buf[FMAX_PATH+1];

	strcpy(Buf, MakeCacheFileName(0).u8string().c_str());
	ExecViewer(Buf, 0);
	return;
}


// ビューワを起動
void ExecViewer(char *Fname, int App) {
	/* FindExecutable()は関連付けられたプログラムのパス名にスペースが	*/
	/* 含まれている時、間違ったパス名を返す事がある。					*/
	/* そこで、関連付けられたプログラムの起動はShellExecute()を使う。	*/
	char ComLine[FMAX_PATH * 2 + 3 + 1];
	auto pFname = fs::u8path(Fname);
	if (wchar_t result[MAX_PATH]; App == -1 && pFname.has_extension() && FindExecutableW(pFname.c_str(), nullptr, result) > (HINSTANCE)32) {
		// 拡張子があるので関連付けを実行する
		DoPrintf("ShellExecute - %s", Fname);
		ShellExecuteW(0, L"open", pFname.c_str(), nullptr, AskLocalCurDir().c_str(), SW_SHOW);
	} else if (App == -1 && (GetFileAttributesW(pFname.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) {
		// ディレクトリなのでフォルダを開く
		MakeDistinguishableFileName(ComLine, Fname);
		DoPrintf("ShellExecute - %s", Fname);
		ShellExecuteW(0, L"open", u8(ComLine).c_str(), nullptr, pFname.c_str(), SW_SHOW);
	} else {
		sprintf(ComLine, "%s \"%s\"", ViewerName[App == -1 ? 0 : App], Fname);
		DoPrintf("CreateProcess - %s", ComLine);
		STARTUPINFOW si{ sizeof(STARTUPINFOW), nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, 0, 0, SW_SHOWNORMAL };
		auto wComLine = u8(ComLine);
		if (ProcessInformation pi; !CreateProcessW(nullptr, data(wComLine), nullptr, nullptr, false, 0, nullptr, systemDirectory().c_str(), &si, &pi)) {
			SetTaskMsg(MSGJPN182, GetLastError());
			SetTaskMsg(">>%s", ComLine);
		}
	}
}


/*----- 差分表示ビューワを起動 ------------------------------------------------
*
*	Parameter
*		char Fname1 : ファイル名
*		char Fname2 : ファイル名2
*		int App : アプリケーション番号（2 or 3）
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ExecViewer2(char *Fname1, char *Fname2, int App)
{
	char AssocProg[FMAX_PATH+1];
	char ComLine[FMAX_PATH*2+3+1];

	/* FindExecutable()は関連付けられたプログラムのパス名にスペースが	*/
	/* 含まれている時、間違ったパス名を返す事がある。					*/
	/* そこで、関連付けられたプログラムの起動はShellExecute()を使う。	*/

	strcpy(AssocProg, ViewerName[App] + 2);	/* 先頭の "d " は読み飛ばす */

	if(strchr(Fname1, ' ') == NULL && strchr(Fname2, ' ') == NULL)
		sprintf(ComLine, "%s %s %s", AssocProg, Fname1, Fname2);
	else
		sprintf(ComLine, "%s \"%s\" \"%s\"", AssocProg, Fname1, Fname2);

	DoPrintf("FindExecutable - %s", ComLine);

	STARTUPINFOW si{ sizeof(STARTUPINFOW), nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, 0, 0, 0, SW_SHOWNORMAL };
	auto wComLine = u8(ComLine);
	if (ProcessInformation pi; !CreateProcessW(nullptr, data(wComLine), nullptr, nullptr, false, 0, nullptr, systemDirectory().c_str(), &si, &pi)) {
		SetTaskMsg(MSGJPN182, GetLastError());
		SetTaskMsg(">>%s", ComLine);
	}
}


// テンポラリファイル名をテンポラリファイルリストに追加
void AddTempFileList(fs::path const& file) {
	TempFiles.push_back(file);
}


// テンポラリファイルリストに登録されているファイルを全て削除
static void DeleteAlltempFile() {
	for (auto const& file : TempFiles)
		fs::remove(file);
	doDeleteRemoteFile();
}


// Ａｂｏｕｔダイアログボックス
static void AboutDialog(HWND hWnd) {
	struct About {
		using result_t = int;
		static INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, ABOUT_URL, EM_LIMITTEXT, 256, 0);
			SetText(hDlg, ABOUT_URL, WebURL);
			return TRUE;
		}
		static void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
			case IDCANCEL:
				EndDialog(hDlg, 0);
				break;
			}
		}
	};
	Dialog(GetFtpInst(), about_dlg, hWnd, About{});
}


void ShowHelp(DWORD_PTR helpTopicId) {
	hHelpWin = HtmlHelpW(NULL, helpPath().c_str(), HH_HELP_CONTEXT, helpTopicId);
}


// INIファイルのパス名を返す
fs::path const& AskIniFilePath() {
	return IniPath;
}

/*----- INIファイルのみを使うかどうかを返す -----------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス : YES/NO
*----------------------------------------------------------------------------*/

int AskForceIni(void)
{
	return(ForceIni);
}




/*----- メッセージ処理 --------------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 終了フラグ (YES=WM_CLOSEが来た/NO)
*----------------------------------------------------------------------------*/

int BackgrndMessageProc(void)
{
	MSG Msg;
	int Ret;

	Ret = NO;
	while(PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if(!IsMainThread() || __pragma(warning(suppress:6387)) !HtmlHelpW(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)&Msg))
		{
			/* ディレクトリ名の表示コンボボックスでBSやRETが効くように */
			/* コンボボックス内ではアクセラレータを無効にする */
			if((Msg.hwnd == GetLocalHistEditHwnd()) ||
			   (Msg.hwnd == GetRemoteHistEditHwnd()) ||
			   ((hHelpWin != NULL) && (Msg.hwnd == hHelpWin)) ||
				AskUserOpeDisabled() ||
			   (TranslateAcceleratorW(GetMainHwnd(), Accel, &Msg) == 0))
			{
				if(Msg.message == WM_QUIT)
				{
					Ret = YES;
					PostQuitMessage(0);
					break;
				}
				TranslateMessage(&Msg);
				DispatchMessageW(&Msg);
			}
		}
	}
	return(Ret);
}


/*----- 自動終了フラグをクリアする --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ResetAutoExitFlg(void)
{
	AutoExit = NO;
	return;
}


/*----- 自動終了フラグを返す --------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int フラグ (YES/NO)
*----------------------------------------------------------------------------*/

int AskAutoExit(void)
{
	return(AutoExit);
}

/*----- ユーザにパスワードを入力させ，それを設定する -----------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int : 0/ユーザキャンセル, 1/設定した, 2/デフォルト設定
*----------------------------------------------------------------------------*/
int EnterMasterPasswordAndSet(bool newpassword, HWND hWnd)
{
	char buf[MAX_PASSWORD_LEN + 1];
	// パスワードの入力欄を非表示
	// 非表示にしたため新しいパスワードを2回入力させる
	char buf1[MAX_PASSWORD_LEN + 1];
	char *p;

	buf[0] = NUL;
	if (InputDialog(newpassword ? newmasterpasswd_dlg : masterpasswd_dlg, hWnd, NULL, buf, MAX_PASSWORD_LEN + 1, nullptr, IDH_HELP_TOPIC_0000064)){
		// パスワードの入力欄を非表示
		if (newpassword)
		{
			buf1[0] = NUL;
			if (!InputDialog(newmasterpasswd_dlg, hWnd, NULL, buf1, MAX_PASSWORD_LEN + 1, nullptr, IDH_HELP_TOPIC_0000064)){
				return 0;
			}
			if(strcmp(buf, buf1) != 0)
			{
				Message(hWnd, IDS_PASSWORD_ISNOT_IDENTICAL, MB_OK | MB_ICONERROR);
				return 0;
			}
		}
		/* 末尾の空白を削除 */
		RemoveTailingSpaces(buf);
		/* 先頭の空白を削除 */
		for( p = buf; *p == ' '; p++ )
			;
		
		if( p[0] != NUL ){
			SetMasterPassword( p );
			return 1;
		}
		else {
			/* 空の場合はデフォルト値を設定 */
			SetMasterPassword( NULL );
			return 2;
		}
	}
	return 0;
}

// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
BOOL IsMainThread()
{
	if(GetCurrentThreadId() != MainThreadId)
		return FALSE;
	return TRUE;
}

void Restart() {
	STARTUPINFOW si;
	GetStartupInfoW(&si);
	ProcessInformation pi;
	CreateProcessW(nullptr, GetCommandLineW(), nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi);
}

void Terminate()
{
	exit(1);
}

// タスクバー進捗表示
static ComPtr<ITaskbarList3> taskbarList;

int LoadTaskbarList3() {
	if (CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_PPV_ARGS(&taskbarList)) == S_OK)
		return FFFTP_SUCCESS;
	return FFFTP_FAIL;
}

void FreeTaskbarList3() {
	taskbarList.Reset();
}

int IsTaskbarList3Loaded() {
	return taskbarList ? YES : NO;
}

void UpdateTaskbarProgress() {
	if (AskTransferSizeTotal() > 0) {
		taskbarList->SetProgressState(GetMainHwnd(), 0 < AskTransferErrorDisplay() ? TBPF_ERROR : TBPF_NORMAL);
		taskbarList->SetProgressValue(GetMainHwnd(), (ULONGLONG)(AskTransferSizeTotal() - AskTransferSizeLeft()), (ULONGLONG)AskTransferSizeTotal());
	} else
		taskbarList->SetProgressState(GetMainHwnd(), TBPF_NOPROGRESS);
}

// 高DPI対応
int AskToolWinHeight(void)
{
	return(ToolWinHeight);
}
