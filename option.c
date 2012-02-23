/*=============================================================================
*
*								オプション設定
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
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commctrl.h>
#include <windowsx.h>

#include "common.h"
#include "resource.h"

#include <htmlhelp.h>
#include "helpid.h"

// UTF-8対応
#undef __MBSWRAPPER_H__
#include "mbswrapper.h"


/*===== プロトタイプ =====*/

// 64ビット対応
//static BOOL CALLBACK UserSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK Trmode1SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK Trmode2SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK Trmode3SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK DefAttrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK UserSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK Trmode1SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK Trmode2SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK Trmode3SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DefAttrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void AddFnameAttrToListView(HWND hDlg, char *Fname, char *Attr);
static void GetFnameAttrFromListView(HWND hDlg, char *Buf);
// 64ビット対応
//static BOOL CALLBACK MirrorSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK NotifySettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK DispSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK MirrorSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK NotifySettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DispSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static int SelectListFont(HWND hWnd, LOGFONT *lFont);
// 64ビット対応
//static BOOL CALLBACK ConnectSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK FireSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK ToolSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK SoundSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK MiscSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK SortSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK ConnectSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK FireSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK ToolSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK SoundSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK MiscSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK SortSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
// hostman.cで使用
//static int GetDecimalText(HWND hDlg, int Ctrl);
//static void SetDecimalText(HWND hDlg, int Ctrl, int Num);
//static void CheckRange2(int *Cur, int Max, int Min);
//static void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize);
//static void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text);
//static void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize);
int GetDecimalText(HWND hDlg, int Ctrl);
void SetDecimalText(HWND hDlg, int Ctrl, int Num);
void CheckRange2(int *Cur, int Max, int Min);
void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize);
void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text);
void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize);



typedef struct {
	char Fname[FMAX_PATH+1];
	char Attr[5];
} ATTRSET;





/*===== 外部参照 =====*/

extern HWND hHelpWin;

/* 設定値 */
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern char ViewerName[VIEWERS][FMAX_PATH+1];
extern int ConnectOnStart;
extern int DebugConsole;
extern int SaveWinPos;
extern char AsciiExt[ASCII_EXT_LEN+1];
extern int RecvMode;
extern int SendMode;
extern int MoveMode;
extern int CacheEntry;
extern int CacheSave;
extern char FwallHost[HOST_ADRS_LEN+1];
extern char FwallUser[USER_NAME_LEN+1];
extern char FwallPass[PASSWORD_LEN+1];
extern int FwallPort;
extern int FwallType;
extern int FwallDefault;
extern int FwallSecurity;
extern int FwallResolv;
extern int FwallLower;
extern int FwallDelimiter;
extern int PasvDefault;
extern char DefaultLocalPath[FMAX_PATH+1];
extern int SaveTimeStamp;
extern int DclickOpen;
extern SOUNDFILE Sound[SOUND_TYPES];
extern int FnameCnv;
extern int ConnectAndSet;
extern int TimeOut;
extern int RmEOF;
extern int RegType;
extern char MirrorNoTrn[MIRROR_LEN+1];
extern char MirrorNoDel[MIRROR_LEN+1];
extern int MirrorFnameCnv;
extern int RasClose;
extern int RasCloseNotify;
extern int FileHist;
extern char DefAttrList[DEFATTRLIST_LEN+1];
extern char TmpPath[FMAX_PATH+1];
extern int QuickAnonymous;
extern int PassToHist;
extern int VaxSemicolon;
extern int SendQuit;
extern int NoRasControl;
extern int DispIgnoreHide;
extern int DispDrives;
extern HFONT ListFont;
extern LOGFONT ListLogFont;
extern int MirUpDelNotify;
extern int MirDownDelNotify;
extern int FolderAttr;
extern int FolderAttrNum;
// ファイルアイコン表示対応
extern int DispFileIcon;
// ディレクトリ自動作成
extern int MakeAllDir;


