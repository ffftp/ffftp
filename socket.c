/*=============================================================================
*
*									ソケット
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

#define	STRICT
// IPv6対応
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windowsx.h>
#include <commctrl.h>

#include "common.h"
#include "resource.h"
// UTF-8対応
#include "punycode.h"

#define USE_THIS	1
#define DBG_MSG		0




// Winsock2で定義される定数と名前が重複し値が異なるため使用不可
//#define FD_CONNECT_BIT		0x0001
//#define FD_CLOSE_BIT		0x0002
//#define FD_ACCEPT_BIT		0x0004
//#define FD_READ_BIT			0x0008
//#define FD_WRITE_BIT		0x0010





typedef struct {
	SOCKET Socket;
	int FdConnect;
	int FdClose;
	int FdAccept;
	int FdRead;
	int FdWrite;
	int Error;
} ASYNCSIGNAL;


typedef struct {
	HANDLE Async;
	int Done;
	int ErrorDb;
} ASYNCSIGNALDATABASE;


// スレッド衝突のバグ修正
// 念のためテーブルを増量
//#define MAX_SIGNAL_ENTRY		10
//#define MAX_SIGNAL_ENTRY_DBASE	5
#define MAX_SIGNAL_ENTRY		16
#define MAX_SIGNAL_ENTRY_DBASE	16




/*===== プロトタイプ =====*/

static LRESULT CALLBACK SocketWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static int AskAsyncDone(SOCKET s, int *Error, int Mask);
static int AskAsyncDoneDbase(HANDLE Async, int *Error);
static int RegistAsyncTable(SOCKET s);
static int RegistAsyncTableDbase(HANDLE Async);
static int UnRegistAsyncTable(SOCKET s);
static int UnRegistAsyncTableDbase(HANDLE Async);
// UTF-8対応
static HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
// IPv6対応
static HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);


/*===== 外部参照 =====*/

extern int TimeOut;


/*===== ローカルなワーク =====*/

static const char SocketWndClass[] = "FFFTPSocketWnd";
static HWND hWndSocket;

static ASYNCSIGNAL Signal[MAX_SIGNAL_ENTRY];
static ASYNCSIGNALDATABASE SignalDbase[MAX_SIGNAL_ENTRY_DBASE];

//static HANDLE hAsyncTblAccMutex;
// スレッド衝突のバグ修正
static HANDLE hAsyncTblAccMutex;





/*----- 
*
*	Parameter
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int MakeSocketWin(HWND hWnd, HINSTANCE hInst)
{
	int i;
	int Sts;
	WNDCLASSEX wClass;

	wClass.cbSize        = sizeof(WNDCLASSEX);
	wClass.style         = 0;
	wClass.lpfnWndProc   = SocketWndProc;
	wClass.cbClsExtra    = 0;
	wClass.cbWndExtra    = 0;
	wClass.hInstance     = hInst;
	wClass.hIcon         = NULL;
	wClass.hCursor       = NULL;
	wClass.hbrBackground = (HBRUSH)CreateSolidBrush(GetSysColor(COLOR_INFOBK));
	wClass.lpszMenuName  = NULL;
	wClass.lpszClassName = SocketWndClass;
	wClass.hIconSm       = NULL;
	RegisterClassEx(&wClass);

	Sts = FFFTP_FAIL;
	hWndSocket = CreateWindowEx(0, SocketWndClass, NULL,
			WS_BORDER | WS_POPUP,
			0, 0, 0, 0,
			hWnd, NULL, hInst, NULL);

	if(hWndSocket != NULL)
	{
//		hAsyncTblAccMutex = CreateMutex(NULL, FALSE, NULL);

		// スレッド衝突のバグ修正
//		for(i = 0; i < MAX_SIGNAL_ENTRY; i++)
//			Signal[i].Socket = INVALID_SOCKET;
//		for(i = 0; i < MAX_SIGNAL_ENTRY_DBASE; i++)
//			SignalDbase[i].Async = 0;
		if(hAsyncTblAccMutex = CreateMutex(NULL, FALSE, NULL))
		{
			for(i = 0; i < MAX_SIGNAL_ENTRY; i++)
				Signal[i].Socket = INVALID_SOCKET;
			for(i = 0; i < MAX_SIGNAL_ENTRY_DBASE; i++)
				SignalDbase[i].Async = 0;
		}
		Sts = FFFTP_SUCCESS;
	}
	return(Sts);
}


/*----- 
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DeleteSocketWin(void)
{
//	CloseHandle(hAsyncTblAccMutex);
	// スレッド衝突のバグ修正
	CloseHandle(hAsyncTblAccMutex);
	hAsyncTblAccMutex = NULL;

	if(hWndSocket != NULL)
		DestroyWindow(hWndSocket);
	return;
}


/*----- 
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		UINT message : メッセージ番号
*		WPARAM wParam : メッセージの WPARAM 引数
*		LPARAM lParam : メッセージの LPARAM 引数
*
*	Return Value
*		BOOL TRUE/FALSE
*----------------------------------------------------------------------------*/

