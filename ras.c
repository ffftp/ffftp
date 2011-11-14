/*=============================================================================
*
*									ＲＡＳ関係
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

// UTF-8対応
//#define WINVER 0x400

// IPv6対応
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include <malloc.h>
#include <windowsx.h>
// IPv6対応
//#include <winsock.h>
#include <ras.h>
#include <rasdlg.h>
#include <raserror.h>

#include "common.h"
#include "resource.h"


typedef DWORD (WINAPI*FUNC_RASENUMCONNECTIONS) (LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (WINAPI*FUNC_RASENUMENTRIES) (LPCTSTR, LPCTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
typedef DWORD (WINAPI*FUNC_RASHANGUP) (HRASCONN);
typedef BOOL (WINAPI*FUNC_RASDIAL) (LPRASDIALEXTENSIONS, LPCTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN);
typedef BOOL (WINAPI*FUNC_RASGETENTRYDIALPARAMS) (LPCTSTR, LPRASDIALPARAMS, LPBOOL);
typedef BOOL (WINAPI*FUNC_RASGETCONNECTSTATUS) (HRASCONN, LPRASCONNSTATUS);
typedef BOOL (WINAPI*FUNC_RASGETERRORSTRING)(UINT, LPTSTR, DWORD);

typedef BOOL (WINAPI*FUNC_RASDIALDLG) (LPSTR, LPSTR, LPSTR, LPRASDIALDLG);


static int GetCurConnections(RASCONN **Buf);
static DWORD RasHangUpWait(HRASCONN hRasConn);
static int DoDisconnect(RASCONN *Buf, int Num);
static BOOL CALLBACK DialCallBackProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static VOID WINAPI RasDialFunc( UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError );
static void MakeRasConnMsg(char *Phone, RASCONNSTATE rasconn, char *Buf);
static BOOL CALLBACK DialPassCallBackProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/*===== ローカルなワーク =====*/

static HINSTANCE m_hDll = NULL;
static HINSTANCE m_hDll2 = NULL;

static FUNC_RASENUMCONNECTIONS m_RasEnumConnections = NULL;
static FUNC_RASENUMENTRIES m_RasEnumEntries = NULL;
static FUNC_RASHANGUP m_RasHangUp = NULL;
static FUNC_RASDIAL m_RasDial = NULL;
static FUNC_RASGETENTRYDIALPARAMS m_RasGetEntryDialParams = NULL;
static FUNC_RASGETCONNECTSTATUS m_RasGetConnectStatus = NULL;
static FUNC_RASGETERRORSTRING m_RasGetErrorString = NULL;

static FUNC_RASDIALDLG m_RasDialDlg = NULL;

static HWND hWndDial;



/*----- RASライブラリをロードする ---------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*
*	Note
*		RASライブラリは必ずインストールされるものではないので、スタティックに
*		リンクしない。
*----------------------------------------------------------------------------*/

void LoadRasLib(void)
{
	if((m_hDll = LoadLibrary("rasapi32.dll")) != NULL)
	{
		m_RasEnumConnections = (FUNC_RASENUMCONNECTIONS)GetProcAddress(m_hDll, "RasEnumConnectionsA");
		m_RasEnumEntries = (FUNC_RASENUMENTRIES)GetProcAddress(m_hDll, "RasEnumEntriesA");
		m_RasHangUp = (FUNC_RASHANGUP)GetProcAddress(m_hDll, "RasHangUpA");
		m_RasDial = (FUNC_RASDIAL)GetProcAddress(m_hDll, "RasDialA");
		m_RasGetEntryDialParams = (FUNC_RASGETENTRYDIALPARAMS)GetProcAddress(m_hDll, "RasGetEntryDialParamsA");
		m_RasGetConnectStatus = (FUNC_RASGETCONNECTSTATUS)GetProcAddress(m_hDll, "RasGetConnectStatusA");
		m_RasGetErrorString = (FUNC_RASGETERRORSTRING)GetProcAddress(m_hDll, "RasGetErrorStringA");

		if((m_RasEnumConnections == NULL) ||
		   (m_RasEnumEntries == NULL) ||
		   (m_RasHangUp == NULL) ||
		   (m_RasDial == NULL) ||
		   (m_RasGetEntryDialParams == NULL) ||
		   (m_RasGetConnectStatus == NULL) ||
		   (m_RasGetErrorString == NULL))
		{
			FreeLibrary(m_hDll);
			m_hDll = NULL;
		}
	}

	if(m_hDll != NULL)
	{
		if((m_hDll2 = LoadLibrary("rasdlg.dll")) != NULL)
		{
			m_RasDialDlg = (FUNC_RASDIALDLG)GetProcAddress(m_hDll2, "RasDialDlgA");

			if(m_RasDialDlg == NULL)
			{
				FreeLibrary(m_hDll2);
				m_hDll2 = NULL;
			}
		}
	}
	return;
}


