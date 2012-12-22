/*=============================================================================
*
*							その他の汎用サブルーチン
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mbstring.h>
// IPv6対応
//#include <winsock.h>
#include <winsock2.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <locale.h>

#include "common.h"
#include "resource.h"

#include <htmlhelp.h>
#include "helpid.h"

// UTF-8対応
#undef __MBSWRAPPER_H__
#include "mbswrapper.h"



/*===== 入力ダイアログデータのストラクチャ =====*/

typedef struct {
	char Title[80];			/* ダイアログのタイトル */
	char Str[FMAX_PATH+1];	/* デフォルト文字列／入力された文字列(Output) */
	int MaxLen;				/* 文字列の最長 */
	int Anonymous;			/* Anonymousフラグ(Output) */
} DIALOGDATA;

/*===== プロトタイプ =====*/

// 64ビット対応
//static BOOL CALLBACK InputDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK InputDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

/*===== 外部参照 =====*/

extern HWND hHelpWin;

/*===== ローカルなワーク =====*/

static DIALOGDATA *DialogData;		/* 入力ダイアログデータ */
static int HelpPage;



/*----- 入力ダイアログを表示 --------------------------------------------------
*
*	Parameter
*		int Res : ダイアログボックスのID
*		HWND hWnd : 親ウインドウのウインドウハンドル
*		char *Title : ウインドウタイトル (NULL=設定しない)
*		char *Buf : エディットボックスの初期文字列／入力文字列を返すバッファ
*		int Max : バッファのサイズ (FMAX_PATH+1以下であること)
*		int *Flg : フラグの初期値／フラグを返すワーク
*		int Help : ヘルプのコンテキスト番号
*
*	Return Value
*		int ステータス (YES/NO=取り消し)
*
*	Note
*		ダイアログは１個のEditBoxと１個のButtonを持つものを使う
*----------------------------------------------------------------------------*/

int InputDialogBox(int Res, HWND hWnd, char *Title, char *Buf, int Max, int *Flg, int Help)
{
	int Ret;
	DIALOGDATA dData;

	dData.MaxLen = Max;
	memset(dData.Str, NUL, FMAX_PATH+1);
	strncpy(dData.Str, Buf, FMAX_PATH);
	strcpy(dData.Title, "");
	if(Title != NULL)
		strcpy(dData.Title, Title);
	dData.Anonymous = *Flg;
	DialogData = &dData;
	HelpPage = Help;

	Ret = DialogBox(GetFtpInst(), MAKEINTRESOURCE(Res), hWnd, InputDialogCallBack);

	if(Ret == YES)
	{
		memset(Buf, NUL, Max);
		strncpy(Buf, dData.Str, Max-1);
		*Flg = dData.Anonymous;
	}
	return(Ret);
}


/*----- 入力ダイアログのコールバック ------------------------------------------
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
//static BOOL CALLBACK InputDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
static INT_PTR CALLBACK InputDialogCallBack(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	char Tmp[FMAX_PATH+1];

	switch (iMessage)
	{
		case WM_INITDIALOG :
			// プロセス保護
			ProtectAllEditControls(hDlg);
			if(strlen(DialogData->Title) != 0)
				SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)DialogData->Title);
			SendDlgItemMessage(hDlg, INP_INPSTR, EM_LIMITTEXT, DialogData->MaxLen-1, 0);
			SendDlgItemMessage(hDlg, INP_INPSTR, WM_SETTEXT, 0, (LPARAM)DialogData->Str);
			SendDlgItemMessage(hDlg, INP_ANONYMOUS, BM_SETCHECK, DialogData->Anonymous, 0);
			return(TRUE);

		case WM_COMMAND :
			switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDOK :
					SendDlgItemMessage(hDlg, INP_INPSTR, WM_GETTEXT, DialogData->MaxLen, (LPARAM)DialogData->Str);
					DialogData->Anonymous = SendDlgItemMessage(hDlg, INP_ANONYMOUS, BM_GETCHECK, 0, 0);
					EndDialog(hDlg, YES);
					break;

				case IDCANCEL :
					EndDialog(hDlg, NO);
					break;

				case IDHELP :
					hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, HelpPage);
					break;

				case INP_BROWSE :
					if(SelectDir(hDlg, Tmp, FMAX_PATH) == TRUE)
						SendDlgItemMessage(hDlg, INP_INPSTR, WM_SETTEXT, 0, (LPARAM)Tmp);
					break;
			}
            return(TRUE);
	}
	return(FALSE);
}


/*----- ［実行］と［取消］だけのダイアログの共通コールバック関数 --------------
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
//BOOL CALLBACK ExeEscDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
INT_PTR CALLBACK ExeEscDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
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
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- ［実行］と［取消］だけのダイアログの共通コールバック関数(テキスト表示つき)
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
//BOOL CALLBACK ExeEscTextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
INT_PTR CALLBACK ExeEscTextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG :
			SendDlgItemMessage(hDlg, COMMON_TEXT, WM_SETTEXT, 0, lParam);
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
			}
			return(TRUE);
	}
    return(FALSE);
}


/*----- 文字列の最後に "\" を付ける -------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*
*	Note
*		オリジナルの文字列 char *Str が変更されます。
*----------------------------------------------------------------------------*/

void SetYenTail(char *Str)
{
	if(_mbscmp(_mbsninc(Str, _mbslen(Str) - 1), "\\") != 0)
		strcat(Str, "\\");

	return;;
}


/*----- 文字列の最後の "\" を取り除く -----------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*
*	Note
*		オリジナルの文字列 char *Str が変更されます。
*----------------------------------------------------------------------------*/

