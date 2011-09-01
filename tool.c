/*=============================================================================
*
*									ツール
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


/*===== プロトタイプ =====*/

static BOOL CALLBACK OtpCalcWinProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/*===== 外部参照 =====*/

extern HWND hHelpWin;


/*----- ワンタイムパスワード計算 ----------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void OtpCalcTool(void)
{
	DialogBox(GetFtpInst(), MAKEINTRESOURCE(otp_calc_dlg), GetMainHwnd(), OtpCalcWinProc);
	return;
}


/*----- ワンタイムパスワード計算ウインドウのコールバック ----------------------
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

static BOOL CALLBACK OtpCalcWinProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Tmp[41];
	char *Pos;
	int Seq;
	int Type;
	char Seed[MAX_SEED_LEN+1];
	char Pass[PASSWORD_LEN+1];

	static const RADIOBUTTON AlgoButton[] = {
		{ OTPCALC_MD4, MD4 },
		{ OTPCALC_MD5, MD5 },
		{ OTPCALC_SHA1, SHA1 }
	};
	#define ALGOBUTTONS	(sizeof(AlgoButton)/sizeof(RADIOBUTTON))

	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, OTPCALC_KEY, EM_LIMITTEXT, 40, 0);
			SendDlgItemMessage(hDlg, OTPCALC_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			SetRadioButtonByValue(hDlg, MD4, AlgoButton, ALGOBUTTONS);
		    return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, OTPCALC_KEY, WM_GETTEXT, 41, (LPARAM)Tmp);
					SendDlgItemMessage(hDlg, OTPCALC_PASS, WM_GETTEXT, PASSWORD_LEN+1, (LPARAM)Pass);
					Type = AskRadioButtonValue(hDlg, AlgoButton, ALGOBUTTONS);

					Pos = Tmp;
					while(*Pos == ' ')
						Pos++;

					if(IsDigit(*Pos))
					{
						Seq = atoi(Pos);
						/* Seed */
						if((Pos = GetNextField(Pos)) != NULL)
						{
							if(GetOneField(Pos, Seed, MAX_SEED_LEN) == SUCCESS)
							{
								Make6WordPass(Seq, Seed, Pass, Type, Tmp);
							}
							else
								strcpy(Tmp, MSGJPN251);
						}
						else
							strcpy(Tmp, MSGJPN252);
					}
					else
						strcpy(Tmp, MSGJPN253);

					SendDlgItemMessage(hDlg, OTPCALC_RES, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000037);
					break;
		}
			return(TRUE);
	}
    return(FALSE);
}