/*----- オプションのプロパティシート ------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SetOption(int Start)
{
	PROPSHEETPAGE psp[12];
	PROPSHEETHEADER psh;

	// 変数が未初期化のバグ修正
	memset(&psp, 0, sizeof(psp));
	memset(&psh, 0, sizeof(psh));

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[0].hInstance = GetFtpInst();
	psp[0].pszTemplate = MAKEINTRESOURCE(opt_user_dlg);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = UserSettingProc;
	psp[0].pszTitle = MSGJPN186;
	psp[0].lParam = 0;
	psp[0].pfnCallback = NULL;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[1].hInstance = GetFtpInst();
	psp[1].pszTemplate = MAKEINTRESOURCE(opt_trmode1_dlg);
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = Trmode1SettingProc;
	psp[1].pszTitle = MSGJPN187;
	psp[1].lParam = 0;
	psp[1].pfnCallback = NULL;

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
	psp[2].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[2].hInstance = GetFtpInst();
	psp[2].pszTemplate = MAKEINTRESOURCE(opt_trmode2_dlg);
	psp[2].pszIcon = NULL;
	psp[2].pfnDlgProc = Trmode2SettingProc;
	psp[2].pszTitle = MSGJPN188;
	psp[2].lParam = 0;
	psp[2].pfnCallback = NULL;

	psp[3].dwSize = sizeof(PROPSHEETPAGE);
	psp[3].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[3].hInstance = GetFtpInst();
	psp[3].pszTemplate = MAKEINTRESOURCE(opt_trmode3_dlg);
	psp[3].pszIcon = NULL;
	psp[3].pfnDlgProc = Trmode3SettingProc;
	psp[3].pszTitle = MSGJPN189;
	psp[3].lParam = 0;
	psp[3].pfnCallback = NULL;

	psp[4].dwSize = sizeof(PROPSHEETPAGE);
	psp[4].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[4].hInstance = GetFtpInst();
	psp[4].pszTemplate = MAKEINTRESOURCE(opt_mirror_dlg);
	psp[4].pszIcon = NULL;
	psp[4].pfnDlgProc = MirrorSettingProc;
	psp[4].pszTitle = MSGJPN190;
	psp[4].lParam = 0;
	psp[4].pfnCallback = NULL;

	psp[5].dwSize = sizeof(PROPSHEETPAGE);
	psp[5].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[5].hInstance = GetFtpInst();
	psp[5].pszTemplate = MAKEINTRESOURCE(opt_notify_dlg);
	psp[5].pszIcon = NULL;
	psp[5].pfnDlgProc = NotifySettingProc;
	psp[5].pszTitle = MSGJPN191;
	psp[5].lParam = 0;
	psp[5].pfnCallback = NULL;

	psp[6].dwSize = sizeof(PROPSHEETPAGE);
	psp[6].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[6].hInstance = GetFtpInst();
	psp[6].pszTemplate = MAKEINTRESOURCE(opt_disp_dlg);
	psp[6].pszIcon = NULL;
	psp[6].pfnDlgProc = DispSettingProc;
	psp[6].pszTitle = MSGJPN192;
	psp[6].lParam = 0;
	psp[6].pfnCallback = NULL;

	psp[7].dwSize = sizeof(PROPSHEETPAGE);
	psp[7].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[7].hInstance = GetFtpInst();
	psp[7].pszTemplate = MAKEINTRESOURCE(opt_connect_dlg);
	psp[7].pszIcon = NULL;
	psp[7].pfnDlgProc = ConnectSettingProc;
	psp[7].pszTitle = MSGJPN193;
	psp[7].lParam = 0;
	psp[7].pfnCallback = NULL;

	psp[8].dwSize = sizeof(PROPSHEETPAGE);
	psp[8].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[8].hInstance = GetFtpInst();
	psp[8].pszTemplate = MAKEINTRESOURCE(opt_fire_dlg);
	psp[8].pszIcon = NULL;
	psp[8].pfnDlgProc = FireSettingProc;
	psp[8].pszTitle = MSGJPN194;
	psp[8].lParam = 0;
	psp[8].pfnCallback = NULL;

	psp[9].dwSize = sizeof(PROPSHEETPAGE);
	psp[9].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[9].hInstance = GetFtpInst();
	psp[9].pszTemplate = MAKEINTRESOURCE(opt_tool_dlg);
	psp[9].pszIcon = NULL;
	psp[9].pfnDlgProc = ToolSettingProc;
	psp[9].pszTitle = MSGJPN195;
	psp[9].lParam = 0;
	psp[9].pfnCallback = NULL;

	psp[10].dwSize = sizeof(PROPSHEETPAGE);
	psp[10].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[10].hInstance = GetFtpInst();
	psp[10].pszTemplate = MAKEINTRESOURCE(opt_sound_dlg);
	psp[10].pszIcon = NULL;
	psp[10].pfnDlgProc = SoundSettingProc;
	psp[10].pszTitle = MSGJPN196;
	psp[10].lParam = 0;
	psp[10].pfnCallback = NULL;

	psp[11].dwSize = sizeof(PROPSHEETPAGE);
	psp[11].dwFlags = PSP_USETITLE | PSP_HASHELP;
	psp[11].hInstance = GetFtpInst();
	psp[11].pszTemplate = MAKEINTRESOURCE(opt_misc_dlg);
	psp[11].pszIcon = NULL;
	psp[11].pfnDlgProc = MiscSettingProc;
	psp[11].pszTitle = MSGJPN197;
	psp[11].lParam = 0;
	psp[11].pfnCallback = NULL;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_HASHELP | PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
	psh.hwndParent = GetMainHwnd();
	psh.hInstance = GetFtpInst();
	psh.pszIcon = NULL;
	psh.pszCaption = MSGJPN198;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = Start;
	psh.ppsp = (LPCPROPSHEETPAGE)&psp;
	psh.pfnCallback = NULL;

	PropertySheet(&psh);
	return;
}


/*----- ユーザ設定ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK UserSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK UserSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, USER_ADRS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SendDlgItemMessage(hDlg, USER_ADRS, WM_SETTEXT, 0, (LPARAM)UserMailAdrs);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SendDlgItemMessage(hDlg, USER_ADRS, WM_GETTEXT, USER_MAIL_LEN+1, (LPARAM)UserMailAdrs);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000041);
					break;
			}
			break;
	}
    return(FALSE);
}


/*----- 転送設定１ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK Trmode1SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK Trmode1SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	int Num;
	char Tmp[FMAX_PATH+1];
	int Trash;

	static const RADIOBUTTON ModeButton[] = {
		{ TRMODE_AUTO, TYPE_X },
		{ TRMODE_ASCII, TYPE_A },
		{ TRMODE_BIN, TYPE_I }
	};
	#define MODEBUTTONS	(sizeof(ModeButton)/sizeof(RADIOBUTTON))

	switch (message)
	{
		case WM_INITDIALOG :
			SetMultiTextToList(hDlg, TRMODE_EXT_LIST, AsciiExt);
			SetRadioButtonByValue(hDlg, AskTransferType(), ModeButton, MODEBUTTONS);
			SendDlgItemMessage(hDlg, TRMODE_TIME, BM_SETCHECK, SaveTimeStamp, 0);
			SendDlgItemMessage(hDlg, TRMODE_EOF, BM_SETCHECK, RmEOF, 0);
			SendDlgItemMessage(hDlg, TRMODE_SEMICOLON, BM_SETCHECK, VaxSemicolon, 0);
			// ディレクトリ自動作成
			SendDlgItemMessage(hDlg, TRMODE_MAKEDIR, BM_SETCHECK, MakeAllDir, 0);

			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(TRMODE_EXT_LIST, 0), 0);

		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SetTransferTypeImm(AskRadioButtonValue(hDlg, ModeButton, MODEBUTTONS));
					SaveTransferType();
					GetMultiTextFromList(hDlg, TRMODE_EXT_LIST, AsciiExt, ASCII_EXT_LEN+1);
					SaveTimeStamp = SendDlgItemMessage(hDlg, TRMODE_TIME, BM_GETCHECK, 0, 0);
					RmEOF = SendDlgItemMessage(hDlg, TRMODE_EOF, BM_GETCHECK, 0, 0);
					VaxSemicolon = SendDlgItemMessage(hDlg, TRMODE_SEMICOLON, BM_GETCHECK, 0, 0);
					// ディレクトリ自動作成
					MakeAllDir = SendDlgItemMessage(hDlg, TRMODE_MAKEDIR, BM_GETCHECK, 0, 0);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000042);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case TRMODE_ASCII :
				case TRMODE_BIN :
					EnableWindow(GetDlgItem(hDlg, TRMODE_EXT_LIST), FALSE);
					EnableWindow(GetDlgItem(hDlg, TRMODE_ADD), FALSE);
					EnableWindow(GetDlgItem(hDlg, TRMODE_DEL), FALSE);
					break;

				case TRMODE_AUTO :
					EnableWindow(GetDlgItem(hDlg, TRMODE_EXT_LIST), TRUE);
					EnableWindow(GetDlgItem(hDlg, TRMODE_ADD), TRUE);
					EnableWindow(GetDlgItem(hDlg, TRMODE_DEL), TRUE);
					break;

				case TRMODE_ADD :
					strcpy(Tmp, "");
					if(InputDialogBox(fname_in_dlg, hDlg, MSGJPN199, Tmp, FMAX_PATH, &Trash, IDH_HELP_TOPIC_0000001) == YES)
						AddTextToListBox(hDlg, Tmp,  TRMODE_EXT_LIST, ASCII_EXT_LEN+1);
					break;

				case TRMODE_DEL :
					if((Num = SendDlgItemMessage(hDlg, TRMODE_EXT_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
						SendDlgItemMessage(hDlg, TRMODE_EXT_LIST, LB_DELETESTRING, Num, 0);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- 転送設定２ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK Trmode2SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK Trmode2SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	char Tmp[FMAX_PATH+1];

	static const RADIOBUTTON CnvButton[] = {
		{ TRMODE2_NOCNV, FNAME_NOCNV },
		{ TRMODE2_LOWER, FNAME_LOWER },
		{ TRMODE2_UPPER, FNAME_UPPER }
	};
	#define CNVBUTTONS	(sizeof(CnvButton)/sizeof(RADIOBUTTON))

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, TRMODE2_LOCAL, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, TRMODE2_LOCAL, WM_SETTEXT, 0, (LPARAM)DefaultLocalPath);

			SetRadioButtonByValue(hDlg, FnameCnv, CnvButton, CNVBUTTONS);

			SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT, EM_LIMITTEXT, (WPARAM)5, 0);
			sprintf(Tmp, "%d", TimeOut);
			SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT, WM_SETTEXT, 0, (LPARAM)Tmp);
			SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT_SPN, UDM_SETRANGE, 0, MAKELONG(300, 0));
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SendDlgItemMessage(hDlg, TRMODE2_LOCAL, WM_GETTEXT, FMAX_PATH+1, (LPARAM)DefaultLocalPath);

					FnameCnv = AskRadioButtonValue(hDlg, CnvButton, CNVBUTTONS);

					SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT, WM_GETTEXT, 5+1, (LPARAM)Tmp);
					TimeOut = atoi(Tmp);
					CheckRange2(&TimeOut, 300, 0);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000043);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case TRMODE2_LOCAL_BR :
					if(SelectDir(hDlg, Tmp, FMAX_PATH) == TRUE)
						SendDlgItemMessage(hDlg, TRMODE2_LOCAL, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- 転送設定３ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK Trmode3SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK Trmode3SettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	LV_COLUMN LvCol;
	long Tmp;
	RECT Rect;
	ATTRSET AttrSet;
	char *Fname;
	char *Attr;
	char TmpStr[10];

	switch (message)
	{
		case WM_INITDIALOG :
		    Tmp = SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
		    Tmp |= LVS_EX_FULLROWSELECT;
	    	SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)Tmp);

			GetClientRect(GetDlgItem(hDlg, TRMODE3_LIST), &Rect);

			LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
			LvCol.cx = (Rect.right / 3) * 2;
			LvCol.pszText = MSGJPN200;
			LvCol.iSubItem = 0;
			SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

			LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
			LvCol.cx = Rect.right - LvCol.cx;
			LvCol.pszText = MSGJPN201;
			LvCol.iSubItem = 1;
			SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

			Fname = DefAttrList;
			while(*Fname != NUL)
			{
				Attr = strchr(Fname, NUL) + 1;
				if(*Attr != NUL)
					AddFnameAttrToListView(hDlg, Fname, Attr);
				Fname = strchr(Attr, NUL) + 1;
			}

			SendDlgItemMessage(hDlg, TRMODE3_FOLDER, BM_SETCHECK, FolderAttr, 0);
			if(FolderAttr == NO)
				EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), FALSE);

			SendDlgItemMessage(hDlg, TRMODE3_FOLDER_ATTR, EM_LIMITTEXT, (WPARAM)5, 0);
			sprintf(TmpStr, "%03d", FolderAttrNum);
			SendDlgItemMessage(hDlg, TRMODE3_FOLDER_ATTR, WM_SETTEXT, 0, (LPARAM)TmpStr);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					GetFnameAttrFromListView(hDlg, DefAttrList);
					SendDlgItemMessage(hDlg, TRMODE3_FOLDER_ATTR, WM_GETTEXT, 5+1, (LPARAM)TmpStr);
					FolderAttrNum = atoi(TmpStr);
					FolderAttr = SendDlgItemMessage(hDlg, TRMODE3_FOLDER, BM_GETCHECK, 0, 0);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000044);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case TRMODE3_ADD :
					if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(def_attr_dlg), hDlg, DefAttrDlgProc, (LPARAM)&AttrSet) == YES)
					{
						if((strlen(AttrSet.Fname) > 0) && (strlen(AttrSet.Attr) > 0))
							AddFnameAttrToListView(hDlg, AttrSet.Fname, AttrSet.Attr);
					}
					break;

				case TRMODE3_DEL :
					if((Tmp = SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_ALL | LVNI_SELECTED, 0))) != -1)
						SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_DELETEITEM, Tmp, 0);
					break;

				case TRMODE3_FOLDER :
					if(SendDlgItemMessage(hDlg, TRMODE3_FOLDER, BM_GETCHECK, 0, 0) == 1)
						EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), TRUE);
					else
						EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), FALSE);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ファイル属性設定ウインドウのコールバック ------------------------------
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
//static BOOL CALLBACK DefAttrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK DefAttrDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static ATTRSET *AttrSet;
	char Tmp[5];

	switch (message)
	{
		case WM_INITDIALOG :
			AttrSet = (ATTRSET *)lParam;
			SendDlgItemMessage(hDlg, DEFATTR_FNAME, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, DEFATTR_ATTR, EM_LIMITTEXT, (WPARAM)4, 0);
		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, DEFATTR_FNAME, WM_GETTEXT, (WPARAM)FMAX_PATH+1, (LPARAM)AttrSet->Fname);
					SendDlgItemMessage(hDlg, DEFATTR_ATTR, WM_GETTEXT, (WPARAM)4+1, (LPARAM)AttrSet->Attr);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case DEFATTR_ATTR_BR :
					SendDlgItemMessage(hDlg, DEFATTR_ATTR, WM_GETTEXT, (WPARAM)4+1, (LPARAM)Tmp);
					if(DialogBoxParam(GetFtpInst(), MAKEINTRESOURCE(chmod_dlg), GetMainHwnd(), ChmodDialogCallBack, (LPARAM)Tmp) == YES)
						SendDlgItemMessage(hDlg, DEFATTR_ATTR, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ファイル名と属性をリストビューに追加 ----------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		char *Fname : ファイル名
*		char *Attr : 属性
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AddFnameAttrToListView(HWND hDlg, char *Fname, char *Attr)
{
	int Num;
	LV_ITEM LvItem;
	char Buf[DEFATTRLIST_LEN+1];

	GetFnameAttrFromListView(hDlg, Buf);
	if(StrMultiLen(Buf) + strlen(Fname) + strlen(Attr) + 2 <= DEFATTRLIST_LEN)
	{
		Num = SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEMCOUNT, 0, 0);

		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Num;
		LvItem.iSubItem = 0;
		LvItem.pszText = Fname;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Num;
		LvItem.iSubItem = 1;
		LvItem.pszText = Attr;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
	else
		MessageBeep(-1);

	return;
}


/*----- リストビューの内容をマルチ文字列にする --------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int CtrlList : リストボックスのID
*		char *Buf : 文字列をセットするバッファ
*		int BufSize : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void GetFnameAttrFromListView(HWND hDlg, char *Buf)
{
	int Num;
	int i;
	LV_ITEM LvItem;

	Num = SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEMCOUNT, 0, 0);
	for(i = 0; i < Num; i++)
	{
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 0;
		LvItem.pszText = Buf;
		LvItem.cchTextMax = FMAX_PATH;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEM, 0, (LPARAM)&LvItem);
		Buf = strchr(Buf, NUL) + 1;

		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 1;
		LvItem.pszText = Buf;
		LvItem.cchTextMax = FMAX_PATH;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEM, 0, (LPARAM)&LvItem);
		Buf = strchr(Buf, NUL) + 1;
	}
	*Buf = NUL;

	return;
}


/*----- ミラーリングウインドウのコールバック ----------------------------------
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
//static BOOL CALLBACK MirrorSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK MirrorSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	int Num;
	char Tmp[FMAX_PATH+1];
	int Trash;

	switch (message)
	{
		case WM_INITDIALOG :
			SetMultiTextToList(hDlg, MIRROR_NOTRN_LIST, MirrorNoTrn);
			SetMultiTextToList(hDlg, MIRROR_NODEL_LIST, MirrorNoDel);
			SendDlgItemMessage(hDlg, MIRROR_LOW, BM_SETCHECK, MirrorFnameCnv, 0);
			SendDlgItemMessage(hDlg, MIRROR_UPDEL_NOTIFY, BM_SETCHECK, MirUpDelNotify, 0);
			SendDlgItemMessage(hDlg, MIRROR_DOWNDEL_NOTIFY, BM_SETCHECK, MirDownDelNotify, 0);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					GetMultiTextFromList(hDlg, MIRROR_NOTRN_LIST, MirrorNoTrn, MIRROR_LEN+1);
					GetMultiTextFromList(hDlg, MIRROR_NODEL_LIST, MirrorNoDel, MIRROR_LEN+1);
					MirrorFnameCnv = SendDlgItemMessage(hDlg, MIRROR_LOW, BM_GETCHECK, 0, 0);
					MirUpDelNotify = SendDlgItemMessage(hDlg, MIRROR_UPDEL_NOTIFY, BM_GETCHECK, 0, 0);
					MirDownDelNotify = SendDlgItemMessage(hDlg, MIRROR_DOWNDEL_NOTIFY, BM_GETCHECK, 0, 0);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000045);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case MIRROR_NOTRN_ADD :
					strcpy(Tmp, "");
					if(InputDialogBox(fname_in_dlg, hDlg, MSGJPN202, Tmp, FMAX_PATH, &Trash, IDH_HELP_TOPIC_0000001) == YES)
						AddTextToListBox(hDlg, Tmp, MIRROR_NOTRN_LIST, MIRROR_LEN+1);
					break;

				case MIRROR_NODEL_ADD :
					strcpy(Tmp, "");
					if(InputDialogBox(fname_in_dlg, hDlg, MSGJPN203, Tmp, FMAX_PATH, &Trash, IDH_HELP_TOPIC_0000001) == YES)
						AddTextToListBox(hDlg, Tmp, MIRROR_NODEL_LIST, MIRROR_LEN+1);
					break;

				case MIRROR_NOTRN_DEL :
					if((Num = SendDlgItemMessage(hDlg, MIRROR_NOTRN_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
						SendDlgItemMessage(hDlg, MIRROR_NOTRN_LIST, LB_DELETESTRING, Num, 0);
					break;

				case MIRROR_NODEL_DEL :
					if((Num = SendDlgItemMessage(hDlg, MIRROR_NODEL_LIST, LB_GETCURSEL, 0, 0)) != LB_ERR)
						SendDlgItemMessage(hDlg, MIRROR_NODEL_LIST, LB_DELETESTRING, Num, 0);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- 操作設定ウインドウのコールバック --------------------------------------
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
//static BOOL CALLBACK NotifySettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK NotifySettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;

	static const RADIOBUTTON DownButton[] = {
		{ NOTIFY_D_DLG, TRANS_DLG },
		{ NOTIFY_D_OVW, TRANS_OVW }
	};
	#define DOWNBUTTONS	(sizeof(DownButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON UpButton[] = {
		{ NOTIFY_U_DLG, TRANS_DLG },
		{ NOTIFY_U_OVW, TRANS_OVW }
	};
	#define UPBUTTONS	(sizeof(UpButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON DclickButton[] = {
		{ NOTIFY_OPEN,     YES },
		{ NOTIFY_DOWNLOAD, NO }
	};
	#define DCLICKBUTTONS	(sizeof(DclickButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON MoveButton[] = {
		{ NOTIFY_M_NODLG,   MOVE_NODLG },
		{ NOTIFY_M_DLG,     MOVE_DLG },
		{ NOTIFY_M_DISABLE, MOVE_DISABLE }
	};
	#define MOVEBUTTONS	(sizeof(MoveButton)/sizeof(RADIOBUTTON))

	switch (message)
	{
		case WM_INITDIALOG :
			SetRadioButtonByValue(hDlg, RecvMode,   DownButton,   DOWNBUTTONS);
			SetRadioButtonByValue(hDlg, SendMode,   UpButton,     UPBUTTONS);
			SetRadioButtonByValue(hDlg, DclickOpen, DclickButton, DCLICKBUTTONS);
			SetRadioButtonByValue(hDlg, MoveMode,   MoveButton,   MOVEBUTTONS);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					RecvMode   = AskRadioButtonValue(hDlg, DownButton,   DOWNBUTTONS);
					SendMode   = AskRadioButtonValue(hDlg, UpButton,     UPBUTTONS);
					DclickOpen = AskRadioButtonValue(hDlg, DclickButton, DCLICKBUTTONS);
					MoveMode   = AskRadioButtonValue(hDlg, MoveButton,   MOVEBUTTONS);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000046);
					break;
			}
			break;
	}
    return(FALSE);
}


/*----- 表示設定ウインドウのコールバック --------------------------------------
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
//static BOOL CALLBACK DispSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK DispSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	static LOGFONT TmpFont;

	switch (message)
	{
		case WM_INITDIALOG :
			memcpy(&TmpFont, &ListLogFont, sizeof(LOGFONT));
			if(ListFont != NULL)
				SendDlgItemMessage(hDlg, DISP_FONT, WM_SETTEXT, 0, (LPARAM)TmpFont.lfFaceName);
			SendDlgItemMessage(hDlg, DISP_HIDE, BM_SETCHECK, DispIgnoreHide, 0);
			SendDlgItemMessage(hDlg, DISP_DRIVE, BM_SETCHECK, DispDrives, 0);
			// ファイルアイコン表示対応
			SendDlgItemMessage(hDlg, DISP_ICON, BM_SETCHECK, DispFileIcon, 0);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					DispIgnoreHide = SendDlgItemMessage(hDlg, DISP_HIDE, BM_GETCHECK, 0, 0);
					DispDrives = SendDlgItemMessage(hDlg, DISP_DRIVE, BM_GETCHECK, 0, 0);
					// ファイルアイコン表示対応
					DispFileIcon = SendDlgItemMessage(hDlg, DISP_ICON, BM_GETCHECK, 0, 0);
					if(strlen(TmpFont.lfFaceName) > 0)
					{
						memcpy(&ListLogFont, &TmpFont, sizeof(LOGFONT));
						ListFont = CreateFontIndirect(&ListLogFont);
					}
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000047);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case DISP_FONT_BR :
					if(SelectListFont(hDlg, &TmpFont) == YES)
						SendDlgItemMessage(hDlg, DISP_FONT, WM_SETTEXT, 0, (LPARAM)TmpFont.lfFaceName);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- フォントを選ぶ --------------------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		LOGFONT *lFont : フォント情報
*
*	Return Value
*		なし
*
*	Parameter change
*		HFONT *hFont : フォントのハンドル
*		LOGFONT *lFont : フォント情報
*----------------------------------------------------------------------------*/