static LRESULT CALLBACK SocketWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Pos;

	switch(message)
	{
		case WM_ASYNC_SOCKET :
			// スレッド衝突のバグ修正
			WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
			for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
			{
				if(Signal[Pos].Socket == (SOCKET)wParam)
				{
					Signal[Pos].Error = WSAGETSELECTERROR(lParam);
#if DBG_MSG
					if(WSAGETSELECTERROR(lParam) != 0)
						DoPrintf("####### Signal: error (%d)", WSAGETSELECTERROR(lParam));
#endif

					switch(WSAGETSELECTEVENT(lParam))
					{
						case FD_CONNECT :
							Signal[Pos].FdConnect = 1;
#if DBG_MSG
							DoPrintf("####### Signal: connect (S=%x)", Signal[Pos].Socket);
#endif
							break;

						case FD_CLOSE :
							Signal[Pos].FdClose = 1;
#if DBG_MSG
							DoPrintf("####### Signal: close (S=%x)", Signal[Pos].Socket);
#endif
//SetTaskMsg("####### Signal: close (%d) (S=%x)", Pos, Signal[Pos].Socket);
							break;

						case FD_ACCEPT :
							Signal[Pos].FdAccept = 1;
#if DBG_MSG
							DoPrintf("####### Signal: accept (S=%x)", Signal[Pos].Socket);
#endif
							break;

						case FD_READ :
							Signal[Pos].FdRead = 1;
#if DBG_MSG
							DoPrintf("####### Signal: read (S=%x)", Signal[Pos].Socket);
#endif
							break;

						case FD_WRITE :
							Signal[Pos].FdWrite = 1;
#if DBG_MSG
							DoPrintf("####### Signal: write (S=%x)", Signal[Pos].Socket);
#endif
							break;
					}
					break;
				}
			}
			// スレッド衝突のバグ修正
			ReleaseMutex(hAsyncTblAccMutex);
			break;

		case WM_ASYNC_DBASE :
			// APIの仕様上ハンドルが登録される前にウィンドウメッセージが呼び出される可能性あり
			RegistAsyncTableDbase((HANDLE)wParam);
			// スレッド衝突のバグ修正
			WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
			for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
			{
				if(SignalDbase[Pos].Async == (HANDLE)wParam)
				{
					if(HIWORD(lParam) != 0)
					{
						SignalDbase[Pos].ErrorDb = 1;
#if DBG_MSG
						DoPrintf("##### SignalDatabase: error");
#endif
					}
					SignalDbase[Pos].Done = 1;
#if DBG_MSG
					DoPrintf("##### SignalDatabase: Done");
#endif
					break;
				}
			}
			// スレッド衝突のバグ修正
			ReleaseMutex(hAsyncTblAccMutex);
			break;

		default :
			return(DefWindowProc(hWnd, message, wParam, lParam));
	}
    return(0);
}




