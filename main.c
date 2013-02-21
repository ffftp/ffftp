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

#define  STRICT
// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
// 自動切断対策
#include <time.h>
#include <malloc.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdarg.h>
// IPv6対応
//#include <winsock.h>

#include "common.h"
#include "resource.h"
#include "aes.h"
// 暗号化通信対応
#include "sha.h"

#include <htmlhelp.h>
#include "helpid.h"

// UTF-8対応
#undef __MBSWRAPPER_H__
#include "mbswrapper.h"


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
// 64ビット対応
//static BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int EnterMasterPasswordAndSet( int Res, HWND hWnd );

/*===== ローカルなワーク =====*/

static const char FtpClassStr[] = "FFFTPWin";

static HINSTANCE hInstFtp;
static HWND hWndFtp = NULL;
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

static char HelpPath[FMAX_PATH+1];
static char IniPath[FMAX_PATH+1];
static int ForceIni = NO;

TRANSPACKET MainTransPkt;		/* ファイル転送用パケット */
								/* これを使って転送を行うと、ツールバーの転送 */
								/* 中止ボタンで中止できる */

char TitleHostName[HOST_ADRS_LEN+1];
char FilterStr[FILTER_EXT_LEN+1] = { "*" };

int CancelFlg;

// 外部アプリケーションへドロップ後にローカル側のファイル一覧に作業フォルダが表示されるバグ対策
//static int SuppressRefresh = 0;
int SuppressRefresh = 0;

static DWORD dwCookie;

// 暗号化通信対応
static char SSLRootCAFilePath[FMAX_PATH+1];
// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
static DWORD MainThreadId;
// ポータブル版判定
static char PortableFilePath[FMAX_PATH+1];
static int PortableVersion;
// ローカル側自動更新
HANDLE ChangeNotification = INVALID_HANDLE_VALUE;


/*===== グローバルなワーク =====*/

HWND hHelpWin = NULL;

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
LOGFONT ListLogFont;
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
// LISTのキャッシュを無効にする（リモートのディレクトリの表示が更新されないバグ対策）
//int CacheEntry = 10;
int CacheEntry = -10;
int CacheSave = NO;
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
int FwallResolv = NO;
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
char TmpPath[FMAX_PATH+1];
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
// 暗号化通信対応
BYTE CertificateCacheHash[MAX_CERT_CACHE_HASH][20];
BYTE SSLRootCAFileHash[20];
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
int UPnPEnabled = YES;
time_t LastDataConnectionTime = 0;





/*----- メインルーチン --------------------------------------------------------
*
*	Parameter
*		HINSTANCE hInstance : このアプリケーションのこのインスタンスのハンドル
*		HINSTANCE hPrevInstance : このアプリケーションの直前のインスタンスのハンドル
*		LPSTR lpszCmdLine : アプリケーションが起動したときのコマンドラインをさすロングポインタ
*		int cmdShow : 最初に表示するウインドウの形式。
*
*	Return Value
*		int 最後のメッセージのwParam
*----------------------------------------------------------------------------*/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
    MSG Msg;
	int Ret;
	BOOL Sts;

	// プロセス保護
#ifdef ENABLE_PROCESS_PROTECTION
	{
		DWORD ProtectLevel;
		char* pCommand;
		char Option[FMAX_PATH+1];
		ProtectLevel = PROCESS_PROTECTION_NONE;
		pCommand = lpszCmdLine;
		while(pCommand = GetToken(pCommand, Option))
		{
			if(strcmp(Option, "--protect") == 0)
			{
				ProtectLevel = PROCESS_PROTECTION_DEFAULT;
				break;
			}
			else if(strcmp(Option, "--protect-high") == 0)
			{
				ProtectLevel = PROCESS_PROTECTION_HIGH;
				break;
			}
			else if(strcmp(Option, "--protect-medium") == 0)
			{
				ProtectLevel = PROCESS_PROTECTION_MEDIUM;
				break;
			}
			else if(strcmp(Option, "--protect-low") == 0)
			{
				ProtectLevel = PROCESS_PROTECTION_LOW;
				break;
			}
		}
		if(ProtectLevel != PROCESS_PROTECTION_NONE)
		{
			SetProcessProtectionLevel(ProtectLevel);
			if(!InitializeLoadLibraryHook())
			{
				MessageBox(NULL, MSGJPN321, "FFFTP", MB_OK | MB_ICONERROR);
				return 0;
			}
#ifndef _DEBUG
			if(IsDebuggerPresent())
			{
				MessageBox(NULL, MSGJPN322, "FFFTP", MB_OK | MB_ICONERROR);
				return 0;
			}
#endif
			if(!UnloadUntrustedModule())
			{
				MessageBox(NULL, MSGJPN323, "FFFTP", MB_OK | MB_ICONERROR);
				return 0;
			}
#ifndef _DEBUG
			if(RestartProtectedProcess(" --restart"))
				return 0;
#endif
			if(!EnableLoadLibraryHook(TRUE))
			{
				MessageBox(NULL, MSGJPN324, "FFFTP", MB_OK | MB_ICONERROR);
				return 0;
			}
		}
		else
			InitializeLoadLibraryHook();
	}
#endif

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

	// FTPS対応
#ifdef USE_OPENSSL
	LoadOpenSSL();
#endif

	Ret = FALSE;
	hWndFtp = NULL;
	hInstFtp = hInstance;
	if(InitApp(lpszCmdLine, cmdShow) == FFFTP_SUCCESS)
	{
		for(;;)
		{
			Sts = GetMessage(&Msg, NULL, 0, 0);
			if((Sts == 0) || (Sts == -1))
				break;

			// 64ビット対応
//			if(!HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD)&Msg))
			if(!HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)&Msg))
			{ 
				/* ディレクトリ名の表示コンボボックスでBSやRETが効くように */
				/* コンボボックス内ではアクセラレータを無効にする */
				if((Msg.hwnd == GetLocalHistEditHwnd()) ||
				   (Msg.hwnd == GetRemoteHistEditHwnd()) ||
				   ((hHelpWin != NULL) && (GetAncestor(Msg.hwnd, GA_ROOT) == hHelpWin)) ||
				   GetHideUI() == YES ||
				   (TranslateAccelerator(hWndFtp, Accel, &Msg) == 0))
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
		}
		Ret = Msg.wParam;
	}
    UnregisterClass(FtpClassStr, hInstFtp);
	// FTPS対応