/*----- RASライブラリをリリースする -------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ReleaseRasLib(void)
{
	if(m_hDll != NULL)
		FreeLibrary(m_hDll);
	m_hDll = NULL;

	if(m_hDll2 != NULL)
		FreeLibrary(m_hDll2);
	m_hDll2 = NULL;

	return;
}


/*----- RASが使えるかどうかを返す ---------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int RASが使えるかどうか
*			YES/NO
*----------------------------------------------------------------------------*/

int AskRasUsable(void)
{
	int Sts;

	Sts = NO;
	if(m_hDll != NULL)
		Sts = YES;

	return(Sts);
}


/*----- 現在のRASコネクション一覧を返す ---------------------------------------
*
*	Parameter
*		RASCONN **Buf : 一覧へのポインタを返すワーク
*
*	Return Value
*		int 個数 (-1=エラー)
*
*	Note
*		Bufの領域は呼出側で開放すること
*----------------------------------------------------------------------------*/

static int GetCurConnections(RASCONN **Buf)
{
	RASCONN *RasConn;
	RASCONN *Tmp;
	DWORD Size;
	DWORD Num;
	DWORD Sts;
	int Ret;

	Ret = -1;
	Size = sizeof(RASCONN);
	if((RasConn = malloc(Size)) != NULL)
	{
		RasConn->dwSize = sizeof(RASCONN);
		Sts = (*m_RasEnumConnections)(RasConn, &Size, &Num);
		if((Sts == ERROR_BUFFER_TOO_SMALL) || (Sts == ERROR_NOT_ENOUGH_MEMORY))
	    {
			if((Tmp = realloc(RasConn, Size)) != NULL)
	        {
				RasConn = Tmp;
				Sts = (*m_RasEnumConnections)(RasConn, &Size, &Num);
			}
		}

		if(Sts == 0)
		{
			Ret = Num;
			*Buf = RasConn;
		}
		else
			free(RasConn);
	}
	return(Ret);
}


/*----- RasHangUp()------------------------------------------------------------
*
*	Parameter
*		HRASCONN hRasConn : ハンドル
*
*	Return Value
*		DWORD ステータス
*
*	Note
*		切断が完了するまで待つ
*----------------------------------------------------------------------------*/

static DWORD RasHangUpWait(HRASCONN hRasConn)
{
	RASCONNSTATUS RasSts;
	DWORD Sts;

	Sts = (*m_RasHangUp)(hRasConn);

	RasSts.dwSize = sizeof(RASCONNSTATUS);
	while((*m_RasGetConnectStatus)(hRasConn, &RasSts) != ERROR_INVALID_HANDLE)
		Sleep(10);

	return(Sts);
}


