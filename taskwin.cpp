/*=============================================================================
*
*								タスクウインドウ
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


#define TASK_BUFSIZE	(16*1024)




/*===== 外部参照 =====*/

extern int ClientWidth;
extern int SepaWidth;
extern int ListHeight;

/* 設定値 */
extern int TaskHeight;
extern HFONT ListFont;
extern int DebugConsole;
// 古い処理内容を消去
extern int RemoveOldLog;

/*===== ローカルなワーク =====*/

static HWND hWndTask = NULL;
static HANDLE DispLogSemaphore;
static HANDLE DispLogSemaphore2;



/*----- タスクウインドウを作成する --------------------------------------------
*
*	Parameter
*		HWND hWnd : 親ウインドウのウインドウハンドル
*		HINSTANCE hInst : インスタンスハンドル
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeTaskWindow(HWND hWnd, HINSTANCE hInst)
{
	int Sts;

	Sts = FFFTP_FAIL;
	// 高DPI対応
//	hWndTask = CreateWindowEx(/*WS_EX_STATICEDGE*/WS_EX_CLIENTEDGE,
//			"EDIT", NULL,
//			WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_CLIPSIBLINGS,
//			0, TOOLWIN_HEIGHT*2+ListHeight+SepaWidth, ClientWidth, TaskHeight,
//			hWnd, (HMENU)1500, hInst, NULL);
	hWndTask = CreateWindowEx(/*WS_EX_STATICEDGE*/WS_EX_CLIENTEDGE,
			"EDIT", NULL,
			WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_CLIPSIBLINGS,
			0, AskToolWinHeight()*2+ListHeight+SepaWidth, ClientWidth, TaskHeight,
			hWnd, (HMENU)1500, hInst, NULL);

	if(hWndTask != NULL)
	{
		// Windows 9x系をサポートしないため不要
//		SendMessage(hWndTask, EM_LIMITTEXT, TASK_BUFSIZE, 0);
		SendMessage(hWndTask, EM_LIMITTEXT, 0x7fffffff, 0);

		if(ListFont != NULL)
			SendMessage(hWndTask, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));

		ShowWindow(hWndTask, SW_SHOW);
		Sts = FFFTP_SUCCESS;

		DispLogSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
		DispLogSemaphore2 = CreateSemaphore(NULL, 1, 1, NULL);

	}
	return(Sts);
}


/*----- タスクウインドウを削除 ------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteTaskWindow(void)
{
	CloseHandle(DispLogSemaphore);
	CloseHandle(DispLogSemaphore2);
	if(hWndTask != NULL)
		DestroyWindow(hWndTask);
	return;
}


/*----- タスクウインドウのウインドウハンドルを返す ----------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		HWND ウインドウハンドル
*----------------------------------------------------------------------------*/

HWND GetTaskWnd(void)
{
	return(hWndTask);
}


// タスクメッセージを表示する
// デバッグビルドではフォーマット済み文字列が渡される
void _SetTaskMsg(const char* format, ...) {
	while (WaitForSingleObject(DispLogSemaphore, 1) == WAIT_TIMEOUT)
		BackgrndMessageProc();
	if (hWndTask != NULL) {
		char buffer[10240 + 3];
#ifdef _DEBUG
		strcpy(buffer, format);
		size_t result = strlen(buffer);
#else
		va_list args;
		va_start(args, format);
		int result = vsprintf(buffer, format, args);
		va_end(args);
#endif
		if (0 < result) {
			strcat(buffer, "\r\n");
			int Pos = (int)SendMessage(GetTaskWnd(), WM_GETTEXTLENGTH, 0, 0);
			if (RemoveOldLog == YES) {
				if ((Pos + strlen(buffer)) >= TASK_BUFSIZE) {
					Pos = (int)SendMessage(GetTaskWnd(), EM_LINEINDEX, 1, 0);
					SendMessage(GetTaskWnd(), EM_SETSEL, 0, Pos);
					SendMessage(GetTaskWnd(), EM_REPLACESEL, FALSE, (LPARAM)"");
					Pos = (int)SendMessage(GetTaskWnd(), WM_GETTEXTLENGTH, 0, 0);
				}
			}
			SendMessage(GetTaskWnd(), EM_SETSEL, Pos, Pos);
			SendMessage(GetTaskWnd(), EM_REPLACESEL, FALSE, (LPARAM)buffer);
		}
	}
	ReleaseSemaphore(DispLogSemaphore, 1, NULL);
}


/*----- タスクメッセージをファイルに保存する ----------------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int SaveTaskMsg(char *Fname)
{
	FILE *Strm;
	int Size;
	char *Buf;
	int Sts;


	Sts = FFFTP_FAIL;
	Size = (int)SendMessage(GetTaskWnd(), WM_GETTEXTLENGTH, 0, 0);
	if((Buf = (char*)malloc(Size)) != NULL)
	{
		if((Strm = fopen(Fname, "wb")) != NULL)
		{
			SendMessage(GetTaskWnd(), WM_GETTEXT, Size, (LPARAM)Buf);
			if(fwrite(Buf, strlen(Buf), 1, Strm) == 1)
				Sts = FFFTP_SUCCESS;
			fclose(Strm);

			if(Sts == FFFTP_FAIL)
				_unlink(Fname);
		}
		free(Buf);
	}
	return(Sts);
}


/*----- タスク内容をビューワで表示 --------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispTaskMsg(void)
{
	char Buf[FMAX_PATH+1];

	strcpy(Buf, AskTmpFilePath());
	SetYenTail(Buf);
	strcat(Buf, "_ffftp.tsk");

	if(SaveTaskMsg(Buf) == FFFTP_SUCCESS)
	{
		AddTempFileList(Buf);
		ExecViewer(Buf, 0);
	}
	return;
}


// デバッグコンソールにメッセージを表示する
// デバッグビルドではフォーマット済み文字列が渡される
void _DoPrintf(const char* format, ...) {
	while (WaitForSingleObject(DispLogSemaphore2, 1) == WAIT_TIMEOUT)
		BackgrndMessageProc();
	if (DebugConsole == YES) {
#ifdef _DEBUG
		const char* buffer = format;
		size_t result = strlen(buffer);
#else
		char buffer[10240];
		va_list args;
		va_start(args, format);
		int result = vsprintf(buffer, format, args);
		va_end(args);
#endif
		if (0 < result)
			SetTaskMsg("## %s", buffer);
	}
	ReleaseSemaphore(DispLogSemaphore2, 1, NULL);
}