#ifdef USE_OPENSSL
	FreeOpenSSL();
#endif
	// UPnP対応
	FreeUPnP();
	CoUninitialize();
	OleUninitialize();
	return(Ret);
}


/*----- アプリケーションの初期設定 --------------------------------------------
*
*	Parameter
*		HINSTANCE hInstance : このアプリケーションのこのインスタンスのハンドル
*		HINSTANCE hPrevInstance : このアプリケーションの直前のインスタンスのハンドル
*		LPSTR lpszCmdLine : アプリケーションが起動したときのコマンドラインをさすロングポインタ
*		int cmdShow : 最初に表示するウインドウの形式。
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

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

	sts = FFFTP_FAIL;

	aes_init();
	srand(GetTickCount());
	
	// 64ビット対応
//	HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD)&dwCookie);
	HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&dwCookie);

	SaveUpdateBellInfo();

	if((Err = WSAStartup((WORD)0x0202, &WSAData)) != 0)
		MessageBox(NULL, ReturnWSError(Err), "FFFTP - Startup", MB_OK);
	else
	{
		Accel = LoadAccelerators(hInstFtp, MAKEINTRESOURCE(ffftp_accel));

		// 環境依存の不具合対策
//		GetTempPath(FMAX_PATH, TmpPath);
		GetAppTempPath(TmpPath);
		_mkdir(TmpPath);
		SetYenTail(TmpPath);

		GetModuleFileName(NULL, HelpPath, FMAX_PATH);
		strcpy(GetFileName(HelpPath), "ffftp.chm");

		if(CheckIniFileName(lpszCmdLine, IniPath) == 0)
		{
			GetModuleFileName(NULL, IniPath, FMAX_PATH);
			strcpy(GetFileName(IniPath), "ffftp.ini");
		}
		else
		{
			ForceIni = YES;
			RegType = REGTYPE_INI;
		}
		// ポータブル版判定
		GetModuleFileName(NULL, PortableFilePath, FMAX_PATH);
		strcpy(GetFileName(PortableFilePath), "portable");
		CheckPortableVersion();
		ImportPortable = NO;
		if(PortableVersion == YES)
		{
			ForceIni = YES;
			RegType = REGTYPE_INI;
			if(IsRegAvailable() == YES && IsIniAvailable() == NO)
			{
				if(DialogBox(GetFtpInst(), MAKEINTRESOURCE(ini_from_reg_dlg), GetMainHwnd(), ExeEscDialogProc) == YES)
				{
					ImportPortable = YES;
					ForceIni = NO;
					RegType = REGTYPE_REG;
				}
			}
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
			masterpass = EnterMasterPasswordAndSet(masterpasswd_dlg, NULL);
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
			LoadRegistry();

			// ポータブル版判定
			if(ImportPortable == YES)
			{
				ForceIni = YES;
				RegType = REGTYPE_INI;
			}

			// 暗号化通信対応
			SetSSLTimeoutCallback(TimeOut * 1000, SSLTimeoutCallback);
			SetSSLConfirmCallback(SSLConfirmCallback);
			GetModuleFileName(NULL, SSLRootCAFilePath, FMAX_PATH);
			strcpy(GetFileName(SSLRootCAFilePath), "ssl.pem");
			LoadSSLRootCAFile();

			LoadJre();
			if(NoRasControl == NO)
				LoadRasLib();
			LoadKernelLib();

			//タイマの精度を改善
			timeBeginPeriod(1);

			CountPrevFfftpWindows();

			if(MakeAllWindows(cmdShow) == FFFTP_SUCCESS)
			{
				hWndCurFocus = GetLocalHwnd();

				if(strlen(DefaultLocalPath) > 0)
					SetCurrentDirectory(DefaultLocalPath);

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

				MakeCacheBuf(CacheEntry);
				if(CacheSave == YES)
					LoadCache();

				if(MakeTransferThread() == FFFTP_SUCCESS)
				{
					DoPrintf("DEBUG MESSAGE ON ! ##");

					DispWindowTitle();
					// SourceForge.JPによるフォーク
//					SetTaskMsg("FFFTP Ver." VER_STR " Copyright(C) 1997-2010 Sota & cooperators.");
					SetTaskMsg("FFFTP Ver." VER_STR " Copyright(C) 1997-2010 Sota & cooperators.\r\nCopyright (C) 2011-2013 FFFTP Project (Hiromichi Matsushima, Suguru Kawamoto, IWAMOTO Kouichi, vitamin0x, unarist, Asami, fortran90, tomo1192, Yuji Tanaka, Moriguchi Hirokazu).");

					if(ForceIni)
						SetTaskMsg("%s%s", MSGJPN283, IniPath);

					if(IsFolderExist(TmpPath) == NO)
					{
						SetTaskMsg(MSGJPN152, TmpPath);
						GetTempPath(FMAX_PATH, TmpPath);
						SetTaskMsg(MSGJPN153, TmpPath);
					}

					DoPrintf("Tmp =%s", TmpPath);
					DoPrintf("Help=%s", HelpPath);

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

	// 暗号化通信対応
#ifdef USE_OPENSSL
	if(IsOpenSSLLoaded())
		SetTaskMsg(MSGJPN318);
	else
		SetTaskMsg(MSGJPN319);
#endif

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
	WNDCLASSEX wClass;
	int Sts;
	int StsTask;
	int StsSbar;
	int StsTbar;
	int StsList;
	int StsLvtips;
	int StsSocket;

	/*===== メインウインドウ =====*/

	RootColorBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));

	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.style         = 0;
	wClass.lpfnWndProc   = FtpWndProc;
	wClass.cbClsExtra    = 0;
	wClass.cbWndExtra    = 0;
	wClass.hInstance     = hInstFtp;
	wClass.hIcon         = LoadIcon(hInstFtp, MAKEINTRESOURCE(ffftp));
	wClass.hCursor       = NULL;
	wClass.hbrBackground = RootColorBrush;
	wClass.lpszMenuName  = (LPSTR)MAKEINTRESOURCE(main_menu);
	wClass.lpszClassName = FtpClassStr;
	wClass.hIconSm       = NULL;
	RegisterClassEx(&wClass);

	if(SaveWinPos == NO)
	{
		WinPosX = CW_USEDEFAULT;
		WinPosY = 0;
	}
	hWndFtp = CreateWindow(FtpClassStr, "FFFTP",
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
				WinPosX, WinPosY, WinWidth, WinHeight,
				HWND_DESKTOP, 0, hInstFtp, NULL);

	if(hWndFtp != NULL)
	{
		SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect1, 0);
		GetWindowRect(hWndFtp, &Rect2);
		if(Rect2.bottom > Rect1.bottom)
		{
			Rect2.top = max1(0, Rect2.top - (Rect2.bottom - Rect1.bottom));
			MoveWindow(hWndFtp, Rect2.left, Rect2.top, WinWidth, WinHeight, FALSE);
		}

		/*===== ステイタスバー =====*/

		StsSbar = MakeStatusBarWindow(hWndFtp, hInstFtp);

		CalcWinSize();

		/*===== ツールバー =====*/

		StsTbar = MakeToolBarWindow(hWndFtp, hInstFtp);

		/*===== ファイルリストウインドウ =====*/

		StsList = MakeListWin(hWndFtp, hInstFtp);

		/*==== タスクウインドウ ====*/

		StsTask = MakeTaskWindow(hWndFtp, hInstFtp);

		if((cmdShow != SW_MINIMIZE) && (cmdShow != SW_SHOWMINIMIZED) && (cmdShow != SW_SHOWMINNOACTIVE) &&
		   (Sizing == SW_MAXIMIZE))
			cmdShow = SW_MAXIMIZE;

		ShowWindow(hWndFtp, cmdShow);

		/*==== ソケットウインドウ ====*/

		StsSocket = MakeSocketWin(hWndFtp, hInstFtp);

		StsLvtips = InitListViewTips(hWndFtp, hInstFtp);
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


