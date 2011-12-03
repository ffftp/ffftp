/*=============================================================================
*
*								ファイル一覧
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

// UTF-8対応
//#define _WIN32_WINNT	0x400

#define	STRICT
// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <mbstring.h>
#include <malloc.h>
#include <windowsx.h>
#include <commctrl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

#include "common.h"
#include "resource.h"

#include <htmlhelp.h>
#include "helpid.h"

#include <shlobj.h>
#include "OleDragDrop.h"
#include "common.h"

// UTF-8対応
#undef __MBSWRAPPER_H__
#include "mbswrapper.h"

#define BUF_SIZE		256
#define CF_CNT 2
#define WM_DRAGDROP		(WM_APP + 100)
#define WM_GETDATA		(WM_APP + 101)
#define WM_DRAGOVER		(WM_APP + 102)


/*===== ファイルリストのリスト用ストラクチャ =====*/

typedef struct {
	FILELIST *Top;			/* ファイルリストの先頭 */
	int Files;				/* ファイルの数 */
} FLISTANCHOR;

/*===== プロトタイプ =====*/

static LRESULT CALLBACK LocalWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK RemoteWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT FileListCommonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void AddDispFileList(FLISTANCHOR *Anchor, char *Name, FILETIME *Time, LONGLONG Size, int Attr, int Type, int Link, char *Owner, int InfoExist, int Win);
static void EraseDispFileList(FLISTANCHOR *Anchor);
static void DispFileList2View(HWND hWnd, FLISTANCHOR *Anchor);
static void AddListView(HWND hWnd, int Pos, char *Name, int Type, LONGLONG Size, FILETIME *Time, int Attr, char *Owner, int Link, int InfoExist);
// 64ビット対応
//static BOOL CALLBACK SelectDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK SelectDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static void DispListList(FILELIST *Pos, char *Title);
static void MakeRemoteTree1(char *Path, char *Cur, FILELIST **Base, int *CancelCheckWork);
static void MakeRemoteTree2(char *Path, char *Cur, FILELIST **Base, int *CancelCheckWork);
static void CopyTmpListToFileList(FILELIST **Base, FILELIST *List);
static int GetListOneLine(char *Buf, int Max, FILE *Fd);
static int MakeDirPath(char *Str, int ListType, char *Path, char *Dir);
static void MakeLocalTree(char *Path, FILELIST **Base);
static void AddFileList(FILELIST *Pkt, FILELIST **Base);
static int AnalizeFileInfo(char *Str);
static int CheckUnixType(char *Str, char *Tmp, int Add1, int Add2, int Day);
static int CheckHHMMformat(char *Str);
static int CheckYYMMDDformat(char *Str, char Sym, int Dig3);
static int CheckYYYYMMDDformat(char *Str, char Sym);
static int ResolvFileInfo(char *Str, int ListType, char *Fname, LONGLONG *Size, FILETIME *Time, int *Attr, char *Owner, int *Link, int *InfoExist);
static int FindField(char *Str, char *Buf, int Num, int ToLast);
// MLSD対応
static int FindField2(char *Str, char *Buf, char Separator, int Num, int ToLast);
static void GetMonth(char *Str, WORD *Month, WORD *Day);
static int GetYearMonthDay(char *Str, WORD *Year, WORD *Month, WORD *Day);
static int GetHourAndMinute(char *Str, WORD *Hour, WORD *Minute);
static int GetVMSdate(char *Str, WORD *Year, WORD *Month, WORD *Day);
static int CheckSpecialDirName(char *Fname);
static int AskFilterStr(char *Fname, int Type);
// 64ビット対応
//static BOOL CALLBACK FilterWndProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK FilterWndProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static int atoi_n(const char *Str, int Len);

/*===== 外部参照 =====*/

extern int SepaWidth;
extern int RemoteWidth;
extern int ListHeight;
extern char FilterStr[FILTER_EXT_LEN+1];
extern HWND hHelpWin;

/* 設定値 */
extern int LocalWidth;
extern int LocalTabWidth[4];
extern int RemoteTabWidth[6];
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern HFONT ListFont;
extern int ListType;
extern int FindMode;
extern int DotFile;
extern int DispIgnoreHide;
extern int DispDrives;
extern int MoveMode;

/*===== ローカルなワーク =====*/

static HWND hWndListLocal = NULL;
static HWND hWndListRemote = NULL;

static WNDPROC LocalProcPtr;
static WNDPROC RemoteProcPtr;

static HIMAGELIST ListImg = NULL;

static char FindStr[40+1] = { "*" };		/* 検索文字列 */
static int IgnoreNew = NO;
static int IgnoreOld = NO;
static int IgnoreExist = NO;

static int Dragging = NO;

static int StratusMode;			/* 0=ファイル, 1=ディレクトリ, 2=リンク */


// リモートファイルリスト (2007.9.3 yutaka)
static FILELIST *remoteFileListBase;
static FILELIST *remoteFileListBaseNoExpand;
static char remoteFileDir[FMAX_PATH + 1];


