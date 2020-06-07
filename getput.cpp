/*=============================================================================
*
*							ダウンロード／アップロード
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

/* このソースは一部、WS_FTP Version 93.12.05 のソースを参考にしました。 */
/* スレッドの作成／終了に関して、樋口殿作成のパッチを組み込みました。 */

/*
	一部、高速化のためのコード追加 by H.Shirouzu at 2002/10/02
*/

#include "common.h"
#include <process.h>


#define SET_BUFFER_SIZE

/* Add by H.Shirouzu at 2002/10/02 */
#define BUFSIZE			(32 * 1024)
#define SOCKBUF_SIZE	(256 * 1024)
/* End */

#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
#undef BUFSIZE
#define BUFSIZE			(64 * 1024)	// RWIN値以下で充分な大きさが望ましいと思われる。
#undef SET_BUFFER_SIZE
#endif

#define TIMER_DISPLAY		1		/* 表示更新用タイマのID */
#define DISPLAY_TIMING		500		/* 表示更新時間 0.5秒 */

#define ERR_MSG_LEN			1024


/*===== プロトタイプ =====*/

static void DispTransPacket(TRANSPACKET const& item);
static void EraseTransFileList();
static unsigned __stdcall TransferThread(void *Dummy);
static int MakeNonFullPath(TRANSPACKET& item, char *CurDir);
static int DownloadNonPassive(TRANSPACKET *Pkt, int *CancelCheckWork);
static int DownloadPassive(TRANSPACKET *Pkt, int *CancelCheckWork);
static int DownloadFile(TRANSPACKET *Pkt, SOCKET dSkt, int CreateMode, int *CancelCheckWork);
static void DispDownloadFinishMsg(TRANSPACKET *Pkt, int iRetCode);
static bool DispUpDownErrDialog(int ResID, TRANSPACKET *Pkt);
static int SetDownloadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode, int *CancelCheckWork);
static int DoUpload(SOCKET cSkt, TRANSPACKET& item);
static int UploadNonPassive(TRANSPACKET *Pkt);
static int UploadPassive(TRANSPACKET *Pkt);
static int UploadFile(TRANSPACKET *Pkt, SOCKET dSkt);
static int TermCodeConvAndSend(SOCKET Skt, char *Data, int Size, int Ascii, int *CancelCheckWork);
static void DispUploadFinishMsg(TRANSPACKET *Pkt, int iRetCode);
static int SetUploadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode);
static LRESULT CALLBACK TransDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void DispTransferStatus(HWND hWnd, int End, TRANSPACKET *Pkt);
static void DispTransFileInfo(TRANSPACKET const& item, char *Title, int SkipButton, int Info);
static int GetAdrsAndPort(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max);
static int IsSpecialDevice(const char* Fname);
static int MirrorDelNotify(int Cur, int Notify, TRANSPACKET const& item);
#define SetErrorMsg(...) do { char* errMsg = GetErrMsg(); if (strlen(errMsg) == 0) sprintf(errMsg, __VA_ARGS__); } while(0)
// 同時接続対応
static char* GetErrMsg();

/*===== ローカルなワーク =====*/

// 同時接続対応
//static HANDLE hTransferThread;
static HANDLE hTransferThread[MAX_DATA_CONNECTION];
static int fTransferThreadExit = FALSE;

static HANDLE hRunMutex;				/* 転送スレッド実行ミューテックス */
static HANDLE hListAccMutex;			/* 転送ファイルアクセス用ミューテックス */

static int TransFiles = 0;				/* 転送待ちファイル数 */
static std::forward_list<TRANSPACKET> TransPacketBase;	/* 転送ファイルリスト */
static auto NextTransPacketBase = end(TransPacketBase);

// 同時接続対応
//static int Canceled;		/* 中止フラグ YES/NO */
static int Canceled[MAX_DATA_CONNECTION];		/* 中止フラグ YES/NO */
static int ClearAll;		/* 全て中止フラグ YES/NO */

static int ForceAbort;		/* 転送中止フラグ */
							/* このフラグはスレッドを終了させるときに使う */

// 同時接続対応
//static LONGLONG AllTransSizeNow;	/* 今回の転送で転送したサイズ */
//static time_t TimeStart;	/* 転送開始時間 */
static LONGLONG AllTransSizeNow[MAX_DATA_CONNECTION];	/* 今回の転送で転送したサイズ */
static time_t TimeStart[MAX_DATA_CONNECTION];	/* 転送開始時間 */

static int KeepDlg = NO;	/* 転送中ダイアログを消さないかどうか (YES/NO) */
static int MoveToForeground = NO;		/* ウインドウを前面に移動するかどうか (YES/NO) */

// 同時接続対応
//static char CurDir[FMAX_PATH+1] = { "" };
static char CurDir[MAX_DATA_CONNECTION][FMAX_PATH+1];
// 同時接続対応
//static char ErrMsg[ERR_MSG_LEN+7];
static char ErrMsg[MAX_DATA_CONNECTION+1][ERR_MSG_LEN+7];
static DWORD ErrMsgThreadId[MAX_DATA_CONNECTION+1];
static HANDLE hErrMsgMutex;

// 同時接続対応
static int WaitForMainThread = NO;
// 再転送対応
static int TransferErrorMode = EXIST_OVW;
static int TransferErrorNotify = NO;
// タスクバー進捗表示
static LONGLONG TransferSizeLeft = 0;
static LONGLONG TransferSizeTotal = 0;
static int TransferErrorDisplay = 0;

/*===== 外部参照 =====*/

/* 設定値 */
extern int SaveTimeStamp;
extern int RmEOF;
// extern int TimeOut;
extern int FwallType;
extern int MirUpDelNotify;
extern int MirDownDelNotify;
extern int FolderAttr;
extern int FolderAttrNum;
// 同時接続対応
extern int SendQuit;
// 自動切断対策
extern time_t LastDataConnectionTime;
// ゾーンID設定追加
extern int MarkAsInternet;


/*----- ファイル転送スレッドを起動する ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

int MakeTransferThread(void)
{
	unsigned int dwID;
	int i;

	hListAccMutex = CreateMutexW( NULL, FALSE, NULL );
	hRunMutex = CreateMutexW( NULL, TRUE, NULL );
	// 同時接続対応
	hErrMsgMutex = CreateMutexW( NULL, FALSE, NULL );

	ClearAll = NO;
	ForceAbort = NO;

	fTransferThreadExit = FALSE;
	// 同時接続対応
//	hTransferThread = (HANDLE)_beginthreadex(NULL, 0, TransferThread, 0, 0, &dwID);
//	if (hTransferThread == NULL)
//		return(FFFTP_FAIL); /* XXX */
	for(i = 0; i < MAX_DATA_CONNECTION; i++)
	{
		hTransferThread[i] = (HANDLE)_beginthreadex(NULL, 0, TransferThread, IntToPtr(i), 0, &dwID);
		if(hTransferThread[i] == NULL)
			return FFFTP_FAIL;
	}

	return(FFFTP_SUCCESS);
}


/*----- ファイル転送スレッドを終了する ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void CloseTransferThread(void)
{
	int i;
	// 同時接続対応
//	Canceled = YES;
	for(i = 0; i < MAX_DATA_CONNECTION; i++)
		Canceled[i] = YES;
	ClearAll = YES;
	// 同時接続対応
//	ForceAbort = YES;

	fTransferThreadExit = TRUE;
	// 同時接続対応
//	while(WaitForSingleObject(hTransferThread, 10) == WAIT_TIMEOUT)
//	{
//		BackgrndMessageProc();
//		Canceled = YES;
//	}
//	CloseHandle(hTransferThread);
	for(i = 0; i < MAX_DATA_CONNECTION; i++)
	{
		while(WaitForSingleObject(hTransferThread[i], 10) == WAIT_TIMEOUT)
		{
			BackgrndMessageProc();
			Canceled[i] = YES;
		}
		CloseHandle(hTransferThread[i]);
	}

	ReleaseMutex( hRunMutex );

	CloseHandle( hListAccMutex );
	CloseHandle( hRunMutex );
	// 同時接続対応
	CloseHandle( hErrMsgMutex );
	return;
}


// 同時接続対応
void AbortAllTransfer()
{
	int i;
	while(!empty(TransPacketBase))
	{
		for(i = 0; i < MAX_DATA_CONNECTION; i++)
			Canceled[i] = YES;
		ClearAll = YES;
		if(BackgrndMessageProc() == YES)
			break;
		Sleep(10);
	}
	ClearAll = NO;
}


// 転送するファイル情報をリストに追加する
int AddTmpTransFileList(TRANSPACKET const& item, std::forward_list<TRANSPACKET>& list) {
	auto it = before_end(list);
	list.insert_after(it, item);
	return FFFTP_SUCCESS;
}


// 転送するファイル情報リストから１つの情報を取り除く
int RemoveTmpTransFileListItem(std::forward_list<TRANSPACKET>& list, int Num) {
	for (auto it = list.before_begin(); it != end(list); ++it)
		if (Num-- == 0) {
			list.erase_after(it);
			return FFFTP_SUCCESS;
		}
	return FFFTP_FAIL;
}


/*----- 転送するファイル情報を転送ファイルリストに登録する --------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void AddTransFileList(TRANSPACKET *Pkt)
{
	DispTransPacket(*Pkt);

	// 同時接続対応
//	WaitForSingleObject(hListAccMutex, INFINITE);
	while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
	{
		WaitForMainThread = YES;
		BackgrndMessageProc();
		Sleep(1);
	}

	if (AddTmpTransFileList(*Pkt, TransPacketBase) == FFFTP_SUCCESS) {
		if((strncmp(Pkt->Cmd, "RETR", 4) == 0) ||
		   (strncmp(Pkt->Cmd, "STOR", 4) == 0))
		{
			TransFiles++;
			// タスクバー進捗表示
			TransferSizeLeft += Pkt->Size;
			TransferSizeTotal += Pkt->Size;
			PostMessageW(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
		}
	}
	if (NextTransPacketBase == end(TransPacketBase))
		NextTransPacketBase = before_end(TransPacketBase);
	ReleaseMutex(hListAccMutex);
	// 同時接続対応
	WaitForMainThread = NO;

	return;
}


// バグ対策
void AddNullTransFileList()
{
	TRANSPACKET Pkt{};
	strcpy(Pkt.Cmd, "NULL");
	AddTransFileList(&Pkt);
}


// 転送ファイル情報を転送ファイルリストに追加する
void AppendTransFileList(std::forward_list<TRANSPACKET>&& list) {
	while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
	{
		WaitForMainThread = YES;
		BackgrndMessageProc();
		Sleep(1);
	}

	auto Pkt = before_end(TransPacketBase);
	TransPacketBase.splice_after(Pkt++, list);
	if (NextTransPacketBase == end(TransPacketBase))
		NextTransPacketBase = Pkt;

	while(Pkt != end(TransPacketBase))
	{
		DispTransPacket(*Pkt);

		if((strncmp(Pkt->Cmd, "RETR", 4) == 0) ||
		   (strncmp(Pkt->Cmd, "STOR", 4) == 0))
		{
			TransFiles++;
			// タスクバー進捗表示
			TransferSizeLeft += Pkt->Size;
			TransferSizeTotal += Pkt->Size;
			PostMessageW(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
		}
		++Pkt;
	}

	ReleaseMutex(hListAccMutex);
	// 同時接続対応
	WaitForMainThread = NO;
	return;
}


// 転送ファイル情報を表示する
static void DispTransPacket(TRANSPACKET const& item) {
	if (strncmp(item.Cmd, "RETR", 4) == 0 || strncmp(item.Cmd, "STOR", 4) == 0)
		DoPrintf("TransList Cmd=%s : %s : %s", item.Cmd, item.RemoteFile, item.LocalFile);
	else if (strncmp(item.Cmd, "R-", 2) == 0)
		DoPrintf("TransList Cmd=%s : %s", item.Cmd, item.RemoteFile);
	else if (strncmp(item.Cmd, "L-", 2) == 0)
		DoPrintf("TransList Cmd=%s : %s", item.Cmd, item.LocalFile);
	else if (strncmp(item.Cmd, "MKD", 3) == 0)
		DoPrintf("TransList Cmd=%s : %s", item.Cmd, 0 < strlen(item.LocalFile) ? item.LocalFile : item.RemoteFile);
	else
		DoPrintf("TransList Cmd=%s", item.Cmd);
}


// 転送ファイルリストをクリアする
static void EraseTransFileList() {
	auto NotDel = end(TransPacketBase);
	while (WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT) {
		WaitForMainThread = YES;
		BackgrndMessageProc();
		Sleep(1);
	}
	for (auto New = begin(TransPacketBase); New != end(TransPacketBase); ++New) {
		/* 最後の"BACKCUR"は必要なので消さない */
		if (strcmp(New->Cmd, "BACKCUR") == 0) {
			if (NotDel != end(TransPacketBase))
				strcpy(NotDel->Cmd, "");
			NotDel = New;
		} else
			strcpy(New->Cmd, "");
	}
	// FIXME: TransPacketBaseをここで変更すべきではないはず
	// TransPacketBase.erase_after(TransPacketBase.before_begin(), NotDel);
	NextTransPacketBase = NotDel;
	TransFiles = 0;
	TransferSizeLeft = 0;
	TransferSizeTotal = 0;
	PostMessageW(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
	ReleaseMutex(hListAccMutex);
	WaitForMainThread = NO;
}