/*----- 現在のRASコネクションを切断する ---------------------------------------
*
*	Parameter
*		RASCONN *RasConn : 接続一覧
*		int Num : 個数
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

static int DoDisconnect(RASCONN *RasConn, int Num)
{
	int i;
	int Sts;

	Sts = FFFTP_SUCCESS;
	if(Num > 0)
	{
		SetTaskMsg(MSGJPN220);
		for(i = 0; i < Num; i++)
		{
			if(RasHangUpWait(RasConn[i].hrasconn) != 0)
				Sts = FFFTP_FAIL;
		}
	}
	return(Sts);
}


/*----- RASを切断する ---------------------------------------------------------
*
*	Parameter
*		int Notify : 確認するかどうか (YES/NO)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DisconnectRas(int Notify)
{
	RASCONN *RasConn;
	int Num;

	if(m_hDll != NULL)
	{
		if((Num = GetCurConnections(&RasConn)) != -1)
		{
			if(Num > 0)
			{
				if((Notify == NO) ||
				   (DialogBox(GetFtpInst(), MAKEINTRESOURCE(rasnotify_dlg), GetMainHwnd(), ExeEscDialogProc) == YES))
				{
					DoDisconnect(RasConn, Num);
				}
			}
			free(RasConn);
		}
	}
	return;
}


/*----- RASのエントリ一覧をコンボボックスにセットする -------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのハンドル
*		int Item : コンボボックスのリソース番号
*		char *CurName : 初期値
*
*	Return Value
*		int エントリ数
*----------------------------------------------------------------------------*/

int SetRasEntryToComboBox(HWND hDlg, int Item, char *CurName)
{
	RASENTRYNAME *Entry;
	RASENTRYNAME *Tmp;
	DWORD i;
	DWORD Size;
	DWORD Num;
	DWORD Sts;

	Num = 0;
	if(m_hDll != NULL)
	{
		Size = sizeof(RASENTRYNAME);
		if((Entry = malloc(Size)) != NULL)
		{
			Entry->dwSize = sizeof(RASENTRYNAME);
			Sts = (*m_RasEnumEntries)(NULL, NULL, Entry, &Size, &Num);
			if((Sts == ERROR_BUFFER_TOO_SMALL) || (Sts == ERROR_NOT_ENOUGH_MEMORY))
			{
				if((Tmp = realloc(Entry, Size)) != NULL)
		        {
					Entry = Tmp;
					Sts = (*m_RasEnumEntries)(NULL, NULL, Entry, &Size, &Num);
				}
			}

			if((Sts == 0) && (Num > 0))
			{
				for(i = 0; i < Num; i++)
					SendDlgItemMessage(hDlg, Item, CB_ADDSTRING, 0, (LPARAM)Entry[i].szEntryName);

				SendDlgItemMessage(hDlg, Item, CB_SELECTSTRING, -1, (LPARAM)CurName);
			}
			free(Entry);
		}
	}
	return(Num);
}


/*----- RASの接続を行う -------------------------------------------------------
*
*	Parameter
*		int Dialup : ダイアルアップするかどうか (YES/NO)
*		int UseThis : 必ずこのエントリに接続するかどうか (YES/NO)
*		int Notify : 再接続前に確認するかどうか (YES/NO)
*		char *Name : 接続先
*
*	Return Value
*		int ステータス (FFFTP_SUCCESS/FFFTP_FAIL)
*----------------------------------------------------------------------------*/

