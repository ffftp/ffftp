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

#include "common.h"


/*===== ローカルなワーク =====*/

static int DisplayDPIX;
static int DisplayDPIY;


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
	if(_mbscmp(_mbsninc((const unsigned char*)Str, _mbslen((const unsigned char*)Str) - 1), (const unsigned char*)"\\") != 0)
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
		Pos = (char*)_mbsninc((const unsigned char*)Str, _mbslen((const unsigned char*)Str) - 1);
		if(_mbscmp((const unsigned char*)Pos, (const unsigned char*)"\\") == 0)
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
		if(_mbscmp(_mbsninc((const unsigned char*)Str, _mbslen((const unsigned char*)Str) - 1), (const unsigned char*)".") != 0)
			strcat(Str, ".");
	} else
#endif
	if(_mbscmp(_mbsninc((const unsigned char*)Str, _mbslen((const unsigned char*)Str) - 1), (const unsigned char*)"/") != 0)
		strcat(Str, "/");

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
	while((Pos = (char*)_mbschr((const unsigned char*)Str, Src)) != NULL)
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

	if((Pos = (char*)_mbschr((const unsigned char*)Path, ':')) != NULL)
		Path = Pos + 1;

	if((Pos = (char*)_mbsrchr((const unsigned char*)Path, '\\')) != NULL)
		Path = Pos + 1;

	if((Pos = (char*)_mbsrchr((const unsigned char*)Path, '/')) != NULL)
		Path = Pos + 1;

#if defined(HAVE_TANDEM)
	/* Tandem は . がデリミッタとなる */
	if((AskHostType() == HTYPE_TANDEM) && ((Pos = (char*)_mbsrchr((const unsigned char*)Path, '.')) != NULL))
		Path = Pos + 1;