void RemoveYenTail(char *Str)
{
	char *Pos;

	if(strlen(Str) > 0)
	{
		Pos = _mbsninc(Str, _mbslen(Str) - 1);
		if(_mbscmp(Pos, "\\") == 0)
			*Pos = NUL;
	}
	return;;
}


/*----- 文字列の最後に "/" を付ける -------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*
*	Note
*		オリジナルの文字列 char *Str が変更されます。
*----------------------------------------------------------------------------*/

void SetSlashTail(char *Str)
{
#if defined(HAVE_TANDEM)
    /* Tandem では / の代わりに . を追加 */
	if(AskHostType() == HTYPE_TANDEM) {
		if(_mbscmp(_mbsninc(Str, _mbslen(Str) - 1), ".") != 0)
			strcat(Str, ".");
	} else
#endif
	if(_mbscmp(_mbsninc(Str, _mbslen(Str) - 1), "/") != 0)
		strcat(Str, "/");

	return;
}


/*----- 文字列から改行コードを取り除く ----------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*
*	Note
*		オリジナルの文字列 char *Str が変更されます。
*----------------------------------------------------------------------------*/

void RemoveReturnCode(char *Str)
{
	char *Pos;

	if((Pos = strchr(Str, 0x0D)) != NULL)
		*Pos = NUL;
	if((Pos = strchr(Str, 0x0A)) != NULL)
		*Pos = NUL;
	return;
}


/*----- 文字列内の特定の文字を全て置き換える ----------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char Src : 検索文字
*		char Dst : 置換文字
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ReplaceAll(char *Str, char Src, char Dst)
{
	char *Pos;

/* Tandem ではノード名の変換を行わない */
/* 最初の1文字が \ でもそのままにする */
#if defined(HAVE_TANDEM)
	if (AskRealHostType() == HTYPE_TANDEM && strlen(Str) > 0)
		Str++;
#endif
	while((Pos = _mbschr(Str, Src)) != NULL)
		*Pos = Dst;
	return;
}


/*----- 数字もしくは特定の１文字かチェック ------------------------------------
*
*	Parameter
*		int Ch : チェックする文字
*		int Sym : 記号
*
*	Return Value
*		int ステータス
*			0=数字でも特定の記号でもない
*----------------------------------------------------------------------------*/

int IsDigitSym(int Ch, int Sym)
{
	int Ret;

	if((Ret = IsDigit(Ch)) == 0)
	{
		if((Sym != NUL) && (Sym == Ch))
			Ret = 1;
	}
	return(Ret);
}


/*----- 文字列が全て同じ文字かチェック ----------------------------------------
*
*	Parameter
*		char *Str : 文字列
*		int Ch : 文字
*
*	Return Value
*		int ステータス
*			YES/NO
*----------------------------------------------------------------------------*/

int StrAllSameChar(char *Str, char Ch)
{
	int Ret;

	Ret = YES;
	while(*Str != NUL)
	{
		if(*Str != Ch)
		{
			Ret = NO;
			break;
		}
		Str++;
	}
	return(Ret);
}


/*----- 文字列の末尾のスペースを削除 ------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RemoveTailingSpaces(char *Str)
{
	char *Pos;

	Pos = Str + strlen(Str);
	while(--Pos > Str)
	{
		if(*Pos != ' ')
			break;
		*Pos = NUL;
	}
	return;
}


/*----- 大文字／小文字を区別しないstrstr --------------------------------------
*
*	Parameter
*		char *s1 : 文字列１
*		char *s2 : 文字列２
*
*	Return Value
*		char *文字列１中で文字列２が見つかった位置
*			NULL=見つからなかった
*----------------------------------------------------------------------------*/

char *stristr(char *s1, char *s2)
{
	char *Ret;

	Ret = NULL;
	while(*s1 != NUL)
	{
		if((tolower(*s1) == tolower(*s2)) &&
		   (_strnicmp(s1, s2, strlen(s2)) == 0))
		{
			Ret = s1;
			break;
		}
		s1++;
	}
	return(Ret);
}


/*----- 文字列中のスペースで区切られた次のフィールドを返す --------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		char *次のフィールド
*			NULL=見つからなかった
*----------------------------------------------------------------------------*/

char *GetNextField(char *Str)
{
	if((Str = strchr(Str, ' ')) != NULL)
	{
		while(*Str == ' ')
		{
			if(*Str == NUL)
			{
				Str = NULL;
				break;
			}
			Str++;
		}
	}
	return(Str);
}


/*----- 現在のフィールドの文字列をコピーする ----------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char *Buf : コピー先
*		int Max : 最大文字数
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL=長さが長すぎる
*----------------------------------------------------------------------------*/

