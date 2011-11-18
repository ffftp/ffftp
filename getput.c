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

#define	STRICT
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
#include <time.h>
// IPv6対応
//#include <winsock.h>
#include <winsock2.h>
#include <windowsx.h>
#include <commctrl.h>
#include <process.h>

#include "common.h"
#include "resource.h"

#define SET_BUFFER_SIZE

/* Add by H.Shirouzu at 2002/10/02 */
#undef BUFSIZE
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


/* 削除確認ダイアログの情報 */
typedef struct {
	int Cur;
	TRANSPACKET *Pkt;
} MIRRORDELETEINFO;


/*===== プロトタイプ =====*/

static void DispTransPacket(TRANSPACKET *Pkt);
static void EraseTransFileList(void);
static ULONG WINAPI TransferThread(void *Dummy);
static int MakeNonFullPath(TRANSPACKET *Pkt, char *CurDir, char *Tmp);
static int DownLoadNonPassive(TRANSPACKET *Pkt, int *CancelCheckWork);
static int DownLoadPassive(TRANSPACKET *Pkt, int *CancelCheckWork);
static int DownLoadFile(TRANSPACKET *Pkt, SOCKET dSkt, int CreateMode, int *CancelCheckWork);
static void DispDownloadFinishMsg(TRANSPACKET *Pkt, int iRetCode);
// 再転送対応
//static int DispUpDownErrDialog(int ResID, HWND hWnd, char *Fname);
static int DispUpDownErrDialog(int ResID, HWND hWnd, TRANSPACKET *Pkt);
static BOOL CALLBACK UpDownErrorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int SetDownloadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode, int *CancelCheckWork);
static BOOL CALLBACK NoResumeWndProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static int DoUpLoad(SOCKET cSkt, TRANSPACKET *Pkt);
static int UpLoadNonPassive(TRANSPACKET *Pkt);
static int UpLoadPassive(TRANSPACKET *Pkt);
static int UpLoadFile(TRANSPACKET *Pkt, SOCKET dSkt);
// 同時接続対応
//static int TermCodeConvAndSend(TERMCODECONVINFO *tInfo, SOCKET Skt, char *Data, int Size, int Ascii);
static int TermCodeConvAndSend(TERMCODECONVINFO *tInfo, SOCKET Skt, char *Data, int Size, int Ascii, int *CancelCheckWork);
static void DispUploadFinishMsg(TRANSPACKET *Pkt, int iRetCode);
static int SetUploadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode);
static LRESULT CALLBACK TransDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void DispTransferStatus(HWND hWnd, int End, TRANSPACKET *Pkt);
static void DispTransFileInfo(TRANSPACKET *Pkt, char *Title, int SkipButton, int Info);
// IPv6対応
//static int GetAdrsAndPort(char *Str, char *Adrs, int *Port, int Max);
static int GetAdrsAndPort(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max);
static int GetAdrsAndPortIPv4(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max);
static int GetAdrsAndPortIPv6(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max);
static int IsSpecialDevice(char *Fname);
static int MirrorDelNotify(int Cur, int Notify, TRANSPACKET *Pkt);
static BOOL CALLBACK MirrorDeleteDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static void SetErrorMsg(char *fmt, ...);
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
static TRANSPACKET *TransPacketBase = NULL;	/* 転送ファイルリスト */
// 同時接続対応
static TRANSPACKET *NextTransPacketBase = NULL;

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
	DWORD dwID;
	int i;

	hListAccMutex = CreateMutex( NULL, FALSE, NULL );
	hRunMutex = CreateMutex( NULL, TRUE, NULL );
	// 同時接続対応
	hErrMsgMutex = CreateMutex( NULL, FALSE, NULL );

	ClearAll = NO;
	ForceAbort = NO;

	fTransferThreadExit = FALSE;
	// 同時接続対応