/*----- 転送中ダイアログを消さないようにするかどうかを設定 --------------------
*
*	Parameter
*		int Sw : 転送中ダイアログを消さないかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void KeepTransferDialog(int Sw)
{
	KeepDlg = Sw;
	return;
}


/*----- 現在転送中かどうかを返す ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス (YES/NO=転送中ではない)
*----------------------------------------------------------------------------*/

int AskTransferNow(void)
{
	return !empty(TransPacketBase) ? YES : NO;
}


/*----- 転送するファイルの数を返す --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int 転送するファイルの数
*----------------------------------------------------------------------------*/

int AskTransferFileNum(void)
{
	return(TransFiles);
}


/*----- 転送中ウインドウを前面に出す ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void GoForwardTransWindow(void)
{
	MoveToForeground = YES;
	return;
}


/*----- 転送ソケットのカレントディレクトリ情報を初期化 ------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void InitTransCurDir(void)
{
	// 同時接続対応
//	strcpy(CurDir, "");
	int i;
	for(i = 0; i < MAX_DATA_CONNECTION; i++)
		strcpy(CurDir[i], "");
	return;
}


/*----- ファイル転送スレッドのメインループ ------------------------------------
*
*	Parameter
*		void *Dummy : 使わない
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static unsigned __stdcall TransferThread(void *Dummy)
{
	HWND hWndTrans;
	char Tmp[FMAX_PATH+1];
	int CwdSts;
	int GoExit;
//	int Down;
//	int Up;
	static int Down;
	static int Up;
	int DelNotify;
	int ThreadCount;
	SOCKET TrnSkt;
	RECT WndRect;
	int i;
	DWORD LastUsed;
	int LastError;
	int Sts;

	hWndTrans = NULL;
	Down = NO;
	Up = NO;
	GoExit = NO;
	DelNotify = NO;
	// 同時接続対応
	// ソケットは各転送スレッドが管理
	ThreadCount = PtrToInt(Dummy);
	TrnSkt = INVALID_SOCKET;
	LastError = NO;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	while(!empty(TransPacketBase) ||
		  (WaitForSingleObject(hRunMutex, 200) == WAIT_TIMEOUT))
	{
		if(fTransferThreadExit == TRUE)
			break;

		if(WaitForMainThread == YES)
		{
			BackgrndMessageProc();
			Sleep(100);
			continue;
		}

//		WaitForSingleObject(hListAccMutex, INFINITE);
		while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
		{
			BackgrndMessageProc();
			Sleep(1);
		}
		memset(GetErrMsg(), NUL, ERR_MSG_LEN+7);

//		Canceled = NO;
		Canceled[ThreadCount] = NO;

		while (!empty(TransPacketBase) && strcmp(TransPacketBase.front().Cmd, "") == 0) {
			TransPacketBase.pop_front();
			if (empty(TransPacketBase))
				GoExit = YES;
		}
		if(AskReuseCmdSkt() == YES && ThreadCount == 0)
		{
			TrnSkt = AskTrnCtrlSkt();
			// セッションあたりの転送量制限対策
			if(TrnSkt != INVALID_SOCKET && AskErrorReconnect() == YES && LastError == YES)
			{
				ReleaseMutex(hListAccMutex);
				PostMessageW(GetMainHwnd(), WM_RECONNECTSOCKET, 0, 0);
				Sleep(100);
				TrnSkt = INVALID_SOCKET;
//				WaitForSingleObject(hListAccMutex, INFINITE);
				while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
				{
					BackgrndMessageProc();
					Sleep(1);
				}
			}
		}
		else
		{
			// セッションあたりの転送量制限対策
			if(TrnSkt != INVALID_SOCKET && AskErrorReconnect() == YES && LastError == YES)
			{
				ReleaseMutex(hListAccMutex);
				DoQUIT(TrnSkt, &Canceled[ThreadCount]);
				DoClose(TrnSkt);
				TrnSkt = INVALID_SOCKET;
//				WaitForSingleObject(hListAccMutex, INFINITE);
				while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
				{
					BackgrndMessageProc();
					Sleep(1);
				}
			}
			if(!empty(TransPacketBase) && AskConnecting() == YES && ThreadCount < AskMaxThreadCount())
			{
				ReleaseMutex(hListAccMutex);
				if(TrnSkt == INVALID_SOCKET)
					ReConnectTrnSkt(&TrnSkt, &Canceled[ThreadCount]);
				else
					CheckClosedAndReconnectTrnSkt(&TrnSkt, &Canceled[ThreadCount]);
				// 同時ログイン数制限対策
				if(TrnSkt == INVALID_SOCKET)
				{
					// 同時ログイン数制限に引っかかった可能性あり
					// 負荷を下げるために約10秒間待機
					i = 1000;
					while(i > 0)
					{
						BackgrndMessageProc();
						Sleep(10);
						i--;
					}
				}
				LastUsed = timeGetTime();
//				WaitForSingleObject(hListAccMutex, INFINITE);
				while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
				{
					BackgrndMessageProc();
					Sleep(1);
				}
			}
			else
			{
				if(TrnSkt != INVALID_SOCKET)
				{
					// 同時ログイン数制限対策
					// 60秒間使用されなければログアウト
					if(timeGetTime() - LastUsed > 60000 || AskConnecting() == NO || ThreadCount >= AskMaxThreadCount())
					{
						ReleaseMutex(hListAccMutex);
						DoQUIT(TrnSkt, &Canceled[ThreadCount]);
						DoClose(TrnSkt);
						TrnSkt = INVALID_SOCKET;
//						WaitForSingleObject(hListAccMutex, INFINITE);
						while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
						{
							BackgrndMessageProc();
							Sleep(1);
						}
					}
				}
			}
		}
		LastError = NO;
		if (TrnSkt != INVALID_SOCKET && NextTransPacketBase != end(TransPacketBase)) {
			auto Pos = NextTransPacketBase++;
			// ディレクトリ操作は非同期で行わない
//			ReleaseMutex(hListAccMutex);
			if(hWndTrans == NULL)
			{
//				if((strncmp(TransPacketBase->Cmd, "RETR", 4) == 0) ||
//				   (strncmp(TransPacketBase->Cmd, "STOR", 4) == 0) ||
//				   (strncmp(TransPacketBase->Cmd, "MKD", 3) == 0) ||
//				   (strncmp(TransPacketBase->Cmd, "L-", 2) == 0) ||
//				   (strncmp(TransPacketBase->Cmd, "R-", 2) == 0))
				if((strncmp(Pos->Cmd, "RETR", 4) == 0) ||
				   (strncmp(Pos->Cmd, "STOR", 4) == 0) ||
				   (strncmp(Pos->Cmd, "MKD", 3) == 0) ||
				   (strncmp(Pos->Cmd, "L-", 2) == 0) ||
				   (strncmp(Pos->Cmd, "R-", 2) == 0))
				{
					hWndTrans = CreateDialogW(GetFtpInst(), MAKEINTRESOURCEW(transfer_dlg), HWND_DESKTOP, (DLGPROC)TransDlgProc);
					if(MoveToForeground == YES)
						SetForegroundWindow(hWndTrans);
					ShowWindow(hWndTrans, SW_SHOWNOACTIVATE);
					GetWindowRect(hWndTrans, &WndRect);
					SetWindowPos(hWndTrans, NULL, WndRect.left, WndRect.top + (WndRect.bottom - WndRect.top) * ThreadCount - (WndRect.bottom - WndRect.top) * (AskMaxThreadCount() - 1) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				}
			}
//			TransPacketBase->hWndTrans = hWndTrans;
			Pos->hWndTrans = hWndTrans;
			Pos->ctrl_skt = TrnSkt;
			Pos->Abort = ABORT_NONE;
			Pos->ThreadCount = ThreadCount;

			if(hWndTrans != NULL)
			{
				if(MoveToForeground == YES)
				{
					SetForegroundWindow(hWndTrans);
					MoveToForeground = NO;
				}
			}

			if(hWndTrans != NULL)
				SendMessageW(hWndTrans, WM_SET_PACKET, 0, (LPARAM)&*Pos);

			// 中断後に受信バッファに応答が残っていると次のコマンドの応答が正しく処理できない
			RemoveReceivedData(TrnSkt);

			/* ダウンロード */
//			if(strncmp(TransPacketBase->Cmd, "RETR", 4) == 0)
			if(strncmp(Pos->Cmd, "RETR", 4) == 0)
			{
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
//				ReleaseMutex(hListAccMutex);
				/* 不正なパスを検出 */
				if(CheckPathViolation(*Pos) == NO)
				{
					/* フルパスを使わないための処理 */
					if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
					{
//						if(strncmp(TransPacketBase->Cmd, "RETR-S", 6) == 0)
						if(strncmp(Pos->Cmd, "RETR-S", 6) == 0)
						{
							/* サイズと日付を取得 */
//							DoSIZE(TransPacketBase->RemoteFile, &TransPacketBase->Size);
//							DoMDTM(TransPacketBase->RemoteFile, &TransPacketBase->Time);
//							strcpy(TransPacketBase->Cmd, "RETR ");
							DoSIZE(TrnSkt, Pos->RemoteFile, &Pos->Size, &Canceled[Pos->ThreadCount]);
							DoMDTM(TrnSkt, Pos->RemoteFile, &Pos->Time, &Canceled[Pos->ThreadCount]);
							strcpy(Pos->Cmd, "RETR ");
						}

						Down = YES;
						// ミラーリング設定追加
						if(Pos->NoTransfer == NO)
						{
							Sts = DoDownload(TrnSkt, *Pos, NO, &Canceled[Pos->ThreadCount]) / 100;
							if(Sts != FTP_COMPLETE)
								LastError = YES;
							// ゾーンID設定追加
							if(MarkAsInternet == YES && IsZoneIDLoaded() == YES)
								MarkFileAsDownloadedFromInternet(Pos->LocalFile);
						}

						if (SaveTimeStamp == YES && (Pos->Time.dwLowDateTime != 0 || Pos->Time.dwHighDateTime != 0))
							if (auto handle = CreateFileW(fs::u8path(Pos->LocalFile).c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0); handle != INVALID_HANDLE_VALUE) {
								SetFileTime(handle, &Pos->Time, &Pos->Time, &Pos->Time);
								CloseHandle(handle);
							}
					}
				}
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
				ReleaseMutex(hListAccMutex);
			}
			/* アップロード */