/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int AskAsyncDone(SOCKET s, int *Error, int Mask)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	*Error = 0;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			*Error = Signal[Pos].Error;
			if(Signal[Pos].Error != 0)
				Sts = YES;
			if((Mask & FD_CONNECT) && (Signal[Pos].FdConnect != 0))
			{
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: connect (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_CLOSE) && (Signal[Pos].FdClose != 0))
//			if(Mask & FD_CLOSE)
			{
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: close (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_ACCEPT) && (Signal[Pos].FdAccept != 0))
			{
				Signal[Pos].FdAccept = 0;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: accept (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_READ) && (Signal[Pos].FdRead != 0))
			{
				Signal[Pos].FdRead = 0;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: read (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			if((Mask & FD_WRITE) && (Signal[Pos].FdWrite != 0))
			{
				Signal[Pos].FdWrite = 0;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: write (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY)
	{
		if(Mask & FD_CLOSE)
		{
				Sts = YES;
		}
		else
		{
			MessageBox(GetMainHwnd(), "AskAsyncDone called with unregisterd socket.", "FFFTP inner error", MB_OK);
			exit(1);
		}
	}
	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int AskAsyncDoneDbase(HANDLE Async, int *Error)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	*Error = 0;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
	{
		if(SignalDbase[Pos].Async == Async)
		{
			if(SignalDbase[Pos].Done != 0)
			{
				*Error = SignalDbase[Pos].ErrorDb;
				Sts = YES;
#if DBG_MSG
				DoPrintf("### Ask: Dbase (Sts=%d, Error=%d)", Sts, *Error);
#endif
			}
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY_DBASE)
	{
		MessageBox(GetMainHwnd(), "AskAsyncDoneDbase called with unregisterd handle.", "FFFTP inner error", MB_OK);
		exit(1);
	}
	return(Sts);
}



/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int RegistAsyncTable(SOCKET s)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{
			// 強制的に閉じられたソケットがあると重複する可能性あり
//			MessageBox(GetMainHwnd(), "Async socket already registerd.", "FFFTP inner error", MB_OK);
//			break;
			Signal[Pos].Socket = INVALID_SOCKET;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY)
	{
		// スレッド衝突のバグ修正
		WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
		for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
		{
			if(Signal[Pos].Socket == INVALID_SOCKET)
			{

//SetTaskMsg("############### Regist socket (%d)", Pos);

				Signal[Pos].Socket = s;
				Signal[Pos].Error = 0;
				Signal[Pos].FdConnect = 0;
				Signal[Pos].FdClose = 0;
				Signal[Pos].FdAccept = 0;
				Signal[Pos].FdRead = 0;
				Signal[Pos].FdWrite = 0;
				Sts = YES;
				break;
			}
		}
		// スレッド衝突のバグ修正
		ReleaseMutex(hAsyncTblAccMutex);

		if(Pos == MAX_SIGNAL_ENTRY)
		{
			MessageBox(GetMainHwnd(), "No more async regist space.", "FFFTP inner error", MB_OK);
			exit(1);
		}
	}

	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int RegistAsyncTableDbase(HANDLE Async)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
	{
		if(SignalDbase[Pos].Async == Async)
		{
			// 強制的に閉じられたハンドルがあると重複する可能性あり
//			MessageBox(GetMainHwnd(), "Async handle already registerd.", "FFFTP inner error", MB_OK);
			// APIの仕様上ハンドルが登録される前にウィンドウメッセージが呼び出される可能性あり
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);

	if(Pos == MAX_SIGNAL_ENTRY_DBASE)
	{
		// スレッド衝突のバグ修正
		WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
		for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
		{
			if(SignalDbase[Pos].Async == 0)
			{

//SetTaskMsg("############### Regist dbase (%d)", Pos);

				SignalDbase[Pos].Async = Async;
				SignalDbase[Pos].Done = 0;
				SignalDbase[Pos].ErrorDb = 0;
				Sts = YES;
				break;
			}
		}
		// スレッド衝突のバグ修正
		ReleaseMutex(hAsyncTblAccMutex);

		if(Pos == MAX_SIGNAL_ENTRY_DBASE)
		{
			MessageBox(GetMainHwnd(), "No more async dbase regist space.", "FFFTP inner error", MB_OK);
			exit(1);
		}
	}

	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int UnRegistAsyncTable(SOCKET s)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY; Pos++)
	{
		if(Signal[Pos].Socket == s)
		{

//SetTaskMsg("############### UnRegist socket (%d)", Pos);

			Signal[Pos].Socket = INVALID_SOCKET;
			Sts = YES;
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);
	return(Sts);
}


/*----- 
*
*	Parameter
*		
*
*	Return Value
*		
*----------------------------------------------------------------------------*/

static int UnRegistAsyncTableDbase(HANDLE Async)
{
	int Sts;
	int Pos;

	// スレッド衝突のバグ修正
	WaitForSingleObject(hAsyncTblAccMutex, INFINITE);
	Sts = NO;
	for(Pos = 0; Pos < MAX_SIGNAL_ENTRY_DBASE; Pos++)
	{
		if(SignalDbase[Pos].Async == Async)
		{

//SetTaskMsg("############### UnRegist dbase (%d)", Pos);

			SignalDbase[Pos].Async = 0;
			Sts = YES;
			break;
		}
	}
	// スレッド衝突のバグ修正
	ReleaseMutex(hAsyncTblAccMutex);
	return(Sts);
}








// IPv6対応
//struct hostent *do_gethostbyname(const char *Name, char *Buf, int Len, int *CancelCheckWork)
struct hostent *do_gethostbynameIPv4(const char *Name, char *Buf, int Len, int *CancelCheckWork)
{
#if USE_THIS
	struct hostent *Ret;
	HANDLE hAsync;
	int Error;

#if DBG_MSG
	DoPrintf("# Start gethostbyname");
#endif
	Ret = NULL;
	// 同時接続対応
//	*CancelCheckWork = NO;

	// UTF-8対応
//	hAsync = WSAAsyncGetHostByName(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len);
	hAsync = WSAAsyncGetHostByNameM(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len);
	if(hAsync != NULL)
	{
		RegistAsyncTableDbase(hAsync);
		while((*CancelCheckWork == NO) && (AskAsyncDoneDbase(hAsync, &Error) != YES))
		{
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				*CancelCheckWork = YES;
		}

		if(*CancelCheckWork == YES)
		{
			WSACancelAsyncRequest(hAsync);
		}
		else if(Error == 0)
		{
			Ret = (struct hostent *)Buf;
		}
		UnRegistAsyncTableDbase(hAsync);
	}
	return(Ret);
#else
	return(gethostbyname(Name));
#endif
}


struct hostent *do_gethostbynameIPv6(const char *Name, char *Buf, int Len, int *CancelCheckWork)
{
#if USE_THIS
	struct hostent *Ret;
	HANDLE hAsync;
	int Error;

#if DBG_MSG
	DoPrintf("# Start gethostbyname");
#endif
	Ret = NULL;
	// 同時接続対応
//	*CancelCheckWork = NO;

	// UTF-8対応
//	hAsync = WSAAsyncGetHostByName(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len);
	hAsync = WSAAsyncGetHostByNameIPv6M(hWndSocket, WM_ASYNC_DBASE, Name, Buf, Len, AF_INET6);
	if(hAsync != NULL)
	{
		RegistAsyncTableDbase(hAsync);
		while((*CancelCheckWork == NO) && (AskAsyncDoneDbase(hAsync, &Error) != YES))
		{
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				*CancelCheckWork = YES;
		}

		if(*CancelCheckWork == YES)
		{
			WSACancelAsyncRequest(hAsync);
		}
		else if(Error == 0)
		{
			Ret = (struct hostent *)Buf;
		}
		UnRegistAsyncTableDbase(hAsync);
	}
	return(Ret);
#else
	return(gethostbyname(Name));
#endif
}





SOCKET do_socket(int af, int type, int protocol)
{
	SOCKET Ret;

	Ret = socket(af, type, protocol);
	if(Ret != INVALID_SOCKET)
	{
		RegistAsyncTable(Ret);
	}
#if DBG_MSG
	DoPrintf("# do_socket (S=%x)", Ret);
#endif
	return(Ret);
}



int do_closesocket(SOCKET s)
{
#if USE_THIS
	int Ret;
	int Error;
	int CancelCheckWork;

#if DBG_MSG
	DoPrintf("# Start close (S=%x)", s);
#endif
	CancelCheckWork = NO;

	// スレッド衝突のバグ修正
	WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, 0);
	UnRegistAsyncTable(s);
	// FTPS対応
//	Ret = closesocket(s);
	Ret = closesocketS(s);
	if(Ret == SOCKET_ERROR)
	{
		Error = 0;
		while((CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_CLOSE) != YES))
		{
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				CancelCheckWork = YES;
		}

		if((CancelCheckWork == NO) && (Error == 0))
			Ret = 0;
	}

	// スレッド衝突のバグ修正
//	WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, 0);
	if(BackgrndMessageProc() == YES)
		CancelCheckWork = YES;
	// スレッド衝突のバグ修正
//	UnRegistAsyncTable(s);

#if DBG_MSG
	DoPrintf("# Exit close");
#endif
	return(Ret);
#else
	return(closesocket(s));
#endif
}






int do_connect(SOCKET s, const struct sockaddr *name, int namelen, int *CancelCheckWork)
{
#if USE_THIS
	int Ret;
	int Error;

#if DBG_MSG
	DoPrintf("# Start connect (S=%x)", s);
#endif
	// 同時接続対応
//	*CancelCheckWork = NO;

#if DBG_MSG
	DoPrintf("## Async set: FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ|FD_WRITE");
#endif
	// 高速化のためFD_READとFD_WRITEを使用しない
//	Ret = WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE);
	Ret = WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT);
	if(Ret != SOCKET_ERROR)
	{
		Ret = connect(s, name, namelen);
		if(Ret == SOCKET_ERROR)
		{
			do
			{
				Error = 0;
				while((*CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_CONNECT) != YES))
				{
					Sleep(1);
					if(BackgrndMessageProc() == YES)
						*CancelCheckWork = YES;
				}

				if(*CancelCheckWork == YES)
					break;
				if(Error == 0)
					Ret = 0;
				else
				{
//					Error = WSAGetLastError();
					DoPrintf("#### Connect: Error=%d", Error);
				}
			}
			while((Ret != 0) && (Error == WSAEWOULDBLOCK));
		}
	}
	else
		DoPrintf("#### Connect: AsyncSelect error (%d)", WSAGetLastError());