/*----- ウインドウのタイトルを表示する ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispWindowTitle(void)
{
	char Tmp[HOST_ADRS_LEN+FILTER_EXT_LEN+20];

	if(AskConnecting() == YES)
		sprintf(Tmp, "%s (%s) - FFFTP", TitleHostName, FilterStr);
	else
		sprintf(Tmp, "FFFTP (%s)", FilterStr);

	SetWindowText(GetMainHwnd(), Tmp);
	return;
}


/*----- 全てのオブジェクトを削除 ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DeleteAllObject(void)
{
	DeleteCacheBuf();

//move to WM_DESTROY
	WSACleanup();

//test システム任せ
//	if(ListFont != NULL)
//		DeleteObject(ListFont);
//	if(RootColorBrush != NULL)
//		DeleteObject(RootColorBrush);

//test システム任せ
//	DeleteListViewTips();
//	DeleteListWin();
//	DeleteStatusBarWindow();
//	DeleteTaskWindow();
//	DeleteToolBarWindow();
//	DeleteSocketWin();

//move to WM_DESTROY
	if(hWndFtp != NULL)
		DestroyWindow(hWndFtp);

	ReleaseJre();
	ReleaseRasLib();
	ReleaseKernelLib();

	return;
}


/*----- メインウインドウのウインドウハンドルを返す ----------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetMainHwnd(void)
{
	return(hWndFtp);
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


/*----- プログラムのインスタンスを返す ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HINSTANCE インスタンス
*----------------------------------------------------------------------------*/

HINSTANCE GetFtpInst(void)
{
	return(hInstFtp);
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
		case WM_CREATE :
			SetTimer(hWnd, 1, 1000, NULL);
			break;

		// ローカル側自動更新
		// 自動切断対策
		case WM_TIMER :
			switch(wParam)
			{
			case 1:
				if(WaitForSingleObject(ChangeNotification, 0) == WAIT_OBJECT_0)
				{
					if(AskUserOpeDisabled() == NO)
					{
						FILELIST* Base;
						char Name[FMAX_PATH+1];
						int Pos;
						FindNextChangeNotification(ChangeNotification);
						Base = NULL;
						MakeSelectedFileList(WIN_LOCAL, NO, NO, &Base, &CancelFlg);
						GetHotSelected(WIN_LOCAL, Name);
						Pos = SendMessage(GetLocalHwnd(), LVM_GETTOPINDEX, 0, 0);
						GetLocalDirForWnd();
						SelectFileInList(GetLocalHwnd(), SELECT_LIST, Base);
						SetHotSelected(WIN_LOCAL, Name);
						SendMessage(GetLocalHwnd(), LVM_ENSUREVISIBLE, (WPARAM)(SendMessage(GetLocalHwnd(), LVM_GETITEMCOUNT, 0, 0) - 1), (LPARAM)TRUE);
						SendMessage(GetLocalHwnd(), LVM_ENSUREVISIBLE, (WPARAM)Pos, (LPARAM)TRUE);
					}
				}
				if(NoopEnable == YES && AskNoopInterval() > 0 && time(NULL) - LastDataConnectionTime >= AskNoopInterval())
				{
					NoopProc(NO);
					LastDataConnectionTime = time(NULL);
				}
				break;
			}
			break;

		case WM_COMMAND :
			// 同時接続対応
			// 中断後に受信バッファに応答が残っていると次のコマンドの応答が正しく処理できない
			if(CancelFlg == YES)
				RemoveReceivedData(AskCmdCtrlSkt());
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
						PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(MENU_LOCAL_UPDIR, 0), 0);
					else
						PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(MENU_REMOTE_UPDIR, 0), 0);
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
					SetOption(0);
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
					// 暗号化通信対応
					SetSSLTimeoutCallback(TimeOut * 1000, SSLTimeoutCallback);
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
					PostMessage(hWnd, WM_CLOSE, 0, 0L);
					break;

				case MENU_AUTO_EXIT :
					if(AutoExit == YES)
						PostMessage(hWnd, WM_CLOSE, 0, 0L);
					break;

				case MENU_ABOUT :
					DialogBox(hInstFtp, MAKEINTRESOURCE(about_dlg), hWnd, AboutDialogProc);
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
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000001);
					break;

				case MENU_HELP_TROUBLE :
					// 任意のコードが実行されるバグ修正