//			else if(strncmp(TransPacketBase->Cmd, "STOR", 4) == 0)
			else if(strncmp(Pos->Cmd, "STOR", 4) == 0)
			{
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
//				ReleaseMutex(hListAccMutex);
				/* フルパスを使わないための処理 */
				if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
				{
					Up = YES;
					// ミラーリング設定追加
					if(Pos->NoTransfer == NO)
					{
						Sts = DoUpload(TrnSkt, *Pos) / 100;
						if(Sts != FTP_COMPLETE)
							LastError = YES;
					}

					// ホスト側の日時設定
					/* ファイルのタイムスタンプを合わせる */
					if((SaveTimeStamp == YES) &&
					   ((Pos->Time.dwLowDateTime != 0) || (Pos->Time.dwHighDateTime != 0)))
					{
						DoMFMT(TrnSkt, Pos->RemoteFile, &Pos->Time, &Canceled[Pos->ThreadCount]);
					}
				}
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
				ReleaseMutex(hListAccMutex);
			}
			/* フォルダ作成（ローカルまたはホスト） */
//			else if(strncmp(TransPacketBase->Cmd, "MKD", 3) == 0)
			else if(strncmp(Pos->Cmd, "MKD", 3) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN078, FALSE, YES);

//				if(strlen(TransPacketBase->RemoteFile) > 0)
				if(strlen(Pos->RemoteFile) > 0)
				{
					/* フルパスを使わないための処理 */
					CwdSts = FTP_COMPLETE;

//					strcpy(Tmp, TransPacketBase->RemoteFile);
					strcpy(Tmp, Pos->RemoteFile);
//					if(ProcForNonFullpath(Tmp, CurDir, hWndTrans, 1) == FFFTP_FAIL)
					if(ProcForNonFullpath(TrnSkt, Tmp, CurDir[Pos->ThreadCount], hWndTrans, &Canceled[Pos->ThreadCount]) == FFFTP_FAIL)
					{
						ClearAll = YES;
						CwdSts = FTP_ERROR;
					}

					if(CwdSts == FTP_COMPLETE)
					{
						Up = YES;
//						CommandProcTrn(NULL, "MKD %s", Tmp);
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "MKD %s", Tmp);
						/* すでにフォルダがある場合もあるので、 */
						/* ここではエラーチェックはしない */

					if(FolderAttr)
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s %03d %s", AskHostChmodCmd().c_str(), FolderAttrNum, Tmp);
					}
				}
//				else if(strlen(TransPacketBase->LocalFile) > 0)
				else if(strlen(Pos->LocalFile) > 0)
				{
					Down = YES;
//					DoLocalMKD(TransPacketBase->LocalFile);
					DoLocalMKD(Pos->LocalFile);
				}
				ReleaseMutex(hListAccMutex);
			}
			/* ディレクトリ作成（常にホスト側） */
//			else if(strncmp(TransPacketBase->Cmd, "R-MKD", 5) == 0)
			else if(strncmp(Pos->Cmd, "R-MKD", 5) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN079, FALSE, YES);

				/* フルパスを使わないための処理 */
				if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
				{
					Up = YES;
//					CommandProcTrn(NULL, "%s%s", TransPacketBase->Cmd+2, TransPacketBase->RemoteFile);
					CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s%s", Pos->Cmd+2, Pos->RemoteFile);

					if(FolderAttr)
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s %03d %s", AskHostChmodCmd().c_str(), FolderAttrNum, Pos->RemoteFile);
				}
				ReleaseMutex(hListAccMutex);
			}
			/* ディレクトリ削除（常にホスト側） */
//			else if(strncmp(TransPacketBase->Cmd, "R-RMD", 5) == 0)
			else if(strncmp(Pos->Cmd, "R-RMD", 5) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN080, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					/* フルパスを使わないための処理 */
					if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
					{
						Up = YES;
//						CommandProcTrn(NULL, "%s%s", TransPacketBase->Cmd+2, TransPacketBase->RemoteFile);
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s%s", Pos->Cmd+2, Pos->RemoteFile);
					}
				}
				ReleaseMutex(hListAccMutex);
			}
			/* ファイル削除（常にホスト側） */
//			else if(strncmp(TransPacketBase->Cmd, "R-DELE", 6) == 0)
			else if(strncmp(Pos->Cmd, "R-DELE", 6) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN081, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					/* フルパスを使わないための処理 */
					if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
					{
						Up = YES;
//						CommandProcTrn(NULL, "%s%s", TransPacketBase->Cmd+2, TransPacketBase->RemoteFile);
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s%s", Pos->Cmd+2, Pos->RemoteFile);
					}
				}
				ReleaseMutex(hListAccMutex);
			}
			/* ディレクトリ作成（常にローカル側） */
//			else if(strncmp(TransPacketBase->Cmd, "L-MKD", 5) == 0)
			else if(strncmp(Pos->Cmd, "L-MKD", 5) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN082, FALSE, YES);

				Down = YES;
//				DoLocalMKD(TransPacketBase->LocalFile);
				DoLocalMKD(Pos->LocalFile);
				ReleaseMutex(hListAccMutex);
			}
			/* ディレクトリ削除（常にローカル側） */
//			else if(strncmp(TransPacketBase->Cmd, "L-RMD", 5) == 0)
			else if(strncmp(Pos->Cmd, "L-RMD", 5) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN083, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					Down = YES;
//					DoLocalRMD(TransPacketBase->LocalFile);
					DoLocalRMD(Pos->LocalFile);
				}
				ReleaseMutex(hListAccMutex);
			}
			/* ファイル削除（常にローカル側） */
//			else if(strncmp(TransPacketBase->Cmd, "L-DELE", 6) == 0)
			else if(strncmp(Pos->Cmd, "L-DELE", 6) == 0)
			{
				DispTransFileInfo(*Pos, MSGJPN084, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					Down = YES;
//					DoLocalDELE(TransPacketBase->LocalFile);
					DoLocalDELE(Pos->LocalFile);
				}
				ReleaseMutex(hListAccMutex);
			}
			/* カレントディレクトリを設定 */
//			else if(strcmp(TransPacketBase->Cmd, "SETCUR") == 0)
			else if(strcmp(Pos->Cmd, "SETCUR") == 0)
			{
//				if(AskShareProh() == YES)
				if(AskReuseCmdSkt() == NO || AskShareProh() == YES)
				{
//					if(strcmp(CurDir, TransPacketBase->RemoteFile) != 0)
					if(strcmp(CurDir[Pos->ThreadCount], Pos->RemoteFile) != 0)
					{
//						if(CommandProcTrn(NULL, "CWD %s", TransPacketBase->RemoteFile)/100 != FTP_COMPLETE)
						if(CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "CWD %s", Pos->RemoteFile)/100 != FTP_COMPLETE)
						{
							DispCWDerror(hWndTrans);
							ClearAll = YES;
						}
					}
				}
//				strcpy(CurDir, TransPacketBase->RemoteFile);
				strcpy(CurDir[Pos->ThreadCount], Pos->RemoteFile);
				ReleaseMutex(hListAccMutex);
			}
			/* カレントディレクトリを戻す */
//			else if(strcmp(TransPacketBase->Cmd, "BACKCUR") == 0)
			else if(strcmp(Pos->Cmd, "BACKCUR") == 0)
			{
//				if(AskShareProh() == NO)
				if(AskReuseCmdSkt() == YES && AskShareProh() == NO)
				{
//					if(strcmp(CurDir, TransPacketBase->RemoteFile) != 0)
//						CommandProcTrn(NULL, "CWD %s", TransPacketBase->RemoteFile);
//					strcpy(CurDir, TransPacketBase->RemoteFile);
					if(strcmp(CurDir[Pos->ThreadCount], Pos->RemoteFile) != 0)
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "CWD %s", Pos->RemoteFile);
					strcpy(CurDir[Pos->ThreadCount], Pos->RemoteFile);
				}
				ReleaseMutex(hListAccMutex);
			}
			/* 自動終了のための通知 */
//			else if(strcmp(TransPacketBase->Cmd, "GOQUIT") == 0)
//			else if(strcmp(Pos->Cmd, "GOQUIT") == 0)
//			{
//				ReleaseMutex(hListAccMutex);
//				GoExit = YES;
//			}
			// バグ対策
			else if(strcmp(Pos->Cmd, "NULL") == 0)
			{
				Sleep(0);
				Sleep(100);
				ReleaseMutex(hListAccMutex);
			}
			else
				ReleaseMutex(hListAccMutex);

			/*===== １つの処理終わり =====*/

			if(ForceAbort == NO)
			{
//				WaitForSingleObject(hListAccMutex, INFINITE);
				while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
				{
					BackgrndMessageProc();
					Sleep(1);
				}
				if(ClearAll == YES)
//					EraseTransFileList();
				{
					for(i = 0; i < MAX_DATA_CONNECTION; i++)
						Canceled[i] = YES;
					if (Pos != end(TransPacketBase))
						strcpy(Pos->Cmd, "");
					Pos = end(TransPacketBase);
					EraseTransFileList();
					GoExit = YES;
				}
				else
				{
//					if((strncmp(TransPacketBase->Cmd, "RETR", 4) == 0) ||
//					   (strncmp(TransPacketBase->Cmd, "STOR", 4) == 0))
					if((strncmp(Pos->Cmd, "RETR", 4) == 0) ||
					   (strncmp(Pos->Cmd, "STOR", 4) == 0) ||
					   (strncmp(Pos->Cmd, "STOU", 4) == 0))
					{
//						TransFiles--;
						if(TransFiles > 0)
							TransFiles--;
						// タスクバー進捗表示
						if(TransferSizeLeft > 0)
							TransferSizeLeft -= Pos->Size;
						if(TransferSizeLeft < 0)
							TransferSizeLeft = 0;
						if(TransFiles == 0)
							TransferSizeTotal = 0;
						PostMessageW(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
					}
				}
//				ClearAll = NO;
				ReleaseMutex(hListAccMutex);

				if(BackgrndMessageProc() == YES)
				{
					WaitForSingleObject(hListAccMutex, INFINITE);
					EraseTransFileList();
					ReleaseMutex(hListAccMutex);
				}
			}
			if(hWndTrans != NULL)
				SendMessageW(hWndTrans, WM_SET_PACKET, 0, 0);
			if (Pos != end(TransPacketBase))
				strcpy(Pos->Cmd, "");
			LastUsed = timeGetTime();
		}
//		else
		else if (empty(TransPacketBase))
		{
			ClearAll = NO;
			DelNotify = NO;

			if(GoExit == YES)
			{
				Sound::Transferred.Play();
				if(AskAutoExit() == NO)
				{
					if(Down == YES)
						PostMessageW(GetMainHwnd(), WM_REFRESH_LOCAL_FLG, 0, 0);
					if(Up == YES)
						PostMessageW(GetMainHwnd(), WM_REFRESH_REMOTE_FLG, 0, 0);
				}
				Down = NO;
				Up = NO;
				PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_AUTO_EXIT, 0), 0);
				GoExit = NO;
			}

			ReleaseMutex(hListAccMutex);
			if(KeepDlg == NO)
			{
				if(hWndTrans != NULL)
				{
					DestroyWindow(hWndTrans);
					hWndTrans = NULL;
				}
			}
			BackgrndMessageProc();