#if DBG_MSG
	DoPrintf("# Exit connect (%d)", Ret);
#endif
	return(Ret);
#else
	return(connect(s, name, namelen));
#endif
}





int do_listen(SOCKET s,	int backlog)
{
	int Ret;

	Ret = 1;
#if DBG_MSG
	DoPrintf("# Start listen (S=%x)", s);
	DoPrintf("## Async set: FD_CLOSE|FD_ACCEPT");
#endif

	Ret = WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CLOSE | FD_ACCEPT);
	if(Ret != SOCKET_ERROR)
		Ret = listen(s, backlog);

#if DBG_MSG
	DoPrintf("# Exit listen (%d)", Ret);
#endif
	return(Ret);
}



SOCKET do_accept(SOCKET s, struct sockaddr *addr, int *addrlen)
{
#if USE_THIS
	SOCKET Ret2;
	int CancelCheckWork;
	int Error;

#if DBG_MSG
	DoPrintf("# Start accept (S=%x)", s);
#endif
	CancelCheckWork = NO;
	Ret2 = INVALID_SOCKET;
	Error = 0;

	while((CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_ACCEPT) != YES))
	{
		if(AskAsyncDone(s, &Error, FD_CLOSE) == YES)
		{
			Error = 1;
			break;
		}
		Sleep(1);
		if(BackgrndMessageProc() == YES)
			CancelCheckWork = YES;
	}

	if((CancelCheckWork == NO) && (Error == 0))
	{
		do
		{
			Ret2 = accept(s, addr, addrlen);
			if(Ret2 != INVALID_SOCKET)
			{
#if DBG_MSG
				DoPrintf("## do_sccept (S=%x)", Ret2);
				DoPrintf("## Async set: FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ|FD_WRITE");
#endif
				RegistAsyncTable(Ret2);
				if(WSAAsyncSelect(Ret2, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE) == SOCKET_ERROR)
				{
					do_closesocket(Ret2);
					Ret2 = INVALID_SOCKET;
				}
				break;
			}
			Error = WSAGetLastError();
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				break;
		}
		while(Error == WSAEWOULDBLOCK);
	}