/*----- ファイルリストウインドウを作成する ------------------------------------
*
*	Parameter
*		HWND hWnd : 親ウインドウのウインドウハンドル
*		HINSTANCE hInst : インスタンスハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeListWin(HWND hWnd, HINSTANCE hInst)
{
	int Sts;
	LV_COLUMN LvCol;
	long Tmp;

	/*===== ローカル側のリストビュー =====*/

	hWndListLocal = CreateWindowEx(/*WS_EX_STATICEDGE*/WS_EX_CLIENTEDGE,
			WC_LISTVIEWA, NULL,
			WS_CHILD | /*WS_BORDER | */LVS_REPORT | LVS_SHOWSELALWAYS,
			0, TOOLWIN_HEIGHT*2, LocalWidth, ListHeight,
			GetMainHwnd(), (HMENU)1500, hInst, NULL);

	if(hWndListLocal != NULL)
	{
		// 64ビット対応
//		LocalProcPtr = (WNDPROC)SetWindowLong(hWndListLocal, GWL_WNDPROC, (LONG)LocalWndProc);
		LocalProcPtr = (WNDPROC)SetWindowLongPtr(hWndListLocal, GWLP_WNDPROC, (LONG_PTR)LocalWndProc);

	    Tmp = SendMessage(hWndListLocal, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	    Tmp |= LVS_EX_FULLROWSELECT;
	    SendMessage(hWndListLocal, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)Tmp);

		if(ListFont != NULL)
			SendMessage(hWndListLocal, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));

		ListImg = ImageList_LoadBitmap(hInst, MAKEINTRESOURCE(dirattr_bmp), 16, 9, RGB(255,0,0));
		SendMessage(hWndListLocal, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImg);
		ShowWindow(hWndListLocal, SW_SHOW);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = LocalTabWidth[0];
		LvCol.pszText = MSGJPN038;
		LvCol.iSubItem = 0;
		SendMessage(hWndListLocal, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = LocalTabWidth[1];
		LvCol.pszText = MSGJPN039;
		LvCol.iSubItem = 1;
		SendMessage(hWndListLocal, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		LvCol.fmt = LVCFMT_RIGHT;
		LvCol.cx = LocalTabWidth[2];
		LvCol.pszText = MSGJPN040;
		LvCol.iSubItem = 2;
		SendMessage(hWndListLocal, LVM_INSERTCOLUMN, 2, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = LocalTabWidth[3];
		LvCol.pszText = MSGJPN041;
		LvCol.iSubItem = 3;
		SendMessage(hWndListLocal, LVM_INSERTCOLUMN, 3, (LPARAM)&LvCol);
	}

	/*===== ホスト側のリストビュー =====*/

	hWndListRemote = CreateWindowEx(/*WS_EX_STATICEDGE*/WS_EX_CLIENTEDGE,
			WC_LISTVIEWA, NULL,
			WS_CHILD | /*WS_BORDER | */LVS_REPORT | LVS_SHOWSELALWAYS,
			LocalWidth + SepaWidth, TOOLWIN_HEIGHT*2, RemoteWidth, ListHeight,
			GetMainHwnd(), (HMENU)1500, hInst, NULL);

	if(hWndListRemote != NULL)
	{
		// 64ビット対応
//		RemoteProcPtr = (WNDPROC)SetWindowLong(hWndListRemote, GWL_WNDPROC, (LONG)RemoteWndProc);
		RemoteProcPtr = (WNDPROC)SetWindowLongPtr(hWndListRemote, GWLP_WNDPROC, (LONG_PTR)RemoteWndProc);

	    Tmp = SendMessage(hWndListRemote, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	    Tmp |= LVS_EX_FULLROWSELECT;
	    SendMessage(hWndListRemote, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)Tmp);

		if(ListFont != NULL)
			SendMessage(hWndListRemote, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));

		SendMessage(hWndListRemote, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImg);
		ShowWindow(hWndListRemote, SW_SHOW);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = RemoteTabWidth[0];
		LvCol.pszText = MSGJPN042;
		LvCol.iSubItem = 0;
		SendMessage(hWndListRemote, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = RemoteTabWidth[1];
		LvCol.pszText = MSGJPN043;
		LvCol.iSubItem = 1;
		SendMessage(hWndListRemote, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		LvCol.fmt = LVCFMT_RIGHT;
		LvCol.cx = RemoteTabWidth[2];
		LvCol.pszText = MSGJPN044;
		LvCol.iSubItem = 2;
		SendMessage(hWndListRemote, LVM_INSERTCOLUMN, 2, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = RemoteTabWidth[3];
		LvCol.pszText = MSGJPN045;
		LvCol.iSubItem = 3;
		SendMessage(hWndListRemote, LVM_INSERTCOLUMN, 3, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = RemoteTabWidth[4];
		LvCol.pszText = MSGJPN046;
		LvCol.iSubItem = 4;
		SendMessage(hWndListRemote, LVM_INSERTCOLUMN, 4, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = RemoteTabWidth[5];
		LvCol.pszText = MSGJPN047;
		LvCol.iSubItem = 5;
		SendMessage(hWndListRemote, LVM_INSERTCOLUMN, 5, (LPARAM)&LvCol);
	}

	Sts = FFFTP_SUCCESS;
	if((hWndListLocal == NULL) ||
	   (hWndListRemote == NULL))
	{
		Sts = FFFTP_FAIL;
	}
	return(Sts);
}


/*----- ファイルリストウインドウを削除 ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteListWin(void)
{
//	if(ListImg != NULL)
//		ImageList_Destroy(ListImg);
	if(hWndListLocal != NULL)
		DestroyWindow(hWndListLocal);
	if(hWndListRemote != NULL)
		DestroyWindow(hWndListRemote);
	return;
}


/*----- ローカル側のファイルリストのウインドウハンドルを返す ------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetLocalHwnd(void)
{
	return(hWndListLocal);
}


/*----- ホスト側のファイルリストのウインドウハンドルを返す --------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetRemoteHwnd(void)
{
	return(hWndListRemote);
}


/*----- ローカル側のファイルリストウインドウのメッセージ処理 ------------------
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

static LRESULT CALLBACK LocalWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return(FileListCommonWndProc(hWnd, message, wParam, lParam));
}


/*----- ホスト側のファイルリストウインドウのメッセージ処理 --------------------
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

static LRESULT CALLBACK RemoteWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return(FileListCommonWndProc(hWnd, message, wParam, lParam));
}


// ダイアログプロシージャ
static BOOL CALLBACK doOleDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
#define TIMER_ID     (100)      // 作成するタイマの識別ID
#define TIMER_ELAPSE (100)       // WM_TIMERの発生間隔
	MSG message;

	switch( msg ){
	case WM_INITDIALOG:  // ダイアログボックスが作成されたとき
		SetTimer( hDlg, TIMER_ID, 0, NULL);
		return TRUE;

	case WM_TIMER:
		ShowWindow(hDlg, SW_HIDE);  // ダイアログは隠す

		if (wp != TIMER_ID)
			break;

		if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&message);
				DispatchMessage(&message);

		} else {
			if (AskTransferNow() == NO) {
				EndDialog( hDlg, 0 );
				return TRUE;
			}
		}

		SetTimer( hDlg, TIMER_ID, TIMER_ELAPSE, NULL );
		return TRUE;

	case WM_COMMAND:     // ダイアログボックス内の何かが選択されたとき
		switch( LOWORD( wp ) ){
//		case IDOK:       // 「OK」ボタンが選択された
		case IDCANCEL:   // 「キャンセル」ボタンが選択された
			// ダイアログボックスを消す
			EndDialog( hDlg, 0 );
			break;
		}
		return TRUE;
	}

	return FALSE;  // DefWindowProc()ではなく、FALSEを返すこと！
#undef TIMER_ID     
#undef TIMER_ELAPSE 
}


static void doTransferRemoteFile(void)
{
	FILELIST *FileListBase, *FileListBaseNoExpand, *pf;
	int CancelFlg = NO;
	char LocDir[FMAX_PATH+1];
	char TmpDir[FMAX_PATH+1];
	char buf[32];
	int i;
	DWORD pid;

	// すでにリモートから転送済みなら何もしない。(2007.9.3 yutaka)
	if (remoteFileListBase != NULL)
		return;

	FileListBase = NULL;
	MakeSelectedFileList(WIN_REMOTE, YES, NO, &FileListBase, &CancelFlg);
	FileListBaseNoExpand = NULL;
	MakeSelectedFileList(WIN_REMOTE, NO, NO, &FileListBaseNoExpand, &CancelFlg);

	// set temporary folder
	AskLocalCurDir(LocDir, FMAX_PATH);

	// アプリを多重起動してもコンフリクトしないように、テンポラリフォルダ名にプロセスID
	// を付加する。(2007.9.13 yutaka)
	GetTempPath(sizeof(TmpDir), TmpDir);
	pid = GetCurrentProcessId();
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "ffftp%d", pid);
	strncat_s(TmpDir, sizeof(TmpDir), buf, _TRUNCATE);
	_mkdir(TmpDir);
#if 0
	if (TmpDir[strlen(TmpDir) - 1] == '\\') {
		TmpDir[strlen(TmpDir) - 1] = '\0';
	}
#endif

	// 既存のファイルを削除する
	for (pf = FileListBase ; pf ; pf = pf->Next) {
		char fn[FMAX_PATH+1];

		strncpy_s(fn, sizeof(fn), TmpDir, _TRUNCATE);
		strncat_s(fn, sizeof(fn), "\\", _TRUNCATE);
		strncat_s(fn, sizeof(fn), pf->File, _TRUNCATE);

		remove(fn);
	}

	// ダウンロード先をテンポラリに設定
	SetLocalDirHist(TmpDir);

	// FFFTPにダウンロード要求を出し、ダウンロードの完了を待つ。
	PostMessage(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_DOWNLOAD, 0), 0);

	for (i = 0 ; i < 10 ; i++) {
		MSG msg;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		} else {
			// 転送スレッドが動き出したら抜ける。
			if (AskTransferNow() == YES)
				break;
		}

		Sleep(10);
	}

	// OLE D&D中にメインウィンドウをユーザに操作させると、おかしくなるので、
	// 隠しモーダルダイアログを作る。
	// (2007.9.11 yutaka)
	DialogBox(GetFtpInst(), MAKEINTRESOURCE(IDD_OLEDRAG), GetMainHwnd(), (DLGPROC)doOleDlgProc);

	// ダウンロード先を元に戻す
	SetLocalDirHist(LocDir);
	SetCurrentDirAsDirHist();

	remoteFileListBase = FileListBase;  // あとでフリーすること
	remoteFileListBaseNoExpand = FileListBaseNoExpand;  // あとでフリーすること
	strncpy_s(remoteFileDir, sizeof(remoteFileDir), TmpDir, _TRUNCATE);

#if 0
	// add temporary list
	if (remoteFileListBase != NULL) {
		FILELIST *pf = remoteFileListBase;
		char fn[FMAX_PATH + 1];
		while (pf) {
			strncpy_s(fn, sizeof(fn), remoteFileDir, _TRUNCATE);
			strncat_s(fn, sizeof(fn), "\\", _TRUNCATE);
			strncat_s(fn, sizeof(fn), pf->File, _TRUNCATE);
			AddTempFileList(fn);
			pf = pf->Next;
		}
	}
#endif
}


int isDirectory(char *fn)
{
	struct _stat buf;

	if (_stat(fn, &buf) == 0) {
		if (buf.st_mode & _S_IFDIR) { // is directory
			return 1;
		}
	}
	return 0;
}

// テンポラリのファイルおよびフォルダを削除する。
void doDeleteRemoteFile(void)
{
	if (remoteFileListBase != NULL) {
#if 0
		int dirs = 0;
		int i, count;
		FILELIST *pf = remoteFileListBase;
		char fn[FMAX_PATH + 1];
		while (pf) {
			strncpy_s(fn, sizeof(fn), remoteFileDir, _TRUNCATE);
			strncat_s(fn, sizeof(fn), "\\", _TRUNCATE);
			strncat_s(fn, sizeof(fn), pf->File, _TRUNCATE);
			if (isDirectory(fn)) {
				dirs++;
			} else {
				remove(fn);
			}
			pf = pf->Next;
		}

		count = 0;
		for (i = 0 ; i < 1000 ; i++) {
			pf = remoteFileListBase;
			while (pf) {
				strncpy_s(fn, sizeof(fn), remoteFileDir, _TRUNCATE);
				strncat_s(fn, sizeof(fn), "\\", _TRUNCATE);
				strncat_s(fn, sizeof(fn), pf->File, _TRUNCATE);
				if (isDirectory(fn)) {
					if (_rmdir(fn) == 0) { // ディレクトリを消せたらカウントアップ
						count++;
						if (count >= dirs)  // すべて消せたら終わり
							goto skip;
					}
				}
				pf = pf->Next;
			}
		}
skip:
		_rmdir(remoteFileDir);  // 自分で作ったディレクトリも消す
#else
		SHFILEOPSTRUCT FileOp = { NULL, FO_DELETE, remoteFileDir, NULL, 
			FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI, 
			FALSE, NULL, NULL };	
		SHFileOperation(&FileOp);
#endif

		DeleteFileList(&remoteFileListBase);
		remoteFileListBase = NULL;
	}

	if (remoteFileListBaseNoExpand != NULL)	{
		DeleteFileList(&remoteFileListBaseNoExpand);
		remoteFileListBaseNoExpand = NULL;
	}
}


// yutaka
// cf. http://www.nakka.com/lib/
/* ドロップファイルの作成 */
static HDROP APIPRIVATE CreateDropFileMem(char **FileName,int cnt,BOOL fWide)
{
	HDROP hDrop;
	LPDROPFILES lpDropFile;
	wchar_t wbuf[BUF_SIZE];
	int flen = 0;
	int i;
	
	if(fWide == TRUE){
		/* ワイドキャラ */
		for(i = 0;i < cnt;i++){
			// UTF-8対応
//			MultiByteToWideChar(CP_ACP,0,FileName[i],-1,wbuf,BUF_SIZE);
//			flen += (wcslen(wbuf) + 1) * sizeof(wchar_t);
			flen += sizeof(wchar_t) * MtoW(NULL, 0, FileName[i], -1);
		}
		flen++;
	}else{
		/* マルチバイト */
		for(i = 0;i < cnt;i++){
			// UTF-8対応
//			flen += lstrlen(FileName[i]) + 1;
			MtoW(wbuf, BUF_SIZE, FileName[i], -1);
			flen += sizeof(char) * WtoA(NULL, 0, wbuf, -1);
		}
	}

	hDrop = (HDROP)GlobalAlloc(GHND,sizeof(DROPFILES) + flen + 1);
	if (hDrop == NULL){
		return NULL;
	}

	lpDropFile = (LPDROPFILES) GlobalLock(hDrop);
	lpDropFile->pFiles = sizeof(DROPFILES);		/* ファイル名のリストまでのオフセット */
	lpDropFile->pt.x = 0;
	lpDropFile->pt.y = 0;
	lpDropFile->fNC = FALSE;
	lpDropFile->fWide = fWide;					/* ワイドキャラの場合は TRUE */

	/* 構造体の後ろにファイル名のリストをコピーする。(ファイル名\0ファイル名\0ファイル名\0\0) */
	if(fWide == TRUE){
		/* ワイドキャラ */
		wchar_t *buf;

		buf = (wchar_t *)(&lpDropFile[1]);
		for(i = 0;i < cnt;i++){
			// UTF-8対応
//			MultiByteToWideChar(CP_ACP,0,FileName[i],-1,wbuf,BUF_SIZE);
//			wcscpy(buf,wbuf);
//			buf += wcslen(wbuf) + 1;
			buf += MtoW(buf, BUF_SIZE, FileName[i], -1);
		}
	}else{
		/* マルチバイト */
		char *buf;

		buf = (char *)(&lpDropFile[1]);
		for(i = 0;i < cnt;i++){
			// UTF-8対応
//			lstrcpy(buf,FileName[i]);
//			buf += lstrlen(FileName[i]) + 1;
			MtoW(wbuf, BUF_SIZE, FileName[i], -1);
			buf += WtoA(buf, BUF_SIZE, wbuf, -1);
		}
	}

	GlobalUnlock(hDrop);
	return(hDrop);
}


// OLE D&Dを開始する 
// (2007.8.30 yutaka)
static void doDragDrop(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT cf[CF_CNT];
	POINT pt;
	int ret;

	// テンポラリをきれいにする (2007.9.3 yutaka)
	doDeleteRemoteFile();

	/* ドラッグ&ドロップの開始 */
	cf[0] = CF_HDROP;
	cf[1] = CF_HDROP;	/* ファイル */
	if((ret = OLE_IDropSource_Start(hWnd,WM_GETDATA, WM_DRAGOVER, cf,1,DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK)) == DROPEFFECT_MOVE){
	}

	// ドロップ先のアプリに WM_LBUTTONUP を飛ばす。
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	PostMessage(hWnd,WM_LBUTTONUP,0,MAKELPARAM(pt.x,pt.y));
}



/*----- ファイル一覧ウインドウの共通メッセージ処理 ----------------------------
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

static LRESULT FileListCommonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT Point;
	HWND hWndPnt;
	HWND hWndParent;
	static HCURSOR hCsrDrg;
	static HCURSOR hCsrNoDrg;
	static POINT DragPoint;
	static HWND hWndDragStart;
	static int RemoteDropFileIndex = -1;
	int Win;
	HWND hWndDst;
	WNDPROC ProcPtr;
	HWND hWndHistEdit;

	Win = WIN_LOCAL;
	hWndDst = hWndListRemote;
	ProcPtr = LocalProcPtr;
	hWndHistEdit = GetLocalHistEditHwnd();
	if(hWnd == hWndListRemote)
	{
		Win = WIN_REMOTE;
		hWndDst = hWndListLocal;
		ProcPtr = RemoteProcPtr;
		hWndHistEdit = GetRemoteHistEditHwnd();
	}

	switch (message)
	{
        case WM_SYSKEYDOWN:
			if (wParam == 'D') {	// Alt+D
				SetFocus(hWndHistEdit);
				break;
			}
			EraseListViewTips();
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));

        case WM_KEYDOWN:
			if(wParam == 0x09)
			{
				SetFocus(hWndDst);
				break;
			}
			EraseListViewTips();
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));

		case WM_SETFOCUS :
			SetFocusHwnd(hWnd);
			MakeButtonsFocus();
			DispCurrentWindow(Win);
			DispSelectedSpace();
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));

		case WM_KILLFOCUS :
			EraseListViewTips();
			MakeButtonsFocus();
			DispCurrentWindow(-1);
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));

		case WM_DROPFILES :
			// ドラッグ中は処理しない。ドラッグ後にWM_LBUTTONDOWNが飛んでくるため、そこで処理する。
			if (Dragging == YES) 
				return (FALSE);

			if(hWnd == hWndListRemote)
			{
				if(AskConnecting() == YES)
					UpLoadDragProc(wParam);
			}
			else if(hWnd == hWndListLocal)
			{
				ChangeDirDropFileProc(wParam);
			}
			break;

		case WM_LBUTTONDOWN :
			EraseListViewTips();
			SetFocus(hWnd);
			DragPoint.x = LOWORD(lParam);
			DragPoint.y = HIWORD(lParam);
			hWndDragStart = hWnd;
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));
			break;

		case WM_LBUTTONUP :
			if(Dragging == YES)
			{
				Dragging = NO;
				ReleaseCapture();
				hCsrDrg = LoadCursor(NULL, IDC_ARROW);
				SetCursor(hCsrDrg);

				Point.x = (long)(short)LOWORD(lParam);
				Point.y = (long)(short)HIWORD(lParam);
				ClientToScreen(hWnd, &Point);
				hWndPnt = WindowFromPoint(Point);
				if(hWndPnt == hWndDst)  // local <-> remote 
				{
					if(hWndPnt == hWndListRemote) {
						PostMessage(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_UPLOAD, 0), 0);
					} else if(hWndPnt == hWndListLocal) {
						PostMessage(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_DOWNLOAD, 0), 0);
					}
				} else { // 同一ウィンドウ内の場合 (yutaka)
					if (hWndDragStart == hWndListRemote && hWndPnt == hWndListRemote) {
						// remote <-> remoteの場合は、サーバでのファイルの移動を行う。(2007.9.5 yutaka)
						if (RemoteDropFileIndex != -1) {
							ListView_SetItemState(hWnd, RemoteDropFileIndex, 0, LVIS_DROPHILITED);
							MoveRemoteFileProc(RemoteDropFileIndex);
						}

					}

				}
			}
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));

 		case WM_DRAGDROP:  
 			// OLE D&Dを開始する (yutaka)
 			doDragDrop(hWnd, message, wParam, lParam);
 			return (TRUE);
 			break;
 
 		case WM_GETDATA:  // ファイルのパスをD&D先のアプリへ返す (yutaka)
 			switch(wParam)
 			{
 			case CF_HDROP:		/* ファイル */
 				{
 					OSVERSIONINFO os_info;
 					BOOL NTFlag = FALSE;
 					char **FileNameList;
 					int filelen;
 					int i, j, filenum = 0;
 
 					FILELIST *FileListBase, *FileListBaseNoExpand, *pf;
 					int CancelFlg = NO;
 					char LocDir[FMAX_PATH+1];
 					char *PathDir;
 
					// 変数が未初期化のバグ修正
					FileListBaseNoExpand = NULL;
 					// ローカル側で選ばれているファイルをFileListBaseに登録
 					if (hWndDragStart == hWndListLocal) {
 						AskLocalCurDir(LocDir, FMAX_PATH);
 						PathDir = LocDir;
 
 						FileListBase = NULL;
						// ローカル側からアプリケーションにD&Dできないバグ修正
// 						MakeSelectedFileList(WIN_LOCAL, YES, NO, &FileListBase, &CancelFlg);			
 						MakeSelectedFileList(WIN_LOCAL, NO, NO, &FileListBase, &CancelFlg);			
						FileListBaseNoExpand = FileListBase;
 
 					} else if (hWndDragStart == hWndListRemote) {
 						GetCursorPos(&Point);
 						hWndPnt = WindowFromPoint(Point);
						hWndParent = GetParent(hWndPnt);
 						if (hWndPnt == hWndListRemote || hWndPnt == hWndListLocal ||
							hWndParent == hWndListRemote || hWndParent == hWndListLocal) {
 							FileListBase = NULL;
 
 						} else {
 							// 選択されているリモートファイルのリストアップ
 							// このタイミングでリモートからローカルの一時フォルダへダウンロードする
 							// (2007.8.31 yutaka)
 							doTransferRemoteFile();
 							PathDir = remoteFileDir;
 							FileListBase = remoteFileListBase;
 							FileListBaseNoExpand = remoteFileListBaseNoExpand;
 						}
 
 					} 
 
 					pf = FileListBaseNoExpand;
 					for (filenum = 0; pf ; filenum++) {
 						pf = pf->Next;
 					}
 					// ファイルが未選択の場合は何もしない。(yutaka)
 					if (filenum <= 0) {
 						*((HANDLE *)lParam) = NULL;
 						return (FALSE);
 					}
 					
 					/* ファイル名の配列を作成する */
					// TODO: GlobalAllocが返すのはメモリポインタではなくハンドルだが実際は同じ値
 					FileNameList = (char **)GlobalAlloc(GPTR,sizeof(char *) * filenum);
 					if(FileNameList == NULL){
 						abort();
 					}
 					pf = FileListBaseNoExpand;
 					for (j = 0; pf ; j++) {
 						filelen = strlen(PathDir) + 1 + strlen(pf->File) + 1;
 						FileNameList[j] = (char *)GlobalAlloc(GPTR, filelen);
 						strncpy_s(FileNameList[j], filelen, PathDir, _TRUNCATE);
 						strncat_s(FileNameList[j], filelen, "\\", _TRUNCATE);
 						strncat_s(FileNameList[j], filelen, pf->File, _TRUNCATE);
 						pf = pf->Next;
#if 0
						if (FileListBase->Node == NODE_DIR) { 
 							// フォルダを掴んだ場合はそれ以降展開しない
 							filenum = 1;
 							break;
 						}
#endif
 					}
 					
 					os_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
 					GetVersionEx(&os_info);
 					if(os_info.dwPlatformId == VER_PLATFORM_WIN32_NT){
 						NTFlag = TRUE;
 					}
 
 					/* ドロップファイルリストの作成 */
 					/* NTの場合はUNICODEになるようにする */
 					*((HANDLE *)lParam) = CreateDropFileMem(FileNameList, filenum, NTFlag);
 
 					/* ファイル名の配列を解放する */
 					for (i = 0; i < filenum ; i++) {
 						GlobalFree(FileNameList[i]);
 					}
 					GlobalFree(FileNameList);
 
 					if (hWndDragStart == hWndListLocal) {
 						DeleteFileList(&FileListBase);
 					} else {
 						// あとでファイル削除してフリーする
 					}
 
 					return (TRUE);
 				}
 				break;
 
 			default:
 				*((HANDLE *)lParam) = NULL;
 				break;
 			}
 
			break;

		case WM_DRAGOVER:
			{
				LVHITTESTINFO hi;
				int Node, index;
				static int prev_index = -1;

				// 同一ウィンドウ内でのD&Dはリモート側のみ
				if (Win != WIN_REMOTE)
					break;

				if(MoveMode == MOVE_DISABLE)
					break;

				memset(&hi, 0, sizeof(hi));

				GetCursorPos(&Point);
				hWndPnt = WindowFromPoint(Point);
				ScreenToClient(hWnd, &Point);

				hi.pt = Point;

				// 以前の選択を消す
				ListView_SetItemState(hWnd, prev_index, 0, LVIS_DROPHILITED);
				RemoteDropFileIndex = -1;

				if ((hWndPnt == hWndListRemote) && (ListView_HitTest(hWnd, &hi) != -1)) {
					if (hi.flags == LVHT_ONITEMLABEL) { // The position is over a list-view item's text.
					
						index = hi.iItem;
						prev_index = index;
						Node = GetNodeType(Win, index);
						if (Node == NODE_DIR) {
							ListView_SetItemState(hWnd, index, LVIS_DROPHILITED, LVIS_DROPHILITED);
							RemoteDropFileIndex = index;
						}
					}
				} 

			}
			break;

		case WM_RBUTTONDOWN :
			/* ここでファイルを選ぶ */
			CallWindowProc(ProcPtr, hWnd, message, wParam, lParam);

			EraseListViewTips();
			SetFocus(hWnd);
			if(hWnd == hWndListRemote)
				RemoteRbuttonMenu(0);
			else if(hWnd == hWndListLocal)
				LocalRbuttonMenu(0);
			break;

		case WM_LBUTTONDBLCLK :
			DoubleClickProc(Win, NO, -1);
			break;

		case WM_MOUSEMOVE :
			if(wParam == MK_LBUTTON)
			{
				if((Dragging == NO) && 
				   (hWnd == hWndDragStart) &&
				   (AskConnecting() == YES) &&
				   (SendMessage(hWnd, LVM_GETSELECTEDCOUNT, 0, 0) > 0) &&
				   ((abs((short)LOWORD(lParam) - DragPoint.x) > 5) ||
					(abs((short)HIWORD(lParam) - DragPoint.y) > 5)))
				{
					SetCapture(hWnd);
					Dragging = YES;
					hCsrDrg = LoadCursor(GetFtpInst(), MAKEINTRESOURCE(drag_csr));
					hCsrNoDrg = LoadCursor(GetFtpInst(), MAKEINTRESOURCE(nodrop_csr));
					SetCursor(hCsrDrg);
				}
				else if(Dragging == YES)
				{
					Point.x = (long)(short)LOWORD(lParam);
					Point.y = (long)(short)HIWORD(lParam);
					ClientToScreen(hWnd, &Point);
					hWndPnt = WindowFromPoint(Point);
					if((hWndPnt == hWndListRemote) || (hWndPnt == hWndListLocal))
						SetCursor(hCsrDrg);
					else {
						// マウスポインタの×表示をやめる (yutaka)
#if 0
						SetCursor(hCsrNoDrg);
#endif
					}

					// OLE D&Dの開始を指示する
					PostMessage(hWnd, WM_DRAGDROP, MAKEWPARAM(wParam, lParam), 0);

				}
				else
					return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));
			}
			else
			{
				CheckTipsDisplay(hWnd, lParam);
				return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));
			}
			break;

		case WM_MOUSEWHEEL :
			if(Dragging == NO)
			{
                short zDelta = (short)HIWORD(wParam);

				EraseListViewTips();
				Point.x = (short)LOWORD(lParam);
				Point.y = (short)HIWORD(lParam);
				hWndPnt = WindowFromPoint(Point);

				if((wParam & MAKEWPARAM(MK_SHIFT, 0)) && 
				   ((hWndPnt == hWndListRemote) ||
					(hWndPnt == hWndListLocal) || 
					(hWndPnt == GetTaskWnd())))
				{
					PostMessage(hWndPnt, WM_VSCROLL, zDelta > 0 ? MAKEWPARAM(SB_PAGEUP, 0) : MAKEWPARAM(SB_PAGEDOWN, 0), 0);
//					PostMessage(hWndPnt, WM_VSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
				}
				else if(hWndPnt == hWnd)
					return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));
				else if((hWndPnt == hWndDst) || (hWndPnt == GetTaskWnd()))
					PostMessage(hWndPnt, message, wParam, lParam);
			}
			break;

		default :
			return(CallWindowProc(ProcPtr, hWnd, message, wParam, lParam));
	}
    return(0L);
}


/*----- ファイルリストのタブ幅を取得する --------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void GetListTabWidth(void)
{
	LV_COLUMN LvCol;
	int i;

	for(i = 0; i <= 3; i++)
	{
		LvCol.mask = LVCF_WIDTH;
		if(SendMessage(hWndListLocal, LVM_GETCOLUMN, i, (LPARAM)&LvCol) == TRUE)
			LocalTabWidth[i] = LvCol.cx;
	}

	for(i = 0; i <= 5; i++)
	{
		LvCol.mask = LVCF_WIDTH;
		if(SendMessage(hWndListRemote, LVM_GETCOLUMN, i, (LPARAM)&LvCol) == TRUE)
			RemoteTabWidth[i] = LvCol.cx;
	}
	return;
}


/*----- ファイル一覧方法にしたがってリストビューを設定する --------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetListViewType(void)
{
	// 64ビット対応
//	long lStyle;
	LONG_PTR lStyle;

	switch(ListType)
	{
		case LVS_LIST :
			// 64ビット対応
//			lStyle = GetWindowLong(GetLocalHwnd(), GWL_STYLE);
			lStyle = GetWindowLongPtr(GetLocalHwnd(), GWL_STYLE);
			lStyle &= ~(LVS_REPORT | LVS_LIST);
			lStyle |= LVS_LIST;
			// 64ビット対応
//			SetWindowLong(GetLocalHwnd(), GWL_STYLE, lStyle);
			SetWindowLongPtr(GetLocalHwnd(), GWL_STYLE, lStyle);

			// 64ビット対応
//			lStyle = GetWindowLong(GetRemoteHwnd(), GWL_STYLE);
			lStyle = GetWindowLongPtr(GetRemoteHwnd(), GWL_STYLE);
			lStyle &= ~(LVS_REPORT | LVS_LIST);
			lStyle |= LVS_LIST;
			// 64ビット対応
//			SetWindowLong(GetRemoteHwnd(), GWL_STYLE, lStyle);
			SetWindowLongPtr(GetRemoteHwnd(), GWL_STYLE, lStyle);
			break;

		default :
			// 64ビット対応
//			lStyle = GetWindowLong(GetLocalHwnd(), GWL_STYLE);
			lStyle = GetWindowLongPtr(GetLocalHwnd(), GWL_STYLE);
			lStyle &= ~(LVS_REPORT | LVS_LIST);
			lStyle |= LVS_REPORT;
			// 64ビット対応
//			SetWindowLong(GetLocalHwnd(), GWL_STYLE, lStyle);
			SetWindowLongPtr(GetLocalHwnd(), GWL_STYLE, lStyle);

			// 64ビット対応
//			lStyle = GetWindowLong(GetRemoteHwnd(), GWL_STYLE);
			lStyle = GetWindowLongPtr(GetRemoteHwnd(), GWL_STYLE);
			lStyle &= ~(LVS_REPORT | LVS_LIST);
			lStyle |= LVS_REPORT;
			// 64ビット対応
//			SetWindowLong(GetRemoteHwnd(), GWL_STYLE, lStyle);
			SetWindowLongPtr(GetRemoteHwnd(), GWL_STYLE, lStyle);
			break;
	}
	return;
}


/*----- ホスト側のファイル一覧ウインドウにファイル名をセット ------------------
*
*	Parameter
*		int Mode : キャッシュモード (CACHE_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void GetRemoteDirForWnd(int Mode, int *CancelCheckWork)
{
	FILE *fd;
	LONGLONG Size;
	char Str[FMAX_PATH+1];
	char Buf[FMAX_PATH+1];
	FILETIME Time;
	int Attr;
	int Type;
	int ListType;
	int Num;
	FLISTANCHOR Anchor;
	char Owner[OWNER_NAME_LEN+1];
	int Link;
	int InfoExist;

//#pragma aaa
//DoPrintf("===== GetRemoteDirForWnd");

	Anchor.Top = NULL;
	Anchor.Files = 0;

	if(AskConnecting() == YES)
	{
//		SetCursor(LoadCursor(NULL, IDC_WAIT));
		DisableUserOpe();

		AskRemoteCurDir(Buf, FMAX_PATH);
		SetRemoteDirHist(Buf);

		Type = FTP_COMPLETE;
		if(Mode != CACHE_LASTREAD)
		{

			if((Num = AskCached(Buf)) == -1)
			{
				Num = AskFreeCache();
				Mode = CACHE_REFRESH;
			}

			if(Mode == CACHE_REFRESH)
			{
				if((Type = DoDirListCmdSkt("", "", Num, CancelCheckWork)) == FTP_COMPLETE)
					SetCache(Num, Buf);
				else
					ClearCache(Num);
			}
		}
		else
			Num = AskCurrentFileListNum();

		if(Type == FTP_COMPLETE)
		{
			SetCurrentFileListNum(Num);
			MakeCacheFileName(Num, Buf);
			if((fd = fopen(Buf, "rb"))!=NULL)
			{
				ListType = LIST_UNKNOWN;

				while(GetListOneLine(Str, FMAX_PATH, fd) == FFFTP_SUCCESS)
				{
					if((ListType = AnalizeFileInfo(Str)) != LIST_UNKNOWN)
					{
						if((Type = ResolvFileInfo(Str, ListType, Buf, &Size, &Time, &Attr, Owner, &Link, &InfoExist)) != NODE_NONE)
						{
							if(AskFilterStr(Buf, Type) == YES)
							{
								if((DotFile == YES) || (Buf[0] != '.'))
								{
									AddDispFileList(&Anchor, Buf, &Time, Size, Attr, Type, Link, Owner, InfoExist, WIN_REMOTE);
								}
							}
						}
					}
				}
				fclose(fd);

				DispFileList2View(GetRemoteHwnd(), &Anchor);
				EraseDispFileList(&Anchor);

				// 先頭のアイテムを選択
				ListView_SetItemState(GetRemoteHwnd(), 0, LVIS_FOCUSED, LVIS_FOCUSED);
			}
			else
			{
				SetTaskMsg(MSGJPN048);
				SendMessage(GetRemoteHwnd(), LVM_DELETEALLITEMS, 0, 0);
			}
		}
		else
		{
#if defined(HAVE_OPENVMS)
			/* OpenVMSの場合空ディレクトリ移動の時に出るので、メッセージだけ出さない
			 * ようにする(VIEWはクリアして良い) */
			if (AskHostType() != HTYPE_VMS)
