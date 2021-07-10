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

#include "common.h"
#include <sys/stat.h>
#include "OleDragDrop.h"
#include "filelist.h"

#define BUF_SIZE		256
#define CF_CNT 2
#define WM_DRAGDROP		(WM_APP + 100)
#define WM_GETDATA		(WM_APP + 101)
#define WM_DRAGOVER		(WM_APP + 102)

/*===== プロトタイプ =====*/

static LRESULT CALLBACK LocalWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK RemoteWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT FileListCommonWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void DispFileList2View(HWND hWnd, std::vector<FILELIST>& files);
// ファイルアイコン表示対応
//static void AddListView(HWND hWnd, int Pos, char *Name, int Type, LONGLONG Size, FILETIME *Time, int Attr, char *Owner, int Link, int InfoExist);
static void AddListView(HWND hWnd, int Pos, char *Name, int Type, LONGLONG Size, FILETIME *Time, int Attr, char *Owner, int Link, int InfoExist, int ImageId);
static int GetImageIndex(int Win, int Pos);
static int MakeRemoteTree1(char *Path, char *Cur, std::vector<FILELIST>& Base, int *CancelCheckWork);
static int MakeRemoteTree2(char *Path, char *Cur, std::vector<FILELIST>& Base, int *CancelCheckWork);
static void CopyTmpListToFileList(std::vector<FILELIST>& Base, std::vector<FILELIST> const& List);
static std::optional<std::vector<std::variant<FILELIST, std::string>>> GetListLine(int Num);
static int MakeDirPath(char *Str, char *Path, char *Dir);
static bool MakeLocalTree(const char *Path, std::vector<FILELIST>& Base);
static void AddFileList(FILELIST const& Pkt, std::vector<FILELIST>& Base);
static int AskFilterStr(const char *Fname, int Type);

/*===== 外部参照 =====*/

extern int SepaWidth;
extern int RemoteWidth;
extern int ListHeight;
extern char FilterStr[FILTER_EXT_LEN+1];
// 外部アプリケーションへドロップ後にローカル側のファイル一覧に作業フォルダが表示されるバグ対策
extern int SuppressRefresh;
// ローカル側自動更新
extern HANDLE ChangeNotification;
// 特定の操作を行うと異常終了するバグ修正
extern int CancelFlg;

/* 設定値 */
extern int LocalWidth;
extern int LocalTabWidth[4];
extern int RemoteTabWidth[6];
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern HFONT ListFont;
extern int ListType;
extern int FindMode;
extern int DotFile;
extern int DispDrives;
extern int MoveMode;
// ファイルアイコン表示対応
extern int DispFileIcon;
// タイムスタンプのバグ修正
extern int DispTimeSeconds;
// ファイルの属性を数字で表示
extern int DispPermissionsNumber;
extern HOSTDATA CurHost;

/*===== ローカルなワーク =====*/

static HWND hWndListLocal = NULL;
static HWND hWndListRemote = NULL;

static WNDPROC LocalProcPtr;
static WNDPROC RemoteProcPtr;

static HIMAGELIST ListImg = NULL;
// ファイルアイコン表示対応
static HIMAGELIST ListImgFileIcon = NULL;

static char FindStr[40+1] = { "*" };		/* 検索文字列 */

static int Dragging = NO;
// 特定の操作を行うと異常終了するバグ修正
static POINT DropPoint;

static int StratusMode;			/* 0=ファイル, 1=ディレクトリ, 2=リンク */


// リモートファイルリスト (2007.9.3 yutaka)
static std::vector<FILELIST> remoteFileListBase;
static std::vector<FILELIST> remoteFileListBaseNoExpand;
static fs::path remoteFileDir;

template<class Fn>
static inline bool FindFile(fs::path const& fileName, Fn&& fn) {
	auto result = false;
	WIN32_FIND_DATAW data;
	if (auto handle = FindFirstFileW(fileName.c_str(), &data); handle != INVALID_HANDLE_VALUE) {
		result = true;
		do {
			if (DispIgnoreHide == YES && (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
				continue;
			if (std::wstring_view filename{ data.cFileName }; filename == L"."sv || filename == L".."sv)
				continue;
			result = fn(data);
		} while (result && FindNextFileW(handle, &data));
		FindClose(handle);
	} else {
		auto lastError = GetLastError();
		SetTaskMsg("FindFile failed (%s): 0x%08X, %s", fileName.u8string().c_str(), lastError, u8(GetErrorMessage(lastError)).c_str());
	}
	return result;
}

// ファイルリストウインドウを作成する
int MakeListWin()
{
	int Sts;

	/*===== ローカル側のリストビュー =====*/

	hWndListLocal = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, 0, AskToolWinHeight() * 2, LocalWidth, ListHeight, GetMainHwnd(), 0, GetFtpInst(), nullptr);

	if(hWndListLocal != NULL)
	{
		LocalProcPtr = (WNDPROC)SetWindowLongPtrW(hWndListLocal, GWLP_WNDPROC, (LONG_PTR)LocalWndProc);

		SendMessageW(hWndListLocal, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		if(ListFont != NULL)
			SendMessageW(hWndListLocal, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));

		ListImg = ImageList_LoadImageW(GetFtpInst(), MAKEINTRESOURCEW(dirattr_bmp), 16, 9, RGB(255,0,0), IMAGE_BITMAP, 0);
		SendMessageW(hWndListLocal, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImg);
		ShowWindow(hWndListLocal, SW_SHOW);

		constexpr std::tuple<int, int> columns[] = {
			{ LVCFMT_LEFT, IDS_MSGJPN038 },
			{ LVCFMT_LEFT, IDS_MSGJPN039 },
			{ LVCFMT_RIGHT, IDS_MSGJPN040 },
			{ LVCFMT_LEFT, IDS_MSGJPN041 },
		};
		int i = 0;
		for (auto [fmt, resourceId] : columns) {
			auto text = GetString(resourceId);
			LVCOLUMNW column{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, fmt, LocalTabWidth[i], data(text), 0, i };
			SendMessageW(hWndListLocal, LVM_INSERTCOLUMNW, i++, (LPARAM)&column);
		}
	}

	/*===== ホスト側のリストビュー =====*/

	hWndListRemote = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, nullptr, WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, LocalWidth + SepaWidth, AskToolWinHeight() * 2, RemoteWidth, ListHeight, GetMainHwnd(), 0, GetFtpInst(), nullptr);

	if(hWndListRemote != NULL)
	{
		RemoteProcPtr = (WNDPROC)SetWindowLongPtrW(hWndListRemote, GWLP_WNDPROC, (LONG_PTR)RemoteWndProc);

		SendMessageW(hWndListRemote, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		if(ListFont != NULL)
			SendMessageW(hWndListRemote, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));

		SendMessageW(hWndListRemote, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImg);
		ShowWindow(hWndListRemote, SW_SHOW);

		constexpr std::tuple<int, int> columns[] = {
			{ LVCFMT_LEFT, IDS_MSGJPN042 },
			{ LVCFMT_LEFT, IDS_MSGJPN043 },
			{ LVCFMT_RIGHT, IDS_MSGJPN044 },
			{ LVCFMT_LEFT, IDS_MSGJPN045 },
			{ LVCFMT_LEFT, IDS_MSGJPN046 },
			{ LVCFMT_LEFT, IDS_MSGJPN047 },
		};
		int i = 0;
		for (auto [fmt, resourceId] : columns) {
			auto text = GetString(resourceId);
			LVCOLUMNW column{ LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, fmt, RemoteTabWidth[i], data(text), 0, i };
			SendMessageW(hWndListRemote, LVM_INSERTCOLUMNW, i++, (LPARAM)&column);
		}
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


static void doTransferRemoteFile(void)
{
	// すでにリモートから転送済みなら何もしない。(2007.9.3 yutaka)
	if (!empty(remoteFileListBase))
		return;

	// 特定の操作を行うと異常終了するバグ修正
	while(1)
	{
		MSG msg;
		if(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else if(AskTransferNow() == NO)
			break;
		Sleep(10);
	}

	std::vector<FILELIST> FileListBase;
	MakeSelectedFileList(WIN_REMOTE, YES, NO, FileListBase, &CancelFlg);
	std::vector<FILELIST> FileListBaseNoExpand;
	MakeSelectedFileList(WIN_REMOTE, NO, NO, FileListBaseNoExpand, &CancelFlg);

	// set temporary folder
	auto LocDir = AskLocalCurDir();

	auto tmp = tempDirectory() / L"file";
	if (auto const created = !fs::create_directory(tmp); !created) {
		// 既存のファイルを削除する
		for (auto const& f : FileListBase)
			fs::remove(tmp / fs::u8path(f.File));
	}

	// 外部アプリケーションへドロップ後にローカル側のファイル一覧に作業フォルダが表示されるバグ対策
	SuppressRefresh = 1;

	// ダウンロード先をテンポラリに設定
	SetLocalDirHist(tmp);

	// FFFTPにダウンロード要求を出し、ダウンロードの完了を待つ。
	PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_DOWNLOAD, 0), 0);

	// 特定の操作を行うと異常終了するバグ修正
	while(1)
	{
		MSG msg;

		if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);

		} else {
			// 転送スレッドが動き出したら抜ける。
			if (AskTransferNow() == YES)
				break;
		}

		Sleep(10);
	}

	// 特定の操作を行うと異常終了するバグ修正
	while(1)
	{
		MSG msg;
		if(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else if(AskTransferNow() == NO)
			break;
		Sleep(10);
	}

	// ダウンロード先を元に戻す
	SetLocalDirHist(LocDir);
	SetCurrentDirAsDirHist();

	// 外部アプリケーションへドロップ後にローカル側のファイル一覧に作業フォルダが表示されるバグ対策
	SuppressRefresh = 0;
	GetLocalDirForWnd();

	remoteFileListBase = std::move(FileListBase);
	remoteFileListBaseNoExpand = std::move(FileListBaseNoExpand);
	remoteFileDir = std::move(tmp);
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
	if (std::error_code ec; !empty(remoteFileListBase)) {
		fs::remove_all(remoteFileDir, ec);
		remoteFileListBase.clear();
	}

	remoteFileListBaseNoExpand.clear();
}