#if DBG_MSG
	DoPrintf("# Exit accept");
#endif
	return(Ret2);
#else
	return(accept(s, addr, addrlen));
#endif
}




/*----- recv相当の関数 --------------------------------------------------------
*
*	Parameter
*		SOCKET s : ソケット
*		char *buf : データを読み込むバッファ
*		int len : 長さ
*		int flags : recvに与えるフラグ
*		int *TimeOutErr : タイムアウトしたかどうかを返すワーク
*
*	Return Value
*		int : recvの戻り値と同じ
*
*	Note
*		タイムアウトの時は TimeOut=YES、Ret=SOCKET_ERROR になる
*----------------------------------------------------------------------------*/
int do_recv(SOCKET s, char *buf, int len, int flags, int *TimeOutErr, int *CancelCheckWork)
{
#if USE_THIS
	int Ret;
	time_t StartTime;
	time_t ElapseTime;
	int Error;

#if DBG_MSG
	DoPrintf("# Start recv (S=%x)", s);
#endif
	*TimeOutErr = NO;
	// 同時接続対応
//	*CancelCheckWork = NO;
	Ret = SOCKET_ERROR;
	Error = 0;

	if(TimeOut != 0)
		time(&StartTime);

	// FTPS対応
	// OpenSSLでは受信確認はFD_READが複数回受信される可能性がある
//	while((*CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_READ) != YES))
	// 短時間にFD_READが2回以上通知される対策
//	while(!IsSSLAttached(s) && (*CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_READ) != YES))
//	{
//		if(AskAsyncDone(s, &Error, FD_CLOSE) == YES)
//		{
//			Ret = 0;
//			break;
//		}
//		Sleep(1);
//		if(BackgrndMessageProc() == YES)
//			*CancelCheckWork = YES;
//		else if(TimeOut != 0)
//		{
//			time(&ElapseTime);
//			ElapseTime -= StartTime;
//			if(ElapseTime >= TimeOut)
//			{
//				DoPrintf("do_recv timed out");
//				*TimeOutErr = YES;
//				*CancelCheckWork = YES;
//			}
//		}
//	}

	if(/*(Ret != 0) && */(Error == 0) && (*CancelCheckWork == NO) && (*TimeOutErr == NO))
	{
		do
		{
#if DBG_MSG
			DoPrintf("## recv()");
#endif

			// FTPS対応
//			Ret = recv(s, buf, len, flags);
			Ret = recvS(s, buf, len, flags);
			if(Ret != SOCKET_ERROR)
				break;
			Error = WSAGetLastError();
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				break;
			// FTPS対応
			// 受信確認をバイパスしたためここでタイムアウトの確認
			if(BackgrndMessageProc() == YES)
				*CancelCheckWork = YES;
			else if(TimeOut != 0)
			{
				time(&ElapseTime);
				ElapseTime -= StartTime;
				if(ElapseTime >= TimeOut)
				{
					DoPrintf("do_recv timed out");
					*TimeOutErr = YES;
					*CancelCheckWork = YES;
				}
			}
			if(*CancelCheckWork == YES)
				break;
		}
		while(Error == WSAEWOULDBLOCK);
	}

	if(BackgrndMessageProc() == YES)
		Ret = SOCKET_ERROR;

#if DBG_MSG
	DoPrintf("# Exit recv (%d)", Ret);
#endif
	return(Ret);
#else
	return(recv(s, buf, len, flags));
#endif
}