//			Sleep(1);
			Sleep(100);

			// 再転送対応
			TransferErrorMode = AskTransferErrorMode();
			TransferErrorNotify = AskTransferErrorNotify();
		}
		else
		{
			ReleaseMutex(hListAccMutex);
			if(hWndTrans != NULL)
			{
				DestroyWindow(hWndTrans);
				hWndTrans = NULL;
			}
			BackgrndMessageProc();
			if(ThreadCount < AskMaxThreadCount())
				Sleep(1);
			else
				Sleep(100);
		}
	}
	if(AskReuseCmdSkt() == NO || ThreadCount > 0)
	{
		if(TrnSkt != INVALID_SOCKET)
		{
			SendData(TrnSkt, "QUIT\r\n", 6, 0, &Canceled[ThreadCount]);
			DoClose(TrnSkt);
		}
	}
	return 0;
}


/*----- フルパスを使わないファイルアクセスの準備 ------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送パケット
*		char *Cur : カレントディレクトリ
*		char *Tmp : 作業用エリア
*
*	Return Value
*		int ステータス(FFFTP_SUCCESS/FFFTP_FAIL)
*
*	Note
*		フルパスを使わない時は、
*			このモジュール内で CWD を行ない、
*			Pkt->RemoteFile にファイル名のみ残す。（パス名は消す）
*----------------------------------------------------------------------------*/

static int MakeNonFullPath(TRANSPACKET& item, char* Cur) {
	auto result = ProcForNonFullpath(item.ctrl_skt, item.RemoteFile, Cur, item.hWndTrans, &Canceled[item.ThreadCount]);
	if (result == FFFTP_FAIL)
		ClearAll = YES;
	return result;
}


/*----- ダウンロードを行なう --------------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		TRANSPACKET *Pkt : 転送ファイル情報
*		int DirList : ディレクトリリストのダウンロード(YES/NO)
*
*	Return Value
*		int 応答コード
*
*	Note
*		このモジュールは、ファイル一覧の取得などを行なう際にメインのスレッド
*		からも呼ばれる。メインのスレッドから呼ばれる時は Pkt->hWndTrans == NULL。
*----------------------------------------------------------------------------*/

int DoDownload(SOCKET cSkt, TRANSPACKET& item, int DirList, int *CancelCheckWork)
{
	int iRetCode;
	char Reply[ERR_MSG_LEN+7];

	item.ctrl_skt = cSkt;
	if(IsSpecialDevice(GetFileName(item.LocalFile)) == YES)
	{
		iRetCode = 500;
		SetTaskMsg(MSGJPN085, GetFileName(item.LocalFile));
	}
	else if(item.Mode != EXIST_IGNORE)
	{
		if(item.Type == TYPE_I)
			item.KanjiCode = KANJI_NOCNV;

		iRetCode = command(item.ctrl_skt, Reply, CancelCheckWork, "TYPE %c", item.Type);
		if(iRetCode/100 < FTP_RETRY)
		{
			if(item.hWndTrans != NULL)
			{
				// 同時接続対応
//				AllTransSizeNow = 0;
				AllTransSizeNow[item.ThreadCount] = 0;

				if(DirList == NO)
					DispTransFileInfo(item, MSGJPN086, TRUE, YES);
				else
					DispTransFileInfo(item, MSGJPN087, FALSE, NO);
			}

			if(BackgrndMessageProc() == NO)
			{
				if(AskPasvMode() != YES)
					iRetCode = DownloadNonPassive(&item, CancelCheckWork);
				else
					iRetCode = DownloadPassive(&item, CancelCheckWork);
			}
			else
				iRetCode = 500;
		}
		else
			SetErrorMsg(Reply);
		// エラーによってはダイアログが表示されない場合があるバグ対策
		DispDownloadFinishMsg(&item, iRetCode);
	}
	else
	{
		DispTransFileInfo(item, MSGJPN088, TRUE, YES);
		SetTaskMsg(MSGJPN089, item.RemoteFile);
		iRetCode = 200;
	}
	return(iRetCode);
}


/*----- 通常モードでファイルをダウンロード ------------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int DownloadNonPassive(TRANSPACKET *Pkt, int *CancelCheckWork)
{
	int iRetCode;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	SOCKET listen_socket = INVALID_SOCKET; // data listen socket
	// 念のため
//	char Buf[1024];
	char Buf[FMAX_PATH+1024];
	int CreateMode;
	// UPnP対応
	int Port;
	char Reply[ERR_MSG_LEN+7];

	if((listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, CancelCheckWork)) != INVALID_SOCKET)
	{
		if(SetDownloadResume(Pkt, Pkt->Mode, Pkt->ExistSize, &CreateMode, CancelCheckWork) == YES)
		{
			sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
			iRetCode = command(Pkt->ctrl_skt, Reply, CancelCheckWork, "%s", Buf);
			if(iRetCode/100 == FTP_PRELIM)
			{
				if (AskHostFireWall() == YES && (FwallType == FWALL_SOCKS4 || FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER)) {
					if (!SocksReceiveReply(listen_socket, CancelCheckWork))
						data_socket = listen_socket;
					else
						listen_socket = DoClose(listen_socket);
				} else {
					sockaddr_storage sa;
					int salen = sizeof(sockaddr_storage);
					data_socket = do_accept(listen_socket, reinterpret_cast<sockaddr*>(&sa), &salen);

					if(shutdown(listen_socket, 1) != 0)
						ReportWSError("shutdown listen", WSAGetLastError());
					// UPnP対応
					if(IsUPnPLoaded() == YES)
					{
						if(GetAsyncTableDataMapPort(listen_socket, &Port) == YES)
							RemovePortMapping(Port);
					}
					listen_socket = DoClose(listen_socket);

					if(data_socket == INVALID_SOCKET)
					{
						SetErrorMsg(MSGJPN280);
						ReportWSError("accept", WSAGetLastError());
						iRetCode = 500;
					}
					else
						DoPrintf("Skt=%zu : accept from %s", data_socket, u8(AddressPortToString(&sa, salen)).c_str());
				}

				if(data_socket != INVALID_SOCKET)
				{
					// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
					ReleaseMutex(hListAccMutex);
					// FTPS対応
//					iRetCode = DownloadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
					if(IsSSLAttached(Pkt->ctrl_skt))
					{
						if(AttachSSL(data_socket, Pkt->ctrl_skt, CancelCheckWork, NULL))
							iRetCode = DownloadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
						else
							iRetCode = 500;
					}
					else
						iRetCode = DownloadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
//					data_socket = DoClose(data_socket);
				}
			}
			else
			{
				SetErrorMsg(Reply);
				SetTaskMsg(MSGJPN090);
				// UPnP対応
				if(IsUPnPLoaded() == YES)
				{
					if(GetAsyncTableDataMapPort(listen_socket, &Port) == YES)
						RemovePortMapping(Port);
				}
				listen_socket = DoClose(listen_socket);
				iRetCode = 500;
			}
		}
		else
		// バグ修正
//			iRetCode = 500;
		{
			// UPnP対応
			if(IsUPnPLoaded() == YES)
			{
				if(GetAsyncTableDataMapPort(listen_socket, &Port) == YES)
					RemovePortMapping(Port);
			}
			listen_socket = DoClose(listen_socket);
			iRetCode = 500;
		}
	}
	else
	{
		iRetCode = 500;
		SetErrorMsg(MSGJPN279);
	}
	// エラーによってはダイアログが表示されない場合があるバグ対策
//	DispDownloadFinishMsg(Pkt, iRetCode);

	return(iRetCode);
}


/*----- Passiveモードでファイルをダウンロード ---------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int DownloadPassive(TRANSPACKET *Pkt, int *CancelCheckWork)
{
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	// 念のため
//	char Buf[1024];
	char Buf[FMAX_PATH+1024];
	int CreateMode;
	// IPv6対応
//	char Adrs[20];
	char Adrs[40];
	int Port;
	int Flg;
	char Reply[ERR_MSG_LEN+7];

	int iRetCode = command(Pkt->ctrl_skt, Buf, CancelCheckWork, AskCurNetType() == NTYPE_IPV6 ? "EPSV" : "PASV");
	if(iRetCode/100 == FTP_COMPLETE)
	{
		// IPv6対応
//		if(GetAdrsAndPort(Buf, Adrs, &Port, 19) == FFFTP_SUCCESS)
		if(GetAdrsAndPort(Pkt->ctrl_skt, Buf, Adrs, &Port, 39) == FFFTP_SUCCESS)
		{
			if((data_socket = connectsock(Adrs, Port, MSGJPN091, CancelCheckWork)) != INVALID_SOCKET)
			{
				// 変数が未初期化のバグ修正
				Flg = 1;
				if(setsockopt(data_socket, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
					ReportWSError("setsockopt", WSAGetLastError());

				if(SetDownloadResume(Pkt, Pkt->Mode, Pkt->ExistSize, &CreateMode, CancelCheckWork) == YES)
				{
					sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
					iRetCode = command(Pkt->ctrl_skt, Reply, CancelCheckWork, "%s", Buf);
					if(iRetCode/100 == FTP_PRELIM)
					{
						// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
						ReleaseMutex(hListAccMutex);
						// FTPS対応
//						iRetCode = DownloadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
						if(IsSSLAttached(Pkt->ctrl_skt))
						{
							if(AttachSSL(data_socket, Pkt->ctrl_skt, CancelCheckWork, NULL))
								iRetCode = DownloadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
							else
								iRetCode = 500;
						}
						else
							iRetCode = DownloadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
//						data_socket = DoClose(data_socket);
					}
					else
					{
						SetErrorMsg(Reply);
						SetTaskMsg(MSGJPN092);
						data_socket = DoClose(data_socket);
						iRetCode = 500;
					}
				}
				else
					iRetCode = 500;
			}
			else
				iRetCode = 500;
		}
		else
		{
			SetErrorMsg(MSGJPN093);
			SetTaskMsg(MSGJPN093);
			iRetCode = 500;
		}
	}
	else
		SetErrorMsg(Buf);

	// エラーによってはダイアログが表示されない場合があるバグ対策
//	DispDownloadFinishMsg(Pkt, iRetCode);

	return(iRetCode);
}


/*----- ダウンロードの実行 ----------------------------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		SOCKET dSkt : データソケット
*		int CreateMode : ファイル作成モード (CREATE_ALWAYS/OPEN_ALWAYS)
*
*	Return Value
*		int 応答コード
*
*	Note
*		転送の経過表示は
*			ダイアログを出す(Pkt->hWndTrans!=NULL)場合、インターバルタイマで経過を表示する
*			ダイアログを出さない場合、このルーチンからDispDownloadSize()を呼ぶ
*----------------------------------------------------------------------------*/

static int DownloadFile(TRANSPACKET *Pkt, SOCKET dSkt, int CreateMode, int *CancelCheckWork) {
#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
	int buf_size = 0;
	setsockopt(dSkt, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size));
#elif defined(SET_BUFFER_SIZE)
	for (int buf_size = SOCKBUF_SIZE; buf_size > 0; buf_size /= 2)
		if (setsockopt(dSkt, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size)) == 0)
			break;