int ConnectRas(int Dialup, int UseThis, int Notify, char *Name)
{
	RASDIALDLG DlgParam;
	RASCONN *RasConn;
	int i;
	int Num;
	int Sts;
	int DoDial;
	RASDIALPARAMS Param;
	BOOL Flg;
	OSVERSIONINFO VerInfo;

	Sts = FFFTP_SUCCESS;
	if(Dialup == YES)
	{
		if(m_hDll != NULL)
		{
			/* 現在の接続を確認 */
			DoDial = 1;
			if((Num = GetCurConnections(&RasConn)) != -1)
			{
				if(Num > 0)
				{
					DoDial = 0;
					if(UseThis == YES)
					{
						DoDial = 2;
						for(i = 0; i < Num; i++)
						{
							if(_mbscmp(RasConn[i].szEntryName, Name) == 0)
								DoDial = 0;
						}

						if(DoDial == 2)
						{
							if((Notify == NO) ||
							   (DialogBox(GetFtpInst(), MAKEINTRESOURCE(rasreconnect_dlg), GetMainHwnd(), ExeEscDialogProc) == YES))
							{
								DoDisconnect(RasConn, Num);
							}
							else
								DoDial = 0;
						}
					}
				}
				free(RasConn);
			}

			if(DoDial != 0)
			{
				/* 接続する */
				SetTaskMsg(MSGJPN221);
				Sts = FFFTP_FAIL;

				Num = 0;
				VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				if(GetVersionEx(&VerInfo) == TRUE)
				{
					if(VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
						Num = 1;
				}

				if(Num == 1)
				{
					if(m_hDll2 != NULL)
					{
						/* Windows NT */
						DlgParam.dwSize = sizeof(RASDIALDLG);
						DlgParam.hwndOwner = GetMainHwnd();
						DlgParam.dwFlags = 0;
						DlgParam.dwSubEntry = 0;
						DlgParam.reserved = 0;
						DlgParam.reserved2 = 0;
						if((*m_RasDialDlg)(NULL, Name, NULL, &DlgParam) != 0)
							Sts = FFFTP_SUCCESS;
					}
					else
						SetTaskMsg(MSGJPN222);
				}
				else
				{
					/* Windows 95,98 */
					Param.dwSize = sizeof(RASDIALPARAMS);
					strcpy(Param.szEntryName, Name);
					Flg = TRUE;
					Sts = (*m_RasGetEntryDialParams)(NULL, &Param, &Flg);
					strcpy(Param.szPhoneNumber, "");

					if(((strlen(Param.szUserName) != 0) && (strlen(Param.szPassword) != 0)) ||
					   (DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(dial_password_dlg), GetMainHwnd(), (DLGPROC)DialPassCallBackProc, (LPARAM)&Param) == YES))
					{
						if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(dial_dlg), GetMainHwnd(), (DLGPROC)DialCallBackProc, (LPARAM)&Param) == YES)
							Sts = FFFTP_SUCCESS;
					}
				}
			}
		}
	}
	return(Sts);
}


/*----- RASダイアルウインドウのコールバック -----------------------------------
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

static BOOL CALLBACK DialCallBackProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RASDIALPARAMS *Param;
	static HRASCONN hRasConn;
	char Tmp[RAS_NAME_LEN+25];
	DWORD Sts;

	switch (message)
	{
		case WM_INITDIALOG :
			Param = (RASDIALPARAMS *)lParam;
			hWndDial = hDlg;
			hRasConn = NULL;
			sprintf(Tmp, MSGJPN223, Param->szEntryName);
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)Tmp);
			Sts = (*m_RasDial)(NULL, NULL, Param, 0, &RasDialFunc, &hRasConn);
			if(Sts != 0)
		        EndDialog(hDlg, NO);
		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDCANCEL :
					RasHangUpWait(hRasConn);
					EndDialog(hDlg, NO);
					break;
			}
			return(TRUE);

		case WM_DIAL_MSG :
			if(lParam != 0)
			{
				/* エラー発生 */
				if((*m_RasGetErrorString)((UINT)lParam, (LPTSTR)Tmp, RAS_NAME_LEN+25) != 0)
					sprintf(Tmp, MSGJPN224);
				RasHangUpWait(hRasConn);
				MessageBox(hDlg, Tmp, MSGJPN225, MB_OK | MB_ICONSTOP);
				EndDialog(hDlg, NO);
			}
			else if(wParam & RASCS_DONE)
			{
				/* 終了 */
				EndDialog(hDlg, YES);
			}
			else
			{
				/* ステータス変更 */
				MakeRasConnMsg(Param->szPhoneNumber, wParam, Tmp);
				SendDlgItemMessage(hDlg, DIAL_STATUS, WM_SETTEXT, 0, (LPARAM)Tmp);
			}
			return(TRUE);
	}
	return(FALSE);
}