#endif
			SetTaskMsg(MSGJPN049);
			SendMessage(GetRemoteHwnd(), LVM_DELETEALLITEMS, 0, 0);
		}

//		SetCursor(LoadCursor(NULL, IDC_ARROW));
		EnableUserOpe();

	}

//#pragma aaa
//DoPrintf("===== GetRemoteDirForWnd Done");

	return;
}


/*----- ローカル側のファイル一覧ウインドウにファイル名をセット ----------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void GetLocalDirForWnd(void)
{
	HANDLE fHnd;
	WIN32_FIND_DATA Find;
	char Scan[FMAX_PATH+1];
	char *Pos;
	char Buf[10];
	FILETIME Time;
	FLISTANCHOR Anchor;
	DWORD NoDrives;
	int Tmp;

	Anchor.Top = NULL;
	Anchor.Files = 0;

	DoLocalPWD(Scan);
	SetLocalDirHist(Scan);
	DispLocalFreeSpace(Scan);

	/* ディレクトリ／ファイル */

	SetYenTail(Scan);
	strcat(Scan, "*");
	if((fHnd = FindFirstFileAttr(Scan, &Find, DispIgnoreHide)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((strcmp(Find.cFileName, ".") != 0) &&
			   (strcmp(Find.cFileName, "..") != 0))
			{
				if((DotFile == YES) || (Find.cFileName[0] != '.'))
				{
					if(Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						AddDispFileList(&Anchor, Find.cFileName, &Find.ftLastWriteTime, MakeLongLong(Find.nFileSizeHigh, Find.nFileSizeLow), 0, NODE_DIR, NO, "", FINFO_ALL, WIN_LOCAL);
					else
					{
						if(AskFilterStr(Find.cFileName, NODE_FILE) == YES)
						{
							AddDispFileList(&Anchor, Find.cFileName, &Find.ftLastWriteTime, MakeLongLong(Find.nFileSizeHigh, Find.nFileSizeLow), 0, NODE_FILE, NO, "", FINFO_ALL, WIN_LOCAL);
						}
					}
				}
			}
		}
		while(FindNextFileAttr(fHnd, &Find, DispIgnoreHide) == TRUE);
		FindClose(fHnd);
	}

	/* ドライブ */
	if(DispDrives)
	{
		GetLogicalDriveStrings(FMAX_PATH, Scan);
		NoDrives = LoadHideDriveListRegistory();

		Pos = Scan;
		while(*Pos != NUL)
		{
			Tmp = toupper(*Pos) - 'A';
			if((NoDrives & (0x00000001 << Tmp)) == 0)
			{
				sprintf(Buf, "%s", Pos);
				memset(&Time, 0, sizeof(FILETIME));
				AddDispFileList(&Anchor, Buf, &Time, 0, 0, NODE_DRIVE, NO, "", FINFO_ALL, WIN_LOCAL);
			}
			Pos = strchr(Pos, NUL) + 1;
		}
	}

	DispFileList2View(GetLocalHwnd(), &Anchor);
	EraseDispFileList(&Anchor);

	// 先頭のアイテムを選択
	ListView_SetItemState(GetLocalHwnd(), 0, LVIS_FOCUSED, LVIS_FOCUSED);

	return;
}


/*----- ファイル情報をファイル一覧用リストに登録する --------------------------
*
*	Parameter
*		FLISTANCHOR *Anchor : ファイルリストの先頭
*		char *Name : ファイル名
*		FILETIME *Time : 日付
*		LONGLONG Size : サイズ
*		int Attr : 属性
*		int Type : タイプ (NODE_xxxx)
*		int Link : リンクかどうか (YES/NO)
*		char *Owner : オーナ名
*		int InfoExist : 情報があるかどうか (FINFO_xxx)
*		int Win : ウィンドウ番号 (WIN_xxxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AddDispFileList(FLISTANCHOR *Anchor, char *Name, FILETIME *Time, LONGLONG Size, int Attr, int Type, int Link, char *Owner, int InfoExist, int Win)
{
	int i;
	FILELIST *Pos;
	FILELIST *Prev;
	FILELIST *New;
	int FileSort;
	int DirSort;
	int Sort;
	LONGLONG Cmp;

	FileSort = AskSortType(ITEM_LFILE);
	DirSort = AskSortType(ITEM_LDIR);
	if(Win == WIN_REMOTE)
	{
		FileSort = AskSortType(ITEM_RFILE);
		DirSort = AskSortType(ITEM_RDIR);
	}

	Pos = Anchor->Top;
	for(i = 0; i < Anchor->Files; i++)
	{
		if((Type == NODE_DIR) && (Pos->Node == NODE_FILE))
			break;
		if((Type == NODE_FILE) && (Pos->Node == NODE_DRIVE))
			break;

		if(Type == Pos->Node)
		{
			if(Type == NODE_DIR)
				Sort = DirSort;
			else
				Sort = FileSort;

			if((Sort & SORT_GET_ORD) == SORT_ASCENT)
			{
				if((((Sort & SORT_MASK_ORD) == SORT_EXT) &&
					((Cmp = _mbsicmp(GetFileExt(Name), GetFileExt(Pos->File))) < 0)) ||
				   (((Sort & SORT_MASK_ORD) == SORT_SIZE) &&
					((Cmp = Size - Pos->Size) < 0)) ||
				   (((Sort & SORT_MASK_ORD) == SORT_DATE) &&
					((Cmp = CompareFileTime(Time, &Pos->Time)) < 0)))
				{
					break;
				}

				if(((Sort & SORT_MASK_ORD) == SORT_NAME) || (Cmp == 0))
				{
					if(_mbsicmp(Name, Pos->File) < 0)
						break;
				}
			}
			else
			{
				if((((Sort & SORT_MASK_ORD) == SORT_EXT) &&
					((Cmp = _mbsicmp(GetFileExt(Name), GetFileExt(Pos->File))) > 0)) ||
				   (((Sort & SORT_MASK_ORD) == SORT_SIZE) &&
					((Cmp = Size - Pos->Size) > 0)) ||
				   (((Sort & SORT_MASK_ORD) == SORT_DATE) &&
					((Cmp = CompareFileTime(Time, &Pos->Time)) > 0)))
				{
					break;
				}

				if(((Sort & SORT_MASK_ORD) == SORT_NAME) || (Cmp == 0))
				{
					if(_mbsicmp(Name, Pos->File) > 0)
						break;
				}
			}
		}
		Prev = Pos;
		Pos = Pos->Next;
	}

	if((New = malloc(sizeof(FILELIST))) != NULL)
	{
		strcpy(New->File, Name);
		New->Node = Type;
		New->Link = Link;
		New->Size = Size;
		New->Attr = Attr;
		New->Time = *Time;
		strcpy(New->Owner, Owner);
		New->InfoExist = InfoExist;

		if(Pos == Anchor->Top)
		{
			New->Next = Anchor->Top;
			Anchor->Top = New;
		}
		else
		{
			New->Next = Prev->Next;
			Prev->Next = New;
		}
		Anchor->Files += 1;
	}
	return;
}


/*----- ファイル一覧用リストをクリアする --------------------------------------
*
*	Parameter
*		FLISTANCHOR *Anchor : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void EraseDispFileList(FLISTANCHOR *Anchor)
{
	FILELIST *Pos;
	FILELIST *Next;
	int i;

	Pos = Anchor->Top;
	for(i = 0; i < Anchor->Files; i++)
	{
		Next = Pos->Next;
		free(Pos);
		Pos = Next;
	}
	Anchor->Files = 0;
	Anchor->Top = NULL;
	return;
}


/*----- ファイル一覧用リストの内容をファイル一覧ウインドウにセット ------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		FLISTANCHOR *Anchor : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispFileList2View(HWND hWnd, FLISTANCHOR *Anchor)
{
	int i;
	FILELIST *Pos;

	SendMessage(hWnd, WM_SETREDRAW, (WPARAM)FALSE, 0);
	SendMessage(hWnd, LVM_DELETEALLITEMS, 0, 0);

	Pos = Anchor->Top;
	for(i = 0; i < Anchor->Files; i++)
	{
		AddListView(hWnd, -1, Pos->File, Pos->Node, Pos->Size, &Pos->Time, Pos->Attr, Pos->Owner, Pos->Link, Pos->InfoExist);
		Pos = Pos->Next;
	}

	SendMessage(hWnd, WM_SETREDRAW, (WPARAM)TRUE, 0);
	UpdateWindow(hWnd);

	DispSelectedSpace();
	return;
}


/*----- ファイル一覧ウインドウ（リストビュー）に追加 --------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		int Pos : 挿入位置
*		char *Name : 名前
*		int Type : タイプ (NIDE_xxxx)
*		LONGLONG Size : サイズ
*		FILETIME *Time : 日付
*		int Attr : 属性
*		char Owner : オーナ名
*		int Link : リンクかどうか
*		int InfoExist : 情報があるかどうか (FINFO_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AddListView(HWND hWnd, int Pos, char *Name, int Type, LONGLONG Size, FILETIME *Time, int Attr, char *Owner, int Link, int InfoExist)
{
	LV_ITEM LvItem;
	char Tmp[20];

	if(Pos == -1)
		Pos = SendMessage(hWnd, LVM_GETITEMCOUNT, 0, 0);

	/* アイコン/ファイル名 */
	LvItem.mask = LVIF_TEXT | LVIF_IMAGE;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 0;
	LvItem.pszText = Name;
	if((Type == NODE_FILE) && (AskTransferTypeAssoc(Name, TYPE_X) == TYPE_I))
		Type = 3;
	if(Link == NO)
		LvItem.iImage = Type;
	else
		LvItem.iImage = 4;
	LvItem.iItem = SendMessage(hWnd, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

	/* 日付/時刻 */
	FileTime2TimeString(Time, Tmp, DISPFORM_LEGACY, InfoExist);
	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 1;
	LvItem.pszText = Tmp;
	LvItem.iItem = SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);

	/* サイズ */
	if(Type == NODE_DIR)
		strcpy(Tmp, "<DIR>");
	else if(Type == NODE_DRIVE)
		strcpy(Tmp, "<DRIVE>");
	else if(Size >= 0)
		MakeNumString(Size, Tmp, TRUE);
	else
		strcpy(Tmp, "");
	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 2;
	LvItem.pszText = Tmp;
	LvItem.iItem = SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);

	/* 拡張子 */
	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 3;
	LvItem.pszText = GetFileExt(Name);
	LvItem.iItem = SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);

	if(hWnd == GetRemoteHwnd())
	{
		/* 属性 */
		strcpy(Tmp, "");
		if(InfoExist & FINFO_ATTR)
			AttrValue2String(Attr, Tmp);
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Pos;
		LvItem.iSubItem = 4;
		LvItem.pszText = Tmp;
		LvItem.iItem = SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);

		/* オーナ名 */
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Pos;
		LvItem.iSubItem = 5;
		LvItem.pszText = Owner;
		LvItem.iItem = SendMessage(hWnd, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
	return;
}


/*----- ファイル名一覧ウインドウをソートし直す --------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ReSortDispList(int Win, int *CancelCheckWork)
{
	if(Win == WIN_REMOTE)
		GetRemoteDirForWnd(CACHE_LASTREAD, CancelCheckWork);
	else
		GetLocalDirForWnd();
	return;
}


/*----- ファイル一覧ウインドウのファイルを選択する ----------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		int Type : 選択方法 (SELECT_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SelectFileInList(HWND hWnd, int Type)
{
	int Win;
	int WinDst;
	int i;
	int Num;
	char RegExp[FMAX_PATH+1];
	char Name[FMAX_PATH+1];
	LV_ITEM LvItem;
	int CsrPos;
	FILETIME Time1;
	FILETIME Time2;
	int Find;

	Win = WIN_LOCAL;
	WinDst = WIN_REMOTE;
	if(hWnd == GetRemoteHwnd())
	{
		Win = WIN_REMOTE;
		WinDst = WIN_LOCAL;
	}

	Num = GetItemCount(Win);
	switch(Type)
	{
		case SELECT_ALL :
			LvItem.state = 0;
			if(GetSelectedCount(Win) <= 1)
				LvItem.state = LVIS_SELECTED;
			for(i = 0; i < Num; i++)
			{
				if(GetNodeType(Win, i) != NODE_DRIVE)
				{
					LvItem.mask = LVIF_STATE;
					LvItem.iItem = i;
					LvItem.stateMask = LVIS_SELECTED;
					LvItem.iSubItem = 0;
					SendMessage(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&LvItem);
				}
			}
			break;

		case SELECT_REGEXP :
			if(((Win == WIN_LOCAL) &&
				(DialogBox(GetFtpInst(), MAKEINTRESOURCE(sel_local_dlg), hWnd, SelectDialogCallBack) == YES)) ||
			   ((Win == WIN_REMOTE) &&
				(DialogBox(GetFtpInst(), MAKEINTRESOURCE(sel_remote_dlg), hWnd, SelectDialogCallBack) == YES)))
			{
				strcpy(RegExp, FindStr);
//				if(FindMode == 0)
//					WildCard2RegExp(RegExp);

				_mbslwr(RegExp);
				if((FindMode == 0) || (JreCompileStr(RegExp) == TRUE))
				{
					CsrPos = -1;
					for(i = 0; i < Num; i++)
					{
						GetNodeName(Win, i, Name, FMAX_PATH);
						Find = FindNameNode(WinDst, Name);

						_mbslwr(Name);
						LvItem.state = 0;
						if(GetNodeType(Win, i) != NODE_DRIVE)
						{
							if(((FindMode == 0) && (CheckFname(Name, RegExp) == FFFTP_SUCCESS)) ||
							   ((FindMode != 0) && (JreGetStrMatchInfo(Name, 0) != NULL)))
							{
								LvItem.state = LVIS_SELECTED;

								if(Find >= 0)
								{
									if(IgnoreExist == YES)
										LvItem.state = 0;

									if((LvItem.state != 0) && (IgnoreNew == YES))
									{
										GetNodeTime(Win, i, &Time1);
										GetNodeTime(WinDst, Find, &Time2);
										if(CompareFileTime(&Time1, &Time2) > 0)
											LvItem.state = 0;
									}

									if((LvItem.state != 0) && (IgnoreOld == YES))
									{
										GetNodeTime(Win, i, &Time1);
										GetNodeTime(WinDst, Find, &Time2);
										if(CompareFileTime(&Time1, &Time2) < 0)
											LvItem.state = 0;
									}
								}
							}
						}

						if((LvItem.state != 0) && (CsrPos == -1))
							CsrPos = i;

						LvItem.mask = LVIF_STATE;
						LvItem.iItem = i;
						LvItem.stateMask = LVIS_SELECTED;
						LvItem.iSubItem = 0;
						SendMessage(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&LvItem);
					}
					if(CsrPos != -1)
					{
						LvItem.mask = LVIF_STATE;
						LvItem.iItem = CsrPos;
						LvItem.state = LVIS_FOCUSED;
						LvItem.stateMask = LVIS_FOCUSED;
						LvItem.iSubItem = 0;
						SendMessage(hWnd, LVM_SETITEMSTATE, CsrPos, (LPARAM)&LvItem);
						SendMessage(hWnd, LVM_ENSUREVISIBLE, CsrPos, (LPARAM)TRUE);
					}
				}
			}
			break;
	}
	return;
}


/*----- 選択ダイアログのコールバック ------------------------------------------
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
//static BOOL CALLBACK SelectDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK SelectDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, SEL_FNAME, EM_LIMITTEXT, 40, 0);
			SendDlgItemMessage(hDlg, SEL_FNAME, WM_SETTEXT, 0, (LPARAM)FindStr);
			SendDlgItemMessage(hDlg, SEL_REGEXP, BM_SETCHECK, FindMode, 0);
			SendDlgItemMessage(hDlg, SEL_NOOLD, BM_SETCHECK, IgnoreOld, 0);
			SendDlgItemMessage(hDlg, SEL_NONEW, BM_SETCHECK, IgnoreNew, 0);
			SendDlgItemMessage(hDlg, SEL_NOEXIST, BM_SETCHECK, IgnoreExist, 0);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, SEL_FNAME, WM_GETTEXT, 40+1, (LPARAM)FindStr);
					FindMode = SendDlgItemMessage(hDlg, SEL_REGEXP, BM_GETCHECK, 0, 0);
					IgnoreOld = SendDlgItemMessage(hDlg, SEL_NOOLD, BM_GETCHECK, 0, 0);
					IgnoreNew = SendDlgItemMessage(hDlg, SEL_NONEW, BM_GETCHECK, 0, 0);
					IgnoreExist = SendDlgItemMessage(hDlg, SEL_NOEXIST, BM_GETCHECK, 0, 0);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000061);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


/*----- ファイル一覧ウインドウのファイルを検索する ----------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		int Type : 検索方法 (FIND_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void FindFileInList(HWND hWnd, int Type)
{
	int Win;
	int i;
	int Num;
	static char RegExp[FMAX_PATH+1] = { "" };
	char Name[FMAX_PATH+1];
	LV_ITEM LvItem;
	char *Title;

	Win = WIN_LOCAL;
	Title = MSGJPN050;
	if(hWnd == GetRemoteHwnd())
	{
		Win = WIN_REMOTE;
		Title = MSGJPN051;
	}

	Num = GetItemCount(Win);
	switch(Type)
	{
		case FIND_FIRST :
			if(InputDialogBox(find_dlg, hWnd, Title, FindStr, 40+1, &FindMode, IDH_HELP_TOPIC_0000001) == YES)
			{
				strcpy(RegExp, FindStr);
//				if(FindMode == 0)
//					WildCard2RegExp(RegExp);

				_mbslwr(RegExp);
				if((FindMode == 0) || (JreCompileStr(RegExp) == TRUE))
				{
					for(i = GetCurrentItem(Win)+1; i < Num; i++)
					{
						GetNodeName(Win, i, Name, FMAX_PATH);
						_mbslwr(Name);

						LvItem.state = 0;
						if(((FindMode == 0) && (CheckFname(Name, RegExp) == FFFTP_SUCCESS)) ||
						   ((FindMode != 0) && (JreGetStrMatchInfo(Name, 0) != NULL)))
						{
							LvItem.mask = LVIF_STATE;
							LvItem.iItem = i;
							LvItem.state = LVIS_FOCUSED;
							LvItem.stateMask = LVIS_FOCUSED;
							LvItem.iSubItem = 0;
							SendMessage(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&LvItem);
							SendMessage(hWnd, LVM_ENSUREVISIBLE, i, (LPARAM)TRUE);
							break;
						}
					}
				}
			}
			break;

		case FIND_NEXT :
			for(i = GetCurrentItem(Win)+1; i < Num; i++)
			{
				GetNodeName(Win, i, Name, FMAX_PATH);
				_mbslwr(Name);

				LvItem.state = 0;
				if(((FindMode == 0) && (CheckFname(Name, RegExp) == FFFTP_SUCCESS)) ||
				   ((FindMode != 0) && (JreGetStrMatchInfo(Name, 0) != NULL)))
				{
					LvItem.mask = LVIF_STATE;
					LvItem.iItem = i;
					LvItem.state = LVIS_FOCUSED;
					LvItem.stateMask = LVIS_FOCUSED;
					LvItem.iSubItem = 0;
					SendMessage(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&LvItem);
					SendMessage(hWnd, LVM_ENSUREVISIBLE, i, (LPARAM)TRUE);
					break;
				}
			}
			break;
	}
	return;
}


#if 0
/*----- ワイルドカードを正規表現に変換する ------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void WildCard2RegExp(char *Str)
{
	char Tmp[FMAX_PATH+1];
	char *Org;
	char *Pos;
	UINT Ch;

	Org = Str;
	Pos = Tmp;

	*Pos++ = '^';
	*Pos++ = '(';
	while(*Str != NUL)
	{
		if(Pos >= Tmp + FMAX_PATH - 3)
			break;

		Ch = _mbsnextc(Str);
		Str = _mbsinc(Str);

		if(Ch <= 0x7F)
		{
			if(strchr("[]()^$.+", Ch) != NULL)
			{
				*Pos++ = '\\';
				*Pos++ = Ch;
			}
			else if(Ch == '*')
			{
				*Pos++ = '.';
				*Pos++ = '*';
			}
			else if(Ch == '?')
				*Pos++ = '.';
			else if(Ch == '|')
			{
				*Pos++ = '|';
			}
			else
				*Pos++ = Ch;
		}
		else
		{
			_mbsnset(Pos, Ch, 1);
			Pos = _mbsinc(Pos);
		}
	}
	*Pos++ = ')';
	*Pos++ = '$';
	*Pos = NUL;
	strcpy(Org, Tmp);

	return;
}
#endif


/*----- カーソル位置のアイテム番号を返す --------------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*
*	Return Value
*		int アイテム番号
*----------------------------------------------------------------------------*/