#endif

	char buf[BUFSIZE];
	Pkt->Abort = ABORT_NONE;
	if (auto attr = GetFileAttributesW(fs::u8path(Pkt->LocalFile).c_str()); attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
		if (Message<IDS_MSGJPN086>(IDS_REMOVE_READONLY, MB_YESNO) == IDYES)
			SetFileAttributesW(fs::u8path(Pkt->LocalFile).c_str(), attr & ~FILE_ATTRIBUTE_READONLY);

	auto opened = false;
	if (std::ofstream os{ fs::u8path(Pkt->LocalFile), std::ios::binary | (CreateMode == OPEN_ALWAYS ? std::ios::ate : std::ios::trunc) }) {
		opened = true;

		if (Pkt->hWndTrans != NULL) {
			TimeStart[Pkt->ThreadCount] = time(NULL);
			SetTimer(Pkt->hWndTrans, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
		}

		CodeConverter cc{ Pkt->KanjiCode, Pkt->KanjiCodeDesired, Pkt->KanaCnv != NO };

		/*===== ファイルを受信するループ =====*/
		int read = 0;
		while (Pkt->Abort == ABORT_NONE && ForceAbort == NO) {
			if (int timeout; (read = do_recv(dSkt, buf, BUFSIZE, 0, &timeout, CancelCheckWork)) <= 0) {
				if (timeout == YES) {
					SetErrorMsg(MSGJPN094);
					SetTaskMsg(MSGJPN094);
					if (Pkt->hWndTrans != NULL)
						ClearAll = YES;
					if (Pkt->Abort == ABORT_NONE)
						Pkt->Abort = ABORT_ERROR;
				} else if (read == SOCKET_ERROR) {
					if (Pkt->Abort == ABORT_NONE)
						Pkt->Abort = ABORT_ERROR;
				}
				break;
			}

			if (auto converted = cc.Convert({ buf, (size_t)read }); !os.write(data(converted), size(converted)))
				Pkt->Abort = ABORT_DISKFULL;

			Pkt->ExistSize += read;
			if (Pkt->hWndTrans != NULL)
				AllTransSizeNow[Pkt->ThreadCount] += read;
			else {
				/* 転送ダイアログを出さない時の経過表示 */
				DispDownloadSize(Pkt->ExistSize);
			}

			if (BackgrndMessageProc() == YES)
				ForceAbort = YES;
		}

		/* グラフ表示を更新 */
		if (Pkt->hWndTrans != NULL) {
			KillTimer(Pkt->hWndTrans, TIMER_DISPLAY);
			DispTransferStatus(Pkt->hWndTrans, YES, Pkt);
			TimeStart[Pkt->ThreadCount] = time(NULL) - TimeStart[Pkt->ThreadCount] + 1;
		} else {
			/* 転送ダイアログを出さない時の経過表示を消す */
			DispDownloadSize(-1);
		}

		if (read == SOCKET_ERROR)
			ReportWSError("recv", WSAGetLastError());
	} else {
		SetErrorMsg(MSGJPN095, Pkt->LocalFile);
		SetTaskMsg(MSGJPN095, Pkt->LocalFile);
		Pkt->Abort = ABORT_ERROR;
	}

	if (shutdown(dSkt, 1) != 0)
		ReportWSError("shutdown", WSAGetLastError());
	LastDataConnectionTime = time(NULL);
	DoClose(dSkt);

	/* Abortをホストに伝える */
	if (ForceAbort == NO && Pkt->Abort != ABORT_NONE && opened) {
		SendData(Pkt->ctrl_skt, "\xFF\xF4\xFF", 3, MSG_OOB, CancelCheckWork);	/* MSG_OOBに注意 */
		SendData(Pkt->ctrl_skt, "\xF2", 1, 0, CancelCheckWork);
		command(Pkt->ctrl_skt, NULL, CancelCheckWork, "ABOR");
	}

	char tmp[ONELINE_BUF_SIZE];
	auto code = ReadReplyMessage(Pkt->ctrl_skt, buf, 1024, CancelCheckWork, tmp);
	if (Pkt->Abort == ABORT_DISKFULL) {
		SetErrorMsg(MSGJPN096);
		SetTaskMsg(MSGJPN096);
	}
	if (code / 100 >= FTP_RETRY)
		SetErrorMsg(buf);
	if (Pkt->Abort != ABORT_NONE)
		code = 500;
	return code;
}


/*----- ダウンロード終了／中止時のメッセージを表示 ----------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		int iRetCode : 応答コード
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispDownloadFinishMsg(TRANSPACKET *Pkt, int iRetCode)
{
	char Fname[FMAX_PATH+1];

	// 同時接続対応
	ReleaseMutex(hListAccMutex);
	if(ForceAbort == NO)
	{
		if((iRetCode/100) >= FTP_CONTINUE)
		{
			strcpy(Fname, Pkt->RemoteFile);

#if defined(HAVE_OPENVMS)
			/* OpenVMSの場合、空ディレクトリへ移動すると550 File not foundになって
			 * エラーダイアログやエラーメッセージが出るので何もしない */
			if (AskHostType() == HTYPE_VMS)
				return;
#endif
#if defined(HAVE_TANDEM)
			/* HP Nonstop Server の場合、ファイルのない subvol へ移動すると550 File not found
			 * になるが問題ないのでエラーダイアログやエラーメッセージを出さないため */
			if (AskHostType() == HTYPE_TANDEM)
				return;
#endif

			// MLSD対応
//			if((strncmp(Pkt->Cmd, "NLST", 4) == 0) || (strncmp(Pkt->Cmd, "LIST", 4) == 0))
			if((strncmp(Pkt->Cmd, "NLST", 4) == 0) || (strncmp(Pkt->Cmd, "LIST", 4) == 0) || (strncmp(Pkt->Cmd, "MLSD", 4) == 0))
			{
				SetTaskMsg(MSGJPN097);
				strcpy(Fname, MSGJPN098);
			}
			// 同時接続対応
//			else if((Pkt->hWndTrans != NULL) && (TimeStart != 0))
//				SetTaskMsg(MSGJPN099, TimeStart, Pkt->ExistSize/TimeStart);
			else if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				SetTaskMsg(MSGJPN099, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
			else
				SetTaskMsg(MSGJPN100);

			if(Pkt->Abort != ABORT_USER)
			{
				// 全て中止を選択後にダイアログが表示されるバグ対策
//				if(DispUpDownErrDialog(downerr_dlg, Pkt->hWndTrans, Fname) == NO)
				// 再転送対応
//				if(Canceled[Pkt->ThreadCount] == NO && ClearAll == NO && DispUpDownErrDialog(downerr_dlg, Pkt->hWndTrans, Fname) == NO)
//					ClearAll = YES;
				if(Canceled[Pkt->ThreadCount] == NO && ClearAll == NO)
				{
					if(strncmp(Pkt->Cmd, "RETR", 4) == 0 || strncmp(Pkt->Cmd, "STOR", 4) == 0)
					{
						// タスクバー進捗表示
						TransferErrorDisplay++;
						if(TransferErrorNotify == YES && !DispUpDownErrDialog(downerr_dlg, Pkt))
							ClearAll = YES;
						else
						{
							Pkt->Mode = TransferErrorMode;
							AddTransFileList(Pkt);
						}
						// タスクバー進捗表示
						TransferErrorDisplay--;
					}
				}
			}
		}
		else
		{
			// MLSD対応
//			if((strncmp(Pkt->Cmd, "NLST", 4) == 0) || (strncmp(Pkt->Cmd, "LIST", 4) == 0))
			if((strncmp(Pkt->Cmd, "NLST", 4) == 0) || (strncmp(Pkt->Cmd, "LIST", 4) == 0) || (strncmp(Pkt->Cmd, "MLSD", 4) == 0))
				SetTaskMsg(MSGJPN101, Pkt->ExistSize);
			// 同時接続対応
//			else if((Pkt->hWndTrans != NULL) && (TimeStart != 0))
//				SetTaskMsg(MSGJPN102, TimeStart, Pkt->ExistSize/TimeStart);
			else if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				// "0 B/S"と表示されるバグを修正
				// 原因は%dにあたる部分に64ビット値が渡されているため
//				SetTaskMsg(MSGJPN102, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
				SetTaskMsg(MSGJPN102, (LONG)TimeStart[Pkt->ThreadCount], (LONG)(Pkt->ExistSize/TimeStart[Pkt->ThreadCount]));
			else
				SetTaskMsg(MSGJPN103, Pkt->ExistSize);
		}
	}
	return;
}


// ダウンロード／アップロードエラーのダイアログを表示
static bool DispUpDownErrDialog(int ResID, TRANSPACKET *Pkt) {
	struct Data {
		using result_t = bool;
		using DownExistButton = RadioButton<DOWN_EXIST_OVW, DOWN_EXIST_RESUME, DOWN_EXIST_IGNORE>;
		TRANSPACKET* Pkt;
		Data(TRANSPACKET* Pkt) : Pkt{ Pkt } {}
		INT_PTR OnInit(HWND hDlg) {
			SetText(hDlg, UPDOWN_ERR_FNAME, u8(Pkt->RemoteFile));
			SetText(hDlg, UPDOWN_ERR_MSG, u8(GetErrMsg()));
			if (Pkt->Type == TYPE_A || Pkt->ExistSize <= 0)
				EnableWindow(GetDlgItem(hDlg, DOWN_EXIST_RESUME), FALSE);
			DownExistButton::Set(hDlg, TransferErrorMode);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK_ALL:
				TransferErrorNotify = NO;
				[[fallthrough]];
			case IDOK:
				TransferErrorMode = DownExistButton::Get(hDlg);
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			}
		}
	};
	Sound::Error.Play();
	return Dialog(GetFtpInst(), ResID, Pkt->hWndTrans, Data{ Pkt });
}


/*----- ダウンロードのリジュームの準備を行う ----------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		iont ProcMode : 処理モード(EXIST_xxx)
*		LONGLONG Size : ロード済みのファイルのサイズ
*		int *Mode : ファイル作成モード (CREATE_xxxx)
*
*	Return Value
*		int 転送を行うかどうか(YES/NO=このファイルを中止/NO_ALL=全て中止)
*
*	Note
*		Pkt->ExistSizeのセットを行なう
*----------------------------------------------------------------------------*/

static int SetDownloadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode, int *CancelCheckWork) {
	struct Data {
		using result_t = int;
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, YES);
				break;
			case IDCANCEL:
				EndDialog(hDlg, NO);
				break;
			case RESUME_CANCEL_ALL:
				EndDialog(hDlg, NO_ALL);
				break;
			}
		}
	};
	int Com = YES;
	Pkt->ExistSize = 0;
	*Mode = CREATE_ALWAYS;
	if (ProcMode == EXIST_RESUME) {
		char Reply[ERR_MSG_LEN + 7];
		auto iRetCode = command(Pkt->ctrl_skt, Reply, CancelCheckWork, "REST %lld", Size);
		if (iRetCode / 100 < FTP_RETRY) {
			/* リジューム */
			if (Pkt->hWndTrans != NULL)
				Pkt->ExistSize = Size;
			*Mode = OPEN_ALWAYS;
		} else {
			Com = Dialog(GetFtpInst(), noresume_dlg, Pkt->hWndTrans, Data{});
			if (Com != YES) {
				if (Com == NO_ALL)		/* 全て中止 */
					ClearAll = YES;
				Pkt->Abort = ABORT_USER;
			}
		}
	}
	return Com;
}