// yutaka
// cf. http://www.nakka.com/lib/
/* ドロップファイルの作成 */
static HDROP CreateDropFileMem(std::vector<fs::path> const& filenames) {
	// 構造体の後ろに続くファイル名のリスト（ファイル名\0ファイル名\0ファイル名\0\0）
	std::wstring extra;
	for (auto const& filename : filenames) {
		extra += filename;
		extra += L'\0';
	}
	extra += L'\0';
	if (auto drop = (HDROP)GlobalAlloc(GHND, sizeof(DROPFILES) + size(extra) * sizeof(wchar_t))) {
		if (auto dropfiles = reinterpret_cast<DROPFILES*>(GlobalLock(drop))) {
			*dropfiles = { sizeof(DROPFILES), {}, false, true };
			std::copy(begin(extra), end(extra), reinterpret_cast<wchar_t*>(dropfiles + 1));
			GlobalUnlock(drop);
			return drop;
		}
		GlobalFree(drop);
	}
	return 0;
}


// OLE D&Dを開始する 
// (2007.8.30 yutaka)
static void doDragDrop(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT pt;

	// テンポラリをきれいにする (2007.9.3 yutaka)
	doDeleteRemoteFile();

	/* ドラッグ&ドロップの開始 */
	CLIPFORMAT clipFormat = CF_HDROP;
	OleDragDrop::DoDragDrop(hWnd, WM_GETDATA, WM_DRAGOVER, &clipFormat, 1, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK);

	// ドロップ先のアプリに WM_LBUTTONUP を飛ばす。
	// 特定の操作を行うと異常終了するバグ修正
//	GetCursorPos(&pt);
	pt = DropPoint;
	ScreenToClient(hWnd, &pt);
	PostMessageW(hWnd,WM_LBUTTONUP,0,MAKELPARAM(pt.x,pt.y));
	// ドロップ先が他プロセスかつカーソルが自プロセスのドロップ可能なウィンドウ上にある場合の対策
	EnableWindow(GetMainHwnd(), TRUE);
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
	// 特定の操作を行うと異常終了するバグ修正
	static int DragFirstTime = NO;

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
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

		case WM_KEYDOWN:
			if(wParam == 0x09)
			{
				SetFocus(hWndDst);
				break;
			}
			EraseListViewTips();
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

		case WM_SETFOCUS :
			SetFocusHwnd(hWnd);
			MakeButtonsFocus();
			DispCurrentWindow(Win);
			DispSelectedSpace();
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

		case WM_KILLFOCUS :
			EraseListViewTips();
			MakeButtonsFocus();
			DispCurrentWindow(-1);
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

		case WM_DROPFILES :
			if (!AskUserOpeDisabled())
				if (Dragging != YES) {		// ドラッグ中は処理しない。ドラッグ後にWM_LBUTTONDOWNが飛んでくるため、そこで処理する。
					if (hWnd == hWndListRemote) {
						if (AskConnecting() == YES)
							UploadDragProc(wParam);
					} else if (hWnd == hWndListLocal)
						ChangeDirDropFileProc(wParam);
				}
			DragFinish((HDROP)wParam);
			return 0;

		case WM_LBUTTONDOWN :
			if (AskUserOpeDisabled())
				break;
			if(Dragging == YES)
				break;
			DragFirstTime = NO;
			GetCursorPos(&DropPoint);
			EraseListViewTips();
			SetFocus(hWnd);
			DragPoint.x = LOWORD(lParam);
			DragPoint.y = HIWORD(lParam);
			hWndDragStart = hWnd;
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);
			break;

		case WM_LBUTTONUP :
			if (AskUserOpeDisabled())
				break;
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
						PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_UPLOAD, 0), 0);
					} else if(hWndPnt == hWndListLocal) {
						PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_DOWNLOAD, 0), 0);
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
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

		case WM_DRAGDROP:  
			// OLE D&Dを開始する (yutaka)
			// 特定の操作を行うと異常終了するバグ修正
//			doDragDrop(hWnd, message, wParam, lParam);
			if(DragFirstTime == NO)
				doDragDrop(hWnd, message, wParam, lParam);
			DragFirstTime = YES;
			return (TRUE);
			break;

		case WM_GETDATA:  // ファイルのパスをD&D先のアプリへ返す (yutaka)
			switch(wParam)
			{
			case CF_HDROP:		/* ファイル */
				{
					std::vector<FILELIST> FileListBase, FileListBaseNoExpand;
					fs::path PathDir;

					// 特定の操作を行うと異常終了するバグ修正
					GetCursorPos(&DropPoint);
					hWndPnt = WindowFromPoint(DropPoint);
					hWndParent = GetParent(hWndPnt);
					DisableUserOpe();
					CancelFlg = NO;

					// ローカル側で選ばれているファイルをFileListBaseに登録
					if (hWndDragStart == hWndListLocal) {
						PathDir = AskLocalCurDir();

						if(hWndPnt != hWndListRemote && hWndPnt != hWndListLocal && hWndParent != hWndListRemote && hWndParent != hWndListLocal)
							MakeSelectedFileList(WIN_LOCAL, NO, NO, FileListBase, &CancelFlg);			
						FileListBaseNoExpand = FileListBase;

					} else if (hWndDragStart == hWndListRemote) {
						if (hWndPnt == hWndListRemote || hWndPnt == hWndListLocal || hWndParent == hWndListRemote || hWndParent == hWndListLocal) {
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

					auto const& pf =
#if defined(HAVE_TANDEM)
						empty(FileListBaseNoExpand) ? FileListBase :
#endif
						FileListBaseNoExpand;
					// 特定の操作を行うと異常終了するバグ修正
					if (!empty(pf)) {
						Dragging = NO;
						ReleaseCapture();
						hCsrDrg = LoadCursor(NULL, IDC_ARROW);
						SetCursor(hCsrDrg);
						// ドロップ先が他プロセスかつカーソルが自プロセスのドロップ可能なウィンドウ上にある場合の対策
						EnableWindow(GetMainHwnd(), FALSE);
					}
					EnableUserOpe();

					if (empty(pf)) {
						// ファイルが未選択の場合は何もしない。(yutaka)
						*(HANDLE*)lParam = NULL;
						return FALSE;
					}
					// ドロップ先が他プロセスかつカーソルが自プロセスのドロップ可能なウィンドウ上にある場合の対策
					EnableWindow(GetMainHwnd(), FALSE);
					
					/* ドロップファイルリストの作成 */
					/* ファイル名の配列を作成する */
					/* NTの場合はUNICODEになるようにする */
					std::vector<fs::path> filenames;
					for (auto const& f : FileListBaseNoExpand)
						filenames.emplace_back(PathDir / fs::u8path(f.File));
					*(HANDLE*)lParam = CreateDropFileMem(filenames);
					return TRUE;
				}
				break;

			default:
				*((HANDLE *)lParam) = NULL;
				break;
			}

			break;

		case WM_DRAGOVER:
			{
				// 同一ウィンドウ内でのD&Dはリモート側のみ
				if (Win != WIN_REMOTE)
					break;
				if (MoveMode == MOVE_DISABLE)
					break;

				GetCursorPos(&Point);
				hWndPnt = WindowFromPoint(Point);
				ScreenToClient(hWnd, &Point);

				// 以前の選択を消す
				static int prev_index = -1;
				ListView_SetItemState(hWnd, prev_index, 0, LVIS_DROPHILITED);
				RemoteDropFileIndex = -1;

				if (hWndPnt == hWndListRemote)
					if (LVHITTESTINFO hi{ Point }; ListView_HitTest(hWnd, &hi) != -1 && hi.flags == LVHT_ONITEMLABEL) { // The position is over a list-view item's text.
						prev_index = hi.iItem;
						if (GetNodeType(Win, hi.iItem) == NODE_DIR) {
							ListView_SetItemState(hWnd, hi.iItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
							RemoteDropFileIndex = hi.iItem;
						}
					}
			}
			break;

		case WM_RBUTTONDOWN :
			if (AskUserOpeDisabled())
				break;
			/* ここでファイルを選ぶ */
			CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

			EraseListViewTips();
			SetFocus(hWnd);
			if(hWnd == hWndListRemote)
				ShowPopupMenu(WIN_REMOTE, 0);
			else if(hWnd == hWndListLocal)
				ShowPopupMenu(WIN_LOCAL, 0);
			break;

		case WM_LBUTTONDBLCLK :
			if (AskUserOpeDisabled())
				break;
			DoubleClickProc(Win, NO, -1);
			break;

		case WM_MOUSEMOVE :
			if (AskUserOpeDisabled())
				break;
			if(wParam == MK_LBUTTON)
			{
				if((Dragging == NO) && 
				   (hWnd == hWndDragStart) &&
				   (AskConnecting() == YES) &&
				   (SendMessageW(hWnd, LVM_GETSELECTEDCOUNT, 0, 0) > 0) &&
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
					PostMessageW(hWnd, WM_DRAGDROP, MAKEWPARAM(wParam, lParam), 0);

				}
				else
					return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);
			}
			else
			{
				CheckTipsDisplay(hWnd, lParam);
				return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);
			}
			break;

		case WM_MOUSEWHEEL :
			if (AskUserOpeDisabled())
				break;
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
					PostMessageW(hWndPnt, WM_VSCROLL, zDelta > 0 ? MAKEWPARAM(SB_PAGEUP, 0) : MAKEWPARAM(SB_PAGEDOWN, 0), 0);
				}
				else if(hWndPnt == hWnd)
					return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);
				else if((hWndPnt == hWndDst) || (hWndPnt == GetTaskWnd()))
					PostMessageW(hWndPnt, message, wParam, lParam);
			}
			break;

		case WM_NOTIFY:
			switch (auto hdr = reinterpret_cast<NMHDR*>(lParam); hdr->code) {
			case HDN_ITEMCHANGEDW:
				if (auto header = reinterpret_cast<NMHEADERW*>(lParam); header->pitem && (header->pitem->mask & HDI_WIDTH))
					(hWnd == hWndListLocal ? LocalTabWidth : RemoteTabWidth)[header->iItem] = header->pitem->cxy;
				break;
			}
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);

		default :
			return CallWindowProcW(ProcPtr, hWnd, message, wParam, lParam);
	}
	return(0L);
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
			lStyle = GetWindowLongPtrW(GetLocalHwnd(), GWL_STYLE);
			SetWindowLongPtrW(GetLocalHwnd(), GWL_STYLE, lStyle & ~LVS_REPORT | LVS_LIST);

			lStyle = GetWindowLongPtrW(GetRemoteHwnd(), GWL_STYLE);
			SetWindowLongPtrW(GetRemoteHwnd(), GWL_STYLE, lStyle & ~LVS_REPORT | LVS_LIST);
			break;

		default :
			lStyle = GetWindowLongPtrW(GetLocalHwnd(), GWL_STYLE);
			SetWindowLongPtrW(GetLocalHwnd(), GWL_STYLE, lStyle & ~LVS_LIST | LVS_REPORT);

			lStyle = GetWindowLongPtrW(GetRemoteHwnd(), GWL_STYLE);
			SetWindowLongPtrW(GetRemoteHwnd(), GWL_STYLE, lStyle & ~LVS_LIST | LVS_REPORT);
			break;
	}
	return;
}