int GetCurrentItem(int Win)
{
	HWND hWnd;
	int Ret;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	if((Ret = SendMessage(hWnd, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_ALL | LVNI_FOCUSED, 0))) == -1)
		Ret = 0;

	return(Ret);
}


/*----- アイテム数を返す ------------------------------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*
*	Return Value
*		int アイテム数
*----------------------------------------------------------------------------*/

int GetItemCount(int Win)
{
	HWND hWnd;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	return(SendMessage(hWnd, LVM_GETITEMCOUNT, 0, 0));
}


/*----- 選択されているアイテム数を返す ----------------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*
*	Return Value
*		int 選択されているアイテム数
*----------------------------------------------------------------------------*/

int GetSelectedCount(int Win)
{
	HWND hWnd;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	return(SendMessage(hWnd, LVM_GETSELECTEDCOUNT, 0, 0));
}


/*----- 選択されている最初のアイテム番号を返す --------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*		int All : 選ばれていないものを含める
*
*	Return Value
*		int アイテム番号
*			-1 = 選択されていない
*----------------------------------------------------------------------------*/

int GetFirstSelected(int Win, int All)
{
	HWND hWnd;
	int Ope;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	Ope = LVNI_SELECTED;
	if(All == YES)
		Ope = LVNI_ALL;

	return(SendMessage(hWnd, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)MAKELPARAM(Ope, 0)));
}


/*----- 選択されている次のアイテム番号を返す ----------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*		int Pos : 今のアイテム番号
*		int All : 選ばれていないものも含める
*
*	Return Value
*		int アイテム番号
*			-1 = 選択されていない
*----------------------------------------------------------------------------*/

int GetNextSelected(int Win, int Pos, int All)
{
	HWND hWnd;
	int Ope;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	Ope = LVNI_SELECTED;
	if(All == YES)
		Ope = LVNI_ALL;

	return(SendMessage(hWnd, LVM_GETNEXTITEM, (WPARAM)Pos, (LPARAM)MAKELPARAM(Ope, 0)));
}


/*----- 指定された名前のアイテムを探す ----------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		char *Name : 名前
*
*	Return Value
*		int アイテム番号
*			-1=見つからなかった
*----------------------------------------------------------------------------*/

int FindNameNode(int Win, char *Name)
{
	LV_FINDINFO FindInfo;
	HWND hWnd;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	FindInfo.flags = LVFI_STRING;
	FindInfo.psz = Name;
	return(SendMessage(hWnd, LVM_FINDITEM, -1, (LPARAM)&FindInfo));
}


/*----- 指定位置のアイテムの名前を返す ----------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*		char *Buf : 名前を返すバッファ
*		int Max : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void GetNodeName(int Win, int Pos, char *Buf, int Max)
{
	HWND hWnd;
	LV_ITEM LvItem;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 0;
	LvItem.pszText = Buf;
	LvItem.cchTextMax = Max;
	SendMessage(hWnd, LVM_GETITEM, 0, (LPARAM)&LvItem);
	return;
}


/*----- 指定位置のアイテムの日付を返す ----------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*		FILETIME *Buf : 日付を返すバッファ
*
*	Return Value
*		int ステータス
*			YES/NO=日付情報がなかった
*----------------------------------------------------------------------------*/

int GetNodeTime(int Win, int Pos, FILETIME *Buf)
{
	HWND hWnd;
	LV_ITEM LvItem;
	char Tmp[20];
	int Ret;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 1;
	LvItem.pszText = Tmp;
	LvItem.cchTextMax = 20;
	SendMessage(hWnd, LVM_GETITEM, 0, (LPARAM)&LvItem);
	Ret = TimeString2FileTime(Tmp, Buf);
	return(Ret);
}


/*----- 指定位置のアイテムのサイズを返す --------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*		int *Buf : サイズを返すワーク
*
*	Return Value
*		int ステータス
*			YES/NO=サイズ情報がなかった
*----------------------------------------------------------------------------*/

int GetNodeSize(int Win, int Pos, LONGLONG *Buf)
{
	HWND hWnd;
	LV_ITEM LvItem;
	char Tmp[40];
	int Ret;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 2;
	LvItem.pszText = Tmp;
	LvItem.cchTextMax = 20;
	SendMessage(hWnd, LVM_GETITEM, 0, (LPARAM)&LvItem);
	*Buf = -1;
	Ret = NO;
	if(strlen(Tmp) > 0)
	{
		RemoveComma(Tmp);
		*Buf = _atoi64(Tmp);
		Ret = YES;
	}
	return(Ret);
}


/*----- 指定位置のアイテムの属性を返す ----------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*		int *Buf : 属性を返すワーク
*
*	Return Value
*		int ステータス
*			YES/NO=サイズ情報がなかった
*----------------------------------------------------------------------------*/

int GetNodeAttr(int Win, int Pos, int *Buf)
{
	LV_ITEM LvItem;
	char Tmp[20];
	int Ret;

	*Buf = 0;
	Ret = NO;
	if(Win == WIN_REMOTE)
	{
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Pos;
		LvItem.iSubItem = 4;
		LvItem.pszText = Tmp;
		LvItem.cchTextMax = 20;
		SendMessage(GetRemoteHwnd(), LVM_GETITEM, 0, (LPARAM)&LvItem);
		if(strlen(Tmp) > 0)
		{
			*Buf = AttrString2Value(Tmp);
			Ret = YES;
		}
	}
	return(Ret);
}


/*----- 指定位置のアイテムのタイプを返す --------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*
*	Return Value
*		int タイプ (NODE_xxx)
*----------------------------------------------------------------------------*/

int GetNodeType(int Win, int Pos)
{
	char Tmp[20];
	LV_ITEM LvItem;
	int Ret;
	HWND hWnd;

	hWnd = GetLocalHwnd();
	if(Win == WIN_REMOTE)
		hWnd = GetRemoteHwnd();

	LvItem.mask = LVIF_TEXT;
	LvItem.iItem = Pos;
	LvItem.iSubItem = 2;
	LvItem.pszText = Tmp;
	LvItem.cchTextMax = 20;
	SendMessage(hWnd, LVM_GETITEM, 0, (LPARAM)&LvItem);

	if(strcmp(Tmp, "<DIR>") == 0)
		Ret = NODE_DIR;
	else if(strcmp(Tmp, "<DRIVE>") == 0)
		Ret = NODE_DRIVE;
	else
		Ret = NODE_FILE;

	return(Ret);
}


/*----- 指定位置のアイテムのオーナ名を返す ------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*		char *Buf : オーナ名を返すバッファ
*		int Max : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void GetNodeOwner(int Win, int Pos, char *Buf, int Max)
{
	LV_ITEM LvItem;

	strcpy(Buf, "");
	if(Win == WIN_REMOTE)
	{
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Pos;
		LvItem.iSubItem = 5;
		LvItem.pszText = Buf;
		LvItem.cchTextMax = Max;
		SendMessage(GetRemoteHwnd(), LVM_GETITEM, 0, (LPARAM)&LvItem);
	}
	return;
}


/*----- ホスト側のファイル一覧ウインドウをクリア ------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void EraseRemoteDirForWnd(void)
{
	SendMessage(GetRemoteHwnd(), LVM_DELETEALLITEMS, 0, 0);
	SendMessage(GetRemoteHistHwnd(), CB_RESETCONTENT, 0, 0);
	return;
}


/*----- 選択されているファイルの総サイズを返す --------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*
*	Return Value
*		double サイズ
*----------------------------------------------------------------------------*/

double GetSelectedTotalSize(int Win)
{
	double Ret;
	LONGLONG Size;
	int Pos;

	Ret = 0;
	if(GetSelectedCount(Win) > 0)
	{
		Pos = GetFirstSelected(Win, NO);
		while(Pos != -1)
		{
			GetNodeSize(Win, Pos, &Size);
			if(Size >= 0)
				Ret += Size;
			Pos = GetNextSelected(Win, Pos, NO);
		}
	}
	return(Ret);
}



/*===================================================================

===================================================================*/



/*----- ファイル一覧で選ばれているファイルをリストに登録する ------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Expand : サブディレクトリを展開する (YES/NO)
*		int All : 選ばれていないものもすべて登録する (YES/NO)
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MakeSelectedFileList(int Win, int Expand, int All, FILELIST **Base, int *CancelCheckWork)
{
	int Pos;
	char Name[FMAX_PATH+1];
	char Cur[FMAX_PATH+1];
	FILELIST Pkt;
	int Node;
	DWORD Attr;
	int Ignore;

	if((All == YES) || (GetSelectedCount(Win) > 0))
	{
		/*===== カレントディレクトリのファイル =====*/

		Pos = GetFirstSelected(Win, All);
		while(Pos != -1)
		{
			Node = GetNodeType(Win, Pos);
			if((Node == NODE_FILE) ||
			   ((Expand == NO) && (Node == NODE_DIR)))
			{
				Pkt.InfoExist = 0;
				GetNodeName(Win, Pos, Pkt.File, FMAX_PATH);
				if(GetNodeSize(Win, Pos, &Pkt.Size) == YES)
					Pkt.InfoExist |= FINFO_SIZE;
				if(GetNodeAttr(Win, Pos, &Pkt.Attr) == YES)
					Pkt.InfoExist |= FINFO_ATTR;
				if(GetNodeTime(Win, Pos, &Pkt.Time) == YES)
					Pkt.InfoExist |= (FINFO_TIME | FINFO_DATE);
				Pkt.Node = Node;

				Ignore = NO;
				if((DispIgnoreHide == YES) && (Win == WIN_LOCAL))
				{
					AskLocalCurDir(Cur, FMAX_PATH);
					SetYenTail(Cur);
					strcat(Cur, Pkt.File);
					Attr = GetFileAttributes(Cur);
					if((Attr != 0xFFFFFFFF) && (Attr & FILE_ATTRIBUTE_HIDDEN))
						Ignore = YES;
				}

				if(Ignore == NO)
					AddFileList(&Pkt, Base);
			}
			Pos = GetNextSelected(Win, Pos, All);
		}

		if(Expand == YES)
		{
			/*===== ディレクトリツリー =====*/

			Pos = GetFirstSelected(Win, All);
			while(Pos != -1)
			{
				if(GetNodeType(Win, Pos) == NODE_DIR)
				{
					GetNodeName(Win, Pos, Name, FMAX_PATH);
					strcpy(Pkt.File, Name);
					ReplaceAll(Pkt.File, '\\', '/');
//8/26

					Ignore = NO;
					if((DispIgnoreHide == YES) && (Win == WIN_LOCAL))
					{
						AskLocalCurDir(Cur, FMAX_PATH);
						SetYenTail(Cur);
						strcat(Cur, Pkt.File);
						ReplaceAll(Cur, '/', '\\');
						Attr = GetFileAttributes(Cur);
						if((Attr != 0xFFFFFFFF) && (Attr & FILE_ATTRIBUTE_HIDDEN))
							Ignore = YES;
					}

					if(Ignore == NO)
					{
						Pkt.Node = NODE_DIR;
						Pkt.Attr = 0;
						Pkt.Size = 0;
						memset(&Pkt.Time, 0, sizeof(FILETIME));
						AddFileList(&Pkt, Base);

						if(Win == WIN_LOCAL)
							MakeLocalTree(Name, Base);
						else
						{
							AskRemoteCurDir(Cur, FMAX_PATH);

							if((AskListCmdMode() == NO) &&
							   (AskUseNLST_R() == YES))
								MakeRemoteTree1(Name, Cur, Base, CancelCheckWork);
							else
								MakeRemoteTree2(Name, Cur, Base, CancelCheckWork);

//DispListList(*Base, "LIST");

						}
					}
				}
				Pos = GetNextSelected(Win, Pos, All);
			}
		}
	}
	return;
}


/* デバッグ用 */
/* ファイルリストの内容を表示 */
static void DispListList(FILELIST *Pos, char *Title)
{
	DoPrintf("############ %s ############", Title);
	while(Pos != NULL)
	{
		DoPrintf("%d %s", Pos->Node, Pos->File);
		Pos = Pos->Next;
	}
	DoPrintf("############ END ############");
	return;
}


/*----- Drag&Dropされたファイルをリストに登録する -----------------------------
*
*	Parameter
*		WPARAM wParam : ドロップされたファイルの情報
*		char *Cur : カレントディレクトリを返すバッファ
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MakeDroppedFileList(WPARAM wParam, char *Cur, FILELIST **Base)
{
	int Max;
	int i;
	char Name[FMAX_PATH+1];
	char Tmp[FMAX_PATH+1];
	FILELIST Pkt;
	HANDLE fHnd;
	WIN32_FIND_DATA Find;

	Max = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);

	DragQueryFile((HDROP)wParam, 0, Cur, FMAX_PATH);
	GetUpperDir(Cur);

	for(i = 0; i < Max; i++)
	{
		DragQueryFile((HDROP)wParam, i, Name, FMAX_PATH);

		if((GetFileAttributes(Name) & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			Pkt.Node = NODE_FILE;
			strcpy(Pkt.File, GetFileName(Name));

			memset(&Pkt.Time, 0, sizeof(FILETIME));
			if((fHnd = FindFirstFile(Name, &Find)) != INVALID_HANDLE_VALUE)
			{
				FindClose(fHnd);
				Pkt.Time = Find.ftLastWriteTime;
			}
			AddFileList(&Pkt, Base);
		}
	}

	GetCurrentDirectory(FMAX_PATH, Tmp);
	SetCurrentDirectory(Cur);
	for(i = 0; i < Max; i++)
	{
		DragQueryFile((HDROP)wParam, i, Name, FMAX_PATH);

		if(GetFileAttributes(Name) & FILE_ATTRIBUTE_DIRECTORY)
		{
			Pkt.Node = NODE_DIR;
			strcpy(Pkt.File, GetFileName(Name));
			AddFileList(&Pkt, Base);

			MakeLocalTree(Pkt.File, Base);
		}
	}
	SetCurrentDirectory(Tmp);

	DragFinish((HDROP)wParam);

	return;
}


/*----- Drag&Dropされたファイルがあるフォルダを取得する -----------------------
*
*	Parameter
*		WPARAM wParam : ドロップされたファイルの情報
*		char *Cur : カレントディレクトリを返すバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MakeDroppedDir(WPARAM wParam, char *Cur)
{
	int Max;

	Max = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
	DragQueryFile((HDROP)wParam, 0, Cur, FMAX_PATH);
	GetUpperDir(Cur);
	DragFinish((HDROP)wParam);

	return;
}


/*----- ホスト側のサブディレクトリ以下のファイルをリストに登録する（１）-------
*
*	Parameter
*		char *Path : パス名
*		char *Cur : カレントディレクトリ
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*
*	Note
*		NLST -alLR を使う
*----------------------------------------------------------------------------*/

static void MakeRemoteTree1(char *Path, char *Cur, FILELIST **Base, int *CancelCheckWork)
{
	int Sts;

	if(DoCWD(Path, NO, NO, NO) == FTP_COMPLETE)
	{
		/* サブフォルダも含めたリストを取得 */
		Sts = DoDirListCmdSkt("R", "", 999, CancelCheckWork);	/* NLST -alLR*/
		DoCWD(Cur, NO, NO, NO);

		if(Sts == FTP_COMPLETE)
			AddRemoteTreeToFileList(999, Path, RDIR_NLST, Base);
	}
	return;
}


/*----- ホスト側のサブディレクトリ以下のファイルをリストに登録する（２）-------
*
*	Parameter
*		char *Path : パス名
*		char *Cur : カレントディレクトリ
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*
*	Note
*		各フォルダに移動してリストを取得
*----------------------------------------------------------------------------*/

static void MakeRemoteTree2(char *Path, char *Cur, FILELIST **Base, int *CancelCheckWork)
{
	int Sts;
	FILELIST *CurList;
	FILELIST *Pos;
	FILELIST Pkt;

	/* VAX VMS は CWD xxx/yyy という指定ができないので	*/
	/* CWD xxx, Cwd yyy と複数に分ける					*/
	if(AskHostType() != HTYPE_VMS)
		Sts = DoCWD(Path, NO, NO, NO);
	else
	{
#if defined(HAVE_OPENVMS)
		/* OpenVMSの場合、ディレクトリ移動時は"HOGE.DIR;1"を"HOGE"にする */
		ReformVMSDirName(Path, TRUE);
#endif
		Sts = DoCWDStepByStep(Path, Cur);
	}

	if(Sts == FTP_COMPLETE)
	{
		Sts = DoDirListCmdSkt("", "", 999, CancelCheckWork);		/* NLST -alL*/
		DoCWD(Cur, NO, NO, NO);

		if(Sts == FTP_COMPLETE)
		{
			CurList = NULL;
			AddRemoteTreeToFileList(999, Path, RDIR_CWD, &CurList);
			CopyTmpListToFileList(Base, CurList);

			Pos = CurList;
			while(Pos != NULL)
			{
				if(Pos->Node == NODE_DIR)
				{
					/* まずディレクトリ名をセット */
					strcpy(Pkt.File, Pos->File);
					Pkt.Node = NODE_DIR;
					Pkt.Size = 0;
					Pkt.Attr = 0;
					memset(&Pkt.Time, 0, sizeof(FILETIME));
					AddFileList(&Pkt, Base);

					/* そのディレクトリの中を検索 */
					MakeRemoteTree2(Pos->File, Cur, Base, CancelCheckWork);
				}
				Pos = Pos->Next;
			}
			DeleteFileList(&CurList);
		}
	}
	return;
}