/*----- アップロードを行なう --------------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int DoUpload(SOCKET cSkt, TRANSPACKET& item)
{
	int iRetCode;
	char Reply[ERR_MSG_LEN+7];

	item.ctrl_skt = cSkt;

	if(item.Mode != EXIST_IGNORE)
	{
		if (std::wifstream{ fs::u8path(item.LocalFile) }) {
			if(item.Type == TYPE_I)
				item.KanjiCode = KANJI_NOCNV;

			iRetCode = command(item.ctrl_skt, Reply, &Canceled[item.ThreadCount], "TYPE %c", item.Type);
			if(iRetCode/100 < FTP_RETRY)
			{
				if(item.Mode == EXIST_UNIQUE)
					strcpy(item.Cmd, "STOU ");

				if(item.hWndTrans != NULL)
					DispTransFileInfo(item, MSGJPN104, TRUE, YES);

				if(BackgrndMessageProc() == NO)
				{
					if(AskPasvMode() != YES)
						iRetCode = UploadNonPassive(&item);
					else
						iRetCode = UploadPassive(&item);
				}
				else
					iRetCode = 500;
			}
			else
				SetErrorMsg(Reply);

			/* 属性変更 */
			if((item.Attr != -1) && ((iRetCode/100) == FTP_COMPLETE))
				command(item.ctrl_skt, Reply, &Canceled[item.ThreadCount], "%s %03X %s", AskHostChmodCmd().c_str(), item.Attr, item.RemoteFile);
		}
		else
		{
			SetErrorMsg(MSGJPN105, item.LocalFile);
			SetTaskMsg(MSGJPN105, item.LocalFile);
			iRetCode = 500;
			item.Abort = ABORT_ERROR;
		}
		DispUploadFinishMsg(&item, iRetCode);
	}
	else
	{
		DispTransFileInfo(item, MSGJPN106, TRUE, YES);
		SetTaskMsg(MSGJPN107, item.LocalFile);
		iRetCode = 200;
	}
	return(iRetCode);
}


/*----- 通常モードでファイルをアップロード ------------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int UploadNonPassive(TRANSPACKET *Pkt)
{
	int iRetCode;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	SOCKET listen_socket = INVALID_SOCKET; // data listen socket
	// 念のため
//	char Buf[1024];
	char Buf[FMAX_PATH+1024];
	// UPnP対応
	int Port;
	int Resume;
	char Reply[ERR_MSG_LEN+7];

	// 同時接続対応
//	if((listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, &Canceled)) != INVALID_SOCKET)
	if((listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, &Canceled[Pkt->ThreadCount])) != INVALID_SOCKET)
	{
		SetUploadResume(Pkt, Pkt->Mode, Pkt->ExistSize, &Resume);
		if(Resume == NO)
#if defined(HAVE_TANDEM)
			if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO && Pkt->Type != TYPE_A) {
				if( Pkt->PriExt == DEF_PRIEXT && Pkt->SecExt == DEF_SECEXT && Pkt->MaxExt == DEF_MAXEXT) {
					// EXTENTがデフォルトのときはコードのみ
					sprintf(Buf, "%s%s,%d", Pkt->Cmd, Pkt->RemoteFile, Pkt->FileCode);
				} else {
					sprintf(Buf, "%s%s,%d,%d,%d,%d", Pkt->Cmd, Pkt->RemoteFile, Pkt->FileCode, Pkt->PriExt, Pkt->SecExt, Pkt->MaxExt);
				}
			} else
#endif
			sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
		else
			sprintf(Buf, "%s%s", "APPE ", Pkt->RemoteFile);

		// 同時接続対応
//		iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled, "%s", Buf);
		iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled[Pkt->ThreadCount], "%s", Buf);
		if((iRetCode/100) == FTP_PRELIM)
		{
			// STOUの応答を処理
			// 応答の形式に規格が無くファイル名を取得できないため属性変更を無効化
			if(Pkt->Mode == EXIST_UNIQUE)
				Pkt->Attr = -1;
			if (AskHostFireWall() == YES && (FwallType == FWALL_SOCKS4 || FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER)) {
				if (SocksReceiveReply(listen_socket, &Canceled[Pkt->ThreadCount]))
					data_socket = listen_socket;
				else
					listen_socket = DoClose(listen_socket);
			} else {
				sockaddr_storage sa;
				int salen = sizeof(sockaddr_storage);
				data_socket = do_accept(listen_socket, reinterpret_cast<sockaddr*>(&sa), &salen);

				if(shutdown(listen_socket, 1) != 0)
					ReportWSError("shutdown listen", WSAGetLastError());
				// UPnP対応
				if(IsUPnPLoaded() == YES)
				{
					if(GetAsyncTableDataMapPort(listen_socket, &Port) == YES)
						RemovePortMapping(Port);
				}
				listen_socket = DoClose(listen_socket);

				if(data_socket == INVALID_SOCKET)
				{
					SetErrorMsg(MSGJPN280);
					ReportWSError("accept", WSAGetLastError());
					iRetCode = 500;
				}
				else
					DoPrintf("Skt=%zu : accept from %s", data_socket, u8(AddressPortToString(&sa, salen)).c_str());
			}

			if(data_socket != INVALID_SOCKET)
			{
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
				ReleaseMutex(hListAccMutex);
				// FTPS対応
//				iRetCode = UploadFile(Pkt, data_socket);
				if(IsSSLAttached(Pkt->ctrl_skt))
				{
					if(AttachSSL(data_socket, Pkt->ctrl_skt, &Canceled[Pkt->ThreadCount], NULL))
						iRetCode = UploadFile(Pkt, data_socket);
					else
						iRetCode = 500;
				}
				else
					iRetCode = UploadFile(Pkt, data_socket);
				data_socket = DoClose(data_socket);
			}
		}
		else
		{
			SetErrorMsg(Reply);
			SetTaskMsg(MSGJPN108);
			// UPnP対応
			if(IsUPnPLoaded() == YES)
			{
				if(GetAsyncTableDataMapPort(listen_socket, &Port) == YES)
					RemovePortMapping(Port);
			}
			listen_socket = DoClose(listen_socket);
			iRetCode = 500;
		}
	}
	else
	{
		SetErrorMsg(MSGJPN279);
		iRetCode = 500;
	}
	// エラーによってはダイアログが表示されない場合があるバグ対策
//	DispUploadFinishMsg(Pkt, iRetCode);

	return(iRetCode);
}


/*----- Passiveモードでファイルをアップロード ---------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int UploadPassive(TRANSPACKET *Pkt)
{
	int iRetCode;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	// 念のため
//	char Buf[1024];
	char Buf[FMAX_PATH+1024];
	// IPv6対応
//	char Adrs[20];
	char Adrs[40];
	int Port;
	int Flg;
	int Resume;
	char Reply[ERR_MSG_LEN+7];

	// 同時接続対応
//	iRetCode = command(Pkt->ctrl_skt, Buf, &Canceled, "PASV");
	// IPv6対応
//	iRetCode = command(Pkt->ctrl_skt, Buf, &Canceled[Pkt->ThreadCount], "PASV");
	switch(AskCurNetType())
	{
	case NTYPE_IPV4:
		iRetCode = command(Pkt->ctrl_skt, Buf, &Canceled[Pkt->ThreadCount], "PASV");
		break;
	case NTYPE_IPV6:
		iRetCode = command(Pkt->ctrl_skt, Buf, &Canceled[Pkt->ThreadCount], "EPSV");
		break;
	}
	if(iRetCode/100 == FTP_COMPLETE)
	{
		// IPv6対応
//		if(GetAdrsAndPort(Buf, Adrs, &Port, 19) == FFFTP_SUCCESS)
		if(GetAdrsAndPort(Pkt->ctrl_skt, Buf, Adrs, &Port, 39) == FFFTP_SUCCESS)
		{
			// 同時接続対応
//			if((data_socket = connectsock(Adrs, Port, MSGJPN109, &Canceled)) != INVALID_SOCKET)
			if((data_socket = connectsock(Adrs, Port, MSGJPN109, &Canceled[Pkt->ThreadCount])) != INVALID_SOCKET)
			{
				// 変数が未初期化のバグ修正
				Flg = 1;
				if(setsockopt(data_socket, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&Flg, sizeof(Flg)) == SOCKET_ERROR)
					ReportWSError("setsockopt", WSAGetLastError());

				SetUploadResume(Pkt, Pkt->Mode, Pkt->ExistSize, &Resume);
				if(Resume == NO)
#if defined(HAVE_TANDEM)
					if(AskHostType() == HTYPE_TANDEM && AskOSS() == NO && Pkt->Type != TYPE_A) {
						if( Pkt->PriExt == DEF_PRIEXT && Pkt->SecExt == DEF_SECEXT && Pkt->MaxExt == DEF_MAXEXT) {
							// EXTENTがデフォルトのときはコードのみ
							sprintf(Buf, "%s%s,%d", Pkt->Cmd, Pkt->RemoteFile, Pkt->FileCode);
						} else {
							sprintf(Buf, "%s%s,%d,%d,%d,%d", Pkt->Cmd, Pkt->RemoteFile, Pkt->FileCode, Pkt->PriExt, Pkt->SecExt, Pkt->MaxExt);
						}
					} else
#endif
					sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
				else
					sprintf(Buf, "%s%s", "APPE ", Pkt->RemoteFile);

				// 同時接続対応
//				iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled, "%s", Buf);
				iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled[Pkt->ThreadCount], "%s", Buf);
				if(iRetCode/100 == FTP_PRELIM)
				{
					// STOUの応答を処理
					// 応答の形式に規格が無くファイル名を取得できないため属性変更を無効化
					if(Pkt->Mode == EXIST_UNIQUE)
						Pkt->Attr = -1;
					// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
					ReleaseMutex(hListAccMutex);
					// FTPS対応
//					iRetCode = UploadFile(Pkt, data_socket);
					if(IsSSLAttached(Pkt->ctrl_skt))
					{
						if(AttachSSL(data_socket, Pkt->ctrl_skt, &Canceled[Pkt->ThreadCount], NULL))
							iRetCode = UploadFile(Pkt, data_socket);
						else
							iRetCode = 500;
					}
					else
						iRetCode = UploadFile(Pkt, data_socket);

					data_socket = DoClose(data_socket);
				}
				else
				{
					SetErrorMsg(Reply);
					SetTaskMsg(MSGJPN110);
					data_socket = DoClose(data_socket);
					iRetCode = 500;
				}
			}
			else
			{
				SetErrorMsg(MSGJPN281);
				iRetCode = 500;
			}
		}
		else
		{
			SetErrorMsg(Buf);
			SetTaskMsg(MSGJPN111);
			iRetCode = 500;
		}
	}
	else
		SetErrorMsg(Buf);

	// エラーによってはダイアログが表示されない場合があるバグ対策
//	DispUploadFinishMsg(Pkt, iRetCode);

	return(iRetCode);
}


/*----- アップロードの実行 ----------------------------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		SOCKET dSkt : データソケット
*
*	Return Value
*		int 応答コード
*
*	Note
*		転送の経過表示は、インターバルタイマで経過を表示する
*		転送ダイアログを出さないでアップロードすることはない
*----------------------------------------------------------------------------*/

static int UploadFile(TRANSPACKET *Pkt, SOCKET dSkt) {
#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
	int buf_size = 0;
	setsockopt(dSkt, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size));
#elif defined(SET_BUFFER_SIZE)
	for (int buf_size = SOCKBUF_SIZE; buf_size > 0; buf_size /= 2)
		if (setsockopt(dSkt, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size)) == 0)
			break;
