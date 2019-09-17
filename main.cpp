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
#if _WIN32_WINNT < _WIN32_WINNT_VISTA
#include <muiload.h>
#pragma comment(lib, "muiload.lib")
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif


#define RESIZE_OFF		0		/* ウインドウの区切り位置変更していない */
#define RESIZE_ON		1		/* ウインドウの区切り位置変更中 */
#define RESIZE_PREPARE	2		/* ウインドウの区切り位置変更の準備 */

#define RESIZE_HPOS		0		/* ローカル－ホスト間の区切り位置変更 */
#define RESIZE_VPOS		1		/* リスト－タスク間の区切り位置の変更 */


/*===== プロトタイプ =====*/

static int InitApp(LPSTR lpszCmdLine, int cmdShow);
static int MakeAllWindows(int cmdShow);
static void DeleteAllObject(void);
static LRESULT CALLBACK FtpWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void StartupProc(char *Cmd);
static int AnalyzeComLine(char *Str, int *AutoConnect, int *CmdOption, char *unc, int Max);
static int CheckIniFileName(char *Str, char *Ini);
static int CheckMasterPassword(char *Str, char *Ini);
static int GetTokenAfterOption(char *Str, char *Result, const char* Opt1, const char* Opt2 );
static char *GetToken(char *Str, char *Buf);
static void ExitProc(HWND hWnd);
static void ChangeDir(int Win, char *Path);
static void ResizeWindowProc(void);
static void CalcWinSize(void);
// static void AskWindowPos(HWND hWnd);
static void CheckResizeFrame(WPARAM Keys, int x, int y);
static void DispDirInfo(void);
static void DeleteAlltempFile(void);
static void AboutDialog(HWND hWnd);
static int EnterMasterPasswordAndSet(bool newpassword, HWND hWnd);

/*===== ローカルなワーク =====*/

static const wchar_t FtpClass[] = L"FFFTPWin";
static const wchar_t WebURL[] = L"https://github.com/ffftp/ffftp";

static HINSTANCE hInstFtp;
static HWND hWndFtp;
static HWND hWndCurFocus = NULL;

static HACCEL Accel;
static HBRUSH RootColorBrush = NULL;

static int Resizing = RESIZE_OFF;
static int ResizePos;
static HCURSOR hCursor;

int ClientWidth;
static int ClientHeight;
int SepaWidth;
int RemoteWidth;
int ListHeight;

static TEMPFILELIST *TempFiles = NULL;

static int SaveExit = YES;
static int AutoExit = NO;

static char IniPath[FMAX_PATH+1];
static int ForceIni = NO;

TRANSPACKET MainTransPkt;		/* ファイル転送用パケット */
								/* これを使って転送を行うと、ツールバーの転送 */
								/* 中止ボタンで中止できる */

char TitleHostName[HOST_ADRS_LEN+1];
char FilterStr[FILTER_EXT_LEN+1] = { "*" };
// タイトルバーにユーザー名表示対応
char TitleUserName[USER_NAME_LEN+1];

int CancelFlg;

// 外部アプリケーションへドロップ後にローカル側のファイル一覧に作業フォルダが表示されるバグ対策
//static int SuppressRefresh = 0;
int SuppressRefresh = 0;

static DWORD dwCookie;

// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
static DWORD MainThreadId;
// ローカル側自動更新
HANDLE ChangeNotification = INVALID_HANDLE_VALUE;
// 高DPI対応
static int ToolWinHeight;


/*===== グローバルなワーク =====*/

static HWND hHelpWin = NULL;
std::map<int, std::string> msgs;
HCRYPTPROV HCryptProv;
bool SupportIdn;

/* 設定値 */
int WinPosX = CW_USEDEFAULT;
int WinPosY = 0;
// 機能が増えたためサイズ変更
// VGAサイズに収まるようになっていたのをSVGAサイズに引き上げ
//int WinWidth = 630;
//int WinHeight = 393;
//int LocalWidth = 309;
//int TaskHeight = 50;
//int LocalTabWidth[4] = { 120, 90, 60, 37 };
//int RemoteTabWidth[6] = { 120, 90, 60, 37, 60, 60 };
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
int DebugConsole = NO;
int SaveWinPos = NO;
// アスキーモード判別の改良
//char AsciiExt[ASCII_EXT_LEN+1] = { "*.txt\0*.html\0*.htm\0*.cgi\0*.pl\0" };
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
SOUNDFILE Sound[SOUND_TYPES] = { { NO, "" }, { NO, "" }, { NO, "" } };
int FnameCnv = FNAME_NOCNV;
int TimeOut = 90;
int RmEOF = NO;
int RegType = REGTYPE_REG;
char FwallHost[HOST_ADRS_LEN+1] = { "" };
char FwallUser[USER_NAME_LEN+1] = { "" };
char FwallPass[PASSWORD_LEN+1] = { "" };
int FwallPort = PORT_NOR;
int FwallType = 1;
int FwallDefault = NO;
int FwallSecurity = SECURITY_AUTO;
int FwallResolve = NO;
int FwallLower = NO;
int FwallDelimiter = '@';
// ルータ対策
//int PasvDefault = NO;
int PasvDefault = YES;
char MirrorNoTrn[MIRROR_LEN+1] = { "*.bak\0" };
char MirrorNoDel[MIRROR_LEN+1] = { "" };
int MirrorFnameCnv = NO;
int SplitVertical = YES;
int RasClose = NO;
int RasCloseNotify = YES;
int FileHist = 5;
char DefAttrList[DEFATTRLIST_LEN+1] = { "" };
SIZE HostDlgSize = { -1, -1 };
SIZE BmarkDlgSize = { -1, -1 };
SIZE MirrorDlgSize = { -1, -1 };
int Sizing = SW_RESTORE;
int SortSave = NO;
int QuickAnonymous = YES;
int PassToHist = YES;
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
// ファイルアイコン表示対応
int DispFileIcon = NO;
// タイムスタンプのバグ修正
int DispTimeSeconds = NO;
// ファイルの属性を数字で表示
int DispPermissionsNumber = NO;
// ディレクトリ自動作成
int MakeAllDir = YES;
// UTF-8対応
int LocalKanjiCode = KANJI_SJIS;
// 自動切断対策
int NoopEnable = NO;
// UPnP対応
int UPnPEnabled = NO;
time_t LastDataConnectionTime = 0;
// 全設定暗号化対応
int EncryptAllSettings = NO;
// ローカル側自動更新
int AutoRefreshFileList = YES;
// 古い処理内容を消去
int RemoveOldLog = NO;
// バージョン確認
int ReadOnlySettings = NO;
// ファイル一覧バグ修正
int AbortOnListError = YES;
// ミラーリング設定追加
int MirrorNoTransferContents = NO; 
// FireWall設定追加
int FwallNoSaveUser = NO; 
// ゾーンID設定追加
int MarkAsInternet = YES; 