static int SelectListFont(HWND hWnd, LOGFONT *lFont)
{
	static CHOOSEFONT cFont;
	int Sts;

	cFont.lStructSize = sizeof(CHOOSEFONT);
	cFont.hwndOwner = hWnd;
	cFont.hDC = 0;
	cFont.lpLogFont = lFont;
	cFont.Flags = CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
	cFont.nFontType = SCREEN_FONTTYPE;

	Sts = NO;
	if(ChooseFont(&cFont) == TRUE)
		Sts = YES;

	return(Sts);
}


/*----- 接続／切断設定ウインドウのコールバック --------------------------------
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
//static BOOL CALLBACK ConnectSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK ConnectSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, CONNECT_CONNECT, BM_SETCHECK, ConnectOnStart, 0);
			SendDlgItemMessage(hDlg, CONNECT_OLDDLG, BM_SETCHECK, ConnectAndSet, 0);
			SendDlgItemMessage(hDlg, CONNECT_RASCLOSE, BM_SETCHECK, RasClose, 0);
			if(AskRasUsable() == NO)
				EnableWindow(GetDlgItem(hDlg, CONNECT_RASCLOSE), FALSE);
			SendDlgItemMessage(hDlg, CONNECT_CLOSE_NOTIFY, BM_SETCHECK, RasCloseNotify, 0);
			if((RasClose == NO) || (AskRasUsable() == NO))
				EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), FALSE);
			SendDlgItemMessage(hDlg, CONNECT_HIST, EM_LIMITTEXT, (WPARAM)2, 0);
			SetDecimalText(hDlg, CONNECT_HIST, FileHist);
			SendDlgItemMessage(hDlg, CONNECT_HIST_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(HISTORY_MAX, 0));
			SendDlgItemMessage(hDlg, CONNECT_QUICK_ANONY, BM_SETCHECK, QuickAnonymous, 0);
			SendDlgItemMessage(hDlg, CONNECT_HIST_PASS, BM_SETCHECK, PassToHist, 0);
			SendDlgItemMessage(hDlg, CONNECT_SENDQUIT, BM_SETCHECK, SendQuit, 0);
			SendDlgItemMessage(hDlg, CONNECT_NORAS, BM_SETCHECK, NoRasControl, 0);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					ConnectOnStart = SendDlgItemMessage(hDlg, MISC_CONNECT, BM_GETCHECK, 0, 0);
					ConnectAndSet = SendDlgItemMessage(hDlg, MISC_OLDDLG, BM_GETCHECK, 0, 0);
					RasClose = SendDlgItemMessage(hDlg, CONNECT_RASCLOSE, BM_GETCHECK, 0, 0);
					RasCloseNotify = SendDlgItemMessage(hDlg, CONNECT_CLOSE_NOTIFY, BM_GETCHECK, 0, 0);
					FileHist = GetDecimalText(hDlg, CONNECT_HIST);
					CheckRange2(&FileHist, HISTORY_MAX, 0);
					QuickAnonymous = SendDlgItemMessage(hDlg, CONNECT_QUICK_ANONY, BM_GETCHECK, 0, 0);
					PassToHist = SendDlgItemMessage(hDlg, CONNECT_HIST_PASS, BM_GETCHECK, 0, 0);
					SendQuit = SendDlgItemMessage(hDlg, CONNECT_SENDQUIT, BM_GETCHECK, 0, 0);
					NoRasControl = SendDlgItemMessage(hDlg, CONNECT_NORAS, BM_GETCHECK, 0, 0);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000048);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case CONNECT_RASCLOSE :
					if(SendDlgItemMessage(hDlg, CONNECT_RASCLOSE, BM_GETCHECK, 0, 0) == 1)
						EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), TRUE);
					else
						EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), FALSE);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- FireWall設定ウインドウのコールバック ----------------------------------
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
//static BOOL CALLBACK FireSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK FireSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	char Tmp[10];
	int Num;
	static int Type;
	static const INTCONVTBL TypeTbl[] = {
		{ 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 8 }, 
		{ 4, 3 }, { 5, 4 }, { 6, 5 }, { 7, 6 }, 
		{ 8, 7 }, { 9, 9 }
	};

	static const int HideTbl[9][6] = {
		{ TRUE,  TRUE,  TRUE,  FALSE, TRUE,  FALSE },
		{ TRUE,  TRUE,  TRUE,  FALSE, FALSE, TRUE  },
		{ TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE },
		{ FALSE, FALSE, FALSE, FALSE, FALSE, TRUE  },
		{ FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE },
		{ TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE },
		{ FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE },
		{ TRUE,  TRUE,  FALSE, TRUE,  FALSE, FALSE },
		{ TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE  }
	};

	switch (message)
	{
		case WM_INITDIALOG :
			// プロセス保護
			ProtectAllEditControls(hDlg);
			Type = ConvertNum(FwallType, 1, TypeTbl, sizeof(TypeTbl)/sizeof(INTCONVTBL));
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN204);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN205);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN206);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN207);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN208);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN209);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN210);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN211);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN294);
			SendDlgItemMessage(hDlg, FIRE_TYPE, CB_SETCURSEL, Type-1, 0);

			SendDlgItemMessage(hDlg, FIRE_HOST, EM_LIMITTEXT, HOST_ADRS_LEN, 0);
			SendDlgItemMessage(hDlg, FIRE_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
			SendDlgItemMessage(hDlg, FIRE_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SendDlgItemMessage(hDlg, FIRE_PORT, EM_LIMITTEXT, 5, 0);
			SendDlgItemMessage(hDlg, FIRE_DELIMIT, EM_LIMITTEXT, 1, 0);

			SendDlgItemMessage(hDlg, FIRE_HOST, WM_SETTEXT, 0, (LPARAM)FwallHost);
			SendDlgItemMessage(hDlg, FIRE_USER, WM_SETTEXT, 0, (LPARAM)FwallUser);
			SendDlgItemMessage(hDlg, FIRE_PASS, WM_SETTEXT, 0, (LPARAM)FwallPass);
			sprintf(Tmp, "%d", FwallPort);
			SendDlgItemMessage(hDlg, FIRE_PORT, WM_SETTEXT, 0, (LPARAM)Tmp);
			sprintf(Tmp, "%c", FwallDelimiter);
			SendDlgItemMessage(hDlg, FIRE_DELIMIT, WM_SETTEXT, 0, (LPARAM)Tmp);

			SendDlgItemMessage(hDlg, FIRE_USEIT, BM_SETCHECK, FwallDefault, 0);
			SendDlgItemMessage(hDlg, FIRE_PASV, BM_SETCHECK, PasvDefault, 0);
			SendDlgItemMessage(hDlg, FIRE_RESOLV, BM_SETCHECK, FwallResolv, 0);
			SendDlgItemMessage(hDlg, FIRE_LOWER, BM_SETCHECK, FwallLower, 0);

			SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN212);
			SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN213);
			SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN214);
			SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN215);
			SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN216);
			SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_SETCURSEL, FwallSecurity, 0);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					Type = SendDlgItemMessage(hDlg, FIRE_TYPE, CB_GETCURSEL, 0, 0) + 1;
					FwallType = ConvertNum(Type, 0, TypeTbl, sizeof(TypeTbl)/sizeof(INTCONVTBL));
					SendDlgItemMessage(hDlg, FIRE_HOST, WM_GETTEXT, HOST_ADRS_LEN+1, (LPARAM)FwallHost);
					SendDlgItemMessage(hDlg, FIRE_USER, WM_GETTEXT, USER_NAME_LEN+1, (LPARAM)FwallUser);
					SendDlgItemMessage(hDlg, FIRE_PASS, WM_GETTEXT, PASSWORD_LEN, (LPARAM)FwallPass);
					SendDlgItemMessage(hDlg, FIRE_PORT, WM_GETTEXT, 5+1, (LPARAM)Tmp);
					FwallPort = atoi(Tmp);
					SendDlgItemMessage(hDlg, FIRE_DELIMIT, WM_GETTEXT, 5, (LPARAM)Tmp);
					FwallDelimiter = Tmp[0];
					FwallDefault = SendDlgItemMessage(hDlg, FIRE_USEIT, BM_GETCHECK, 0, 0);
					PasvDefault = SendDlgItemMessage(hDlg, FIRE_PASV, BM_GETCHECK, 0, 0);
					FwallResolv = SendDlgItemMessage(hDlg, FIRE_RESOLV, BM_GETCHECK, 0, 0);
					FwallLower = SendDlgItemMessage(hDlg, FIRE_LOWER, BM_GETCHECK, 0, 0);
					FwallSecurity = SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_GETCURSEL, 0, 0);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000049);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case FIRE_TYPE :
					Num = SendDlgItemMessage(hDlg, FIRE_TYPE, CB_GETCURSEL, 0, 0);
					EnableWindow(GetDlgItem(hDlg, FIRE_USER), HideTbl[Num][0]);
					EnableWindow(GetDlgItem(hDlg, FIRE_PASS), HideTbl[Num][1]);
					EnableWindow(GetDlgItem(hDlg, FIRE_SECURITY), HideTbl[Num][2]);
					EnableWindow(GetDlgItem(hDlg, FIRE_RESOLV), HideTbl[Num][3]);
					EnableWindow(GetDlgItem(hDlg, FIRE_LOWER), HideTbl[Num][4]);
					EnableWindow(GetDlgItem(hDlg, FIRE_DELIMIT), HideTbl[Num][5]);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ツール設定ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK ToolSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK ToolSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	char Tmp[FMAX_PATH+1];

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, TOOL_EDITOR1, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, TOOL_EDITOR2, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, TOOL_EDITOR3, EM_LIMITTEXT, FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, TOOL_EDITOR1, WM_SETTEXT, 0, (LPARAM)ViewerName[0]);
			SendDlgItemMessage(hDlg, TOOL_EDITOR2, WM_SETTEXT, 0, (LPARAM)ViewerName[1]);
			SendDlgItemMessage(hDlg, TOOL_EDITOR3, WM_SETTEXT, 0, (LPARAM)ViewerName[2]);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SendDlgItemMessage(hDlg, TOOL_EDITOR1, WM_GETTEXT, FMAX_PATH+1, (LPARAM)ViewerName[0]);
					SendDlgItemMessage(hDlg, TOOL_EDITOR2, WM_GETTEXT, FMAX_PATH+1, (LPARAM)ViewerName[1]);
					SendDlgItemMessage(hDlg, TOOL_EDITOR3, WM_GETTEXT, FMAX_PATH+1, (LPARAM)ViewerName[2]);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000050);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case TOOL_EDITOR1_BR :
				case TOOL_EDITOR2_BR :
				case TOOL_EDITOR3_BR :
					strcpy(Tmp, "");
					if(SelectFile(hDlg, Tmp, MSGJPN217, MSGJPN218, NULL, OFN_FILEMUSTEXIST, 0) == TRUE)
					{
						switch(GET_WM_COMMAND_ID(wParam, lParam))
						{
							case TOOL_EDITOR1_BR :
								SendDlgItemMessage(hDlg, TOOL_EDITOR1, WM_SETTEXT, 0, (LPARAM)Tmp);
								break;

							case TOOL_EDITOR2_BR :
								SendDlgItemMessage(hDlg, TOOL_EDITOR2, WM_SETTEXT, 0, (LPARAM)Tmp);
								break;

							case TOOL_EDITOR3_BR :
								SendDlgItemMessage(hDlg, TOOL_EDITOR3, WM_SETTEXT, 0, (LPARAM)Tmp);
								break;
						}
					}
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- サウンド設定ウインドウのコールバック ----------------------------------
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
//static BOOL CALLBACK SoundSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK SoundSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	char Tmp[FMAX_PATH+1];

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, SOUND_CONNECT, BM_SETCHECK, Sound[SND_CONNECT].On, 0);
			SendDlgItemMessage(hDlg, SOUND_TRANS, BM_SETCHECK, Sound[SND_TRANS].On, 0);
			SendDlgItemMessage(hDlg, SOUND_ERROR, BM_SETCHECK, Sound[SND_ERROR].On, 0);

			SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_SETTEXT, 0, (LPARAM)Sound[SND_CONNECT].Fname);
			SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_SETTEXT, 0, (LPARAM)Sound[SND_TRANS].Fname);
			SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_SETTEXT, 0, (LPARAM)Sound[SND_ERROR].Fname);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					Sound[SND_CONNECT].On = SendDlgItemMessage(hDlg, SOUND_CONNECT, BM_GETCHECK, 0, 0);
					Sound[SND_TRANS].On = SendDlgItemMessage(hDlg, SOUND_TRANS, BM_GETCHECK, 0, 0);
					Sound[SND_ERROR].On = SendDlgItemMessage(hDlg, SOUND_ERROR, BM_GETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Sound[SND_CONNECT].Fname);
					SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Sound[SND_TRANS].Fname);
					SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Sound[SND_ERROR].Fname);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000051);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case SOUND_CONNECT_BR :
				case SOUND_TRANS_BR :
				case SOUND_ERROR_BR :
					strcpy(Tmp, "");
					if(SelectFile(hDlg, Tmp, MSGJPN219, MSGJPN277, NULL, OFN_FILEMUSTEXIST, 0) == TRUE)
					{
						switch(GET_WM_COMMAND_ID(wParam, lParam))
						{
							case SOUND_CONNECT_BR :
								SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_SETTEXT, 0, (LPARAM)Tmp);
								break;

							case SOUND_TRANS_BR :
								SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_SETTEXT, 0, (LPARAM)Tmp);
								break;

							case SOUND_ERROR_BR :
								SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_SETTEXT, 0, (LPARAM)Tmp);
								break;
						}
					}
					break;

				case SOUND_CONNECT_TEST :
					SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Tmp);
					sndPlaySound(Tmp, SND_ASYNC | SND_NODEFAULT);
					break;

				case SOUND_TRANS_TEST :
					SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Tmp);
					sndPlaySound(Tmp, SND_ASYNC | SND_NODEFAULT);
					break;

				case SOUND_ERROR_TEST :
					SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_GETTEXT, FMAX_PATH+1, (LPARAM)Tmp);
					sndPlaySound(Tmp, SND_ASYNC | SND_NODEFAULT);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- その他の設定ウインドウのコールバック ----------------------------------
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
//static BOOL CALLBACK MiscSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK MiscSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NMHDR *pnmhdr;
	char Tmp[FMAX_PATH+1];

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, MISC_WINPOS, BM_SETCHECK, SaveWinPos, 0);
			SendDlgItemMessage(hDlg, MISC_DEBUG, BM_SETCHECK, DebugConsole, 0);
			SendDlgItemMessage(hDlg, MISC_REGTYPE, BM_SETCHECK, RegType, 0);
			// ポータブル版判定
			if(AskForceIni() == YES)
				EnableWindow(GetDlgItem(hDlg, MISC_REGTYPE), FALSE);

			SendDlgItemMessage(hDlg, MISC_CACHE_SAVE, BM_SETCHECK, CacheSave, 0);
			SendDlgItemMessage(hDlg, MISC_BUFNUM, EM_LIMITTEXT, (WPARAM)2, 0);
			SetDecimalText(hDlg, MISC_BUFNUM, abs(CacheEntry));
			SendDlgItemMessage(hDlg, MISC_BUFNUM_SPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(99, 1));
			if(CacheEntry > 0)
			{
				SendDlgItemMessage(hDlg, MISC_CACHE, BM_SETCHECK, 1, 0);
				EnableWindow(GetDlgItem(hDlg, MISC_BUFNUM), TRUE);
				EnableWindow(GetDlgItem(hDlg, MISC_CACHE_SAVE), TRUE);
			}
			else
			{
				SendDlgItemMessage(hDlg, MISC_CACHE, BM_SETCHECK, 0, 0);
				EnableWindow(GetDlgItem(hDlg, MISC_BUFNUM), FALSE);
				EnableWindow(GetDlgItem(hDlg, MISC_CACHE_SAVE), FALSE);
			}

			SendDlgItemMessage(hDlg, MISC_CACHEDIR, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
			SendDlgItemMessage(hDlg, MISC_CACHEDIR, WM_SETTEXT, 0, (LPARAM)TmpPath);
		    return(TRUE);

		case WM_NOTIFY:
			pnmhdr = (NMHDR FAR *)lParam;
			switch(pnmhdr->code)
			{
				case PSN_APPLY :
					SaveWinPos = SendDlgItemMessage(hDlg, MISC_WINPOS, BM_GETCHECK, 0, 0);
					DebugConsole = SendDlgItemMessage(hDlg, MISC_DEBUG, BM_GETCHECK, 0, 0);
					// ポータブル版判定
//					RegType = SendDlgItemMessage(hDlg, MISC_REGTYPE, BM_GETCHECK, 0, 0);
					if(AskForceIni() == NO)
						RegType = SendDlgItemMessage(hDlg, MISC_REGTYPE, BM_GETCHECK, 0, 0);

					CacheSave = SendDlgItemMessage(hDlg, MISC_CACHE_SAVE, BM_GETCHECK, 0, 0);
					CacheEntry = GetDecimalText(hDlg, MISC_BUFNUM);
					if(SendDlgItemMessage(hDlg, MISC_CACHE, BM_GETCHECK, 0, 0) == 0)
						CacheEntry = -CacheEntry;

					SendDlgItemMessage(hDlg, MISC_CACHEDIR, WM_GETTEXT, FMAX_PATH+1, (LPARAM)TmpPath);
					break;

				case PSN_RESET :
					break;

				case PSN_HELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000052);
					break;
			}
			break;

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case MISC_CACHE :
					if(SendDlgItemMessage(hDlg, MISC_CACHE, BM_GETCHECK, 0, 0) == 1)
					{
						EnableWindow(GetDlgItem(hDlg, TRMODE_EXT), TRUE);
						EnableWindow(GetDlgItem(hDlg, MISC_CACHE_SAVE), TRUE);
					}
					else
					{
						EnableWindow(GetDlgItem(hDlg, TRMODE_EXT), FALSE);
						EnableWindow(GetDlgItem(hDlg, MISC_CACHE_SAVE), FALSE);
					}
					break;

				case MISC_CACHEDIR_BR :
					if(SelectDir(hDlg, Tmp, FMAX_PATH) == TRUE)
						SendDlgItemMessage(hDlg, MISC_CACHEDIR, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;

				case MISC_CACHEDIR_DEF :
					// 環境依存の不具合対策
//					GetTempPath(FMAX_PATH, Tmp);
					GetAppTempPath(Tmp);
					SetYenTail(Tmp);
					SendDlgItemMessage(hDlg, MISC_CACHEDIR, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ソート設定ウインドウ --------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		int ステータス (YES=実行/NO=取消)
*----------------------------------------------------------------------------*/

