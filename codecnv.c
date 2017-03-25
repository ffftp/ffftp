/*=============================================================================
*
*							漢字コード変換／改行コード変換
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
#include <string.h>
// IPv6対応
//#include <winsock.h>
#include <winsock2.h>
#include <mbstring.h>
#include <windowsx.h>
// UTF-8対応
#include <winnls.h>

#include "common.h"
#include "resource.h"



#define CONV_ASCII		0		/* ASCII文字処理中 */
#define CONV_KANJI		1		/* 漢字処理中 */
#define CONV_KANA		2		/* 半角カタカナ処理中 */


/*===== プロトタイプ =====*/

static char *ConvEUCtoSJISkanaProc(CODECONVINFO *cInfo, char Dt, char *Put);
static char *ConvJIStoSJISkanaProc(CODECONVINFO *cInfo, char Dt, char *Put);
static char *ConvSJIStoEUCkanaProc(CODECONVINFO *cInfo, char Dt, char *Put);
static char *ConvSJIStoJISkanaProc(CODECONVINFO *cInfo, char Dt, char *Put);
static int HanKataToZen(char Ch);
static int AskDakuon(char Ch, char Daku);

static int CheckOnSJIS(uchar *Pos, uchar *Btm);
static int CheckOnEUC(uchar *Pos, uchar *Btm);
static int ConvertIBMExtendedChar(int code);


typedef enum _NORM_FORM
{
	NormalizationOther = 0,
	NormalizationC = 0x1,
	NormalizationD = 0x2,
	NormalizationKC = 0x5,
	NormalizationKD = 0x6
} NORM_FORM;

typedef int (WINAPI* _NormalizeString)(NORM_FORM, LPCWSTR, int, LPWSTR, int);

HMODULE hUnicodeNormalizationDll;
_NormalizeString p_NormalizeString;

#if 0
/*----- 漢字コード変換のテストプログラム ------------------------------------*/

void CodeCnvTest(void)
{
	#define BUFBUF	43
	#define BUFBUF2	BUFBUF+3

	CODECONVINFO cInfo;
	char Buf[BUFBUF];
	char Buf2[BUFBUF2];
	FILE *Strm1;
	FILE *Strm2;
	int Byte;
	int Continue;

//	DoPrintf("---START ZEN");

	Strm1 = fopen("in.txt", "rb");
	Strm2 = fopen("out_zen.txt", "wb");

	InitCodeConvInfo(&cInfo);
	cInfo.KanaCnv = YES;


	while((Byte = fread(Buf, 1, BUFBUF, Strm1)) != 0)
	{
		cInfo.Str = Buf;
		cInfo.StrLen = Byte;
		cInfo.Buf = Buf2;
		cInfo.BufSize = BUFBUF2;

//		DoPrintf("READ %d", Byte);

		do
		{
//			Continue = ConvEUCtoSJIS(&cInfo);
//			Continue = ConvJIStoSJIS(&cInfo);
//			Continue = ConvSJIStoEUC(&cInfo);
//			Continue = ConvSJIStoJIS(&cInfo);
			Continue = ConvSMBtoSJIS(&cInfo);
//			Continue = ConvSJIStoSMB_HEX(&cInfo);
//			Continue = ConvSJIStoSMB_CAP(&cInfo);

			fwrite(Buf2, cInfo.OutLen, 1, Strm2);
//			DoPrintf("WRITE %d", cInfo.OutLen);

		}
		while(Continue == YES);
	}

	cInfo.Buf = Buf2;
	cInfo.BufSize = BUFBUF2;
	FlushRestData(&cInfo);
	fwrite(Buf2, cInfo.OutLen, 1, Strm2);
//	DoPrintf("WRITE %d", cInfo.OutLen);


	fclose(Strm1);
	fclose(Strm2);


//	DoPrintf("---START HAN");

	Strm1 = fopen("in.txt", "rb");
	Strm2 = fopen("out_han.txt", "wb");

	InitCodeConvInfo(&cInfo);
	cInfo.KanaCnv = NO;


	while((Byte = fread(Buf, 1, BUFBUF, Strm1)) != 0)
	{
		cInfo.Str = Buf;
		cInfo.StrLen = Byte;
		cInfo.Buf = Buf2;
		cInfo.BufSize = BUFBUF2;

//		DoPrintf("READ %d", Byte);

		do
		{
//			Continue = ConvEUCtoSJIS(&cInfo);
//			Continue = ConvJIStoSJIS(&cInfo);
//			Continue = ConvSJIStoEUC(&cInfo);
//			Continue = ConvSJIStoJIS(&cInfo);
			Continue = ConvSMBtoSJIS(&cInfo);
//			Continue = ConvSJIStoSMB_HEX(&cInfo);
//			Continue = ConvSJIStoSMB_CAP(&cInfo);
			fwrite(Buf2, cInfo.OutLen, 1, Strm2);
//			DoPrintf("WRITE %d", cInfo.OutLen);

		}
		while(Continue == YES);
	}

	cInfo.Buf = Buf2;
	cInfo.BufSize = BUFBUF2;
	FlushRestData(&cInfo);
	fwrite(Buf2, cInfo.OutLen, 1, Strm2);
//	DoPrintf("WRITE %d", cInfo.OutLen);

	fclose(Strm1);
	fclose(Strm2);

//	DoPrintf("---END");

	return;
}
#endif



#if 0
/*----- 改行コード変換のテストプログラム ------------------------------------*/

void TermCodeCnvTest(void)
{
	#define BUFBUF	10
	#define BUFBUF2	BUFBUF

	TERMCODECONVINFO cInfo;
	char Buf[BUFBUF];
	char Buf2[BUFBUF2];
	FILE *Strm1;
	FILE *Strm2;
	int Byte;
	int Continue;

//	DoPrintf("---START");

	Strm1 = fopen("in.txt", "rb");
	Strm2 = fopen("out.txt", "wb");

	InitTermCodeConvInfo(&cInfo);

	while((Byte = fread(Buf, 1, BUFBUF, Strm1)) != 0)
	{
		cInfo.Str = Buf;
		cInfo.StrLen = Byte;
		cInfo.Buf = Buf2;
		cInfo.BufSize = BUFBUF2;

//		DoPrintf("READ %d", Byte);

		do
		{
			Continue = ConvTermCodeToCRLF(&cInfo);

			fwrite(Buf2, cInfo.OutLen, 1, Strm2);
//			DoPrintf("WRITE %d", cInfo.OutLen);

		}
		while(Continue == YES);
	}

	cInfo.Buf = Buf2;
	cInfo.BufSize = BUFBUF2;
	FlushRestTermCodeConvData(&cInfo);
	fwrite(Buf2, cInfo.OutLen, 1, Strm2);
//	DoPrintf("WRITE %d", cInfo.OutLen);

	fclose(Strm1);
	fclose(Strm2);

//	DoPrintf("---END");

	return;
}
#endif












/*----- 改行コード変換情報を初期化 --------------------------------------------
*
*	Parameter
*		TERMCODECONVINFO *cInfo : 改行コード変換情報
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void InitTermCodeConvInfo(TERMCODECONVINFO *cInfo)
{
	cInfo->Term = 0;
	return;
}


/*----- 改行コード変換の残り情報を出力 ----------------------------------------
*
*	Parameter
*		TERMCODECONVINFO *cInfo : 改行コード変換情報
*
*	Return Value
*		int くり返しフラグ (=NO)
*
*	Note
*		改行コード変換の最後に呼ぶ事
*----------------------------------------------------------------------------*/

int FlushRestTermCodeConvData(TERMCODECONVINFO *cInfo)
{
	char *Put;

	Put = cInfo->Buf;

	if(cInfo->Term == 0x0D)
		*Put++ = 0x0A;

	cInfo->OutLen = Put - cInfo->Buf;

	return(NO);
}


/*----- 改行コードをCRLFに変換 -------------------------------------------------
*
*	Parameter
*		TERMCODECONVINFO *cInfo : 改行コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*----------------------------------------------------------------------------*/