int do_send(SOCKET s, const char *buf, int len, int flags, int *TimeOutErr, int *CancelCheckWork)
{
#if USE_THIS
	int Ret;
	time_t StartTime;
	time_t ElapseTime;
	int Error;

#if DBG_MSG
	DoPrintf("# Start send (S=%x)", s);
#endif
	*TimeOutErr = NO;
	// 同時接続対応
//	*CancelCheckWork = NO;
	Ret = SOCKET_ERROR;
	Error = 0;

	if(TimeOut != 0)
		time(&StartTime);

#if DBG_MSG
	DoPrintf("## Async set: FD_CONNECT|FD_CLOSE|FD_ACCEPT|FD_READ|FD_WRITE");
#endif
	// Windows 2000でFD_WRITEが通知されないことがあるバグ修正
	// 毎回通知されたのはNT 4.0までのバグであり仕様ではない
	// XP以降は互換性のためか毎回通知される
//	WSAAsyncSelect(s, hWndSocket, WM_ASYNC_SOCKET, FD_CONNECT | FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE);
	if(BackgrndMessageProc() == YES)
		*CancelCheckWork = YES;

	// FTPS対応
	// 送信バッファの空き確認には影響しないが念のため
//	while((*CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_WRITE) != YES))
	// Windows 2000でFD_WRITEが通知されないことがあるバグ修正
//	while(!IsSSLAttached(s) && (*CancelCheckWork == NO) && (AskAsyncDone(s, &Error, FD_WRITE) != YES))
//	{
//		if(AskAsyncDone(s, &Error, FD_CLOSE) == YES)
//		{
//			Error = 1;
//			break;
//		}
//
//		Sleep(1);
//		if(BackgrndMessageProc() == YES)
//			*CancelCheckWork = YES;
//		else if(TimeOut != 0)
//		{
//			time(&ElapseTime);
//			ElapseTime -= StartTime;
//			if(ElapseTime >= TimeOut)
//			{
//				DoPrintf("do_write timed out");
//				*TimeOutErr = YES;
//				*CancelCheckWork = YES;
//			}
//		}
//	}

	if((Error == 0) && (*CancelCheckWork == NO) && (*TimeOutErr == NO))
	{
		do
		{
#if DBG_MSG
			DoPrintf("## send()");
#endif

			// FTPS対応
//			Ret = send(s, buf, len, flags);
			Ret = sendS(s, buf, len, flags);
			if(Ret != SOCKET_ERROR)
			{
#if DBG_MSG
				DoPrintf("## send() OK");
#endif
				break;
			}
			Error = WSAGetLastError();
			Sleep(1);
			if(BackgrndMessageProc() == YES)
				break;
			// FTPS対応
			// 送信バッファ確認をバイパスしたためここでタイムアウトの確認
			if(BackgrndMessageProc() == YES)
				*CancelCheckWork = YES;
			else if(TimeOut != 0)
			{
				time(&ElapseTime);
				ElapseTime -= StartTime;
				if(ElapseTime >= TimeOut)
				{
					DoPrintf("do_recv timed out");
					*TimeOutErr = YES;
					*CancelCheckWork = YES;
				}
			}
			if(*CancelCheckWork == YES)
				break;
		}
		while(Error == WSAEWOULDBLOCK);
	}

	if(BackgrndMessageProc() == YES)
		Ret = SOCKET_ERROR;

#if DBG_MSG
	DoPrintf("# Exit send (%d)", Ret);
#endif
	return(Ret);
#else
	return(send(s, buf, len, flags));
#endif
}


// 同時接続対応
void RemoveReceivedData(SOCKET s)
{
	char buf[1024];
	int len;
	int Error;
	while((len = recvS(s, buf, sizeof(buf), MSG_PEEK)) >= 0)
	{
		AskAsyncDone(s, &Error, FD_READ);
		recvS(s, buf, len, 0);
	}
}