#endif
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

	Ret = (char*)_mbschr((const unsigned char*)Path, NUL);
	if((_mbscmp((const unsigned char*)Path, (const unsigned char*)".") != 0) &&
	   (_mbscmp((const unsigned char*)Path, (const unsigned char*)"..") != 0))
	{
		while((Path = (char*)_mbschr((const unsigned char*)Path, '.')) != NULL)
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

	if((Pos = (char*)_mbsrchr((const unsigned char*)Buf, '/')) != NULL)
		*Pos = NUL;
	else if((Pos = (char*)_mbsrchr((const unsigned char*)Buf, '\\')) != NULL)
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

	if(((Top = (char*)_mbschr((const unsigned char*)Path, '/')) != NULL) ||
	   ((Top = (char*)_mbschr((const unsigned char*)Path, '\\')) != NULL))
	{
		Top++;
		if(((Pos = (char*)_mbsrchr((const unsigned char*)Top, '/')) != NULL) ||
		   ((Pos = (char*)_mbsrchr((const unsigned char*)Top, '\\')) != NULL))
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

	if(((Pos = (char*)_mbsrchr((const unsigned char*)Path, '/')) != NULL) ||
	   ((Pos = (char*)_mbsrchr((const unsigned char*)Path, '\\')) != NULL))
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
	while(((Pos = (char*)_mbschr((const unsigned char*)Path, '/')) != NULL) ||
		  ((Pos = (char*)_mbschr((const unsigned char*)Path, '\\')) != NULL))
	{
		Path = Pos + 1;
		Level++;
	}
	return(Level);
}


// ファイルサイズを文字列に変換する
void MakeSizeString(double Size, char *Buf) {
	if (Size < 1024)
		sprintf(Buf, "%.0lfB", Size);
	else if (Size /= 1024; Size < 1024)
		sprintf(Buf, "%.2lfKB", Size);
	else if (Size /= 1024; Size < 1024)
		sprintf(Buf, "%.2lfMB", Size);
	else if (Size /= 1024; Size < 1024)
		sprintf(Buf, "%.2lfGB", Size);
	else if (Size /= 1024; Size < 1024)
		sprintf(Buf, "%.2lfTB", Size);
	else
		sprintf(Buf, "%.2lfPB", Size / 1024);
}


// StaticTextの領域に収まるようにパス名を整形して表示
void DispStaticText(HWND hWnd, char *Str) {
	RECT rect;
	GetClientRect(hWnd, &rect);
	auto width = rect.right - rect.left;

	auto dc = GetDC(hWnd);
	auto wStr = u8(Str);
	auto p = wStr.data();
	for (;;) {
		int len = (int)wcslen(p);
		if (len <= 4)
			break;
		SIZE size;
		GetTextExtentPoint32W(dc, p, len, &size);
		if (size.cx <= rect.right)
			break;
		p += 4;
		auto q = wcschr(p, L'\\');
		if (q == NULL)
			q = wcschr(p, L'/');
		if (q == NULL)
			q = p + 4;
		p = q - 3;
		std::fill_n(p, 3, L'.');
	}
	ReleaseDC(hWnd, dc);
	SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)p);
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
		Tmp = (int)strlen(Str) + 1;
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

	if((Pos1 = (char*)_mbsstr((const unsigned char*)unc, (const unsigned char*)"//")) != NULL)
		Pos1 += 2;
	else
		Pos1 = unc;

	if((Pos2 = (char*)_mbschr((const unsigned char*)Pos1, '@')) != NULL)
	{
		memset(Tmp, NUL, FMAX_PATH+1);
		memcpy(Tmp, Pos1, Pos2-Pos1);
		Pos1 = Pos2 + 1;

		if((Pos2 = (char*)_mbschr((const unsigned char*)Tmp, ':')) != NULL)
		{
			memcpy(User, Tmp, min1((int)(Pos2-Tmp), USER_NAME_LEN));
			strncpy(Pass, Pos2+1, PASSWORD_LEN);
		}
		else
			strncpy(User, Tmp, USER_NAME_LEN);
	}

	// IPv6対応
	if((Pos2 = (char*)_mbschr((const unsigned char*)Pos1, '[')) != NULL && Pos2 < (char*)_mbschr((const unsigned char*)Pos1, ':'))
	{
		Pos1 = Pos2 + 1;
		if((Pos2 = (char*)_mbschr((const unsigned char*)Pos2, ']')) != NULL)
		{
			memcpy(Host, Pos1, min1((int)(Pos2-Pos1), HOST_ADRS_LEN));
			Pos1 = Pos2 + 1;
		}
	}

	if((Pos2 = (char*)_mbschr((const unsigned char*)Pos1, ':')) != NULL)
	{
		// IPv6対応
//		memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
		if(strlen(Host) == 0)
			memcpy(Host, Pos1, min1((int)(Pos2-Pos1), HOST_ADRS_LEN));
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
	else if((Pos2 = (char*)_mbschr((const unsigned char*)Pos1, '/')) != NULL)
	{
		// IPv6対応
//		memcpy(Host, Pos1, min1(Pos2-Pos1, HOST_ADRS_LEN));
		if(strlen(Host) == 0)
			memcpy(Host, Pos1, min1((int)(Pos2-Pos1), HOST_ADRS_LEN));
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
		// タイムスタンプのバグ修正
//		FileTimeToSystemTime(&fTime, &sTime);
		if(!FileTimeToSystemTime(&fTime, &sTime))
			InfoExist = 0;

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
	// ファイルの属性を数字で表示
//	strncpy(Tmp, Str, 9);
//
//	if(Tmp[0] != '-')
//		Ret |= 0x400;
//	if(Tmp[1] != '-')
//		Ret |= 0x200;
//	if(Tmp[2] != '-')
//		Ret |= 0x100;
//
//	if(Tmp[3] != '-')
//		Ret |= 0x40;
//	if(Tmp[4] != '-')
//		Ret |= 0x20;
//	if(Tmp[5] != '-')
//		Ret |= 0x10;
//
//	if(Tmp[6] != '-')
//		Ret |= 0x4;
//	if(Tmp[7] != '-')
//		Ret |= 0x2;
//	if(Tmp[8] != '-')
//		Ret |= 0x1;
	if(strlen(Str) >= 9)
	{
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
	}
	else if(strlen(Str) >= 3)
	{
		strncpy(Tmp, Str, 3);
		Ret = strtol(Tmp, NULL, 16);
	}

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

// ファイルの属性を数字で表示
//void AttrValue2String(int Attr, char *Buf)
void AttrValue2String(int Attr, char *Buf, int ShowNumber)
{
	// ファイルの属性を数字で表示
//	strcpy(Buf, "---------");
//
//	if(Attr & 0x400)
//		Buf[0] = 'r';
//	if(Attr & 0x200)
//		Buf[1] = 'w';
//	if(Attr & 0x100)
//		Buf[2] = 'x';
//
//	if(Attr & 0x40)
//		Buf[3] = 'r';
//	if(Attr & 0x20)
//		Buf[4] = 'w';
//	if(Attr & 0x10)
//		Buf[5] = 'x';
//
//	if(Attr & 0x4)
//		Buf[6] = 'r';
//	if(Attr & 0x2)
//		Buf[7] = 'w';
//	if(Attr & 0x1)
//		Buf[8] = 'x';
	if(ShowNumber == YES)
	{
		sprintf(Buf, "%03x", Attr);
	}
	else
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
	}

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
		if((*Str != '\"') && (*Str != '\n'))
			*Put++ = *Str;
		Str++;
	}
	*Put = NUL;

	return;
}

