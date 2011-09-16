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

#define  STRICT
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <malloc.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdarg.h>
#include <winsock.h>

#include "common.h"
#include "resource.h"

#define TASK_BUFSIZE	(16*1024)




/*===== 外部参照 =====*/

extern int ClientWidth;
extern int SepaWidth;
extern int ListHeight;

/* 設定値 */
extern int TaskHeight;
extern HFONT ListFont;
extern int DebugConsole;

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
	hWndTask = CreateWindowEx(/*WS_EX_STATICEDGE*/WS_EX_CLIENTEDGE,
			"EDIT", NULL,
			WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_CLIPSIBLINGS,
			0, TOOLWIN_HEIGHT*2+ListHeight+SepaWidth, ClientWidth, TaskHeight,
			hWnd, (HMENU)1500, hInst, NULL);

	if(hWndTask != NULL)
	{
		SendMessage(hWndTask, EM_LIMITTEXT, TASK_BUFSIZE, 0);

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


/*----- タスクメッセージを表示する --------------------------------------------
*
*	Parameter
*		char *szFormat : フォーマット文字列
*		... : パラメータ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetTaskMsg(char *szFormat, ...)
{
	int Pos;
	va_list vaArgs;
	char *szBuf;
//	DWORD Tmp;

//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "111\n", 4, &Tmp, NULL);
	while(WaitForSingleObject(DispLogSemaphore, 1) == WAIT_TIMEOUT)
		BackgrndMessageProc();
//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "222\n", 4, &Tmp, NULL);

	if(hWndTask != NULL)
	{
		if((szBuf = malloc(10*1024+3)) != NULL)
		{
			va_start(vaArgs, szFormat);
			if(wvsprintf(szBuf, szFormat, vaArgs) != EOF)
			{
//#pragma aaa
//				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szBuf, strlen(szBuf), &Tmp, NULL);
//				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "\n", strlen("\n"), &Tmp, NULL);

				strcat(szBuf, "\r\n");
				Pos = SendMessage(GetTaskWnd(), WM_GETTEXTLENGTH, 0, 0);

				/* テキストサイズのリミット値をチェック */
				if((Pos + strlen(szBuf)) >= TASK_BUFSIZE)
				{
					/* リミットを越えそうなら、先頭部分を切り捨てる */
					Pos = SendMessage(GetTaskWnd(), EM_LINEFROMCHAR, TASK_BUFSIZE/10, 0) + 1;
					Pos = SendMessage(GetTaskWnd(), EM_LINEINDEX, Pos, 0);
					SendMessage(GetTaskWnd(), EM_SETSEL, 0, Pos);
					SendMessage(GetTaskWnd(), EM_REPLACESEL, FALSE, (LPARAM)"");

					Pos = SendMessage(GetTaskWnd(), WM_GETTEXTLENGTH, 0, 0);
				}

				SendMessage(GetTaskWnd(), EM_SETSEL, Pos, Pos);
				SendMessage(GetTaskWnd(), EM_REPLACESEL, FALSE, (LPARAM)szBuf);
			}
			va_end(vaArgs);
			free(szBuf);
		}
	}

//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "333\n", 4, &Tmp, NULL);
	ReleaseSemaphore(DispLogSemaphore, 1, NULL);
//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "444\n", 4, &Tmp, NULL);

	return;
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
	Size = SendMessage(GetTaskWnd(), WM_GETTEXTLENGTH, 0, 0);
	if((Buf = malloc(Size)) != NULL)
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


/*----- デバッグコンソールにメッセージを表示する ------------------------------
*
*	Parameter
*		char *szFormat : フォーマット文字列
*		... : パラメータ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoPrintf(char *szFormat, ...)
{
	va_list vaArgs;
	char *szBuf;
//	DWORD Tmp;

//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "11111\n", 6, &Tmp, NULL);
	while(WaitForSingleObject(DispLogSemaphore2, 1) == WAIT_TIMEOUT)
		BackgrndMessageProc();
//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "22222\n", 6, &Tmp, NULL);

	if(DebugConsole == YES)
	{
		if((szBuf = malloc(10*1024)) != NULL)
		{
			va_start(vaArgs, szFormat);
			if(wvsprintf(szBuf, szFormat, vaArgs) != EOF)
			{
				SetTaskMsg("## %s", szBuf);

//#pragma aaa
//				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szBuf, strlen(szBuf), &Tmp, NULL);
//				WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "\n", strlen("\n"), &Tmp, NULL);
			}
			va_end(vaArgs);
			free(szBuf);
		}
	}

//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "33333\n", 6, &Tmp, NULL);
	ReleaseSemaphore(DispLogSemaphore2, 1, NULL);
//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "44444\n", 6, &Tmp, NULL);

	return;
}


/*----- デバッグコンソールにメッセージを表示する ------------------------------
*
*	Parameter
*		char *szFormat : フォーマット文字列
*		... : パラメータ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoPrintf2(char *szFormat, ...)
{
	va_list vaArgs;
	char *szBuf;
	DWORD Tmp;

//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "11111111\n", 9, &Tmp, NULL);
	while(WaitForSingleObject(DispLogSemaphore2, 1) == WAIT_TIMEOUT)
		BackgrndMessageProc();
//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "22222222\n", 9, &Tmp, NULL);

	if((szBuf = malloc(10*1024)) != NULL)
	{
		va_start(vaArgs, szFormat);
		if(wvsprintf(szBuf, szFormat, vaArgs) != EOF)
		{
			WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szBuf, strlen(szBuf), &Tmp, NULL);
			WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "\n", strlen("\n"), &Tmp, NULL);
		}
		va_end(vaArgs);
		free(szBuf);
	}

//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "33333333\n", 9, &Tmp, NULL);
	ReleaseSemaphore(DispLogSemaphore2, 1, NULL);
//WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), "44444444\n", 9, &Tmp, NULL);

	return;
}