int ConvTermCodeToCRLF(TERMCODECONVINFO *cInfo)
{
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 1;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if(*Str == 0x0D)
		{
			if(cInfo->Term == 0x0D)
				*Put++ = 0x0A;
			*Put++ = 0x0D;
			cInfo->Term = *Str++;
		}
		else
		{
			if(*Str == 0x0A)
			{
				if(cInfo->Term != 0x0D)
					*Put++ = 0x0D;
			}
			else
			{
				if(cInfo->Term == 0x0D)
					*Put++ = 0x0A;
			}
			cInfo->Term = 0;
			*Put++ = *Str++;
		}
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- 漢字コード変換情報を初期化 --------------------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void InitCodeConvInfo(CODECONVINFO *cInfo)
{
	cInfo->KanaCnv = YES;

	cInfo->EscProc = 0;
	cInfo->KanjiMode = CONV_ASCII;
	cInfo->KanjiFst = 0;
	cInfo->KanaPrev = 0;
	cInfo->KanaProc = NULL;
	// UTF-8対応
	cInfo->EscUTF8Len = 0;
	cInfo->EscFlush = NO;
	cInfo->FlushProc = NULL;
	return;
}


/*----- 漢字コード変換の残り情報を出力 ----------------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (=NO)
*
*	Note
*		漢字コード変換の最後に呼ぶ事
*----------------------------------------------------------------------------*/

int FlushRestData(CODECONVINFO *cInfo)
{
	char *Put;

	// UTF-8対応
	if(cInfo->FlushProc != NULL)
	{
		cInfo->EscFlush = YES;
		return cInfo->FlushProc(cInfo);
	}

	Put = cInfo->Buf;

	if(cInfo->KanaProc != NULL)
		Put = (cInfo->KanaProc)(cInfo, 0, Put);

	if(cInfo->KanjiFst != 0)
		*Put++ = cInfo->KanjiFst;
	if(cInfo->EscProc >= 1)
		*Put++ = cInfo->EscCode[0];
	if(cInfo->EscProc == 2)
		*Put++ = cInfo->EscCode[1];

	cInfo->OutLen = Put - cInfo->Buf;

	return(NO);
}


// UTF-8対応
int ConvNoConv(CODECONVINFO *cInfo)
{
	int Continue;
	Continue = NO;
	if(cInfo->BufSize >= cInfo->StrLen)
		cInfo->OutLen = cInfo->StrLen;
	else
	{
		cInfo->OutLen = cInfo->BufSize;
		Continue = YES;
	}
	memcpy(cInfo->Buf, cInfo->Str, sizeof(char) * cInfo->OutLen);
	cInfo->Str += cInfo->OutLen;
	cInfo->StrLen -= cInfo->OutLen;
	return Continue;
}

/*----- EUC漢字コードをSHIFT-JIS漢字コードに変換 ------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*----------------------------------------------------------------------------*/

int ConvEUCtoSJIS(CODECONVINFO *cInfo)
{
	int Kcode;
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	cInfo->KanaProc = &ConvEUCtoSJISkanaProc;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 2;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if((*Str & 0x80) != 0)
		{
			if(cInfo->KanjiFst == 0)
				cInfo->KanjiFst = *Str++;
			else
			{
				if((uchar)cInfo->KanjiFst == (uchar)0x8E)	/* 半角カタカナ */
				{
					Put = ConvEUCtoSJISkanaProc(cInfo, *Str++, Put);
				}
				else
				{
					Put = ConvEUCtoSJISkanaProc(cInfo, 0, Put);

					Kcode = _mbcjistojms(((cInfo->KanjiFst & 0x7F) * 0x100) + (*Str++ & 0x7F));
					*Put++ = HIGH8(Kcode);
					*Put++ = LOW8(Kcode);
				}
				cInfo->KanjiFst = 0;
			}
		}
		else
		{
			Put = ConvEUCtoSJISkanaProc(cInfo, 0, Put);

			if(cInfo->KanjiFst != 0)
			{
				*Put++ = cInfo->KanjiFst;
				cInfo->KanjiFst = 0;
			}
			*Put++ = *Str++;
		}
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- EUC-->SHIFT-JIS漢字コードに変換の半角カタカナの処理 -------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*		char Dt : 文字
*		char *Put : データセット位置
*
*	Return Value
*		char *次のデータセット位置
*----------------------------------------------------------------------------*/

static char *ConvEUCtoSJISkanaProc(CODECONVINFO *cInfo, char Dt, char *Put)
{
	int Kcode;
	int Daku;

	if(cInfo->KanaCnv == NO)
	{
		if(Dt != 0)
			*Put++ = Dt;
	}
	else
	{
		if(cInfo->KanaPrev != 0)
		{
			Daku = AskDakuon(cInfo->KanaPrev, Dt);

			Kcode = _mbcjistojms(HanKataToZen(cInfo->KanaPrev)) + Daku;
			*Put++ = HIGH8(Kcode);
			*Put++ = LOW8(Kcode);

			if(Daku == 0)
				cInfo->KanaPrev = Dt;
			else
				cInfo->KanaPrev = 0;
		}
		else
			cInfo->KanaPrev = Dt;
	}
	return(Put);
}


/*----- JIS漢字コードをSHIFT-JIS漢字コードに変換 ------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*
*		エスケープコードは、次のものに対応している
*			漢字開始		<ESC>$B		<ESC>$@
*			半角カナ開始	<ESC>(I
*			漢字終了		<ESC>(B		<ESC>(J		<ESC>(H
*----------------------------------------------------------------------------*/

int ConvJIStoSJIS(CODECONVINFO *cInfo)
{
	int Kcode;
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	cInfo->KanaProc = &ConvJIStoSJISkanaProc;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 3;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if(cInfo->EscProc == 0)
		{
			if(*Str == 0x1B)
			{
				if(cInfo->KanjiFst != 0)
				{
					*Put++ = cInfo->KanjiFst;
					cInfo->KanjiFst = 0;
				}
				Put = ConvJIStoSJISkanaProc(cInfo, 0, Put);

				cInfo->EscCode[cInfo->EscProc] = *Str++;
				cInfo->EscProc++;
			}
			else
			{
				if(cInfo->KanjiMode == CONV_KANA)
				{
					if(cInfo->KanjiFst != 0)
					{
						*Put++ = cInfo->KanjiFst;
						cInfo->KanjiFst = 0;
					}

					if((*Str >= 0x21) && (*Str <= 0x5F))
					{
						Put = ConvJIStoSJISkanaProc(cInfo, *Str++, Put);
					}
					else
					{
						Put = ConvJIStoSJISkanaProc(cInfo, 0, Put);
						*Put++ = *Str++;
					}
				}
				else if(cInfo->KanjiMode == CONV_KANJI)
				{
					Put = ConvJIStoSJISkanaProc(cInfo, 0, Put);
					if((*Str >= 0x21) && (*Str <= 0x7E))
					{
						if(cInfo->KanjiFst == 0)
							cInfo->KanjiFst = *Str++;
						else
						{
							Kcode = _mbcjistojms((cInfo->KanjiFst * 0x100) + *Str++);
							*Put++ = HIGH8(Kcode);
							*Put++ = LOW8(Kcode);
							cInfo->KanjiFst = 0;
						}
					}
					else
					{
						if(cInfo->KanjiFst == 0)
							*Put++ = *Str++;
						else
						{
							*Put++ = cInfo->KanjiFst;
							*Put++ = *Str++;
							cInfo->KanjiFst = 0;
						}
					}
				}
				else
				{
					Put = ConvJIStoSJISkanaProc(cInfo, 0, Put);
					*Put++ = *Str++;
				}
			}
		}
		else if(cInfo->EscProc == 1)
		{
			if((*Str == '$') || (*Str == '('))
			{
				cInfo->EscCode[cInfo->EscProc] = *Str++;
				cInfo->EscProc++;
			}
			else
			{
				*Put++ = cInfo->EscCode[0];
				*Put++ = *Str++;
				cInfo->EscProc = 0;
			}
		}
		else if(cInfo->EscProc == 2)
		{
			if((cInfo->EscCode[1] == '$') && ((*Str == 'B') || (*Str == '@')))
				cInfo->KanjiMode = CONV_KANJI;
			else if((cInfo->EscCode[1] == '(') && (*Str == 'I'))
				cInfo->KanjiMode = CONV_KANA;
			else if((cInfo->EscCode[1] == '(') && ((*Str == 'B') || (*Str == 'J') || (*Str == 'H')))
				cInfo->KanjiMode = CONV_ASCII;
			else
			{
				*Put++ = cInfo->EscCode[0];
				*Put++ = cInfo->EscCode[1];
				if((cInfo->KanjiMode == CONV_KANJI) && ((*Str >= 0x21) && (*Str <= 0x7E)))
					cInfo->KanjiFst = *Str;
				else
					*Put++ = *Str;
			}
			Str++;
			cInfo->EscProc = 0;
		}
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- JIS-->SHIFT-JIS漢字コードに変換の半角カタカナの処理 -------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*		char Dt : 文字
*		char *Put : データセット位置
*
*	Return Value
*		char *次のデータセット位置
*----------------------------------------------------------------------------*/

static char *ConvJIStoSJISkanaProc(CODECONVINFO *cInfo, char Dt, char *Put)
{
	int Kcode;
	int Daku;

	Dt = (uchar)Dt + (uchar)0x80;
	if(cInfo->KanaCnv == NO)
	{
		if((uchar)Dt != (uchar)0x80)
			*Put++ = Dt;
	}
	else
	{
		if(cInfo->KanaPrev != 0)
		{
			Daku = AskDakuon(cInfo->KanaPrev, Dt);
			Kcode = _mbcjistojms(HanKataToZen(cInfo->KanaPrev)) + Daku;
			*Put++ = HIGH8(Kcode);
			*Put++ = LOW8(Kcode);

			if((Daku == 0) && ((uchar)Dt != (uchar)0x80))
				cInfo->KanaPrev = Dt;
			else
				cInfo->KanaPrev = 0;
		}
		else if((uchar)Dt != (uchar)0x80)
			cInfo->KanaPrev = Dt;
	}
	return(Put);
}


/*----- Samba-HEX/Samba-CAP漢字コードをSHIFT-JIS漢字コードに変換 --------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*		分割された入力文字列の変換はサポートしていない
*		半角カタカナの変換設定には対応していない
*----------------------------------------------------------------------------*/

int ConvSMBtoSJIS(CODECONVINFO *cInfo)
{
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 2;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if((*Str == SAMBA_HEX_TAG) && (cInfo->StrLen >= 3))
		{
			if(isxdigit(*(Str+1)) && isxdigit(*(Str+2)))
			{
				*Put++ = N2INT(hex2bin(*(Str+1)), hex2bin(*(Str+2)));
				Str += 3;
				cInfo->StrLen -= 2;
			}
			else
				*Put++ = *Str++;
		}
		else
			*Put++ = *Str++;
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- SHIFT-JIS漢字コードをEUC漢字コードに変換 ------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*----------------------------------------------------------------------------*/

int ConvSJIStoEUC(CODECONVINFO *cInfo)
{
	int Kcode;
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	cInfo->KanaProc = &ConvSJIStoEUCkanaProc;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 2;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if(cInfo->KanjiFst == 0)
		{
			if((((uchar)*Str >= (uchar)0x81) && ((uchar)*Str <= (uchar)0x9F)) ||
			   ((uchar)*Str >= (uchar)0xE0))
			{
				Put = ConvSJIStoEUCkanaProc(cInfo, 0, Put);
				cInfo->KanjiFst = *Str++;
			}
			else if(((uchar)*Str >= (uchar)0xA0) && ((uchar)*Str <= (uchar)0xDF))
			{
				Put = ConvSJIStoEUCkanaProc(cInfo, *Str++, Put);
			}
			else
			{
				Put = ConvSJIStoEUCkanaProc(cInfo, 0, Put);
				*Put++ = *Str++;
			}
		}
		else
		{
			if((uchar)*Str >= (uchar)0x40)
			{
				Kcode = ConvertIBMExtendedChar(((uchar)cInfo->KanjiFst * 0x100) + (uchar)*Str++);
				Kcode = _mbcjmstojis(Kcode);
				*Put++ = HIGH8(Kcode) | 0x80;
				*Put++ = LOW8(Kcode) | 0x80;
			}
			else
			{
				*Put++ = cInfo->KanjiFst;
				*Put++ = *Str++;
			}
			cInfo->KanjiFst = 0;
		}
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- SHIFT-JIS-->EUC漢字コードに変換の半角カタカナの処理 -------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*		char Dt : 文字
*		char *Put : データセット位置
*
*	Return Value
*		char *次のデータセット位置
*----------------------------------------------------------------------------*/

static char *ConvSJIStoEUCkanaProc(CODECONVINFO *cInfo, char Dt, char *Put)
{
	int Kcode;
	int Daku;

	if(cInfo->KanaCnv == NO)
	{
		if(Dt != 0)
		{
			Kcode = 0x8E00 + (uchar)Dt;
			*Put++ = HIGH8(Kcode) | 0x80;
			*Put++ = LOW8(Kcode) | 0x80;
		}
	}
	else
	{
		if(cInfo->KanaPrev != 0)
		{
			Daku = AskDakuon(cInfo->KanaPrev, Dt);
			Kcode = HanKataToZen(cInfo->KanaPrev) + Daku;
			*Put++ = HIGH8(Kcode) | 0x80;
			*Put++ = LOW8(Kcode) | 0x80;

			if(Daku == 0)
				cInfo->KanaPrev = Dt;
			else
				cInfo->KanaPrev = 0;
		}
		else
			cInfo->KanaPrev = Dt;
	}
	return(Put);
}


/*----- SHIFT-JIS漢字コードをJIS漢字コードに変換 ------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*
*		エスケープコードは、次のものを使用する
*			漢字開始		<ESC>$B
*			半角カナ開始	<ESC>(I
*			漢字終了		<ESC>(B
*----------------------------------------------------------------------------*/

int ConvSJIStoJIS(CODECONVINFO *cInfo)
{
	int Kcode;
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	cInfo->KanaProc = &ConvSJIStoJISkanaProc;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 5;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if(cInfo->KanjiFst == 0)
		{
			if((((uchar)*Str >= (uchar)0x81) && ((uchar)*Str <= (uchar)0x9F)) ||
			   ((uchar)*Str >= (uchar)0xE0))
			{
				Put = ConvSJIStoJISkanaProc(cInfo, 0, Put);
				cInfo->KanjiFst = *Str++;
			}
			else if(((uchar)*Str >= (uchar)0xA0) && ((uchar)*Str <= (uchar)0xDF))
			{
				Put = ConvSJIStoJISkanaProc(cInfo, *Str++, Put);
			}
			else
			{
				Put = ConvSJIStoJISkanaProc(cInfo, 0, Put);
				if(cInfo->KanjiMode != CONV_ASCII)
				{
					*Put++ = 0x1B;
					*Put++ = '(';
					*Put++ = 'B';
					cInfo->KanjiMode = CONV_ASCII;
				}
				*Put++ = *Str++;
			}
		}
		else
		{
			Put = ConvSJIStoJISkanaProc(cInfo, 0, Put);
			if((uchar)*Str >= (uchar)0x40)
			{
				if(cInfo->KanjiMode != CONV_KANJI)
				{
					*Put++ = 0x1B;
					*Put++ = '$';
					*Put++ = 'B';
					cInfo->KanjiMode = CONV_KANJI;
				}

				Kcode = ConvertIBMExtendedChar(((uchar)cInfo->KanjiFst * 0x100) + (uchar)*Str++);
				Kcode = _mbcjmstojis(Kcode);
				*Put++ = HIGH8(Kcode);
				*Put++ = LOW8(Kcode);
			}
			else
			{
				if(cInfo->KanjiMode != CONV_ASCII)
				{
					*Put++ = 0x1B;
					*Put++ = '(';
					*Put++ = 'B';
					cInfo->KanjiMode = CONV_ASCII;
				}
				*Put++ = cInfo->KanjiFst;
				*Put++ = *Str++;
			}
			cInfo->KanjiFst = 0;
		}
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- SHIFT-JIS-->JIS漢字コードに変換の半角カタカナの処理 -------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*		char Dt : 文字
*		char *Put : データセット位置
*
*	Return Value
*		char *次のデータセット位置
*----------------------------------------------------------------------------*/

static char *ConvSJIStoJISkanaProc(CODECONVINFO *cInfo, char Dt, char *Put)
{
	int Kcode;
	int Daku;

	if(cInfo->KanaCnv == NO)
	{
		if(Dt != 0)
		{
			if(cInfo->KanjiMode != CONV_KANA)
			{
				*Put++ = 0x1B;
				*Put++ = '(';
				*Put++ = 'I';
				cInfo->KanjiMode = CONV_KANA;
			}
			*Put++ = (uchar)Dt - (uchar)0x80;
		}
	}
	else
	{
		if(cInfo->KanaPrev != 0)
		{
			if(cInfo->KanjiMode != CONV_KANJI)
			{
				*Put++ = 0x1B;
				*Put++ = '$';
				*Put++ = 'B';
				cInfo->KanjiMode = CONV_KANJI;
			}
			Daku = AskDakuon(cInfo->KanaPrev, Dt);
			Kcode = HanKataToZen(cInfo->KanaPrev) + Daku;
			*Put++ = HIGH8(Kcode);
			*Put++ = LOW8(Kcode);

			if(Daku == 0)
				cInfo->KanaPrev = Dt;
			else
				cInfo->KanaPrev = 0;
		}
		else
			cInfo->KanaPrev = Dt;
	}
	return(Put);
}


/*----- SHIFT-JIS漢字コードをSamba-HEX漢字コードに変換 ------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*		分割された入力文字列の変換はサポートしていない
*		半角カタカナの変換設定には対応していない
*----------------------------------------------------------------------------*/

int ConvSJIStoSMB_HEX(CODECONVINFO *cInfo)
{
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 6;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if((cInfo->StrLen >= 2) &&
		   ((((uchar)*Str >= (uchar)0x81) && ((uchar)*Str <= (uchar)0x9F)) ||
			((uchar)*Str >= (uchar)0xE0)))
		{
			sprintf(Put, "%c%02x%c%02x", SAMBA_HEX_TAG, (uchar)*Str, SAMBA_HEX_TAG, (uchar)*(Str+1));
			Str += 2;
			Put += 6;
			cInfo->StrLen--;
		}
		else if((uchar)*Str >= (uchar)0x80)
		{
			sprintf(Put, "%c%02x", SAMBA_HEX_TAG, (uchar)*Str++);
			Put += 3;
		}
		else
			*Put++ = *Str++;
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- SHIFT-JIS漢字コードをSamba-CAP漢字コードに変換 ------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*		分割された入力文字列の変換はサポートしていない
*----------------------------------------------------------------------------*/

int ConvSJIStoSMB_CAP(CODECONVINFO *cInfo)
{
	char *Str;
	char *Put;
	char *Limit;
	int Continue;

	Continue = NO;
	Str = cInfo->Str;
	Put = cInfo->Buf;
	Limit = cInfo->Buf + cInfo->BufSize - 6;

	for(; cInfo->StrLen > 0; cInfo->StrLen--)
	{
		if(Put >= Limit)
		{
			Continue = YES;
			break;
		}

		if((uchar)*Str >= (uchar)0x80)
		{
			sprintf(Put, "%c%02x", SAMBA_HEX_TAG, (uchar)*Str++);
			Put += 3;
		}
		else
			*Put++ = *Str++;
	}

	cInfo->Str = Str;
	cInfo->OutLen = Put - cInfo->Buf;

	return(Continue);
}


/*----- １バイトカタカナをJIS漢字コードに変換 ---------------------------------
*
*	Parameter
*		char Ch : １バイトカタカナコード
*
*	Return Value
*		int JIS漢字コード
*----------------------------------------------------------------------------*/

static int HanKataToZen(char Ch)
{
	static const int Katakana[] = {
		0x2121, 0x2123, 0x2156, 0x2157, 0x2122, 0x2126, 0x2572, 0x2521, 
		0x2523, 0x2525, 0x2527, 0x2529, 0x2563, 0x2565, 0x2567, 0x2543, 
		0x213C, 0x2522, 0x2524, 0x2526, 0x2528, 0x252A, 0x252B, 0x252D, 
		0x252F, 0x2531, 0x2533, 0x2535, 0x2537, 0x2539, 0x253B, 0x253D, 
		0x253F, 0x2541, 0x2544, 0x2546, 0x2548, 0x254A, 0x254B, 0x254C, 
		0x254D, 0x254E, 0x254F, 0x2552, 0x2555, 0x2558, 0x255B, 0x255E, 
		0x255F, 0x2560, 0x2561, 0x2562, 0x2564, 0x2566, 0x2568, 0x2569, 
		0x256A, 0x256B, 0x256C, 0x256D, 0x256F, 0x2573, 0x212B, 0x212C
	};

	return(Katakana[(uchar)Ch - (uchar)0xA0]);
}


/*----- 濁音／半濁音になる文字かチェック --------------------------------------
*
*	Parameter
*		char Ch : １バイトカタカナコード
*		char Daku : 濁点／半濁点
*
*	Return Value
*		int 文字コードに加える値 (0=濁音／半濁音にならない)
*----------------------------------------------------------------------------*/

static int AskDakuon(char Ch, char Daku)
{
	int Ret;

	Ret = 0;
	if((uchar)Daku == (uchar)0xDE)
	{
		if((((uchar)Ch >= (uchar)0xB6) && ((uchar)Ch <= (uchar)0xC4)) ||
		   (((uchar)Ch >= (uchar)0xCA) && ((uchar)Ch <= (uchar)0xCE)))
		{
			Ret = 1;
		}
	}
	else if((uchar)Daku == (uchar)0xDF)
	{
		if(((uchar)Ch >= (uchar)0xCA) && ((uchar)Ch <= (uchar)0xCE))
		{
			Ret = 2;
		}
	}
	return(Ret);
}












/*----- 文字列の漢字コードを調べ、Shift-JISに変換 -----------------------------
*
*	Parameter
*		char *Text : 文字列
*		int Pref : SJIS/EUCの優先指定
＊			KANJI_SJIS / KANJI_EUC / KANJI_NOCNV=SJIS/EUCのチェックはしない
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ConvAutoToSJIS(char *Text, int Pref)
{
	int Code;
	char *Buf;
	CODECONVINFO cInfo;

	Code = CheckKanjiCode(Text, strlen(Text), Pref);
	if(Code != KANJI_SJIS)
	{
		Buf = malloc(strlen(Text)+1);
		if(Buf != NULL)
		{
			InitCodeConvInfo(&cInfo);
			cInfo.KanaCnv = NO;
			cInfo.Str = Text;
			cInfo.StrLen = strlen(Text);
			cInfo.Buf = Buf;
			cInfo.BufSize = strlen(Text);

			switch(Code)
			{
				case KANJI_JIS :
					ConvJIStoSJIS(&cInfo);
					break;

				case KANJI_EUC :
					ConvEUCtoSJIS(&cInfo);
					break;
			}

			*(Buf + cInfo.OutLen) = NUL;
			strcpy(Text, Buf);
			free(Buf);
		}
	}
	return;
}


/*----- 使われている漢字コードを調べる ----------------------------------------
*
*	Parameter
*		char *Text : 文字列
*		int Size : 文字列の長さ
*		int Pref : SJIS/EUCの優先指定
＊			KANJI_SJIS / KANJI_EUC / KANJI_NOCNV=SJIS/EUCのチェックはしない
*
*	Return Value
*		int 漢字コード (KANJI_xxx)
*----------------------------------------------------------------------------*/

int CheckKanjiCode(char *Text, int Size, int Pref)
{
	uchar *Pos;
	uchar *Btm;
	int Ret;
	int PointSJIS;
	int PointEUC;

	Ret = KANJI_SJIS;
	if(Size >= 2)
	{
		Ret = -1;
		Btm = Text + Size;

		/* JIS漢字コードのチェック */
		Pos = Text;
		while((Pos = memchr(Pos, 0x1b, Btm-Pos-2)) != NULL)
		{
			Pos++;
			if((memcmp(Pos, "$B", 2) == 0) ||	/* <ESC>$B */
			   (memcmp(Pos, "$@", 2) == 0) ||	/* <ESC>$@ */
			   (memcmp(Pos, "(I", 2) == 0))		/* <ESC>(I */
			{
				Ret = KANJI_JIS;
				break;
			}
		}

		/* EUCとSHIFT-JIS漢字コードのチェック */
		if(Ret == -1)
		{
			if(Pref != KANJI_NOCNV)
			{
				Ret = Pref;
				Pos = Text;
				while(Pos < Btm)
				{
					PointSJIS = CheckOnSJIS(Pos, Btm);
					PointEUC = CheckOnEUC(Pos, Btm);
					if(PointSJIS > PointEUC)
					{
						Ret = KANJI_SJIS;
						break;
					}
					if(PointSJIS < PointEUC)
					{
						Ret = KANJI_EUC;
						break;
					}
					if((Pos = memchr(Pos, '\n', Btm-Pos)) == NULL)
						break;
					Pos++;
				}
			}
			else
				Ret = KANJI_SJIS;
		}
	}
	return(Ret);
}


/*----- SHIFT-JISコードの可能性があるかチェック --------------------------------
*
*	Parameter
*		uchar *Pos : 文字列
*		uchar *Btm : 文字列の末尾
*
*	Return Value
*		int 得点
*
*	Note
*		High	81-FF (A0-DFは半角)	(EB以降はほとんど無い)
*		Low		40-FC
*----------------------------------------------------------------------------*/

static int CheckOnSJIS(uchar *Pos, uchar *Btm)
{
	int FstOnTwo;
	int Point;

	FstOnTwo = NO;
	Point = 100;
	while((Point > 0) && (Pos < Btm) && (*Pos != '\n'))
	{
		if(FstOnTwo == YES)
		{
			if((*Pos < 0x40) || (*Pos > 0xFC))	/* 2バイト目は 0x40～0xFC */
				Point = 0;
			FstOnTwo = NO;
		}
		else if(*Pos >= 0x81)
		{
			if((*Pos < 0xA0) || (*Pos > 0xDF))	/* 半角カナでなければ */
			{
				if(*Pos >= 0xEB)		/* 1バイト目は0xEB以降はほとんど無い */
					Point -= 50;
				FstOnTwo = YES;
			}
		}
		Pos++;
	}
	if(FstOnTwo == YES)		/* １バイト目で終わっているのはおかしい  */
		Point = 0;

	return(Point);
}


/*----- EUCコードの可能性があるかチェック -------------------------------------
*
*	Parameter
*		uchar *Pos : 文字列
*		uchar *Btm : 文字列の末尾
*
*	Return Value
*		int 得点
*
*	Note
*		High	A1-FE , 8E
*		Low		A1-FE
*----------------------------------------------------------------------------*/

static int CheckOnEUC(uchar *Pos, uchar *Btm)
{
	int FstOnTwo;
	int Point;

	FstOnTwo = 0;
	Point = 100;
	while((Point > 0) && (Pos < Btm) && (*Pos != '\n'))
	{
		if(FstOnTwo == 1)
		{
			if((*Pos < 0xA1) || (*Pos > 0xFE))	/* 2バイト目は 0xA1～0xFE */
				Point = 0;
			FstOnTwo = 0;
		}
		else if(FstOnTwo == 2)		/* 半角カナ */
		{
			if((*Pos < 0xA0) || (*Pos > 0xDF))	/* 2バイト目は 0xA0～0xDF */
				Point = 0;
			FstOnTwo = 0;
		}
		else
		{
			if(*Pos == 0x8E)		/* 0x8E??は半角カナ */
				FstOnTwo = 2;
			else if((*Pos >= 0xA1) && (*Pos <= 0xFE))
				FstOnTwo = 1;
		}
		Pos++;
	}
	if(FstOnTwo != 0)		/* １バイト目で終わっているのはおかしい  */
		Point = 0;

	return(Point);
}


// UTF-8対応 ここから↓
/*----- UTF-8漢字コードをSHIFT-JIS漢字コードに変換 ------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*----------------------------------------------------------------------------*/

// UTF-8対応
// UTF-8からShift_JISへの変換後のバイト列が確定可能な長さを変換後の長さで返す
// バイナリ            UTF-8       戻り値 Shift_JIS
// E3 81 82 E3 81 84   あい     -> 2      82 A0   あ+結合文字の先頭バイトの可能性（い゛等）
// E3 81 82 E3 81      あ+E3 81 -> 0              結合文字の先頭バイトの可能性
// E3 81 82 E3         あ+E3    -> 0              結合文字の先頭バイトの可能性
// E3 81 82            あ       -> 0              結合文字の先頭バイトの可能性
int ConvUTF8NtoSJIS_TruncateToDelimiter(char* pUTF8, int UTF8Length, int* pNewUTF8Length)
{
	int UTF16Length;
	wchar_t* pUTF16;
	int SJISLength;
	int NewSJISLength;
	int NewUTF16Length;
	// UTF-8の場合、不完全な文字は常に変換されない
	// バイナリ            UTF-8       バイナリ      UTF-16 LE
	// E3 81 82 E3 81 84   あい     -> 42 30 44 30   あい
	// E3 81 82 E3 81      あ+E3 81 -> 42 30         あ
	// E3 81 82 E3         あ+E3    -> 42 30         あ
	UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, NULL, 0);
	if(!(pUTF16 = (wchar_t*)malloc(sizeof(wchar_t) * UTF16Length)))
		return -1;
	// Shift_JISへ変換した時に文字数が増減する位置がUnicode結合文字の区切り
	UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, pUTF16, UTF16Length);
	SJISLength = WideCharToMultiByte(CP_ACP, 0, pUTF16, UTF16Length, NULL, 0, NULL, NULL);
	NewSJISLength = SJISLength;
	while(UTF8Length > 0 && NewSJISLength >= SJISLength)
	{
		UTF8Length--;
		UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, pUTF16, UTF16Length);
		NewSJISLength = WideCharToMultiByte(CP_ACP, 0, pUTF16, UTF16Length, NULL, 0, NULL, NULL);
	}
	free(pUTF16);
	// UTF-16 LE変換した時に文字数が増減する位置がUTF-8の区切り
	if(pNewUTF8Length)
	{
		NewUTF16Length = UTF16Length;
		while(UTF8Length > 0 && NewUTF16Length >= UTF16Length)
		{
			UTF8Length--;
			NewUTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, NULL, 0);
		}
		if(UTF16Length > 0)
			UTF8Length++;
		*pNewUTF8Length = UTF8Length;
	}
	return NewSJISLength;
}

int ConvUTF8NtoSJIS(CODECONVINFO *cInfo)
{
	int Continue;

//	char temp_string[2048];
//	int string_length;

	// 大きいサイズに対応
	// 終端のNULLを含むバグを修正
	int SrcLength;
	char* pSrc;
	wchar_t* pUTF16;
	int UTF16Length;

	Continue = NO;

	// 生成される中間コードのサイズを調べる
//	string_length = MultiByteToWideChar(
//						CP_UTF8,		// 変換先文字コード
//						0,				// フラグ(0:なし)
//						cInfo->Str,		// 変換元文字列
//						-1,				// 変換元文字列バイト数(-1:自動)
//						NULL,			// 変換した文字列の格納先
//						0				// 格納先サイズ
//					);
	// 前回の変換不能な残りの文字列を入力の先頭に結合
	SrcLength = cInfo->StrLen + cInfo->EscUTF8Len;
	if(!(pSrc = (char*)malloc(sizeof(char) * (SrcLength + 1))))
	{
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}
	memcpy(pSrc, cInfo->EscUTF8, sizeof(char) * cInfo->EscUTF8Len);
	memcpy(pSrc + cInfo->EscUTF8Len, cInfo->Str, sizeof(char) * cInfo->StrLen);
	*(pSrc + SrcLength) = '\0';
	if(cInfo->EscFlush == NO)
	{
		// バッファに収まらないため変換文字数を半減
		while(SrcLength > 0 && ConvUTF8NtoSJIS_TruncateToDelimiter(pSrc, SrcLength, &SrcLength) > cInfo->BufSize)
		{
			SrcLength = SrcLength / 2;
		}
	}
	UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pSrc, SrcLength, NULL, 0);

	// サイズ0 or バッファサイズより大きい場合は
	// cInfo->Bufの最初に'\0'を入れて、
	// cInfo->BufSizeに0を入れて返す。
//	if( string_length == 0 ||
//		string_length >= 1024 ){
//		*(cInfo->Buf) = '\0';
//		cInfo->BufSize = 0;
//		return(Continue);
//	}
	if(!(pUTF16 = (wchar_t*)malloc(sizeof(wchar_t) * UTF16Length)))
	{
		free(pSrc);
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}

	// 中間コード(unicode)に変換
//	MultiByteToWideChar(
//		CP_UTF8,						// 変換先文字コード
//		0,								// フラグ(0:なし)
//		cInfo->Str,						// 変換元文字列
//		-1,								// 変換元文字列バイト数(-1:自動)
//		(unsigned short *)temp_string,	// 変換した文字列の格納先
//		1024							// 格納先サイズ
//	);
	MultiByteToWideChar(CP_UTF8, 0, pSrc, SrcLength, pUTF16, UTF16Length);

	// 生成されるUTF-8コードのサイズを調べる
//	string_length = WideCharToMultiByte(
//						CP_ACP,			// 変換先文字コード
//						0,				// フラグ(0:なし)
//						(unsigned short *)temp_string,	// 変換元文字列
//						-1,				// 変換元文字列バイト数(-1:自動)
//						NULL,			// 変換した文字列の格納先
//						0,				// 格納先サイズ
//						NULL,NULL
//					);

	// サイズ0 or 出力バッファサイズより大きい場合は、
	// cInfo->Bufの最初に'\0'を入れて、
	// cInfo->BufSizeに0を入れて返す。
//	if( string_length == 0 ||
//		string_length >= cInfo->BufSize ){
//		*(cInfo->Buf) = '\0';
//		cInfo->BufSize = 0;
//		return(Continue);
//	}

	// 出力サイズを設定
//	cInfo->OutLen = string_length;

	// UTF-8コードに変換
//	WideCharToMultiByte(
//		CP_ACP,							// 変換先文字コード
//		0,								// フラグ(0:なし)
//		(unsigned short *)temp_string,	// 変換元文字列
//		-1,								// 変換元文字列バイト数(-1:自動)
//		cInfo->Buf,						// 変換した文字列の格納先(BOM:3bytes)
//		cInfo->BufSize,					// 格納先サイズ
//		NULL,NULL
//	);
	cInfo->OutLen = WideCharToMultiByte(CP_ACP, 0, pUTF16, UTF16Length, cInfo->Buf, cInfo->BufSize, NULL, NULL);
	cInfo->Str += SrcLength - cInfo->EscUTF8Len;
	cInfo->StrLen -= SrcLength - cInfo->EscUTF8Len;
	cInfo->EscUTF8Len = 0;
	if(ConvUTF8NtoSJIS_TruncateToDelimiter(cInfo->Str, cInfo->StrLen, NULL) > 0)
		Continue = YES;
	else
	{
		// 変換不能なため次の入力の先頭に結合
		memcpy(cInfo->EscUTF8, cInfo->Str, sizeof(char) * cInfo->StrLen);
		cInfo->EscUTF8Len = cInfo->StrLen;
		cInfo->Str += cInfo->StrLen;
		cInfo->StrLen = 0;
		cInfo->FlushProc = ConvUTF8NtoSJIS;
		Continue = NO;
	}

	free(pSrc);
	free(pUTF16);

	return(Continue);
}

/*----- SHIFT-JIS漢字コードをUTF-8漢字コードに変換 ------------------------------
*
*	Parameter
*		CODECONVINFO *cInfo : 漢字コード変換情報
*
*	Return Value
*		int くり返しフラグ (YES/NO)
*
*	Note
*		くり返しフラグがYESの時は、cInfoの内容を変えずにもう一度呼ぶこと
*----------------------------------------------------------------------------*/
int ConvSJIStoUTF8N(CODECONVINFO *cInfo)
{
	int Continue;

//	char temp_string[2048];
	int string_length;

	// 大きいサイズに対応
	// 終端のNULLを含むバグを修正
	int SrcLength;
	char* pSrc;
	wchar_t* pUTF16;
	int UTF16Length;
	int Count;

	Continue = NO;

	// 生成される中間コードのサイズを調べる
//	string_length = MultiByteToWideChar(
//						CP_ACP,			// 変換先文字コード
//						0,				// フラグ(0:なし)
//						cInfo->Str,		// 変換元文字列
//						-1,				// 変換元文字列バイト数(-1:自動)
//						NULL,			// 変換した文字列の格納先
//						0				// 格納先サイズ
//					);
	// 前回の変換不能な残りの文字列を入力の先頭に結合
	SrcLength = cInfo->StrLen + cInfo->EscUTF8Len;
	if(!(pSrc = (char*)malloc(sizeof(char) * (SrcLength + 1))))
	{
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}
	memcpy(pSrc, cInfo->EscUTF8, sizeof(char) * cInfo->EscUTF8Len);
	memcpy(pSrc + cInfo->EscUTF8Len, cInfo->Str, sizeof(char) * cInfo->StrLen);
	*(pSrc + SrcLength) = '\0';
	if(cInfo->EscFlush == NO)
	{
		// Shift_JISの場合、不完全な文字でも変換されることがあるため、末尾の不完全な部分を削る
		Count = 0;
		while(Count < SrcLength)
		{
			if(((unsigned char)*(pSrc + Count) >= 0x81 && (unsigned char)*(pSrc + Count) <= 0x9f) || (unsigned char)*(pSrc + Count) >= 0xe0)
			{
				if((unsigned char)*(pSrc + Count + 1) >= 0x40)
					Count += 2;
				else
				{
					if(Count + 2 > SrcLength)
						break;
					Count += 1;
				}
			}
			else
				Count += 1;
		}
		SrcLength = Count;
	}
	UTF16Length = MultiByteToWideChar(CP_ACP, 0, pSrc, SrcLength, NULL, 0);

	// サイズ0 or バッファサイズより大きい場合は、
	// cInfo->Bufの最初に'\0'を入れて、
	// cInfo->BufSizeに0を入れて返す。
//	if( string_length == 0 ||
//		string_length >= 1024 ){
//		*(cInfo->Buf) = '\0';
//		cInfo->BufSize = 0;
//		return(Continue);
//	}
	if(!(pUTF16 = (wchar_t*)malloc(sizeof(wchar_t) * UTF16Length)))
	{
		free(pSrc);
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}

	// 中間コード(unicode)に変換
//	MultiByteToWideChar(
//		CP_ACP,							// 変換先文字コード
//		0,								// フラグ(0:なし)
//		cInfo->Str,						// 変換元文字列
//		-1,								// 変換元文字列バイト数(-1:自動)
//		(unsigned short *)temp_string,	// 変換した文字列の格納先
//		1024							// 格納先サイズ
//	);
	MultiByteToWideChar(CP_ACP, 0, pSrc, SrcLength, pUTF16, UTF16Length);

	// 生成されるUTF-8コードのサイズを調べる
//	string_length = WideCharToMultiByte(
//						CP_UTF8,		// 変換先文字コード
//						0,				// フラグ(0:なし)
//						(unsigned short *)temp_string,	// 変換元文字列
//						-1,				// 変換元文字列バイト数(-1:自動)
//						NULL,			// 変換した文字列の格納先
//						0,				// 格納先サイズ
//						NULL,NULL
//					);
	string_length = WideCharToMultiByte(CP_UTF8, 0, pUTF16, UTF16Length, NULL, 0, NULL, NULL);

	// サイズ0 or 出力バッファサイズより大きい場合は、
	// cInfo->Bufの最初に'\0'を入れて、
	// cInfo->BufSizeに0を入れて返す。
//	if( string_length == 0 ||
//		string_length >= cInfo->BufSize ){
//		*(cInfo->Buf) = '\0';
//		cInfo->BufSize = 0;
//		return(Continue);
//	}

	// 出力サイズを設定
//	cInfo->OutLen = string_length;

	/*
	// ↓付けちゃだめ コマンドにも追加されてしまう
	// 出力文字列の先頭にBOM(byte order mark)をつける
	*(cInfo->Buf) = (char)0xef;
	*(cInfo->Buf+1) = (char)0xbb;
	*(cInfo->Buf+2) = (char)0xbf;
	*/

	// UTF-8コードに変換
//	WideCharToMultiByte(
//		CP_UTF8,						// 変換先文字コード
//		0,								// フラグ(0:なし)
//		(unsigned short *)temp_string,	// 変換元文字列
//		-1,								// 変換元文字列バイト数(-1:自動)
//		cInfo->Buf,					// 変換した文字列の格納先(BOM:3bytes)
//		cInfo->BufSize,					// 格納先サイズ
//		NULL,NULL
//	);
	cInfo->OutLen = WideCharToMultiByte(CP_UTF8, 0, pUTF16, UTF16Length, cInfo->Buf, cInfo->BufSize, NULL, NULL);
	// バッファに収まらないため変換文字数を半減
	while(cInfo->OutLen == 0 && UTF16Length > 0)
	{
		UTF16Length = UTF16Length / 2;
		cInfo->OutLen = WideCharToMultiByte(CP_UTF8, 0, pUTF16, UTF16Length, cInfo->Buf, cInfo->BufSize, NULL, NULL);
	}
	// 変換された元の文字列での文字数を取得
	Count = WideCharToMultiByte(CP_ACP, 0, pUTF16, UTF16Length, NULL, 0, NULL, NULL);
	// 変換可能な残りの文字数を取得
	UTF16Length = MultiByteToWideChar(CP_ACP, 0, pSrc + Count, SrcLength - Count, NULL, 0);
	cInfo->Str += Count - cInfo->EscUTF8Len;
	cInfo->StrLen -= Count - cInfo->EscUTF8Len;
	cInfo->EscUTF8Len = 0;
	if(UTF16Length > 0)
		Continue = YES;
	else
	{
		// 変換不能なため次の入力の先頭に結合
		memcpy(cInfo->EscUTF8, cInfo->Str, sizeof(char) * cInfo->StrLen);
		cInfo->EscUTF8Len = cInfo->StrLen;
		cInfo->Str += cInfo->StrLen;
		cInfo->StrLen = 0;
		cInfo->FlushProc = ConvSJIStoUTF8N;
		Continue = NO;
	}

	free(pSrc);
	free(pUTF16);

	return(Continue);
}
// UTF-8対応 ここまで↑

// UTF-8 HFS+対応
int ConvUTF8NtoUTF8HFSX(CODECONVINFO *cInfo)
{
	int Continue;
	int SrcLength;
	char* pSrc;
	char* pSrcCur;
	char* pSrcEnd;
	char* pSrcNext;
	char* pDstCur;
	char* pDstEnd;
	DWORD Code;
	int Count;
	wchar_t Temp1[4];
	wchar_t Temp2[4];
	char Temp3[16];
	char* Temp3Cur;
	char* Temp3End;
	int TempCount;
	if(IsUnicodeNormalizationDllLoaded() == NO)
		return ConvNoConv(cInfo);
	Continue = NO;
	SrcLength = cInfo->StrLen + cInfo->EscUTF8Len;
	if(!(pSrc = (char*)malloc(sizeof(char) * (SrcLength + 1))))
	{
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}
	memcpy(pSrc, cInfo->EscUTF8, sizeof(char) * cInfo->EscUTF8Len);
	memcpy(pSrc + cInfo->EscUTF8Len, cInfo->Str, sizeof(char) * cInfo->StrLen);
	*(pSrc + SrcLength) = '\0';
	cInfo->OutLen = 0;
	pSrcCur = pSrc;
	pSrcEnd = pSrc + SrcLength;
	pSrcNext = pSrc;
	pDstCur = cInfo->Buf;
	pDstEnd = cInfo->Buf + cInfo->BufSize;
	while(pSrcCur < pSrcEnd)
	{
		Code = GetNextCharM(pSrcCur, pSrcEnd, (LPCSTR*)&pSrcNext);
		if(Code == 0x80000000)
		{
			if(pSrcNext == pSrcEnd)
				// 入力の末尾が不完全
				break;
		}
		else if((Code >= 0x00002000 && Code <= 0x00002fff)
			|| (Code >= 0x0000f900 && Code <= 0x0000faff)
			|| (Code >= 0x0002f800 && Code <= 0x0002faff))
		{
			// HFS+特有の例外
			Count = PutNextCharM(pDstCur, pDstEnd, &pDstCur, Code);
			if(Count > 0)
				cInfo->OutLen += Count;
			else
			{
				// 出力バッファが不足
				Continue = YES;
				break;
			}
		}
		else
		{
			// Normalization Form Dに変換
			Count = MultiByteToWideChar(CP_UTF8, 0, pSrcCur, (int)(pSrcNext - pSrcCur), Temp1, 4);
			Count = p_NormalizeString(NormalizationD, Temp1, Count, Temp2, 4);
			Count = WideCharToMultiByte(CP_UTF8, 0, Temp2, Count, Temp3, 16, NULL, NULL);
			Temp3Cur = Temp3;
			Temp3End = Temp3 + Count;
			TempCount = 0;
			while(Temp3Cur < Temp3End)
			{
				Code = GetNextCharM(Temp3Cur, Temp3End, (LPCSTR*)&Temp3Cur);
				Count = PutNextCharM(pDstCur, pDstEnd, &pDstCur, Code);
				if(Count > 0)
					TempCount += Count;
				else
				{
					// 出力バッファが不足
					Continue = YES;
					break;
				}
			}
			cInfo->OutLen += TempCount;
		}
		pSrcCur = pSrcNext;
	}
	cInfo->Str += (int)(pSrcCur - pSrc) - cInfo->EscUTF8Len;
	cInfo->StrLen -= (int)(pSrcCur - pSrc) - cInfo->EscUTF8Len;
	cInfo->EscUTF8Len = 0;
	free(pSrc);
	if(Continue == NO)
	{
		memcpy(cInfo->EscUTF8, cInfo->Str, sizeof(char) * cInfo->StrLen);
		cInfo->EscUTF8Len = cInfo->StrLen;
		cInfo->Str += cInfo->StrLen;
		cInfo->StrLen = 0;
		cInfo->FlushProc = ConvUTF8NtoUTF8HFSX;
	}
	return YES;
}

// バグの可能性あり
// 確認するまで複数個のバッファを用いた変換には用いないこと
// UTF-8 Nomalization Form DからCへの変換後のバイト列が確定可能な長さを変換後のコードポイントの個数で返す
// バイナリ            UTF-8       戻り値
// E3 81 82 E3 81 84   あい     -> 1   あ+結合文字の先頭バイトの可能性（い゛等）
// E3 81 82 E3 81      あ+E3 81 -> 0   結合文字の先頭バイトの可能性
// E3 81 82 E3         あ+E3    -> 0   結合文字の先頭バイトの可能性
// E3 81 82            あ       -> 0   結合文字の先頭バイトの可能性
int ConvUTF8HFSXtoUTF8N_TruncateToDelimiter(char* pUTF8, int UTF8Length, int* pNewUTF8Length)
{
	int UTF16Length;
	wchar_t* pUTF16;
	int UTF16HFSXLength;
	wchar_t* pUTF16HFSX;
	int CodeCount;
	int NewCodeCount;
	int NewUTF16Length;
	UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, NULL, 0);
	if(!(pUTF16 = (wchar_t*)malloc(sizeof(wchar_t) * UTF16Length)))
		return -1;
	UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, pUTF16, UTF16Length);
	UTF16HFSXLength = p_NormalizeString(NormalizationC, pUTF16, UTF16Length, NULL, 0);
	if(!(pUTF16HFSX = (wchar_t*)malloc(sizeof(wchar_t) * UTF16HFSXLength)))
	{
		free(pUTF16);
		return -1;
	}
	UTF16HFSXLength = p_NormalizeString(NormalizationC, pUTF16, UTF16Length, pUTF16HFSX, UTF16HFSXLength);
	// 変換した時にコードポイントの個数が増減する位置がUnicode結合文字の区切り
	CodeCount = GetCodeCountW(pUTF16HFSX, UTF16HFSXLength);
	NewCodeCount = CodeCount;
	while(UTF8Length > 0 && NewCodeCount >= CodeCount)
	{
		UTF8Length--;
		UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, pUTF16, UTF16Length);
		UTF16HFSXLength = p_NormalizeString(NormalizationC, pUTF16, UTF16Length, pUTF16HFSX, UTF16HFSXLength);
		NewCodeCount = GetCodeCountW(pUTF16HFSX, UTF16HFSXLength);
	}
	free(pUTF16);
	free(pUTF16HFSX);
	// UTF-16 LE変換した時に文字数が増減する位置がUTF-8の区切り
	if(pNewUTF8Length)
	{
		NewUTF16Length = UTF16Length;
		while(UTF8Length > 0 && NewUTF16Length >= UTF16Length)
		{
			UTF8Length--;
			NewUTF16Length = MultiByteToWideChar(CP_UTF8, 0, pUTF8, UTF8Length, NULL, 0);
		}
		if(UTF16Length > 0)
			UTF8Length++;
		*pNewUTF8Length = UTF8Length;
	}
	return NewCodeCount;
}