static auto GetFilterString(std::initializer_list<FileType> fileTypes) {
	static auto const map = [] {
		std::map<FileType, std::wstring> map;
		for (auto fileType : AllFileTyes) {
			// リソース文字列は末尾 '\0' を扱えないので '\t' を利用し、ここで置換する。
			auto text = GetString(static_cast<UINT>(fileType));
			for (auto& ch : text)
				if (ch == L'\t')
					ch = L'\0';
			map.emplace(fileType, text);
		}
		return map;
	}();
	return std::accumulate(begin(fileTypes), end(fileTypes), L""s, [](auto const& result, auto fileType) { return result + map.at(fileType); });
}

// ファイル選択
fs::path SelectFile(bool open, HWND hWnd, UINT titleId, const wchar_t* initialFileName, const wchar_t* extension, std::initializer_list<FileType> fileTypes) {
	auto const filter = GetFilterString(fileTypes);
	wchar_t buffer[FMAX_PATH + 1];
	wcscpy(buffer, initialFileName);
	auto const title = GetString(titleId);
	DWORD flags = (open ? OFN_FILEMUSTEXIST : OFN_EXTENSIONDIFFERENT | OFN_OVERWRITEPROMPT) | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	OPENFILENAMEW ofn{ sizeof(OPENFILENAMEW), hWnd, 0, filter.c_str(), nullptr, 0, 1, buffer, size_as<DWORD>(buffer), nullptr, 0, nullptr, title.c_str(), flags, 0, 0, extension };
	auto const cwd = fs::current_path();
	int result = (open ? GetOpenFileNameW : GetSaveFileNameW)(&ofn);
	fs::current_path(cwd);
	if (!result)
		return {};
	return buffer;
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

int SelectDir(HWND hWnd, char *Buf, int MaxLen) {
	int result = FALSE;
	auto const cwd = fs::current_path();
	wchar_t buffer[FMAX_PATH + 1];
	auto const title = u8(MSGJPN185);
	BROWSEINFOW bi{ hWnd, nullptr, buffer, title.c_str(), BIF_RETURNONLYFSDIRS };
	if (auto idlist = SHBrowseForFolderW(&bi)) {
		SHGetPathFromIDListW(idlist, buffer);
		strncpy(Buf, u8(buffer).c_str(), MaxLen - 1);
		result = TRUE;
		CoTaskMemFree(idlist);
	}
	fs::current_path(cwd);
	return result;
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


// ファイルをゴミ箱に削除
int MoveFileToTrashCan(const char *Path) {
	auto wPath = u8(Path);
	wPath += L'\0';			// for PCZZSTR
	SHFILEOPSTRUCTW op{ 0, FO_DELETE, wPath.c_str(), nullptr, FOF_SILENT | FOF_NOCONFIRMATION | FOF_ALLOWUNDO | FOF_NOERRORUI };
	return SHFileOperationW(&op);
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


// ShellExecute等で使用されるファイル名を修正
// UNCでない場合に末尾の半角スペースは無視されるため拡張子が補完されなくなるまで半角スペースを追加
// 現在UNC対応の予定は無い
char* MakeDistinguishableFileName(char* Out, char* In) {
	if (strlen(GetFileExt(GetFileName(In))) > 0)
		strcpy(Out, In);
	else {
		auto const Fname = u8(GetFileName(In));
		auto current = u8(In);
		WIN32_FIND_DATAW data;
		for (HANDLE handle; (handle = FindFirstFileW((current + L".*"sv).c_str(), &data)) != INVALID_HANDLE_VALUE; current += L' ') {
			bool invalid = false;
			do {
				if (data.cFileName != Fname) {
					invalid = true;
					break;
				}
			} while (FindNextFileW(handle, &data));
			FindClose(handle);
			if (!invalid)
				break;
		}
		strcpy(Out, u8(current).c_str());
	}
	return Out;
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

// 高DPI対応
void QueryDisplayDPI()
{
	HDC hDC;
	if(DisplayDPIX == 0)
	{
		if(hDC = GetDC(NULL))
		{
			DisplayDPIX = GetDeviceCaps(hDC, LOGPIXELSX);
			DisplayDPIY = GetDeviceCaps(hDC, LOGPIXELSY);
			ReleaseDC(NULL, hDC);
		}
	}
}

int CalcPixelX(int x)
{
	QueryDisplayDPI();
	return (x * DisplayDPIX + 96 / 2) / 96;
}

int CalcPixelY(int y)
{
	QueryDisplayDPI();
	return (y * DisplayDPIY + 96 / 2) / 96;
}

HBITMAP ResizeBitmap(HBITMAP hBitmap, int UnitSizeX, int UnitSizeY, int ScaleNumerator, int ScaleDenominator)
{
	HBITMAP hDstBitmap;
	HDC hDC;
	HDC hSrcDC;
	HDC hDstDC;
	BITMAP Bitmap;
	HGDIOBJ hSrcOld;
	HGDIOBJ hDstOld;
	int Width;
	int Height;
	hDstBitmap = NULL;
	if(hDC = GetDC(NULL))
	{
		if(hSrcDC = CreateCompatibleDC(hDC))
		{
			if(hDstDC = CreateCompatibleDC(hDC))
			{
				if(GetObjectW(hBitmap, sizeof(BITMAP), &Bitmap) > 0)
				{
					if(UnitSizeX == 0)
						UnitSizeX = Bitmap.bmWidth;
					if(UnitSizeY == 0)
						UnitSizeY = Bitmap.bmHeight;
					Width = (Bitmap.bmWidth / UnitSizeX) * CalcPixelX((UnitSizeX * ScaleNumerator) / ScaleDenominator);
					Height = (Bitmap.bmHeight / UnitSizeY) * CalcPixelY((UnitSizeY * ScaleNumerator) / ScaleDenominator);
					if(hDstBitmap = CreateCompatibleBitmap(hDC, Width, Height))
					{
						hSrcOld = SelectObject(hSrcDC, hBitmap);
						hDstOld = SelectObject(hDstDC, hDstBitmap);
						SetStretchBltMode(hDstDC, COLORONCOLOR);
						StretchBlt(hDstDC, 0, 0, Width, Height, hSrcDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);
						SelectObject(hSrcDC, hSrcOld);
						SelectObject(hDstDC, hDstOld);
					}
				}
				DeleteDC(hDstDC);
			}
			DeleteDC(hSrcDC);
		}
		ReleaseDC(NULL, hDC);
	}
	return hDstBitmap;
}
