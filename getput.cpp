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
static int MakeNonFullPath(TRANSPACKET& item, std::wstring& CurDir);
static int DownloadFile(TRANSPACKET *Pkt, std::shared_ptr<SocketContext> dSkt, int CreateMode, int *CancelCheckWork);
static void DispDownloadFinishMsg(TRANSPACKET *Pkt, int iRetCode);
static bool DispUpDownErrDialog(int ResID, TRANSPACKET *Pkt) noexcept;
static int DoUpload(std::shared_ptr<SocketContext> cSkt, TRANSPACKET& item);
static int UploadFile(TRANSPACKET *Pkt, std::shared_ptr<SocketContext> dSkt, int Resume, int* CancelCheckWork);
static int TermCodeConvAndSend(SocketContext& s, char *Data, int Size, int Ascii, int *CancelCheckWork);
static void DispUploadFinishMsg(TRANSPACKET *Pkt, int iRetCode);
static LRESULT CALLBACK TransDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void DispTransferStatus(HWND hWnd, int End, TRANSPACKET *Pkt);
static void DispTransFileInfo(TRANSPACKET const& item, UINT titleId, int SkipButton, int Info);
static std::optional<std::tuple<std::wstring, int>> GetAdrsAndPort(SocketContext& s, std::wstring const& reply);
static bool IsSpecialDevice(std::wstring_view filename);
static int MirrorDelNotify(int Cur, int Notify, TRANSPACKET const& item) noexcept;

/*===== ローカルなワーク =====*/

// 同時接続対応
//static HANDLE hTransferThread;
static HANDLE hTransferThread[MAX_DATA_CONNECTION];
static bool fTransferThreadExit = false;
static int TransFiles = 0;				/* 転送待ちファイル数 */
static Concurrency::concurrent_queue<TRANSPACKET> TransPacketBase;	/* 転送ファイルリスト */

// 同時接続対応
//static int Canceled;		/* 中止フラグ YES/NO */
static int Canceled[MAX_DATA_CONNECTION];		/* 中止フラグ YES/NO */
static int ClearAll;		/* 全て中止フラグ YES/NO */

static int ForceAbort;		/* 転送中止フラグ */
							/* このフラグはスレッドを終了させるときに使う */
extern HANDLE initialized;

// 同時接続対応
//static LONGLONG AllTransSizeNow;	/* 今回の転送で転送したサイズ */
//static time_t TimeStart;	/* 転送開始時間 */
static LONGLONG AllTransSizeNow[MAX_DATA_CONNECTION];	/* 今回の転送で転送したサイズ */
static time_t TimeStart[MAX_DATA_CONNECTION];	/* 転送開始時間 */

static int KeepDlg = NO;	/* 転送中ダイアログを消さないかどうか (YES/NO) */
static int MoveToForeground = NO;		/* ウインドウを前面に移動するかどうか (YES/NO) */
static std::wstring CurDir[MAX_DATA_CONNECTION];
static thread_local std::wstring ErrMsg;

// 送受信の完了を検出する。
// [0..MAX_DATA_CONNECTION-1]： manual reset、各スレッドの状態。開始時にreset、完了したらset
// [MAX_DATA_CONNECTION]： auto reset、送受信開始時にset
// これら全てをWaitForMultipleObjectsで待つ。auto resetなので１スレッドだけが送受信完了の通知を受ける。
static HANDLE completed[MAX_DATA_CONNECTION + 1];

// 再転送対応
static int TransferErrorMode = EXIST_OVW;
static int TransferErrorNotify = NO;
// タスクバー進捗表示
static LONGLONG TransferSizeLeft = 0;
static LONGLONG TransferSizeTotal = 0;
static int TransferErrorDisplay = 0;


static void SetErrorMsg(std::wstring&& msg) {
	if (empty(ErrMsg))
		ErrMsg = std::move(msg);
}


// ファイル転送スレッドを起動する
int MakeTransferThread() noexcept {
	ClearAll = NO;
	ForceAbort = NO;
	fTransferThreadExit = false;
	for (int i = 0; i < MAX_DATA_CONNECTION; i++) {
		completed[i] = CreateEventW(nullptr, true, true, nullptr);
		hTransferThread[i] = (HANDLE)_beginthreadex(nullptr, 0, TransferThread, IntToPtr(i), 0, nullptr);
		if (hTransferThread[i] == (HANDLE)-1)
			return FFFTP_FAIL;
	}
	completed[MAX_DATA_CONNECTION] = CreateEventW(nullptr, false, false, nullptr);
	return FFFTP_SUCCESS;
}