#endif

	char buf[BUFSIZE];
	Pkt->Abort = ABORT_NONE;
	if (std::ifstream is{ fs::u8path(Pkt->LocalFile), std::ios::binary }) {
		if (Pkt->hWndTrans != NULL) {
			Pkt->Size = is.seekg(0, std::ios::end).tellg();
			is.seekg(Pkt->ExistSize, std::ios::beg);

			AllTransSizeNow[Pkt->ThreadCount] = 0;
			TimeStart[Pkt->ThreadCount] = time(NULL);
			SetTimer(Pkt->hWndTrans, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
		}

		CodeConverter cc{ Pkt->KanjiCodeDesired, Pkt->KanjiCode, Pkt->KanaCnv != NO };

		/*===== ファイルを送信するループ =====*/
		auto eof = false;
		for (std::streamsize read; Pkt->Abort == ABORT_NONE && ForceAbort == NO && !eof && (read = is.read(buf, std::size(buf)).gcount()) != 0;) {
			/* EOF除去 */
			if (RmEOF == YES && Pkt->Type == TYPE_A)
				if (auto pos = std::find(buf, buf + read, '\x1A'); pos != buf + read) {
					eof = true;
					read = pos - buf;
				}

			auto converted = cc.Convert({ buf, (std::string_view::size_type)read });
			if (TermCodeConvAndSend(dSkt, data(converted), size_as<DWORD>(converted), Pkt->Type, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
				Pkt->Abort = ABORT_ERROR;

			Pkt->ExistSize += read;
			if (Pkt->hWndTrans != NULL)
				AllTransSizeNow[Pkt->ThreadCount] += read;

			if (BackgrndMessageProc() == YES)
				ForceAbort = YES;
		}

		/* グラフ表示を更新 */
		if (Pkt->hWndTrans != NULL) {
			KillTimer(Pkt->hWndTrans, TIMER_DISPLAY);
			DispTransferStatus(Pkt->hWndTrans, YES, Pkt);
			TimeStart[Pkt->ThreadCount] = time(NULL) - TimeStart[Pkt->ThreadCount] + 1;
		}
	} else {
		SetErrorMsg(MSGJPN112, Pkt->LocalFile);
		SetTaskMsg(MSGJPN112, Pkt->LocalFile);
		Pkt->Abort = ABORT_ERROR;
	}

	LastDataConnectionTime = time(NULL);
	if (shutdown(dSkt, 1) != 0)
		ReportWSError("shutdown", WSAGetLastError());

	char tmp[ONELINE_BUF_SIZE];
	auto code = ReadReplyMessage(Pkt->ctrl_skt, buf, 1024, &Canceled[Pkt->ThreadCount], tmp);
	if (code / 100 >= FTP_RETRY)
		SetErrorMsg(buf);
	if (Pkt->Abort != ABORT_NONE)
		code = 500;
	return code;
}


// バッファの内容を改行コード変換して送信
static int TermCodeConvAndSend(SOCKET Skt, char *Data, int Size, int Ascii, int *CancelCheckWork) {
	// CR-LF以外の改行コードを変換しないモードはここへ追加
	if (Ascii == TYPE_A) {
		auto encoded = ToCRLF({ Data, (size_t)Size });
		return SendData(Skt, data(encoded), size_as<int>(encoded), 0, CancelCheckWork);
	}
	return SendData(Skt, Data, Size, 0, CancelCheckWork);
}


/*----- アップロード終了／中止時のメッセージを表示 ----------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		int iRetCode : 応答コード
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispUploadFinishMsg(TRANSPACKET *Pkt, int iRetCode)
{
	// 同時接続対応
	ReleaseMutex(hListAccMutex);
	if(ForceAbort == NO)
	{
		if((iRetCode/100) >= FTP_CONTINUE)
		{
			// 同時接続対応
//			if((Pkt->hWndTrans != NULL) && (TimeStart != 0))
//				SetTaskMsg(MSGJPN113, TimeStart, Pkt->ExistSize/TimeStart);
			if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				SetTaskMsg(MSGJPN113, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
			else
				SetTaskMsg(MSGJPN114);

			if(Pkt->Abort != ABORT_USER)
			{
				// 全て中止を選択後にダイアログが表示されるバグ対策
//				if(DispUpDownErrDialog(uperr_dlg, Pkt->hWndTrans, Pkt->LocalFile) == NO)
				// 再転送対応
//				if(Canceled[Pkt->ThreadCount] == NO && ClearAll == NO && DispUpDownErrDialog(uperr_dlg, Pkt->hWndTrans, Pkt->LocalFile) == NO)
//					ClearAll = YES;
				if(Canceled[Pkt->ThreadCount] == NO && ClearAll == NO)
				{
					if(strncmp(Pkt->Cmd, "RETR", 4) == 0 || strncmp(Pkt->Cmd, "STOR", 4) == 0)
					{
						// タスクバー進捗表示
						TransferErrorDisplay++;
						if(TransferErrorNotify == YES && !DispUpDownErrDialog(uperr_dlg, Pkt))
							ClearAll = YES;
						else
						{
							Pkt->Mode = TransferErrorMode;
							AddTransFileList(Pkt);
						}
						// タスクバー進捗表示
						TransferErrorDisplay--;
					}
				}
			}
		}
		else
		{
			// 同時接続対応
//			if((Pkt->hWndTrans != NULL) && (TimeStart != 0))
//				SetTaskMsg(MSGJPN115, TimeStart, Pkt->ExistSize/TimeStart);
			if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				// "0 B/S"と表示されるバグを修正
				// 原因は%dにあたる部分に64ビット値が渡されているため
//				SetTaskMsg(MSGJPN115, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
				SetTaskMsg(MSGJPN115, (LONG)TimeStart[Pkt->ThreadCount], (LONG)(Pkt->ExistSize/TimeStart[Pkt->ThreadCount]));
			else
				SetTaskMsg(MSGJPN116);
		}
	}
	return;
}


/*----- アップロードのリジュームの準備を行う ----------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		iont ProcMode : 処理モード(EXIST_xxx)
*		LONGLONG Size : ホストにあるファイルのサイズ
*		int *Mode : リジュームを行うかどうか (YES/NO)
*
*	Return Value
*		int ステータス = YES
*
*	Note
*		Pkt->ExistSizeのセットを行なう
*----------------------------------------------------------------------------*/

static int SetUploadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode)
{
	Pkt->ExistSize = 0;
	*Mode = NO;
	if(ProcMode == EXIST_RESUME)
	{
		if(Pkt->hWndTrans != NULL)
		{
			Pkt->ExistSize = Size;
			*Mode = YES;
		}
	}
	return(YES);
}


/*----- 転送中ダイアログボックスのコールバック --------------------------------
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

static LRESULT CALLBACK TransDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	RECT RectDlg;
	RECT RectPar;
	HMENU hMenu;
	// 同時接続対応
//	static TRANSPACKET *Pkt;
	TRANSPACKET *Pkt;
	int i;

	switch(Msg)
	{
		case WM_INITDIALOG :
			GetWindowRect(hDlg, &RectDlg);
			RectDlg.right -= RectDlg.left;
			RectDlg.bottom -= RectDlg.top;
			GetWindowRect(GetMainHwnd(), &RectPar);
			MoveWindow(hDlg,
				((RectPar.right + RectPar.left) / 2) - (RectDlg.right / 2),
				((RectPar.bottom + RectPar.top) / 2) - (RectDlg.bottom / 2),
				RectDlg.right,
				RectDlg.bottom,
				FALSE);

			hMenu = GetSystemMenu(hDlg, FALSE);
			EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED);
			break;

		case WM_COMMAND :
			switch(LOWORD(wParam))
			{
				case TRANS_STOP_NEXT :
					ClearAll = YES;
					break;

				case TRANS_STOP_ALL :
					ClearAll = YES;
					for(i = 0; i < MAX_DATA_CONNECTION; i++)
						Canceled[i] = YES;
					/* ここに break はない */

				case IDCANCEL :
					if(!(Pkt = (TRANSPACKET*)GetWindowLongPtrW(hDlg, GWLP_USERDATA)))
						break;
					Pkt->Abort = ABORT_USER;
//					Canceled = YES;
					Canceled[Pkt->ThreadCount] = YES;
					break;
			}
			break;

		case WM_TIMER :
			if(wParam == TIMER_DISPLAY)
			{
				if(MoveToForeground == YES)
					SetForegroundWindow(hDlg);
				MoveToForeground = NO;
				KillTimer(hDlg, TIMER_DISPLAY);
				if(!(Pkt = (TRANSPACKET*)GetWindowLongPtrW(hDlg, GWLP_USERDATA)))
					break;
				DispTransferStatus(hDlg, NO, Pkt);
				SetTimer(hDlg, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
			}
			break;

		case WM_SET_PACKET :
			SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
			break;
	}
	return(FALSE);
}


// 転送ステータスを表示
static void DispTransferStatus(HWND hWnd, int End, TRANSPACKET* Pkt) {
	if (!hWnd)
		return;
	{
		static boost::wregex re{ LR"(^(?:\(\d+\))?)" };
		auto title = GetText(hWnd);
		title = boost::regex_replace(title, re, L'(' + std::to_wstring(AskTransferFileNum()) + L')');
		SetText(hWnd, title);
	}
	{
		std::wstring status;
		if (Pkt->Abort != ABORT_NONE)
			status = GetString(IDS_CANCELLED);
		else if (End != NO)
			status = GetString(IDS_FINISHED);
		else {
			std::wstringstream ss;
			ss.imbue(std::locale{ "" });
			ss << std::fixed << std::setprecision(1);

			if (Pkt->Size <= 0)
				ss << Pkt->ExistSize << L"B ";
			else if (Pkt->Size < 1024)
				ss << Pkt->ExistSize << L"B / " << Pkt->Size << L"B ";
			else if (Pkt->Size < 1024 * 1024)
				ss << Pkt->ExistSize / 1024. << L"KB / " << Pkt->Size / 1024. << L"KB ";
			else if (Pkt->Size < 1024 * 1024 * 1024)
				ss << Pkt->ExistSize / 1024 / 1024. << L"MB / " << Pkt->Size / 1024 / 1024. << L"MB ";
			else
				ss << Pkt->ExistSize / 1024 / 1024 / 1024. << L"GB / " << Pkt->Size / 1024 / 1024 / 1024. << L"GB ";

			auto TotalLap = time(nullptr) - TimeStart[Pkt->ThreadCount] + 1;
			auto Bps = TotalLap != 0 ? AllTransSizeNow[Pkt->ThreadCount] / TotalLap : 0;
			if (Bps < 1024)
				ss << L"( " << Bps << L"B/s )";
			else if (Bps < 1024 * 1024)
				ss << L"( " << Bps / 1024. << L"KB/s )";
			else if (Bps < 1024 * 1024 * 1024)
				ss << L"( " << Bps / 1024 / 1024. << L"MB/s )";
			else
				ss << L"( " << Bps / 1024 / 1024 / 1024. << L"GB/s )";

			if (auto Transed = Pkt->Size - Pkt->ExistSize; 0 < Bps && 0 < Pkt->Size && 0 <= Transed)
				ss << L"  " << Transed / Bps / 60 << L':' << std::setfill(L'0') << std::setw(2) << Transed / Bps % 60;
			else
				ss << L"  ??:??";

			status = ss.str();
		}
		SetText(hWnd, TRANS_STATUS, status);
	}
	{
		int percent = Pkt->Size <= 0 ? 0 : Pkt->Size < std::numeric_limits<decltype(Pkt->Size)>::max() / 100 ? (int)(Pkt->ExistSize * 100 / Pkt->Size) : (int)((Pkt->ExistSize / 1024) * 100 / (Pkt->Size / 1024));
		SendDlgItemMessageW(hWnd, TRANS_TIME_BAR, PBM_SETPOS, percent, 0);
	}
}