int ConvUTF8HFSXtoUTF8N(CODECONVINFO *cInfo)
{
	int Continue;
	int SrcLength;
	char* pSrc;
	int UTF16Length;
	wchar_t* pUTF16;
	int UTF16HFSXLength;
	wchar_t* pUTF16HFSX;
	CODECONVINFO Temp;
	int Count;
	if(IsUnicodeNormalizationDllLoaded() == NO)
		return ConvNoConv(cInfo);
	Continue = NO;
	// 前回の変換不能な残りの文字列を入力の先頭に結合
	SrcLength = cInfo->StrLen + cInfo->EscUTF8Len;
	if(!(pSrc = (char*)malloc(sizeof(char) * (SrcLength + 1))))
	{
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}
	memcpy(pSrc, cInfo->EscUTF8, sizeof(char) * cInfo->EscUTF8Len);
	memcpy(pSrc + cInfo->EscUTF8Len, cInfo->Str, sizeof(char) * cInfo->StrLen);
	*(pSrc + SrcLength) = '\0';
	UTF16Length = MultiByteToWideChar(CP_UTF8, 0, pSrc, SrcLength, NULL, 0);
	if(!(pUTF16 = (wchar_t*)malloc(sizeof(wchar_t) * UTF16Length)))
	{
		free(pSrc);
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}
	MultiByteToWideChar(CP_UTF8, 0, pSrc, SrcLength, pUTF16, UTF16Length);
	UTF16HFSXLength = p_NormalizeString(NormalizationC, pUTF16, UTF16Length, NULL, 0);
	if(!(pUTF16HFSX = (wchar_t*)malloc(sizeof(wchar_t) * UTF16HFSXLength)))
	{
		free(pSrc);
		free(pUTF16);
		*(cInfo->Buf) = '\0';
		cInfo->BufSize = 0;
		return Continue;
	}
	UTF16HFSXLength = p_NormalizeString(NormalizationC, pUTF16, UTF16Length, pUTF16HFSX, UTF16HFSXLength);
	cInfo->OutLen = WideCharToMultiByte(CP_UTF8, 0, pUTF16HFSX, UTF16HFSXLength, cInfo->Buf, cInfo->BufSize, NULL, NULL);
	if(cInfo->OutLen == 0 && UTF16HFSXLength > 0)
	{
		// バッファに収まらないため変換文字数を半減
		Temp = *cInfo;
		Temp.StrLen = cInfo->StrLen / 2;
		ConvUTF8HFSXtoUTF8N(&Temp);
		cInfo->OutLen = Temp.OutLen;
		Count = cInfo->StrLen / 2 + cInfo->EscUTF8Len - Temp.StrLen - Temp.EscUTF8Len;
		cInfo->Str += Count - cInfo->EscUTF8Len;
		cInfo->StrLen -= Count - cInfo->EscUTF8Len;
		cInfo->EscUTF8Len = 0;
	}
	else
	{
		cInfo->Str += SrcLength - cInfo->EscUTF8Len;
		cInfo->StrLen -= SrcLength - cInfo->EscUTF8Len;
		cInfo->EscUTF8Len = 0;
	}
	if(ConvUTF8HFSXtoUTF8N_TruncateToDelimiter(cInfo->Str, cInfo->StrLen, NULL) > 0)
		Continue = YES;
	else
	{
		// 変換不能なため次の入力の先頭に結合
		memcpy(cInfo->EscUTF8, cInfo->Str, sizeof(char) * cInfo->StrLen);
		cInfo->EscUTF8Len = cInfo->StrLen;
		cInfo->Str += cInfo->StrLen;
		cInfo->StrLen = 0;
		cInfo->FlushProc = ConvUTF8HFSXtoUTF8N;
		Continue = NO;
	}
	free(pSrc);
	free(pUTF16);
	free(pUTF16HFSX);
	return Continue;
}