// ホスト側のファイル一覧ウインドウにファイル名をセット
void GetRemoteDirForWnd(int Mode, int *CancelCheckWork) {
	if (AskConnecting() == YES) {
		DisableUserOpe();
		SetRemoteDirHist(AskRemoteCurDir());
		if (Mode == CACHE_LASTREAD || DoDirListCmdSkt("", "", 0, CancelCheckWork) == FTP_COMPLETE) {
			if (auto lines = GetListLine(0)) {
				std::vector<FILELIST> files;
				for (auto& line : *lines)
					std::visit([&files](auto&& arg) {
						if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, FILELIST>)
							if (arg.Node != NODE_NONE && AskFilterStr(arg.File, arg.Node) == YES && (DotFile == YES || arg.File[0] != '.'))
								files.emplace_back(arg);
					}, line);
				DispFileList2View(GetRemoteHwnd(), files);

				// 先頭のアイテムを選択
				ListView_SetItemState(GetRemoteHwnd(), 0, LVIS_FOCUSED, LVIS_FOCUSED);
			} else {
				SetTaskMsg(MSGJPN048);
				SendMessageW(GetRemoteHwnd(), LVM_DELETEALLITEMS, 0, 0);
			}
		} else {
#if defined(HAVE_OPENVMS)
			/* OpenVMSの場合空ディレクトリ移動の時に出るので、メッセージだけ出さない
			 * ようにする(VIEWはクリアして良い) */
			if (AskHostType() != HTYPE_VMS)
#endif
				SetTaskMsg(MSGJPN049);
			SendMessageW(GetRemoteHwnd(), LVM_DELETEALLITEMS, 0, 0);
		}
		EnableUserOpe();
	}
}


// ローカル側のファイル一覧ウインドウにファイル名をセット
void RefreshIconImageList(std::vector<FILELIST>& files)
{
	HBITMAP hBitmap;
	
	if(DispFileIcon == YES)
	{
		SendMessageW(hWndListLocal, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)NULL);
		ShowWindow(hWndListLocal, SW_SHOW);
		SendMessageW(hWndListRemote, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)NULL);
		ShowWindow(hWndListRemote, SW_SHOW);
		ImageList_Destroy(ListImgFileIcon);
		ListImgFileIcon = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 1);
		hBitmap = LoadBitmapW(GetFtpInst(), MAKEINTRESOURCEW(dirattr16_bmp));
		ImageList_AddMasked(ListImgFileIcon, hBitmap, RGB(255, 0, 0));
		DeleteObject(hBitmap);
		int ImageId = 0;
		for (auto& file : files) {
			file.ImageId = -1;
			auto fullpath = fs::u8path(file.File);
			if (file.Node != NODE_DRIVE)
				fullpath = fs::current_path() / fullpath;
			if (SHFILEINFOW fi; __pragma(warning(suppress:6001)) SHGetFileInfoW(fullpath.c_str(), 0, &fi, sizeof(SHFILEINFOW), SHGFI_SMALLICON | SHGFI_ICON)) {
				if (ImageList_AddIcon(ListImgFileIcon, fi.hIcon) >= 0)
					file.ImageId = ImageId++;
				DestroyIcon(fi.hIcon);
			}
		}
		SendMessageW(hWndListLocal, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImgFileIcon);
		ShowWindow(hWndListLocal, SW_SHOW);
		SendMessageW(hWndListRemote, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImgFileIcon);
		ShowWindow(hWndListRemote, SW_SHOW);
	}
	else
	{
		SendMessageW(hWndListLocal, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImg);
		ShowWindow(hWndListLocal, SW_SHOW);
		SendMessageW(hWndListRemote, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)ListImg);
		ShowWindow(hWndListRemote, SW_SHOW);
	}
}

void GetLocalDirForWnd(void)
{
	char Scan[FMAX_PATH+1];
	std::vector<FILELIST> files;

	DoLocalPWD(Scan);
	SetLocalDirHist(fs::u8path(Scan));
	DispLocalFreeSpace(Scan);

	// ローカル側自動更新
	if(ChangeNotification != INVALID_HANDLE_VALUE)
		FindCloseChangeNotification(ChangeNotification);
	ChangeNotification = FindFirstChangeNotificationW(fs::u8path(Scan).c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);

	/* ディレクトリ／ファイル */
	FindFile(fs::u8path(Scan) / L"*", [&files](WIN32_FIND_DATAW const& data) {
		if (DotFile != YES && data.cFileName[0] == L'.')
			return true;
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			files.emplace_back(u8(data.cFileName), NODE_DIR, NO, (int64_t)data.nFileSizeHigh << 32 | data.nFileSizeLow, 0, data.ftLastWriteTime, ""sv, FINFO_ALL);
		else if (AskFilterStr(u8(data.cFileName).c_str(), NODE_FILE) == YES)
			files.emplace_back(u8(data.cFileName), NODE_FILE, NO, (int64_t)data.nFileSizeHigh << 32 | data.nFileSizeLow, 0, data.ftLastWriteTime, ""sv, FINFO_ALL);
		return true;
	});

	/* ドライブ */
	if (DispDrives)
		GetDrives([&files](const wchar_t drive[]) { files.emplace_back(u8(drive), NODE_DRIVE, NO, 0, 0, FILETIME{}, ""sv, FINFO_ALL); });

	// ファイルアイコン表示対応
	RefreshIconImageList(files);
	DispFileList2View(GetLocalHwnd(), files);

	// 先頭のアイテムを選択
	ListView_SetItemState(GetLocalHwnd(), 0, LVIS_FOCUSED, LVIS_FOCUSED);

	return;
}


