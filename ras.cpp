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

#include "common.h"
#include <Ras.h>
#include <RasDlg.h>
#include <RasError.h>
// これらはリンクオプションで遅延ロードに設定する
#pragma comment(lib, "Rasapi32.lib")
#pragma comment(lib, "Rasdlg.lib")


static int GetCurConnections(RASCONN **Buf);
static DWORD RasHangUpWait(HRASCONN hRasConn);
static int DoDisconnect(RASCONN *Buf, int Num);
static void MakeRasConnMsg(char *Phone, RASCONNSTATE rasconn, char *Buf);


/*===== ローカルなワーク =====*/

static HWND hWndDial;


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
	if((RasConn = (RASCONN*)malloc(Size)) != NULL)
	{
		RasConn->dwSize = sizeof(RASCONN);
		Sts = RasEnumConnections(RasConn, &Size, &Num);
		if((Sts == ERROR_BUFFER_TOO_SMALL) || (Sts == ERROR_NOT_ENOUGH_MEMORY))
		{
			if((Tmp = (RASCONN*)realloc(RasConn, Size)) != NULL)
			{
				RasConn = Tmp;
				Sts = RasEnumConnections(RasConn, &Size, &Num);
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

	Sts = RasHangUp(hRasConn);

	RasSts.dwSize = sizeof(RASCONNSTATUS);
	while(RasGetConnectStatus(hRasConn, &RasSts) != ERROR_INVALID_HANDLE)
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


// RASを切断する
void DisconnectRas(int Notify) {
	RASCONN* RasConn;
	if (int Num = GetCurConnections(&RasConn); Num != -1) {
		if (0 < Num && (Notify == NO || DialogBox(GetFtpInst(), MAKEINTRESOURCE(rasnotify_dlg), GetMainHwnd(), ExeEscDialogProc) == YES))
			DoDisconnect(RasConn, Num);
		free(RasConn);
	}
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
		Size = sizeof(RASENTRYNAME);
		if((Entry = (RASENTRYNAME*)malloc(Size)) != NULL)
		{
			Entry->dwSize = sizeof(RASENTRYNAME);
			Sts = RasEnumEntries(NULL, NULL, Entry, &Size, &Num);
			if((Sts == ERROR_BUFFER_TOO_SMALL) || (Sts == ERROR_NOT_ENOUGH_MEMORY))
			{
				if((Tmp = (RASENTRYNAME*)realloc(Entry, Size)) != NULL)
				{
					Entry = Tmp;
					Sts = RasEnumEntries(NULL, NULL, Entry, &Size, &Num);
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

int ConnectRas(int Dialup, int UseThis, int Notify, char *Name) {
	if (Dialup != YES)
		return FFFTP_SUCCESS;

	/* 現在の接続を確認 */
	bool DoDial = true;
	RASCONN *RasConn;
	if (int Num = GetCurConnections(&RasConn); Num != -1) {
		if (Num > 0) {
			if (UseThis == YES) {
				for (int i = 0; i < Num; i++)
					if (_mbscmp((const unsigned char*)RasConn[i].szEntryName, (const unsigned char*)Name) == 0) {
						DoDial = false;
						break;
					}
				if (DoDial) {
					if (Notify == NO || DialogBox(GetFtpInst(), MAKEINTRESOURCE(rasreconnect_dlg), GetMainHwnd(), ExeEscDialogProc) == YES)
						DoDisconnect(RasConn, Num);
					else
						DoDial = false;
				}
			} else
				DoDial = false;
		}
		free(RasConn);
	}
	if (!DoDial)
		return FFFTP_SUCCESS;

	/* 接続する */
	SetTaskMsg(MSGJPN221);
	RASDIALDLG DlgParam{ sizeof(RASDIALDLG), GetMainHwnd() };
	return RasDialDlg(NULL, Name, NULL, &DlgParam) != 0 ? FFFTP_SUCCESS : FFFTP_FAIL;
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