//					ShellExecute(NULL, "open", MYWEB_URL, NULL, ".", SW_SHOW);
					ShellExecute(NULL, "open", MYWEB_URL, NULL, NULL, SW_SHOW);
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
					// ローカル側自動更新
//					SelectFileInList(hWndCurFocus, SELECT_ALL);
					SelectFileInList(hWndCurFocus, SELECT_ALL, NULL);
					break;

				case MENU_SELECT :
					// ローカル側自動更新
//					SelectFileInList(hWndCurFocus, SELECT_REGEXP);
					SelectFileInList(hWndCurFocus, SELECT_REGEXP, NULL);
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
						PostMessage(hWnd, WM_CLOSE, 0, 0L);
					}
					break;

				case MENU_REGINIT :
					if(DialogBox(hInstFtp, MAKEINTRESOURCE(reginit_dlg), hWnd, ExeEscDialogProc) == YES)
					{
						ClearRegistry();
						// ポータブル版判定
						ClearIni();
						SaveExit = NO;
						PostMessage(hWnd, WM_CLOSE, 0, 0L);
					}
					break;
				case MENU_CHANGEPASSWD:	/* 2010.01.31 genta */
					if( GetMasterPasswordStatus() != PASSWORD_OK )
					{
						/* 強制的に設定するか確認 */
						if( DialogBox(hInstFtp, MAKEINTRESOURCE(forcepasschange_dlg), hWnd, ExeEscDialogProc) != YES){
							break;
						}
						// セキュリティ強化
						if(EnterMasterPasswordAndSet(newmasterpasswd_dlg, hWnd) != 0)
							SetTaskMsg(MSGJPN303);
					}
					// セキュリティ強化
//					if( EnterMasterPasswordAndSet( newmasterpasswd_dlg, hWnd ) != 0 ){
//						SetTaskMsg( MSGJPN303 );
//					}
					else if(GetMasterPasswordStatus() == PASSWORD_OK)
					{
						char Password[MAX_PASSWORD_LEN + 1];
						GetMasterPassword(Password);
						SetMasterPassword(NULL);
						while(ValidateMasterPassword() == YES && GetMasterPasswordStatus() == PASSWORD_UNMATCH)
						{
							if(EnterMasterPasswordAndSet(masterpasswd_dlg, NULL) == 0)
								break;
						}
						if(GetMasterPasswordStatus() == PASSWORD_OK && EnterMasterPasswordAndSet(newmasterpasswd_dlg, hWnd) != 0)
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

				case MENU_URL_COPY :
					CopyURLtoClipBoard();
					break;

				case MENU_APPKEY :
					EraseListViewTips();
					if(hWndCurFocus == GetRemoteHwnd())
						RemoteRbuttonMenu(1);
					else if(hWndCurFocus == GetLocalHwnd())
						LocalRbuttonMenu(1);
					break;

#if defined(HAVE_TANDEM)
				case MENU_SWITCH_OSS :
					SwitchOSSProc();
					break;
#endif

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
					lpttt->hinst = hInstFtp;
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
					DispSelectedSpace();
					MakeButtonsFocus();
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
				return(DefWindowProc(hWnd, message, wParam, lParam));
			break;

		case WM_MOVING :
			WinPosX = ((RECT *)lParam)->left;
			WinPosY = ((RECT *)lParam)->top;
			return(DefWindowProc(hWnd, message, wParam, lParam));

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
//			PostMessage(hWnd,  WM_COMMAND, MAKEWPARAM(REFRESH_LOCAL, 0), 0);
			if(SuppressRefresh == 0)
				PostMessage(hWnd,  WM_COMMAND, MAKEWPARAM(REFRESH_LOCAL, 0), 0);
			break;

		case WM_REFRESH_REMOTE_FLG :
			if(SuppressRefresh == 0)
				PostMessage(hWnd,  WM_COMMAND, MAKEWPARAM(REFRESH_REMOTE, 0), 0);
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
//			WSACleanup();
//			DestroyWindow(hWndFtp);
			PostQuitMessage(0);
			break;

		case WM_QUERYENDSESSION :
			ExitProc(hWnd);
			return(TRUE);

		case WM_CLOSE :
			if((AskTransferNow() == NO) ||
			   (DialogBox(hInstFtp, MAKEINTRESOURCE(exit_dlg), hWnd, ExeEscDialogProc) == YES))
			{
				ExitProc(hWnd);
				return(DefWindowProc(hWnd, message, wParam, lParam));
			}
			break;

		default :
			return(DefWindowProc(hWnd, message, wParam, lParam));
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
			PostMessage(hWndFtp, WM_COMMAND, MAKEWPARAM(MENU_CONNECT, 0), 0);
	}
	else if(Sts == 1)
	{
		DirectConnectProc(unc, Kanji, Kana, FnameKanji, TrMode);
	}
	else if(Sts == 2)
	{
		PostMessage(hWndFtp, WM_COMMAND, MAKEWPARAM(MENU_CONNECT_NUM, CmdOption), (LPARAM)AutoConnect);
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
							SetTaskMsg(MSGJPN177, Tmp);
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
				hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000024);
			}
			// プロセス保護