fs::path systemDirectory() {
	static auto path = [] {
		wchar_t directory[FMAX_PATH];
		auto length = GetSystemDirectoryW(directory, size_as<UINT>(directory));
		assert(0 < length);
		return fs::path{ directory, directory + length };
	}();
	return path;
}


static auto const& moduleDirectory() {
	static const auto directory = [] {
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


// メインルーチン
int WINAPI wWinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPWSTR lpCmdLine, __in int nShowCmd) {
#if _WIN32_WINNT < _WIN32_WINNT_VISTA
	{
		wchar_t moduleFileName[FMAX_PATH];
		GetModuleFileNameW(nullptr, moduleFileName, FMAX_PATH);
		auto const fileName = fs::path{ moduleFileName }.filename();
		hInstFtp = LoadMUILibraryW(fileName.c_str(), MUI_LANGUAGE_NAME, GetUserDefaultUILanguage());
		if (hInstFtp == NULL)
			hInstFtp = LoadMUILibraryW(fileName.c_str(), MUI_LANGUAGE_NAME, LANG_NEUTRAL);
	}
#else
	hInstFtp = hInstance;
#endif
	EnumResourceNames(GetFtpInst(), RT_STRING, [](auto hModule, auto lpType, auto lpName, auto lParam) -> BOOL {
		wchar_t buffer[1024];
		if (IS_INTRESOURCE(lpName))
			for (int id = (PtrToInt(lpName) - 1) * 16, end = id + 16; id < end; id++)
				if (auto length = LoadStringW(hModule, id, buffer, size_as<int>(buffer)); 0 < length)
					msgs.emplace(id, u8(buffer, length));
		return true;
	}, 0);

	MSG Msg;
	int Ret;
	BOOL Sts;

	// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
#ifdef DISABLE_MULTI_CPUS
	SetProcessAffinityMask(GetCurrentProcess(), 1);
#endif
	MainThreadId = GetCurrentThreadId();

	// yutaka
	if(OleInitialize(NULL) != S_OK){
		MessageBox(NULL, MSGJPN298, "FFFTP", MB_OK | MB_ICONERROR);
		return 0;
	}

	InitCommonControls();

	// UPnP対応
	CoInitialize(NULL);
	LoadUPnP();
	// タスクバー進捗表示
	LoadTaskbarList3();
	// ゾーンID設定追加
	LoadZoneID();

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
	// Vista以降およびIE7以降で導入済みとなる
	SupportIdn = [] {
		if (auto module = LoadLibraryW((systemDirectory() / L"Normaliz.dll").c_str()); module == NULL)
			return false;
		__HrLoadAllImportsForDll("Normaliz.dll");
		return true;
	}();
#else
	SupportIdn = true;
#endif

	if (!CryptAcquireContextW(&HCryptProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
		Message(IDS_ERR_CRYPTO, MB_OK | MB_ICONERROR);
		return 0;
	}

	if (!LoadSSL()) {
		Message(IDS_ERR_SSL, MB_OK | MB_ICONERROR);
		return 0;
	}

	Ret = FALSE;
	if (auto u8CmdLine = u8(lpCmdLine); InitApp(data(u8CmdLine), nShowCmd) == FFFTP_SUCCESS) {
		for(;;)
		{
			Sts = GetMessageW(&Msg, NULL, 0, 0);
			if((Sts == 0) || (Sts == -1))
				break;

			if(!HtmlHelpW(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)&Msg))
			{ 
				/* ディレクトリ名の表示コンボボックスでBSやRETが効くように */
				/* コンボボックス内ではアクセラレータを無効にする */
				if((Msg.hwnd == GetLocalHistEditHwnd()) ||
				   (Msg.hwnd == GetRemoteHistEditHwnd()) ||
				   ((hHelpWin != NULL) && (GetAncestor(Msg.hwnd, GA_ROOT) == hHelpWin)) ||
				   GetHideUI() == YES ||
				   (TranslateAcceleratorW(GetMainHwnd(), Accel, &Msg) == 0))
				{
					TranslateMessage(&Msg);
					DispatchMessageW(&Msg);
				}
			}
		}
		Ret = (int)Msg.wParam;
	}
	UnregisterClassW(FtpClass, GetFtpInst());
	FreeSSL();
	CryptReleaseContext(HCryptProv, 0);
	// ゾーンID設定追加
	FreeZoneID();
	// タスクバー進捗表示
	FreeTaskbarList3();
	// UPnP対応
	FreeUPnP();
	CoUninitialize();
	OleUninitialize();
	return(Ret);
}


// アプリケーションの初期設定
static int InitApp(LPSTR lpszCmdLine, int cmdShow)
{
	int sts;
	int Err;
	WSADATA WSAData;
	char PwdBuf[FMAX_PATH+1];
	int useDefautPassword = 0; /* 警告文表示用 */
	int masterpass;
	// ポータブル版判定
	int ImportPortable;
	// 高DPI対応
	int i;

	sts = FFFTP_FAIL;
	
	HtmlHelpW(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&dwCookie);

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

		if(CheckIniFileName(lpszCmdLine, IniPath) == 0)
		{
			strcpy(IniPath, (moduleDirectory() / L"ffftp.ini").u8string().c_str());
		}
		else
		{
			ForceIni = YES;
			RegType = REGTYPE_INI;
		}
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
					switch(MessageBox(GetMainHwnd(), MSGJPN350, "FFFTP", MB_YESNOCANCEL | MB_DEFBUTTON2))
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

//		AllocConsole();

		/* 2010.02.01 genta マスターパスワードを入力させる
		  -z オプションがあるときは最初だけスキップ
		  -z オプションがないときは，デフォルトパスワードをまず試す
		  LoadRegistry()する
		  パスワードが不一致なら再入力するか尋ねる．
		  (破損していた場合はさせない)
		*/
		if( CheckMasterPassword(lpszCmdLine, PwdBuf))
		{
			SetMasterPassword( PwdBuf );
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
				if( MessageBox(NULL, MSGJPN304, "FFFTP", MB_YESNO | MB_ICONEXCLAMATION) == IDNO ){
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

			if(MakeAllWindows(cmdShow) == FFFTP_SUCCESS)
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
						"Copyright (C) 2018-2019, KURATA Sayuri."
					);

					if(ForceIni)
						SetTaskMsg("%s%s", MSGJPN283, IniPath);

					DoPrintf("Help=%s", helpPath().u8string().c_str());

					DragAcceptFiles(GetRemoteHwnd(), TRUE);
					DragAcceptFiles(GetLocalHwnd(), TRUE);

					SetAllHistoryToMenu();
					GetLocalDirForWnd();
					MakeButtonsFocus();
					DispTransferFiles();

					StartupProc(lpszCmdLine);
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


/*----- ウインドウを作成する --------------------------------------------------
*
*	Parameter
*		int cmdShow : 最初に表示するウインドウの形式。
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int MakeAllWindows(int cmdShow)
{
	RECT Rect1;
	RECT Rect2;
	int Sts;
	int StsTask;
	int StsSbar;
	int StsTbar;
	int StsList;
	int StsLvtips;
	int StsSocket;

	/*===== メインウインドウ =====*/

	RootColorBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));

	WNDCLASSEXW classEx{ sizeof(WNDCLASSEXW), 0, FtpWndProc, 0, 0, GetFtpInst(), LoadIconW(GetFtpInst(), MAKEINTRESOURCEW(ffftp)), 0, RootColorBrush, MAKEINTRESOURCEW(main_menu), FtpClass };
	RegisterClassExW(&classEx);

	// 高DPI対応
//	ToolWinHeight = TOOLWIN_HEIGHT;
	ToolWinHeight = CalcPixelY(16) + 12;

	if(SaveWinPos == NO)
	{
		WinPosX = CW_USEDEFAULT;
		WinPosY = 0;
	}
	hWndFtp = CreateWindowExW(0, FtpClass, L"FFFTP", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WinPosX, WinPosY, WinWidth, WinHeight, HWND_DESKTOP, 0, GetFtpInst(), nullptr);

	if(hWndFtp != NULL)
	{
		SystemParametersInfoW(SPI_GETWORKAREA, 0, &Rect1, 0);
		GetWindowRect(GetMainHwnd(), &Rect2);
		if(Rect2.bottom > Rect1.bottom)
		{
			Rect2.top = max1(0, Rect2.top - (Rect2.bottom - Rect1.bottom));
			MoveWindow(GetMainHwnd(), Rect2.left, Rect2.top, WinWidth, WinHeight, FALSE);
		}

		/*===== ステイタスバー =====*/

		StsSbar = MakeStatusBarWindow();

		CalcWinSize();

		/*===== ツールバー =====*/

		StsTbar = MakeToolBarWindow();

		/*===== ファイルリストウインドウ =====*/

		StsList = MakeListWin();

		/*==== タスクウインドウ ====*/

		StsTask = MakeTaskWindow();

		if((cmdShow != SW_MINIMIZE) && (cmdShow != SW_SHOWMINIMIZED) && (cmdShow != SW_SHOWMINNOACTIVE) &&
		   (Sizing == SW_MAXIMIZE))
			cmdShow = SW_MAXIMIZE;

		ShowWindow(GetMainHwnd(), cmdShow);

		/*==== ソケットウインドウ ====*/

		StsSocket = MakeSocketWin();

		StsLvtips = InitListViewTips();
	}

	Sts = FFFTP_SUCCESS;
	if((hWndFtp == NULL) ||
	   (StsTbar == FFFTP_FAIL) ||
	   (StsList == FFFTP_FAIL) ||
	   (StsSbar == FFFTP_FAIL) ||
	   (StsTask == FFFTP_FAIL) ||
	   (StsLvtips == FFFTP_FAIL) ||
	   (StsSocket == FFFTP_FAIL))
	{
		Sts = FFFTP_FAIL;
	}

	if(Sts == FFFTP_SUCCESS)
		SetListViewType();

	return(Sts);
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
	LPTOOLTIPTEXT lpttt;
	// UTF-8対応
	LPTOOLTIPTEXTW wlpttt;
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
					if(AskUserOpeDisabled() == NO)
					{
						FindNextChangeNotification(ChangeNotification);
						if(AutoRefreshFileList == YES)
						{
							char Name[FMAX_PATH+1];
							int Pos;
							std::vector<FILELIST> Base;
							MakeSelectedFileList(WIN_LOCAL, NO, NO, Base, &CancelFlg);
							GetHotSelected(WIN_LOCAL, Name);
							Pos = (int)SendMessage(GetLocalHwnd(), LVM_GETTOPINDEX, 0, 0);
							GetLocalDirForWnd();
							SelectFileInList(GetLocalHwnd(), SELECT_LIST, Base);
							SetHotSelected(WIN_LOCAL, Name);
							SendMessage(GetLocalHwnd(), LVM_ENSUREVISIBLE, (WPARAM)(SendMessage(GetLocalHwnd(), LVM_GETITEMCOUNT, 0, 0) - 1), (LPARAM)TRUE);
							SendMessage(GetLocalHwnd(), LVM_ENSUREVISIBLE, (WPARAM)Pos, (LPARAM)TRUE);
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
					// デッドロック対策
					if(AskUserOpeDisabled() == YES)
						break;
					SuppressRefresh = 1;
					SetCurrentDirAsDirHist();
					ChangeDir(WIN_REMOTE, "..");
					SuppressRefresh = 0;
					break;

				case MENU_LOCAL_UPDIR :
					// デッドロック対策
					if(AskUserOpeDisabled() == YES)
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
						SendMessage(GetLocalHwnd(), WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
						SendMessage(GetRemoteHwnd(), WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
						SendMessage(GetTaskWnd(), WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
					}
					GetLocalDirForWnd();
					DispTransferType();
					CheckHistoryNum(0);
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
					// デッドロック対策
					if(AskUserOpeDisabled() == YES)
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
					// デッドロック対策
					if(AskUserOpeDisabled() == YES)
						break;
					GetLocalDirForWnd();
					break;

				case REFRESH_REMOTE :
					// デッドロック対策
					if(AskUserOpeDisabled() == YES)
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
					// デッドロック対策
					if(AskUserOpeDisabled() == YES)
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
						MessageBox(hWnd, MSGJPN292, "FFFTP", MB_OK);
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
				// UTF-8対応
//				case TTN_NEEDTEXT:
				case TTN_NEEDTEXTW:
					lpttt = (LPTOOLTIPTEXT)lParam;
					// UTF-8対応
					// lptttは単なる警告回避用
					wlpttt = (LPTOOLTIPTEXTW)lParam;
					lpttt->hinst = GetFtpInst();
					switch(lpttt->hdr.idFrom)
					{
						case MENU_CONNECT :
							lpttt->lpszText = MSGJPN154;
							break;

						case MENU_QUICK :
							lpttt->lpszText = MSGJPN155;
							break;

						case MENU_DISCONNECT :
							lpttt->lpszText = MSGJPN156;
							break;

						case MENU_DOWNLOAD :
							lpttt->lpszText = MSGJPN157;
							break;
#if defined(HAVE_TANDEM)
						case MENU_DOWNLOAD_AS :
							lpttt->lpszText = MSGJPN065;
							break;

						case MENU_UPLOAD_AS :
							lpttt->lpszText = MSGJPN064;
							break;
#endif
						case MENU_UPLOAD :
							lpttt->lpszText = MSGJPN158;
							break;

						case MENU_MIRROR_UPLOAD :
							lpttt->lpszText = MSGJPN159;
							break;

						case MENU_DELETE :
							lpttt->lpszText = MSGJPN160;
							break;

						case MENU_RENAME :
							lpttt->lpszText = MSGJPN161;
							break;

						case MENU_MKDIR :
							lpttt->lpszText = MSGJPN162;
							break;

						case MENU_LOCAL_UPDIR :
						case MENU_REMOTE_UPDIR :
							lpttt->lpszText = MSGJPN163;
							break;

						case MENU_LOCAL_CHDIR :
						case MENU_REMOTE_CHDIR :
							lpttt->lpszText = MSGJPN164;
							break;

						case MENU_TEXT :
							lpttt->lpszText = MSGJPN165;
							break;

						case MENU_BINARY :
							lpttt->lpszText = MSGJPN166;
							break;

						case MENU_AUTO :
							lpttt->lpszText = MSGJPN167;
							break;

						case MENU_REFRESH :
							lpttt->lpszText = MSGJPN168;
							break;

						case MENU_LIST :
							lpttt->lpszText = MSGJPN169;
							break;

						case MENU_REPORT :
							lpttt->lpszText = MSGJPN170;
							break;

						case MENU_KNJ_SJIS :
							lpttt->lpszText = MSGJPN307;
							break;

						case MENU_KNJ_EUC :
							lpttt->lpszText = MSGJPN171;
							break;

						case MENU_KNJ_JIS :
							lpttt->lpszText = MSGJPN172;
							break;

						case MENU_KNJ_UTF8N :
							lpttt->lpszText = MSGJPN308;
							break;

						case MENU_KNJ_UTF8BOM :
							lpttt->lpszText = MSGJPN330;
							break;

						case MENU_KNJ_NONE :
							lpttt->lpszText = MSGJPN173;
							break;

						case MENU_L_KNJ_SJIS :
							lpttt->lpszText = MSGJPN309;
							break;

						case MENU_L_KNJ_EUC :
							lpttt->lpszText = MSGJPN310;
							break;

						case MENU_L_KNJ_JIS :
							lpttt->lpszText = MSGJPN311;
							break;

						case MENU_L_KNJ_UTF8N :
							lpttt->lpszText = MSGJPN312;
							break;

						case MENU_L_KNJ_UTF8BOM :
							lpttt->lpszText = MSGJPN331;
							break;

						case MENU_KANACNV :
							lpttt->lpszText = MSGJPN174;
							break;

						case MENU_SYNC :
							lpttt->lpszText = MSGJPN175;
							break;

						case MENU_ABORT :
							lpttt->lpszText = MSGJPN176;
							break;
					}
					// UTF-8対応
					// UTF-8からUTF-16 LEへ変換
					{
						static wchar_t StringBufferUTF16[1024];
						if(lpttt->lpszText)
						{
							MtoW(StringBufferUTF16, sizeof(StringBufferUTF16)/ sizeof(wchar_t), lpttt->lpszText, -1);
							wlpttt->lpszText = StringBufferUTF16;
						}
					}
					break;

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


/*----- プログラム開始時の処理 ------------------------------------------------
*
*	Parameter
*		char *Cmd : コマンドライン文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void StartupProc(char *Cmd)
{
	int Sts;
	int AutoConnect;
	int CmdOption;
	int Kanji;
	int Kana;
	int FnameKanji;
	int TrMode;
	char unc[FMAX_PATH+1];

	Sts = AnalyzeComLine(Cmd, &AutoConnect, &CmdOption, unc, FMAX_PATH);

	TrMode = TYPE_DEFAULT;
	Kanji = KANJI_NOCNV;
	FnameKanji = KANJI_NOCNV;
	Kana = YES;
	if(CmdOption & OPT_ASCII)
		TrMode = TYPE_A;
	if(CmdOption & OPT_BINARY)
		TrMode = TYPE_I;
	if(CmdOption & OPT_EUC)
		Kanji = KANJI_EUC;
	if(CmdOption & OPT_JIS)
		Kanji = KANJI_JIS;
	if(CmdOption & OPT_EUC_NAME)
		FnameKanji = KANJI_EUC;
	if(CmdOption & OPT_JIS_NAME)
		FnameKanji = KANJI_JIS;
	if(CmdOption & OPT_KANA)
		Kana = NO;

	if(CmdOption & OPT_QUIT)
		AutoExit = YES;

	if(CmdOption & OPT_SAVEOFF)
		SuppressSave = YES;
	if(CmdOption & OPT_SAVEON)
		SuppressSave = NO;

	// UTF-8対応
	if(CmdOption & OPT_SJIS)
		Kanji = KANJI_SJIS;
	if(CmdOption & OPT_UTF8N)
		Kanji = KANJI_UTF8N;
	if(CmdOption & OPT_UTF8BOM)
		Kanji = KANJI_UTF8BOM;
	if(CmdOption & OPT_SJIS_NAME)
		FnameKanji = KANJI_SJIS;
	if(CmdOption & OPT_UTF8N_NAME)
		FnameKanji = KANJI_UTF8N;

	if(Sts == 0)
	{
		if(ConnectOnStart == YES)
			PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_CONNECT, 0), 0);
	}
	else if(Sts == 1)
	{
		DirectConnectProc(unc, Kanji, Kana, FnameKanji, TrMode);
	}
	else if(Sts == 2)
	{
		PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_CONNECT_NUM, CmdOption), (LPARAM)AutoConnect);
	}
	return;
}


/*----- コマンドラインを解析 --------------------------------------------------
*
*	Parameter
*		char *Str : コマンドライン文字列
*		int *AutoConnect : 接続ホスト番号を返すワーク
*		int *CmdOption : オプションを返すワーク
*		char *unc : uncを返すワーク
*		int Max : uncの最大長
*
*	Return Value
*		int ステータス
*			0=指定なし、1=URL指定、2=設定名指定、-1=エラー
*
*	Note
*		-m	--mirror
*		-d	--mirrordown
*		-f	--force
*		-q	--quit
*		-s	--set
*		-h	--help
*		-e	--euc
*		-j	--jis
*		-a	--ascii
*		-b	--binary
*		-x	--auto
*		-k	--kana
*		-u	--eucname
*		-i	--jisname
*		-n  --ini		(CheckIniFileNameで検索)
*			--saveoff
*			--saveon
*		-z	--mpasswd	(CheckMasterPasswordで検索)	2010.01.30 genta 追加
*----------------------------------------------------------------------------*/

static int AnalyzeComLine(char *Str, int *AutoConnect, int *CmdOption, char *unc, int Max)
{
	int Ret;
	char Tmp[FMAX_PATH+1];

	*AutoConnect = -1;
	*CmdOption = 0;

	Ret = 0;
	memset(unc, NUL, Max+1);

	while((Ret != -1) && ((Str = GetToken(Str, Tmp)) != NULL))
	{
		if(Tmp[0] == '-')
		{
			_strlwr(Tmp);
			if((strcmp(&Tmp[1], "m") == 0) || (strcmp(&Tmp[1], "-mirror") == 0))
				*CmdOption |= OPT_MIRROR;
			else if((strcmp(&Tmp[1], "d") == 0) || (strcmp(&Tmp[1], "-mirrordown") == 0))
				*CmdOption |= OPT_MIRRORDOWN;
			// 廃止予定
//			else if((strcmp(&Tmp[1], "e") == 0) || (strcmp(&Tmp[1], "-euc") == 0))
//				*CmdOption |= OPT_EUC;
//			else if((strcmp(&Tmp[1], "j") == 0) || (strcmp(&Tmp[1], "-jis") == 0))
//				*CmdOption |= OPT_JIS;
			else if((strcmp(&Tmp[1], "eu") == 0) || (strcmp(&Tmp[1], "e") == 0) || (strcmp(&Tmp[1], "-euc") == 0))
				*CmdOption |= OPT_EUC;
			else if((strcmp(&Tmp[1], "ji") == 0) || (strcmp(&Tmp[1], "j") == 0) || (strcmp(&Tmp[1], "-jis") == 0))
				*CmdOption |= OPT_JIS;
			else if((strcmp(&Tmp[1], "a") == 0) || (strcmp(&Tmp[1], "-ascii") == 0))
				*CmdOption |= OPT_ASCII;
			else if((strcmp(&Tmp[1], "b") == 0) || (strcmp(&Tmp[1], "-binary") == 0))
				*CmdOption |= OPT_BINARY;
			else if((strcmp(&Tmp[1], "x") == 0) || (strcmp(&Tmp[1], "-auto") == 0))
				*CmdOption |= OPT_AUTO;
			else if((strcmp(&Tmp[1], "f") == 0) || (strcmp(&Tmp[1], "-force") == 0))
				*CmdOption |= OPT_FORCE;
			else if((strcmp(&Tmp[1], "q") == 0) || (strcmp(&Tmp[1], "-quit") == 0))
				*CmdOption |= OPT_QUIT;
			else if((strcmp(&Tmp[1], "k") == 0) || (strcmp(&Tmp[1], "-kana") == 0))
				*CmdOption |= OPT_KANA;
			// 廃止予定
//			else if((strcmp(&Tmp[1], "u") == 0) || (strcmp(&Tmp[1], "-eucname") == 0))
//				*CmdOption |= OPT_EUC_NAME;
//			else if((strcmp(&Tmp[1], "i") == 0) || (strcmp(&Tmp[1], "-jisname") == 0))
//				*CmdOption |= OPT_JIS_NAME;
			else if((strcmp(&Tmp[1], "eun") == 0) || (strcmp(&Tmp[1], "u") == 0) || (strcmp(&Tmp[1], "-eucname") == 0))
				*CmdOption |= OPT_EUC_NAME;
			else if((strcmp(&Tmp[1], "jin") == 0) || (strcmp(&Tmp[1], "i") == 0) || (strcmp(&Tmp[1], "-jisname") == 0))
				*CmdOption |= OPT_JIS_NAME;
			else if((strcmp(&Tmp[1], "n") == 0) || (strcmp(&Tmp[1], "-ini") == 0))
			{
				if((Str = GetToken(Str, Tmp)) == NULL)
				{
					SetTaskMsg(MSGJPN282);
					Ret = -1;
				}
			}
			else if(strcmp(&Tmp[1], "-saveoff") == 0)
				*CmdOption |= OPT_SAVEOFF;
			else if(strcmp(&Tmp[1], "-saveon") == 0)
				*CmdOption |= OPT_SAVEON;
			else if((strcmp(&Tmp[1], "z") == 0) || (strcmp(&Tmp[1], "-mpasswd") == 0))
			{	/* 2010.01.30 genta : Add master password option */
				if((Str = GetToken(Str, Tmp)) == NULL)
				{
					SetTaskMsg(MSGJPN299);
					Ret = -1;
				}
			}
			else if((strcmp(&Tmp[1], "s") == 0) || (strcmp(&Tmp[1], "-set") == 0))
			{
				if(Ret == 0)
				{
					if((Str = GetToken(Str, Tmp)) != NULL)
					{
						if((*AutoConnect = SearchHostName(Tmp)) != -1)
							Ret = 2;
						else
						{
							__pragma(warning(suppress:4474)) SetTaskMsg(MSGJPN177, Tmp);
							Ret = -1;
						}
					}
					else
					{
						SetTaskMsg(MSGJPN178);
						Ret = -1;
					}
				}
				else
				{
					SetTaskMsg(MSGJPN179);
					Ret = -1;
				}
			}
			else if((strcmp(&Tmp[1], "h") == 0) || (strcmp(&Tmp[1], "-help") == 0))
			{
				ShowHelp(IDH_HELP_TOPIC_0000024);
			}
			// UTF-8対応
			else if((strcmp(&Tmp[1], "sj") == 0) || (strcmp(&Tmp[1], "-sjis") == 0))
				*CmdOption |= OPT_SJIS;
			else if((strcmp(&Tmp[1], "u8") == 0) || (strcmp(&Tmp[1], "-utf8") == 0))
				*CmdOption |= OPT_UTF8N;
			else if((strcmp(&Tmp[1], "8b") == 0) || (strcmp(&Tmp[1], "-utf8bom") == 0))
				*CmdOption |= OPT_UTF8BOM;
			else if((strcmp(&Tmp[1], "sjn") == 0) || (strcmp(&Tmp[1], "-sjisname") == 0))
				*CmdOption |= OPT_SJIS_NAME;
			else if((strcmp(&Tmp[1], "u8n") == 0) || (strcmp(&Tmp[1], "-utf8name") == 0))
				*CmdOption |= OPT_UTF8N_NAME;
			else
			{
				__pragma(warning(suppress:4474)) SetTaskMsg(MSGJPN180, Tmp);
				Ret = -1;
			}
		}
		else
		{
			if(Ret == 0)
			{
				strncpy(unc, Tmp, Max);
				Ret = 1;
			}
			else
			{
				SetTaskMsg(MSGJPN181);
				Ret = -1;
			}
		}
	}
	return(Ret);
}


/*----- INIファイルのパス名の指定をチェック ------------------------------------
*
*	Parameter
*		char *Str : コマンドライン文字列
*		char *Ini : iniファイル名を返すワーク
*
*	Return Value
*		int ステータス
*			0=指定なし、1=あり
*
*	Note
*		-n  --ini
*----------------------------------------------------------------------------*/

static int CheckIniFileName(char *Str, char *Ini)
{
	return GetTokenAfterOption( Str, Ini, "n", "-ini" );
}

/* マスターパスワードの指定をチェック */
static int CheckMasterPassword(char *Str, char *Ini)
{
	return GetTokenAfterOption( Str, Ini, "z", "-mpasswd" );
}

/*----- オプションの後ろのトークンを取り出す ------------------------------------
*
*	Parameter
*		char *Str : コマンドライン文字列
*		char *Result : 取り出した文字列を格納するワーク
*		const char* Opt1, *Opt2: オプション文字列(2つ)
*
*	Return Value
*		int ステータス
*			0=指定なし、1=あり
*
*	Note
*		2010.01.30 genta マスターパスワード取り出しのため共通化
*----------------------------------------------------------------------------*/
static int GetTokenAfterOption(char *Str, char *Result, const char* Opt1, const char* Opt2 )
{
	int Ret = 0;
	char Tmp[FMAX_PATH+1];

	Result[0] = NUL;
	while((Str = GetToken(Str, Tmp)) != NULL)
	{
		if(Tmp[0] == '-')
		{
			_strlwr(Tmp);
			if((strcmp(&Tmp[1], Opt1) == 0) || (strcmp(&Tmp[1], Opt2) == 0))
			{
				if((Str = GetToken(Str, Result)) != NULL)
					Ret = 1;
				break;
			}
		}
	}
	return(Ret);
}

/*----- トークンを返す --------------------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char *Buf : 文字列を返すバッファ
*
*	Return Value
*		char *返したトークンの末尾
*			NULL=終わり
*----------------------------------------------------------------------------*/

static char *GetToken(char *Str, char *Buf)
{
	int InQuote;

	while(*Str != NUL)
	{
		if((*Str != ' ') && (*Str != '\t'))
			break;
		Str++;
	}

	if(*Str != NUL)
	{
		InQuote = 0;
		while(*Str != NUL)
		{
			if(*Str == '\"')
				InQuote = !InQuote;
			else
			{
				if(((*Str == ' ') || (*Str == '\t')) &&
				   (InQuote == 0))
				{
					break;
				}
				*Buf++ = *Str;
			}
			Str++;
		}
	}
	else
		Str = NULL;

	*Buf = NUL;

	return(Str);
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
	HtmlHelpW(NULL, NULL, HH_UNINITIALIZE, dwCookie); 
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
	int Sts;
	int UseDiffViewer;

	if(AskUserOpeDisabled() == NO)
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
							AskLocalCurDir(Local, FMAX_PATH);
							ReplaceAll(Local, '/', '\\');
							SetYenTail(Local);
							strcat(Local, Tmp);
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
							MainTransPkt.Next = NULL;

							DisableUserOpe();

							/* 不正なパスを検出 */
							if(CheckPathViolation(&MainTransPkt) == NO)
							{
//								if((Sts = DoDownload(AskCmdCtrlSkt(), &MainTransPkt, NO)) == 429)
//								{
//									ReConnectCmdSkt();
									// 同時接続対応
									CancelFlg = NO;
									Sts = DoDownload(AskCmdCtrlSkt(), &MainTransPkt, NO, &CancelFlg);
									// ゾーンID設定追加
									if(MarkAsInternet == YES && IsZoneIDLoaded() == YES)
										MarkFileAsDownloadedFromInternet(data(remotePath));
//								}
							}

							EnableUserOpe();

							AddTempFileList(data(remotePath));
							if(Sts/100 == FTP_COMPLETE) {
								if (UseDiffViewer == YES) {
									AskLocalCurDir(Local, FMAX_PATH);
									ReplaceAll(Local, '/', '\\');
									SetYenTail(Local);
									strcat(Local, Tmp);
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
	char Local[FMAX_PATH+1];
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
			AskLocalCurDir(Local, FMAX_PATH);
			AskRemoteCurDir(Remote, FMAX_PATH);
			if(strcmp(GetFileName(Local), GetFileName(Remote)) != 0)
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
	SendMessage(GetSbarWnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(Rect.right, Rect.bottom));

	CalcWinSize();
	SetWindowPos(GetMainTbarWnd(), 0, 0, 0, Rect.right, AskToolWinHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetLocalTbarWnd(), 0, 0, AskToolWinHeight(), LocalWidth, AskToolWinHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetRemoteTbarWnd(), 0, LocalWidth + SepaWidth, AskToolWinHeight(), RemoteWidth, AskToolWinHeight(), SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessage(GetLocalTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetLocalHistHwnd(), 0, Rect.right, Rect.top, LocalWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessage(GetRemoteTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
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
	LocalWidth = max1(0, min1(LocalWidth, ClientWidth - SepaWidth));
	RemoteWidth = max1(0, ClientWidth - LocalWidth - SepaWidth);
//	TaskHeight = min1(TaskHeight, max1(0, ClientHeight - TOOLWIN_HEIGHT * 2 - SepaWidth));

	GetClientRect(GetSbarWnd(), &Rect);

	// 高DPI対応
//	ListHeight = max1(0, ClientHeight - TOOLWIN_HEIGHT * 2 - TaskHeight - SepaWidth - Rect.bottom);
	ListHeight = max1(0, ClientHeight - AskToolWinHeight() * 2 - TaskHeight - SepaWidth - Rect.bottom);

	return;
}


#if 0
/*----- ウインドウの表示位置を取得する ----------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AskWindowPos(HWND hWnd)
{
	WINDOWPLACEMENT WinPlace;

	WinPlace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hWnd, &WinPlace);
	WinPosX = WinPlace.rcNormalPosition.left;
	WinPosY = WinPlace.rcNormalPosition.top;

	return;
}
#endif


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
		// 高DPI対応
//		if((x >= LocalWidth) && (x <= LocalWidth + SepaWidth) &&
//		   (y > TOOLWIN_HEIGHT) && (y < (TOOLWIN_HEIGHT * 2 + ListHeight)))
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
		// 高DPI対応
//		else if((y >= TOOLWIN_HEIGHT*2+ListHeight) && (y <= TOOLWIN_HEIGHT*2+ListHeight+SepaWidth))
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
			// 高DPI対応
//			Rect.top += TOOLWIN_HEIGHT*2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
			Rect.top += AskToolWinHeight()*2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
			Rect.bottom -= GetSystemMetrics(SM_CYFRAME) + Rect1.bottom;
			ClipCursor(&Rect);
		}
		else
		{
			// 高DPI対応
//			if(((ResizePos == RESIZE_HPOS) &&
//				((x < LocalWidth) || (x > LocalWidth + SepaWidth) ||
//				 (y <= TOOLWIN_HEIGHT) || (y >= (TOOLWIN_HEIGHT * 2 + ListHeight)))) ||
//			   ((ResizePos == RESIZE_VPOS) &&
//				((y < TOOLWIN_HEIGHT*2+ListHeight) || (y > TOOLWIN_HEIGHT*2+ListHeight+SepaWidth))))
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
			TaskHeight = max1(0, Rect.bottom - y - Rect1.bottom);
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
		char CurDir[FMAX_PATH + 1];
		AskLocalCurDir(CurDir, FMAX_PATH);
		ShellExecuteW(0, L"open", pFname.c_str(), nullptr, fs::u8path(CurDir).c_str(), SW_SHOW);
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


/*----- テンポラリファイル名をテンポラリファイルリストに追加 ------------------
*
*	Parameter
*		char *Fname : テンポラリファイル名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AddTempFileList(char *Fname)
{
	TEMPFILELIST *New;

	if((New = (TEMPFILELIST*)malloc(sizeof(TEMPFILELIST))) != NULL)
	{
		if((New->Fname = (char*)malloc(strlen(Fname)+1)) != NULL)
		{
			strcpy(New->Fname, Fname);
			if(TempFiles == NULL)
				New->Next = NULL;
			else
				New->Next = TempFiles;
			TempFiles = New;
		}
		else
			free(New);
	}
	return;
}


/*----- テンポラリファイルリストに登録されているファイルを全て削除 ------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DeleteAlltempFile(void)
{
	TEMPFILELIST *Pos;
	TEMPFILELIST *Next;

	Pos = TempFiles;
	while(Pos != NULL)
	{
		fs::remove(fs::u8path(Pos->Fname));

		Next = Pos->Next;
		free(Pos->Fname);
		free(Pos);
		Pos = Next;
	}

	// OLE D&Dのテンポラリを削除する (2007.9.11 yutaka)
	doDeleteRemoteFile();

	return;
}


// Ａｂｏｕｔダイアログボックス
static void AboutDialog(HWND hWnd) {
	struct About {
		using result_t = int;
		static INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, ABOUT_URL, EM_LIMITTEXT, 256, 0);
			SendDlgItemMessageW(hDlg, ABOUT_URL, WM_SETTEXT, 0, (LPARAM)WebURL);
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


// サウンドを鳴らす
void SoundPlay(int Num) {
	if (Sound[Num].On == YES)
		sndPlaySoundW(u8(Sound[Num].Fname).c_str(), SND_ASYNC | SND_NODEFAULT);
}


void ShowHelp(DWORD_PTR helpTopicId) {
	hHelpWin = HtmlHelpW(NULL, helpPath().c_str(), HH_HELP_CONTEXT, helpTopicId);
}


/*----- INIファイルのパス名を返す ---------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *パス名
*----------------------------------------------------------------------------*/

char *AskIniFilePath(void)
{
	return(IniPath);
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
		if(!IsMainThread() || !HtmlHelpW(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)&Msg))
		{
			/* ディレクトリ名の表示コンボボックスでBSやRETが効くように */
			/* コンボボックス内ではアクセラレータを無効にする */
			if((Msg.hwnd == GetLocalHistEditHwnd()) ||
			   (Msg.hwnd == GetRemoteHistEditHwnd()) ||
			   ((hHelpWin != NULL) && (Msg.hwnd == hHelpWin)) ||
			   GetHideUI() == YES ||
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
				MessageBox(hWnd, MSGJPN325, "FFFTP", MB_OK | MB_ICONERROR);
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
