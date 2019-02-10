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

#include "common.h"


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

static int ConvertIBMExtendedChar(int code);


// 改行コードをCRLFに変換
std::string ToCRLF(std::string_view source) {
	static std::regex re{ R"(\r\n|[\r\n])" };
	std::string result;
	std::regex_replace(back_inserter(result), begin(source), end(source), re, "\r\n");
	return result;
}


static auto normalize(NORM_FORM form, std::wstring_view src) {
	// NormalizeString returns estimated buffer length.
	auto estimated = NormalizeString(form, data(src), size_as<int>(src), nullptr, 0);
	std::wstring normalized(estimated, L'\0');
	auto len = NormalizeString(form, data(src), size_as<int>(src), data(normalized), estimated);
	normalized.resize(len);
	return normalized;
}


static auto convert(std::string_view input, DWORD incp, DWORD outcp, void (*update)(std::wstring&)) {
	static auto mlang = LoadLibraryW(L"mlang.dll");
	static auto convertINetMultiByteToUnicode = reinterpret_cast<decltype(&ConvertINetMultiByteToUnicode)>(GetProcAddress(mlang, "ConvertINetMultiByteToUnicode"));
	static auto convertINetUnicodeToMultiByte = reinterpret_cast<decltype(&ConvertINetUnicodeToMultiByte)>(GetProcAddress(mlang, "ConvertINetUnicodeToMultiByte"));
	static DWORD mb2u = 0, u2mb = 0;
	static INT scale = 2;
	std::wstring wstr;
	for (;; ++scale) {
		auto inlen = size_as<INT>(input), wlen = inlen * scale;
		wstr.resize(wlen);
		auto hr = convertINetMultiByteToUnicode(&mb2u, incp, data(input), &inlen, data(wstr), &wlen);
		assert(hr == S_OK);
		if (inlen == size_as<INT>(input)) {
			wstr.resize(wlen);
			break;
		}
	}
	update(wstr);
	for (std::string output;; ++scale) {
		auto wlen = size_as<INT>(wstr), outlen = wlen * scale;
		output.resize(outlen);
		auto hr = convertINetUnicodeToMultiByte(&u2mb, outcp, data(wstr), &wlen, data(output), &outlen);
		assert(hr == S_OK);
		if (wlen == size_as<INT>(wstr)) {
			output.resize(outlen);
			return output;
		}
	}
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

	cInfo->OutLen = (int)(Put - cInfo->Buf);

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
	cInfo->OutLen = (int)(Put - cInfo->Buf);

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
	cInfo->OutLen = (int)(Put - cInfo->Buf);

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
	cInfo->OutLen = (int)(Put - cInfo->Buf);

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
	cInfo->OutLen = (int)(Put - cInfo->Buf);

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


void CodeDetector::Test(std::string_view str) {
	static std::regex reJISor8BIT{ R"(\x1B(\$[B@]|\([BHIJ]|\$\([DOPQ])|[\x80-\xFF])" },
		// Shift-JIS
		// JIS X 0208  1～8、16～47、48～84  81-84,88-9F,E0-EA
		// NEC特殊文字                       87
		// NEC選定IBM拡張文字                ED-EE
		// IBM拡張文字                       FA-FC
		reSJIS{ R"((?:[\x00-\x7F\xA1-\xDF]|(?:[\x81-\x84\x87-\x9F\xE0-\xEA\xED\xEE\xFA-\xFC]|([\x81-\x9F\xE0-\xFC]))[\x40-\x7E\x80-\xFC])*)" },
		reEUC{ R"((?:[\x00-\x7F]|\x8E[\xA1-\xDF]|(?:[\xA1-\xA8\xB0-\xF4]|(\x8F?[\xA1-\xFE]))[\xA1-\xFE])*)" },
		// UTF-8
		// U+000000-U+00007F                   0b0000000-                 0b1111111  00-7F
		// U+000080-U+0007FF              0b00010'000000-            0b11111'111111  C2-DF             80-BF
		// U+000800-U+000FFF        0b0000'100000'000000-      0b0000'111111'111111  E0    A0-BF       80-BF
		// U+001000-U+00FFFF        0b0001'000000'000000-      0b1111'111111'111111  E1-EF       80-BF 80-BF
		// U+003099-U+00309A        0b0011'000010'011001-      0b0011'000010'011010  E3    82    99-9A       (COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK, COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK)
		// U+00D800-U+00DFFF        0b1101'100000'000000-      0b1101'111111'111111  ED    A0-BF       80-BF (High Surrogate Area, Low Surrogate Area)
		// U+00E000-U+00EFFF        0b1110'000000'000000-      0b1110'111111'111111  EE          80-BF 80-BF (Private Use Area)
		// U+00F000-U+00F8FF        0b1111'000000'000000-      0b1111'100011'111111  EF    80-A3       80-BF (Private Use Area)
		// U+010000-U+03FFFF  0b000'010000'000000'000000-0b000'111111'111111'111111  F0    90-BF 80-BF 80-BF
		// U+040000-U+0FFFFF  0b001'000000'000000'000000-0b011'111111'111111'111111  F1-F3 80-BF 80-BF 80-BF
		// U+0F0000-U+0FFFFF  0b011'110000'000000'000000-0b011'111111'111111'111111  F3    B0-BF 80-BF 80-BF (Supplementary Private Use Area-A)
		// U+100000-U+10FFFF  0b100'000000'000000'000000-0b100'001111'111111'111111  F4    80-8F 80-BF 80-BF (Supplementary Private Use Area-B)
		reUTF8{ R"((?:[\x00-\x7F]|(\xE3\x82[\x99\x9A])|(?:(\xED[\xA0-\xBF]|\xEF[\x80-\xA3])|[\xC2-\xDF]|\xE0[\xA0-\xBF]|(?:(\xEE|\xF3[\xB0-\xBF]|\xF4[\x80-\x8F])|[\xE1-\xEF]|\xF0[\x90-\xBF]|[\xF1-\xF3][\x80-\xBF])[\x80-\xBF])[\x80-\xBF])*)" };
	if (std::match_results<std::string_view::const_iterator> m; std::regex_search(begin(str), end(str), m, reJISor8BIT)) {
		if (m[1].matched)
			jis += 2;
		else {
			if (std::match_results<std::string_view::const_iterator> m; std::regex_match(begin(str), end(str), m, reSJIS))
				sjis += m[1].matched ? 1 : 2;
			if (std::match_results<std::string_view::const_iterator> m; std::regex_match(begin(str), end(str), m, reEUC))
				euc += m[1].matched ? 1 : 2;
			if (std::match_results<std::string_view::const_iterator> m; std::regex_match(begin(str), end(str), m, reUTF8)) {
				utf8 += m[2].matched || m[3].matched ? 1 : 2;
				if (m[1].matched)
					hfsx = true;
			}
		}
	}
}


static void fullwidth(std::wstring& str) {
	static std::wregex re{ LR"([\uFF61-\uFF9F]+)" };
	str = replace<wchar_t>(str, re, [](auto const& m) { return normalize(NormalizationKC, { m[0].first, (size_t)m.length() }); });
}


static auto tohex(std::cmatch const& m) {
	static char hex[] = "0123456789abcdef";
	std::string converted;
	for (auto it = m[0].first; it != m[0].second; ++it) {
		unsigned char ch = *it;
		converted += ':';
		converted += hex[ch >> 4];
		converted += hex[ch & 15];
	}
	return converted;
}


std::string ConvertFrom(std::string_view str, int kanji) {
	switch (kanji) {
	case KANJI_SJIS:
		return convert(str, 932, CP_UTF8, [](auto) {});
	case KANJI_JIS:
		return convert(str, 50221, CP_UTF8, [](auto) {});
	case KANJI_EUC:
		return convert(str, 51932, CP_UTF8, [](auto) {});
	case KANJI_SMB_HEX:
	case KANJI_SMB_CAP: {
		static std::regex re{ R"(:([0-9A-Fa-f]{2}))" };
		auto decoded = replace(str, re, [](auto const& m) {
			char ch;
			std::from_chars(m[1].first, m[1].second, ch, 16);
			return ch;
		});
		return convert(decoded, 932, CP_UTF8, [](auto) {});
	}
	case KANJI_UTF8HFSX:
		return convert(str, CP_UTF8, CP_UTF8, [](auto& str) { str = normalize(NormalizationC, str); });
	case KANJI_UTF8N:
	default:
		return std::string(str);
	}
}


std::string ConvertTo(std::string_view str, int kanji, int kana) {
	switch (kanji) {
	case KANJI_SJIS:
		return convert(str, CP_UTF8, 932, [](auto) {});
	case KANJI_JIS:
		return convert(str, CP_UTF8, 50221, kana ? fullwidth : [](auto) {});
	case KANJI_EUC:
		return convert(str, CP_UTF8, 51932, kana ? fullwidth : [](auto) {});
	case KANJI_SMB_HEX: {
		auto sjis = convert(str, CP_UTF8, 932, [](auto) {});
		static std::regex re{ R"((?:[\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]|[\xA1-\xDF])+)" };
		return replace<char>(sjis, re, tohex);
	}
	case KANJI_SMB_CAP: {
		auto sjis = convert(str, CP_UTF8, 932, [](auto) {});
		static std::regex re{ R"([\x80-\xFF]+)" };
		return replace<char>(sjis, re, tohex);
	}
	case KANJI_UTF8HFSX:
		// TODO: fullwidth
		return convert(str, CP_UTF8, CP_UTF8, [](auto& str) {
			// HFS+のNFD対象外の範囲
			// U+02000–U+02FFF       2000-2FFF
			// U+0F900–U+0FAFF       F900-FAFF
			// U+2F800–U+2FAFF  D87E DC00-DEFF
			static std::wregex re{ LR"((.*?)((?:[\u2000-\u2FFF\uF900-\uFAFF]|\uD87E[\uDC00-\uDEFF])+|$))" };
			str = replace<wchar_t>(str, re, [](auto const& m) { return normalize(NormalizationD, { m[1].first, (size_t)m.length(1) }).append(m[2].first, m[2].second); });
		});
	case KANJI_UTF8N:
	default:
		return std::string(str);
	}
}