int GetOneField(char *Str, char *Buf, int Max)
{
	int Sts;
	char *Pos;

	Sts = FFFTP_FAIL;
	if((Pos = strchr(Str, ' ')) == NULL)
	{
		if((int)strlen(Str) <= Max)
		{
			strcpy(Buf, Str);
			Sts = FFFTP_SUCCESS;
		}
	}
	else
	{
		if(Pos - Str <= Max)
		{
			strncpy(Buf, Str, Pos - Str);
			*(Buf + (Pos - Str)) = NUL;
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- カンマを取り除く ------------------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RemoveComma(char *Str)
{
	char *Put;

	Put = Str;
	while(*Str != NUL)
	{
		if(*Str != ',')
		{
			*Put = *Str;
			Put++;
		}
		Str++;
	}
	*Put = NUL;
	return;
}


/*----- パス名の中のファイル名の先頭を返す ------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		char *ファイル名の先頭
*
*	Note
*		ディレクトリの区切り記号は "\" と "/" の両方が有効
*----------------------------------------------------------------------------*/

char *GetFileName(char *Path)
{
	char *Pos;

	if((Pos = _mbschr(Path, ':')) != NULL)
		Path = Pos + 1;

	if((Pos = _mbsrchr(Path, '\\')) != NULL)
		Path = Pos + 1;

	if((Pos = _mbsrchr(Path, '/')) != NULL)
		Path = Pos + 1;

#if defined(HAVE_TANDEM)
	/* Tandem は . がデリミッタとなる */
	if((AskHostType() == HTYPE_TANDEM) && ((Pos = _mbsrchr(Path, '.')) != NULL))
		Path = Pos + 1;
#endif
	return(Path);
}


/*----- ツールの表示名を返す --------------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		char * : 表示名
*----------------------------------------------------------------------------*/

char *GetToolName(char *Path)
{
	char *Pos;

	if((Pos = _mbschr(Path, ':')) != NULL)
		Path = Pos + 1;

	if((Pos = _mbsrchr(Path, '\\')) != NULL)
		Path = Pos + 1;

	return(Path);
}


/*----- パス名の中の拡張子の先頭を返す ----------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		char *拡張子の先頭
*----------------------------------------------------------------------------*/

char *GetFileExt(char *Path)
{
	char *Ret;

	Ret = _mbschr(Path, NUL);
	if((_mbscmp(Path, ".") != 0) &&
	   (_mbscmp(Path, "..") != 0))
	{
		while((Path = _mbschr(Path, '.')) != NULL)
		{
			Path++;
			Ret = Path;
		}
	}
	return(Ret);
}


/*----- パス名からファイル名を取り除く ----------------------------------------
*
*	Parameter
*		char *Path : パス名
*		char *Buf : ファイル名を除いたパス名のコピー先
*
*	Return Value
*		なし
*
*	Note
*		ディレクトリの区切り記号は "\" と "/" の両方が有効
*----------------------------------------------------------------------------*/

void RemoveFileName(char *Path, char *Buf)
{
	char *Pos;

	strcpy(Buf, Path);

	if((Pos = _mbsrchr(Buf, '/')) != NULL)
		*Pos = NUL;
	else if((Pos = _mbsrchr(Buf, '\\')) != NULL)
	{
		if((Pos == Buf) || 
		   ((Pos != Buf) && (*(Pos - 1) != ':')))
			*Pos = NUL;
	}
	return;
}


/*----- 上位ディレクトリのパス名を取得 ----------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*
*	Note
*		ディレクトリの区切り記号は "\" と "/" の両方が有効
*		最初の "\"や"/"は残す
*			例） "/pub"   --> "/"
*			例） "C:\DOS" --> "C:\"
*----------------------------------------------------------------------------*/

void GetUpperDir(char *Path)
{
	char *Top;
	char *Pos;

	if(((Top = _mbschr(Path, '/')) != NULL) ||
	   ((Top = _mbschr(Path, '\\')) != NULL))
	{
		Top++;
		if(((Pos = _mbsrchr(Top, '/')) != NULL) ||
		   ((Pos = _mbsrchr(Top, '\\')) != NULL))
			*Pos = NUL;
		else
			*Top = NUL;
	}
	return;
}


/*----- 上位ディレクトリのパス名を取得 ----------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*
*	Note
*		ディレクトリの区切り記号は "\" と "/" の両方が有効
*		最初の "\"や"/"も消す
*			例） "/pub"   --> ""
*			例） "C:\DOS" --> "C:"
*----------------------------------------------------------------------------*/

void GetUpperDirEraseTopSlash(char *Path)
{
	char *Pos;

	if(((Pos = _mbsrchr(Path, '/')) != NULL) ||
	   ((Pos = _mbsrchr(Path, '\\')) != NULL))
		*Pos = NUL;
	else
		*Path = NUL;

	return;
}


/*----- ディレクトリの階層数を返す --------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*
*	Note
*		単に '\' と '/'の数を返すだけ
*----------------------------------------------------------------------------*/

int AskDirLevel(char *Path)
{
	char *Pos;
	int Level;

	Level = 0;
	while(((Pos = _mbschr(Path, '/')) != NULL) ||
		  ((Pos = _mbschr(Path, '\\')) != NULL))
	{
		Path = Pos + 1;
		Level++;
	}
	return(Level);
}


/*----- ファイルサイズを文字列に変換する --------------------------------------
*
*	Parameter
*		double Size : ファイルサイズ
*		char *Buf : 文字列を返すバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void MakeSizeString(double Size, char *Buf)
{
	if(Size >= (1024*1024))
	{
		Size /= (1024*1024);
		sprintf(Buf, "%.2fM Bytes", Size);
	}
	else if (Size >= 1024)
	{
		Size /= 1024;
		sprintf(Buf, "%.2fK Bytes", Size);
	}
	else
		sprintf(Buf, "%.0f Bytes", Size);

	return;
}


/*----- StaticTextの領域に収まるようにパス名を整形して表示 --------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		char *Str : 文字列 (長さはFMAX_PATH以下)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispStaticText(HWND hWnd, char *Str)
{
	char Buf[FMAX_PATH+1];
	char *Pos;
	char *Tmp;
	RECT Rect;
	SIZE fSize;
	HDC hDC;
	int Force;

	GetClientRect(hWnd, &Rect);
	Rect.right -= Rect.left;

	hDC = GetDC(hWnd);
	strcpy(Buf, Str);
	Pos = Buf;
	Force = NO;
	while(Force == NO)
	{
		GetTextExtentPoint32(hDC, Pos, strlen(Pos), &fSize);

		if(fSize.cx <= Rect.right)
			break;

		if(_mbslen(Pos) <= 4)
			Force = YES;
		else
		{
			Pos = _mbsninc(Pos, 4);
			if((Tmp = _mbschr(Pos, '\\')) == NULL)
				Tmp = _mbschr(Pos, '/');

			if(Tmp == NULL)
				Tmp = _mbsninc(Pos, 4);

			Pos = Tmp - 3;
			memset(Pos, '.', 3);
		}
	}
	ReleaseDC(hWnd, hDC);

	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)Pos);
	return;
}


/*----- 文字列アレイの長さを求める --------------------------------------------
*
*	Parameter
*		char *Str : 文字列アレイ (末尾はNUL２つ)
*
*	Return Value
*		int 長さ
*
*	Note
*		終端の2つのNULのうちの最後の物は数えない
*			StrMultiLen("") = 0
*			StrMultiLen("abc\0xyz\0") = 8
*			StrMultiLen("abc") = 終端が２つのNULでないので求められない
*----------------------------------------------------------------------------*/

int StrMultiLen(char *Str)
{
	int Len;
	int Tmp;

	Len = 0;
	while(*Str != NUL)
	{
		Tmp = strlen(Str) + 1;
		Str += Tmp;
		Len += Tmp;
	}
	return(Len);
}


/*----- RECTをクライアント座標からスクリーン座標に変換 ------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		RECT *Rect : RECT
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RectClientToScreen(HWND hWnd, RECT *Rect)
{
	POINT Tmp;

	Tmp.x = Rect->left;
	Tmp.y = Rect->top;
	ClientToScreen(hWnd, &Tmp);
	Rect->left = Tmp.x;
	Rect->top = Tmp.y;

	Tmp.x = Rect->right;
	Tmp.y = Rect->bottom;
	ClientToScreen(hWnd, &Tmp);
	Rect->right = Tmp.x;
	Rect->bottom = Tmp.y;

	return;
}


/*----- 16進文字をバイナリに変換 ----------------------------------------------
*
*	Parameter
*		char Ch : 16進文字
*
*	Return Value
*		int バイナリ値
*----------------------------------------------------------------------------*/

int hex2bin(char Ch)
{
	int Ret;

	if((Ch >= '0') && (Ch <= '9'))
		Ret = Ch - '0';
	else if((Ch >= 'A') && (Ch <= 'F'))
		Ret = Ch - 'A' + 10;
	else if((Ch >= 'a') && (Ch <= 'f'))
		Ret = Ch - 'a' + 10;

	return(Ret);
}


/*----- ＵＮＣ文字列を分解する ------------------------------------------------
*
*	Parameter
*		char *unc : UNC文字列
*		char *Host : ホスト名をコピーするバッファ (サイズは HOST_ADRS_LEN+1)
*		char *Path : パス名をコピーするバッファ (サイズは FMAX_PATH+1)
*		char *File : ファイル名をコピーするバッファ (サイズは FMAX_PATH+1)
*		char *User : ユーザ名をコピーするバッファ (サイズは USER_NAME_LEN+1)
*		char *Pass : パスワードをコピーするバッファ (サイズは PASSWORD_LEN+1)
*		int *Port : ポート番号をコピーするバッファ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*
*	"\"は全て"/"に置き換える
*----------------------------------------------------------------------------*/

int SplitUNCpath(char *unc, char *Host, char *Path, char *File, char *User, char *Pass, int *Port)
{
	int Sts;
	char *Pos1;
	char *Pos2;
	char Tmp[FMAX_PATH+1];

	memset(Host, NUL, HOST_ADRS_LEN+1);
	memset(Path, NUL, FMAX_PATH+1);
	memset(File, NUL, FMAX_PATH+1);
	memset(User, NUL, USER_NAME_LEN+1);
	memset(Pass, NUL, PASSWORD_LEN+1);
	*Port = PORT_NOR;

	ReplaceAll(unc, '\\', '/');

	if((Pos1 = _mbsstr(unc, "//")) != NULL)
		Pos1 += 2;
	else
		Pos1 = unc;

	if((Pos2 = _mbschr(Pos1, '@')) != NULL)
	{
		memset(Tmp, NUL, FMAX_PATH+1);
		memcpy(Tmp, Pos1, Pos2-Pos1);
		Pos1 = Pos2 + 1;

		if((Pos2 = _mbschr(Tmp, ':')) != NULL)
		{
			memcpy(User, Tmp, min1(Pos2-Tmp, USER_NAME_LEN));
			strncpy(Pass, Pos2+1, PASSWORD_LEN);
		}
		else
			strncpy(User, Tmp, USER_NAME_LEN);
	}

	// IPv6対応
	if((Pos2 = _mbschr(Pos1, '[')) != NULL && Pos2 < _mbschr(Pos1, ':'))
	{
		Pos1 = Pos2 + 1;
		if((Pos2 = _mbschr(Pos2, ']')) != NULL)
		{
			memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
			Pos1 = Pos2 + 1;
		}
	}

	if((Pos2 = _mbschr(Pos1, ':')) != NULL)
	{
		// IPv6対応
//		memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
		if(strlen(Host) == 0)
			memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
		Pos2++;
		if(IsDigit(*Pos2))
		{
			*Port = atoi(Pos2);
			while(*Pos2 != NUL)
			{
				if(IsDigit(*Pos2) == 0)
					break;
				Pos2++;
			}
		}
		RemoveFileName(Pos2, Path);
		strncpy(File, GetFileName(Pos2), FMAX_PATH);
	}
	else if((Pos2 = _mbschr(Pos1, '/')) != NULL)
	{
		// IPv6対応
//		memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
		if(strlen(Host) == 0)
			memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
		RemoveFileName(Pos2, Path);
		strncpy(File, GetFileName(Pos2), FMAX_PATH);
	}
	else
	{
		// IPv6対応
//		strncpy(Host, Pos1, HOST_ADRS_LEN);
		if(strlen(Host) == 0)
			strncpy(Host, Pos1, HOST_ADRS_LEN);
	}

	Sts = FFFTP_FAIL;
	if(strlen(Host) > 0)
		Sts = FFFTP_SUCCESS;

	return(Sts);
}


/*----- 日付文字列(JST)をFILETIME(UTC)に変換 ----------------------------------
*
*	Parameter
*		char *Time : 日付文字列 ("yyyy/mm/dd hh:mm")
*		FILETIME *Buf : ファイルタイムを返すワーク
*
*	Return Value
*		int ステータス
*			YES/NO=日付情報がなかった
*----------------------------------------------------------------------------*/

int TimeString2FileTime(char *Time, FILETIME *Buf)
{
	SYSTEMTIME sTime;
	FILETIME fTime;
	int Ret;

	Ret = NO;
    Buf->dwLowDateTime = 0;
    Buf->dwHighDateTime = 0;

	if(strlen(Time) >= 16)
	{
		if(IsDigit(Time[0]) && IsDigit(Time[5]) && IsDigit(Time[8]) && 
		   IsDigit(Time[12]) && IsDigit(Time[14]))
		{
			Ret = YES;
		}

		sTime.wYear = atoi(Time);
		sTime.wMonth = atoi(Time + 5);
		sTime.wDay = atoi(Time + 8);
		if(Time[11] != ' ')
			sTime.wHour = atoi(Time + 11);
		else
			sTime.wHour = atoi(Time + 12);
		sTime.wMinute = atoi(Time + 14);
		// タイムスタンプのバグ修正
//		sTime.wSecond = 0;
		if(strlen(Time) >= 19)
			sTime.wSecond = atoi(Time + 17);
		else
			sTime.wSecond = 0;
		sTime.wMilliseconds = 0;

		SystemTimeToFileTime(&sTime, &fTime);
		LocalFileTimeToFileTime(&fTime, Buf);
	}
	return(Ret);
}


/*----- FILETIME(UTC)を日付文字列(JST)に変換 ----------------------------------
*
*	Parameter
*		FILETIME *Time : ファイルタイム
*		char *Buf : 日付文字列を返すワーク
*		int Mode : モード (DISPFORM_xxx)
*		int InfoExist : 情報があるかどうか (FINFO_xxx)
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// タイムスタンプのバグ修正
//void FileTime2TimeString(FILETIME *Time, char *Buf, int Mode, int InfoExist)
void FileTime2TimeString(FILETIME *Time, char *Buf, int Mode, int InfoExist, int ShowSeconds)
{
	SYSTEMTIME sTime;
	FILETIME fTime;

	if(Mode == DISPFORM_LEGACY)
	{
		if((Time->dwLowDateTime == 0) && (Time->dwHighDateTime == 0))
			InfoExist = 0;

		// タイムスタンプのバグ修正
//		/* "yyyy/mm/dd hh:mm" */
		/* "yyyy/mm/dd hh:mm:ss" */
		FileTimeToLocalFileTime(Time, &fTime);
		FileTimeToSystemTime(&fTime, &sTime);

		// タイムスタンプのバグ修正
//		if(InfoExist & FINFO_DATE)
//			sprintf(Buf, "%04d/%02d/%02d ", sTime.wYear, sTime.wMonth, sTime.wDay);
//		else
//			sprintf(Buf, "           ");
//
//		if(InfoExist & FINFO_TIME)
//			sprintf(Buf+11, "%2d:%02d", sTime.wHour, sTime.wMinute);
//		else
//			sprintf(Buf+11, "     ");
		if(InfoExist & (FINFO_DATE | FINFO_TIME))
		{
			if(InfoExist & FINFO_DATE)
				sprintf(Buf, "%04d/%02d/%02d ", sTime.wYear, sTime.wMonth, sTime.wDay);
			else
				sprintf(Buf, "           ");
			if(ShowSeconds == YES)
			{
				if(InfoExist & FINFO_TIME)
					sprintf(Buf+11, "%2d:%02d:%02d", sTime.wHour, sTime.wMinute, sTime.wSecond);
				else
					sprintf(Buf+11, "        ");
			}
			else
			{
				if(InfoExist & FINFO_TIME)
					sprintf(Buf+11, "%2d:%02d", sTime.wHour, sTime.wMinute);
				else
					sprintf(Buf+11, "     ");
			}
		}
		else
			Buf[0] = NUL;
	}
	else
	{
//		if (!strftime((char *)str, 100, "%c",  (const struct tm *)thetime))
//			SetTaskMsg("strftime が失敗しました!\n");
	}
	return;
}


/*----- ファイルタイムを指定タイムゾーンのローカルタイムからGMTに変換 ---------
*
*	Parameter
*		FILETIME *Time : ファイルタイム
*		int TimeZone : タイムゾーン
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SpecificLocalFileTime2FileTime(FILETIME *Time, int TimeZone)
{
	unsigned __int64 Tmp64;

	Tmp64 = (unsigned __int64)Time->dwLowDateTime +
			((unsigned __int64)Time->dwHighDateTime << 32);

	Tmp64 -= (__int64)TimeZone * (__int64)36000000000;

	Time->dwHighDateTime = (DWORD)(Tmp64 >> 32);
	Time->dwLowDateTime = (DWORD)(Tmp64 & 0xFFFFFFFF);

	return;
}


/*----- 属性文字列を値に変換 --------------------------------------------------
*
*	Parameter
*		char *Str : 属性文字列 ("rwxrwxrwx")
*
*	Return Value
*		int 値
*----------------------------------------------------------------------------*/

int AttrString2Value(char *Str)
{
	int Ret;
	char Tmp[10];

	Ret = 0;
	memset(Tmp, 0, 10);
	strncpy(Tmp, Str, 9);

	if(Tmp[0] != '-')
		Ret |= 0x400;
	if(Tmp[1] != '-')
		Ret |= 0x200;
	if(Tmp[2] != '-')
		Ret |= 0x100;

	if(Tmp[3] != '-')
		Ret |= 0x40;
	if(Tmp[4] != '-')
		Ret |= 0x20;
	if(Tmp[5] != '-')
		Ret |= 0x10;

	if(Tmp[6] != '-')
		Ret |= 0x4;
	if(Tmp[7] != '-')
		Ret |= 0x2;
	if(Tmp[8] != '-')
		Ret |= 0x1;

	return(Ret);
}


/*----- 属性の値を文字列に変換 ------------------------------------------------
*
*	Parameter
*		int Attr : 属性の値
*		char *Buf : 属性文字列をセットするバッファ ("rwxrwxrwx")
*
*	Return Value
*		int 値
*----------------------------------------------------------------------------*/

void AttrValue2String(int Attr, char *Buf)
{
	strcpy(Buf, "---------");

	if(Attr & 0x400)
		Buf[0] = 'r';
	if(Attr & 0x200)
		Buf[1] = 'w';
	if(Attr & 0x100)
		Buf[2] = 'x';

	if(Attr & 0x40)
		Buf[3] = 'r';
	if(Attr & 0x20)
		Buf[4] = 'w';
	if(Attr & 0x10)
		Buf[5] = 'x';

	if(Attr & 0x4)
		Buf[6] = 'r';
	if(Attr & 0x2)
		Buf[7] = 'w';
	if(Attr & 0x1)
		Buf[8] = 'x';

	return;
}


/*----- INIファイル文字列を整形 -----------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void FormatIniString(char *Str)
{
	char *Put;

	Put = Str;
	while(*Str != NUL)
	{
		if((*Str != ' ') && (*Str != '\t') && (*Str != '\n'))
			*Put++ = *Str;
		if(*Str++ == '=')
			break;
	}

	while(*Str != NUL)
	{
		if((*Str != 0x22) && (*Str != '\n'))
			*Put++ = *Str;
		Str++;
	}
	*Put = NUL;

	return;
}


/*----- ファイル選択 ----------------------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		char *Fname : ファイル名を返すバッファ
*		char *Title : タイトル
*		char *Filters : フィルター文字列
*		char *Ext : デフォルト拡張子
*		int Flags : 追加するフラグ
*		int Save : 「開く」か「保存」か (0=開く, 1=保存)
*
*	Return Value
*		int ステータス
*			TRUE/FALSE=取消
*----------------------------------------------------------------------------*/

int SelectFile(HWND hWnd, char *Fname, char *Title, char *Filters, char *Ext, int Flags, int Save)
{
	OPENFILENAME OpenFile;
	char Tmp[FMAX_PATH+1];
	char Cur[FMAX_PATH+1];
	int Sts;

	GetCurrentDirectory(FMAX_PATH, Cur);

	strcpy(Tmp, Fname);
	// 変数が未初期化のバグ修正
	memset(&OpenFile, 0, sizeof(OPENFILENAME));
	OpenFile.lStructSize = sizeof(OPENFILENAME);
	OpenFile.hwndOwner = hWnd;
	OpenFile.hInstance = 0;
	OpenFile.lpstrFilter = Filters;
	OpenFile.lpstrCustomFilter = NULL;
	OpenFile.nFilterIndex = 1;
	OpenFile.lpstrFile = Tmp;
	OpenFile.nMaxFile = FMAX_PATH;
	OpenFile.lpstrFileTitle = NULL;
	OpenFile.nMaxFileTitle = 0;
	OpenFile.lpstrInitialDir = NULL;
	OpenFile.lpstrTitle = Title;
	OpenFile.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | Flags;
	OpenFile.nFileOffset = 0;
	OpenFile.nFileExtension = 0;
	OpenFile.lpstrDefExt = Ext;
	OpenFile.lCustData = 0;
	OpenFile.lpfnHook = NULL;
	OpenFile.lpTemplateName = NULL;

	if(Save == 0)
	{
		if((Sts = GetOpenFileName(&OpenFile)) == TRUE)
			strcpy(Fname,Tmp);
	}
	else
	{
		if((Sts = GetSaveFileName(&OpenFile)) == TRUE)
			strcpy(Fname,Tmp);
	}
	SetCurrentDirectory(Cur);
	return(Sts);
}


/*----- ディレクトリを選択 ----------------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		char *Buf : ディレクトリ名を返すバッファ（初期ディレクトリ名）
*		int MaxLen : バッファのサイズ
*
*	Return Value
*		int ステータス
*			TRUE/FALSE=取消
*----------------------------------------------------------------------------*/

int SelectDir(HWND hWnd, char *Buf, int MaxLen)
{
	char Tmp[FMAX_PATH+1];
	char Cur[FMAX_PATH+1];
	BROWSEINFO  Binfo;
	LPITEMIDLIST lpIdll;
	int Sts;
	LPMALLOC lpMalloc;

	Sts = FALSE;
	GetCurrentDirectory(FMAX_PATH, Cur);

	if(SHGetMalloc(&lpMalloc) == NOERROR)
	{
		Binfo.hwndOwner = hWnd;
		Binfo.pidlRoot = NULL;
		Binfo.pszDisplayName = Tmp;
		Binfo.lpszTitle = MSGJPN185;
		Binfo.ulFlags = BIF_RETURNONLYFSDIRS;
		Binfo.lpfn = NULL;
		Binfo.lParam = 0;
		Binfo.iImage = 0;
		if((lpIdll = SHBrowseForFolder(&Binfo)) != NULL)
		{
			SHGetPathFromIDList(lpIdll, Tmp);
			memset(Buf, NUL, MaxLen);
			strncpy(Buf, Tmp, MaxLen-1);
			Sts = TRUE;
			lpMalloc->lpVtbl->Free(lpMalloc, lpIdll);
	    }
	    lpMalloc->lpVtbl->Release(lpMalloc);
		SetCurrentDirectory(Cur);
	}
	return(Sts);
}


/*----- 値に関連付けられたラジオボタンをチェックする --------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int Value : 値
*		const RADIOBUTTON *Buttons : ラジオボタンと値の関連付けテーブル
*		int Num : ボタンの数
*
*	Return Value
*		なし
*
*	Note
*		値に関連付けられたボタンが無い時は、テーブルの最初に登録されているボタ
*		ンをチェックする
*----------------------------------------------------------------------------*/

void SetRadioButtonByValue(HWND hDlg, int Value, const RADIOBUTTON *Buttons, int Num)
{
	int i;
	int Def;

	Def = Buttons->ButID;
	for(i = 0; i < Num; i++)
	{
		if(Value == Buttons->Value)
		{
			SendDlgItemMessage(hDlg, Buttons->ButID, BM_SETCHECK, 1, 0);
			/* ラジオボタンを変更した時に他の項目のハイドなどを行なう事が	*/
			/* あるので、そのために WM_COMMAND を送る 						*/
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(Buttons->ButID, 0), 0);
			break;
		}
		Buttons++;
	}
	if(i == Num)
	{
		SendDlgItemMessage(hDlg, Def, BM_SETCHECK, 1, 0);
		SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(Def, 0), 0);
	}
	return;
}