/*----- 
*
*	Parameter
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int CheckClosedAndReconnect(void)
{
	int Error;
	int Sts;

//SetTaskMsg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	Sts = FFFTP_SUCCESS;
	if(AskAsyncDone(AskCmdCtrlSkt(), &Error, FD_CLOSE) == YES)
	{
		Sts = ReConnectCmdSkt();
	}
	return(Sts);
}



// IPv6対応

typedef struct
{
	HANDLE h;
	HWND hWnd;
	u_int wMsg;
	char * name;
	char * buf;
	int buflen;
	short Family;
} GETHOSTBYNAMEDATA;

static DWORD WINAPI WSAAsyncGetHostByNameIPv6ThreadProc(LPVOID lpParameter)
{
	GETHOSTBYNAMEDATA* pData;
	struct hostent* pHost;
	struct addrinfo* pAddr;
	struct addrinfo* p;
	pHost = NULL;
	pData = (GETHOSTBYNAMEDATA*)lpParameter;
	if(getaddrinfo(pData->name, NULL, NULL, &pAddr) == 0)
	{
		p = pAddr;
		while(p)
		{
			if(p->ai_family == pData->Family)
			{
				switch(p->ai_family)
				{
				case AF_INET:
					pHost = (struct hostent*)pData->buf;
					if((size_t)pData->buflen >= sizeof(struct hostent) + sizeof(char*) * 2 + sizeof(struct in_addr)
						&& p->ai_addrlen >= sizeof(struct sockaddr_in))
					{
						pHost->h_name = NULL;
						pHost->h_aliases = NULL;
						pHost->h_addrtype = p->ai_family;
						pHost->h_length = sizeof(struct in_addr);
						pHost->h_addr_list = (char**)(&pHost[1]);
						pHost->h_addr_list[0] = (char*)(&pHost->h_addr_list[2]);
						pHost->h_addr_list[1] = NULL;
						memcpy(pHost->h_addr_list[0], &((struct sockaddr_in*)p->ai_addr)->sin_addr, sizeof(struct in_addr));
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(sizeof(struct hostent) + sizeof(char*) * 2 + p->ai_addrlen));
					}
					else
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(WSAENOBUFS << 16));
					break;
				case AF_INET6:
					pHost = (struct hostent*)pData->buf;
					if((size_t)pData->buflen >= sizeof(struct hostent) + sizeof(char*) * 2 + sizeof(struct in6_addr)
						&& p->ai_addrlen >= sizeof(struct sockaddr_in6))
					{
						pHost->h_name = NULL;
						pHost->h_aliases = NULL;
						pHost->h_addrtype = p->ai_family;
						pHost->h_length = sizeof(struct in6_addr);
						pHost->h_addr_list = (char**)(&pHost[1]);
						pHost->h_addr_list[0] = (char*)(&pHost->h_addr_list[2]);
						pHost->h_addr_list[1] = NULL;
						memcpy(pHost->h_addr_list[0], &((struct sockaddr_in6*)p->ai_addr)->sin6_addr, sizeof(struct in6_addr));
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(sizeof(struct hostent) + sizeof(char*) * 2 + p->ai_addrlen));
					}
					else
						PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(WSAENOBUFS << 16));
					break;
				}
			}
			if(pHost)
				break;
			p = p->ai_next;
		}
		if(!p)
			PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(ERROR_INVALID_FUNCTION << 16));
		freeaddrinfo(pAddr);
	}
	else
		PostMessage(pData->hWnd, pData->wMsg, (WPARAM)pData->h, (LPARAM)(ERROR_INVALID_FUNCTION << 16));
	free(pData->name);
	free(pData);
	// CreateThreadが返すハンドルが重複するのを回避
	Sleep(10000);
	return 0;
}

// IPv6対応のWSAAsyncGetHostByName相当の関数
// FamilyにはAF_INETまたはAF_INET6を指定可能
// ただしANSI用
static HANDLE WSAAsyncGetHostByNameIPv6(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family)
{
	HANDLE hResult;
	GETHOSTBYNAMEDATA* pData;
	hResult = NULL;
	if(pData = malloc(sizeof(GETHOSTBYNAMEDATA)))
	{
		pData->hWnd = hWnd;
		pData->wMsg = wMsg;
		if(pData->name = malloc(sizeof(char) * (strlen(name) + 1)))
		{
			strcpy(pData->name, name);
			pData->buf = buf;
			pData->buflen = buflen;
			pData->Family = Family;
			if(pData->h = CreateThread(NULL, 0, WSAAsyncGetHostByNameIPv6ThreadProc, pData, CREATE_SUSPENDED, NULL))
			{
				ResumeThread(pData->h);
				hResult = pData->h;
			}
		}
	}
	if(!hResult)
	{
		if(pData)
		{
			if(pData->name)
				free(pData->name);
			free(pData);
		}
	}
	return hResult;
}

// WSAAsyncGetHostByNameIPv6用のWSACancelAsyncRequest相当の関数
int WSACancelAsyncRequestIPv6(HANDLE hAsyncTaskHandle)
{
	int Result;
	Result = SOCKET_ERROR;
	if(TerminateThread(hAsyncTaskHandle, 0))
		Result = 0;
	return Result;
}

char* AddressToStringIPv6(char* str, void* in6)
{
	char* pResult;
	unsigned char* p;
	int MaxZero;
	int MaxZeroLen;
	int i;
	int j;
	char Tmp[5];
	pResult = str;
	p = (unsigned char*)in6;
	MaxZero = 8;
	MaxZeroLen = 1;
	for(i = 0; i < 8; i++)
	{
		for(j = i; j < 8; j++)
		{
			if(p[j * 2] != 0 || p[j * 2 + 1] != 0)
				break;
		}
		if(j - i > MaxZeroLen)
		{
			MaxZero = i;
			MaxZeroLen = j - i;
		}
	}
	strcpy(str, "");
	for(i = 0; i < 8; i++)
	{
		if(i == MaxZero)
		{
			if(i == 0)
				strcat(str, ":");
			strcat(str, ":");
		}
		else if(i < MaxZero || i >= MaxZero + MaxZeroLen)
		{
			sprintf(Tmp, "%x", (((int)p[i * 2] & 0xff) << 8) | ((int)p[i * 2 + 1] & 0xff));
			strcat(str, Tmp);
			if(i < 7)
				strcat(str, ":");
		}
	}
	return pResult;
}

// IPv6対応のinet_ntoa相当の関数
// ただしANSI用
char* inet6_ntoa(struct in6_addr in6)
{
	char* pResult;
	static char Adrs[40];
	pResult = NULL;
	memset(Adrs, 0, sizeof(Adrs));
	pResult = AddressToStringIPv6(Adrs, &in6);
	return pResult;
}

// IPv6対応のinet_addr相当の関数
// ただしANSI用
struct in6_addr inet6_addr(const char* cp)
{
	struct in6_addr Result;
	int AfterZero;
	int i;
	char* p;
	memset(&Result, 0, sizeof(Result));
	AfterZero = 0;
	for(i = 0; i < 8; i++)
	{
		if(!cp)
		{
			memset(&Result, 0xff, sizeof(Result));
			break;
		}
		if(i >= AfterZero)
		{
			if(strncmp(cp, ":", 1) == 0)
			{
				cp = cp + 1;
				if(i == 0 && strncmp(cp, ":", 1) == 0)
					cp = cp + 1;
				p = (char*)cp;
				AfterZero = 7;
				while(p = strstr(p, ":"))
				{
					p = p + 1;
					AfterZero--;
				}
			}
			else
			{
				Result.u.Word[i] = (USHORT)strtol(cp, &p, 16);
				Result.u.Word[i] = ((Result.u.Word[i] & 0xff00) >> 8) | ((Result.u.Word[i] & 0x00ff) << 8);
				if(strncmp(p, ":", 1) != 0 && strlen(p) > 0)
				{
					memset(&Result, 0xff, sizeof(Result));
					break;
				}
				if(cp = strstr(cp, ":"))
					cp = cp + 1;
			}
		}
	}
	return Result;
}

// UTF-8対応

static BOOL ConvertStringToPunycode(LPSTR Output, DWORD Count, LPCSTR Input)
{
	BOOL bResult;
	punycode_uint* pUnicode;
	punycode_uint* p;
	BOOL bNeeded;
	LPCSTR InputString;
	punycode_uint Length;
	punycode_uint OutputLength;
	bResult = FALSE;
	if(pUnicode = malloc(sizeof(punycode_uint) * strlen(Input)))
	{
		p = pUnicode;
		bNeeded = FALSE;
		InputString = Input;
		Length = 0;
		while(*InputString != '\0')
		{
			*p = (punycode_uint)GetNextCharM(InputString, &InputString);
			if(*p >= 0x80)
				bNeeded = TRUE;
			p++;
			Length++;
		}
		if(bNeeded)
		{
			if(Count >= strlen("xn--") + 1)
			{
				strcpy(Output, "xn--");
				OutputLength = Count - strlen("xn--");
				if(punycode_encode(Length, pUnicode, NULL, (punycode_uint*)&OutputLength, Output + strlen("xn--")) == punycode_success)
				{
					Output[strlen("xn--") + OutputLength] = '\0';
					bResult = TRUE;
				}
			}
		}
		free(pUnicode);
	}
	if(!bResult)
	{
		if(Count >= strlen(Input) + 1)
		{
			strcpy(Output, Input);
			bResult = TRUE;
		}
	}
	return bResult;
}

static BOOL ConvertNameToPunycode(LPSTR Output, LPCSTR Input)
{
	BOOL bResult;
	DWORD Length;
	char* pm0;
	char* pm1;
	char* p;
	char* pNext;
	bResult = FALSE;
	Length = strlen(Input);
	if(pm0 = AllocateStringM(Length + 1))
	{
		if(pm1 = AllocateStringM(Length * 4 + 1))
		{
			strcpy(pm0, Input);
			p = pm0;
			while(p)
			{
				if(pNext = strchr(p, '.'))
				{
					*pNext = '\0';
					pNext++;
				}
				if(ConvertStringToPunycode(pm1, Length * 4, p))
					strcat(Output, pm1);
				if(pNext)
					strcat(Output, ".");
				p = pNext;
			}
			bResult = TRUE;
			FreeDuplicatedString(pm1);
		}
		FreeDuplicatedString(pm0);
	}
	return bResult;
}

static HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen)
{
	HANDLE r = NULL;
	char* pa0 = NULL;
	if(pa0 = AllocateStringA(strlen(name) * 4))
	{
		if(ConvertNameToPunycode(pa0, name))
			r = WSAAsyncGetHostByName(hWnd, wMsg, pa0, buf, buflen);
	}
	FreeDuplicatedString(pa0);
	return r;
}

static HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family)
{
	HANDLE r = NULL;
	char* pa0 = NULL;
	if(pa0 = AllocateStringA(strlen(name) * 4))
	{
		if(ConvertNameToPunycode(pa0, name))
			r = WSAAsyncGetHostByNameIPv6(hWnd, wMsg, pa0, buf, buflen, Family);
	}
	FreeDuplicatedString(pa0);
	return r;
}