//	hTransferThread = (HANDLE)_beginthreadex(NULL, 0, TransferThread, 0, 0, &dwID);
//	if (hTransferThread == NULL)
//		return(FFFTP_FAIL); /* XXX */
	for(i = 0; i < MAX_DATA_CONNECTION; i++)
	{
		hTransferThread[i] = (HANDLE)_beginthreadex(NULL, 0, TransferThread, (void*)i, 0, &dwID);
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
	ForceAbort = YES;

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


/*----- 転送するファイル情報をリストに追加する --------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		TRANSPACKET **Base : リストの先頭
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int AddTmpTransFileList(TRANSPACKET *Pkt, TRANSPACKET **Base)
{
	TRANSPACKET *Pos;
	TRANSPACKET *Prev;
	int Sts;

	Sts = FFFTP_FAIL;
	if((Pos = malloc(sizeof(TRANSPACKET))) != NULL)
	{
		memcpy(Pos, Pkt, sizeof(TRANSPACKET));
		Pos->Next = NULL;

		if(*Base == NULL)
			*Base = Pos;
		else
		{
			Prev = *Base;
			while(Prev->Next != NULL)
				Prev = Prev->Next;
			Prev->Next = Pos;
		}
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 転送するファイル情報リストをクリアする --------------------------------
*
*	Parameter
*		TRANSPACKET **Base : リストの先頭
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void EraseTmpTransFileList(TRANSPACKET **Base)
{
	TRANSPACKET *Pos;
	TRANSPACKET *Next;

	Pos = *Base;
	while(Pos != NULL)
	{
		Next = Pos->Next;
		free(Pos);
		Pos = Next;
	}
	*Base = NULL;
	return;
}


/*----- 転送するファイル情報リストから１つの情報を取り除く --------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		TRANSPACKET **Base : リストの先頭
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int RemoveTmpTransFileListItem(TRANSPACKET **Base, int Num)
{
	TRANSPACKET *Pos;
	TRANSPACKET *Prev;
	int Sts;

	Sts = FFFTP_FAIL;
	Pos = *Base;
	if(Num == 0)
	{
		*Base = Pos->Next;
		free(Pos);
		Sts = FFFTP_SUCCESS;
	}
	else
	{
		while(Pos != NULL)
		{
			Prev = Pos;
			Pos = Pos->Next;
			if(--Num == 0)
			{
				Prev->Next = Pos->Next;
				free(Pos);
				Sts = FFFTP_SUCCESS;
				break;
			}
		}
	}
	return(Sts);
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
	// 同時接続対応
	TRANSPACKET *Pos;

	DispTransPacket(Pkt);

	// 同時接続対応
//	WaitForSingleObject(hListAccMutex, INFINITE);
	while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
	{
		WaitForMainThread = YES;
		BackgrndMessageProc();
		Sleep(1);
	}

	// 同時接続対応
	Pos = TransPacketBase;
	if(Pos != NULL)
	{
		while(Pos->Next != NULL)
			Pos = Pos->Next;
	}
	if(AddTmpTransFileList(Pkt, &TransPacketBase) == FFFTP_SUCCESS)
	{
		if((strncmp(Pkt->Cmd, "RETR", 4) == 0) ||
		   (strncmp(Pkt->Cmd, "STOR", 4) == 0))
		{
			TransFiles++;
			PostMessage(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
		}
	}
	// 同時接続対応
	if(NextTransPacketBase == NULL)
	{
		if(Pos)
			NextTransPacketBase = Pos->Next;
		else
			NextTransPacketBase = TransPacketBase;
	}
	ReleaseMutex(hListAccMutex);
	// 同時接続対応
	WaitForMainThread = NO;

	return;
}


/*----- 転送ファイル情報を転送ファイルリストに追加する ------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*		TRANSPACKET **Base : リストの先頭
*
*	Return Value
*		なし
*
*	Note
*		Pkt自体をリストに連結する
*----------------------------------------------------------------------------*/

void AppendTransFileList(TRANSPACKET *Pkt)
{
	TRANSPACKET *Pos;

	// 同時接続対応
//	WaitForSingleObject(hListAccMutex, INFINITE);
	while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
	{
		WaitForMainThread = YES;
		BackgrndMessageProc();
		Sleep(1);
	}

	if(TransPacketBase == NULL)
		TransPacketBase = Pkt;
	else
	{
		Pos = TransPacketBase;
		while(Pos->Next != NULL)
			Pos = Pos->Next;
		Pos->Next = Pkt;
	}
	// 同時接続対応
	if(NextTransPacketBase == NULL)
		NextTransPacketBase = Pkt;

	while(Pkt != NULL)
	{
		DispTransPacket(Pkt);

		if((strncmp(Pkt->Cmd, "RETR", 4) == 0) ||
		   (strncmp(Pkt->Cmd, "STOR", 4) == 0))
		{
			TransFiles++;
			PostMessage(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
		}
		Pkt = Pkt->Next;
	}

	ReleaseMutex(hListAccMutex);
	// 同時接続対応
	WaitForMainThread = NO;
	return;
}


/*----- 転送ファイル情報を表示する --------------------------------------------
*
*	Parameter
*		TRANSPACKET *Pkt : 転送ファイル情報
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispTransPacket(TRANSPACKET *Pkt)
{
	if((strncmp(Pkt->Cmd, "RETR", 4) == 0) || (strncmp(Pkt->Cmd, "STOR", 4) == 0))
		DoPrintf("TransList Cmd=%s : %s : %s", Pkt->Cmd, Pkt->RemoteFile, Pkt->LocalFile);
	else if(strncmp(Pkt->Cmd, "R-", 2) == 0)
		DoPrintf("TransList Cmd=%s : %s", Pkt->Cmd, Pkt->RemoteFile);
	else if(strncmp(Pkt->Cmd, "L-", 2) == 0)
		DoPrintf("TransList Cmd=%s : %s", Pkt->Cmd, Pkt->LocalFile);
	else if(strncmp(Pkt->Cmd, "MKD", 3) == 0)
	{
		if(strlen(Pkt->LocalFile) > 0)
			DoPrintf("TransList Cmd=%s : %s", Pkt->Cmd, Pkt->LocalFile);
		else
			DoPrintf("TransList Cmd=%s : %s", Pkt->Cmd, Pkt->RemoteFile);
	}
	else
		DoPrintf("TransList Cmd=%s", Pkt->Cmd);
	return;
}


/*----- 転送ファイルリストをクリアする ----------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void EraseTransFileList(void)
{
	TRANSPACKET *New;
	TRANSPACKET *Next;
	TRANSPACKET *NotDel;
	TRANSPACKET Pkt;

	NotDel = NULL;

	// 同時接続対応
//	WaitForSingleObject(hListAccMutex, INFINITE);
	while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
	{
		WaitForMainThread = YES;
		BackgrndMessageProc();
		Sleep(1);
	}
	New = TransPacketBase;
	while(New != NULL)
	{
		/* 最後の"BACKCUR"は必要なので消さない */
		if(strcmp(New->Cmd, "BACKCUR") == 0)
		{
			if(NotDel != NULL)
				// 同時接続対応
//				free(NotDel);
				strcpy(NotDel->Cmd, "");
			NotDel = New;
			New = New->Next;
			// 同時接続対応
//			NotDel->Next = NULL;
		}
		else
		{
			Next = New->Next;
			// 同時接続対応
//			free(New);
			strcpy(New->Cmd, "");
			New = Next;
		}
	}
	TransPacketBase = NotDel;
	// 同時接続対応
	NextTransPacketBase = NotDel;
	TransFiles = 0;
	PostMessage(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
	ReleaseMutex(hListAccMutex);
	// 同時接続対応
	WaitForMainThread = NO;

	strcpy(Pkt.Cmd, "GOQUIT");
	AddTransFileList(&Pkt);
	return;
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
	return(TransPacketBase != NULL ? YES : NO);
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

static ULONG WINAPI TransferThread(void *Dummy)
{
	TRANSPACKET *Pos;
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
	SOCKET CmdSkt;
	SOCKET NewCmdSkt;
	SOCKET TrnSkt;
	RECT WndRect;
	int i;

	hWndTrans = NULL;
	Down = NO;
	Up = NO;
	GoExit = NO;
	DelNotify = NO;
	// 同時接続対応
	// ソケットは各転送スレッドが管理
	ThreadCount = (int)Dummy;
	CmdSkt = INVALID_SOCKET;
	NewCmdSkt = INVALID_SOCKET;
	TrnSkt = INVALID_SOCKET;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

	while((TransPacketBase != NULL) ||
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
//		memset(ErrMsg, NUL, ERR_MSG_LEN+7);
		memset(GetErrMsg(), NUL, ERR_MSG_LEN+7);

//		Canceled = NO;
		Canceled[ThreadCount] = NO;

		while(TransPacketBase != NULL && strcmp(TransPacketBase->Cmd, "") == 0)
		{
			Pos = TransPacketBase;
			TransPacketBase = TransPacketBase->Next;
			free(Pos);
		}
		NewCmdSkt = AskCmdCtrlSkt();
		if(AskReuseCmdSkt() == YES && ThreadCount == 0)
		{
			if(TransPacketBase && ThreadCount < AskMaxThreadCount())
				TrnSkt = AskTrnCtrlSkt();
		}
		else
		{
			if(TransPacketBase && NewCmdSkt != INVALID_SOCKET && ThreadCount < AskMaxThreadCount())
			{
				if(TrnSkt == INVALID_SOCKET || NewCmdSkt != CmdSkt)
				{
					ReleaseMutex(hListAccMutex);
					ReConnectTrnSkt(&TrnSkt, &Canceled[ThreadCount]);
					// 同時ログイン数制限に引っかかった可能性あり
					// 負荷を下げるためにしばらく待機
					if(TrnSkt == INVALID_SOCKET)
					{
						i = 10000;
						while(NewCmdSkt != CmdSkt && i > 0)
						{
							BackgrndMessageProc();
							Sleep(1);
							i--;
						}
					}
//					WaitForSingleObject(hListAccMutex, INFINITE);
					while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
					{
						BackgrndMessageProc();
						Sleep(1);
					}
				}
			}
			else
			{
				if(TrnSkt != INVALID_SOCKET)
				{
					ReleaseMutex(hListAccMutex);
					SendData(TrnSkt, "QUIT\r\n", 6, 0, &Canceled[ThreadCount]);
					DoClose(TrnSkt);
					TrnSkt = INVALID_SOCKET;
//					WaitForSingleObject(hListAccMutex, INFINITE);
					while(WaitForSingleObject(hListAccMutex, 0) == WAIT_TIMEOUT)
					{
						BackgrndMessageProc();
						Sleep(1);
					}
				}
			}
		}
		CmdSkt = NewCmdSkt;
//		if(TransPacketBase != NULL)
		if(TrnSkt != INVALID_SOCKET && NextTransPacketBase != NULL)
		{
			Pos = NextTransPacketBase;
			NextTransPacketBase = NextTransPacketBase->Next;
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
					hWndTrans = CreateDialog(GetFtpInst(), MAKEINTRESOURCE(transfer_dlg), HWND_DESKTOP, (DLGPROC)TransDlgProc);
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
//				SendMessage(hWndTrans, WM_SET_PACKET, 0, (LPARAM)TransPacketBase);
				SendMessage(hWndTrans, WM_SET_PACKET, 0, (LPARAM)Pos);

			/* ダウンロード */
//			if(strncmp(TransPacketBase->Cmd, "RETR", 4) == 0)
			if(strncmp(Pos->Cmd, "RETR", 4) == 0)
			{
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
//				ReleaseMutex(hListAccMutex);
				/* 不正なパスを検出 */
//				if(CheckPathViolation(TransPacketBase) == NO)
				if(CheckPathViolation(Pos) == NO)
				{
					/* フルパスを使わないための処理 */
//					if(MakeNonFullPath(TransPacketBase, CurDir, Tmp) == FFFTP_SUCCESS)
					if(MakeNonFullPath(Pos, CurDir[Pos->ThreadCount], Tmp) == FFFTP_SUCCESS)
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
//						if(DoDownLoad(AskTrnCtrlSkt(), TransPacketBase, NO) == 429)
//						{
//							if(ReConnectTrnSkt() == FFFTP_SUCCESS)
//								DoDownLoad(AskTrnCtrlSkt(), TransPacketBase, NO, &Canceled);
								DoDownLoad(TrnSkt, Pos, NO, &Canceled[Pos->ThreadCount]);
//						}
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
//				if(MakeNonFullPath(TransPacketBase, CurDir, Tmp) == FFFTP_SUCCESS)
				if(MakeNonFullPath(Pos, CurDir[Pos->ThreadCount], Tmp) == FFFTP_SUCCESS)
				{
					Up = YES;
//					if(DoUpLoad(AskTrnCtrlSkt(), TransPacketBase) == 429)
//					{
//						if(ReConnectTrnSkt() == FFFTP_SUCCESS)
//							DoUpLoad(AskTrnCtrlSkt(), TransPacketBase);
							DoUpLoad(TrnSkt, Pos);
//					}
				}
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
				ReleaseMutex(hListAccMutex);
			}
			/* フォルダ作成（ローカルまたはホスト） */
//			else if(strncmp(TransPacketBase->Cmd, "MKD", 3) == 0)
			else if(strncmp(Pos->Cmd, "MKD", 3) == 0)
			{
//				DispTransFileInfo(TransPacketBase, MSGJPN078, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN078, FALSE, YES);

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
//						CommandProcTrn(NULL, "%s %03d %s", AskHostChmodCmd(), FolderAttrNum, Tmp);
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s %03d %s", AskHostChmodCmd(), FolderAttrNum, Tmp);
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
//				DispTransFileInfo(TransPacketBase, MSGJPN079, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN079, FALSE, YES);

				/* フルパスを使わないための処理 */
//				if(MakeNonFullPath(TransPacketBase, CurDir, Tmp) == FFFTP_SUCCESS)
				if(MakeNonFullPath(Pos, CurDir[Pos->ThreadCount], Tmp) == FFFTP_SUCCESS)
				{
					Up = YES;
//					CommandProcTrn(NULL, "%s%s", TransPacketBase->Cmd+2, TransPacketBase->RemoteFile);
					CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s%s", Pos->Cmd+2, Pos->RemoteFile);

					if(FolderAttr)
//						CommandProcTrn(NULL, "%s %03d %s", AskHostChmodCmd(), FolderAttrNum, TransPacketBase->RemoteFile);
						CommandProcTrn(TrnSkt, NULL, &Canceled[Pos->ThreadCount], "%s %03d %s", AskHostChmodCmd(), FolderAttrNum, Pos->RemoteFile);
				}
				ReleaseMutex(hListAccMutex);
			}
			/* ディレクトリ削除（常にホスト側） */
//			else if(strncmp(TransPacketBase->Cmd, "R-RMD", 5) == 0)
			else if(strncmp(Pos->Cmd, "R-RMD", 5) == 0)
			{
//				DispTransFileInfo(TransPacketBase, MSGJPN080, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN080, FALSE, YES);

//				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, TransPacketBase);
				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					/* フルパスを使わないための処理 */
//					if(MakeNonFullPath(TransPacketBase, CurDir, Tmp) == FFFTP_SUCCESS)
					if(MakeNonFullPath(Pos, CurDir[Pos->ThreadCount], Tmp) == FFFTP_SUCCESS)
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
//				DispTransFileInfo(TransPacketBase, MSGJPN081, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN081, FALSE, YES);

//				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, TransPacketBase);
				DelNotify = MirrorDelNotify(WIN_REMOTE, DelNotify, Pos);
				if((DelNotify == YES) || (DelNotify == YES_ALL))
				{
					/* フルパスを使わないための処理 */
//					if(MakeNonFullPath(TransPacketBase, CurDir, Tmp) == FFFTP_SUCCESS)
					if(MakeNonFullPath(Pos, CurDir[Pos->ThreadCount], Tmp) == FFFTP_SUCCESS)
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
//				DispTransFileInfo(TransPacketBase, MSGJPN082, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN082, FALSE, YES);

				Down = YES;
//				DoLocalMKD(TransPacketBase->LocalFile);
				DoLocalMKD(Pos->LocalFile);
				ReleaseMutex(hListAccMutex);
			}
			/* ディレクトリ削除（常にローカル側） */
//			else if(strncmp(TransPacketBase->Cmd, "L-RMD", 5) == 0)
			else if(strncmp(Pos->Cmd, "L-RMD", 5) == 0)
			{
//				DispTransFileInfo(TransPacketBase, MSGJPN083, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN083, FALSE, YES);

//				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, TransPacketBase);
				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, Pos);
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
//				DispTransFileInfo(TransPacketBase, MSGJPN084, FALSE, YES);
				DispTransFileInfo(Pos, MSGJPN084, FALSE, YES);

//				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, TransPacketBase);
				DelNotify = MirrorDelNotify(WIN_LOCAL, DelNotify, Pos);
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
			else if(strcmp(Pos->Cmd, "GOQUIT") == 0)
			{
				ReleaseMutex(hListAccMutex);
				GoExit = YES;
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
					EraseTransFileList();
					Pos = NULL;
				}
				else
				{
//					if((strncmp(TransPacketBase->Cmd, "RETR", 4) == 0) ||
//					   (strncmp(TransPacketBase->Cmd, "STOR", 4) == 0))
					if((strncmp(Pos->Cmd, "RETR", 4) == 0) ||
					   (strncmp(Pos->Cmd, "STOR", 4) == 0))
					{
//						TransFiles--;
						if(TransFiles > 0)
							TransFiles--;
						PostMessage(GetMainHwnd(), WM_CHANGE_COND, 0, 0);
					}
//					Pos = TransPacketBase;
//					TransPacketBase = TransPacketBase->Next;
//					free(Pos);
				}
				ClearAll = NO;
				ReleaseMutex(hListAccMutex);

				if(BackgrndMessageProc() == YES)
				{
					WaitForSingleObject(hListAccMutex, INFINITE);
					EraseTransFileList();
					ReleaseMutex(hListAccMutex);
				}
			}
			if(hWndTrans != NULL)
				SendMessage(hWndTrans, WM_SET_PACKET, 0, 0);
			if(Pos != NULL)
				strcpy(Pos->Cmd, "");
		}
//		else
		else if(TransPacketBase == NULL)
		{
			DelNotify = NO;

			ReleaseMutex(hListAccMutex);
			if(KeepDlg == NO)
			{
				if(hWndTrans != NULL)
				{
					DestroyWindow(hWndTrans);
					hWndTrans = NULL;

//					if(GoExit == YES)
//					{
//						SoundPlay(SND_TRANS);
//
//						if(AskAutoExit() == NO)
//						{
//							if(Down == YES)
//								PostMessage(GetMainHwnd(), WM_REFRESH_LOCAL_FLG, 0, 0);
//							if(Up == YES)
//								PostMessage(GetMainHwnd(), WM_REFRESH_REMOTE_FLG, 0, 0);
//						}
//						Down = NO;
//						Up = NO;
//					}
				}
			}
			BackgrndMessageProc();
//			Sleep(1);
			Sleep(100);

			if(GoExit == YES)
			{
				SoundPlay(SND_TRANS);
				if(AskAutoExit() == NO)
				{
					if(Down == YES)
						PostMessage(GetMainHwnd(), WM_REFRESH_LOCAL_FLG, 0, 0);
					if(Up == YES)
						PostMessage(GetMainHwnd(), WM_REFRESH_REMOTE_FLG, 0, 0);
				}
				Down = NO;
				Up = NO;
				PostMessage(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_AUTO_EXIT, 0), 0);
				GoExit = NO;
			}

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

// 同時接続対応
static int MakeNonFullPath(TRANSPACKET *Pkt, char *Cur, char *Tmp)
{
	int Sts;

//	Sts = ProcForNonFullpath(Pkt->RemoteFile, Cur, Pkt->hWndTrans, 1);
	Sts = ProcForNonFullpath(Pkt->ctrl_skt, Pkt->RemoteFile, Cur, Pkt->hWndTrans, &Canceled[Pkt->ThreadCount]);
	if(Sts == FFFTP_FAIL)
		ClearAll = YES;

	return(Sts);
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

int DoDownLoad(SOCKET cSkt, TRANSPACKET *Pkt, int DirList, int *CancelCheckWork)
{
	int iRetCode;
	char Reply[ERR_MSG_LEN+7];

	Pkt->ctrl_skt = cSkt;
	if(IsSpecialDevice(GetFileName(Pkt->LocalFile)) == YES)
	{
		iRetCode = 500;
		SetTaskMsg(MSGJPN085, GetFileName(Pkt->LocalFile));
		DispDownloadFinishMsg(Pkt, iRetCode);
	}
	else if(Pkt->Mode != EXIST_IGNORE)
	{
		if(Pkt->Type == TYPE_I)
			Pkt->KanjiCode = KANJI_NOCNV;

		iRetCode = command(Pkt->ctrl_skt, Reply, CancelCheckWork, "TYPE %c", Pkt->Type);
		if(iRetCode/100 < FTP_RETRY)
		{
			if(Pkt->hWndTrans != NULL)
			{
				// 同時接続対応
//				AllTransSizeNow = 0;
				AllTransSizeNow[Pkt->ThreadCount] = 0;

				if(DirList == NO)
					DispTransFileInfo(Pkt, MSGJPN086, TRUE, YES);
				else
					DispTransFileInfo(Pkt, MSGJPN087, FALSE, NO);
			}

			if(BackgrndMessageProc() == NO)
			{
				if(AskPasvMode() != YES)
					iRetCode = DownLoadNonPassive(Pkt, CancelCheckWork);
				else
					iRetCode = DownLoadPassive(Pkt, CancelCheckWork);
			}
			else
				iRetCode = 500;
		}
		else
			SetErrorMsg(Reply);
	}
	else
	{
		DispTransFileInfo(Pkt, MSGJPN088, TRUE, YES);
		SetTaskMsg(MSGJPN089, Pkt->RemoteFile);
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

static int DownLoadNonPassive(TRANSPACKET *Pkt, int *CancelCheckWork)
{
	int iRetCode;
	int iLength;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	SOCKET listen_socket = INVALID_SOCKET; // data listen socket
	char Buf[1024];
	int CreateMode;
	// IPv6対応
//	struct sockaddr_in saSockAddr1;
	struct sockaddr_in saSockAddrIPv4;
	struct sockaddr_in6 saSockAddrIPv6;
	char Reply[ERR_MSG_LEN+7];

	if((listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, CancelCheckWork)) != INVALID_SOCKET)
	{
		if(SetDownloadResume(Pkt, Pkt->Mode, Pkt->ExistSize, &CreateMode, CancelCheckWork) == YES)
		{
			sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
			iRetCode = command(Pkt->ctrl_skt, Reply, CancelCheckWork, "%s", Buf);
			if(iRetCode/100 == FTP_PRELIM)
			{
				// 同時接続対応
//				if(SocksGet2ndBindReply(listen_socket, &data_socket) == FFFTP_FAIL)
				if(SocksGet2ndBindReply(listen_socket, &data_socket, CancelCheckWork) == FFFTP_FAIL)
				{
					// IPv6対応
//					iLength = sizeof(saSockAddr1);
//					data_socket = do_accept(listen_socket, (struct sockaddr *)&saSockAddr1, (int *)&iLength);
					switch(AskCurNetType())
					{
					case NTYPE_IPV4:
						iLength=sizeof(saSockAddrIPv4);
						data_socket = do_accept(listen_socket,(struct sockaddr *)&saSockAddrIPv4, (int *)&iLength);
						break;
					case NTYPE_IPV6:
						iLength=sizeof(saSockAddrIPv6);
						data_socket = do_accept(listen_socket,(struct sockaddr *)&saSockAddrIPv6, (int *)&iLength);
						break;
					}

					if(shutdown(listen_socket, 1) != 0)
						ReportWSError("shutdown listen", WSAGetLastError());
					listen_socket = DoClose(listen_socket);

					if(data_socket == INVALID_SOCKET)
					{
						SetErrorMsg(MSGJPN280);
						ReportWSError("accept", WSAGetLastError());
						iRetCode = 500;
					}
					else
						// IPv6対応
//						DoPrintf("Skt=%u : accept from %s port %u", data_socket, inet_ntoa(saSockAddr1.sin_addr), ntohs(saSockAddr1.sin_port));
					{
						switch(AskCurNetType())
						{
						case NTYPE_IPV4:
							DoPrintf("Skt=%u : accept from %s port %u", data_socket, inet_ntoa(saSockAddrIPv4.sin_addr), ntohs(saSockAddrIPv4.sin_port));
							break;
						case NTYPE_IPV6:
							DoPrintf("Skt=%u : accept from %s port %u", data_socket, inet6_ntoa(saSockAddrIPv6.sin6_addr), ntohs(saSockAddrIPv6.sin6_port));
							break;
						}
					}
				}

				if(data_socket != INVALID_SOCKET)
				{
					// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
					ReleaseMutex(hListAccMutex);
					// FTPS対応
//					iRetCode = DownLoadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
					if(IsSSLAttached(Pkt->ctrl_skt))
					{
						if(AttachSSL(data_socket, Pkt->ctrl_skt, CancelCheckWork))
							iRetCode = DownLoadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
						else
							iRetCode = 500;
					}
					else
						iRetCode = DownLoadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
//					data_socket = DoClose(data_socket);
				}
			}
			else
			{
				SetErrorMsg(Reply);
				SetTaskMsg(MSGJPN090);
				listen_socket = DoClose(listen_socket);
				iRetCode = 500;
			}
		}
		else
			iRetCode = 500;
	}
	else
	{
		iRetCode = 500;
		SetErrorMsg(MSGJPN279);
	}
	DispDownloadFinishMsg(Pkt, iRetCode);

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

static int DownLoadPassive(TRANSPACKET *Pkt, int *CancelCheckWork)
{
	int iRetCode;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	char Buf[1024];
	int CreateMode;
	// IPv6対応
//	char Adrs[20];
	char Adrs[40];
	int Port;
	int Flg;
	char Reply[ERR_MSG_LEN+7];

	// IPv6対応
//	iRetCode = command(Pkt->ctrl_skt, Buf, CancelCheckWork, "PASV");
	switch(AskCurNetType())
	{
	case NTYPE_IPV4:
		iRetCode = command(Pkt->ctrl_skt, Buf, CancelCheckWork, "PASV");
		break;
	case NTYPE_IPV6:
		iRetCode = command(Pkt->ctrl_skt, Buf, CancelCheckWork, "EPSV");
		break;
	}
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
//						iRetCode = DownLoadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
						if(IsSSLAttached(Pkt->ctrl_skt))
						{
							if(AttachSSL(data_socket, Pkt->ctrl_skt, CancelCheckWork))
								iRetCode = DownLoadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
							else
								iRetCode = 500;
						}
						else
							iRetCode = DownLoadFile(Pkt, data_socket, CreateMode, CancelCheckWork);
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

	DispDownloadFinishMsg(Pkt, iRetCode);

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

static int DownLoadFile(TRANSPACKET *Pkt, SOCKET dSkt, int CreateMode, int *CancelCheckWork)
{
	int iNumBytes;
	char Buf[BUFSIZE];
	char Buf2[BUFSIZE+3];
	HANDLE iFileHandle;
	SECURITY_ATTRIBUTES Sec;
	DWORD Writed;
	CODECONVINFO cInfo;
	int Continue;
//	fd_set ReadFds;
//	struct timeval Tout;
//	struct timeval *ToutPtr;
	int iRetCode;
	int TimeOutErr;
	char TmpBuf[ONELINE_BUF_SIZE];
	DWORD dwFileAttributes;

#ifdef SET_BUFFER_SIZE
/* Add by H.Shirouzu at 2002/10/02 */
	int buf_size = SOCKBUF_SIZE;
	for ( ; buf_size > 0; buf_size /= 2)
		if (setsockopt(dSkt, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(buf_size)) == 0)
			break;
/* End */
#endif

	// 念のため受信バッファを無効にする
#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
	int buf_size = 0;
	setsockopt(dSkt, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(buf_size));
#endif

	Pkt->Abort = ABORT_NONE;

	Sec.nLength = sizeof(SECURITY_ATTRIBUTES);
	Sec.lpSecurityDescriptor = NULL;
	Sec.bInheritHandle = FALSE;

	dwFileAttributes = GetFileAttributes(Pkt->LocalFile);
	if (dwFileAttributes != INVALID_FILE_ATTRIBUTES && (dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
		// 読み取り専用
		if (MessageBox(GetMainHwnd(), MSGJPN296, MSGJPN086, MB_YESNO) == IDYES) {
			// 属性を解除
			SetFileAttributes(Pkt->LocalFile, dwFileAttributes ^ FILE_ATTRIBUTE_READONLY);
		}
	}

	if((iFileHandle = CreateFile(Pkt->LocalFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &Sec, CreateMode, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		// UTF-8対応
		char Buf3[(BUFSIZE + 3) * 4];
		CODECONVINFO cInfo2;
		int ProcessedBOM = NO;
		// 4GB超対応（kaokunさん提供）
		DWORD High = 0;
		if(CreateMode == OPEN_ALWAYS)
			// 4GB超対応（kaokunさん提供）
//			SetFilePointer(iFileHandle, 0, 0, FILE_END);
			SetFilePointer(iFileHandle, 0, &High, FILE_END);

		if(Pkt->hWndTrans != NULL)
		{
			// 同時接続対応
//			TimeStart = time(NULL);
			TimeStart[Pkt->ThreadCount] = time(NULL);
			SetTimer(Pkt->hWndTrans, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
		}

		InitCodeConvInfo(&cInfo);
		cInfo.KanaCnv = Pkt->KanaCnv;

		InitCodeConvInfo(&cInfo2);
		cInfo2.KanaCnv = Pkt->KanaCnv;

		/*===== ファイルを受信するループ =====*/
		while((Pkt->Abort == ABORT_NONE) && (ForceAbort == NO))
		{
//			FD_ZERO(&ReadFds);
//			FD_SET(dSkt, &ReadFds);
//			ToutPtr = NULL;
//			if(TimeOut != 0)
//			{
//				Tout.tv_sec = TimeOut;
//				Tout.tv_usec = 0;
//				ToutPtr = &Tout;
//			}
//			iNumBytes = select(0, &ReadFds, NULL, NULL, ToutPtr);
//			if(iNumBytes == SOCKET_ERROR)
//			{
//				ReportWSError("select", WSAGetLastError());
//				if(Pkt->Abort == ABORT_NONE)
//					Pkt->Abort = ABORT_ERROR;
//				break;
//			}
//			else if(iNumBytes == 0)
//			{
//				SetErrorMsg(MSGJPN094);
//				SetTaskMsg(MSGJPN094);
//				Pkt->Abort = ABORT_ERROR;
//				break;
//			}

			if((iNumBytes = do_recv(dSkt, Buf, BUFSIZE, 0, &TimeOutErr, CancelCheckWork)) <= 0)
			{
				if(TimeOutErr == YES)
				{
					SetErrorMsg(MSGJPN094);
					SetTaskMsg(MSGJPN094);
					if(Pkt->hWndTrans != NULL)
						ClearAll = YES;
					if(Pkt->Abort == ABORT_NONE)
						Pkt->Abort = ABORT_ERROR;
				}
				else if(iNumBytes == SOCKET_ERROR)
				{
					if(Pkt->Abort == ABORT_NONE)
						Pkt->Abort = ABORT_ERROR;
				}
				break;
			}

			/* 漢字コード変換 */
			if(Pkt->KanjiCode != KANJI_NOCNV)
			{
				cInfo.Str = Buf;
				cInfo.StrLen = iNumBytes;
				cInfo.Buf = Buf2;
				cInfo.BufSize = BUFSIZE+3;
				do
				{
					// ここで全てUTF-8へ変換する
					// TODO: SJIS以外も直接UTF-8へ変換
//					if(Pkt->KanjiCode == KANJI_JIS)
//						Continue = ConvJIStoSJIS(&cInfo);
//					else
//						Continue = ConvEUCtoSJIS(&cInfo);
					char Buf3[(BUFSIZE + 3) * 4];
					CODECONVINFO cInfo2;
					switch(Pkt->KanjiCode)
					{
					case KANJI_SJIS:
						switch(Pkt->KanjiCodeDesired)
						{
						case KANJI_SJIS:
//							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
//							cInfo2.OutLen = cInfo.StrLen;
//							Continue = NO;
							// カナ変換のため
							Continue = ConvSJIStoJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvJIStoSJIS(&cInfo2);
							break;
						case KANJI_JIS:
							Continue = ConvSJIStoJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_EUC:
							Continue = ConvSJIStoEUC(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_UTF8N:
							Continue = ConvSJIStoUTF8N(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							Continue = ConvSJIStoUTF8N(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						}
						break;
					case KANJI_JIS:
						switch(Pkt->KanjiCodeDesired)
						{
						case KANJI_SJIS:
							Continue = ConvJIStoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
//							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
//							cInfo2.OutLen = cInfo.StrLen;
//							Continue = NO;
							// カナ変換のため
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						}
						break;
					case KANJI_EUC:
						switch(Pkt->KanjiCodeDesired)
						{
						case KANJI_SJIS:
							Continue = ConvEUCtoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
//							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
//							cInfo2.OutLen = cInfo.StrLen;
//							Continue = NO;
							// カナ変換のため
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						}
						break;
					case KANJI_UTF8N:
						switch(Pkt->KanjiCodeDesired)
						{
						case KANJI_SJIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						}
						break;
					case KANJI_UTF8BOM:
						if(ProcessedBOM == NO)
						{
							if(memcmp(Buf, "\xEF\xBB\xBF", 3) == 0)
							{
								cInfo.Str += 3;
								cInfo.StrLen -= 3;
							}
							cInfo2.OutLen = 0;
							switch(Pkt->KanjiCodeDesired)
							{
							case KANJI_UTF8BOM:
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								break;
							}
							Continue = YES;
							ProcessedBOM = YES;
							break;
						}
						switch(Pkt->KanjiCodeDesired)
						{
						case KANJI_SJIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						case KANJI_UTF8BOM:
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						}
						break;
					}
//					if(WriteFile(iFileHandle, Buf2, cInfo.OutLen, &Writed, NULL) == FALSE)
					if(WriteFile(iFileHandle, Buf3, cInfo2.OutLen, &Writed, NULL) == FALSE)
						Pkt->Abort = ABORT_DISKFULL;
				}
				while((Continue == YES) && (Pkt->Abort == ABORT_NONE));
			}
			else
			{
				if(WriteFile(iFileHandle, Buf, iNumBytes, &Writed, NULL) == FALSE)
					Pkt->Abort = ABORT_DISKFULL;
			}

			Pkt->ExistSize += iNumBytes;
			if(Pkt->hWndTrans != NULL)
				// 同時接続対応
//				AllTransSizeNow += iNumBytes;
				AllTransSizeNow[Pkt->ThreadCount] += iNumBytes;
			else
			{
				/* 転送ダイアログを出さない時の経過表示 */
				DispDownloadSize(Pkt->ExistSize);
			}

			if(BackgrndMessageProc() == YES)
				ForceAbort = YES;
		}

		/* 書き残したデータを書き込む */
		if(Pkt->KanjiCode != KANJI_NOCNV)
		{
			cInfo.Buf = Buf2;
			cInfo.BufSize = BUFSIZE+3;
			FlushRestData(&cInfo);
			switch(Pkt->KanjiCode)
			{
			case KANJI_SJIS:
				switch(Pkt->KanjiCodeDesired)
				{
				case KANJI_SJIS:
					// カナ変換のため
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvJIStoSJIS(&cInfo2);
					break;
				case KANJI_JIS:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_EUC:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_UTF8N:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_UTF8BOM:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				}
				break;
			case KANJI_JIS:
				switch(Pkt->KanjiCodeDesired)
				{
				case KANJI_SJIS:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_JIS:
					// カナ変換のため
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoJIS(&cInfo2);
					break;
				case KANJI_EUC:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoEUC(&cInfo2);
					break;
				case KANJI_UTF8N:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoUTF8N(&cInfo2);
					break;
				case KANJI_UTF8BOM:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoUTF8N(&cInfo2);
					break;
				}
				break;
			case KANJI_EUC:
				switch(Pkt->KanjiCodeDesired)
				{
				case KANJI_SJIS:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_JIS:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoJIS(&cInfo2);
					break;
				case KANJI_EUC:
					// カナ変換のため
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoEUC(&cInfo2);
					break;
				case KANJI_UTF8N:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoUTF8N(&cInfo2);
					break;
				case KANJI_UTF8BOM:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoUTF8N(&cInfo2);
					break;
				}
				break;
			case KANJI_UTF8N:
				switch(Pkt->KanjiCodeDesired)
				{
				case KANJI_SJIS:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_JIS:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoJIS(&cInfo2);
					break;
				case KANJI_EUC:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoEUC(&cInfo2);
					break;
				case KANJI_UTF8N:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_UTF8BOM:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				}
				break;
			case KANJI_UTF8BOM:
				switch(Pkt->KanjiCodeDesired)
				{
				case KANJI_SJIS:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_JIS:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoJIS(&cInfo2);
					break;
				case KANJI_EUC:
					cInfo2.Str = cInfo.Buf;
					cInfo2.StrLen = cInfo.OutLen;
					cInfo2.Buf = Buf3;
					cInfo2.BufSize = (BUFSIZE + 3) * 4;
					ConvSJIStoEUC(&cInfo2);
					break;
				case KANJI_UTF8N:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				case KANJI_UTF8BOM:
					memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
					cInfo2.OutLen = cInfo.OutLen;
					break;
				}
				break;
			}
//			if(WriteFile(iFileHandle, Buf2, cInfo.OutLen, &Writed, NULL) == FALSE)
			if(WriteFile(iFileHandle, Buf3, cInfo2.OutLen, &Writed, NULL) == FALSE)
				Pkt->Abort = ABORT_DISKFULL;
			cInfo2.Buf = Buf3;
			cInfo2.BufSize = (BUFSIZE + 3) * 4;
			FlushRestData(&cInfo2);
			if(WriteFile(iFileHandle, Buf3, cInfo2.OutLen, &Writed, NULL) == FALSE)
				Pkt->Abort = ABORT_DISKFULL;
		}

		/* グラフ表示を更新 */
		if(Pkt->hWndTrans != NULL)
		{
			KillTimer(Pkt->hWndTrans, TIMER_DISPLAY);
			DispTransferStatus(Pkt->hWndTrans, YES, Pkt);
			// 同時接続対応
//			TimeStart = time(NULL) - TimeStart + 1;
			TimeStart[Pkt->ThreadCount] = time(NULL) - TimeStart[Pkt->ThreadCount] + 1;
		}
		else
		{
			/* 転送ダイアログを出さない時の経過表示を消す */
			DispDownloadSize(-1);
		}

		/* ファイルのタイムスタンプを合わせる */
		if((SaveTimeStamp == YES) &&
		   ((Pkt->Time.dwLowDateTime != 0) || (Pkt->Time.dwHighDateTime != 0)))
		{
			SetFileTime(iFileHandle, &Pkt->Time, &Pkt->Time, &Pkt->Time);
		}

		CloseHandle(iFileHandle);

		if(iNumBytes == SOCKET_ERROR)
			ReportWSError("recv",WSAGetLastError());
	}
	else
	{
		SetErrorMsg(MSGJPN095, Pkt->LocalFile);
		SetTaskMsg(MSGJPN095, Pkt->LocalFile);
		Pkt->Abort = ABORT_ERROR;
	}


	if(shutdown(dSkt, 1) != 0)
		ReportWSError("shutdown", WSAGetLastError());

	DoClose(dSkt);

	if(ForceAbort == NO)
	{
		/* Abortをホストに伝える */
		if(Pkt->Abort != ABORT_NONE && iFileHandle != INVALID_HANDLE_VALUE)
		{
			SendData(Pkt->ctrl_skt, "\xFF\xF4\xFF", 3, MSG_OOB, CancelCheckWork);	/* MSG_OOBに注意 */
			SendData(Pkt->ctrl_skt, "\xF2", 1, 0, CancelCheckWork);
			command(Pkt->ctrl_skt, NULL, CancelCheckWork, "ABOR");
		}
	}

	iRetCode = ReadReplyMessage(Pkt->ctrl_skt, Buf, 1024, CancelCheckWork, TmpBuf);

//#pragma aaa
//DoPrintf("##DOWN REPLY : %s", Buf);

	if(Pkt->Abort == ABORT_DISKFULL)
	{
		SetErrorMsg(MSGJPN096);
		SetTaskMsg(MSGJPN096);
	}
	if(iRetCode >= FTP_RETRY)
		SetErrorMsg(Buf);
	if(Pkt->Abort != ABORT_NONE)
		iRetCode = 500;

	return(iRetCode);
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

			if((strncmp(Pkt->Cmd, "NLST", 4) == 0) || (strncmp(Pkt->Cmd, "LIST", 4) == 0))
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
					if(TransferErrorNotify == YES && DispUpDownErrDialog(downerr_dlg, Pkt->hWndTrans, Pkt) == NO)
						ClearAll = YES;
					else
					{
						Pkt->Mode = TransferErrorMode;
						AddTransFileList(Pkt);
					}
				}
			}
		}
		else
		{
			if((strncmp(Pkt->Cmd, "NLST", 4) == 0) || (strncmp(Pkt->Cmd, "LIST", 4) == 0))
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


/*----- ダウンロード／アップロードエラーのダイアログを表示 --------------------
*
*	Parameter
*		int RedID : ダイアログボックスのリソースID
*		HWND hWnd : 書き込み中ダイアログのウインドウ
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス (YES=中止/NO=全て中止)
*----------------------------------------------------------------------------*/

// 再転送対応
//static int DispUpDownErrDialog(int ResID, HWND hWnd, char *Fname)
static int DispUpDownErrDialog(int ResID, HWND hWnd, TRANSPACKET *Pkt)
{
	if(hWnd == NULL)
		hWnd = GetMainHwnd();

	SoundPlay(SND_ERROR);
	// 再転送対応
//	return(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(ResID), hWnd, UpDownErrorDialogProc, (LPARAM)Fname));
	return(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(ResID), hWnd, UpDownErrorDialogProc, (LPARAM)Pkt));
}


/*----- ダウンロードエラー／アップロードエラーダイアログのコールバック --------
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

static BOOL CALLBACK UpDownErrorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TRANSPACKET *Pkt;
	static const RADIOBUTTON DownExistButton[] = {
		{ DOWN_EXIST_OVW, EXIST_OVW },
		{ DOWN_EXIST_RESUME, EXIST_RESUME },
		{ DOWN_EXIST_IGNORE, EXIST_IGNORE }
	};
	#define DOWNEXISTBUTTONS	(sizeof(DownExistButton)/sizeof(RADIOBUTTON))

	switch (message)
	{
		case WM_INITDIALOG :
			Pkt = (TRANSPACKET *)lParam;
//			SendDlgItemMessage(hDlg, UPDOWN_ERR_FNAME, WM_SETTEXT, 0, (LPARAM)lParam);
			SendDlgItemMessage(hDlg, UPDOWN_ERR_FNAME, WM_SETTEXT, 0, (LPARAM)Pkt->RemoteFile);
			// 同時接続対応
//			SendDlgItemMessage(hDlg, UPDOWN_ERR_MSG, WM_SETTEXT, 0, (LPARAM)ErrMsg);
			SendDlgItemMessage(hDlg, UPDOWN_ERR_MSG, WM_SETTEXT, 0, (LPARAM)GetErrMsg());

			if((Pkt->Type == TYPE_A) || (Pkt->ExistSize <= 0))
				EnableWindow(GetDlgItem(hDlg, DOWN_EXIST_RESUME), FALSE);

			SetRadioButtonByValue(hDlg, TransferErrorMode, DownExistButton, DOWNEXISTBUTTONS);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK_ALL :
					TransferErrorNotify = NO;
					/* ここに break はない */

				case IDOK :
					TransferErrorMode = AskRadioButtonValue(hDlg, DownExistButton, DOWNEXISTBUTTONS);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case IDHELP :
//					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000009);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
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

static int SetDownloadResume(TRANSPACKET *Pkt, int ProcMode, LONGLONG Size, int *Mode, int *CancelCheckWork)
{
	int iRetCode;
	int Com;
	char Reply[ERR_MSG_LEN+7];
	char Tmp[40];

	Com = YES;

	Pkt->ExistSize = 0;
	*Mode = CREATE_ALWAYS;

	if(ProcMode == EXIST_RESUME)
	{
		iRetCode = command(Pkt->ctrl_skt, Reply, CancelCheckWork, "REST %s", MakeNumString(Size, Tmp, FALSE));
		if(iRetCode/100 < FTP_RETRY)
		{
			/* リジューム */
			if(Pkt->hWndTrans != NULL)
				Pkt->ExistSize = Size;
			*Mode = OPEN_ALWAYS;
		}
		else
		{
			Com = DialogBox(GetFtpInst(), MAKEINTRESOURCE(noresume_dlg), Pkt->hWndTrans, NoResumeWndProc);
			if(Com != YES)
			{
				if(Com == NO_ALL)		/* 全て中止 */
					ClearAll = YES;
				Pkt->Abort = ABORT_USER;
			}
		}
	}
	return(Com);
}


/*----- resumeエラーダイアログのコールバック ----------------------------------
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

static BOOL CALLBACK NoResumeWndProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
		case WM_INITDIALOG :
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case RESUME_CANCEL_ALL :
					EndDialog(hDlg, NO_ALL);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
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

static int DoUpLoad(SOCKET cSkt, TRANSPACKET *Pkt)
{
	int iRetCode;
	char Reply[ERR_MSG_LEN+7];

	Pkt->ctrl_skt = cSkt;

	if(Pkt->Mode != EXIST_IGNORE)
	{
		if(CheckFileReadable(Pkt->LocalFile) == FFFTP_SUCCESS)
		{
			if(Pkt->Type == TYPE_I)
				Pkt->KanjiCode = KANJI_NOCNV;

			// 同時接続対応
//			iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled, "TYPE %c", Pkt->Type);
			iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled[Pkt->ThreadCount], "TYPE %c", Pkt->Type);
			if(iRetCode/100 < FTP_RETRY)
			{
				if(Pkt->Mode == EXIST_UNIQUE)
					strcpy(Pkt->Cmd, "STOU ");

				if(Pkt->hWndTrans != NULL)
					DispTransFileInfo(Pkt, MSGJPN104, TRUE, YES);

				if(BackgrndMessageProc() == NO)
				{
					if(AskPasvMode() != YES)
						iRetCode = UpLoadNonPassive(Pkt);
					else
						iRetCode = UpLoadPassive(Pkt);
				}
				else
					iRetCode = 500;
			}
			else
				SetErrorMsg(Reply);

			/* 属性変更 */
			if((Pkt->Attr != -1) && ((iRetCode/100) == FTP_COMPLETE))
				// 同時接続対応
//				command(Pkt->ctrl_skt, Reply, &Canceled, "%s %03X %s", AskHostChmodCmd(), Pkt->Attr, Pkt->RemoteFile);
				command(Pkt->ctrl_skt, Reply, &Canceled[Pkt->ThreadCount], "%s %03X %s", AskHostChmodCmd(), Pkt->Attr, Pkt->RemoteFile);
		}
		else
		{
			SetErrorMsg(MSGJPN105, Pkt->LocalFile);
			SetTaskMsg(MSGJPN105, Pkt->LocalFile);
			iRetCode = 500;
			Pkt->Abort = ABORT_ERROR;
			DispUploadFinishMsg(Pkt, iRetCode);
		}
	}
	else
	{
		DispTransFileInfo(Pkt, MSGJPN106, TRUE, YES);
		SetTaskMsg(MSGJPN107, Pkt->LocalFile);
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

static int UpLoadNonPassive(TRANSPACKET *Pkt)
{
	int iRetCode;
	int iLength;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	SOCKET listen_socket = INVALID_SOCKET; // data listen socket
	char Buf[1024];
	// IPv6対応
//	struct sockaddr_in saSockAddr1;
	struct sockaddr_in saSockAddrIPv4;
	struct sockaddr_in6 saSockAddrIPv6;
	int Resume;
	char Reply[ERR_MSG_LEN+7];

	// 同時接続対応
//	if((listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, &Canceled)) != INVALID_SOCKET)
	if((listen_socket = GetFTPListenSocket(Pkt->ctrl_skt, &Canceled[Pkt->ThreadCount])) != INVALID_SOCKET)
	{
		SetUploadResume(Pkt, Pkt->Mode, Pkt->ExistSize, &Resume);
		if(Resume == NO)
			sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
		else
			sprintf(Buf, "%s%s", "APPE ", Pkt->RemoteFile);

		// 同時接続対応
//		iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled, "%s", Buf);
		iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled[Pkt->ThreadCount], "%s", Buf);
		if((iRetCode/100) == FTP_PRELIM)
		{
			// 同時接続対応
//			if(SocksGet2ndBindReply(listen_socket, &data_socket) == FFFTP_FAIL)
			if(SocksGet2ndBindReply(listen_socket, &data_socket, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
			{
				// IPv6対応
//				iLength=sizeof(saSockAddr1);
//				data_socket = do_accept(listen_socket,(struct sockaddr *)&saSockAddr1, (int *)&iLength);
				switch(AskCurNetType())
				{
				case NTYPE_IPV4:
					iLength=sizeof(saSockAddrIPv4);
					data_socket = do_accept(listen_socket,(struct sockaddr *)&saSockAddrIPv4, (int *)&iLength);
					break;
				case NTYPE_IPV6:
					iLength=sizeof(saSockAddrIPv6);
					data_socket = do_accept(listen_socket,(struct sockaddr *)&saSockAddrIPv6, (int *)&iLength);
					break;
				}

				if(shutdown(listen_socket, 1) != 0)
					ReportWSError("shutdown listen", WSAGetLastError());
				listen_socket = DoClose(listen_socket);

				if(data_socket == INVALID_SOCKET)
				{
					SetErrorMsg(MSGJPN280);
					ReportWSError("accept", WSAGetLastError());
					iRetCode = 500;
				}
				else
					// IPv6対応
//					DoPrintf("Skt=%u : accept from %s port %u", data_socket, inet_ntoa(saSockAddr1.sin_addr), ntohs(saSockAddr1.sin_port));
				{
					switch(AskCurNetType())
					{
					case NTYPE_IPV4:
						DoPrintf("Skt=%u : accept from %s port %u", data_socket, inet_ntoa(saSockAddrIPv4.sin_addr), ntohs(saSockAddrIPv4.sin_port));
						break;
					case NTYPE_IPV6:
						DoPrintf("Skt=%u : accept from %s port %u", data_socket, inet6_ntoa(saSockAddrIPv6.sin6_addr), ntohs(saSockAddrIPv6.sin6_port));
						break;
					}
				}
			}

			if(data_socket != INVALID_SOCKET)
			{
				// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
				ReleaseMutex(hListAccMutex);
				// FTPS対応
//				iRetCode = UpLoadFile(Pkt, data_socket);
				if(IsSSLAttached(Pkt->ctrl_skt))
				{
					if(AttachSSL(data_socket, Pkt->ctrl_skt, &Canceled[Pkt->ThreadCount]))
						iRetCode = UpLoadFile(Pkt, data_socket);
					else
						iRetCode = 500;
				}
				else
					iRetCode = UpLoadFile(Pkt, data_socket);
				data_socket = DoClose(data_socket);
			}
		}
		else
		{
			SetErrorMsg(Reply);
			SetTaskMsg(MSGJPN108);
			listen_socket = DoClose(listen_socket);
			iRetCode = 500;
		}
	}
	else
	{
		SetErrorMsg(MSGJPN279);
		iRetCode = 500;
	}
	DispUploadFinishMsg(Pkt, iRetCode);

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

static int UpLoadPassive(TRANSPACKET *Pkt)
{
	int iRetCode;
	SOCKET data_socket = INVALID_SOCKET;   // data channel socket
	char Buf[1024];
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
					sprintf(Buf, "%s%s", Pkt->Cmd, Pkt->RemoteFile);
				else
					sprintf(Buf, "%s%s", "APPE ", Pkt->RemoteFile);

				// 同時接続対応
//				iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled, "%s", Buf);
				iRetCode = command(Pkt->ctrl_skt, Reply, &Canceled[Pkt->ThreadCount], "%s", Buf);
				if(iRetCode/100 == FTP_PRELIM)
				{
					// 一部TYPE、STOR(RETR)、PORT(PASV)を並列に処理できないホストがあるため
					ReleaseMutex(hListAccMutex);
					// FTPS対応
//					iRetCode = UpLoadFile(Pkt, data_socket);
					if(IsSSLAttached(Pkt->ctrl_skt))
					{
						if(AttachSSL(data_socket, Pkt->ctrl_skt, &Canceled[Pkt->ThreadCount]))
							iRetCode = UpLoadFile(Pkt, data_socket);
						else
							iRetCode = 500;
					}
					else
						iRetCode = UpLoadFile(Pkt, data_socket);

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

	DispUploadFinishMsg(Pkt, iRetCode);

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

static int UpLoadFile(TRANSPACKET *Pkt, SOCKET dSkt)
{
	DWORD iNumBytes;
	HANDLE iFileHandle;
	SECURITY_ATTRIBUTES Sec;
	char Buf[BUFSIZE];
	char Buf2[BUFSIZE+3];
	CODECONVINFO cInfo;
	TERMCODECONVINFO tInfo;
	int Continue;
	char *EofPos;
	int iRetCode;
#if 0
	int TimeOutErr;
#endif
	char TmpBuf[ONELINE_BUF_SIZE];
	DWORD Low;
	DWORD High;

#ifdef SET_BUFFER_SIZE
/* Add by H.Shirouzu at 2002/10/02 */
	int buf_size = SOCKBUF_SIZE;
	for ( ; buf_size > 0; buf_size /= 2)
		if (setsockopt(dSkt, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(buf_size)) == 0)
			break;
/* End */
#endif

	// 念のため送信バッファを無効にする
#ifdef DISABLE_TRANSFER_NETWORK_BUFFERS
	int buf_size = 0;
	setsockopt(dSkt, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(buf_size));
#endif

	Pkt->Abort = ABORT_NONE;

	Sec.nLength = sizeof(SECURITY_ATTRIBUTES);
	Sec.lpSecurityDescriptor = NULL;
	Sec.bInheritHandle = FALSE;

	if((iFileHandle = CreateFile(Pkt->LocalFile, GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE, &Sec, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		// UTF-8対応
		char Buf3[(BUFSIZE + 3) * 4];
		CODECONVINFO cInfo2;
		int ProcessedBOM = NO;
		if(Pkt->hWndTrans != NULL)
		{
			Low = GetFileSize(iFileHandle, &High);
			Pkt->Size = MakeLongLong(High, Low);

			High = (DWORD)HIGH32(Pkt->ExistSize);
			Low = (DWORD)LOW32(Pkt->ExistSize);
			SetFilePointer(iFileHandle, Low, &High, FILE_BEGIN);

			// 同時接続対応
//			AllTransSizeNow = 0;
//			TimeStart = time(NULL);
			AllTransSizeNow[Pkt->ThreadCount] = 0;
			TimeStart[Pkt->ThreadCount] = time(NULL);
			SetTimer(Pkt->hWndTrans, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
		}

		InitCodeConvInfo(&cInfo);
		cInfo.KanaCnv = Pkt->KanaCnv;
		InitTermCodeConvInfo(&tInfo);

		InitCodeConvInfo(&cInfo2);
		cInfo2.KanaCnv = Pkt->KanaCnv;

		/*===== ファイルを送信するループ =====*/
		while((Pkt->Abort == ABORT_NONE) &&
			  (ForceAbort == NO) &&
			  (ReadFile(iFileHandle, Buf, BUFSIZE, &iNumBytes, NULL) == TRUE))
		{
			if(iNumBytes == 0)
				break;

			/* EOF除去 */
			EofPos = NULL;
			if((RmEOF == YES) && (Pkt->Type == TYPE_A))
			{
				if((EofPos = memchr(Buf, 0x1A, iNumBytes)) != NULL)
					iNumBytes = EofPos - Buf;
			}

			/* 漢字コード変換 */
			if(Pkt->KanjiCode != KANJI_NOCNV)
			{
				cInfo.Str = Buf;
				cInfo.StrLen = iNumBytes;
				cInfo.Buf = Buf2;
				cInfo.BufSize = BUFSIZE+3;
				do
				{
					// ここで全てUTF-8へ変換する
					// TODO: SJIS以外も直接UTF-8へ変換
//					if(Pkt->KanjiCode == KANJI_JIS)
//						Continue = ConvSJIStoJIS(&cInfo);
//					else
//						Continue = ConvSJIStoEUC(&cInfo);
					switch(Pkt->KanjiCodeDesired)
					{
					case KANJI_SJIS:
						switch(Pkt->KanjiCode)
						{
						case KANJI_SJIS:
//							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
//							cInfo2.OutLen = cInfo.StrLen;
//							Continue = NO;
							// カナ変換のため
							Continue = ConvSJIStoJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvJIStoSJIS(&cInfo2);
							break;
						case KANJI_JIS:
							Continue = ConvSJIStoJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_EUC:
							Continue = ConvSJIStoEUC(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_UTF8N:
							Continue = ConvSJIStoUTF8N(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							Continue = ConvSJIStoUTF8N(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						}
						break;
					case KANJI_JIS:
						switch(Pkt->KanjiCode)
						{
						case KANJI_SJIS:
							Continue = ConvJIStoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
//							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
//							cInfo2.OutLen = cInfo.StrLen;
//							Continue = NO;
							// カナ変換のため
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							Continue = ConvJIStoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						}
						break;
					case KANJI_EUC:
						switch(Pkt->KanjiCode)
						{
						case KANJI_SJIS:
							Continue = ConvEUCtoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
//							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
//							cInfo2.OutLen = cInfo.StrLen;
//							Continue = NO;
							// カナ変換のため
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							Continue = ConvEUCtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoUTF8N(&cInfo2);
							break;
						}
						break;
					case KANJI_UTF8N:
						switch(Pkt->KanjiCode)
						{
						case KANJI_SJIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						case KANJI_UTF8BOM:
							if(ProcessedBOM == NO)
							{
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								Continue = YES;
								ProcessedBOM = YES;
								break;
							}
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						}
						break;
					case KANJI_UTF8BOM:
						if(ProcessedBOM == NO)
						{
							if(memcmp(Buf, "\xEF\xBB\xBF", 3) == 0)
							{
								cInfo.Str += 3;
								cInfo.StrLen -= 3;
							}
							cInfo2.OutLen = 0;
							switch(Pkt->KanjiCode)
							{
							case KANJI_UTF8BOM:
								memcpy(Buf3, "\xEF\xBB\xBF", 3);
								cInfo2.OutLen = 3;
								break;
							}
							Continue = YES;
							ProcessedBOM = YES;
							break;
						}
						switch(Pkt->KanjiCode)
						{
						case KANJI_SJIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
							cInfo2.OutLen = cInfo.OutLen;
							break;
						case KANJI_JIS:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoJIS(&cInfo2);
							break;
						case KANJI_EUC:
							Continue = ConvUTF8NtoSJIS(&cInfo);
							cInfo2.Str = cInfo.Buf;
							cInfo2.StrLen = cInfo.OutLen;
							cInfo2.Buf = Buf3;
							cInfo2.BufSize = (BUFSIZE + 3) * 4;
							ConvSJIStoEUC(&cInfo2);
							break;
						case KANJI_UTF8N:
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						case KANJI_UTF8BOM:
							memcpy(Buf3, cInfo.Str, cInfo.StrLen);
							cInfo2.OutLen = cInfo.StrLen;
							Continue = NO;
							break;
						}
						break;
					}

//					if(TermCodeConvAndSend(&tInfo, dSkt, Buf2, cInfo.OutLen, Pkt->Type) == FFFTP_FAIL)
					if(TermCodeConvAndSend(&tInfo, dSkt, Buf3, cInfo2.OutLen, Pkt->Type, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
					{
						Pkt->Abort = ABORT_ERROR;
							break;
					}
				}
				while(Continue == YES);
			}
			else
			{
				// 同時接続対応
//				if(TermCodeConvAndSend(&tInfo, dSkt, Buf, iNumBytes, Pkt->Type) == FFFTP_FAIL)
				if(TermCodeConvAndSend(&tInfo, dSkt, Buf, iNumBytes, Pkt->Type, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
					Pkt->Abort = ABORT_ERROR;
			}

			Pkt->ExistSize += iNumBytes;
			if(Pkt->hWndTrans != NULL)
				// 同時接続対応
//				AllTransSizeNow += iNumBytes;
				AllTransSizeNow[Pkt->ThreadCount] += iNumBytes;

			if(BackgrndMessageProc() == YES)
				ForceAbort = YES;

			if(EofPos != NULL)
				break;
		}

		if((ForceAbort == NO) && (Pkt->Abort == ABORT_NONE))
		{
			/* 送り残したデータを送信 */
			if(Pkt->KanjiCode != KANJI_NOCNV)
			{
				cInfo.Buf = Buf2;
				cInfo.BufSize = BUFSIZE+3;
				FlushRestData(&cInfo);
				switch(Pkt->KanjiCodeDesired)
				{
				case KANJI_SJIS:
					switch(Pkt->KanjiCode)
					{
					case KANJI_SJIS:
						// カナ変換のため
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvJIStoSJIS(&cInfo2);
						break;
					case KANJI_JIS:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_EUC:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_UTF8N:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_UTF8BOM:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					}
					break;
				case KANJI_JIS:
					switch(Pkt->KanjiCode)
					{
					case KANJI_SJIS:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_JIS:
						// カナ変換のため
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoJIS(&cInfo2);
						break;
					case KANJI_EUC:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoEUC(&cInfo2);
						break;
					case KANJI_UTF8N:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoUTF8N(&cInfo2);
						break;
					case KANJI_UTF8BOM:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoUTF8N(&cInfo2);
						break;
					}
					break;
				case KANJI_EUC:
					switch(Pkt->KanjiCode)
					{
					case KANJI_SJIS:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_JIS:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoJIS(&cInfo2);
						break;
					case KANJI_EUC:
						// カナ変換のため
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoEUC(&cInfo2);
						break;
					case KANJI_UTF8N:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoUTF8N(&cInfo2);
						break;
					case KANJI_UTF8BOM:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoUTF8N(&cInfo2);
						break;
					}
					break;
				case KANJI_UTF8N:
					switch(Pkt->KanjiCode)
					{
					case KANJI_SJIS:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_JIS:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoJIS(&cInfo2);
						break;
					case KANJI_EUC:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoEUC(&cInfo2);
						break;
					case KANJI_UTF8N:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_UTF8BOM:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					}
					break;
				case KANJI_UTF8BOM:
					switch(Pkt->KanjiCode)
					{
					case KANJI_SJIS:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_JIS:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoJIS(&cInfo2);
						break;
					case KANJI_EUC:
						cInfo2.Str = cInfo.Buf;
						cInfo2.StrLen = cInfo.OutLen;
						cInfo2.Buf = Buf3;
						cInfo2.BufSize = (BUFSIZE + 3) * 4;
						ConvSJIStoEUC(&cInfo2);
						break;
					case KANJI_UTF8N:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					case KANJI_UTF8BOM:
						memcpy(Buf3, cInfo.Buf, cInfo.OutLen);
						cInfo2.OutLen = cInfo.OutLen;
						break;
					}
					break;
				}

//				if(TermCodeConvAndSend(&tInfo, dSkt, Buf2, cInfo.OutLen, Pkt->Type) == FFFTP_FAIL)
				if(TermCodeConvAndSend(&tInfo, dSkt, Buf3, cInfo2.OutLen, Pkt->Type, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
					Pkt->Abort = ABORT_ERROR;
				cInfo2.Buf = Buf3;
				cInfo2.BufSize = (BUFSIZE + 3) * 4;
				FlushRestData(&cInfo2);
				if(TermCodeConvAndSend(&tInfo, dSkt, Buf3, cInfo2.OutLen, Pkt->Type, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
					Pkt->Abort = ABORT_ERROR;
			}

			tInfo.Buf = Buf2;
			tInfo.BufSize = BUFSIZE+3;
			FlushRestTermCodeConvData(&tInfo);
			// 同時接続対応
//			if(SendData(dSkt, Buf2, tInfo.OutLen, 0, &Canceled) == FFFTP_FAIL)
			if(SendData(dSkt, Buf2, tInfo.OutLen, 0, &Canceled[Pkt->ThreadCount]) == FFFTP_FAIL)
				Pkt->Abort = ABORT_ERROR;
		}

		/* グラフ表示を更新 */
		if(Pkt->hWndTrans != NULL)
		{
			KillTimer(Pkt->hWndTrans, TIMER_DISPLAY);
			DispTransferStatus(Pkt->hWndTrans, YES, Pkt);
			// 同時接続対応
//			TimeStart = time(NULL) - TimeStart + 1;
			TimeStart[Pkt->ThreadCount] = time(NULL) - TimeStart[Pkt->ThreadCount] + 1;
		}
		CloseHandle(iFileHandle);
	}
	else
	{
		SetErrorMsg(MSGJPN112, Pkt->LocalFile);
		SetTaskMsg(MSGJPN112, Pkt->LocalFile);
		Pkt->Abort = ABORT_ERROR;
	}

	if(shutdown(dSkt, 1) != 0)
		ReportWSError("shutdown", WSAGetLastError());

#if 0
	/* clean up */
	while(do_recv(dSkt, Buf, BUFSIZE, 0, &TimeOutErr, &Canceled) > 0)
		;
#endif

	// 同時接続対応
//	iRetCode = ReadReplyMessage(Pkt->ctrl_skt, Buf, 1024, &Canceled, TmpBuf);
	iRetCode = ReadReplyMessage(Pkt->ctrl_skt, Buf, 1024, &Canceled[Pkt->ThreadCount], TmpBuf);

//#pragma aaa
//DoPrintf("##UP REPLY : %s", Buf);

	if(iRetCode >= FTP_RETRY)
		SetErrorMsg(Buf);

	if(Pkt->Abort != ABORT_NONE)
		iRetCode = 500;

	return(iRetCode);
}


/*----- バッファの内容を改行コード変換して送信 --------------------------------
*
*	Parameter
*		TERMCODECONVINFO *tInfo : 改行コード変換パケット
*		SOCKET Skt : ソケット
*		char *Data : データ
*		int Size : データのサイズ
*		int Ascii : モード　　(TYPE_xx)
*
*	Return Value
*		int 応答コード
*----------------------------------------------------------------------------*/

// 同時接続対応
//static int TermCodeConvAndSend(TERMCODECONVINFO *tInfo, SOCKET Skt, char *Data, int Size, int Ascii)
static int TermCodeConvAndSend(TERMCODECONVINFO *tInfo, SOCKET Skt, char *Data, int Size, int Ascii, int *CancelCheckWork)
{
	char Buf3[BUFSIZE*2];
	int Continue;
	int Ret;

	Ret = FFFTP_SUCCESS;

// CR-LF以外の改行コードを変換しないモードはここへ追加
	if(Ascii == TYPE_A)
	{
		tInfo->Str = Data;
		tInfo->StrLen = Size;
		tInfo->Buf = Buf3;
		tInfo->BufSize = BUFSIZE*2;
		do
		{
			Continue = ConvTermCodeToCRLF(tInfo);
			// 同時接続対応
//			if((Ret = SendData(Skt, Buf3, tInfo->OutLen, 0, &Canceled)) == FFFTP_FAIL)
			if((Ret = SendData(Skt, Buf3, tInfo->OutLen, 0, CancelCheckWork)) == FFFTP_FAIL)
				break;
		}
		while(Continue == YES);
	}
	else
		// 同時接続対応
//		Ret = SendData(Skt, Data, Size, 0, &Canceled);
		Ret = SendData(Skt, Data, Size, 0, CancelCheckWork);

	return(Ret);
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
					if(TransferErrorNotify == YES && DispUpDownErrDialog(uperr_dlg, Pkt->hWndTrans, Pkt) == NO)
						ClearAll = YES;
					else
					{
						Pkt->Mode = TransferErrorMode;
						AddTransFileList(Pkt);
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
					if(!(Pkt = (TRANSPACKET*)GetWindowLong(hDlg, GWL_USERDATA)))
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
				if(!(Pkt = (TRANSPACKET*)GetWindowLong(hDlg, GWL_USERDATA)))
					break;
				if(Canceled[Pkt->ThreadCount] == YES)
					Pkt->Abort = ABORT_USER;
				DispTransferStatus(hDlg, NO, Pkt);
				SetTimer(hDlg, TIMER_DISPLAY, DISPLAY_TIMING, NULL);
			}
			break;

		case WM_SET_PACKET :
//			Pkt = (TRANSPACKET *)lParam;
			SetWindowLong(hDlg, GWL_USERDATA, (LONG)lParam);
			break;
	}
	return(FALSE);
}


/*----- 転送ステータスを表示 --------------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		int End : 転送が完了したかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void DispTransferStatus(HWND hWnd, int End, TRANSPACKET *Pkt)
{
	time_t TotalLap;
	int Per;
	LONGLONG Bps;
	LONGLONG Transed;
	char Num1[40];
	char Num2[40];
	char Tmp[80];
	char Str[80];
	char *Pos;

	if(hWnd != NULL)
	{
		SendMessage(hWnd, WM_GETTEXT, 79, (LPARAM)Str);
		if((Pos = strchr(Str, ')')) != NULL)
			Pos ++;
		else
			Pos = Str;
		sprintf(Tmp, "(%d)%s", AskTransferFileNum(), Pos);
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)Tmp);

		if(Pkt->Abort == ABORT_NONE)
		{
			if(End == NO)
			{
				// 同時接続対応
//				TotalLap = time(NULL) - TimeStart + 1;
				TotalLap = time(NULL) - TimeStart[Pkt->ThreadCount] + 1;

				Bps = 0;
				if(TotalLap != 0)
					// 同時接続対応
//					Bps = AllTransSizeNow / TotalLap;
					Bps = AllTransSizeNow[Pkt->ThreadCount] / TotalLap;
				Transed = Pkt->Size - Pkt->ExistSize;

				if(Pkt->Size <= 0)
					sprintf(Tmp, "%d ", Pkt->ExistSize);
				else if(Pkt->Size < 1024)
					sprintf(Tmp, "%s / %s ", MakeNumString(Pkt->ExistSize, Num1, TRUE), MakeNumString(Pkt->Size, Num2, TRUE));
				else
					sprintf(Tmp, "%sk / %sk ", MakeNumString(Pkt->ExistSize/1024, Num1, TRUE), MakeNumString(Pkt->Size/1024, Num2, TRUE));
				strcpy(Str, Tmp);

				if(Bps == 0)
					sprintf(Tmp, "( 0 B/S )");
				else if(Bps < 1000)
					sprintf(Tmp, "( %s B/S )", MakeNumString(Bps, Num1, TRUE));
				else
					sprintf(Tmp, "( %s.%02d KB/S )", MakeNumString(Bps/1000, Num1, TRUE), (int)((Bps%1000)/10));
				strcat(Str, Tmp);

				if((Bps > 0) && (Pkt->Size > 0) && (Transed >= 0))
				{
					sprintf(Tmp, "  %d:%02d", (int)((Transed/Bps)/60), (int)((Transed/Bps)%60));
					strcat(Str, Tmp);
				}
				else
					strcat(Str, "  ??:??");
			}
			else
				strcpy(Str, MSGJPN117);
		}
		else
			strcpy(Str, MSGJPN118);

		SendDlgItemMessage(hWnd, TRANS_STATUS, WM_SETTEXT, 0, (LPARAM)Str);

		if(Pkt->Size <= 0)
			Per = 0;
		else if(Pkt->Size < 1024*1024)
			Per = (int)(Pkt->ExistSize * 100 / Pkt->Size);
		else
			Per = (int)((Pkt->ExistSize/1024) * 100 / (Pkt->Size/1024));
		SendDlgItemMessage(hWnd, TRANS_TIME_BAR, PBM_SETPOS, Per, 0);
	}
	return;
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

static void DispTransFileInfo(TRANSPACKET *Pkt, char *Title, int SkipButton, int Info)
{
	char Tmp[40];

	if(Pkt->hWndTrans != NULL)
	{
		EnableWindow(GetDlgItem(Pkt->hWndTrans, IDCANCEL), SkipButton);

		sprintf(Tmp, "(%d)%s", AskTransferFileNum(), Title);
		SendMessage(Pkt->hWndTrans, WM_SETTEXT, 0, (LPARAM)Tmp);
		SendDlgItemMessage(Pkt->hWndTrans, TRANS_STATUS, WM_SETTEXT, 0, (LPARAM)"");

		SendDlgItemMessage(Pkt->hWndTrans, TRANS_TIME_BAR, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		SendDlgItemMessage(Pkt->hWndTrans, TRANS_TIME_BAR, PBM_SETSTEP, 1, 0);
		SendDlgItemMessage(Pkt->hWndTrans, TRANS_TIME_BAR, PBM_SETPOS, 0, 0);

		if(Info == YES)
		{
			DispStaticText(GetDlgItem(Pkt->hWndTrans, TRANS_REMOTE), Pkt->RemoteFile);
			DispStaticText(GetDlgItem(Pkt->hWndTrans, TRANS_LOCAL), Pkt->LocalFile);

			if(Pkt->Type == TYPE_I)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_MODE, WM_SETTEXT, 0, (LPARAM)MSGJPN119);
			else if(Pkt->Type == TYPE_A)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_MODE, WM_SETTEXT, 0, (LPARAM)MSGJPN120);

			// UTF-8対応
			if(Pkt->KanjiCode == KANJI_NOCNV)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)MSGJPN121);
			else if(Pkt->KanjiCode == KANJI_SJIS)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)MSGJPN305);
			else if(Pkt->KanjiCode == KANJI_JIS)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)MSGJPN122);
			else if(Pkt->KanjiCode == KANJI_EUC)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)MSGJPN123);
			else if(Pkt->KanjiCode == KANJI_UTF8N)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)MSGJPN306);
			else if(Pkt->KanjiCode == KANJI_UTF8BOM)
				SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)MSGJPN329);
		}
		else
		{
			SendDlgItemMessage(Pkt->hWndTrans, TRANS_REMOTE, WM_SETTEXT, 0, (LPARAM)"");
			SendDlgItemMessage(Pkt->hWndTrans, TRANS_LOCAL, WM_SETTEXT, 0, (LPARAM)"");
			SendDlgItemMessage(Pkt->hWndTrans, TRANS_MODE, WM_SETTEXT, 0, (LPARAM)"");
			SendDlgItemMessage(Pkt->hWndTrans, TRANS_KANJI, WM_SETTEXT, 0, (LPARAM)"");
		}
	}
	return;
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

static int GetAdrsAndPort(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max)
{
	int Result;
	Result = FFFTP_FAIL;
	switch(AskCurNetType())
	{
	case NTYPE_IPV4:
		Result = GetAdrsAndPortIPv4(Skt, Str, Adrs, Port, Max);
		break;
	case NTYPE_IPV6:
		Result = GetAdrsAndPortIPv6(Skt, Str, Adrs, Port, Max);
		break;
	}
	return Result;
}


// IPv6対応
//static int GetAdrsAndPort(char *Str, char *Adrs, int *Port, int Max)
static int GetAdrsAndPortIPv4(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max)
{
	char *Pos;
	char *Btm;
	// コンマではなくドットを返すホストがあるため
	char *OldBtm;
	int Sts;

	Sts = FFFTP_FAIL;

	Pos = strchr(Str, '(');
	if(Pos != NULL)
	{
		Pos++;
		Btm = strchr(Pos, ',');
		// コンマではなくドットを返すホストがあるため
		if(Btm == NULL)
			Btm = strchr(Pos, '.');
		if(Btm != NULL)
		{
			Btm++;
			// コンマではなくドットを返すホストがあるため
//			Btm = strchr(Btm, ',');
			OldBtm = Btm;
			Btm = strchr(OldBtm, ',');
			if(Btm == NULL)
				Btm = strchr(OldBtm, '.');
			if(Btm != NULL)
			{
				Btm++;
				// コンマではなくドットを返すホストがあるため
//				Btm = strchr(Btm, ',');
				OldBtm = Btm;
				Btm = strchr(OldBtm, ',');
				if(Btm == NULL)
					Btm = strchr(OldBtm, '.');
				if(Btm != NULL)
				{
					Btm++;
					// コンマではなくドットを返すホストがあるため
//					Btm = strchr(Btm, ',');
					OldBtm = Btm;
					Btm = strchr(OldBtm, ',');
					if(Btm == NULL)
						Btm = strchr(OldBtm, '.');
					if(Btm != NULL)
					{
						if((Btm - Pos) <= Max)
						{
							strncpy(Adrs, Pos, Btm - Pos);
							*(Adrs + (Btm - Pos)) = NUL;
							ReplaceAll(Adrs, ',', '.');

							Pos = Btm + 1;
							Btm = strchr(Pos, ',');
							// コンマではなくドットを返すホストがあるため
							if(Btm == NULL)
								Btm = strchr(Pos, '.');
							if(Btm != NULL)
							{
								Btm++;
								*Port = (atoi(Pos) * 0x100) + atoi(Btm);
								Sts = FFFTP_SUCCESS;
							}
						}
					}
				}
			}
		}
	}
	return(Sts);
}


// IPv6対応
static int GetAdrsAndPortIPv6(SOCKET Skt, char *Str, char *Adrs, int *Port, int Max)
{
	char *Pos;
	char *Btm;
	int Sts;
	int i;
	struct sockaddr_in6 SockAddr;

	Sts = FFFTP_FAIL;

	Btm = strchr(Str, '(');
	if(Btm != NULL)
	{
		Btm++;
		Btm = strchr(Btm, '|');
		if(Btm != NULL)
		{
			Pos = Btm + 1;
			Btm = strchr(Pos, '|');
			if(Btm != NULL)
			{
				Pos = Btm + 1;
				Btm = strchr(Pos, '|');
				if(Btm != NULL)
				{
					if((Btm - Pos) <= Max)
					{
						if((Btm - Pos) > 0)
						{
							strncpy(Adrs, Pos, Btm - Pos);
							*(Adrs + (Btm - Pos)) = NUL;
						}
						else
						{
							i = sizeof(SockAddr);
							if(getpeername(Skt, (struct sockaddr*)&SockAddr, &i) != SOCKET_ERROR)
								AddressToStringIPv6(Adrs, &SockAddr.sin6_addr);
						}

						Pos = Btm + 1;
						Btm = strchr(Pos, '|');
						if(Btm != NULL)
						{
							Btm++;
							*Port = atoi(Pos);
							Btm = strchr(Btm, ')');
							if(Btm != NULL)
								Sts = FFFTP_SUCCESS;
						}
					}
				}
			}
		}
	}
	return(Sts);
}


/*----- Windowsのスペシャルデバイスかどうかを返す -----------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス (YES/NO)
*----------------------------------------------------------------------------*/

static int IsSpecialDevice(char *Fname)
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


/*----- ミラーリングでのファイル削除確認 --------------------------------------
*
*	Parameter
*		int Cur
*		int Notify
*		TRANSPACKET *Pkt
*
*	Return Value
*		BOOL TRUE/FALSE
*----------------------------------------------------------------------------*/

static int MirrorDelNotify(int Cur, int Notify, TRANSPACKET *Pkt)
{
	MIRRORDELETEINFO DelInfo;
	HWND hWnd;

	if(((Cur == WIN_LOCAL) && (MirDownDelNotify == NO)) ||
	   ((Cur == WIN_REMOTE) && (MirUpDelNotify == NO)))
	{
		Notify = YES_ALL;
	}

	if(Notify != YES_ALL)
	{
		DelInfo.Cur = Cur;
		DelInfo.Pkt = Pkt;
		hWnd = Pkt->hWndTrans;
		if(hWnd == NULL)
			hWnd = GetMainHwnd();
		Notify = DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(delete_dlg), hWnd, MirrorDeleteDialogCallBack, (LPARAM)&DelInfo);
	}
	return(Notify);
}


/*----- ミラーリングでのファイル削除ダイアログのコールバック ------------------
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

static BOOL CALLBACK MirrorDeleteDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static MIRRORDELETEINFO *DelInfo;
	switch (iMessage)
	{
		case WM_INITDIALOG :
			DelInfo = (MIRRORDELETEINFO *)lParam;

			if(DelInfo->Cur == WIN_LOCAL)
			{
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN124);
				SendDlgItemMessage(hDlg, DELETE_TEXT, WM_SETTEXT, 0, (LPARAM)DelInfo->Pkt->LocalFile);
			}
			else
			{
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)MSGJPN125);
				SendDlgItemMessage(hDlg, DELETE_TEXT, WM_SETTEXT, 0, (LPARAM)DelInfo->Pkt->RemoteFile);
			}
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					EndDialog(hDlg, YES);
					break;

				case DELETE_NO :
					EndDialog(hDlg, NO);
					break;

				case DELETE_ALL :
					EndDialog(hDlg, YES_ALL);
					break;

				case IDCANCEL :
					ClearAll = YES;
					EndDialog(hDlg, NO_ALL);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}





static void SetErrorMsg(char *fmt, ...)
{
	va_list Args;

	// 同時接続対応
//	if(strlen(ErrMsg) == 0)
	if(strlen(GetErrMsg()) == 0)
	{
		va_start(Args, fmt);
		// 同時接続対応
//		wvsprintf(ErrMsg, fmt, Args);
		wvsprintf(GetErrMsg(), fmt, Args);
		va_end(Args);
	}
	return;
}






/*----- ダウンロード時の不正なパスをチェック ----------------------------------
*
*	Parameter
*		TRANSPACKET *packet : ダウンロード情報
*
*	Return Value
*		int YES=不正なパス/NO=問題ないパス
*----------------------------------------------------------------------------*/
int CheckPathViolation(TRANSPACKET *packet)
{
	int result = NO;
	char *msg;

	if((strncmp(packet->RemoteFile, "..\\", 3) == 0) ||
	   (strncmp(packet->RemoteFile, "../", 3) == 0) ||
	   (strstr(packet->RemoteFile, "\\..\\") != NULL) ||
	   (strstr(packet->RemoteFile, "/../") != NULL))
	{
		msg = malloc(strlen(MSGJPN297) + strlen(packet->RemoteFile) + 1);
		if(msg)
		{
			sprintf(msg, MSGJPN297, packet->RemoteFile);
			MessageBox(GetMainHwnd(), msg, MSGJPN086, MB_OK);
			free(msg);
		}
		result = YES;
	}
	return(result);
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