/*----- IBM拡張漢字をNEC選定IBM拡張漢字等に変換 -------------------------------
*
*	Parameter
*		code	漢字コード
*
*	Return Value
*		int 漢字コード
*----------------------------------------------------------------------------*/
static int ConvertIBMExtendedChar(int code)
{
	if((code >= 0xfa40) && (code <= 0xfa49))		code -= (0xfa40 - 0xeeef);
	else if((code >= 0xfa4a) && (code <= 0xfa53))	code -= (0xfa4a - 0x8754);
	else if((code >= 0xfa54) && (code <= 0xfa57))	code -= (0xfa54 - 0xeef9);
	else if(code == 0xfa58)							code = 0x878a;
	else if(code == 0xfa59)							code = 0x8782;
	else if(code == 0xfa5a)							code = 0x8784;
	else if(code == 0xfa5b)							code = 0x879a;
	else if((code >= 0xfa5c) && (code <= 0xfa7e))	code -= (0xfa5c - 0xed40);
	else if((code >= 0xfa80) && (code <= 0xfa9b))	code -= (0xfa80 - 0xed63);
	else if((code >= 0xfa9c) && (code <= 0xfafc))	code -= (0xfa9c - 0xed80);
	else if((code >= 0xfb40) && (code <= 0xfb5b))	code -= (0xfb40 - 0xede1);
	else if((code >= 0xfb5c) && (code <= 0xfb7e))	code -= (0xfb5c - 0xee40);
	else if((code >= 0xfb80) && (code <= 0xfb9b))	code -= (0xfb80 - 0xee63);
	else if((code >= 0xfb9c) && (code <= 0xfbfc))	code -= (0xfb9c - 0xee80);
	else if((code >= 0xfc40) && (code <= 0xfc4b))	code -= (0xfc40 - 0xeee1);
	return code;
}