// ファイル一覧用リストの内容をファイル一覧ウインドウにセット
static void DispFileList2View(HWND hWnd, std::vector<FILELIST>& files) {
	std::sort(begin(files), end(files), [hWnd](FILELIST& l, FILELIST& r) {
		if (l.Node != r.Node)
			return l.Node < r.Node;
		auto Sort = AskSortType(hWnd == GetRemoteHwnd() ? l.Node == NODE_DIR ? ITEM_RDIR : ITEM_RFILE : l.Node == NODE_DIR ? ITEM_LDIR : ITEM_LFILE);
		auto test = [ascent = (Sort & SORT_GET_ORD) == SORT_ASCENT](auto r) { return ascent ? r < 0 : r > 0; };
		LONGLONG Cmp = 0;
		auto lf = fs::u8path(l.File), rf = fs::u8path(r.File);
		if ((Sort & SORT_MASK_ORD) == SORT_EXT && test(Cmp = _wcsicmp(lf.extension().c_str(), rf.extension().c_str())))
			return true;
#if defined(HAVE_TANDEM)
		if (AskHostType() == HTYPE_TANDEM && (Sort & SORT_MASK_ORD) == SORT_EXT && test(Cmp = (LONGLONG)l.Attr - r.Attr))
			return true;
#endif
		if ((Sort & SORT_MASK_ORD) == SORT_SIZE && test(Cmp = l.Size - r.Size))
			return true;
		if ((Sort & SORT_MASK_ORD) == SORT_DATE && test(Cmp = CompareFileTime(&l.Time, &r.Time)))
			return true;
		if ((Sort & SORT_MASK_ORD) == SORT_NAME || Cmp == 0)
			if (test(_wcsicmp(lf.c_str(), rf.c_str())))
				return true;
		return false;
	});

	SendMessageW(hWnd, WM_SETREDRAW, false, 0);
	SendMessageW(hWnd, LVM_DELETEALLITEMS, 0, 0);

	for (auto& file : files)
		AddListView(hWnd, -1, file.File, file.Node, file.Size, &file.Time, file.Attr, file.Owner, file.Link, file.InfoExist, file.ImageId);

	SendMessageW(hWnd, WM_SETREDRAW, true, 0);
	UpdateWindow(hWnd);

	DispSelectedSpace();
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
static void AddListView(HWND hWnd, int Pos, char* Name, int Type, LONGLONG Size, FILETIME* Time, int Attr, char* Owner, int Link, int InfoExist, int ImageId) {
	LVITEMW item;
	char Tmp[20];

	/* アイコン/ファイル名 */
	if (Pos == -1)
		Pos = std::numeric_limits<int>::max();
	auto wName = u8(Name);
	if (Type == NODE_FILE && AskTransferTypeAssoc(Name, TYPE_X) == TYPE_I)
		Type = 3;
	item = { .mask = LVIF_TEXT | LVIF_IMAGE, .iItem = Pos, .pszText = data(wName), .iImage = DispFileIcon == YES && hWnd == GetLocalHwnd() ? ImageId + 5 : Link == NO ? Type : 4 };
	Pos = (int)SendMessageW(hWnd, LVM_INSERTITEMW, 0, (LPARAM)&item);

	/* 日付/時刻 */
	FileTime2TimeString(Time, Tmp, DISPFORM_LEGACY, InfoExist, DispTimeSeconds);
	auto wTime = u8(Tmp);
	item = { .mask = LVIF_TEXT, .iItem = Pos, .iSubItem = 1, .pszText = data(wTime) };
	SendMessageW(hWnd, LVM_SETITEMW, 0, (LPARAM)&item);

	/* サイズ */
	if (Type == NODE_DIR)
		strcpy(Tmp, "<DIR>");
	else if (Type == NODE_DRIVE)
		strcpy(Tmp, "<DRIVE>");
	else if (Size >= 0)
		strcpy(Tmp, MakeNumString(Size).c_str());
	else
		strcpy(Tmp, "");
	auto wSize = u8(Tmp);
	item = { .mask = LVIF_TEXT, .iItem = Pos, .iSubItem = 2, .pszText = data(wSize) };
	SendMessageW(hWnd, LVM_SETITEMW, 0, (LPARAM)&item);

	/* 拡張子 */
#if defined(HAVE_TANDEM)
	if (AskHostType() == HTYPE_TANDEM)
		_itoa_s(Attr, Tmp, sizeof(Tmp), 10);
	else
#endif
		strncpy_s(Tmp, GetFileExt(Name), _TRUNCATE);
	auto wExt = u8(Tmp);
	item = { .mask = LVIF_TEXT, .iItem = Pos, .iSubItem = 3, .pszText = data(wExt) };
	SendMessageW(hWnd, LVM_SETITEMW, 0, (LPARAM)&item);

	if (hWnd == GetRemoteHwnd()) {
		/* 属性 */
#if defined(HAVE_TANDEM)
		if ((InfoExist & FINFO_ATTR) && (AskHostType() != HTYPE_TANDEM))
#else
		if (InfoExist & FINFO_ATTR)
#endif
			AttrValue2String(Attr, Tmp, DispPermissionsNumber);
		else
			strcpy(Tmp, "");
		auto wAttr = u8(Tmp);
		item = { .mask = LVIF_TEXT, .iItem = Pos, .iSubItem = 4, .pszText = data(wAttr) };
		SendMessageW(hWnd, LVM_SETITEMW, 0, (LPARAM)&item);

		/* オーナ名 */
		auto wOwner = u8(Owner);
		item = { .mask = LVIF_TEXT, .iItem = Pos, .iSubItem = 5, .pszText = data(wOwner) };
		SendMessageW(hWnd, LVM_SETITEMW, 0, (LPARAM)&item);
	}
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


// ワイルドカードにマッチするかどうかを返す
bool CheckFname(std::wstring str, std::wstring const& regexp) {
	// VAX VMSの時は ; 以降は無視する
	if (AskHostType() == HTYPE_VMS)
		if (auto pos = str.find(L';'); pos != std::wstring::npos)
			str.resize(pos);
	return PathMatchSpecW(str.c_str(), regexp.c_str());
}


// ファイル一覧ウインドウのファイルを選択する
void SelectFileInList(HWND hWnd, int Type, std::vector<FILELIST> const& Base) {
	static bool IgnoreNew = false;
	static bool IgnoreOld = false;
	static bool IgnoreExist = false;
	struct Select {
		using result_t = bool;
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, SEL_FNAME, EM_LIMITTEXT, 40, 0);
			SetText(hDlg, SEL_FNAME, u8(FindStr));
			SendDlgItemMessageW(hDlg, SEL_REGEXP, BM_SETCHECK, FindMode, 0);
			SendDlgItemMessageW(hDlg, SEL_NOOLD, BM_SETCHECK, IgnoreOld ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessageW(hDlg, SEL_NONEW, BM_SETCHECK, IgnoreNew ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessageW(hDlg, SEL_NOEXIST, BM_SETCHECK, IgnoreExist ? BST_CHECKED : BST_UNCHECKED, 0);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				strcpy(FindStr, u8(GetText(hDlg, SEL_FNAME)).c_str());
				FindMode = (int)SendDlgItemMessageW(hDlg, SEL_REGEXP, BM_GETCHECK, 0, 0);
				IgnoreOld = SendDlgItemMessageW(hDlg, SEL_NOOLD, BM_GETCHECK, 0, 0) == BST_CHECKED;
				IgnoreNew = SendDlgItemMessageW(hDlg, SEL_NONEW, BM_GETCHECK, 0, 0) == BST_CHECKED;
				IgnoreExist = SendDlgItemMessageW(hDlg, SEL_NOEXIST, BM_GETCHECK, 0, 0) == BST_CHECKED;
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000061);
				break;
			}
		}
	};
	int Win = WIN_LOCAL, WinDst = WIN_REMOTE;
	if (hWnd == GetRemoteHwnd())
		std::swap(Win, WinDst);
	if (Type == SELECT_ALL) {
		LVITEMW item{ 0, 0, 0, GetSelectedCount(Win) <= 1 ? LVIS_SELECTED : 0u, LVIS_SELECTED };
		for (int i = 0, Num = GetItemCount(Win); i < Num; i++)
			if (GetNodeType(Win, i) != NODE_DRIVE)
				SendMessageW(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&item);
		return;
	}
	if (Type == SELECT_REGEXP) {
		if (!Dialog(GetFtpInst(), Win == WIN_LOCAL ? sel_local_dlg : sel_remote_dlg, hWnd, Select{}))
			return;
		try {
			std::variant<std::wstring, boost::wregex> pattern;
			if (FindMode == 0)
				pattern = u8(FindStr);
			else
				pattern = boost::wregex{ u8(FindStr), boost::regex_constants::icase };
			int CsrPos = -1;
			for (int i = 0, Num = GetItemCount(Win); i < Num; i++) {
				char Name[FMAX_PATH + 1];
				GetNodeName(Win, i, Name, FMAX_PATH);
				int Find = FindNameNode(WinDst, Name);
				UINT state = 0;
				if (GetNodeType(Win, i) != NODE_DRIVE) {
					auto matched = std::visit([wName = u8(Name)](auto&& pattern) {
						using t = std::decay_t<decltype(pattern)>;
						if constexpr (std::is_same_v<t, std::wstring>)
							return CheckFname(wName, pattern);
						else if constexpr (std::is_same_v<t, boost::wregex>)
							return boost::regex_match(wName, pattern);
						else
							static_assert(false_v<t>, "not supported variant type.");
					}, pattern);
					if (matched) {
						state = LVIS_SELECTED;
						if (Find >= 0) {
							if (IgnoreExist)
								state = 0;
							else {
								FILETIME Time1, Time2;
								GetNodeTime(Win, i, &Time1);
								GetNodeTime(WinDst, Find, &Time2);
								if (IgnoreNew && CompareFileTime(&Time1, &Time2) > 0 || IgnoreOld && CompareFileTime(&Time1, &Time2) < 0)
									state = 0;
							}
						}
					}
					if (state != 0 && CsrPos == -1)
						CsrPos = i;
				}
				LVITEMW item{ 0, 0, 0, state, LVIS_SELECTED };
				SendMessageW(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&item);
			}
			if (CsrPos != -1) {
				LVITEMW item{ 0, 0, 0, LVIS_FOCUSED, LVIS_FOCUSED };
				SendMessageW(hWnd, LVM_SETITEMSTATE, CsrPos, (LPARAM)&item);
				SendMessageW(hWnd, LVM_ENSUREVISIBLE, CsrPos, (LPARAM)TRUE);
			}
		}
		catch (boost::regex_error&) {}
		return;
	}
	if (Type == SELECT_LIST) {
		for (int i = 0, Num = GetItemCount(Win); i < Num; i++) {
			char Name[FMAX_PATH + 1];
			GetNodeName(Win, i, Name, FMAX_PATH);
			LVITEMW item{ 0, 0, 0, SearchFileList(Name, Base, COMP_STRICT) != NULL ? LVIS_SELECTED : 0u, LVIS_SELECTED };
			SendMessageW(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&item);
		}
		return;
	}
}


// ファイル一覧ウインドウのファイルを検索する
void FindFileInList(HWND hWnd, int Type) {
	static std::variant<std::wstring, boost::wregex> pattern;
	int Win = hWnd == GetRemoteHwnd() ? WIN_REMOTE : WIN_LOCAL;
	switch (Type) {
	case FIND_FIRST:
		if (!InputDialog(find_dlg, hWnd, Win == WIN_LOCAL ? MSGJPN050 : MSGJPN051, FindStr, 40 + 1, &FindMode))
			return;
		try {
			if (FindMode == 0)
				pattern = u8(FindStr);
			else
				pattern = boost::wregex{ u8(FindStr), boost::regex_constants::icase };
		}
		catch (boost::regex_error&) {
			return;
		}
		[[fallthrough]];
	case FIND_NEXT:
		for (int i = GetCurrentItem(Win) + 1, Num = GetItemCount(Win); i < Num; i++) {
			char Name[FMAX_PATH + 1];
			GetNodeName(Win, i, Name, FMAX_PATH);
			auto match = std::visit([wName = u8(Name)](auto&& pattern) {
				using t = std::decay_t<decltype(pattern)>;
				if constexpr (std::is_same_v<t, std::wstring>)
					return CheckFname(wName, pattern);
				else if constexpr (std::is_same_v<t, boost::wregex>)
					return boost::regex_match(wName, pattern);
				else
					static_assert(false_v<t>, "not supported variant type.");
			}, pattern);
			if (match) {
				LVITEMW item{ 0, 0, 0, LVIS_FOCUSED, LVIS_FOCUSED };
				SendMessageW(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&item);
				SendMessageW(hWnd, LVM_ENSUREVISIBLE, i, (LPARAM)TRUE);
				break;
			}
		}
		break;
	}
}


/*----- カーソル位置のアイテム番号を返す --------------------------------------
*
*	Parameter
*		int Win : ウィンドウ番号 (WIN_xxxx)
*
*	Return Value
*		int アイテム番号
*----------------------------------------------------------------------------*/