/*----- 転送するファイルの情報を表示 ------------------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		char *Title : ウインドウのタイトル
*		int SkipButton : 「このファイルを中止」ボタンの有無 (TRUE/FALSE)
*		int Info : ファイル情報を表示するかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispTransFileInfo(TRANSPACKET const& item, char* Title, int SkipButton, int Info) {
	char Tmp[40];

	if (item.hWndTrans != NULL) {
		EnableWindow(GetDlgItem(item.hWndTrans, IDCANCEL), SkipButton);

		sprintf(Tmp, "(%d)%s", AskTransferFileNum(), Title);
		SetText(item.hWndTrans, u8(Tmp));
		SetText(item.hWndTrans, TRANS_STATUS, L"");

		SendDlgItemMessageW(item.hWndTrans, TRANS_TIME_BAR, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendDlgItemMessageW(item.hWndTrans, TRANS_TIME_BAR, PBM_SETSTEP, 1, 0);
		SendDlgItemMessageW(item.hWndTrans, TRANS_TIME_BAR, PBM_SETPOS, 0, 0);

		if (Info == YES) {
			DispStaticText(GetDlgItem(item.hWndTrans, TRANS_REMOTE), item.RemoteFile);
			DispStaticText(GetDlgItem(item.hWndTrans, TRANS_LOCAL), item.LocalFile);

			if (item.Type == TYPE_I)
				SetText(item.hWndTrans, TRANS_MODE, GetString(IDS_MSGJPN119));
			else if (item.Type == TYPE_A)
				SetText(item.hWndTrans, TRANS_MODE, GetString(IDS_MSGJPN120));

			if (item.KanjiCode == KANJI_NOCNV)
				SetText(item.hWndTrans, TRANS_KANJI, GetString(IDS_MSGJPN121));
			else if (item.KanjiCode == KANJI_SJIS)
				SetText(item.hWndTrans, TRANS_KANJI, GetString(IDS_MSGJPN305));
			else if (item.KanjiCode == KANJI_JIS)
				SetText(item.hWndTrans, TRANS_KANJI, GetString(IDS_MSGJPN122));
			else if (item.KanjiCode == KANJI_EUC)
				SetText(item.hWndTrans, TRANS_KANJI, GetString(IDS_MSGJPN123));
			else if (item.KanjiCode == KANJI_UTF8N)
				SetText(item.hWndTrans, TRANS_KANJI, GetString(IDS_MSGJPN306));
			else if (item.KanjiCode == KANJI_UTF8BOM)
				SetText(item.hWndTrans, TRANS_KANJI, GetString(IDS_MSGJPN329));
		} else {
			SetText(item.hWndTrans, TRANS_REMOTE, L"");
			SetText(item.hWndTrans, TRANS_LOCAL, L"");
			SetText(item.hWndTrans, TRANS_MODE, L"");
			SetText(item.hWndTrans, TRANS_KANJI, L"");
		}
	}
}


/*----- PASVコマンドの戻り値からアドレスとポート番号を抽出 --------------------
*
*	Parameter
*		char *Str : PASVコマンドのリプライ
*		char *Adrs : アドレスのコピー先 ("www.xxx.yyy.zzz")
*		int *Port : ポート番号をセットするワーク
*		int Max : アドレス文字列の最大長
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

static int GetAdrsAndPort(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max) {
	if (AskCurNetType() == NTYPE_IPV4) {
		// RFC1123 4.1.2.6  PASV Command: RFC-959 Section 4.1.2
		// Therefore, a User-FTP program that interprets the PASV reply must scan the reply for the first digit of the host and port numbers.
		// コンマではなくドットを返すホストがある
		static boost::regex re{ R"((\d+[,.]\d+[,.]\d+[,.]\d+)[,.](\d+)[,.](\d+))" };
		if (boost::cmatch m; boost::regex_search(Str, m, re)) {
			int p1, p2;
			std::from_chars(m[2].first, m[2].second, p1);
			std::from_chars(m[3].first, m[3].second, p2);
			*Port = p1 << 8 | p2;

			// ホスト側の設定ミス対策
			if (AskNoPasvAdrs() == NO) {
				auto addr = m[1].str();
				std::replace(begin(addr), end(addr), ',', '.');
				strcpy(Adrs, addr.c_str());
				return FFFTP_SUCCESS;
			}
		} else
			return FFFTP_FAIL;
	} else {
		// RFC2428 3.  The EPSV Command
		// The text returned in response to the EPSV command MUST be:
		// <text indicating server is entering extended passive mode> (<d><d><d><tcp-port><d>)
		static boost::regex re{ R"(\(([\x21-\xFE])\1\1(\d+)\1\))" };
		if (boost::cmatch m; boost::regex_search(Str, m, re))
			std::from_chars(m[2].first, m[2].second, *Port);
		else
			return FFFTP_FAIL;
	}
	std::variant<sockaddr_storage, std::tuple<std::string, int>> target;
	GetAsyncTableData(Skt, target);
	std::visit([Adrs](auto addr) {
		using type = std::decay_t<decltype(addr)>;
		if constexpr (std::is_same_v<type, sockaddr_storage>) {
			strcpy(Adrs, u8(AddressToString(addr)).c_str());
		} else if constexpr (std::is_same_v<type, std::tuple<std::string, int>>) {
			auto [host, port] = addr;
			strcpy(Adrs, host.c_str());
		} else
			static_assert(false_v<type>);
	}, target);
	return FFFTP_SUCCESS;
}


/*----- Windowsのスペシャルデバイスかどうかを返す -----------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス (YES/NO)
*----------------------------------------------------------------------------*/

static int IsSpecialDevice(const char* Fname)
{
	int Sts;

	Sts = NO;
	// 比較が不完全なバグ修正
//	if((_stricmp(Fname, "CON") == 0) ||
//	   (_stricmp(Fname, "PRN") == 0) ||
//	   (_stricmp(Fname, "AUX") == 0) ||
//	   (_strnicmp(Fname, "CON.", 4) == 0) ||
//	   (_strnicmp(Fname, "PRN.", 4) == 0) ||
//	   (_strnicmp(Fname, "AUX.", 4) == 0))
//	{
//		Sts = YES;
//	}
	if(_strnicmp(Fname, "AUX", 3) == 0|| _strnicmp(Fname, "CON", 3) == 0 || _strnicmp(Fname, "NUL", 3) == 0 || _strnicmp(Fname, "PRN", 3) == 0)
	{
		if(*(Fname + 3) == '\0' || *(Fname + 3) == '.')
			Sts = YES;
	}
	else if(_strnicmp(Fname, "COM", 3) == 0 || _strnicmp(Fname, "LPT", 3) == 0)
	{
		if(isdigit(*(Fname + 3)) != 0)
		{
			if(*(Fname + 4) == '\0' || *(Fname + 4) == '.')
				Sts = YES;
		}
	}
	return(Sts);
}


// ミラーリングでのファイル削除確認
static int MirrorDelNotify(int Cur, int Notify, TRANSPACKET const& item) {
	struct Data {
		using result_t = int;
		int Cur;
		TRANSPACKET const& item;
		Data(int Cur, TRANSPACKET const& item) : Cur{ Cur }, item{ item } {}
		INT_PTR OnInit(HWND hDlg) {
			if (Cur == WIN_LOCAL) {
				SetText(hDlg, GetString(IDS_MSGJPN124));
				SetText(hDlg, DELETE_TEXT, u8(item.LocalFile));
			} else {
				SetText(hDlg, GetString(IDS_MSGJPN125));
				SetText(hDlg, DELETE_TEXT, u8(item.RemoteFile));
			}
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, YES);
				break;
			case DELETE_NO:
				EndDialog(hDlg, NO);
				break;
			case DELETE_ALL:
				EndDialog(hDlg, YES_ALL);
				break;
			case IDCANCEL:
				ClearAll = YES;
				EndDialog(hDlg, NO_ALL);
				break;
			}
		}
	};
	if (Cur == WIN_LOCAL && MirDownDelNotify == NO || Cur == WIN_REMOTE && MirUpDelNotify == NO)
		Notify = YES_ALL;
	if (Notify != YES_ALL)
		Notify = Dialog(GetFtpInst(), delete_dlg, item.hWndTrans ? item.hWndTrans : GetMainHwnd(), Data{ Cur, item });
	return Notify;
}


// ダウンロード時の不正なパスをチェック
//   YES=不正なパス/NO=問題ないパス
int CheckPathViolation(TRANSPACKET const& item) {
	static boost::wregex re{ LR"((?:^|[/\\])\.\.[/\\])" };
	if (auto const name = u8(item.RemoteFile); boost::regex_search(name, re)) {
		auto const format = GetString(IDS_INVALID_PATH);
		auto const length = _scwprintf(format.c_str(), name.c_str());
		std::wstring message(length, L'\0');
		swprintf(data(message), length, format.c_str(), name.c_str());
		MessageBoxW(GetMainHwnd(), message.c_str(), GetString(IDS_MSGJPN086).c_str(), MB_OK);
		return YES;
	}
	return NO;
}


// 同時接続対応
static char* GetErrMsg()
{
	char* r;
	DWORD ThreadId;
	int i;
	r = NULL;
	WaitForSingleObject(hErrMsgMutex, INFINITE);
	ThreadId = GetCurrentThreadId();
	i = 0;
	while(i < MAX_DATA_CONNECTION + 1)
	{
		if(ErrMsgThreadId[i] == ThreadId)
		{
			r = ErrMsg[i];
			break;
		}
		i++;
	}
	if(!r)
	{
		i = 0;
		while(i < MAX_DATA_CONNECTION + 1)
		{
			if(ErrMsgThreadId[i] == 0)
			{
				ErrMsgThreadId[i] = ThreadId;
				r = ErrMsg[i];
				break;
			}
			i++;
		}
	}
	ReleaseMutex(hErrMsgMutex);
	return r;
}

// タスクバー進捗表示
LONGLONG AskTransferSizeLeft(void)
{
	return(TransferSizeLeft);
}

LONGLONG AskTransferSizeTotal(void)
{
	return(TransferSizeTotal);
}

int AskTransferErrorDisplay(void)
{
	return(TransferErrorDisplay);
}

// ゾーンID設定
static ComPtr<IZoneIdentifier> zoneIdentifier;
static ComPtr<IPersistFile> persistFile;

int LoadZoneID() {
	if (IsMainThread())
		if (CoCreateInstance(CLSID_PersistentZoneIdentifier, NULL, CLSCTX_ALL, IID_PPV_ARGS(&zoneIdentifier)) == S_OK)
			if (zoneIdentifier->SetId(URLZONE_INTERNET) == S_OK)
				if (zoneIdentifier.As(&persistFile) == S_OK)
					return FFFTP_SUCCESS;
	return FFFTP_FAIL;
}

void FreeZoneID() {
	if (IsMainThread()) {
		persistFile.Reset();
		zoneIdentifier.Reset();
	}
}

int IsZoneIDLoaded() {
	return zoneIdentifier && persistFile ? YES : NO;
}

int MarkFileAsDownloadedFromInternet(char* Fname) {
	MARKFILEASDOWNLOADEDFROMINTERNETDATA Data;
	int result = FFFTP_FAIL;
	if (IsMainThread()) {
		if (persistFile->Save(_bstr_t{ u8(Fname).c_str() }, FALSE) == S_OK)
			return FFFTP_SUCCESS;
	} else {
		if (Data.h = CreateEventW(NULL, TRUE, FALSE, NULL)) {
			Data.Fname = Fname;
			if (PostMessageW(GetMainHwnd(), WM_MARKFILEASDOWNLOADEDFROMINTERNET, 0, (LPARAM)&Data))
				if (WaitForSingleObject(Data.h, INFINITE) == WAIT_OBJECT_0)
					result = Data.r;
			CloseHandle(Data.h);
		}
	}
	return result;
}