// UTF-8対応
int LoadUnicodeNormalizationDll()
{
	int Sts;
	char CurDir[FMAX_PATH+1];
	char SysDir[FMAX_PATH+1];
	Sts = FFFTP_FAIL;
	if(GetCurrentDirectory(FMAX_PATH, CurDir) > 0)
	{
		if(GetSystemDirectory(SysDir, FMAX_PATH) > 0)
		{
			if(SetCurrentDirectory(SysDir))
			{
				if((hUnicodeNormalizationDll = LoadLibrary("normaliz.dll")) != NULL)
				{
					if((p_NormalizeString = (_NormalizeString)GetProcAddress(hUnicodeNormalizationDll, "NormalizeString")) != NULL)
						Sts = FFFTP_SUCCESS;
				}
				SetCurrentDirectory(CurDir);
			}
		}
	}
	return Sts;
}

void FreeUnicodeNormalizationDll()
{
	if(hUnicodeNormalizationDll != NULL)
		FreeLibrary(hUnicodeNormalizationDll);
	hUnicodeNormalizationDll = NULL;
	p_NormalizeString = NULL;
}

int IsUnicodeNormalizationDllLoaded()
{
	int Sts;
	Sts = FFFTP_FAIL;
	if(hUnicodeNormalizationDll != NULL && p_NormalizeString != NULL)
		Sts = FFFTP_SUCCESS;
	return Sts;
}