/*----- ファイルリストの内容を別のファイルリストにコピー ----------------------
*
*	Parameter
*		FILELIST **Base : コピー先
*		FILELIST *List : コピー元
*
*	Return Value
*		なし
*
*	Note
*		コピーするのはファイルの情報だけ
*		ディレクトリの情報はコピーしない
*----------------------------------------------------------------------------*/

static void CopyTmpListToFileList(FILELIST **Base, FILELIST *List)
{
	while(List != NULL)
	{
		if(List->Node == NODE_FILE)
			AddFileList(List, Base);

		List = List->Next;
	}
	return;
}


/*----- ホスト側のファイル情報をファイルリストに登録 --------------------------
*
*	Parameter
*		int Num : テンポラリファイルのファイル名番号 (_ffftp.???)
*		char *Path : パス名
*		int IncDir : 再帰検索の方法 (RDIR_xxx)
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AddRemoteTreeToFileList(int Num, char *Path, int IncDir, FILELIST **Base)
{
	char Str[FMAX_PATH+1];
	char Dir[FMAX_PATH+1];
	char Name[FMAX_PATH+1];
	LONGLONG Size;
	FILETIME Time;
	int Attr;
	FILELIST Pkt;
	FILE *fd;
	int Node;
	int ListType;
	char Owner[OWNER_NAME_LEN+1];
	int Link;
	int InfoExist;

	MakeCacheFileName(Num, Str);
	if((fd = fopen(Str, "rb")) != NULL)
	{
		strcpy(Dir, Path);

		ListType = LIST_UNKNOWN;

		while(GetListOneLine(Str, FMAX_PATH, fd) == FFFTP_SUCCESS)
		{
			if((ListType = AnalizeFileInfo(Str)) == LIST_UNKNOWN)
			{
				if(MakeDirPath(Str, ListType, Path, Dir) == FFFTP_SUCCESS)
				{
					if(IncDir == RDIR_NLST)
					{
						strcpy(Pkt.File, Dir);
						Pkt.Node = NODE_DIR;
						Pkt.Size = 0;
						Pkt.Attr = 0;
						memset(&Pkt.Time, 0, sizeof(FILETIME));
						AddFileList(&Pkt, Base);
					}
				}
			}
			else
			{
				Node = ResolvFileInfo(Str, ListType, Name, &Size, &Time, &Attr, Owner, &Link, &InfoExist);

				if(AskFilterStr(Name, Node) == YES)
				{
					if((Node == NODE_FILE) ||
					   ((IncDir == RDIR_CWD) && (Node == NODE_DIR)))
					{
						strcpy(Pkt.File, Dir);
						if(strlen(Pkt.File) > 0)
							SetSlashTail(Pkt.File);
						strcat(Pkt.File, Name);
						Pkt.Node = Node;
						Pkt.Link = Link;
						Pkt.Size = Size;
						Pkt.Attr = Attr;
						Pkt.Time = Time;
						Pkt.InfoExist = InfoExist;
						AddFileList(&Pkt, Base);
					}
				}
			}
		}
		fclose(fd);
	}
	return;
}


/*----- ファイル一覧情報の１行を取得 ------------------------------------------
*
*	Parameter
*		char *Buf : １行の情報をセットするバッファ
*		int Max : 最大文字数
*		FILE *Fd : ストリーム
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*
*	Note
*		VAX VMS以外の時は fgets(Buf, Max, Fd) と同じ
*		Vax VMSの時は、複数行のファイル情報を１行にまとめる
*----------------------------------------------------------------------------*/

static int GetListOneLine(char *Buf, int Max, FILE *Fd)
{
	char Tmp[FMAX_PATH+1];
	int Sts;

	Sts = FFFTP_FAIL;
	while((Sts == FFFTP_FAIL) && (fgets(Buf, Max, Fd) != NULL))
	{
		Sts = FFFTP_SUCCESS;
		RemoveReturnCode(Buf);
		ReplaceAll(Buf, '\x08', ' ');

		/* VAX VMSではファイル情報が複数行にわかれている	*/
		/* それを１行にまとめる								*/
		if(AskHostType() == HTYPE_VMS)
		{
			if(strchr(Buf, ';') == NULL)	/* ファイル名以外の行 */
				Sts = FFFTP_FAIL;
			else
			{
				Max -= strlen(Buf);
				while(strchr(Buf, ')') == NULL)
				{
					if(fgets(Tmp, FMAX_PATH, Fd) != NULL)
					{
						RemoveReturnCode(Tmp);
						ReplaceAll(Buf, '\x08', ' ');
						if((int)strlen(Tmp) > Max)
							Tmp[Max] = NUL;
						Max -= strlen(Tmp);
						strcat(Buf, Tmp);
					}
					else
						break;
				}
			}
		}
	}

//	DoPrintf("List : %s", Buf);

	return(Sts);
}


/*----- サブディレクトリ情報の解析 --------------------------------------------
*
*	Parameter
*		char *Str : ファイル情報（１行）
*		int ListType : リストのタイプ
*		char *Path : 先頭からのパス名
*		char *Dir : ディレクトリ名
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL=ディレクトリ情報でない
*----------------------------------------------------------------------------*/

static int MakeDirPath(char *Str, int ListType, char *Path, char *Dir)
{
	int Sts;

	Sts = FFFTP_FAIL;
	switch(ListType)
	{
		case LIST_ACOS :
		case LIST_ACOS_4 :
			break;

		default:
			if(*(Str + strlen(Str) - 1) == ':')		/* 最後が : ならサブディレクトリ */
			{
				if(strcmp(Str, ".:") != 0)
				{
					if((strncmp(Str, "./", 2) == 0) ||
					   (strncmp(Str, ".\\", 2) == 0))
					{
						Str += 2;
					}

					if(strlen(Str) > 1)
					{
						strcpy(Dir, Path);
						SetSlashTail(Dir);
						strcat(Dir, Str);
						*(Dir + strlen(Dir) - 1) = NUL;

						ChangeFnameRemote2Local(Dir, FMAX_PATH);

						ReplaceAll(Dir, '\\', '/');
					}
				}
				Sts = FFFTP_SUCCESS;
			}
			break;
	}
	return(Sts);
}


/*----- ローカル側のサブディレクトリ以下のファイルをリストに登録する ----------
*
*	Parameter
*		char *Path : パス名
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void MakeLocalTree(char *Path, FILELIST **Base)
{
	char Src[FMAX_PATH+1];
	HANDLE fHnd;
	WIN32_FIND_DATA FindBuf;
	FILELIST Pkt;
	SYSTEMTIME TmpStime;

	strcpy(Src, Path);
	SetYenTail(Src);
	strcat(Src, "*");
	ReplaceAll(Src, '/', '\\');

	if((fHnd = FindFirstFileAttr(Src, &FindBuf, DispIgnoreHide)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((FindBuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				if(AskFilterStr(FindBuf.cFileName, NODE_FILE) == YES)
				{
					strcpy(Pkt.File, Path);
					SetSlashTail(Pkt.File);
					strcat(Pkt.File, FindBuf.cFileName);
					ReplaceAll(Pkt.File, '\\', '/');
					Pkt.Node = NODE_FILE;
					Pkt.Size = MakeLongLong(FindBuf.nFileSizeHigh, FindBuf.nFileSizeLow);
					Pkt.Attr = 0;
					Pkt.Time = FindBuf.ftLastWriteTime;
					FileTimeToSystemTime(&Pkt.Time, &TmpStime);
					TmpStime.wSecond = 0;
					TmpStime.wMilliseconds = 0;
					SystemTimeToFileTime(&TmpStime, &Pkt.Time);
					AddFileList(&Pkt, Base);
				}
			}
		}
		while(FindNextFileAttr(fHnd, &FindBuf, DispIgnoreHide) == TRUE);
		FindClose(fHnd);
	}

	if((fHnd = FindFirstFileAttr(Src, &FindBuf, DispIgnoreHide)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((FindBuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			   (strcmp(FindBuf.cFileName, ".") != 0) &&
			   (strcmp(FindBuf.cFileName, "..") != 0))
			{
				strcpy(Src, Path);
				SetYenTail(Src);
				strcat(Src, FindBuf.cFileName);
				strcpy(Pkt.File, Src);
				ReplaceAll(Pkt.File, '\\', '/');
				Pkt.Node = NODE_DIR;
				Pkt.Size = 0;
				Pkt.Attr = 0;
				memset(&Pkt.Time, 0, sizeof(FILETIME));
				AddFileList(&Pkt, Base);

				MakeLocalTree(Src, Base);
			}
		}
		while(FindNextFileAttr(fHnd, &FindBuf, DispIgnoreHide) == TRUE);
		FindClose(fHnd);
	}
	return;
}


/*----- ファイルリストに情報を登録する ----------------------------------------
*
*	Parameter
*		FILELIST *Pkt : 登録するファイル情報
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AddFileList(FILELIST *Pkt, FILELIST **Base)
{
	FILELIST *Pos;
	FILELIST *Prev;

	DoPrintf("FileList : NODE=%d : %s", Pkt->Node, Pkt->File);

	/* リストの重複を取り除く */
	Pos = *Base;
	while(Pos != NULL)
	{
		if(strcmp(Pkt->File, Pos->File) == 0)
		{
			DoPrintf(" --> Duplicate!!");
			break;
		}
		Prev = Pos;
		Pos = Pos->Next;
	}

	if(Pos == NULL)		/* 重複していないので登録する */
	{
		if((Pos = malloc(sizeof(FILELIST))) != NULL)
		{
			memcpy(Pos, Pkt, sizeof(FILELIST));
			Pos->Next = NULL;

			if(*Base == NULL)
				*Base = Pos;
			else
				Prev->Next = Pos;
		}
	}
	return;
}


/*----- ファイルリストをクリアする --------------------------------------------
*
*	Parameter
*		FILELIST **Base : ファイルリストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteFileList(FILELIST **Base)
{
	FILELIST *New;
	FILELIST *Next;

	New = *Base;
	while(New != NULL)
	{
		Next = New->Next;
		free(New);
		New = Next;
	}
	*Base = NULL;
	return;
}


/*----- ファイルリストに指定のファイルがあるかチェック ------------------------
*
*	Parameter
*		char *Fname : ファイル名
*		FILELIST *Base : ファイルリストの先頭
*		int Caps : 大文字/小文字の区別モード (COMP_xxx)
*
*	Return Value
*		FILELIST *見つかったファイルリストのデータ
*			NULL=見つからない
*----------------------------------------------------------------------------*/

FILELIST *SearchFileList(char *Fname, FILELIST *Base, int Caps)
{
	char Tmp[FMAX_PATH+1];

	while(Base != NULL)
	{
		if(Caps == COMP_STRICT)
		{
			if(_mbscmp(Fname, Base->File) == 0)
				break;
		}
		else
		{
			if(_mbsicmp(Fname, Base->File) == 0)
			{
				if(Caps == COMP_IGNORE)
					break;
				else
				{
					strcpy(Tmp, Base->File);
					_mbslwr(Tmp);
					if(_mbscmp(Tmp, Base->File) == 0)
						break;
				}
			}
		}
		Base = Base->Next;
	}
	return(Base);
}


/*----- ファイル情報からリストタイプを求める ----------------------------------
*
*	Parameter
*		char *Str : ファイル情報（１行）
*
*	Return Value
*		int リストタイプ (LIST_xxx)
*----------------------------------------------------------------------------*/