/*----- チェックされているボタンに関連付けられた値を返す ----------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		const RADIOBUTTON *Buttons : ラジオボタンと値の関連付けテーブル
*		int Num : ボタンの数
*
*	Return Value
*		int 値
*
*	Note
*		どのボタンもチェックされていない時は、テーブルの最初に登録されているボ
*		タンの値を返す
*----------------------------------------------------------------------------*/

int AskRadioButtonValue(HWND hDlg, const RADIOBUTTON *Buttons, int Num)
{
	int i;
	int Ret;

	Ret = Buttons->Value;
	for(i = 0; i < Num; i++)
	{
		if(SendDlgItemMessage(hDlg, Buttons->ButID, BM_GETCHECK, 0, 0) == 1)
		{
			Ret = Buttons->Value;
			break;
		}
		Buttons++;
	}
	return(Ret);
}


/*----- １６進文字列を数値に変換 ----------------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		int 値
*----------------------------------------------------------------------------*/

int xtoi(char *Str)
{
	int Ret;

	Ret = 0;
	while(*Str != NUL)
	{
		Ret *= 0x10;
		if((*Str >= '0') && (*Str <= '9'))
			Ret += *Str - '0';
		else if((*Str >= 'A') && (*Str <= 'F'))
			Ret += *Str - 'A' + 10;
		else if((*Str >= 'a') && (*Str <= 'f'))
			Ret += *Str - 'a' + 10;
		else
			break;

		Str++;
	}
	return(Ret);
}