/*----- RasDial()のコールバック -----------------------------------------------
*
*	Parameter
*		UINT unMsg : メッセージ
*		RASCONNSTATE rasconnstate : ステータス
*		DWORD dwError : エラー
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static VOID WINAPI RasDialFunc(UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError)
{
	PostMessage(hWndDial, WM_DIAL_MSG, (WPARAM)rasconnstate, (LPARAM)dwError );
}


/*----- RasDial()のステータスメッセージを作成する -----------------------------
*
*	Parameter
*		char *Phone : 電話番号
*		RASCONNSTATE rasconn : ステータス
*		char *Buf : 文字列をセットするバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void MakeRasConnMsg(char *Phone, RASCONNSTATE rasconn, char *Buf)
{
    switch( rasconn )
    {
        case RASCS_OpenPort:
            strcpy(Buf, MSGJPN226);
			break;
        case RASCS_PortOpened:
            strcpy(Buf, MSGJPN227);
			break;
        case RASCS_ConnectDevice:
            sprintf(Buf, MSGJPN228);
			break;
        case RASCS_DeviceConnected:
            strcpy(Buf, MSGJPN229);
			break;
        case RASCS_AllDevicesConnected:
            strcpy(Buf, MSGJPN230);
			break;
        case RASCS_Authenticate:
        case RASCS_AuthNotify:
            strcpy(Buf, MSGJPN231);
			break;
        case RASCS_AuthRetry:
        case RASCS_ReAuthenticate:
            strcpy(Buf, MSGJPN232);
			break;
        case RASCS_AuthChangePassword:
            strcpy(Buf, MSGJPN233);
			break;
        case RASCS_Authenticated:
            strcpy(Buf, MSGJPN234);
			break;
        case RASCS_Connected:
            strcpy(Buf, MSGJPN235);
			break;
        case RASCS_Disconnected:
            strcpy(Buf, MSGJPN236);
			break;
        case RASCS_AuthCallback:
        case RASCS_PrepareForCallback:
        case RASCS_WaitForModemReset:
        case RASCS_WaitForCallback:
        case RASCS_Interactive:
        case RASCS_RetryAuthentication:
        case RASCS_CallbackSetByCaller:
        case RASCS_PasswordExpired:
        case RASCS_AuthProject:
        case RASCS_AuthLinkSpeed:
        case RASCS_AuthAck:
        default:
            strcpy(Buf, MSGJPN237);
			break;
    }
	return;
}


/*----- ユーザ名、パスワード入力ウインドウのコールバック ----------------------
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

static BOOL CALLBACK DialPassCallBackProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RASDIALPARAMS *Param;
	char Tmp[RAS_NAME_LEN+25];

	switch (message)
	{
		case WM_INITDIALOG :
			Param = (RASDIALPARAMS *)lParam;
			sprintf(Tmp, MSGJPN238, Param->szEntryName);
			SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)Tmp);
			SendDlgItemMessage(hDlg, RASPASS_USER, EM_LIMITTEXT, UNLEN, 0);
			SendDlgItemMessage(hDlg, RASPASS_PASS, EM_LIMITTEXT, PWLEN, 0);
			SendDlgItemMessage(hDlg, RASPASS_USER, WM_SETTEXT, 0, (LPARAM)Param->szUserName);
			SendDlgItemMessage(hDlg, RASPASS_PASS, WM_SETTEXT, 0, (LPARAM)Param->szPassword);
		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, RASPASS_USER, WM_GETTEXT, UNLEN+1, (LPARAM)Param->szUserName);
					SendDlgItemMessage(hDlg, RASPASS_PASS, WM_GETTEXT, PWLEN+1, (LPARAM)Param->szPassword);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;
			}
			return(TRUE);
	}
	return(FALSE);
}