#ifdef ENABLE_PROCESS_PROTECTION
			else if(strcmp(Tmp, "--restart") == 0)
			{
			}
			else if(strcmp(Tmp, "--protect") == 0)
			{
			}
			else if(strcmp(Tmp, "--protect-high") == 0)
			{
			}
			else if(strcmp(Tmp, "--protect-medium") == 0)
			{
			}
			else if(strcmp(Tmp, "--protect-low") == 0)
			{
			}
#endif
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
				SetTaskMsg(MSGJPN180, Tmp);
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
			if(*Str == 0x22)
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
	// 環境依存の不具合対策
	char Tmp[FMAX_PATH+1];

	CancelFlg = YES;

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

		if((CacheEntry > 0) && (CacheSave == YES))
			SaveCache();
		else
			DeleteCache();
	}
	else
		DeleteCache();

	// 環境依存の不具合対策
	GetAppTempPath(Tmp);
	SetYenTail(Tmp);
	strcat(Tmp, "file");
	_rmdir(Tmp);
	GetAppTempPath(Tmp);
	_rmdir(Tmp);

	if(RasClose == YES)
	{
		DisconnectRas(RasCloseNotify);
	}
	DeleteAllObject();
	HtmlHelp(NULL, NULL, HH_UNINITIALIZE, dwCookie); 
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
	char Remote[FMAX_PATH+1];
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
							PostMessage(hWndFtp, WM_COMMAND, MAKEWPARAM(MENU_UPLOAD, 0), 0);
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

							strcpy(Remote, TmpPath);
							SetYenTail(Remote);
							// 環境依存の不具合対策
							strcat(Remote, "file");
							_mkdir(Remote);
							SetYenTail(Remote);
							if (UseDiffViewer == YES) {
								strcat(Remote, "remote.");
							}
							strcat(Remote, Tmp);

							if(AskTransferNow() == YES)
								SktShareProh();

	//						MainTransPkt.ctrl_skt = AskCmdCtrlSkt();
							strcpy(MainTransPkt.Cmd, "RETR ");
							if(AskHostType() == HTYPE_ACOS)
							{
								strcpy(MainTransPkt.RemoteFile, "'");
								strcat(MainTransPkt.RemoteFile, AskHostLsName());
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
							strcpy(MainTransPkt.LocalFile, Remote);
							MainTransPkt.Type = AskTransferTypeAssoc(MainTransPkt.RemoteFile, AskTransferType());
							MainTransPkt.Size = 1;
							MainTransPkt.KanjiCode = AskHostKanjiCode();
							MainTransPkt.KanjiCodeDesired = AskLocalKanjiCode();
							MainTransPkt.KanaCnv = AskHostKanaCnv();
							MainTransPkt.Mode = EXIST_OVW;
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
//								}
							}

							EnableUserOpe();

							AddTempFileList(Remote);
							if(Sts/100 == FTP_COMPLETE) {
								if (UseDiffViewer == YES) {
									AskLocalCurDir(Local, FMAX_PATH);
									ReplaceAll(Local, '/', '\\');
									SetYenTail(Local);
									strcat(Local, Tmp);
									ExecViewer2(Local, Remote, App);
								} else {
									ExecViewer(Remote, App);
								}
							}
						}
						else
							PostMessage(hWndFtp, WM_COMMAND, MAKEWPARAM(MENU_DOWNLOAD, 0), 0);
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
#if 0
	RECT Rect;
	int RemotePosX;

	GetClientRect(hWndFtp, &Rect);
	SendMessage(GetSbarWnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(Rect.right, Rect.bottom));

	CalcWinSize();
	SetWindowPos(GetMainTbarWnd(), 0, 0, 0, WinWidth, TOOLWIN_HEIGHT, SWP_NOACTIVATE | SWP_NOZORDER);

	SetWindowPos(GetLocalTbarWnd(), 0, 0, TOOLWIN_HEIGHT, LocalWidth, TOOLWIN_HEIGHT, SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessage(GetLocalTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetLocalHistHwnd(), 0, Rect.right, Rect.top, LocalWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetLocalHwnd(), 0, 0, TOOLWIN_HEIGHT*2, LocalWidth, ListHeight, SWP_NOACTIVATE | SWP_NOZORDER);

	RemotePosX = LocalWidth + SepaWidth;
	if(SplitVertical == YES)
		RemotePosX = 0;

	SetWindowPos(GetRemoteTbarWnd(), 0, RemotePosX, TOOLWIN_HEIGHT, RemoteWidth, TOOLWIN_HEIGHT, SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessage(GetRemoteTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetRemoteHistHwnd(), 0, Rect.right, Rect.top, RemoteWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetRemoteHwnd(), 0, RemotePosX, TOOLWIN_HEIGHT*2, RemoteWidth, ListHeight, SWP_NOACTIVATE | SWP_NOZORDER);

	SetWindowPos(GetTaskWnd(), 0, 0, TOOLWIN_HEIGHT*2+ListHeight+SepaWidth, ClientWidth, TaskHeight, SWP_NOACTIVATE | SWP_NOZORDER);
#else
	RECT Rect;

	GetClientRect(hWndFtp, &Rect);
	SendMessage(GetSbarWnd(), WM_SIZE, SIZE_RESTORED, MAKELPARAM(Rect.right, Rect.bottom));

	CalcWinSize();
	SetWindowPos(GetMainTbarWnd(), 0, 0, 0, Rect.right, TOOLWIN_HEIGHT, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetLocalTbarWnd(), 0, 0, TOOLWIN_HEIGHT, LocalWidth, TOOLWIN_HEIGHT, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetRemoteTbarWnd(), 0, LocalWidth + SepaWidth, TOOLWIN_HEIGHT, RemoteWidth, TOOLWIN_HEIGHT, SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessage(GetLocalTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetLocalHistHwnd(), 0, Rect.right, Rect.top, LocalWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SendMessage(GetRemoteTbarWnd(), TB_GETITEMRECT, 3, (LPARAM)&Rect);
	SetWindowPos(GetRemoteHistHwnd(), 0, Rect.right, Rect.top, RemoteWidth - Rect.right, 200, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetLocalHwnd(), 0, 0, TOOLWIN_HEIGHT*2, LocalWidth, ListHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetRemoteHwnd(), 0, LocalWidth + SepaWidth, TOOLWIN_HEIGHT*2, RemoteWidth, ListHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	SetWindowPos(GetTaskWnd(), 0, 0, TOOLWIN_HEIGHT*2+ListHeight+SepaWidth, ClientWidth, TaskHeight, SWP_NOACTIVATE | SWP_NOZORDER);
#endif

	return;
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

	GetWindowRect(hWndFtp, &Rect);

	if(Sizing != SW_MAXIMIZE)
	{
		WinWidth = Rect.right - Rect.left;
		WinHeight = Rect.bottom - Rect.top;
	}

	GetClientRect(hWndFtp, &Rect);

	ClientWidth = Rect.right;
	ClientHeight = Rect.bottom;

	SepaWidth = 4;
	LocalWidth = max1(0, min1(LocalWidth, ClientWidth - SepaWidth));
	RemoteWidth = max1(0, ClientWidth - LocalWidth - SepaWidth);
//	TaskHeight = min1(TaskHeight, max1(0, ClientHeight - TOOLWIN_HEIGHT * 2 - SepaWidth));

	GetClientRect(GetSbarWnd(), &Rect);

	ListHeight = max1(0, ClientHeight - TOOLWIN_HEIGHT * 2 - TaskHeight - SepaWidth - Rect.bottom);

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
		if((x >= LocalWidth) && (x <= LocalWidth + SepaWidth) &&
		   (y > TOOLWIN_HEIGHT) && (y < (TOOLWIN_HEIGHT * 2 + ListHeight)))
		{
			/* 境界位置変更用カーソルに変更 */
			SetCapture(hWndFtp);
			hCursor = LoadCursor(hInstFtp, MAKEINTRESOURCE(resize_lr_csr));
			SetCursor(hCursor);
			Resizing = RESIZE_PREPARE;
			ResizePos = RESIZE_HPOS;
		}
		else if((y >= TOOLWIN_HEIGHT*2+ListHeight) && (y <= TOOLWIN_HEIGHT*2+ListHeight+SepaWidth))
		{
			/* 境界位置変更用カーソルに変更 */
			SetCapture(hWndFtp);
			hCursor = LoadCursor(hInstFtp, MAKEINTRESOURCE(resize_ud_csr));
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
			GetWindowRect(hWndFtp, &Rect);
			GetClientRect(GetSbarWnd(), &Rect1);
			Rect.left += GetSystemMetrics(SM_CXFRAME);
			Rect.right -= GetSystemMetrics(SM_CXFRAME);
			Rect.top += TOOLWIN_HEIGHT*2 + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
			Rect.bottom -= GetSystemMetrics(SM_CYFRAME) + Rect1.bottom;
			ClipCursor(&Rect);
		}
		else
		{
			if(((ResizePos == RESIZE_HPOS) &&
				((x < LocalWidth) || (x > LocalWidth + SepaWidth) ||
				 (y <= TOOLWIN_HEIGHT) || (y >= (TOOLWIN_HEIGHT * 2 + ListHeight)))) ||
			   ((ResizePos == RESIZE_VPOS) &&
				((y < TOOLWIN_HEIGHT*2+ListHeight) || (y > TOOLWIN_HEIGHT*2+ListHeight+SepaWidth))))
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
			GetClientRect(hWndFtp, &Rect);
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

	MakeCacheFileName(AskCurrentFileListNum(), Buf);
	ExecViewer(Buf, 0);
	return;
}



/*----- ビューワを起動 --------------------------------------------------------
*
*	Parameter
*		char Fname : ファイル名
*		int App : アプリケーション番号（-1=関連づけ優先）
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ExecViewer(char *Fname, int App)
{
	PROCESS_INFORMATION Info;
	STARTUPINFO Startup;
	char AssocProg[FMAX_PATH+1];
	char ComLine[FMAX_PATH*2+3+1];
	char CurDir[FMAX_PATH+1];
	// 任意のコードが実行されるバグ修正
	char SysDir[FMAX_PATH+1];

	/* FindExecutable()は関連付けられたプログラムのパス名にスペースが	*/
	/* 含まれている時、間違ったパス名を返す事がある。					*/
	/* そこで、関連付けられたプログラムの起動はShellExecute()を使う。	*/

	AskLocalCurDir(CurDir, FMAX_PATH);

	// 任意のコードが実行されるバグ修正
	// 拡張子が無いと補完されるため
//	if((App == -1) && (FindExecutable(Fname, NULL, AssocProg) > (HINSTANCE)32))
	if((App == -1) && (strlen(GetFileExt(GetFileName(Fname))) > 0) && (FindExecutable(Fname, NULL, AssocProg) > (HINSTANCE)32))
	{
		DoPrintf("ShellExecute - %s", Fname);
		ShellExecute(NULL, "open", Fname, NULL, CurDir, SW_SHOW);
	}
	// ローカルフォルダを開く
	else if((App == -1) && (GetFileAttributes(Fname) & FILE_ATTRIBUTE_DIRECTORY))
	{
		MakeDistinguishableFileName(ComLine, Fname);
		DoPrintf("ShellExecute - %s", Fname);
		ShellExecute(NULL, "open", ComLine, NULL, Fname, SW_SHOW);
	}
	else
	{
		App = max1(0, App);
		strcpy(AssocProg, ViewerName[App]);

		if(strchr(Fname, ' ') == NULL)
			sprintf(ComLine, "%s %s", AssocProg, Fname);
		else
			sprintf(ComLine, "%s \"%s\"", AssocProg, Fname);

		DoPrintf("FindExecutable - %s", ComLine);

		memset(&Startup, NUL, sizeof(STARTUPINFO));
		Startup.cb = sizeof(STARTUPINFO);
		Startup.wShowWindow = SW_SHOW;
		// 任意のコードが実行されるバグ修正
//		if(CreateProcess(NULL, ComLine, NULL, NULL, FALSE, 0, NULL, NULL, &Startup, &Info) == FALSE)
//		{
//			SetTaskMsg(MSGJPN182, GetLastError());
//			SetTaskMsg(">>%s", ComLine);
//		}
		if(GetCurrentDirectory(FMAX_PATH, CurDir) > 0)
		{
			if(GetSystemDirectory(SysDir, FMAX_PATH) > 0)
			{
				if(SetCurrentDirectory(SysDir))
				{
					if(CreateProcess(NULL, ComLine, NULL, NULL, FALSE, 0, NULL, NULL, &Startup, &Info) == FALSE)
					{
						SetTaskMsg(MSGJPN182, GetLastError());
						SetTaskMsg(">>%s", ComLine);
					}
					SetCurrentDirectory(CurDir);
				}
			}
		}
	}
	return;
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
	PROCESS_INFORMATION Info;
	STARTUPINFO Startup;
	char AssocProg[FMAX_PATH+1];
	char ComLine[FMAX_PATH*2+3+1];
	char CurDir[FMAX_PATH+1];
	// 任意のコードが実行されるバグ修正
	char SysDir[FMAX_PATH+1];

	/* FindExecutable()は関連付けられたプログラムのパス名にスペースが	*/
	/* 含まれている時、間違ったパス名を返す事がある。					*/
	/* そこで、関連付けられたプログラムの起動はShellExecute()を使う。	*/

	AskLocalCurDir(CurDir, FMAX_PATH);

	strcpy(AssocProg, ViewerName[App] + 2);	/* 先頭の "d " は読み飛ばす */

	if(strchr(Fname1, ' ') == NULL && strchr(Fname2, ' ') == NULL)
		sprintf(ComLine, "%s %s %s", AssocProg, Fname1, Fname2);
	else
		sprintf(ComLine, "%s \"%s\" \"%s\"", AssocProg, Fname1, Fname2);

	DoPrintf("FindExecutable - %s", ComLine);

	memset(&Startup, NUL, sizeof(STARTUPINFO));
	Startup.cb = sizeof(STARTUPINFO);
	Startup.wShowWindow = SW_SHOW;
	// 任意のコードが実行されるバグ修正
//	if(CreateProcess(NULL, ComLine, NULL, NULL, FALSE, 0, NULL, NULL, &Startup, &Info) == FALSE)
//	{
//		SetTaskMsg(MSGJPN182, GetLastError());
//		SetTaskMsg(">>%s", ComLine);
//	}
	if(GetCurrentDirectory(FMAX_PATH, CurDir) > 0)
	{
		if(GetSystemDirectory(SysDir, FMAX_PATH) > 0)
		{
			if(SetCurrentDirectory(SysDir))
			{
				if(CreateProcess(NULL, ComLine, NULL, NULL, FALSE, 0, NULL, NULL, &Startup, &Info) == FALSE)
				{
					SetTaskMsg(MSGJPN182, GetLastError());
					SetTaskMsg(">>%s", ComLine);
				}
				SetCurrentDirectory(CurDir);
			}
		}
	}

	return;
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

	if((New = malloc(sizeof(TEMPFILELIST))) != NULL)
	{
		if((New->Fname = malloc(strlen(Fname)+1)) != NULL)
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
		DeleteFile(Pos->Fname);

		Next = Pos->Next;
		free(Pos->Fname);
		free(Pos);
		Pos = Next;
	}

	// OLE D&Dのテンポラリを削除する (2007.9.11 yutaka)
	doDeleteRemoteFile();

	return;
}


/*----- Ａｂｏｕｔダイアログボックスのコールバック関数 ------------------------
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

// 64ビット対応
//static BOOL CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char Tmp[80];
	int Ver;

	switch (message)
	{
		case WM_INITDIALOG :
			Ver = GetJreVersion();
			if(Ver == -1)
				sprintf(Tmp, MSGJPN183);
			else
				sprintf(Tmp, MSGJPN184, Ver / 0x100, Ver % 0x100);
			SendDlgItemMessage(hDlg, ABOUT_JRE, WM_SETTEXT, 0, (LPARAM)Tmp);
			SendDlgItemMessage(hDlg, ABOUT_URL, EM_LIMITTEXT, 256, 0);
			SendDlgItemMessage(hDlg, ABOUT_URL, WM_SETTEXT, 0, (LPARAM)MSGJPN284);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
				case IDCANCEL :
					EndDialog(hDlg, YES);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- サウンドを鳴らす ------------------------------------------------------
*
*	Parameter
*		Int num : サウンドの種類 (SND_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SoundPlay(int Num)
{
	if(Sound[Num].On == YES)
		sndPlaySound(Sound[Num].Fname, SND_ASYNC | SND_NODEFAULT);

	return;
}


/*----- ヘルプファイルのパス名を返す ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *パス名
*----------------------------------------------------------------------------*/

char *AskHelpFilePath(void)
{
	return(HelpPath);
}


/*----- テンポラリファイルのパス名を返す --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		char *パス名
*----------------------------------------------------------------------------*/

char *AskTmpFilePath(void)
{
	return(TmpPath);
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
	while(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
//		if(!HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD)&Msg))
		// 64ビット対応
//		if(!IsMainThread() || !HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD)&Msg))
		if(!IsMainThread() || !HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)&Msg))
		{
	 		/* ディレクトリ名の表示コンボボックスでBSやRETが効くように */
			/* コンボボックス内ではアクセラレータを無効にする */
			if((Msg.hwnd == GetLocalHistEditHwnd()) ||
			   (Msg.hwnd == GetRemoteHistEditHwnd()) ||
			   ((hHelpWin != NULL) && (Msg.hwnd == hHelpWin)) ||
			   GetHideUI() == YES ||
			   (TranslateAccelerator(GetMainHwnd(), Accel, &Msg) == 0))
			{
				if(Msg.message == WM_QUIT)
				{
					Ret = YES;
					PostQuitMessage(0);
					break;
				}
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
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
int EnterMasterPasswordAndSet( int Res, HWND hWnd )
{
	char buf[MAX_PASSWORD_LEN + 1];
	// パスワードの入力欄を非表示
	// 非表示にしたため新しいパスワードを2回入力させる
	char buf1[MAX_PASSWORD_LEN + 1];
	char *p;
	int Flag;

	buf[0] = NUL;
	if( InputDialogBox(Res, hWnd, NULL, buf, MAX_PASSWORD_LEN + 1,
		&Flag, IDH_HELP_TOPIC_0000064) == YES){
		// パスワードの入力欄を非表示
		if(Res == newmasterpasswd_dlg)
		{
			buf1[0] = NUL;
			if( InputDialogBox(Res, hWnd, NULL, buf1, MAX_PASSWORD_LEN + 1,
				&Flag, IDH_HELP_TOPIC_0000064) != YES){
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

// 暗号化通信対応
BOOL __stdcall SSLTimeoutCallback(BOOL* pbAborted)
{
	Sleep(1);
	if(BackgrndMessageProc() == YES)
		return TRUE;
	if(*pbAborted == YES)
		return TRUE;
	return FALSE;
}

BOOL __stdcall SSLConfirmCallback(BOOL* pbAborted, BOOL bVerified, LPCSTR Certificate, LPCSTR CommonName)
{
	BOOL bResult;
	uint32 Hash[5];
	int i;
	char* pm0;
	bResult = FALSE;
	sha_memory((char*)Certificate, (uint32)(strlen(Certificate) * sizeof(char)), (uint32*)&Hash);
	// sha.cはビッグエンディアンのため
	for(i = 0; i < 5; i++)
		Hash[i] = _byteswap_ulong(Hash[i]);
	i = 0;
	while(i < MAX_CERT_CACHE_HASH)
	{
		if(memcmp(&CertificateCacheHash[i], &Hash, 20) == 0)
		{
			bResult = TRUE;
			break;
		}
		i++;
	}
	if(!bResult)
	{
		if(pm0 = AllocateStringM(strlen(Certificate) + 1024))
		{
			sprintf(pm0, MSGJPN326, IsHostNameMatched(AskHostAdrs(), CommonName) ? MSGJPN327 : MSGJPN328, bVerified ? MSGJPN327 : MSGJPN328, Certificate);
			if(MessageBox(GetMainHwnd(), pm0, "FFFTP", MB_YESNO) == IDYES)
			{
				for(i = MAX_CERT_CACHE_HASH - 1; i >= 1; i--)
					memcpy(&CertificateCacheHash[i], &CertificateCacheHash[i - 1], 20);
				memcpy(&CertificateCacheHash[0], &Hash, 20);
				bResult = TRUE;
			}
			FreeDuplicatedString(pm0);
		}
	}
	if(!bResult)
		*pbAborted = YES;
	return bResult;
}

BOOL LoadSSLRootCAFile()
{
	BOOL bResult;
	HANDLE hFile;
	DWORD Size;
	BYTE* pBuffer;
	uint32 Hash[5];
	int i;
	bResult = FALSE;
	if((hFile = CreateFile(SSLRootCAFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		Size = GetFileSize(hFile, NULL);
		if(pBuffer = (BYTE*)malloc(Size))
		{
			if(ReadFile(hFile, pBuffer, Size, &Size, NULL))
			{
				sha_memory((char*)pBuffer, (uint32)Size, (uint32*)&Hash);
				// sha.cはビッグエンディアンのため
				for(i = 0; i < 5; i++)
					Hash[i] = _byteswap_ulong(Hash[i]);
				// 同梱する"ssl.pem"に合わせてSHA1ハッシュ値を変更すること
				if(memcmp(&Hash, &SSLRootCAFileHash, 20) == 0 || memcmp(&Hash, "\xD8\x8A\x7B\x2F\xBF\x23\x57\x16\xDA\x02\x14\x2B\xD4\x2E\x09\x80\xA0\x4C\x72\x62", 20) == 0
					|| DialogBox(GetFtpInst(), MAKEINTRESOURCE(updatesslroot_dlg), GetMainHwnd(), ExeEscDialogProc) == YES)
				{
					memcpy(&SSLRootCAFileHash, &Hash, 20);
					if(SetSSLRootCertificate(pBuffer, Size))
						bResult = TRUE;
				}
			}
			free(pBuffer);
		}
		CloseHandle(hFile);
	}
	return bResult;
}

// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
BOOL IsMainThread()
{
	if(GetCurrentThreadId() != MainThreadId)
		return FALSE;
	return TRUE;
}

// ポータブル版判定
void CheckPortableVersion()
{
	HANDLE hFile;
	if((hFile = CreateFile(PortableFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		PortableVersion = YES;
		CloseHandle(hFile);
	}
	else
		PortableVersion = NO;
}

int AskPortableVersion(void)
{
	return(PortableVersion);
}