/*----- ファイルが読み取り可能かどうかを返す ----------------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*----------------------------------------------------------------------------*/

int CheckFileReadable(char *Fname)
{
	int Sts;
	HANDLE iFileHandle;
	SECURITY_ATTRIBUTES Sec;

	Sts = FFFTP_FAIL;

	Sec.nLength = sizeof(SECURITY_ATTRIBUTES);
	Sec.lpSecurityDescriptor = NULL;
	Sec.bInheritHandle = FALSE;

	if((iFileHandle = CreateFile(Fname, GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE, &Sec, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		Sts = FFFTP_SUCCESS;
		CloseHandle(iFileHandle);
	}
	return(Sts);
}





int max1(int n, int m)
{
	if(n > m)
		return(n);
	else
		return(m);
}



int min1(int n, int m)
{
	if(n < m)
		return(n);
	else
		return(m);
}


void ExcEndianDWORD(DWORD *x)
{
	BYTE *Pos;
	BYTE Tmp;

	Pos = (BYTE *)x;
	Tmp = *(Pos + 0);
	*(Pos + 0) = *(Pos + 3);
	*(Pos + 3) = Tmp;
	Tmp = *(Pos + 1);
	*(Pos + 1) = *(Pos + 2);
	*(Pos + 2) = Tmp;
	return;
}




/*----- int値の入れ替え -------------------------------------------------------
*
*	Parameter
*		int *Num1 : 数値１
*		int *Num2 : 数値２
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void SwapInt(int *Num1, int *Num2)
{
	int Tmp;

	Tmp = *Num1;
	*Num1 = *Num2;
	*Num2 = Tmp;
	return;
}


/*----- 指定されたフォルダがあるかどうかチェック -------------------------------
*
*	Parameter
*		char *Path : パス
*
*	Return Value
*		int ステータス (YES/NO)
*----------------------------------------------------------------------------*/

int IsFolderExist(char *Path)
{
	int Sts;
	char Tmp[FMAX_PATH+1];
	DWORD Attr;

	Sts = YES;
	if(strlen(Path) > 0)
	{
		strcpy(Tmp, Path);
		if(_mbscmp(Tmp+1, ":\\") != 0)
			RemoveYenTail(Tmp);

		Attr = GetFileAttributes(Tmp);
		if((Attr == 0xFFFFFFFF) || ((Attr & FILE_ATTRIBUTE_DIRECTORY) == 0))
			Sts = NO;
	}
	return(Sts);
}


/*----- テーブルにしたがって数値を登録 -----------------------------------------
*
*	Parameter
*		int x : 数値
*		int Dir : 変換方向
*		INTCONVTBL *Tbl : テーブル
*		int Num : テーブルの数値の数
*
*	Return Value
*		int 数値
*----------------------------------------------------------------------------*/

int ConvertNum(int x, int Dir, const INTCONVTBL *Tbl, int Num)
{
	int i;
	int Ret;

	Ret = x;
	for(i = 0; i < Num; i++)
	{
		if((Dir == 0) && (Tbl->Num1 == x))
		{
			Ret = Tbl->Num2;
			break;
		}
		else if((Dir == 1) && (Tbl->Num2 == x))
		{
			Ret = Tbl->Num1;
			break;
		}
		Tbl++;
	}
	return(Ret);
}






/*----- ファイルをゴミ箱に削除 ------------------------------------------------
*
*	Parameter
*		char *Path : ファイル名
*
*	Return Value
*		int ステータス (0=正常終了)
*----------------------------------------------------------------------------*/

int MoveFileToTrashCan(char *Path)
{
	SHFILEOPSTRUCT FileOp;
	char Tmp[FMAX_PATH+2];

	memset(Tmp, 0, FMAX_PATH+2);
	strcpy(Tmp, Path);
	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE;
	FileOp.pFrom = Tmp;
	FileOp.pTo = "";
	FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_ALLOWUNDO;
	FileOp.lpszProgressTitle = "";
	return(SHFileOperation(&FileOp));
}




LONGLONG MakeLongLong(DWORD High, DWORD Low)
{
	LONGLONG z;
	LONGLONG x1, y1;

	x1 = (LONGLONG)Low;
	y1 = (LONGLONG)High;
	z = x1 | (y1 << 32);
	return(z);
}


char *MakeNumString(LONGLONG Num, char *Buf, BOOL Comma)
{
	int i;
	char *Pos;

	Pos = Buf;
	*Pos = '\0';

	i = 1;
	do
	{
		*Pos++ = (char)(Num % 10) + '0';
		Num /= 10;
		if((Comma == TRUE) && ((i % 3) == 0) && (Num != 0))
			*Pos++ = ',';
		i++;
	}
	while(Num != 0);
	*Pos = NUL;
	_strrev(Buf);

	return(Buf);
}


// 異なるファイルが表示されるバグ修正

// ShellExecute等で使用されるファイル名を修正
// UNCでない場合に末尾の半角スペースは無視されるため拡張子が補完されなくなるまで半角スペースを追加
// 現在UNC対応の予定は無い
char* MakeDistinguishableFileName(char* Out, char* In)
{
	char* Fname;
	char Tmp[FMAX_PATH+1];
	char Tmp2[FMAX_PATH+3];
	HANDLE hFind;
	WIN32_FIND_DATA Find;
	if(strlen(GetFileExt(GetFileName(In))) > 0)
		strcpy(Out, In);
	else
	{
		Fname = GetFileName(In);
		strcpy(Tmp, In);
		strcpy(Tmp2, Tmp);
		strcat(Tmp2, ".*");
		while(strlen(Tmp) < FMAX_PATH && (hFind = FindFirstFile(Tmp2, &Find)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(strcmp(Find.cFileName, Fname) != 0)
					break;
			}
			while(FindNextFile(hFind, &Find));
			FindClose(hFind);
			if(strcmp(Find.cFileName, Fname) != 0)
			{
				strcat(Tmp, " ");
				strcpy(Tmp2, Tmp);
				strcat(Tmp2, ".*");
			}
			else
				break;
		}
		strcpy(Out, Tmp);
	}
	return Out;
}

// 環境依存の不具合対策
char* GetAppTempPath(char* Buf)
{
	char Temp[32];
	GetTempPath(MAX_PATH, Buf);
	SetYenTail(Buf);
	sprintf(Temp, "ffftp%08x", GetCurrentProcessId());
	strcat(Buf, Temp);
	return Buf;
}

#if defined(HAVE_TANDEM)
/*----- ファイルサイズからEXTENTサイズの計算を行う ----------------------------
*
*	Parameter
*		LONGLONG Size : ファイルサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
void CalcExtentSize(TRANSPACKET *Pkt, LONGLONG Size)
{
	LONGLONG extent;

	/* EXTENTS(4,28) MAXEXTENTS 978 */
	if(Size < 56025088) {
		Pkt->PriExt = DEF_PRIEXT;
		Pkt->SecExt = DEF_SECEXT;
		Pkt->MaxExt = DEF_MAXEXT;
	} else {
		/* 増加余地を残すため Used 75% 近辺になるように EXTENT サイズを調整) */
		extent = (LONGLONG)(Size / ((DEF_MAXEXT * 0.75) * 2048LL));
		/* 28未満にすると誤差でFile Fullになる可能性がある */
		if(extent < 28)
			extent = 28;

		Pkt->PriExt = (int)extent;
		Pkt->SecExt = (int)extent;
		Pkt->MaxExt = DEF_MAXEXT;
	}
}
#endif