int GetCurrentItem(int Win) {
	auto Ret = (int)SendMessageW(Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd(), LVM_GETNEXTITEM, -1, LVNI_ALL | LVNI_FOCUSED);
	return Ret == -1 ? 0 : Ret;
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

	return (int)(SendMessageW(hWnd, LVM_GETITEMCOUNT, 0, 0));
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

	return (int)(SendMessageW(hWnd, LVM_GETSELECTEDCOUNT, 0, 0));
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

int GetFirstSelected(int Win, int All) {
	return GetNextSelected(Win, -1, All);
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

int GetNextSelected(int Win, int Pos, int All) {
	return (int)SendMessageW(Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd(), LVM_GETNEXTITEM, Pos, All == YES ? LVNI_ALL : LVNI_SELECTED);
}


// ローカル側自動更新
int GetHotSelected(int Win, char* Fname) {
	auto Pos = (int)SendMessageW(Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd(), LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
	if (Pos != -1)
		GetNodeName(Win, Pos, Fname, FMAX_PATH);
	return Pos;
}

int SetHotSelected(int Win, char* Fname) {
	auto wFname = u8(Fname);
	auto hWnd = Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd();
	int Pos = -1;
	for (int i = 0, Num = GetItemCount(Win); i < Num; i++) {
		LVITEMW item{ .stateMask = LVIS_FOCUSED };
		if (wFname == GetNodeName(Win, i)) {
			Pos = i;
			item.state = LVIS_FOCUSED;
		}
		SendMessageW(hWnd, LVM_SETITEMSTATE, i, (LPARAM)&item);
	}
	return Pos;
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

int FindNameNode(int Win, char* Name) {
	auto wName = u8(Name);
	LVFINDINFOW fi{ LVFI_STRING, wName.c_str() };
	return (int)SendMessageW(Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd(), LVM_FINDITEMW, -1, (LPARAM)&fi);
}


static std::wstring GetItemText(int Win, int index, int subitem) {
	wchar_t buffer[260 + 1];
	LVITEMW item{ .iSubItem = subitem, .pszText = buffer, .cchTextMax = size_as<int>(buffer) };
	auto length = SendMessageW(Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd(), LVM_GETITEMTEXTW, index, (LPARAM)&item);
	return { buffer, (size_t)length };
}

// 指定位置のアイテムの名前を返す
std::wstring GetNodeName(int Win, int Pos) {
	return GetItemText(Win, Pos, 0);
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

void GetNodeName(int Win, int Pos, char* Buf, int Max) {
	strncpy_s(Buf, Max, u8(GetNodeName(Win, Pos)).c_str(), _TRUNCATE);
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

int GetNodeTime(int Win, int Pos, FILETIME* Buf) {
	auto time = GetItemText(Win, Pos, 1);
	return TimeString2FileTime(u8(time).c_str(), Buf);
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

int GetNodeSize(int Win, int Pos, LONGLONG* Buf) {
	if (auto size = GetItemText(Win, Pos, 2); !empty(size)) {
		size.erase(std::remove(begin(size), end(size), L','), end(size));
		*Buf = !empty(size) && std::iswdigit(size[0]) ? stoll(size) : 0;
		return YES;
	} else {
		*Buf = -1;
		return NO;
	}
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

int GetNodeAttr(int Win, int Pos, int* Buf) {
	if (Win == WIN_REMOTE) {
		auto subitem =
#if defined(HAVE_TANDEM)
			AskHostType() == HTYPE_TANDEM ? 3 :
#endif
			4;
		if (auto attr = GetItemText(WIN_REMOTE, Pos, subitem); !empty(attr)) {
			*Buf =
#if defined(HAVE_TANDEM)
				AskHostType() == HTYPE_TANDEM ? (std::iswdigit(attr[0]) ? stoi(attr) : 0) :
#endif
				AttrString2Value(u8(attr).c_str());
			return YES;
		}
	}
	*Buf = 0;
	return NO;
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

int GetNodeType(int Win, int Pos) {
	auto type = GetItemText(Win, Pos, 2);
	return type == L"<DIR>"sv ? NODE_DIR : type == L"<DRIVE>"sv ? NODE_DRIVE : NODE_FILE;
}


/*----- 指定位置のアイテムのイメージ番号を返す ----------------------------------------
*
*	Parameter
*		int Win : ウインドウ番号 (WIN_xxx)
*		int Pos : 位置
*
*	Return Value
*		int イメージ番号
*			4 Symlink
*----------------------------------------------------------------------------*/
static int GetImageIndex(int Win, int Pos) {
	LVITEMW item{ .mask = LVIF_IMAGE, .iItem = Pos, .iSubItem = 0 };
	SendMessageW(Win == WIN_REMOTE ? GetRemoteHwnd() : GetLocalHwnd(), LVM_GETITEMW, 0, (LPARAM)&item);
	return item.iImage;
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

void GetNodeOwner(int Win, int Pos, char* Buf, int Max) {
	if (Win == WIN_REMOTE) {
		auto owner = GetItemText(WIN_REMOTE, Pos, 5);
		strncpy_s(Buf, Max, u8(owner).c_str(), _TRUNCATE);
	} else
		strcpy(Buf, "");
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
	SendMessageW(GetRemoteHwnd(), LVM_DELETEALLITEMS, 0, 0);
	SendMessageW(GetRemoteHistHwnd(), CB_RESETCONTENT, 0, 0);
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

int MakeSelectedFileList(int Win, int Expand, int All, std::vector<FILELIST>& Base, int *CancelCheckWork) {
	int Sts;
	int Pos;
	char Name[FMAX_PATH+1];
	char Cur[FMAX_PATH+1];
	int Node;
	int Ignore;

	// ファイル一覧バグ修正
	Sts = FFFTP_SUCCESS;
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
				FILELIST Pkt{};
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
					if (auto attr = GetFileAttributesW((AskLocalCurDir() / fs::u8path(Pkt.File)).c_str()); attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_HIDDEN))
						Ignore = YES;

				if(Ignore == NO)
					AddFileList(Pkt, Base);
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
					FILELIST Pkt{};
					GetNodeName(Win, Pos, Name, FMAX_PATH);
					strcpy(Pkt.File, Name);
					ReplaceAll(Pkt.File, '\\', '/');
//8/26

					Ignore = NO;
					if((DispIgnoreHide == YES) && (Win == WIN_LOCAL))
						if (auto attr = GetFileAttributesW((AskLocalCurDir() / fs::u8path(Pkt.File)).c_str()); attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_HIDDEN))
							Ignore = YES;

					if(Ignore == NO)
					{
//						Pkt.Node = NODE_DIR;
						if(GetImageIndex(Win, Pos) == 4) // symlink
							Pkt.Node = NODE_FILE;
						else
							Pkt.Node = NODE_DIR;
						AddFileList(Pkt, Base);

						if(GetImageIndex(Win, Pos) != 4) { // symlink
							if(Win == WIN_LOCAL)
							// ファイル一覧バグ修正
//								MakeLocalTree(Name, Base);
							{
								if(!MakeLocalTree(Name, Base))
									Sts = FFFTP_FAIL;
							}
							else
							{
								strcpy(Cur, u8(AskRemoteCurDir()).c_str());

								if((AskListCmdMode() == NO) &&
								   (AskUseNLST_R() == YES))
								// ファイル一覧バグ修正
//									MakeRemoteTree1(Name, Cur, Base, CancelCheckWork);
								{
									if(MakeRemoteTree1(Name, Cur, Base, CancelCheckWork) == FFFTP_FAIL)
										Sts = FFFTP_FAIL;
								}
								else
								// ファイル一覧バグ修正
//									MakeRemoteTree2(Name, Cur, Base, CancelCheckWork);
								{
									if(MakeRemoteTree2(Name, Cur, Base, CancelCheckWork) == FFFTP_FAIL)
										Sts = FFFTP_FAIL;
								}
							}
						}
					}
				}
				Pos = GetNextSelected(Win, Pos, All);
			}
		}
	}
	// ファイル一覧バグ修正
//	return;
	return(Sts);
}


static inline fs::path DragFile(HDROP hdrop, UINT index) {
	auto const length1 = DragQueryFileW(hdrop, index, nullptr, 0);
	std::wstring buffer(length1, L'\0');
	auto const length2 = DragQueryFileW(hdrop, index, data(buffer), length1 + 1);
	assert(length1 == length2);
	return std::move(buffer);
}

// Drag&Dropされたファイルをリストに登録する
void MakeDroppedFileList(WPARAM wParam, char* Cur, std::vector<FILELIST>& Base) {
	int count = DragQueryFileW((HDROP)wParam, 0xFFFFFFFF, NULL, 0);

	auto const baseDirectory = DragFile((HDROP)wParam, 0).parent_path();
	strncpy(Cur, baseDirectory.u8string().c_str(), FMAX_PATH);

	std::vector<fs::path> directories;
	for (int i = 0; i < count; i++) {
		auto const path = DragFile((HDROP)wParam, i);
		WIN32_FILE_ATTRIBUTE_DATA attr;
		if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attr))
			continue;
		if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			directories.emplace_back(path);
		else {
			FILELIST Pkt{};
			Pkt.Node = NODE_FILE;
			strcpy(Pkt.File, path.filename().u8string().c_str());
			if (SYSTEMTIME TmpStime; FileTimeToSystemTime(&attr.ftLastWriteTime, &TmpStime)) {
				if (DispTimeSeconds == NO)
					TmpStime.wSecond = 0;
				TmpStime.wMilliseconds = 0;
				SystemTimeToFileTime(&TmpStime, &Pkt.Time);
			}
#if defined(HAVE_TANDEM)
			Pkt.Size = LONGLONG(attr.nFileSizeHigh) << 32 | attr.nFileSizeLow;
			Pkt.InfoExist = FINFO_TIME | FINFO_DATE | FINFO_SIZE;
#endif
			AddFileList(Pkt, Base);
		}
	}

	auto const saved = fs::current_path();
	std::error_code ec;
	fs::current_path(baseDirectory, ec);
	for (auto const& path : directories) {
		FILELIST Pkt{};
		Pkt.Node = NODE_DIR;
		strcpy(Pkt.File, path.filename().u8string().c_str());
		AddFileList(Pkt, Base);
		MakeLocalTree(Pkt.File, Base);
	}
	fs::current_path(saved);
}


// Drag&Dropされたファイルがあるフォルダを取得する
void MakeDroppedDir(WPARAM wParam, char* Cur) {
	strncpy(Cur, DragFile((HDROP)wParam, 0).parent_path().u8string().c_str(), FMAX_PATH);
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

static int MakeRemoteTree1(char *Path, char *Cur, std::vector<FILELIST>& Base, int *CancelCheckWork) {
	int Ret;
	int Sts;

	// ファイル一覧バグ修正
	Ret = FFFTP_FAIL;
	if(DoCWD(Path, NO, NO, NO) == FTP_COMPLETE)
	{
		/* サブフォルダも含めたリストを取得 */
		Sts = DoDirListCmdSkt("R", "", 999, CancelCheckWork);	/* NLST -alLR*/
		DoCWD(Cur, NO, NO, NO);

		if(Sts == FTP_COMPLETE)
		{
			AddRemoteTreeToFileList(999, Path, RDIR_NLST, Base);
			Ret = FFFTP_SUCCESS;
		}
	}
	return(Ret);
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

static int MakeRemoteTree2(char *Path, char *Cur, std::vector<FILELIST>& Base, int *CancelCheckWork) {
	int Ret;
	int Sts;

	// ファイル一覧バグ修正
	Ret = FFFTP_FAIL;
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
			std::vector<FILELIST> CurList;
			AddRemoteTreeToFileList(999, Path, RDIR_CWD, CurList);
			CopyTmpListToFileList(Base, CurList);

			// ファイル一覧バグ修正
			Ret = FFFTP_SUCCESS;

			for (auto const& f : CurList)
				if (f.Node == NODE_DIR) {
					FILELIST Pkt{};
					/* まずディレクトリ名をセット */
					strcpy(Pkt.File, f.File);
					Pkt.Link = f.Link;
					if (Pkt.Link == YES)
						Pkt.Node = NODE_FILE;
					else
						Pkt.Node = NODE_DIR;
					AddFileList(Pkt, Base);

					if (Pkt.Link == NO && MakeRemoteTree2(const_cast<char*>(f.File), Cur, Base, CancelCheckWork) == FFFTP_FAIL)
						Ret = FFFTP_FAIL;
				}
		}
	}
	return(Ret);
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

static void CopyTmpListToFileList(std::vector<FILELIST>& Base, std::vector<FILELIST> const& List) {
	for (auto& f : List)
		if (f.Node == NODE_FILE)
			AddFileList(f, Base);
}


// ホスト側のファイル情報をファイルリストに登録
void AddRemoteTreeToFileList(int Num, char *Path, int IncDir, std::vector<FILELIST>& Base) {
	char Dir[FMAX_PATH+1];
	strcpy(Dir, Path);
	if (auto lines = GetListLine(Num))
		for (auto& line : *lines)
			std::visit([&Path, IncDir, &Base, &Dir](auto&& arg) {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, FILELIST>) {
					if (AskFilterStr(arg.File, arg.Node) == YES && (arg.Node == NODE_FILE || IncDir == RDIR_CWD && arg.Node == NODE_DIR)) {
						FILELIST Pkt{ Dir, arg.Node, arg.Link, arg.Size, arg.Attr, arg.Time, ""sv, arg.InfoExist };
						if (0 < strlen(Pkt.File))
							SetSlashTail(Pkt.File);
						strcat(Pkt.File, arg.File);
						AddFileList(Pkt, Base);
					}
				} else {
					static_assert(std::is_same_v<std::decay_t<decltype(arg)>, std::string>);
					if (MakeDirPath(data(arg), Path, Dir) == FFFTP_SUCCESS && IncDir == RDIR_NLST)
						AddFileList({ Dir, NODE_DIR }, Base);
				}
			}, line);
}

namespace re {
	static auto compile(std::tuple<std::string_view, bool> const& p) {
		auto [pattern, icase] = p;
		return icase ? boost::regex{ data(pattern), data(pattern) + size(pattern), boost::regex::icase } : boost::regex{ data(pattern), data(pattern) + size(pattern) };
	}
	static auto const mlsd      = compile(filelistparser::mlsd);
	static auto const unix      = compile(filelistparser::unix);
	static auto const linux     = compile(filelistparser::linux);
	static auto const dos       = compile(filelistparser::dos);
	static auto const melcom80  = compile(filelistparser::melcom80);
	static auto const agilent   = compile(filelistparser::agilent);
	static auto const as400     = compile(filelistparser::as400);
	static auto const m1800     = compile(filelistparser::m1800);
	static auto const gp6000    = compile(filelistparser::gp6000);
	static auto const chameleon = compile(filelistparser::chameleon);
	static auto const os2       = compile(filelistparser::os2);
	static auto const os7       = compile(filelistparser::os7);
	static auto const os9       = compile(filelistparser::os9);
	static auto const allied    = compile(filelistparser::allied);
	static auto const ibm       = compile(filelistparser::ibm);
	static auto const shibasoku = compile(filelistparser::shibasoku);
	static auto const stratus   = compile(filelistparser::stratus);
	static auto const vms       = compile(filelistparser::vms);
	static auto const irmx      = compile(filelistparser::irmx);
	static auto const tandem    = compile(filelistparser::tandem);
}

template<class SubMatch, class StringView = std::basic_string_view<SubMatch::value_type>>
static inline StringView sv(SubMatch const& sm) {
	return { &*sm.begin(), static_cast<typename StringView::size_type>(sm.length()) };
}

template<class Int>
static inline Int parse(std::string_view sv) {
	Int value = 0;
	std::from_chars(data(sv), data(sv) + size(sv), value);
	return value;
}

template<class Int>
static inline Int parse(boost::ssub_match const& sm) {
	return parse<Int>(sv(sm));
}

static inline WORD parseyear(boost::ssub_match const& sm) {
	auto year = parse<WORD>(sm);
	return year < 70 ? 2000 + year : year < 1601 ? 1900 + year : year;
}

static inline WORD parsemonth(boost::ssub_match const& sm) {
	if (sm.length() != 3)
		return parse<WORD>(sm);
	auto it = sm.begin();
	char name[] = { (char)toupper(*it++), (char)tolower(*it++), (char)tolower(*it++) };
	auto i = "JanFebMarAprMayJunJulAugSepOctNovDec"sv.find({ name, 3 });
	return (WORD)i / 3 + 1;
}

static int parseattr(boost::ssub_match const& m) {
	int attr = 0;
	auto it = m.begin();
	for (auto mask : { 0x400, 0x200, 0x100, 0x40, 0x20, 0x10, 0x4, 0x2, 0x1 }) {
		if (*it++ != '-')
			attr |= mask;
		if (it == m.end())
			break;
	}
	return attr;
}


static inline FILETIME tofiletime(SYSTEMTIME const& systemTime, bool fix = false) {
	FILETIME fileTime;
	if (fix) {
		TIME_ZONE_INFORMATION tz{ AskHostTimeZone() * -60 };
		SYSTEMTIME fixed;
		TzSpecificLocalTimeToSystemTime(&tz, &systemTime, &fixed);
		SystemTimeToFileTime(&fixed, &fileTime);
	} else
		SystemTimeToFileTime(&systemTime, &fileTime);
	return fileTime;
}

static inline std::string_view stripSymlink(std::string_view name) {
	auto const pos = name.find(" -> "sv);
	return pos == std::string_view::npos ? name : name.substr(0, pos);
}

static std::optional<FILELIST> ParseMlsd(boost::smatch const& m) {
	char type = NODE_NONE;
	int64_t size = 0;
	int attr = 0;
	FILETIME fileTime{};
	std::string_view owner;
	char infoExist = 0;
	static const boost::regex re{ R"(([^;=]+)=(([^;=]+)(?:=[^;]*)?))" };
	for (boost::sregex_iterator it{ m[1].begin(), m[1].end(), re }, end; it != end; ++it) {
		auto factname = lc((*it)[1]);
		auto value = sv((*it)[2]);
		if (factname == "type"sv) {
			if (auto lcvalue = lc(value); lcvalue == "dir"sv)
				type = NODE_DIR;
			else if (lcvalue == "file"sv)
				type = NODE_FILE;
			// TODO: OS.unix=symlink、OS.unix=slinkの判定を行っているがバグっていて成功しない
		} else if (factname == "modify"sv) {
			infoExist |= FINFO_DATE | FINFO_TIME;
			SYSTEMTIME systemTime{};
			std::from_chars(value.data() + 0, value.data() + 4, systemTime.wYear);
			std::from_chars(value.data() + 4, value.data() + 6, systemTime.wMonth);
			std::from_chars(value.data() + 6, value.data() + 8, systemTime.wDay);
			std::from_chars(value.data() + 8, value.data() + 10, systemTime.wHour);
			std::from_chars(value.data() + 10, value.data() + 12, systemTime.wMinute);
			std::from_chars(value.data() + 12, value.data() + 14, systemTime.wSecond);
			SystemTimeToFileTime(&systemTime, &fileTime);
		} else if (factname == "size"sv) {
			infoExist |= FINFO_SIZE;
			size = parse<int64_t>(value);
		} else if (factname == "unix.mode"sv) {
			infoExist |= FINFO_ATTR;
			std::from_chars(value.data(), value.data() + value.size(), attr, 16);
		} else if (factname == "unix.owner"sv)
			owner = value;
	}
	return { { sv(m[2]), type, NO, size, attr, fileTime, owner, infoExist } };
}

static std::optional<FILELIST> ParseUnix(boost::smatch const& m) {
	SYSTEMTIME systemTime{};
	auto fixtimezone = false;
	char infoExist = FINFO_SIZE | FINFO_ATTR | FINFO_DATE;
	if (m[5].matched) {
		systemTime.wMonth = parsemonth(m[5]);
		systemTime.wDay = parse<WORD>(m[6]);
		if (m[7].matched) {
			systemTime.wYear = parse<WORD>(m[7]);
		} else {
			infoExist |= FINFO_TIME;
			SYSTEMTIME utcnow, localnow;
			GetSystemTime(&utcnow);
			TIME_ZONE_INFORMATION tz{ AskHostTimeZone() * -60 };
			SystemTimeToTzSpecificLocalTime(&tz, &utcnow, &localnow);
			systemTime.wHour = parse<WORD>(m[8]);
			systemTime.wMinute = parse<WORD>(m[9]);
			auto serialize = [](SYSTEMTIME const& st) { return (uint64_t)st.wMonth << 48 | (uint64_t)st.wDay << 32 | (uint64_t)st.wHour << 16 | st.wMinute; };
			systemTime.wYear = serialize(localnow) < serialize(systemTime) ? localnow.wYear - 1 : localnow.wYear;
			fixtimezone = true;
		}
	} else {
		systemTime.wYear = parse<WORD>(m[10]);
		systemTime.wMonth = parsemonth(m[11]);
		systemTime.wDay = parse<WORD>(m[12]);
	}
	auto ch = *m[1].begin();
	return { { stripSymlink(sv(m[13])), ch == 'd' || ch == 'l' ? NODE_DIR : NODE_FILE, ch == 'l' ? YES : NO, parse<int64_t>(m[4]), parseattr(m[2]), tofiletime(systemTime, fixtimezone), sv(m[3]), infoExist } };
}

static std::optional<FILELIST> ParseLinux(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[5]), .wMonth = parse<WORD>(m[6]), .wDay = parse<WORD>(m[7]), .wHour = parse<WORD>(m[8]), .wMinute = parse<WORD>(m[9]) };
	auto ch = *m[1].begin();
	return { { stripSymlink(sv(m[10])), ch == 'd' || ch == 'l' ? NODE_DIR : NODE_FILE, ch == 'l' ? YES : NO, parse<int64_t>(m[4]), parseattr(m[2]), tofiletime(systemTime, true), sv(m[3]), FINFO_SIZE | FINFO_ATTR | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseMelcom80(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parse<WORD>(m[7]), .wMonth = parsemonth(m[5]), .wDay = parse<WORD>(m[6]) };
	char node;
	auto ch = *m[1].begin();
	if (ch == 'd' || ch == 'l')
		node = NODE_DIR;
	else if (*m[9].begin() == 'B')
		node = NODE_FILE;
	else
		return {};
	return { { sv(m[8]), node, ch == 'l' ? YES : NO, parse<int64_t>(m[4]), parseattr(m[2]), tofiletime(systemTime), sv(m[3]), FINFO_SIZE | FINFO_ATTR | FINFO_DATE } };
}

static std::optional<FILELIST> ParseAgilent(boost::smatch const& m) {
	auto ch = *m[1].begin();
	return { { stripSymlink(sv(m[5])), ch == 'd' || ch == 'l' ? NODE_DIR : NODE_FILE, ch == 'l' ? YES : NO, parse<int64_t>(m[4]), parseattr(m[2]), {}, sv(m[3]), FINFO_SIZE | FINFO_ATTR } };
}

static std::optional<FILELIST> ParseDos(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wHour = parse<WORD>(m[9]), .wMinute = parse<WORD>(m[10]) };
	if (m[1].matched) {
		systemTime.wYear = parseyear(m[1]);
		systemTime.wMonth = parse<WORD>(m[3]);
		systemTime.wDay = parse<WORD>(m[4]);
	} else {
		systemTime.wYear = parseyear(m[8]);
		systemTime.wMonth = parse<WORD>(m[5]);
		systemTime.wDay = parse<WORD>(m[7]);
	}
	if (m[11].matched)
		systemTime.wSecond = parse<WORD>(m[11]);
	if (auto ch = *m[12].begin(); ch == 'a' || ch == 'A') {
		if (systemTime.wHour == 12)
			systemTime.wHour = 0;
	} else
		if (systemTime.wHour < 12)
			systemTime.wHour += 12;
	if (*m[13].begin() == '<')
		return { { sv(m[14]), NODE_DIR, NO, 0, 0, tofiletime(systemTime, true), ""sv, FINFO_DATE | FINFO_TIME } };
	return { { sv(m[14]), NODE_FILE, NO, parse<int64_t>(m[13]), 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseChameleon(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[5]), .wMonth = parsemonth(m[3]), .wDay = parse<WORD>(m[4]), .wHour = parse<WORD>(m[6]), .wMinute = parse<WORD>(m[7]) };
	if (*m[2].begin() == '<')
		return { { sv(m[1]), NODE_DIR, 0, 0, 0, tofiletime(systemTime, true), ""sv, FINFO_DATE | FINFO_TIME } };
	return { { sv(m[1]), NODE_FILE, NO, parse<int64_t>(m[2]), 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseOs2(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[5]), .wMonth = parse<WORD>(m[3]), .wDay = parse<WORD>(m[4]), .wHour = parse<WORD>(m[6]), .wMinute = parse<WORD>(m[7]) };
	return { { sv(m[8]), m[2].matched ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(m[1]), 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseAllied(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[8]), .wMonth = parsemonth(m[3]), .wDay = parse<WORD>(m[4]), .wHour = parse<WORD>(m[5]), .wMinute = parse<WORD>(m[6]), .wSecond = parse<WORD>(m[7]) };
	if(*m[1].begin() == '<')
		return { { sv(m[2]), NODE_DIR, NO, 0, 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
	return { { sv(m[2]), NODE_FILE, NO, parse<int64_t>(m[1]), 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseShibasoku(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parse<WORD>(m[4]), .wMonth = parsemonth(m[2]), .wDay = parse<WORD>(m[3]), .wHour = parse<WORD>(m[5]), .wMinute = parse<WORD>(m[6]), .wSecond = parse<WORD>(m[7]) };
	return { { sv(m[8]), m[9].matched ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(m[1]), 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseAs400(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[3]), .wMonth = parse<WORD>(m[4]), .wDay = parse<WORD>(m[5]), .wHour = parse<WORD>(m[6]), .wMinute = parse<WORD>(m[7]), .wSecond = parse<WORD>(m[8]) };
	return { { sv(m[9]), m[10].matched ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(m[2]), 0, tofiletime(systemTime, true), sv(m[1]), FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseM1800(boost::smatch const& m) {
	FILETIME fileTime{};
	char infoExist = FINFO_ATTR;
	if (m[2].matched) {
		infoExist |= FINFO_DATE;
		SYSTEMTIME systemTime{ .wYear = parseyear(m[2]), .wMonth = parse<WORD>(m[3]), .wDay = parse<WORD>(m[4]) };
		fileTime = tofiletime(systemTime);
	}
	return { { sv(m[5]), m[6].matched ? NODE_DIR : NODE_FILE, NO, 0, parseattr(m[1]), fileTime, ""sv, infoExist } };
}

static std::optional<FILELIST> ParseGp6000(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[3]), .wMonth = parse<WORD>(m[4]), .wDay = parse<WORD>(m[5]), .wHour = parse<WORD>(m[6]), .wMinute = parse<WORD>(m[7]), .wSecond = parse<WORD>(m[8]) };
	auto ch = *m[1].begin();
	return { { sv(m[11]), ch == 'd' || ch == 'l' ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(m[10]), parseattr(m[2]), tofiletime(systemTime, true), sv(m[9]), FINFO_SIZE | FINFO_ATTR | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseOs7(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[4]), .wMonth = parse<WORD>(m[5]), .wDay = parse<WORD>(m[6]), .wHour = parse<WORD>(m[7]), .wMinute = parse<WORD>(m[8]), .wSecond = parse<WORD>(m[9]) };
	char infoExist = FINFO_ATTR | FINFO_DATE | FINFO_TIME;
	int64_t size = -1;
	if (m[3].matched) {
		infoExist |= FINFO_SIZE;
		size = parse<int64_t>(m[3]);
	}
	auto ch = *m[1].begin();
	return { { sv(m[10]), ch == 'd' || ch == 'l' ? NODE_DIR : NODE_FILE, NO, size, parseattr(m[2]), tofiletime(systemTime, true), ""sv, infoExist } };
}

static std::optional<FILELIST> ParseOs9(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[2]), .wMonth = parse<WORD>(m[3]), .wDay = parse<WORD>(m[4]), .wHour = parse<WORD>(m[5]), .wMinute = parse<WORD>(m[6]) };
	return { { sv(m[9]), *m[7].begin() == 'd' ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(m[8]), 0, tofiletime(systemTime, true), sv(m[1]), FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseIbm(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parse<WORD>(m[1]), .wMonth = parse<WORD>(m[2]), .wDay = parse<WORD>(m[3]) };
	return { { sv(m[5]), *m[4].begin() == 'O' ? NODE_DIR : NODE_FILE, NO, 0, 0, tofiletime(systemTime), ""sv, FINFO_DATE } };
}

static std::optional<FILELIST> ParseStratus(boost::smatch const& m) {
	static char type = NODE_NONE;
	if (m[1].matched)
		type = NODE_FILE;
	else if (m[2].matched)
		type = NODE_DIR;
	else if (m[3].matched)
		type = NODE_NONE;
	if (type == NODE_NONE)
		return {};
	int64_t size = 0;
	char infoExist = FINFO_DATE | FINFO_TIME;
	if (type == NODE_FILE) {
		size = parse<int64_t>(m[4]) * 4096;
		infoExist |= FINFO_SIZE;
	}
	SYSTEMTIME systemTime{ .wYear = parseyear(m[6]), .wMonth = parse<WORD>(m[7]), .wDay = parse<WORD>(m[8]), .wHour = parse<WORD>(m[9]), .wMinute = parse<WORD>(m[10]), .wSecond = parse<WORD>(m[11]) };
	return { { sv(m[12]), type, NO, size, 0, tofiletime(systemTime, true), sv(m[5]), infoExist } };
}

static std::optional<FILELIST> ParseVms(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parse<WORD>(m[7]), .wMonth = parsemonth(m[6]), .wDay = parse<WORD>(m[5]), .wHour = parse<WORD>(m[8]), .wMinute = parse<WORD>(m[9]), .wSecond = parse<WORD>(m[10]) };
#ifdef HAVE_OPENVMS
	constexpr int filename = 1;
#else
	constexpr int filename = 2;
#endif
	return { { sv(m[filename]), m[3].matched ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(m[4]) * 512, 0, tofiletime(systemTime, true), ""sv, FINFO_SIZE | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> ParseIrmx(boost::smatch const& m) {
	SYSTEMTIME systemTime{ .wYear = parseyear(m[7]), .wMonth = parsemonth(m[6]), .wDay = parse<WORD>(m[5]) };
	std::string size = m[3];
	size.erase(std::remove(begin(size), end(size), ','), end(size));
	return { { sv(m[1]), m[2].matched ? NODE_DIR : NODE_FILE, NO, parse<int64_t>(size), 0, tofiletime(systemTime), sv(m[4]), FINFO_SIZE | FINFO_DATE } };
}

static std::optional<FILELIST> ParseTandem(boost::smatch const& m) {
	// 属性にはFileCodeを充てる
	SYSTEMTIME systemTime{ .wYear = parseyear(m[6]), .wMonth = parsemonth(m[5]), .wDay = parse<WORD>(m[4]), .wHour = parse<WORD>(m[7]), .wMinute = parse<WORD>(m[8]), .wSecond = parse<WORD>(m[9]) };
	return { { sv(m[1]), NODE_FILE, NO, parse<int64_t>(m[3]), parse<int>(m[2]), tofiletime(systemTime, true), sv(m[10]), FINFO_SIZE | FINFO_ATTR | FINFO_DATE | FINFO_TIME } };
}

static std::optional<FILELIST> Parse(std::string const& line) {
	boost::smatch m;
	switch (AskHostType()) {
	case HTYPE_ACOS:
	case HTYPE_ACOS_4:
		return { { line.c_str(), NODE_FILE } };
	case HTYPE_VMS:
		return boost::regex_search(line, m, re::vms) ? ParseVms(m) : std::nullopt;
	case HTYPE_IRMX:
		return boost::regex_search(line, m, re::irmx) ? ParseIrmx(m) : std::nullopt;
	case HTYPE_STRATUS:
		return boost::regex_search(line, m, re::stratus) ? ParseStratus(m) : std::nullopt;
	case HTYPE_AGILENT:
		return boost::regex_search(line, m, re::agilent) ? ParseAgilent(m) : std::nullopt;
	case HTYPE_SHIBASOKU:
		return boost::regex_search(line, m, re::shibasoku) ? ParseShibasoku(m) : std::nullopt;
	}
#if defined(HAVE_TANDEM)
	if (AskRealHostType() == HTYPE_TANDEM)
		SetOSS(YES);
#endif
	if (boost::regex_search(line, m, re::mlsd))
		return ParseMlsd(m);
	if (boost::regex_search(line, m, re::unix))
		return ParseUnix(m);
	if (boost::regex_search(line, m, re::linux))
		return ParseLinux(m);
	if (boost::regex_search(line, m, re::dos))
		return ParseDos(m);
	if (boost::regex_search(line, m, re::melcom80))
		return ParseMelcom80(m);
	if (boost::regex_search(line, m, re::agilent))
		return ParseAgilent(m);
	if (boost::regex_search(line, m, re::as400))
		return ParseAs400(m);
	if (boost::regex_search(line, m, re::m1800))
		return ParseM1800(m);
	if (boost::regex_search(line, m, re::gp6000))
		return ParseGp6000(m);
	if (boost::regex_search(line, m, re::chameleon))
		return ParseChameleon(m);
	if (boost::regex_search(line, m, re::os2))
		return ParseOs2(m);
	if (boost::regex_search(line, m, re::os7))
		return ParseOs7(m);
	if (boost::regex_search(line, m, re::os9))
		return ParseOs9(m);
	if (boost::regex_search(line, m, re::allied))
		return ParseAllied(m);
	if (boost::regex_search(line, m, re::ibm))
		return ParseIbm(m);
	if (boost::regex_search(line, m, re::shibasoku))
		return ParseShibasoku(m);
	if (boost::regex_search(line, m, re::stratus))
		return ParseStratus(m);
	if (boost::regex_search(line, m, re::vms))
		return ParseVms(m);
	if (boost::regex_search(line, m, re::irmx))
		return ParseIrmx(m);
	if (boost::regex_search(line, m, re::tandem)) {
#if defined(HAVE_TANDEM)
		SetOSS(NO);
#endif
		return ParseTandem(m);
	}
	return {};
}

// ファイル一覧情報の１行を取得
static std::optional<std::vector<std::variant<FILELIST, std::string>>> GetListLine(int Num) {
	std::ifstream is{ MakeCacheFileName(Num), std::ios::binary };
	if (!is)
		return {};
	std::vector<std::variant<FILELIST, std::string>> lines;
	for (std::string line; getline(is, line);) {
		if (DebugConsole == YES) {
			static const boost::regex re{ R"([^\x20-\x7E]|%)" };
			DoPrintf("%s", replace<char>(line, re, [](auto& m) {
				char percent[4];
				sprintf(percent, "%%%02X", static_cast<unsigned char>(*m[0].begin()));
				return std::string(percent);
			}).c_str());
		}
		line.erase(std::remove(begin(line), end(line), '\r'), end(line));
		std::replace(begin(line), end(line), '\b', ' ');
		if (auto result = Parse(line))
			lines.push_back(*result);
		else
			lines.push_back(line);
	}
	if (CurHost.NameKanjiCode == KANJI_AUTO) {
		CodeDetector cd;
		for (auto& line : lines)
			std::visit([&cd](auto&& arg) {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, FILELIST>)
					if (arg.Node != NODE_NONE && 0 < strlen(arg.File))
						cd.Test(arg.File);
			}, line);
		CurHost.CurNameKanjiCode = cd.result();
	} else
		CurHost.CurNameKanjiCode = CurHost.NameKanjiCode;
	for (auto& line : lines)
		std::visit([](auto&& arg) {
			if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, FILELIST>)
				if (arg.Node != NODE_NONE && 0 < strlen(arg.File)) {
					auto file = ConvertFrom(arg.File, CurHost.CurNameKanjiCode);
					if (auto last = file.back(); last == '/' || last == '\\')
						file.resize(file.size() - 1);
					if (empty(file) || file == "."sv || file == ".."sv)
						arg.Node = NODE_NONE;
					strcpy(arg.File, file.c_str());
				}
		}, line);
	return lines;
}


/*----- サブディレクトリ情報の解析 --------------------------------------------
*
*	Parameter
*		char *Str : ファイル情報（１行）
*		char *Path : 先頭からのパス名
*		char *Dir : ディレクトリ名
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL=ディレクトリ情報でない
*----------------------------------------------------------------------------*/

static int MakeDirPath(char *Str, char *Path, char *Dir)
{
	int Sts;

	Sts = FFFTP_FAIL;
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

				// 文字化け対策
//				ChangeFnameRemote2Local(Dir, FMAX_PATH);

				ReplaceAll(Dir, '\\', '/');
			}
		}
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


// ローカル側のサブディレクトリ以下のファイルをリストに登録する
static bool MakeLocalTree(const char* Path, std::vector<FILELIST>& Base) {
	auto const path = fs::u8path(Path);
	std::vector<WIN32_FIND_DATAW> items;
	if (!FindFile(path / L"*", [&items](auto const& item) { items.push_back(item); return true; }))
		return false;
	for (auto const& data : items)
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && AskFilterStr(u8(data.cFileName).c_str(), NODE_FILE) == YES) {
			FILELIST Pkt{};
			auto const src = (path / data.cFileName).u8string();
			strcpy(Pkt.File, src.c_str());
			ReplaceAll(Pkt.File, '\\', '/');
			Pkt.Node = NODE_FILE;
			Pkt.Size = LONGLONG(data.nFileSizeHigh) << 32 | data.nFileSizeLow;
			if (SYSTEMTIME TmpStime; FileTimeToSystemTime(&data.ftLastWriteTime, &TmpStime)) {
				if (DispTimeSeconds == NO)
					TmpStime.wSecond = 0;
				TmpStime.wMilliseconds = 0;
				SystemTimeToFileTime(&TmpStime, &Pkt.Time);
			}
			AddFileList(Pkt, Base);
		}
	for (auto const& data : items)
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			FILELIST Pkt{};
			auto const src = (path / data.cFileName).u8string();
			strcpy(Pkt.File, src.c_str());
			ReplaceAll(Pkt.File, '\\', '/');
			Pkt.Node = NODE_DIR;
			AddFileList(Pkt, Base);
			if (!MakeLocalTree(src.c_str(), Base))
				return false;
		}
	return true;
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

static void AddFileList(FILELIST const& Pkt, std::vector<FILELIST>& Base) {
	DoPrintf("FileList : NODE=%d : %s", Pkt.Node, Pkt.File);
	/* リストの重複を取り除く */
	if (std::any_of(begin(Base), end(Base), [name = Pkt.File](auto const& f) { return strcmp(name, f.File) == 0; })) {
		DoPrintf(" --> Duplicate!!");
		return;
	}
	Base.emplace_back(Pkt);
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

const FILELIST* SearchFileList(const char* Fname, std::vector<FILELIST> const& Base, int Caps) {
	for (auto p = data(Base), end = data(Base) + size(Base); p != end; ++p)
		if (Caps == COMP_STRICT) {
			if (strcmp(Fname, p->File) == 0)
				return p;
		} else {
			if (_stricmp(Fname, p->File) == 0) {
				if (Caps == COMP_IGNORE)
					return p;
				char Tmp[FMAX_PATH + 1];
				strcpy(Tmp, p->File);
				_strlwr(Tmp);
				if (strcmp(Tmp, p->File) == 0)
					return p;
			}
		}
	return nullptr;
}


// フィルタに指定されたファイル名かどうかを返す
static int AskFilterStr(const char *Fname, int Type) {
	static boost::wregex re{ L";" };
	if (Type != NODE_FILE || strlen(FilterStr) == 0)
		return YES;
	auto const wFname = u8(Fname), wFilterStr = u8(FilterStr);
	for (boost::wsregex_token_iterator it{ begin(wFilterStr), end(wFilterStr), re, -1 }, end; it != end; ++it)
		if (it->matched && CheckFname(wFname, *it))
			return YES;
	return NO;
}


// フィルタを設定する
void SetFilter(int *CancelCheckWork) {
	struct Filter {
		using result_t = bool;
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, FILTER_STR, EM_LIMITTEXT, FILTER_EXT_LEN + 1, 0);
			SetText(hDlg, FILTER_STR, u8(FilterStr));
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				strcpy(FilterStr, u8(GetText(hDlg, FILTER_STR)).c_str());
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case FILTER_NOR:
				strcpy(FilterStr, "*");
				EndDialog(hDlg, true);
				break;
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000021);
				break;
			}
		}
	};
	if (Dialog(GetFtpInst(), filter_dlg, GetMainHwnd(), Filter{})) {
		DispWindowTitle();
		UpdateStatusBar();
		GetLocalDirForWnd();
		GetRemoteDirForWnd(CACHE_LASTREAD, CancelCheckWork);
	}
}