static int AnalizeFileInfo(char *Str)
{
	int Ret;
	char Tmp[FMAX_PATH+1];
	int Add1;
	int TmpInt;
	int Flag1;
	WORD Month;
	WORD Day;

//DoPrintf("LIST : %s", Str);

	Ret = LIST_UNKNOWN;
	Flag1 = AskHostType();
	if(Flag1 == HTYPE_ACOS)
		Ret = LIST_ACOS;
	else if(Flag1 == HTYPE_ACOS_4)
		Ret = LIST_ACOS_4;
	else if(Flag1 == HTYPE_VMS)
		Ret = LIST_VMS;
	else if(Flag1 == HTYPE_IRMX)
		Ret = LIST_IRMX;
	else if(Flag1 == HTYPE_STRATUS)
		Ret = LIST_STRATUS;
	else if(Flag1 == HTYPE_AGILENT)
		Ret = LIST_AGILENT;
	else if(Flag1 == HTYPE_SHIBASOKU)
		Ret = LIST_SHIBASOKU;
	else
	{
		/* 以下のフォーマットをチェック */
		/* LIST_UNIX_10, LIST_UNIX_20, LIST_UNIX_12, LIST_UNIX_22, LIST_UNIX_50, LIST_UNIX_60 */
		/* MELCOM80 */

		if(FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS)
		{
			/* MELCOM80は "d rwxrwxrwx" のようにスペースが空いている */
			Flag1 = NO;
			if((strlen(Tmp) == 1) && (strchr("-dDlL", Tmp[0]) != NULL))
			{
				if(FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS)
				{
					if((strlen(Tmp) == 9) ||
					   ((strlen(Tmp) > 9) && (IsDigit(Tmp[9]) != 0)))
					{
						memmove(Str+1, Str+2, strlen(Str+2)+1);
						FindField(Str, Tmp, 0, NO);
						Flag1 = YES;
					}
				}
			}

			if(strlen(Tmp) >= 10)
			{
				Add1 = 0;
				if((strlen(Tmp) > 10) && (IsDigit(Tmp[10]) != 0))
				{
					/* こういう時 */
					/*   drwxr-xr-x1234  owner group  1024  Nov 6 14:21 Linux/    */
					Add1 = -1;
				}

////////////
// LIST_UNIX_60 support
				if(FindField(Str, Tmp, 7+Add1, NO) == FFFTP_SUCCESS)
				{
					GetMonth(Tmp, &Month, &Day);
					if(Month != 0)
					{
						Ret = CheckUnixType(Str, Tmp, Add1, 2, Day);
					}
				}
///////////

////////////
// LIST_UNIX_12 support
				if((Ret == LIST_UNKNOWN) &&
				   (FindField(Str, Tmp, 6+Add1, NO) == FFFTP_SUCCESS))
				{
					GetMonth(Tmp, &Month, &Day);
					if(Month != 0)
					{
						Ret = CheckUnixType(Str, Tmp, Add1, 0, Day);
					}
				}
//////////////////

////////////
// LIST_UNIX_70 support
				if((Ret == LIST_UNKNOWN) &&
				   (FindField(Str, Tmp, 6+Add1, NO) == FFFTP_SUCCESS))
				{
					GetMonth(Tmp, &Month, &Day);
					if(Month != 0)
					{
						Ret = CheckUnixType(Str, Tmp, Add1, 1, Day);
					}
				}
///////////

				if((Ret == LIST_UNKNOWN) &&
				   (FindField(Str, Tmp, 5+Add1, NO) == FFFTP_SUCCESS))
				{
					GetMonth(Tmp, &Month, &Day);
					if(Month != 0)
					{
						Ret = CheckUnixType(Str, Tmp, Add1, 0, Day);
					}
				}

				if((Ret == LIST_UNKNOWN) &&
				   (FindField(Str, Tmp, 4+Add1, NO) == FFFTP_SUCCESS))
				{
					GetMonth(Tmp, &Month, &Day);
					if(Month != 0)
					{
						Ret = CheckUnixType(Str, Tmp, Add1, -1, Day);
					}
				}

				if((Ret == LIST_UNKNOWN) &&
				   (FindField(Str, Tmp, 3+Add1, NO) == FFFTP_SUCCESS))
				{
					GetMonth(Tmp, &Month, &Day);
					if(Month != 0)
					{
						Ret = CheckUnixType(Str, Tmp, Add1, -2, Day);
					}
				}

				if((Ret != LIST_UNKNOWN) && (Flag1 == YES))
					Ret |= LIST_MELCOM;
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_AS400 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
			   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
			{
				if((FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS) &&
				   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
				{
					if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
					   (IsDigit(Tmp[0]) != 0))
					{
						if(FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_AS400;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_M1800 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS) &&
			   (CheckYYMMDDformat(Tmp, '*', NO) != 0))
			{
				if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
				   ((IsDigit(Tmp[0]) != 0) || (StrAllSameChar(Tmp, '*') == YES)))
				{
					if((FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS) &&
					   ((IsDigit(Tmp[0]) != 0) || (StrAllSameChar(Tmp, '*') == YES)))
					{
						if((FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS) &&
						   (strlen(Tmp) == 4))
						{
							if(FindField(Str, Tmp, 6, NO) == FFFTP_SUCCESS)
							{
								Ret = LIST_M1800;
							}
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_GP6000 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
			   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
			{
				if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
				   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
				{
					if((FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS) &&
					   (IsDigit(Tmp[0]) != 0))
					{
						if(FindField(Str, Tmp, 6, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_GP6000;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_DOS_1, LIST_DOS_2 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
			   (CheckHHMMformat(Tmp) == YES))
			{
				if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
				   ((Tmp[0] == '<') || (IsDigit(Tmp[0]) != 0)))
				{
					if(FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS)
					{
						if((FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS) &&
						   (CheckYYMMDDformat(Tmp, NUL, YES) != 0))
						{
							TmpInt = atoi(Tmp);
							if((TmpInt >= 1) && (TmpInt <= 12))
								Ret = LIST_DOS_2;
							else
								Ret = LIST_DOS_1;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_DOS_3 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS) &&
			   (CheckHHMMformat(Tmp) == YES))
			{
				if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
				   ((Tmp[0] == '<') || (IsDigit(Tmp[0]) != 0)))
				{
					if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
					   (CheckYYMMDDformat(Tmp, NUL, YES) != 0))
					{
						Ret = LIST_DOS_3;
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_DOS_4 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS) &&
			   (CheckYYYYMMDDformat(Tmp, NUL) == YES))
			{
				if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
				   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
				{
					if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
					   ((Tmp[0] == '<') || (IsDigit(Tmp[0]) != 0)))
					{
						if(FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_DOS_4;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_CHAMELEON */

		if(Ret == LIST_UNKNOWN)
		{
			if(FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS)
			{
				GetMonth(Tmp, &Month, &Day);
				if((Month != 0) && (Day == 0))
				{
					if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
					   ((Tmp[0] == '<') || (IsDigit(Tmp[0]) != 0)))
					{
						if((FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS) &&
						   (CheckHHMMformat(Tmp) == YES))
						{
							Ret = LIST_CHAMELEON;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_OS2 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS) &&
			   (CheckHHMMformat(Tmp) == YES))
			{
				if((FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS) &&
				   (IsDigit(Tmp[0]) != 0))
				{
					if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
					   (CheckYYMMDDformat(Tmp, NUL, YES) != 0))
					{
						if(FindField(Str, Tmp, 4, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_OS2;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_OS7 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS) &&
			   (strlen(Tmp) == 10))
			{
				if((FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS) &&
				   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
				{
					if((FindField(Str, Tmp, 4, NO) == FFFTP_SUCCESS) &&
					   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
					{
						if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
						   (IsDigit(Tmp[0]) != 0))
						{
							if(FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS)
							{
								Ret = LIST_OS7_2;
							}
						}
					}
				}
				else if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
						(CheckYYMMDDformat(Tmp, NUL, NO) != 0))
				{
					if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
					   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
					{
						if(FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_OS7_1;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_ALLIED */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 0, NO) == FFFTP_SUCCESS) &&
			   ((Tmp[0] == '<') || (IsDigit(Tmp[0]) != 0)))
			{
				if((FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS) &&
				   (CheckHHMMformat(Tmp) == YES))
				{
					if(FindField(Str, Tmp, 3, NO) == FFFTP_SUCCESS)
					{
						GetMonth(Tmp, &Month, &Day);
						if(Month != 0)
						{
							if((FindField(Str, Tmp, 6, NO) == FFFTP_SUCCESS) &&
							   (IsDigit(Tmp[0]) != 0))
							{
								Ret = LIST_ALLIED;
							}
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_OS9 */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) &&
			   (CheckYYMMDDformat(Tmp, NUL, NO) != 0))
			{
				if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
				   (IsDigit(Tmp[0]) != 0) && (strlen(Tmp) == 4))
				{
					if((FindField(Str, Tmp, 5, NO) == FFFTP_SUCCESS) &&
					   (IsDigit(Tmp[0]) != 0))
					{
						if(FindField(Str, Tmp, 6, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_OS9;
						}
					}
				}
			}
		}

		/* 以下のフォーマットをチェック */
		/* LIST_IBM */

		if(Ret == LIST_UNKNOWN)
		{
			if((FindField(Str, Tmp, 2, NO) == FFFTP_SUCCESS) &&
			   (CheckYYYYMMDDformat(Tmp, NUL) == YES))
			{
				if((FindField(Str, Tmp, 1, NO) == FFFTP_SUCCESS) && IsDigit(Tmp[0]))
				{
					if((FindField(Str, Tmp, 7, NO) == FFFTP_SUCCESS) && IsDigit(Tmp[0]))
					{
						if(FindField(Str, Tmp, 9, NO) == FFFTP_SUCCESS)
						{
							Ret = LIST_IBM;
						}
					}
				}
			}
		}

		// MLSD対応
		if(Ret == LIST_UNKNOWN)
		{
			if(FindField2(Str, Tmp, ';', 1, NO) == FFFTP_SUCCESS && FindField2(Str, Tmp, '=', 1, NO) == FFFTP_SUCCESS)
			{
				Ret = LIST_MLSD;
			}
		}
	}

DoPrintf("ListType=%d", Ret);

	return(Ret);
}


/*----- UNIX系リストタイプのチェックを行なう ----------------------------------
*
*	Parameter
*		char *Str : ファイル情報（１行）
*		char *Tmp : 一時ワーク
*		int Add1 : 加算パラメータ1
*		int Add2 : 加算パラメータ2
*		int Day : 日 (0=ここで取得する)
*
*	Return Value
*		int リストタイプ (LIST_xxx)
*----------------------------------------------------------------------------*/

static int CheckUnixType(char *Str, char *Tmp, int Add1, int Add2, int Day)
{
	int Ret;
	int Add3;
	WORD Hour;
	WORD Minute;
	int Flag;

	Flag = 0;
	Ret = LIST_UNKNOWN;

//DayによってAdd3を変える

	Add3 = 0;
	if(Day != 0)
		Add3 = -1;

	// unix系チェック
	if((Day != 0) ||
	   ((FindField(Str, Tmp, 6+Add1+Add2+Add3, NO) == FFFTP_SUCCESS) &&
		((atoi(Tmp) >= 1) && (atoi(Tmp) <= 31))))
	{
		if((FindField(Str, Tmp, 7+Add1+Add2+Add3, NO) == FFFTP_SUCCESS) &&
		   ((atoi(Tmp) >= 1900) || (GetHourAndMinute(Tmp, &Hour, &Minute) == FFFTP_SUCCESS)))
		{
			if(FindField(Str, Tmp, 8+Add1+Add2+Add3, NO) == FFFTP_SUCCESS)
			{
				Flag = 1;
			}
		}
	}

	// 中国語Solaris専用
	if(Flag == 0)
	{
	   if((FindField(Str, Tmp, 7+Add1+Add2+Add3, NO) == FFFTP_SUCCESS) &&
		  ((atoi(Tmp) >= 1) && (atoi(Tmp) <= 31)))
		{
			if((FindField(Str, Tmp, 5+Add1+Add2+Add3, NO) == FFFTP_SUCCESS) &&
			   (atoi(Tmp) >= 1900))
			{
				if((FindField(Str, Tmp, 6+Add1+Add2+Add3, NO) == FFFTP_SUCCESS) &&
				   (((atoi(Tmp) >= 1) && (atoi(Tmp) <= 9) && 
					 ((unsigned char)Tmp[1] == 0xD4) &&
					 ((unsigned char)Tmp[2] == 0xC2)) ||
				    ((atoi(Tmp) >= 10) && (atoi(Tmp) <= 12) && 
					 ((unsigned char)Tmp[2] == 0xD4) && 
					 ((unsigned char)Tmp[3] == 0xC2))))
				{
					if(FindField(Str, Tmp, 8+Add1+Add2+Add3, NO) == FFFTP_SUCCESS)
					{
						Flag = 2;
					}
				}
			}
		}
	}

	if(Flag != 0)
	{
		if(Add2 == 2)
		{
			Ret = LIST_UNIX_60;
			if(Flag == 2)
				Ret = LIST_UNIX_64;
			if(Day != 0)
				Ret = LIST_UNIX_61;

			if(Add1 == -1)
			{
				Ret = LIST_UNIX_62;
				if(Flag == 2)
					Ret = LIST_UNIX_65;
				if(Day != 0)
					Ret = LIST_UNIX_63;
			}
		}
		else if(Add2 == 1)
		{
			Ret = LIST_UNIX_70;
			if(Flag == 2)
				Ret = LIST_UNIX_74;
			if(Day != 0)
				Ret = LIST_UNIX_71;

			if(Add1 == -1)
			{
				Ret = LIST_UNIX_72;
				if(Flag == 2)
					Ret = LIST_UNIX_75;
				if(Day != 0)
					Ret = LIST_UNIX_73;
			}
		}
		else if(Add2 == 0)
		{
			Ret = LIST_UNIX_10;
			if(Flag == 2)
				Ret = LIST_UNIX_14;
			if(Day != 0)
				Ret = LIST_UNIX_11;

			if(Add1 == -1)
			{
				Ret = LIST_UNIX_12;
				if(Flag == 2)
					Ret = LIST_UNIX_15;
				if(Day != 0)
					Ret = LIST_UNIX_13;
			}
		}
		else if(Add2 == -1)
		{
			Ret = LIST_UNIX_20;
			if(Flag == 2)
				Ret = LIST_UNIX_24;
			if(Day != 0)
				Ret = LIST_UNIX_21;

			if(Add1 == -1)
			{
				Ret = LIST_UNIX_22;
				if(Flag == 2)
					Ret = LIST_UNIX_25;
				if(Day != 0)
					Ret = LIST_UNIX_23;
			}
		}
		else
		{
			Ret = LIST_UNIX_50;
			if(Flag == 2)
				Ret = LIST_UNIX_54;
			if(Day != 0)
				Ret = LIST_UNIX_51;
		}
	}
	return(Ret);
}


/*----- HH:MM 形式の文字列かどうかをチェック ----------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		int ステータス (YES/NO)
*
*	Note
*		区切り文字は何でもよい
*		時分でなくてもよい
*		後ろに余分な文字が付いていてもよい
*----------------------------------------------------------------------------*/

static int CheckHHMMformat(char *Str)
{
	int Ret;

	Ret = NO;
	if((strlen(Str) >= 3) &&
	   (IsDigit(Str[0]) != 0))
	{
		if((Str = strchr(Str, ':')) != NULL)
		{
			if(IsDigit(*(Str+1)) != 0)
				Ret = YES;
		}
	}
	return(Ret);
}


/*----- YY/MM/DD 形式の文字列かどうかをチェック -------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char Sym : 数字の代わりに使える記号 (NUL=数字以外使えない)
*		int Dig3 : 3桁の年を許可
*
*	Return Value
*		int ステータス
*			0 = 該当しない
*			1 = ??/??/??, ??/??/???
*			2 = ???/??/??
*
*	Note
*		区切り文字は何でもよい
*		年月日でなくてもよい
*----------------------------------------------------------------------------*/

static int CheckYYMMDDformat(char *Str, char Sym, int Dig3)
{
	int Ret;

	Ret = 0;
	if((strlen(Str) == 8) &&
	   (IsDigitSym(Str[0], Sym) != 0) && (IsDigitSym(Str[1], Sym) != 0) &&
	   (IsDigit(Str[2]) == 0) &&
	   (IsDigitSym(Str[3], Sym) != 0) && (IsDigitSym(Str[4], Sym) != 0) &&
	   (IsDigit(Str[5]) == 0) &&
	   (IsDigitSym(Str[6], Sym) != 0) && (IsDigitSym(Str[7], Sym) != 0))
	{
		Ret = 1; 
	}
	if(Dig3 == YES)
	{
		if((strlen(Str) == 9) &&
		   (IsDigitSym(Str[0], Sym) != 0) && (IsDigitSym(Str[1], Sym) != 0) && (IsDigitSym(Str[2], Sym) != 0) &&
		   (IsDigit(Str[3]) == 0) &&
		   (IsDigitSym(Str[4], Sym) != 0) && (IsDigitSym(Str[5], Sym) != 0) &&
		   (IsDigit(Str[6]) == 0) &&
		   (IsDigitSym(Str[7], Sym) != 0) && (IsDigitSym(Str[8], Sym) != 0))
		{
			Ret = 2; 
		}
		else if((strlen(Str) == 9) &&
				(IsDigitSym(Str[0], Sym) != 0) && (IsDigitSym(Str[1], Sym) != 0) &&
				(IsDigit(Str[2]) == 0) &&
				(IsDigitSym(Str[3], Sym) != 0) && (IsDigitSym(Str[4], Sym) != 0) &&
				(IsDigit(Str[5]) == 0) &&
				(IsDigitSym(Str[6], Sym) != 0) && (IsDigitSym(Str[7], Sym) != 0) && (IsDigitSym(Str[8], Sym) != 0))
		{
			Ret = 1; 
		}
	}
	return(Ret);
}


/*----- YYYY/MM/DD 形式の文字列かどうかをチェック -----------------------------
*
*	Parameter
*		char *Str : 文字列
*		char Sym : 数字の代わりに使える記号 (NUL=数字以外使えない)
*
*	Return Value
*		int ステータス (YES/NO)
*
*	Note
*		区切り文字は何でもよい
*		年月日でなくてもよい
*----------------------------------------------------------------------------*/

static int CheckYYYYMMDDformat(char *Str, char Sym)
{
	int Ret;

	Ret = NO;
	if((strlen(Str) == 10) &&
	   (IsDigitSym(Str[0], Sym) != 0) && (IsDigitSym(Str[1], Sym) != 0) &&
	   (IsDigitSym(Str[2], Sym) != 0) && (IsDigitSym(Str[3], Sym) != 0) &&
	   (IsDigit(Str[4]) == 0) &&
	   (IsDigitSym(Str[5], Sym) != 0) && (IsDigitSym(Str[6], Sym) != 0) &&
	   (IsDigit(Str[7]) == 0) &&
	   (IsDigitSym(Str[8], Sym) != 0) && (IsDigitSym(Str[9], Sym) != 0))
	{
		Ret = YES; 
	}
	return(Ret);
}


/*----- ファイル情報からファイル名、サイズなどを取り出す ----------------------
*
*	Parameter
*		char *Str : ファイル情報（１行）
*		int ListType : リストのタイプ
*		char *Name : ファイル名のコピー先
*		LONGLONG *Size : サイズのコピー先
*		FILETIME *Time : 日付のコピー先
*		int *Attr : 属性のコピー先
*		char *Owner : オーナ名
*		int *Link : リンクかどうか (YES/NO)
*		int *InfoExist : 時刻の情報があったかどうか (YES/NO)
*
*	Return Value
*		int 種類 (NODE_xxxx)
*----------------------------------------------------------------------------*/

static int ResolvFileInfo(char *Str, int ListType, char *Fname, LONGLONG *Size, FILETIME *Time, int *Attr, char *Owner, int *Link, int *InfoExist)
{
	SYSTEMTIME sTime;
	SYSTEMTIME sTimeNow;
	char Buf[FMAX_PATH+1];
	char *Pos;
	char Flag;
	int Ret;
	int offs;
	int offs2;
	int offs3;
	int OrgListType;
	int err;
	int Flag2;

	static const int DosPos[3][4] = { { 1, 0, 2, 3 }, { 1, 0, 2, 3 }, { 3, 2, 1, 0 } };
	static const int DosDate[3][3][2] = { { {0, 0}, {3, 4}, {6, 7} }, { {6, 7}, {0, 0}, {3, 4} }, { {6, 7}, {0, 0}, {3, 4} } };
	static const int DosLongFname[3] = { YES, YES, NO };

	/* まずクリアしておく */
	Ret = NODE_NONE;
	*Size = -1;
	*Attr = 0;
	*Link = NO;
	memset(Owner, NUL, OWNER_NAME_LEN+1);
	Time->dwLowDateTime = 0;
	Time->dwHighDateTime = 0;
	*InfoExist = 0;
	offs = 0;
	offs2 = 0;
	offs3 = 0;

	OrgListType = ListType;
	ListType &= LIST_MASKFLG;
	switch(ListType)
	{
		case LIST_DOS_1 :
		case LIST_DOS_2 :
		case LIST_DOS_3 :
			if(ListType == LIST_DOS_1)
				offs = 0;
			else if(ListType == LIST_DOS_2)
				offs = 1;
			else
				offs = 2;

			*InfoExist |= (FINFO_DATE | FINFO_SIZE);

			/* 時刻 */
			FindField(Str, Buf, DosPos[offs][0], NO);
			if((Pos = strchr(Buf, ':')) != NULL)
			{
				*InfoExist |= FINFO_TIME;
				sTime.wHour = atoi(Buf);
				sTime.wMinute = atoi(Pos+1);
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;

				if(strlen(Pos) >= 4)
				{
					if(tolower(Pos[3]) == 'a')
					{
						if(sTime.wHour == 12)
							sTime.wHour = 0;
					}
					else if(tolower(Pos[3]) == 'p')
					{
						if(sTime.wHour != 12)
							sTime.wHour += 12;
					}
				}
			}

			/* 日付 */
			FindField(Str, Buf, DosPos[offs][1], NO);
			if((offs2 = CheckYYMMDDformat(Buf, NUL, YES)) == 0)
				break;
			offs2--;
			sTime.wYear = Assume1900or2000(atoi(Buf + DosDate[offs][0][offs2]));
			sTime.wMonth = atoi(Buf + DosDate[offs][1][offs2]);
			sTime.wDay = atoi(Buf + DosDate[offs][2][offs2]);
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, DosPos[offs][2], NO);
			*Size = _atoi64(Buf);

			/* 名前 */
			if(FindField(Str, Fname, DosPos[offs][3], DosLongFname[offs]) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if(Buf[0] == '<')
					Ret = NODE_DIR;
			}
			break;

		case LIST_DOS_4 :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE);

			/* 日付 */
			FindField(Str, Buf, 0, NO);
			sTime.wYear = atoi(Buf);
			sTime.wMonth = atoi(Buf+5);
			sTime.wDay = atoi(Buf+8);

			/* 時刻 */
			*InfoExist |= FINFO_TIME;
			FindField(Str, Buf, 1, NO);
			sTime.wHour = atoi(Buf);
			sTime.wMinute = atoi(Buf+3);
			sTime.wSecond = 0;				// atoi(Buf+6);
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 2, NO);
			*Size = _atoi64(Buf);

			/* 名前 */
			if(FindField(Str, Fname, 3, YES) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if(Buf[0] == '<')
					Ret = NODE_DIR;
			}
			break;

		case LIST_OS2 :
			*InfoExist |= (FINFO_DATE | FINFO_SIZE);

			/* 時刻 */
			FindField(Str, Buf, 3, NO);
			if((Pos = strchr(Buf, ':')) != NULL)
			{
				*InfoExist |= FINFO_TIME;
				sTime.wHour = atoi(Buf);
				sTime.wMinute = atoi(Pos+1);
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;
			}

			/* 日付 */
			FindField(Str, Buf, 2, NO);
			sTime.wYear = Assume1900or2000(atoi(Buf+6));
			sTime.wMonth = atoi(Buf+0);
			sTime.wDay = atoi(Buf+3);
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 0, NO);
			*Size = _atoi64(Buf);

			/* 名前 */
			if(FindField(Str, Fname, 4, YES) == FFFTP_SUCCESS)
			{
				FindField(Str, Buf, 1, NO);
				Ret = NODE_FILE;
				if(strstr(Buf, "DIR") != NULL)
					Ret = NODE_DIR;
			}
			break;

		case LIST_CHAMELEON :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE | FINFO_ATTR);

			/* 属性 */
			FindField(Str, Buf, 6, NO);
			strcat(Buf, "------");
			*Attr = AttrString2Value(Buf+1);

			/* 日付 */
			FindField(Str, Buf, 2, NO);
			GetMonth(Buf, &sTime.wMonth, &sTime.wDay);	/* wDayは常に0 */
			FindField(Str, Buf, 3, NO);
			sTime.wDay = atoi(Buf);
			FindField(Str, Buf, 4, NO);
			sTime.wYear = atoi(Buf);

			/* 時刻 */
			FindField(Str, Buf, 5, NO);
			sTime.wHour = atoi(Buf);
			sTime.wMinute = atoi(Buf+3);
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 1, NO);
			*Size = _atoi64(Buf);

			/* 名前 */
			if(FindField(Str, Fname, 0, NO) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if(Buf[0] == '<')
					Ret = NODE_DIR;
			}
			break;

		case LIST_AS400 :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE);

			/* オーナ名 */
			FindField(Str, Buf, 0, NO);
			strncpy(Owner, Buf, OWNER_NAME_LEN);

			/* 時刻 */
			FindField(Str, Buf, 3, NO);
			sTime.wHour = atoi(Buf);
			sTime.wMinute = atoi(Buf+3);
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;

			/* 日付 */
			FindField(Str, Buf, 2, NO);
			sTime.wYear = Assume1900or2000(atoi(Buf));
			sTime.wMonth = atoi(Buf + 3);
			sTime.wDay = atoi(Buf + 6);
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 1, NO);
			*Size = _atoi64(Buf);

			/* 名前 */
			if(FindField(Str, Fname, 5, YES) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if((Pos = strchr(Fname, '/')) != NULL)
				{
					Ret = NODE_DIR;
					*Pos = NUL;
				}
			}
			break;

		case LIST_M1800 :
			*InfoExist |= FINFO_ATTR;

			/* 属性 */
			FindField(Str, Buf, 0, NO);
			strcat(Buf, "------");
			*Attr = AttrString2Value(Buf+1);

			/* 日付 */
			Time->dwLowDateTime = 0;
			Time->dwHighDateTime = 0;
			FindField(Str, Buf, 5, NO);
			if(Buf[0] != '*')
			{
				*InfoExist |= FINFO_DATE;
				sTime.wHour = 0;
				sTime.wMinute = 0;
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;

				sTime.wYear = Assume1900or2000(atoi(Buf));
				sTime.wMonth = atoi(Buf + 3);
				sTime.wDay = atoi(Buf + 6);
				SystemTimeToFileTime(&sTime, Time);
				SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());
			}

			/* 名前 */
			if(FindField(Str, Fname, 6, YES) == FFFTP_SUCCESS)
			{
				RemoveTailingSpaces(Fname);
				Ret = NODE_FILE;
				if((Pos = strchr(Fname, '/')) != NULL)
				{
					Ret = NODE_DIR;
					*Pos = NUL;
				}
			}
			break;

		case LIST_GP6000 :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE | FINFO_ATTR);

			/* オーナ名 */
			FindField(Str, Buf, 3, NO);
			strncpy(Owner, Buf, OWNER_NAME_LEN);

			/* 時刻 */
			FindField(Str, Buf, 2, NO);
			sTime.wHour = atoi(Buf);
			sTime.wMinute = atoi(Buf+3);
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;

			/* 日付 */
			FindField(Str, Buf, 1, NO);
			sTime.wYear = Assume1900or2000(atoi(Buf));
			sTime.wMonth = atoi(Buf + 3);
			sTime.wDay = atoi(Buf + 6);
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 5, NO);
			*Size = _atoi64(Buf);

			/* 属性 */
			FindField(Str, Buf, 0, NO);
			*Attr = AttrString2Value(Buf+1);

			/* 名前 */
			if(FindField(Str, Fname, 6, YES) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if(strchr("dl", Buf[0]) != NULL)
					Ret = NODE_DIR;
			}
			break;

		case LIST_ACOS :
		case LIST_ACOS_4 :
			/* 名前 */
			FindField(Str, Fname, 0, NO);
			Ret = NODE_FILE;
			break;

		case LIST_VMS :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE);

			/* サイズ */
			FindField(Str, Buf, 1, NO);
			*Size = _atoi64(Buf) * BLOCK_SIZE;

			/* 時刻／日付 */
			FindField(Str, Buf, 2, NO);
			GetVMSdate(Buf, &sTime.wYear, &sTime.wMonth, &sTime.wDay);

			FindField(Str, Buf, 3, NO);
			GetHourAndMinute(Buf, &sTime.wHour, &sTime.wMinute);

			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* 名前 */
			FindField(Str, Fname, 0, NO);

			Ret = NODE_FILE;
			if((Pos = strchr(Fname, '.')) != NULL)
			{
				if(_strnicmp(Pos, ".DIR;", 5) == 0)
				{
					/* OpenVMSの場合、ファイル/ディレクトリ削除時には".DIR;?"までないと
					 * 削除できないので、ここではつぶさない */
#if !defined(HAVE_OPENVMS)
					*Pos = NUL;
#endif
					Ret = NODE_DIR;
				}
			}
			break;

		case LIST_OS7_2 :
			*InfoExist |= FINFO_SIZE;
			offs = 2;

			/* サイズ */
			FindField(Str, Buf, 2, NO);
			*Size = _atoi64(Buf);
			/* ここにbreakはない */

		case LIST_OS7_1 :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_ATTR);

			/* 日付 */
			FindField(Str, Buf, 1+offs, NO);
			sTime.wYear = Assume1900or2000(atoi(Buf));
			sTime.wMonth = atoi(Buf + 3);
			sTime.wDay = atoi(Buf + 6);

			/* 時刻 */
			FindField(Str, Buf, 2+offs, NO);
			sTime.wHour = atoi(Buf);
			sTime.wMinute = atoi(Buf+3);
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* 属性 */
			FindField(Str, Buf, 0, NO);
			*Attr = AttrString2Value(Buf+1);

			/* 名前 */
			if(FindField(Str, Fname, 3+offs, YES) == FFFTP_SUCCESS)
			{
				RemoveTailingSpaces(Fname);
				Ret = NODE_FILE;
				if(strchr("dl", Buf[0]) != NULL)
					Ret = NODE_DIR;
			}
			break;

		case LIST_STRATUS :
			if(FindField(Str, Buf, 0, NO) != FFFTP_SUCCESS)
				break;
			if(_strnicmp(Buf, "Files:", 6) == 0)
				StratusMode = 0;
			else if(_strnicmp(Buf, "Dirs:", 5) == 0)
				StratusMode = 1;
			else if(_strnicmp(Buf, "Links:", 6) == 0)
				StratusMode = 2;
			else
			{
				if(StratusMode == 0)
					offs = 1;
				else if(StratusMode == 1)
					offs = 0;
				else
					break;

				*InfoExist |= (FINFO_TIME | FINFO_DATE);

				/* 日付 */
				if(FindField(Str, Buf, 2+offs, NO) != FFFTP_SUCCESS)
					break;
				sTime.wYear = Assume1900or2000(atoi(Buf));
				sTime.wMonth = atoi(Buf + 3);
				sTime.wDay = atoi(Buf + 6);

				/* 時刻 */
				if(FindField(Str, Buf, 3+offs, NO) != FFFTP_SUCCESS)
					break;
				sTime.wHour = atoi(Buf);
				sTime.wMinute = atoi(Buf+3);
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;
				SystemTimeToFileTime(&sTime, Time);
				SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

				/* 名前 */
				if(FindField(Str, Fname, 4+offs, YES) != FFFTP_SUCCESS)
					break;

				if(StratusMode == 0)
				{
					*InfoExist |= FINFO_SIZE;

					/* サイズ */
					if(FindField(Str, Buf, 1, NO) != FFFTP_SUCCESS)
						break;
					*Size = _atoi64(Buf) * 4096;

					/* 種類（オーナ名のフィールドにいれる） */
					if(FindField(Str, Buf, 2, NO) != FFFTP_SUCCESS)
						break;
					strncpy(Owner, Buf, OWNER_NAME_LEN);

					Ret = NODE_FILE;
				}
				else
					Ret = NODE_DIR;
			}
			break;

		case LIST_IRMX :
			*InfoExist |= (FINFO_DATE | FINFO_SIZE);

			/* 日付 */
			for(offs = 11; offs > 7; offs--)
			{
				if((err = FindField(Str, Buf, offs, NO)) == FFFTP_SUCCESS)
					break;
			}
			if(err != FFFTP_SUCCESS)
				break;
			if(IsDigit(*Buf) == 0)
				break;
			sTime.wYear = Assume1900or2000(atoi(Buf));
			if(FindField(Str, Buf, --offs, NO) != FFFTP_SUCCESS)
				break;
			GetMonth(Buf, &sTime.wMonth, &sTime.wDay);
			if(FindField(Str, Buf, --offs, NO) != FFFTP_SUCCESS)
				break;
			if(IsDigit(*Buf) == 0)
				break;
			sTime.wDay = atoi(Buf);
			sTime.wHour = 0;
			sTime.wMinute = 0;
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* オーナ名 */
			if(FindField(Str, Buf, --offs, NO) != FFFTP_SUCCESS)
				break;
			strncpy(Owner, Buf, OWNER_NAME_LEN);

			/* サイズ */
			do
			{
				if((err = FindField(Str, Buf, --offs, NO)) != FFFTP_SUCCESS)
					break;
			}
			while(IsDigit(*Buf) == 0);
			--offs;
			if((err = FindField(Str, Buf, --offs, NO)) != FFFTP_SUCCESS)
				break;
			RemoveComma(Buf);
			*Size = _atoi64(Buf);
			if((err = FindField(Str, Buf, --offs, NO)) != FFFTP_SUCCESS)
				break;
			if(IsDigit(*Buf) == 0)
				break;
			/* 名前 */
			if(FindField(Str, Fname, 0, NO) != FFFTP_SUCCESS)
				break;
			/* 種類 */
			if(offs == 0)
				Ret = NODE_FILE;
			else
			{
				if((FindField(Str, Buf, 1, NO) == FFFTP_SUCCESS) &&
				   (strcmp(Buf, "DR") == 0))
					Ret = NODE_DIR;
				else
					Ret = NODE_FILE;
			}
			break;

		case LIST_ALLIED :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE);

			/* 日付 */
			FindField(Str, Buf, 3, NO);
			GetMonth(Buf, &sTime.wMonth, &sTime.wDay);	/* wDayは常に0 */
			FindField(Str, Buf, 4, NO);
			sTime.wDay = atoi(Buf);
			FindField(Str, Buf, 6, NO);
			sTime.wYear = atoi(Buf);

			/* 時刻 */
			FindField(Str, Buf, 5, NO);
			sTime.wHour = atoi(Buf);
			sTime.wMinute = atoi(Buf+3);
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 0, NO);
			*Size = _atoi64(Buf);

			/* 名前 */
			if(FindField(Str, Fname, 1, NO) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if(Buf[0] == '<')
					Ret = NODE_DIR;
			}
			break;

		case LIST_OS9 :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE);

			/* 日付 */
			FindField(Str, Buf, 1, NO);
			sTime.wYear = Assume1900or2000(atoi(Buf));
			sTime.wMonth = atoi(Buf + 3);
			sTime.wDay = atoi(Buf + 6);
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* 時刻 */
			FindField(Str, Buf, 2, NO);
			sTime.wHour = atoi_n(Buf, 2);
			sTime.wMinute = atoi(Buf+2);
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* サイズ */
			FindField(Str, Buf, 5, NO);
			*Size = _atoi64(Buf);

			/* オーナ名 */
			FindField(Str, Buf, 0, NO);
			strncpy(Owner, Buf, OWNER_NAME_LEN);

			/* オーナ名 */
			FindField(Str, Buf, 3, NO);

			/* 名前 */
			if(FindField(Str, Fname, 6, NO) == FFFTP_SUCCESS)
			{
				if((Buf[0] == 'd') || (Buf[0] == 'D'))
					Ret = NODE_DIR;
				else
					Ret = NODE_FILE;
			}
			break;

		case LIST_IBM :
			*InfoExist |= FINFO_DATE;


			/* 日付 */
			FindField(Str, Buf, 2, NO);
			sTime.wYear = atoi(Buf);
			sTime.wMonth = atoi(Buf + 5);
			sTime.wDay = atoi(Buf + 8);
			sTime.wHour = 0;
			sTime.wMinute = 0;
			sTime.wSecond = 0;
			sTime.wMilliseconds = 0;
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* 名前 */
			FindField(Str, Buf, 8, NO);
			if(FindField(Str, Fname, 9, NO) == FFFTP_SUCCESS)
			{
				if(strcmp(Buf, "PO") == 0)
					Ret = NODE_DIR;
				else if(strcmp(Buf, "PS") == 0)
					Ret = NODE_FILE;
			}
			break;

		case LIST_AGILENT :
			*InfoExist |= (FINFO_SIZE | FINFO_ATTR);

			/* オーナ名 */
			FindField(Str, Buf, 2, NO);
			strncpy(Owner, Buf, OWNER_NAME_LEN);

			/* サイズ */
			FindField(Str, Buf, 4, NO);
			*Size = _atoi64(Buf);

			/* 属性 */
			FindField(Str, Buf, 0, NO);
			*Attr = AttrString2Value(Buf+1);

			/* 名前 */
			if(FindField(Str, Fname, 5, YES) == FFFTP_SUCCESS)
			{
				Ret = NODE_FILE;
				if(strchr("dl", Buf[0]) != NULL)
					Ret = NODE_DIR;
			}
			break;

		case LIST_SHIBASOKU :
			*InfoExist |= (FINFO_TIME | FINFO_DATE | FINFO_SIZE);

			/* サイズ */
			FindField(Str, Buf, 0, NO);
			if(IsDigit(Buf[0]))
			{
				*Size = _atoi64(Buf);

				/* 日付 */
				FindField(Str, Buf, 1, NO);
				Buf[3] = '\0';
				GetMonth(Buf, &sTime.wMonth, &sTime.wDay);
				sTime.wDay = atoi(Buf+4);
				sTime.wYear = atoi(Buf+7);

				/* 時刻 */
				FindField(Str, Buf, 2, NO);
				sTime.wHour = atoi(Buf);
				sTime.wMinute = atoi(Buf+3);
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;
				SystemTimeToFileTime(&sTime, Time);
				SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

				/* 名前 */
				FindField(Str, Fname, 3, NO);

				/* 種類 */
				Ret = NODE_FILE;
				if(FindField(Str, Buf, 4, NO) == FFFTP_SUCCESS)
				{
					if(strcmp(Buf, "<DIR>") == 0)
						Ret = NODE_DIR;
				}
			}
			break;

		case LIST_UNIX_10 :
		case LIST_UNIX_11 :
		case LIST_UNIX_12 :
		case LIST_UNIX_13 :
		case LIST_UNIX_14 :
		case LIST_UNIX_15 :
		case LIST_UNIX_20 :
		case LIST_UNIX_21 :
		case LIST_UNIX_22 :
		case LIST_UNIX_23 :
		case LIST_UNIX_24 :
		case LIST_UNIX_25 :
		case LIST_UNIX_50 :
		case LIST_UNIX_51 :
		case LIST_UNIX_54 :
		case LIST_UNIX_60 :
		case LIST_UNIX_61 :
		case LIST_UNIX_62 :
		case LIST_UNIX_63 :
		case LIST_UNIX_64 :
		case LIST_UNIX_65 :
		case LIST_UNIX_70 :
		case LIST_UNIX_71 :
		case LIST_UNIX_72 :
		case LIST_UNIX_73 :
		case LIST_UNIX_74 :
		case LIST_UNIX_75 :
// MELCOMはビットフラグになっている
//		case LIST_MELCOM :
		default:
			/* offsはサイズの位置, offs=0はカラム4 */
			offs = 0;
			if((ListType == LIST_UNIX_12) ||
			   (ListType == LIST_UNIX_13) ||
			   (ListType == LIST_UNIX_15) ||
			   (ListType == LIST_UNIX_20) ||
			   (ListType == LIST_UNIX_21) ||
			   (ListType == LIST_UNIX_24))
				offs = -1;

			if((ListType == LIST_UNIX_22) ||
			   (ListType == LIST_UNIX_23) ||
			   (ListType == LIST_UNIX_25) ||
			   (ListType == LIST_UNIX_50) ||
			   (ListType == LIST_UNIX_51) ||
			   (ListType == LIST_UNIX_54))
				offs = -2;

			if((ListType == LIST_UNIX_60) ||
			   (ListType == LIST_UNIX_61) ||
			   (ListType == LIST_UNIX_64))
				offs = 2;

			if((ListType == LIST_UNIX_62) ||
			   (ListType == LIST_UNIX_63) ||
			   (ListType == LIST_UNIX_65) ||
			   (ListType == LIST_UNIX_70) ||
			   (ListType == LIST_UNIX_71) ||
			   (ListType == LIST_UNIX_74))
				offs = 1;

			/* offs2は時間(もしくは年)の位置 */
			offs2 = 0;
			if((ListType == LIST_UNIX_11) ||
			   (ListType == LIST_UNIX_13) ||
			   (ListType == LIST_UNIX_21) ||
			   (ListType == LIST_UNIX_23) ||
			   (ListType == LIST_UNIX_51) ||
			   (ListType == LIST_UNIX_61) ||
			   (ListType == LIST_UNIX_63) ||
			   (ListType == LIST_UNIX_71) ||
			   (ListType == LIST_UNIX_73))
				offs2 = -1;

			/* offs3はオーナ名の位置 */
			offs3 = 0;
			if((ListType == LIST_UNIX_12) ||
			   (ListType == LIST_UNIX_13) ||
			   (ListType == LIST_UNIX_15) ||
			   (ListType == LIST_UNIX_22) ||
			   (ListType == LIST_UNIX_23) ||
			   (ListType == LIST_UNIX_25) ||
			   (ListType == LIST_UNIX_50) ||
			   (ListType == LIST_UNIX_51) ||
			   (ListType == LIST_UNIX_62) ||
			   (ListType == LIST_UNIX_63) ||
			   (ListType == LIST_UNIX_65) ||
			   (ListType == LIST_UNIX_72) ||
			   (ListType == LIST_UNIX_73) ||
			   (ListType == LIST_UNIX_75))
				offs3 = -1;

			Flag2 = 0;
			if((ListType == LIST_UNIX_14) ||
			   (ListType == LIST_UNIX_15) ||
			   (ListType == LIST_UNIX_24) ||
			   (ListType == LIST_UNIX_25) ||
			   (ListType == LIST_UNIX_54) ||
			   (ListType == LIST_UNIX_64) ||
			   (ListType == LIST_UNIX_65) ||
			   (ListType == LIST_UNIX_74) ||
			   (ListType == LIST_UNIX_75))
				Flag2 = 1;

			*InfoExist |= (FINFO_DATE | FINFO_SIZE | FINFO_ATTR);

			/* 属性 */
			FindField(Str, Buf, 0, NO);
			*Attr = AttrString2Value(Buf+1);

			/* オーナ名 */
			FindField(Str, Buf, 2+offs3, NO);
			strncpy(Owner, Buf, OWNER_NAME_LEN);

			/* サイズ */
			FindField(Str, Buf, 4+offs, NO);
			Pos = Buf;
			if((*Pos != NUL) && (IsDigit(*Pos) == 0))
			{
				Pos = strchr(Pos, NUL) - 1;
				for(; Pos > Buf; Pos--)
				{
					if(IsDigit(*Pos) == 0)
					{
						Pos++;
						break;
					}
				}
			}
			*Size = _atoi64(Pos);

			if(Flag2 == 0)
			{
				/* 時刻／日付 */
				GetLocalTime(&sTime);
				memcpy(&sTimeNow, &sTime, sizeof(SYSTEMTIME));
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;

				FindField(Str, Buf, 5+offs, NO);
				/* 日付が yy/mm/dd の場合に対応 */
				if(GetYearMonthDay(Buf, &sTime.wYear, &sTime.wMonth, &sTime.wDay) == FFFTP_SUCCESS)
				{
					sTime.wYear = Assume1900or2000(sTime.wYear);

					FindField(Str, Buf, 7+offs+offs2, NO);
					if(GetHourAndMinute(Buf, &sTime.wHour, &sTime.wMinute) == FFFTP_SUCCESS)
						*InfoExist |= FINFO_TIME;
				}
				else
				{
					GetMonth(Buf, &sTime.wMonth, &sTime.wDay);
					if(offs2 == 0)
					{
						FindField(Str, Buf, 6+offs, NO);
						sTime.wDay = atoi(Buf);
					}

					FindField(Str, Buf, 7+offs+offs2, NO);
					if(GetHourAndMinute(Buf, &sTime.wHour, &sTime.wMinute) == FFFTP_FAIL)
					{
						sTime.wYear = atoi(Buf);
					}
					else
					{
						*InfoExist |= FINFO_TIME;

						/* 年がない */
						/* 現在の日付から推定 */
						// 恐らくホストとローカルの時刻が異なる場合の対処のようだがとりあえず無効にする
//						if((sTimeNow.wMonth == 12) && (sTime.wMonth == 1))
//							sTime.wYear++;
//						else if(sTimeNow.wMonth+1 == sTime.wMonth)
						if(sTimeNow.wMonth+1 == sTime.wMonth)
							/* nothing */;
						else if(sTimeNow.wMonth < sTime.wMonth)
							sTime.wYear--;


//#################
						/* 今年の今日以降のファイルは、実は去年のファイル */
						if((sTime.wYear == sTimeNow.wYear) &&
						   ((sTime.wMonth > sTimeNow.wMonth) ||
							((sTime.wMonth == sTimeNow.wMonth) && (sTime.wDay > sTimeNow.wDay))))
						{
							sTime.wYear--;
						}
					}
				}
			}
			else
			{
				/* LIST_UNIX_?4, LIST_UNIX_?5 の時 */
				FindField(Str, Buf, 5+offs, NO);
				sTime.wYear = atoi(Buf);
				FindField(Str, Buf, 6+offs, NO);
				sTime.wMonth = atoi(Buf);
				FindField(Str, Buf, 7+offs, NO);
				sTime.wDay = atoi(Buf);
				sTime.wHour = 0;
				sTime.wMinute = 0;
				sTime.wSecond = 0;
				sTime.wMilliseconds = 0;
			}
			SystemTimeToFileTime(&sTime, Time);
			SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());

			/* 名前 */
			if(FindField(Str, Fname, 8+offs+offs2, YES) == FFFTP_SUCCESS)
			{
				Flag = 'B';
				if(OrgListType & LIST_MELCOM)
				{
					Flag = Fname[14];
					Fname[14] = NUL;
					RemoveTailingSpaces(Fname);
				}
				else
				{
					if((Pos = strstr(Fname, " -> ")) != NULL)
						*Pos = NUL;
				}

				if(strchr("dl", *Str) != NULL)
				{
					// 0x5Cが含まれる文字列を扱えないバグ修正
//					if((_mbscmp(_mbsninc(Fname, _mbslen(Fname) - 1), "/") == 0) ||
//					   (_mbscmp(_mbsninc(Fname, _mbslen(Fname) - 1), "\\") == 0))
//					{
//						*(Fname + strlen(Fname) - 1) = NUL;
//					}
					Ret = NODE_DIR;
					if(*Str == 'l')
						*Link = YES;
				}
				else if(strchr("-+f", *Str) != NULL)
					Ret = NODE_FILE;

				if((Ret == NODE_FILE) && (Flag != 'B'))
					Ret = NODE_NONE;
			}
			break;

			// MLSD対応
		case LIST_MLSD:
			{
				int i = 0;
				char Tmp[FMAX_PATH + 1];
				char Name[FMAX_PATH + 1];
				char Value[FMAX_PATH + 1];
				while(FindField2(Str, Tmp, ';', i, NO) == FFFTP_SUCCESS)
				{
					if(i >= 1 && strncmp(Tmp, " ", 1) == 0)
						strcpy(Fname, strstr(Str, "; ") + 2);
					else if(FindField2(Tmp, Name, '=', 0, NO) == FFFTP_SUCCESS && FindField2(Tmp, Value, '=', 1, NO) == FFFTP_SUCCESS)
					{
						if(_stricmp(Name, "type") == 0)
						{
							if(_stricmp(Value, "dir") == 0)
								Ret = NODE_DIR;
							else if(_stricmp(Value, "file") == 0)
								Ret = NODE_FILE;
						}
						else if(_stricmp(Name, "size") == 0)
						{
							*Size = _atoi64(Value);
							*InfoExist |= FINFO_SIZE;
						}
						else if(_stricmp(Name, "modify") == 0)
						{
							sTime.wYear = atoi_n(Value, 4);
							sTime.wMonth = atoi_n(Value + 4, 2);
							sTime.wDay = atoi_n(Value + 6, 2);
							sTime.wHour = atoi_n(Value + 8, 2);
							sTime.wMinute = atoi_n(Value + 10, 2);
							sTime.wSecond = atoi_n(Value + 12, 2);
							SystemTimeToFileTime(&sTime, Time);
//							SpecificLocalFileTime2FileTime(Time, AskHostTimeZone());
							*InfoExist |= FINFO_DATE | FINFO_TIME;
						}
						else if(_stricmp(Name, "UNIX.mode") == 0)
						{
							*Attr = strtol(Value, NULL, 16);
							*InfoExist |= FINFO_ATTR;
						}
						else if(_stricmp(Name, "UNIX.owner") == 0)
							strcpy(Owner, Value);
					}
					i++;
				}
			}
			break;
	}

	// UTF-8対応
//	if((Ret != NODE_NONE) && (strlen(Fname) > 0))
	if(!(OrgListType & LIST_RAW_NAME) && (Ret != NODE_NONE) && (strlen(Fname) > 0))
	{
		// UTF-8対応
//		if(CheckSpecialDirName(Fname) == YES)
//			Ret = NODE_NONE;
//		else
//			ChangeFnameRemote2Local(Fname, FMAX_PATH);
		ChangeFnameRemote2Local(Fname, FMAX_PATH);
		// UTF-8の冗長表現によるディレクトリトラバーサル対策
		FixStringM(Fname, Fname);
		// 0x5Cが含まれる文字列を扱えないバグ修正
		if((_mbscmp(_mbsninc(Fname, _mbslen(Fname) - 1), "/") == 0)
			|| (_mbscmp(_mbsninc(Fname, _mbslen(Fname) - 1), "\\") == 0))
			*(Fname + strlen(Fname) - 1) = NUL;
		if(CheckSpecialDirName(Fname) == YES)
			Ret = NODE_NONE;
		// 文字コードが正しくないために長さが0になったファイル名は表示しない
		if(strlen(Fname) == 0)
			Ret = NODE_NONE;
	}
	return(Ret);
}


/*----- 指定の番号のフィールドを求める ----------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char *Buf : 文字列のコピー先
*		int Num : フィールド番号
*		int ToLast : 文字列の最後までコピー (YES/NO)
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int FindField(char *Str, char *Buf, int Num, int ToLast)
{
	char *Pos;
	int Sts;

	Sts = FFFTP_FAIL;
	*Buf = NUL;
	if(Num >= 0)
	{
		while(*Str == ' ')
			Str++;

		for(; Num > 0; Num--)
		{
			if((Str = strchr(Str, ' ')) != NULL)
			{
				while(*Str == ' ')
				{
					if(*Str == NUL)
					{
						Str = NULL;
						break;
					}
					Str++;
				}
			}
			else
				break;
		}
	}

	if(Str != NULL)
	{
		if((ToLast == YES) || ((Pos = strchr(Str, ' ')) == NULL))
			strcpy(Buf, Str);
		else
		{
			strncpy(Buf, Str, Pos - Str);
			*(Buf + (Pos - Str)) = NUL;
		}
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


// MLSD対応
static int FindField2(char *Str, char *Buf, char Separator, int Num, int ToLast)
{
	char *Pos;
	int Sts;

	Sts = FFFTP_FAIL;
	*Buf = NUL;
	if(Num >= 0)
	{
		while(*Str == Separator)
			Str++;

		for(; Num > 0; Num--)
		{
			if((Str = strchr(Str, Separator)) != NULL)
			{
				while(*Str == Separator)
				{
					if(*Str == NUL)
					{
						Str = NULL;
						break;
					}
					Str++;
				}
			}
			else
				break;
		}
	}

	if(Str != NULL)
	{
		if((ToLast == YES) || ((Pos = strchr(Str, Separator)) == NULL))
			strcpy(Buf, Str);
		else
		{
			strncpy(Buf, Str, Pos - Str);
			*(Buf + (Pos - Str)) = NUL;
		}
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 文字列から月を求める --------------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		WORD *Month : 月 (0=月を表す文字列ではない)
*		WORD *Day : 日 (0=日は含まれていない)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void GetMonth(char *Str, WORD *Month, WORD *Day)
{
	static const char DateStr[] = { "JanFebMarAprMayJunJulAugSepOctNovDec" };
	char *Pos;

	*Month = 0;
	*Day = 0;

	if(IsDigit(*Str) == 0)
	{
		_strlwr(Str);
		*Str = toupper(*Str);
		if((Pos = strstr(DateStr, Str)) != NULL)
			*Month = ((Pos - DateStr) / 3) + 1;
	}
	else
	{
		/* 「10月」のような日付を返すものがある */
		/* 漢字がJISの時のみSJISに変換 */
		ConvAutoToSJIS(Str, KANJI_NOCNV);

		Pos = Str;
		while(*Pos != NUL)
		{
			if(!IsDigit(*Pos))
			{
				// UTF-8対応
//				if((_mbsncmp(Pos, "月", 1) == 0) ||
//				   (memcmp(Pos, "\xB7\xEE", 2) == 0) ||	/* EUCの「月」 */
//				   (memcmp(Pos, "\xD4\xC2", 2) == 0))	/* GBコードの「月」 */
				if(memcmp(Pos, "\xE6\x9C\x88", 3) == 0 || memcmp(Pos, "\x8C\x8E", 2) == 0 || memcmp(Pos, "\xB7\xEE", 2) == 0 || memcmp(Pos, "\xD4\xC2", 2) == 0)
				{
					Pos += 2;
					*Month = atoi(Str);
					if((*Month < 1) || (*Month > 12))
						*Month = 0;
					else
					{
						/* 「10月11日」のように日がくっついている事がある */
						if(*Pos != NUL)
						{
							*Day = atoi(Pos);
							if((*Day < 1) || (*Day > 31))
								*Day = 0;
						}
					}
				}
				else if(_mbsncmp(Pos, "/", 1) == 0)
				{
					/* 「10/」のような日付を返すものがある */
					Pos += 1;
					*Month = atoi(Str);
					if((*Month < 1) || (*Month > 12))
						*Month = 0;
					else
					{
						/* 「10/11」のように日がくっついている事がある */
						if(*Pos != NUL)
						{
							*Day = atoi(Pos);
							if((*Day < 1) || (*Day > 31))
								*Day = 0;
						}
					}
				}
				break;
			}
			Pos++;
		}
	}
	return;
}


/*----- 文字列から年月日を求める ----------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		WORD *Year : 年
*		WORD *Month : 月
*		WORD *Day : 日
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL=日付を表す文字ではない)
*
*	Note
*		以下の形式をサポート
*			01/07/25
*		FFFTP_FAILを返す時は *Year = 0; *Month = 0; *Day = 0
*----------------------------------------------------------------------------*/
static int GetYearMonthDay(char *Str, WORD *Year, WORD *Month, WORD *Day)
{
	int Sts;

	Sts = FFFTP_FAIL;
	if(strlen(Str) == 8)
	{
		if(IsDigit(Str[0]) && IsDigit(Str[1]) && !IsDigit(Str[2]) &&
		   IsDigit(Str[3]) && IsDigit(Str[4]) && !IsDigit(Str[5]) &&
		   IsDigit(Str[6]) && IsDigit(Str[7]))
		{
			*Year = atoi(&Str[0]);
			*Month = atoi(&Str[3]);
			*Day = atoi(&Str[6]);
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- 文字列から時刻を取り出す ----------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		WORD *Hour : 時
*		WORD *Minute : 分
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL=時刻を表す文字ではない)
*
*	Note
*		以下の形式をサポート
*			HH:MM
*			HH時MM分
*		FFFTP_FAILを返す時は *Hour = 0; *Minute = 0
*----------------------------------------------------------------------------*/

static int GetHourAndMinute(char *Str, WORD *Hour, WORD *Minute)
{
	int Ret;
	char *Pos;

	Ret = FFFTP_FAIL;
	if((_mbslen(Str) >= 3) && (isdigit(Str[0]) != 0))
	{
		*Hour = atoi(Str);
		if(*Hour <= 24)
		{
			if((Pos = _mbschr(Str, ':')) != NULL)
			{
				Pos++;
				if(IsDigit(*Pos) != 0)
				{
					*Minute = atoi(Pos);
					if(*Minute < 60)
						Ret = FFFTP_SUCCESS;
				}
			}
			else
			{
				/* 漢字がJISの時のみSJISに変換 */
				ConvAutoToSJIS(Str, KANJI_NOCNV);

				Pos = Str;
				while(*Pos != NUL)
				{
					if(IsDigit(*Pos) == 0)
					{
						// UTF-8対応
//						if((_mbsncmp(Pos, "時", 1) == 0) ||
//						   (memcmp(Pos, "\xBB\xFE", 2) == 0))	/* EUCの「時」 */
						if(memcmp(Pos, "\xE6\x99\x82", 3) == 0 || memcmp(Pos, "\x8E\x9E", 2) == 0 || memcmp(Pos, "\xBB\xFE", 2) == 0)
						{
							Pos += 2;
							if(*Pos != NUL)
							{
								*Minute = atoi(Pos);
								if(*Minute < 60)
									Ret = FFFTP_SUCCESS;
							}
						}
						break;
					}
					Pos++;
				}
			}
		}
	}
	else if((_stricmp(Str, "a:m") == 0) || (_stricmp(Str, "p:m") == 0))
	{
		*Hour = 0;
		*Minute = 0;
		Ret = FFFTP_SUCCESS;
	}

	if(Ret == FFFTP_FAIL)
	{
		*Hour = 0;
		*Minute = 0;
	}
	return(Ret);
}


/*----- VAX VMSの日付文字列から日付を取り出す ---------------------------------
*
*	Parameter
*		char *Str : 文字列
*		WORD *Year : 年
*		WORD *Month : 月
*		WORD *Day : 日
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL=日付を表す文字ではない)
*
*	Note
*		以下の形式をサポート
*			18-SEP-1998
*		FFFTP_FAILを返す時は *Year = 0; *Month = 0; *Day = 0
*----------------------------------------------------------------------------*/

static int GetVMSdate(char *Str, WORD *Year, WORD *Month, WORD *Day)
{
	char *Pos;
	int Ret;
	WORD Tmp;
	char Buf[4];

	Ret = FFFTP_FAIL;
	*Day = atoi(Str);
	if((Pos = strchr(Str, '-')) != NULL)
	{
		Pos++;
		strncpy(Buf, Pos, 3);
		Buf[3] = NUL;
		GetMonth(Buf, Month, &Tmp);
		if((Pos = strchr(Pos, '-')) != NULL)
		{
			Pos++;
			*Year = atoi(Pos);
			Ret = FFFTP_SUCCESS;
		}
	}

	if(Ret == FFFTP_FAIL)
	{
		*Year = 0;
		*Month = 0;
		*Day = 0;
	}
	return(Ret);
}


/*----- 1900年代か2000年代かを決める ------------------------------------------
*
*	Parameter
*		int Year : 年（２桁）
*
*	Return Value
*		int 年
*----------------------------------------------------------------------------*/

int Assume1900or2000(int Year)
{
	if(Year >= 60)
		Year += 1900;
	else
		Year += 2000;
	return(Year);
}



/*----- "."や".."かどうかを返す -----------------------------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス (YES="."か".."のどちらか/NO)
*----------------------------------------------------------------------------*/

static int CheckSpecialDirName(char *Fname)
{
	int Sts;

	Sts = NO;
	if((strcmp(Fname, ".") == 0) || (strcmp(Fname, "..") == 0))
		Sts = YES;

	return(Sts);
}


/*----- フィルタに指定されたファイル名かどうかを返す --------------------------
*
*	Parameter
*		char Fname : ファイル名
*		int Type : ファイルのタイプ (NODE_xxx)
*
*	Return Value
*		int ステータス
*			YES/NO=表示しない
*
*	Note
*		フィルタ文字列は以下の形式
*			*.txt;*.log
*----------------------------------------------------------------------------*/

static int AskFilterStr(char *Fname, int Type)
{
	int Ret;
	char *Tbl;
	char *Pos;
	char Tmp[FILTER_EXT_LEN+1];

	Tbl = FilterStr;
	Ret = YES;
	if((strlen(Tbl) > 0) && (Type == NODE_FILE))
	{
		Ret = NO;
		while((Tbl != NULL) && (*Tbl != NUL))
		{
			while(*Tbl == ';')
				Tbl++;
			if(*Tbl == NUL)
				break;

			strcpy(Tmp, Tbl);
			if((Pos = strchr(Tmp, ';')) != NULL)
				*Pos = NUL;

			if(CheckFname(Fname, Tmp) == FFFTP_SUCCESS)
			{
				Ret = YES;
				break;
			}

			Tbl = strchr(Tbl, ';');
		}
	}
	return(Ret);
}


/*----- フィルタを設定する ----------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetFilter(int *CancelCheckWork)
{
 	if(DialogBox(GetFtpInst(), MAKEINTRESOURCE(filter_dlg), GetMainHwnd(), FilterWndProc) == YES)
	{
		DispWindowTitle();
		GetLocalDirForWnd();
		GetRemoteDirForWnd(CACHE_LASTREAD, CancelCheckWork);
	}
	return;
}


/*----- フィルタ入力ダイアログのコールバック ----------------------------------
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
//static BOOL CALLBACK FilterWndProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK FilterWndProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, FILTER_STR, EM_LIMITTEXT, FILTER_EXT_LEN+1, 0);
			SendDlgItemMessage(hDlg, FILTER_STR, WM_SETTEXT, 0, (LPARAM)FilterStr);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, FILTER_STR, WM_GETTEXT, FILTER_EXT_LEN, (LPARAM)FilterStr);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case FILTER_NOR :
					strcpy(FilterStr, "*");
					EndDialog(hDlg, YES);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000021);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}





static int atoi_n(const char *Str, int Len)
{
	char *Tmp;
	int Ret;

	Ret = 0;
	if((Tmp = malloc(Len+1)) != NULL)
	{
		memset(Tmp, 0, Len+1);
		strncpy(Tmp, Str, Len);
		Ret = atoi(Tmp);
		free(Tmp);
	}
	return(Ret);
}




// UTF-8対応
// ファイル一覧から漢字コードを推測
// 優先度はUTF-8、Shift_JIS、EUC、JISの順
int AnalyzeNameKanjiCode(int Num)
{
	char Str[FMAX_PATH+1];
	char Name[FMAX_PATH+1];
	LONGLONG Size;
	FILETIME Time;
	int Attr;
	FILE *fd;
	int Node;
	int ListType;
	char Owner[OWNER_NAME_LEN+1];
	int Link;
	int InfoExist;
	int NameKanjiCode;
	int Point;
	int PointSJIS;
	int PointJIS;
	int PointEUC;
	int PointUTF8N;
	char* p;

	NameKanjiCode = KANJI_AUTO;
	Point = 0;
	PointSJIS = 0;
	PointJIS = 0;
	PointEUC = 0;
	PointUTF8N = 0;
	MakeCacheFileName(Num, Str);
	if((fd = fopen(Str, "rb")) != NULL)
	{
		while(GetListOneLine(Str, FMAX_PATH, fd) == FFFTP_SUCCESS)
		{
			if((ListType = AnalizeFileInfo(Str)) != LIST_UNKNOWN)
			{
				strcpy(Name, "");
				Node = ResolvFileInfo(Str, ListType | LIST_RAW_NAME, Name, &Size, &Time, &Attr, Owner, &Link, &InfoExist);
				p = Name;
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
				{
					if(!CheckStringM(Name))
						PointUTF8N++;
					else
					{
						switch(CheckKanjiCode(Name, strlen(Name), KANJI_SJIS))
						{
						case KANJI_SJIS:
							PointSJIS++;
							break;
						case KANJI_JIS:
							PointJIS++;
							break;
						case KANJI_EUC:
							PointEUC++;
							break;
						}
					}
				}
			}
		}
		fclose(fd);
	}
	if(PointJIS >= Point)
	{
		NameKanjiCode = KANJI_JIS;
		Point = PointJIS;
	}
	if(PointEUC >= Point)
	{
		NameKanjiCode = KANJI_EUC;
		Point = PointEUC;
	}
	if(PointSJIS >= Point)
	{
		NameKanjiCode = KANJI_SJIS;
		Point = PointSJIS;
	}
	if(PointUTF8N >= Point)
	{
		NameKanjiCode = KANJI_UTF8N;
		Point = PointUTF8N;
	}
	return NameKanjiCode;
}