int SortSetting(void)
{
	int Sts;

	Sts = DialogBox(GetFtpInst(), MAKEINTRESOURCE(sort_dlg), GetMainHwnd(), SortSettingProc);
	return(Sts);
}


/*----- ソート設定ウインドウのコールバック ------------------------------------
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
//static BOOL CALLBACK SortSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK SortSettingProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int LFsort;
	int LDsort;
	int RFsort;
	int RDsort;

	static const RADIOBUTTON LsortOrdButton[] = {
		{ SORT_LFILE_NAME, SORT_NAME },
		{ SORT_LFILE_EXT, SORT_EXT },
		{ SORT_LFILE_SIZE, SORT_SIZE },
		{ SORT_LFILE_DATE, SORT_DATE }
	};
	#define LSORTORDBUTTONS	(sizeof(LsortOrdButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON LDirsortOrdButton[] = {
		{ SORT_LDIR_NAME, SORT_NAME },
		{ SORT_LDIR_DATE, SORT_DATE }
	};
	#define LDIRSORTORDBUTTONS	(sizeof(LDirsortOrdButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON RsortOrdButton[] = {
		{ SORT_RFILE_NAME, SORT_NAME },
		{ SORT_RFILE_EXT, SORT_EXT },
		{ SORT_RFILE_SIZE, SORT_SIZE },
		{ SORT_RFILE_DATE, SORT_DATE }
	};
	#define RSORTORDBUTTONS	(sizeof(RsortOrdButton)/sizeof(RADIOBUTTON))

	static const RADIOBUTTON RDirsortOrdButton[] = {
		{ SORT_RDIR_NAME, SORT_NAME },
		{ SORT_RDIR_DATE, SORT_DATE }
	};
	#define RDIRSORTORDBUTTONS	(sizeof(RDirsortOrdButton)/sizeof(RADIOBUTTON))

	switch (message)
	{
		case WM_INITDIALOG :

			SetRadioButtonByValue(hDlg, AskSortType(ITEM_LFILE) & SORT_MASK_ORD, LsortOrdButton, LSORTORDBUTTONS);

			if((AskSortType(ITEM_LFILE) & SORT_GET_ORD) == SORT_ASCENT)
				SendDlgItemMessage(hDlg, SORT_LFILE_REV, BM_SETCHECK, 0, 0);
			else
				SendDlgItemMessage(hDlg, SORT_LFILE_REV, BM_SETCHECK, 1, 0);

			SetRadioButtonByValue(hDlg, AskSortType(ITEM_LDIR) & SORT_MASK_ORD, LDirsortOrdButton, LDIRSORTORDBUTTONS);

			if((AskSortType(ITEM_LDIR) & SORT_GET_ORD) == SORT_ASCENT)
				SendDlgItemMessage(hDlg, SORT_LDIR_REV, BM_SETCHECK, 0, 0);
			else
				SendDlgItemMessage(hDlg, SORT_LDIR_REV, BM_SETCHECK, 1, 0);

			SetRadioButtonByValue(hDlg, AskSortType(ITEM_RFILE) & SORT_MASK_ORD, RsortOrdButton, RSORTORDBUTTONS);

			if((AskSortType(ITEM_RFILE) & SORT_GET_ORD) == SORT_ASCENT)
				SendDlgItemMessage(hDlg, SORT_RFILE_REV, BM_SETCHECK, 0, 0);
			else
				SendDlgItemMessage(hDlg, SORT_RFILE_REV, BM_SETCHECK, 1, 0);

			SetRadioButtonByValue(hDlg, AskSortType(ITEM_RDIR) & SORT_MASK_ORD, RDirsortOrdButton, RDIRSORTORDBUTTONS);

			if((AskSortType(ITEM_RDIR) & SORT_GET_ORD) == SORT_ASCENT)
				SendDlgItemMessage(hDlg, SORT_RDIR_REV, BM_SETCHECK, 0, 0);
			else
				SendDlgItemMessage(hDlg, SORT_RDIR_REV, BM_SETCHECK, 1, 0);

			SendDlgItemMessage(hDlg, SORT_SAVEHOST, BM_SETCHECK, AskSaveSortToHost(), 0);

		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					LFsort = AskRadioButtonValue(hDlg, LsortOrdButton, LSORTORDBUTTONS);

					if(SendDlgItemMessage(hDlg, SORT_LFILE_REV, BM_GETCHECK, 0, 0) == 1)
						LFsort |= SORT_DESCENT;

					LDsort = AskRadioButtonValue(hDlg, LDirsortOrdButton, LDIRSORTORDBUTTONS);

					if(SendDlgItemMessage(hDlg, SORT_LDIR_REV, BM_GETCHECK, 0, 0) == 1)
						LDsort |= SORT_DESCENT;

					RFsort = AskRadioButtonValue(hDlg, RsortOrdButton, RSORTORDBUTTONS);

					if(SendDlgItemMessage(hDlg, SORT_RFILE_REV, BM_GETCHECK, 0, 0) == 1)
						RFsort |= SORT_DESCENT;

					RDsort = AskRadioButtonValue(hDlg, RDirsortOrdButton, RDIRSORTORDBUTTONS);

					if(SendDlgItemMessage(hDlg, SORT_RDIR_REV, BM_GETCHECK, 0, 0) == 1)
						RDsort |= SORT_DESCENT;

					SetSortTypeImm(LFsort, LDsort, RFsort, RDsort);

					SetSaveSortToHost(SendDlgItemMessage(hDlg, SORT_SAVEHOST, BM_GETCHECK, 0, 0));

					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;
	
				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000001);
					break;
		}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ダイアログのコントロールから１０進数を取得 ----------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		int Ctrl : コントロールのID
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static int GetDecimalText(HWND hDlg, int Ctrl)
int GetDecimalText(HWND hDlg, int Ctrl)
{
	char Tmp[40];

	SendDlgItemMessage(hDlg, Ctrl, WM_GETTEXT, (WPARAM)39, (LPARAM)Tmp);
	return(atoi(Tmp));
}


/*----- ダイアログのコントロールに１０進数をセット ----------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		int Ctrl : コントロールのID
*		int Num : 数値
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void SetDecimalText(HWND hDlg, int Ctrl, int Num)
void SetDecimalText(HWND hDlg, int Ctrl, int Num)
{
	char Tmp[40];

	sprintf(Tmp, "%d", Num);
	SendDlgItemMessage(hDlg, Ctrl, WM_SETTEXT, 0, (LPARAM)Tmp);
	return;
}


/*----- 設定値の範囲チェック --------------------------------------------------
*
*	Parameter
*		int *Cur : 設定値
*		int Max : 最大値
*		int Min : 最小値
*
*	Return Value
*		なし
*
*	Parameter change
*		int *Cur : 設定値
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void CheckRange2(int *Cur, int Max, int Min)
void CheckRange2(int *Cur, int Max, int Min)
{
	if(*Cur < Min)
		*Cur = Min;
	if(*Cur > Max)
		*Cur = Max;
	return;
}


/*----- 文字列をリストボックスに追加 ------------------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		char *Str : 文字列
*		int CtrlList : リストボックスのID
*		int BufSize : バッファサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize)
void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize)
{
	char Tmp[FMAX_PATH+1];
	int Num;
	int i;
	int Len;

	Len = strlen(Str);
	if(Len > 0)
	{
		Len++;
		Num = SendDlgItemMessage(hDlg, CtrlList, LB_GETCOUNT, 0, 0);
		for(i = 0; i < Num; i++)
		{
			SendDlgItemMessage(hDlg, CtrlList, LB_GETTEXT, i, (LPARAM)Tmp);
			Len += strlen(Tmp) + 1;
		}

		if(Len > (BufSize-1))
			MessageBeep(-1);
		else
			SendDlgItemMessage(hDlg, CtrlList, LB_ADDSTRING, 0, (LPARAM)Str);
	}
	return;
}


/*----- マルチ文字列をリストボックスにセット ----------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int CtrlList : リストボックスのID
*		char *Text : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text)
void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text)
{
	char *Pos;

	Pos = Text;
	while(*Pos != NUL)
	{
		SendDlgItemMessage(hDlg, CtrlList, LB_ADDSTRING, 0, (LPARAM)Pos);
		Pos += strlen(Pos) + 1;
	}
	return;
}


/*----- リストボックスの内容をマルチ文字列にする ------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int CtrlList : リストボックスのID
*		char *Buf : 文字列をセットするバッファ
*		int BufSize : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize)
void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize)
{
	char Tmp[FMAX_PATH+1];
	int Num;
	int i;

	memset(Buf, NUL, BufSize);
	Num = SendDlgItemMessage(hDlg, CtrlList, LB_GETCOUNT, 0, 0);
	for(i = 0; i < Num; i++)
	{
		SendDlgItemMessage(hDlg, CtrlList, LB_GETTEXT, i, (LPARAM)Tmp);
		strcpy(Buf + StrMultiLen(Buf), Tmp);
	}
	return;
}