// ファイル転送スレッドを終了する
void CloseTransferThread() noexcept {
	for (int i = 0; i < MAX_DATA_CONNECTION; i++)
		Canceled[i] = YES;
	ClearAll = YES;
	fTransferThreadExit = true;
	SetEvent(initialized);
	for (int i = 0; i < MAX_DATA_CONNECTION; i++) {
		while (WaitForSingleObject(hTransferThread[i], 10) == WAIT_TIMEOUT) {
			BackgrndMessageProc();
			Canceled[i] = YES;
		}
		CloseHandle(hTransferThread[i]);
		CloseHandle(completed[i]);
	}
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

	TransPacketBase.push(*Pkt);
	SetEvent(completed[MAX_DATA_CONNECTION]);
	if (Pkt->Command.starts_with(L"RETR"sv) || Pkt->Command.starts_with(L"STOR"sv)) {
		TransFiles++;
		// タスクバー進捗表示
		TransferSizeLeft += Pkt->Size;
		TransferSizeTotal += Pkt->Size;
		PostMessageW(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
	}
}


// バグ対策
void AddNullTransFileList()
{
	TRANSPACKET Pkt{};
	Pkt.Command = L"NULL"s;
	AddTransFileList(&Pkt);
}


// 転送ファイル情報を転送ファイルリストに追加する
void AppendTransFileList(std::vector<TRANSPACKET>&& list) {
	for (auto& Pkt : list)
		AddTransFileList(&Pkt);
}


// 転送ファイル情報を表示する
static void DispTransPacket(TRANSPACKET const& item) {
	if (item.Command.starts_with(L"RETR"sv) || item.Command.starts_with(L"STOR"sv))
		Debug(L"TransList Cmd={} : {} : {}"sv, item.Command, item.Remote, item.Local.native());
	else if (item.Command.starts_with(L"R-"sv))
		Debug(L"TransList Cmd={} : {}"sv, item.Command, item.Remote);
	else if (item.Command.starts_with(L"L-"sv))
		Debug(L"TransList Cmd={} : {}"sv, item.Command, item.Local.native());
	else if (item.Command.starts_with(L"MKD"sv))
		Debug(L"TransList Cmd={} : {}"sv, item.Command, item.Local.empty() ? item.Remote : item.Local.native());
	else
		Debug(L"TransList Cmd={}"sv, item.Command);
}


// 転送ファイルリストをクリアする
static void EraseTransFileList() {
	/* 最後の"BACKCUR"は必要なので消さない */
	// TODO: 一時的に全要素が消えるため、不整合が生じる可能性がある。try_popが速いのでEraseを排除できるのではないか？
	std::optional<TRANSPACKET> backcur;
	for (TRANSPACKET pkt; TransPacketBase.try_pop(pkt);)
		if (pkt.Command == L"BACKCUR"sv)
			backcur = pkt;
	if (backcur)
		TransPacketBase.push(*std::move(backcur));
	TransFiles = 0;
	TransferSizeLeft = 0;
	TransferSizeTotal = 0;
	PostMessageW(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
}


// 転送中ダイアログを消さないようにするかどうかを設定
void KeepTransferDialog(int Sw) noexcept {
	KeepDlg = Sw;
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


// 転送するファイルの数を返す
int AskTransferFileNum() noexcept {
	return TransFiles;
}


// 転送中ウインドウを前面に出す
void GoForwardTransWindow() noexcept {
	MoveToForeground = YES;
}


// 転送ソケットのカレントディレクトリ情報を初期化
void InitTransCurDir() noexcept {
	for (auto& dir : CurDir)
		dir.clear();
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
	int CwdSts;
//	int Down;
//	int Up;
	static int Down;
	static int Up;
	int DelNotify;
	int ThreadCount;
	std::shared_ptr<SocketContext> TrnSkt;
	RECT WndRect;
	int i;
	DWORD LastUsed = timeGetTime();
	int LastError;
	int Sts;

	hWndTrans = NULL;
	Down = NO;
	Up = NO;
	DelNotify = NO;
	// 同時接続対応
	// ソケットは各転送スレッドが管理
	ThreadCount = PtrToInt(Dummy);
	LastError = NO;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	auto const result = WaitForSingleObject(initialized, INFINITE);
	assert(result == WAIT_OBJECT_0);

	while (!fTransferThreadExit) {
		ErrMsg.clear();

//		Canceled = NO;
		Canceled[ThreadCount] = NO;

		if (GetCurHost().ReuseCmdSkt == YES && ThreadCount == 0) {
			TrnSkt = AskTrnCtrlSkt();
			// セッションあたりの転送量制限対策
			if (TrnSkt && GetCurHost().TransferErrorReconnect == YES && LastError == YES) {
				PostMessageW(GetMainHwnd(), WM_RECONNECTSOCKET, 0, 0);
				Sleep(100);
				TrnSkt.reset();
			}
		}
		else
		{
			// セッションあたりの転送量制限対策
			if (TrnSkt && GetCurHost().TransferErrorReconnect == YES && LastError == YES) {
				DoQUIT(TrnSkt, &Canceled[ThreadCount]);
				TrnSkt.reset();
			}
			if (!empty(TransPacketBase) && AskConnecting() == YES && ThreadCount < GetCurHost().MaxThreadCount) {
				if (!TrnSkt)
					ReConnectTrnSkt(TrnSkt, &Canceled[ThreadCount]);
				else
					CheckClosedAndReconnectTrnSkt(TrnSkt, &Canceled[ThreadCount]);
				// 同時ログイン数制限対策
				if (!TrnSkt)
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
			}
			else
			{
				if (TrnSkt) {
					// 同時ログイン数制限対策
					// 60秒間使用されなければログアウト
					if (timeGetTime() - LastUsed > 60000 || AskConnecting() == NO || ThreadCount >= GetCurHost().MaxThreadCount) {
						DoQUIT(TrnSkt, &Canceled[ThreadCount]);
						TrnSkt.reset();
					}
				}
			}
		}
		LastError = NO;
		ResetEvent(completed[ThreadCount]);
		if (TRANSPACKET pkt; TrnSkt && TransPacketBase.try_pop(pkt)) {
			auto Pos = &pkt;
			if(hWndTrans == NULL)
			{
				if (Pos->Command.starts_with(L"RETR"sv) || Pos->Command.starts_with(L"STOR"sv) || Pos->Command.starts_with(L"MKD"sv) || Pos->Command.starts_with(L"L-"sv) || Pos->Command.starts_with(L"R-"sv)) {
					hWndTrans = CreateDialogW(GetFtpInst(), MAKEINTRESOURCEW(transfer_dlg), HWND_DESKTOP, (DLGPROC)TransDlgProc);
					if(MoveToForeground == YES)
						SetForegroundWindow(hWndTrans);
					ShowWindow(hWndTrans, SW_SHOWNOACTIVATE);
					GetWindowRect(hWndTrans, &WndRect);
					SetWindowPos(hWndTrans, NULL, WndRect.left, WndRect.top + (WndRect.bottom - WndRect.top) * ThreadCount - (WndRect.bottom - WndRect.top) * (GetCurHost().MaxThreadCount - 1) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				}
			}
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
			TrnSkt->ClearReadBuffer();

			/* ダウンロード */
			if(Pos->Command.starts_with(L"RETR"sv))
			{
				/* 不正なパスを検出 */
				if(CheckPathViolation(*Pos) == NO)
				{
					/* フルパスを使わないための処理 */
					if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
					{
						if(Pos->Command.starts_with(L"RETR-S"sv))
						{
							/* サイズと日付を取得 */
							DoSIZE(TrnSkt, Pos->Remote, &Pos->Size, &Canceled[Pos->ThreadCount]);
							DoMDTM(TrnSkt, Pos->Remote, &Pos->Time, &Canceled[Pos->ThreadCount]);
							Pos->Command = L"RETR "s;
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
								MarkFileAsDownloadedFromInternet(Pos->Local);
						}

						if (SaveTimeStamp == YES && (Pos->Time.dwLowDateTime != 0 || Pos->Time.dwHighDateTime != 0))
							if (auto handle = CreateFileW(Pos->Local.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, 0); handle != INVALID_HANDLE_VALUE) {
								SetFileTime(handle, &Pos->Time, &Pos->Time, &Pos->Time);
								CloseHandle(handle);
							}
					}
				}
			}
			/* アップロード */
			else if(Pos->Command.starts_with(L"STOR"sv))
			{
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
						DoMFMT(TrnSkt, Pos->Remote, &Pos->Time, &Canceled[Pos->ThreadCount]);
					}
				}
			}
			/* フォルダ作成（ローカルまたはホスト） */
			else if(Pos->Command.starts_with(L"MKD"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN078, FALSE, YES);

				if (!empty(Pos->Remote)) {
					/* フルパスを使わないための処理 */
					CwdSts = FTP_COMPLETE;

					auto dir = Pos->Remote;
					if (ProcForNonFullpath(TrnSkt, dir, CurDir[Pos->ThreadCount], hWndTrans, &Canceled[Pos->ThreadCount]) == FFFTP_FAIL) {
						ClearAll = YES;
						CwdSts = FTP_ERROR;
					}

					if(CwdSts == FTP_COMPLETE)
					{
						Up = YES;
						Command(TrnSkt, &Canceled[Pos->ThreadCount], L"MKD {}"sv, dir);
						/* すでにフォルダがある場合もあるので、 */
						/* ここではエラーチェックはしない */

					if(FolderAttr)
						Command(TrnSkt, &Canceled[Pos->ThreadCount], L"{} {:03d} {}"sv, GetConnectingHost().ChmodCmd, FolderAttrNum, dir);
					}
				} else if (!Pos->Local.empty()) {
					Down = YES;
					DoLocalMKD(Pos->Local);
				}
			}
			/* ディレクトリ作成（常にホスト側） */
			else if(Pos->Command.starts_with(L"R-MKD"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN078, FALSE, YES);

				/* フルパスを使わないための処理 */
				if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
				{
					Up = YES;
					Command(TrnSkt, &Canceled[Pos->ThreadCount], L"{}{}"sv, std::wstring_view{ Pos->Command }.substr(2), Pos->Remote);

					if(FolderAttr)
						Command(TrnSkt, &Canceled[Pos->ThreadCount], L"{} {:03d} {}"sv, GetConnectingHost().ChmodCmd, FolderAttrNum, Pos->Remote);
				}
			}
			/* ディレクトリ削除（常にホスト側） */
			else if(Pos->Command.starts_with(L"R-RMD"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN080, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					/* フルパスを使わないための処理 */
					if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
					{
						Up = YES;
						Command(TrnSkt, &Canceled[Pos->ThreadCount], L"{}{}"sv, std::wstring_view{ Pos->Command }.substr(2), Pos->Remote);
					}
				}
			}
			/* ファイル削除（常にホスト側） */
			else if(Pos->Command.starts_with(L"R-DELE"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN081, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					/* フルパスを使わないための処理 */
					if(MakeNonFullPath(*Pos, CurDir[Pos->ThreadCount]) == FFFTP_SUCCESS)
					{
						Up = YES;
						Command(TrnSkt, &Canceled[Pos->ThreadCount], L"{}{}"sv, std::wstring_view{ Pos->Command }.substr(2), Pos->Remote);
					}
				}
			}
			/* ディレクトリ作成（常にローカル側） */
			else if(Pos->Command.starts_with(L"L-MKD"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN078, FALSE, YES);

				Down = YES;
				DoLocalMKD(Pos->Local);
			}
			/* ディレクトリ削除（常にローカル側） */
			else if(Pos->Command.starts_with(L"L-RMD"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN080, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					Down = YES;
					DoLocalRMD(Pos->Local);
				}
			}
			/* ファイル削除（常にローカル側） */
			else if(Pos->Command.starts_with(L"L-DELE"sv))
			{
				DispTransFileInfo(*Pos, IDS_MSGJPN081, FALSE, YES);

				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, *Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					Down = YES;
					DoLocalDELE(Pos->Local);
				}
			}
			/* カレントディレクトリを設定 */
			else if(Pos->Command == L"SETCUR"sv)
			{
				if (GetCurHost().ReuseCmdSkt == NO || AskShareProh() == YES) {
					if (CurDir[Pos->ThreadCount] != Pos->Remote) {
						if (std::get<0>(Command(TrnSkt, &Canceled[Pos->ThreadCount], L"CWD {}"sv, Pos->Remote)) / 100 != FTP_COMPLETE) {
							DispCWDerror(hWndTrans);
							ClearAll = YES;
						}
					}
				}
				CurDir[Pos->ThreadCount] = Pos->Remote;
			}
			/* カレントディレクトリを戻す */
			else if(Pos->Command == L"BACKCUR"sv)
			{
				if (GetCurHost().ReuseCmdSkt == YES && AskShareProh() == NO) {
					if (CurDir[Pos->ThreadCount] != Pos->Remote)
						Command(TrnSkt, &Canceled[Pos->ThreadCount], L"CWD {}"sv, Pos->Remote);
					CurDir[Pos->ThreadCount] = Pos->Remote;
				}
			}
			else if(Pos->Command == L"NULL"sv)
			{
				Sleep(0);
				Sleep(100);
			}

			/*===== １つの処理終わり =====*/

			if(ForceAbort == NO)
			{
				if(ClearAll == YES)
//					EraseTransFileList();
				{
					for(i = 0; i < MAX_DATA_CONNECTION; i++)
						Canceled[i] = YES;
					EraseTransFileList();
				}
				else
				{
					if (Pos->Command.starts_with(L"RETR"sv) || Pos->Command.starts_with(L"STOR"sv) || Pos->Command.starts_with(L"STOU"sv)) {
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

				if(BackgrndMessageProc() == YES)
					EraseTransFileList();
			}
			if(hWndTrans != NULL)
				SendMessageW(hWndTrans, WM_SET_PACKET, 0, 0);
			LastUsed = timeGetTime();
		}
//		else
		else if (empty(TransPacketBase))
		{
			ClearAll = NO;
			DelNotify = NO;
			SetEvent(completed[ThreadCount]);
			if (WaitForMultipleObjects(size_as<DWORD>(completed), completed, true, 0) == WAIT_OBJECT_0) {
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
			}

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
			TransferErrorMode = GetCurHost().TransferErrorMode;
			TransferErrorNotify = GetCurHost().TransferErrorNotify;
		}
		else
		{
			if(hWndTrans != NULL)
			{
				DestroyWindow(hWndTrans);
				hWndTrans = NULL;
			}
			BackgrndMessageProc();
			if (ThreadCount < GetCurHost().MaxThreadCount)
				Sleep(1);
			else
				Sleep(100);
		}
	}
	if (GetCurHost().ReuseCmdSkt == NO || ThreadCount > 0) {
		if (TrnSkt) {
			TrnSkt->Send("QUIT\r\n", 6, 0, &Canceled[ThreadCount]);
			TrnSkt.reset();
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
*			Pkt->Remote にファイル名のみ残す。（パス名は消す）
*----------------------------------------------------------------------------*/

static int MakeNonFullPath(TRANSPACKET& item, std::wstring& Cur) {
	auto const result = ProcForNonFullpath(item.ctrl_skt, item.Remote, Cur, item.hWndTrans, &Canceled[item.ThreadCount]);
	if (result == FFFTP_FAIL)
		ClearAll = YES;
	return result;
}


static int SendDownloadCommand(TRANSPACKET* Pkt, int& CreateMode, int* CancelCheckWork) {
	struct Data {
		using result_t = bool;
		void OnCommand(HWND hDlg, WORD id) noexcept {
			switch (id) {
			case IDOK:
				EndDialog(hDlg, true);
				break;
			case RESUME_CANCEL_ALL:
				ClearAll = YES;
				[[fallthrough]];
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			}
		}
	};
	auto tempSize = std::exchange(Pkt->ExistSize, 0);
	CreateMode = CREATE_ALWAYS;
	if (Pkt->Mode == EXIST_RESUME) {
		if (auto [code, text] = Command(Pkt->ctrl_skt, CancelCheckWork, L"REST {}"sv, tempSize); code / 100 < FTP_RETRY) {
			/* リジューム */
			if (Pkt->hWndTrans != NULL)
				Pkt->ExistSize = tempSize;
			CreateMode = OPEN_ALWAYS;
		} else {
			if (!Dialog(GetFtpInst(), noresume_dlg, Pkt->hWndTrans, Data{})) {
				Pkt->Abort = ABORT_USER;
				return 500;
			}
		}
	}
	auto [code, text] = Command(Pkt->ctrl_skt, CancelCheckWork, L"{}{}"sv, Pkt->Command, Pkt->Remote);
	if (code / 100 != FTP_PRELIM) {
		SetErrorMsg(std::move(text));
		Notice(IDS_MSGJPN090);
		return 500;
	}
	return code;
}


static int SendUploadCommand(TRANSPACKET* Pkt, int& Resume, int* CancelCheckWork) {
	Resume = Pkt->Mode == EXIST_RESUME && Pkt->hWndTrans != NULL ? YES : NO;
	auto cmd = L"APPE "sv;
	std::wstring extra;
	if (Resume == NO) {
		cmd = Pkt->Command;
		Pkt->ExistSize = 0;
#if defined(HAVE_TANDEM)
		if (AskHostType() == HTYPE_TANDEM && AskOSS() == NO && Pkt->Type != TYPE_A)
			extra = std::vformat(Pkt->PriExt == DEF_PRIEXT && Pkt->SecExt == DEF_SECEXT && Pkt->MaxExt == DEF_MAXEXT ? L",{}"sv : L",{},{},{},{}"sv, std::make_wformat_args(Pkt->FileCode, Pkt->PriExt, Pkt->SecExt, Pkt->MaxExt));
#endif
	}
	auto [code, text] = Command(Pkt->ctrl_skt, CancelCheckWork, L"{}{}{}"sv, cmd, Pkt->Remote, extra);
	if (code / 100 != FTP_PRELIM) {
		SetErrorMsg(std::move(text));
		Notice(IDS_MSGJPN090);
		return 500;
	}
	return code;
}


static int TransferActive(TRANSPACKET* Pkt, int (*SendTransferCommand)(TRANSPACKET* Pkt, int& mode, int* CancelCheckWork), int (*TransferFile)(TRANSPACKET* Pkt, std::shared_ptr<SocketContext> dSkt, int mode, int* CancelCheckWork), int* CancelCheckWork) {
	auto listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, CancelCheckWork);
	if (!listen_socket) {
		SetErrorMsg(GetString(IDS_MSGJPN279));
		return 500;
	}
	int mode;
	int code = SendTransferCommand(Pkt, mode, CancelCheckWork);
	if (code / 100 == FTP_PRELIM) {
		std::shared_ptr<SocketContext> data_socket;
		if (GetCurHost().FireWall == YES && (FwallType == FWALL_SOCKS4 || FwallType == FWALL_SOCKS5_NOAUTH || FwallType == FWALL_SOCKS5_USER)) {
			if (SocksReceiveReply(*listen_socket, CancelCheckWork))
				data_socket = listen_socket;
		} else {
			sockaddr_storage sa;
			int salen = sizeof(sockaddr_storage);
			data_socket = listen_socket->Accept(reinterpret_cast<sockaddr*>(&sa), &salen);
			if (shutdown(listen_socket->handle, 1) != 0)
				WSAError(L"shutdown(listen)"sv);
			if (!data_socket) {
				SetErrorMsg(GetString(IDS_MSGJPN280));
				WSAError(L"accept()"sv);
				code = 500;
			} else
				Debug(L"Skt={} : accept from {}"sv, data_socket->handle, AddressPortToString(&sa, salen));
		}
		if (data_socket)
			code = TransferFile(Pkt, data_socket, mode, CancelCheckWork);
	}
	if (IsUPnPLoaded() == YES)
		RemovePortMapping(listen_socket->mapPort);
	return code;
}


static int TransferPassive(TRANSPACKET* Pkt, int (*SendTransferCommand)(TRANSPACKET* Pkt, int& mode, int* CancelCheckWork), int (*TransferFile)(TRANSPACKET* Pkt, std::shared_ptr<SocketContext> dSkt, int mode, int* CancelCheckWork), int* CancelCheckWork) {
	auto [code, text] = Command(Pkt->ctrl_skt, CancelCheckWork, GetCurHost().CurNetType == NTYPE_IPV4 ? L"PASV"sv : L"EPSV"sv);
	if (code / 100 == FTP_COMPLETE) {
		if (auto const target = GetAdrsAndPort(*Pkt->ctrl_skt, text)) {
			if (auto [host, port] = *target; auto data_socket = connectsock(*Pkt->ctrl_skt, std::move(host), port, CancelCheckWork)) {
				if (constexpr BOOL optval = 1; setsockopt(data_socket->handle, IPPROTO_TCP, TCP_NODELAY, (LPSTR)&optval, sizeof(optval)) == SOCKET_ERROR)
					WSAError(L"setsockopt(IPPROTO_TCP, TCP_NODELAY)"sv);
				int mode;
				code = SendTransferCommand(Pkt, mode, CancelCheckWork);
				if (code / 100 == FTP_PRELIM)
					code = TransferFile(Pkt, data_socket, mode, CancelCheckWork);
			} else {
				SetErrorMsg(GetString(IDS_MSGJPN281));		// ダウンロードにはない
				return 500;
			}
		} else {
			SetErrorMsg(GetString(IDS_MSGJPN093));
			Notice(IDS_MSGJPN093);
			return 500;
		}
	} else
		SetErrorMsg(std::move(text));
	return code;
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

int DoDownload(std::shared_ptr<SocketContext> cSkt, TRANSPACKET& item, int DirList, int *CancelCheckWork)
{
	int iRetCode;

	item.ctrl_skt = cSkt;
	if (IsSpecialDevice(GetFileName(item.Local.native()))) {
		iRetCode = 500;
		Notice(IDS_MSGJPN085, GetFileName(item.Local.native()));
	}
	else if(item.Mode != EXIST_IGNORE)
	{
		if(item.Type == TYPE_I)
			item.KanjiCode = KANJI_NOCNV;

		std::wstring text;
		std::tie(iRetCode, text) = Command(item.ctrl_skt, CancelCheckWork, L"TYPE {}"sv, (wchar_t)item.Type);
		if(iRetCode/100 < FTP_RETRY)
		{
			if(item.hWndTrans != NULL)
			{
				// 同時接続対応
//				AllTransSizeNow = 0;
				AllTransSizeNow[item.ThreadCount] = 0;

				if(DirList == NO)
					DispTransFileInfo(item, IDS_MSGJPN086, TRUE, YES);
				else
					DispTransFileInfo(item, IDS_MSGJPN087, FALSE, NO);
			}

			if(BackgrndMessageProc() == NO)
			{
				if (GetCurHost().Pasv != YES)
					iRetCode = TransferActive(&item, SendDownloadCommand, DownloadFile, CancelCheckWork);
				else
					iRetCode = TransferPassive(&item, SendDownloadCommand, DownloadFile, CancelCheckWork);
			}
			else
				iRetCode = 500;
		}
		else
			SetErrorMsg(std::move(text));
		// エラーによってはダイアログが表示されない場合があるバグ対策
		DispDownloadFinishMsg(&item, iRetCode);
	}
	else
	{
		DispTransFileInfo(item, IDS_MSGJPN088, TRUE, YES);
		Notice(IDS_MSGJPN089, item.Remote);
		iRetCode = 200;
	}
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

static int DownloadFile(TRANSPACKET *Pkt, std::shared_ptr<SocketContext> dSkt, int CreateMode, int *CancelCheckWork) {
	assert(dSkt);
	assert(Pkt->ctrl_skt);
	if (Pkt->ctrl_skt->IsSSLAttached() && !dSkt->AttachSSL(CancelCheckWork))
		return 500;
#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
	int buf_size = 0;
	setsockopt(dSkt, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size));
#elif defined(SET_BUFFER_SIZE)
	for (int buf_size = SOCKBUF_SIZE; buf_size > 0; buf_size /= 2)
		if (setsockopt(dSkt->handle, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size)) == 0)
			break;
#endif

	Pkt->Abort = ABORT_NONE;
	if (auto const attr = GetFileAttributesW(Pkt->Local.c_str()); attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY))
		if (Message<IDS_MSGJPN086>(IDS_REMOVE_READONLY, MB_YESNO) == IDYES)
			SetFileAttributesW(Pkt->Local.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);

	auto opened = false;
	if (std::ofstream os{ Pkt->Local, std::ios::binary | (CreateMode == OPEN_ALWAYS ? std::ios::ate : std::ios::trunc) }) {
		opened = true;

		if (Pkt->hWndTrans != NULL) {
			TimeStart[Pkt->ThreadCount] = time(NULL);
			SetTimer(Pkt->hWndTrans, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
		}

		CodeConverter cc{ Pkt->KanjiCode, Pkt->KanjiCodeDesired, Pkt->KanaCnv != NO };
		auto const result = dSkt->ReadAll(CancelCheckWork, [&Pkt, &cc, &os](std::vector<char> const& buf) {
			if (auto converted = cc.Convert({ begin(buf), end(buf) }); !os.write(data(converted), size(converted))) {
				Pkt->Abort = ABORT_DISKFULL;
				return true;
			}
			Pkt->ExistSize += size_as<LONGLONG>(buf);
			if (Pkt->hWndTrans != NULL)
				AllTransSizeNow[Pkt->ThreadCount] += size_as<LONGLONG>(buf);
			else
				DispDownloadSize(Pkt->ExistSize);	/* 転送ダイアログを出さない時の経過表示 */
			return false;
		});
		if (result != ERROR_HANDLE_EOF) {
			if (result == ERROR_OPERATION_ABORTED)
				ForceAbort = YES;
			if (Pkt->Abort == ABORT_NONE)
				Pkt->Abort = ABORT_ERROR;
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
	} else {
		SetErrorMsg(std::vformat(GetString(IDS_MSGJPN095), std::make_wformat_args(Pkt->Local.native())));
		Notice(IDS_MSGJPN095, Pkt->Local.native());
		Pkt->Abort = ABORT_ERROR;
	}

	if (shutdown(dSkt->handle, 1) != 0)
		WSAError(L"shutdown()"sv);
	LastDataConnectionTime = time(NULL);
	dSkt.reset();

	/* Abortをホストに伝える */
	if (ForceAbort == NO && Pkt->Abort != ABORT_NONE && opened) {
		Pkt->ctrl_skt->Send("\xFF\xF4\xFF", 3, MSG_OOB, CancelCheckWork);	/* MSG_OOBに注意 */
		Pkt->ctrl_skt->Send("\xF2", 1, 0, CancelCheckWork);
		Command(Pkt->ctrl_skt, CancelCheckWork, L"ABOR"sv);
	}

	auto [code, text] = Pkt->ctrl_skt->ReadReply(CancelCheckWork);
	if (Pkt->Abort == ABORT_DISKFULL) {
		SetErrorMsg(GetString(IDS_MSGJPN096));
		Notice(IDS_MSGJPN096);
	}
	if (code / 100 >= FTP_RETRY)
		SetErrorMsg(std::move(text));
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
	if(ForceAbort == NO)
	{
		if((iRetCode/100) >= FTP_CONTINUE)
		{
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

			if (Pkt->Command.starts_with(L"NLST"sv) || Pkt->Command.starts_with(L"LIST"sv) || Pkt->Command.starts_with(L"MLSD"sv))
				Notice(IDS_MSGJPN097);
			else if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				Notice(IDS_MSGJPN099, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
			else
				Notice(IDS_MSGJPN100);

			if(Pkt->Abort != ABORT_USER)
			{
				if(Canceled[Pkt->ThreadCount] == NO && ClearAll == NO)
				{
					if(Pkt->Command.starts_with(L"RETR"sv) || Pkt->Command.starts_with(L"STOR"sv))
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
			if (Pkt->Command.starts_with(L"NLST"sv) || Pkt->Command.starts_with(L"LIST"sv) || Pkt->Command.starts_with(L"MLSD"sv))
				Notice(IDS_MSGJPN101, Pkt->ExistSize);
			else if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				Notice(IDS_MSGJPN102, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
			else
				Notice(IDS_MSGJPN103, Pkt->ExistSize);
		}
	}
	return;
}


// ダウンロード／アップロードエラーのダイアログを表示
static bool DispUpDownErrDialog(int ResID, TRANSPACKET *Pkt) noexcept {
	struct Data {
		using result_t = bool;
		using DownExistButton = RadioButton<DOWN_EXIST_OVW, DOWN_EXIST_RESUME, DOWN_EXIST_IGNORE>;
		TRANSPACKET* Pkt;
		Data(TRANSPACKET* Pkt) noexcept : Pkt{ Pkt } {}
		INT_PTR OnInit(HWND hDlg) noexcept {
			SetText(hDlg, UPDOWN_ERR_FNAME, Pkt->Remote);
			SetText(hDlg, UPDOWN_ERR_MSG, ErrMsg);
			if (Pkt->Type == TYPE_A || Pkt->ExistSize <= 0)
				EnableWindow(GetDlgItem(hDlg, DOWN_EXIST_RESUME), FALSE);
			DownExistButton::Set(hDlg, TransferErrorMode);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) noexcept {
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


/*----- アップロードを行なう --------------------------------------------------
*
*	Parameter
*		SOCKET cSkt : コントロールソケット
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

static int DoUpload(std::shared_ptr<SocketContext> cSkt, TRANSPACKET& item)
{
	int iRetCode;

	item.ctrl_skt = cSkt;

	if(item.Mode != EXIST_IGNORE)
	{
		if (std::wifstream{ item.Local }) {
			if(item.Type == TYPE_I)
				item.KanjiCode = KANJI_NOCNV;

			std::wstring text;
			std::tie(iRetCode, text) = Command(item.ctrl_skt, &Canceled[item.ThreadCount], L"TYPE {}", (wchar_t)item.Type);
			if(iRetCode/100 < FTP_RETRY)
			{
				if (item.Mode == EXIST_UNIQUE) {
					item.Command = L"STOU "s;
					// 応答の形式に規格が無くファイル名を取得できないため属性変更を無効化
					item.Attr = -1;
				}

				if(item.hWndTrans != NULL)
					DispTransFileInfo(item, IDS_MSGJPN104, TRUE, YES);

				if(BackgrndMessageProc() == NO)
				{
					if (GetCurHost().Pasv != YES)
						iRetCode = TransferActive(&item, SendUploadCommand, UploadFile, &Canceled[item.ThreadCount]);
					else
						iRetCode = TransferPassive(&item, SendUploadCommand, UploadFile, &Canceled[item.ThreadCount]);
				}
				else
					iRetCode = 500;
			}
			else
				SetErrorMsg(std::move(text));

			/* 属性変更 */
			if((item.Attr != -1) && ((iRetCode/100) == FTP_COMPLETE))
				Command(item.ctrl_skt, &Canceled[item.ThreadCount], L"{} {:03X} {}"sv, GetConnectingHost().ChmodCmd, item.Attr, item.Remote);
		}
		else
		{
			SetErrorMsg(std::vformat(GetString(IDS_MSGJPN105), std::make_wformat_args(item.Local.native())));
			Notice(IDS_MSGJPN105, item.Local.native());
			iRetCode = 500;
			item.Abort = ABORT_ERROR;
		}
		DispUploadFinishMsg(&item, iRetCode);
	}
	else
	{
		DispTransFileInfo(item, IDS_MSGJPN106, TRUE, YES);
		Notice(IDS_MSGJPN107, item.Local.native());
		iRetCode = 200;
	}
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

static int UploadFile(TRANSPACKET *Pkt, __pragma(warning(suppress: 26415 26418)) std::shared_ptr<SocketContext> dSkt, int Resume, int* CancelCheckWork) {
	if (Pkt->ctrl_skt->IsSSLAttached() && !dSkt->AttachSSL(CancelCheckWork))
		return 500;
#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
	int buf_size = 0;
	setsockopt(dSkt, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size));
#elif defined(SET_BUFFER_SIZE)
	for (int buf_size = SOCKBUF_SIZE; buf_size > 0; buf_size /= 2)
		if (setsockopt(dSkt->handle, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size)) == 0)
			break;
#endif

	Pkt->Abort = ABORT_NONE;
	if (std::ifstream is{ Pkt->Local, std::ios::binary }) {
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
		std::vector<char> buf(BUFSIZE);
		std::vector<char> rest;
		for (std::streamsize read; Pkt->Abort == ABORT_NONE && ForceAbort == NO && !eof && (read = is.read(data(buf), BUFSIZE).gcount()) != 0;) {
			/* EOF除去 */
			if (RmEOF == YES && Pkt->Type == TYPE_A)
				if (auto pos = std::find(data(buf), data(buf) + read, '\x1A'); pos != data(buf) + read) {
					eof = true;
					read = pos - data(buf);
				}

			// 漢字コード変換と改行コード変換が途中で分断されないようにASCIIで区切る。incodeにUTF-16は存在しないので、全てUS-ASCII互換を仮定できる。
			// TODO: ISO-2022-JPはこの区切り方でも正しくない。
			if (cc.incode != KANJI_NOCNV || Pkt->Type == TYPE_A) {
				read += size(rest);
				buf.insert(begin(buf), begin(rest), end(rest));
				rest.clear();
				for (auto i = read; 0 < i; i--)
					if (auto ch = buf[size_t(i) - 1]; ch == '\n' || ' ' <= ch && ch <= '~') {
						rest.assign(begin(buf) + int(i), begin(buf) + int(read));
						read = i;
						break;
					}
			}

			auto converted = cc.Convert({ data(buf), (std::string_view::size_type)read });
			if (TermCodeConvAndSend(*dSkt, data(converted), size_as<DWORD>(converted), Pkt->Type, CancelCheckWork) == FFFTP_FAIL)
				Pkt->Abort = ABORT_ERROR;

			Pkt->ExistSize += read;
			if (Pkt->hWndTrans != NULL)
				AllTransSizeNow[Pkt->ThreadCount] += read;

			if (BackgrndMessageProc() == YES)
				ForceAbort = YES;
		}
		if (Pkt->Abort == ABORT_NONE && ForceAbort == NO && !eof && !empty(rest)) {
			auto converted = cc.Convert({ data(rest), size(rest) });
			if (TermCodeConvAndSend(*dSkt, data(converted), size_as<DWORD>(converted), Pkt->Type, CancelCheckWork) == FFFTP_FAIL)
				Pkt->Abort = ABORT_ERROR;
			Pkt->ExistSize += size_as<LONGLONG>(rest);
			if (Pkt->hWndTrans != NULL)
				AllTransSizeNow[Pkt->ThreadCount] += size_as<LONGLONG>(rest);
		}

		/* グラフ表示を更新 */
		if (Pkt->hWndTrans != NULL) {
			KillTimer(Pkt->hWndTrans, TIMER_DISPLAY);
			DispTransferStatus(Pkt->hWndTrans, YES, Pkt);
			TimeStart[Pkt->ThreadCount] = time(NULL) - TimeStart[Pkt->ThreadCount] + 1;
		}
	} else {
		SetErrorMsg(std::vformat(GetString(IDS_MSGJPN112), std::make_wformat_args(Pkt->Local.native())));
		Notice(IDS_MSGJPN112, Pkt->Local.native());
		Pkt->Abort = ABORT_ERROR;
	}

	LastDataConnectionTime = time(NULL);
	if (shutdown(dSkt->handle, 1) != 0)
		WSAError(L"shutdown()"sv);

	auto [code, text] = Pkt->ctrl_skt->ReadReply(CancelCheckWork);
	if (code / 100 >= FTP_RETRY)
		SetErrorMsg(std::move(text));
	if (Pkt->Abort != ABORT_NONE)
		code = 500;
	return code;
}


// バッファの内容を改行コード変換して送信
static int TermCodeConvAndSend(SocketContext& s, char *Data, int Size, int Ascii, int *CancelCheckWork) {
	// CR-LF以外の改行コードを変換しないモードはここへ追加
	if (Ascii == TYPE_A) {
		auto encoded = ToCRLF({ Data, (size_t)Size });
		return s.Send(data(encoded), size_as<int>(encoded), 0, CancelCheckWork);
	}
	return s.Send(Data, Size, 0, CancelCheckWork);
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
	if(ForceAbort == NO)
	{
		if((iRetCode/100) >= FTP_CONTINUE)
		{
			if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				Notice(IDS_MSGJPN113, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
			else
				Notice(IDS_MSGJPN114);

			if(Pkt->Abort != ABORT_USER)
			{
				if(Canceled[Pkt->ThreadCount] == NO && ClearAll == NO)
				{
					if(Pkt->Command.starts_with(L"RETR"sv) || Pkt->Command.starts_with(L"STOR"sv))
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
			if((Pkt->hWndTrans != NULL) && (TimeStart[Pkt->ThreadCount] != 0))
				Notice(IDS_MSGJPN115, TimeStart[Pkt->ThreadCount], Pkt->ExistSize/TimeStart[Pkt->ThreadCount]);
			else
				Notice(IDS_MSGJPN116);
		}
	}
	return;
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
					[[fallthrough]];
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

			auto const TotalLap = time(nullptr) - TimeStart[Pkt->ThreadCount] + 1;
			auto const Bps = TotalLap != 0 ? AllTransSizeNow[Pkt->ThreadCount] / TotalLap : 0;
			if (Bps < 1024)
				ss << L"( " << Bps << L"B/s )";
			else if (Bps < 1024 * 1024)
				ss << L"( " << Bps / 1024. << L"KB/s )";
			else if (Bps < 1024 * 1024 * 1024)
				ss << L"( " << Bps / 1024 / 1024. << L"MB/s )";
			else
				ss << L"( " << Bps / 1024 / 1024 / 1024. << L"GB/s )";

			if (auto const Transed = Pkt->Size - Pkt->ExistSize; 0 < Bps && 0 < Pkt->Size && 0 <= Transed)
				ss << L"  " << Transed / Bps / 60 << L':' << std::setfill(L'0') << std::setw(2) << Transed / Bps % 60;
			else
				ss << L"  ??:??";

			status = ss.str();
		}
		SetText(hWnd, TRANS_STATUS, status);
	}
	{
		int const percent = Pkt->Size <= 0 ? 0 : Pkt->Size < std::numeric_limits<decltype(Pkt->Size)>::max() / 100 ? (int)(Pkt->ExistSize * 100 / Pkt->Size) : (int)((Pkt->ExistSize / 1024) * 100 / (Pkt->Size / 1024));
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

static void DispTransFileInfo(TRANSPACKET const& item, UINT titleId, int SkipButton, int Info) {
	if (item.hWndTrans != NULL) {
		EnableWindow(GetDlgItem(item.hWndTrans, IDCANCEL), SkipButton);

		SetText(item.hWndTrans, std::format(L"({}){}"sv, AskTransferFileNum(), GetString(titleId)));
		SetText(item.hWndTrans, TRANS_STATUS, L"");

		SendDlgItemMessageW(item.hWndTrans, TRANS_TIME_BAR, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendDlgItemMessageW(item.hWndTrans, TRANS_TIME_BAR, PBM_SETSTEP, 1, 0);
		SendDlgItemMessageW(item.hWndTrans, TRANS_TIME_BAR, PBM_SETPOS, 0, 0);

		if (Info == YES) {
			DispStaticText(GetDlgItem(item.hWndTrans, TRANS_REMOTE), item.Remote);
			DispStaticText(GetDlgItem(item.hWndTrans, TRANS_LOCAL), item.Local);

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


// PASVコマンドの戻り値からアドレスとポート番号を抽出
static std::optional<std::tuple<std::wstring, int>> GetAdrsAndPort(SocketContext& s, std::wstring const& reply) {
	std::wstring addr;
	int port;
	if (auto const& curHost = GetCurHost(); curHost.CurNetType == NTYPE_IPV4) {
		// RFC1123 4.1.2.6  PASV Command: RFC-959 Section 4.1.2
		// Therefore, a User-FTP program that interprets the PASV reply must scan the reply for the first digit of the host and port numbers.
		// コンマではなくドットを返すホストがある
		static boost::wregex re{ LR"((\d+[,.]\d+[,.]\d+[,.]\d+)[,.](\d+)[,.](\d+))" };
		if (boost::wsmatch m; boost::regex_search(reply, m, re)) {
			port = std::stoi(m[2]) << 8 | std::stoi(m[3]);
			if (curHost.NoPasvAdrs == NO) {
				addr = m[1];
				std::ranges::replace(addr, L',', L'.');
				return { { addr, port } };
			}
		} else
			return {};
	} else {
		// RFC2428 3.  The EPSV Command
		// The text returned in response to the EPSV command MUST be:
		// <text indicating server is entering extended passive mode> (<d><d><d><tcp-port><d>)
		static boost::wregex re{ LR"(\(([\x21-\xFE])\1\1(\d+)\1\))" };
		if (boost::wsmatch m; boost::regex_search(reply, m, re)) {
			port = std::stoi(m[2]);
		} else
			return {};
	}
	addr = s.target.index() == 0 ? AddressToString(std::get<0>(s.target)) : std::get<0>(std::get<1>(s.target));
	return { { addr, port } };
}


// Windowsのスペシャルデバイスかどうかを返す
static bool IsSpecialDevice(std::wstring_view filename) {
	static boost::wregex re{ LR"(^(?:AUX|CON|NUL|PRN|COM[0-9]|LPT[0-9])(?:\.|$))", boost::regex_constants::icase };
	return boost::regex_search(begin(filename), end(filename), re);
}


// ミラーリングでのファイル削除確認
static int MirrorDelNotify(int Cur, int Notify, TRANSPACKET const& item) noexcept {
	struct Data {
		using result_t = int;
		int Cur;
		TRANSPACKET const& item;
		Data(int Cur, TRANSPACKET const& item) noexcept : Cur{ Cur }, item{ item } {}
		INT_PTR OnInit(HWND hDlg) {
			if (Cur == WIN_LOCAL) {
				SetText(hDlg, GetString(IDS_MSGJPN124));
				SetText(hDlg, DELETE_TEXT, item.Local);
			} else {
				SetText(hDlg, GetString(IDS_MSGJPN125));
				SetText(hDlg, DELETE_TEXT, item.Remote);
			}
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) noexcept {
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
	if (boost::regex_search(item.Remote, re)) {
		auto const message = std::vformat(GetString(IDS_INVALID_PATH), std::make_wformat_args(item.Remote));
		MessageBoxW(GetMainHwnd(), message.c_str(), GetString(IDS_MSGJPN086).c_str(), MB_OK);
		return YES;
	}
	return NO;
}


// タスクバー進捗表示
LONGLONG AskTransferSizeLeft() noexcept {
	return TransferSizeLeft;
}

LONGLONG AskTransferSizeTotal() noexcept {
	return TransferSizeTotal;
}

int AskTransferErrorDisplay() noexcept {
	return TransferErrorDisplay;
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

int IsZoneIDLoaded() noexcept {
	return zoneIdentifier && persistFile ? YES : NO;
}

bool MarkFileAsDownloadedFromInternet(fs::path const& path) {
	struct Data : public MainThreadRunner {
		fs::path const& path;
		Data(fs::path const& path) noexcept : path{ path } {}
		int DoWork() override {
			return persistFile->Save(_bstr_t{ path.c_str() }, FALSE);
		}
	} data{ path };
	auto const result = (HRESULT)data.Run();
	return result == S_OK;
}
