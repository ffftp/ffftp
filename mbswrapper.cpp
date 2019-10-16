// mbswrapper.c
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー
// マルチバイト文字はUTF-8、ワイド文字はUTF-16であるものとする
// 全ての制御用の文字はASCIIの範囲であるため、Shift_JISとUTF-8間の変換は不要

#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <tchar.h>
#include <direct.h>
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <htmlhelp.h>

#define DO_NOT_REPLACE
#include "mbswrapper.h"

// マルチバイト文字列からワイド文字列へ変換
int MtoW(LPWSTR pDst, int size, LPCSTR pSrc, int count)
{
	if(pSrc < (LPCSTR)0x00010000 || pSrc == (LPCSTR)~0)
		return 0;
	if(pDst)
		return MultiByteToWideChar(CP_UTF8, 0, pSrc, count, pDst, size);
	return MultiByteToWideChar(CP_UTF8, 0, pSrc, count, NULL, 0);
}

// ワイド文字列からマルチバイト文字列へ変換
int WtoM(LPSTR pDst, int size, LPCWSTR pSrc, int count)
{
	if(pSrc < (LPCWSTR)0x00010000 || pSrc == (LPCWSTR)~0)
		return 0;
	if(pDst)
		return WideCharToMultiByte(CP_UTF8, 0, pSrc, count, pDst, size, NULL, NULL);
	return WideCharToMultiByte(CP_UTF8, 0, pSrc, count, NULL, 0, NULL, NULL);
}

// Shift_JIS文字列からワイド文字列へ変換
int AtoW(LPWSTR pDst, int size, LPCSTR pSrc, int count)
{
	if(pSrc < (LPCSTR)0x00010000 || pSrc == (LPCSTR)~0)
		return 0;
	if(pDst)
		return MultiByteToWideChar(CP_ACP, 0, pSrc, count, pDst, size);
	return MultiByteToWideChar(CP_ACP, 0, pSrc, count, NULL, 0);
}

// ワイド文字列からShift_JIS文字列へ変換
int WtoA(LPSTR pDst, int size, LPCWSTR pSrc, int count)
{
	if(pSrc < (LPCWSTR)0x00010000 || pSrc == (LPCWSTR)~0)
		return 0;
	if(pDst)
		return WideCharToMultiByte(CP_ACP, 0, pSrc, count, pDst, size, NULL, NULL);
	return WideCharToMultiByte(CP_ACP, 0, pSrc, count, NULL, 0, NULL, NULL);
}

// マルチバイト文字列バッファ終端を強制的にNULLで置換
int TerminateStringM(LPSTR lpString, int size)
{
	int i;
	if(lpString < (LPSTR)0x00010000 || lpString == (LPSTR)~0)
		return 0;
	for(i = 0; i < size; i++)
	{
		if(lpString[i] == '\0')
			return i;
	}
	i--;
	lpString[i] = '\0';
	return i;
}

// ワイド文字列バッファ終端を強制的にNULLで置換
int TerminateStringW(LPWSTR lpString, int size)
{
	int i;
	if(lpString < (LPWSTR)0x00010000 || lpString == (LPWSTR)~0)
		return 0;
	for(i = 0; i < size; i++)
	{
		if(lpString[i] == L'\0')
			return i;
	}
	i--;
	lpString[i] = L'\0';
	return i;
}

// Shift_JIS文字列バッファ終端を強制的にNULLで置換
int TerminateStringA(LPSTR lpString, int size)
{
	int i;
	if(lpString < (LPSTR)0x00010000 || lpString == (LPSTR)~0)
		return 0;
	for(i = 0; i < size; i++)
	{
		if(lpString[i] == '\0')
			return i;
	}
	i--;
	lpString[i] = '\0';
	return i;
}

// NULL区切り複数マルチバイト文字列の長さを取得
size_t GetMultiStringLengthM(LPCSTR lpString)
{
	size_t i;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return 0;
	i = 0;
	while(lpString[i] != '\0' || lpString[i + 1] != '\0')
	{
		i++;
	}
	i++;
	return i;
}

// NULL区切り複数ワイド文字列の長さを取得
size_t GetMultiStringLengthW(LPCWSTR lpString)
{
	size_t i;
	if(lpString < (LPCWSTR)0x00010000 || lpString == (LPCWSTR)~0)
		return 0;
	i = 0;
	while(lpString[i] != L'\0' || lpString[i + 1] != L'\0')
	{
		i++;
	}
	i++;
	return i;
}

// NULL区切り複数Shift_JIS文字列の長さを取得
size_t GetMultiStringLengthA(LPCSTR lpString)
{
	size_t i;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return 0;
	i = 0;
	while(lpString[i] != '\0' || lpString[i + 1] != '\0')
	{
		i++;
	}
	i++;
	return i;
}

// NULL区切りマルチバイト文字列からワイド文字列へ変換
int MtoWMultiString(LPWSTR pDst, int size, LPCSTR pSrc)
{
	int i;
	if(pSrc < (LPCSTR)0x00010000 || pSrc == (LPCSTR)~0)
		return 0;
	if(!pDst)
		return (int)GetMultiStringLengthM(pSrc);
	i = 0;
	while(*pSrc != '\0')
	{
		i += MultiByteToWideChar(CP_UTF8, 0, pSrc, -1, pDst + i, size - i - 1);
		pSrc += strlen(pSrc) + 1;
	}
	pDst[i] = L'\0';
	return i;
}

// NULL区切りワイド文字列からマルチバイト文字列へ変換
int WtoMMultiString(LPSTR pDst, int size, LPCWSTR pSrc)
{
	int i;
	if(pSrc < (LPCWSTR)0x00010000 || pSrc == (LPCWSTR)~0)
		return 0;
	if(!pDst)
		return (int)GetMultiStringLengthW(pSrc);
	i = 0;
	while(*pSrc != L'\0')
	{
		i += WideCharToMultiByte(CP_UTF8, 0, pSrc, -1, pDst + i, size - i - 1, NULL, NULL);
		pSrc += wcslen(pSrc) + 1;
	}
	pDst[i] = '\0';
	return i;
}

// NULL区切りShift_JIS文字列からワイド文字列へ変換
int AtoWMultiString(LPWSTR pDst, int size, LPCSTR pSrc)
{
	int i;
	if(pSrc < (LPCSTR)0x00010000 || pSrc == (LPCSTR)~0)
		return 0;
	if(!pDst)
		return (int)GetMultiStringLengthA(pSrc);
	i = 0;
	while(*pSrc != '\0')
	{
		i += MultiByteToWideChar(CP_ACP, 0, pSrc, -1, pDst + i, size - i - 1);
		pSrc += strlen(pSrc) + 1;
	}
	pDst[i] = L'\0';
	return i;
}

// NULL区切りワイド文字列からShift_JIS文字列へ変換
int WtoAMultiString(LPSTR pDst, int size, LPCWSTR pSrc)
{
	int i;
	if(pSrc < (LPCWSTR)0x00010000 || pSrc == (LPCWSTR)~0)
		return 0;
	if(!pDst)
		return (int)GetMultiStringLengthW(pSrc);
	i = 0;
	while(*pSrc != L'\0')
	{
		i += WideCharToMultiByte(CP_ACP, 0, pSrc, -1, pDst + i, size - i - 1, NULL, NULL);
		pSrc += wcslen(pSrc) + 1;
	}
	pDst[i] = '\0';
	return i;
}

// マルチバイト文字列用のメモリを確保
char* AllocateStringM(size_t size)
{
	char* p;
	// 0が指定される場合があるため1文字分追加
	p = (char*)malloc(sizeof(char) * (size + 1));
	// 念のため先頭にNULL文字を代入
	if(p)
		*p = '\0';
	return p;
}

// ワイド文字列用のメモリを確保
wchar_t* AllocateStringW(size_t size)
{
	wchar_t* p;
	// 0が指定される場合があるため1文字分追加
	p = (wchar_t*)malloc(sizeof(wchar_t) * (size + 1));
	// 念のため先頭にNULL文字を代入
	if(p)
		*p = L'\0';
	return p;
}

// Shift_JIS文字列用のメモリを確保
char* AllocateStringA(size_t size)
{
	char* p;
	// 0が指定される場合があるため1文字分追加
	p = (char*)malloc(sizeof(char) * (size + 1));
	// 念のため先頭にNULL文字を代入
	if(p)
		*p = '\0';
	return p;
}

// メモリを確保してマルチバイト文字列からワイド文字列へ変換
// リソースIDならば元の値を返す
wchar_t* DuplicateMtoW(LPCSTR lpString, int c)
{
	wchar_t* p;
	int i;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return (wchar_t*)lpString;
	if(c < 0)
		c = (int)strlen(lpString);
	p = AllocateStringW((size_t)MtoW(NULL, 0, lpString, c) + 1);
	if(p)
	{
		i = MtoW(p, 65535, lpString, c);
		p[i] = L'\0';
	}
	return p;
}

// 指定したサイズのメモリを確保してマルチバイト文字列からワイド文字列へ変換
// リソースIDならば元の値を返す
wchar_t* DuplicateMtoWBuffer(LPCSTR lpString, int c, int size)
{
	wchar_t* p;
	int i;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return (wchar_t*)lpString;
	if(c < 0)
		c = (int)strlen(lpString);
	p = AllocateStringW(size);
	if(p)
	{
		i = MtoW(p, size, lpString, c);
		p[i] = L'\0';
	}
	return p;
}

// メモリを確保してNULL区切りマルチバイト文字列からワイド文字列へ変換
// リソースIDならば元の値を返す
wchar_t* DuplicateMtoWMultiString(LPCSTR lpString)
{
	int count;
	wchar_t* p;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return (wchar_t*)lpString;
	count = (int)GetMultiStringLengthM(lpString) + 1;
	p = AllocateStringW(count);
	if(p)
		MtoW(p, count, lpString, count);
	return p;
}

// 指定したサイズのメモリを確保してNULL区切りマルチバイト文字列からワイド文字列へ変換
// リソースIDならば元の値を返す
wchar_t* DuplicateMtoWMultiStringBuffer(LPCSTR lpString, int size)
{
	int count;
	wchar_t* p;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return (wchar_t*)lpString;
	count = (int)GetMultiStringLengthM(lpString) + 1;
	p = AllocateStringW(size);
	if(p)
	{
		MtoW(p, size, lpString, count);
		p[size - 2] = L'\0';
		p[size - 1] = L'\0';
	}
	return p;
}

// メモリを確保してワイド文字列からマルチバイト文字列へ変換
// リソースIDならば元の値を返す
char* DuplicateWtoM(LPCWSTR lpString, int c)
{
	char* p;
	int i;
	if(lpString < (LPCWSTR)0x00010000 || lpString == (LPCWSTR)~0)
		return (char*)lpString;
	if(c < 0)
		c = (int)wcslen(lpString);
	p = AllocateStringM((size_t)WtoM(NULL, 0, lpString, c) + 1);
	if(p)
	{
		i = WtoM(p, 65535, lpString, c);
		p[i] = L'\0';
	}
	return p;
}

// メモリを確保してShift_JIS文字列からワイド文字列へ変換
// リソースIDならば元の値を返す
wchar_t* DuplicateAtoW(LPCSTR lpString, int c)
{
	wchar_t* p;
	int i;
	if(lpString < (LPCSTR)0x00010000 || lpString == (LPCSTR)~0)
		return (wchar_t*)lpString;
	if(c < 0)
		c = (int)strlen(lpString);
	p = AllocateStringW((size_t)AtoW(NULL, 0, lpString, c) + 1);
	if(p)
	{
		i = AtoW(p, 65535, lpString, c);
		p[i] = L'\0';
	}
	return p;
}

// メモリを確保してワイド文字列からShift_JIS文字列へ変換
// リソースIDならば元の値を返す
char* DuplicateWtoA(LPCWSTR lpString, int c)
{
	char* p;
	int i;
	if(lpString < (LPCWSTR)0x00010000 || lpString == (LPCWSTR)~0)
		return (char*)lpString;
	if(c < 0)
		c = (int)wcslen(lpString);
	p = AllocateStringA((size_t)WtoA(NULL, 0, lpString, c) + 1);
	if(p)
	{
		i = WtoA(p, 65535, lpString, c);
		p[i] = L'\0';
	}
	return p;
}

// マルチバイト文字列からコードポイントと次のポインタを取得
// エンコードが不正な場合は0x80000000を返す
DWORD GetNextCharM(LPCSTR lpString, LPCSTR pLimit, LPCSTR* ppNext)
{
	DWORD Code;
	int i;
	Code = 0;
	i = -1;
	if(!pLimit)
		pLimit = (LPCSTR)(~0);
	if(lpString < pLimit)
	{
		if((*lpString & 0xfe) == 0xfc)
		{
			i = 5;
			Code |= (DWORD)*lpString & 0x01;
		}
		else if((*lpString & 0xfc) == 0xf8)
		{
			i = 4;
			Code |= (DWORD)*lpString & 0x03;
		}
		else if((*lpString & 0xf8) == 0xf0)
		{
			i = 3;
			Code |= (DWORD)*lpString & 0x07;
		}
		else if((*lpString & 0xf0) == 0xe0)
		{
			i = 2;
			Code |= (DWORD)*lpString & 0x0f;
		}
		else if((*lpString & 0xe0) == 0xc0)
		{
			i = 1;
			Code |= (DWORD)*lpString & 0x1f;
		}
		else if((*lpString & 0x80) == 0x00)
		{
			i = 0;
			Code |= (DWORD)*lpString & 0x7f;
		}
		lpString++;
		while(lpString < pLimit && i > 0 && (*lpString & 0xc0) == 0x80)
		{
			i--;
			Code = Code << 6;
			Code |= (DWORD)*lpString & 0x3f;
			lpString++;
		}
	}
	if(i != 0)
		Code = 0x80000000;
	if(ppNext)
		*ppNext = lpString;
	return Code;
}

// マルチバイト文字列へコードポイントの文字を追加して次のポインタを取得
// 文字の長さを返す
int PutNextCharM(LPSTR lpString, LPSTR pLimit, LPSTR* ppNext, DWORD Code)
{
	int Count;
	int i;
	Count = 0;
	i = -1;
	if(!pLimit)
		pLimit = (LPSTR)(~0);
	if(lpString < pLimit)
	{
		if(Code & 0x7c000000)
		{
			i = 5;
			*lpString = 0xfc | ((CHAR)(Code >> 30) & 0x01);
		}
		else if(Code & 0x03e00000)
		{
			i = 4;
			*lpString = 0xf8 | ((CHAR)(Code >> 24) & 0x03);
		}
		else if(Code & 0x001f0000)
		{
			i = 3;
			*lpString = 0xf0 | ((CHAR)(Code >> 18) & 0x07);
		}
		else if(Code & 0x0000f800)
		{
			i = 2;
			*lpString = 0xe0 | ((CHAR)(Code >> 12) & 0x0f);
		}
		else if(Code & 0x00000780)
		{
			i = 1;
			*lpString = 0xc0 | ((CHAR)(Code >> 6) & 0x1f);
		}
		else
		{
			i = 0;
			*lpString = (CHAR)Code & 0x7f;
		}
		Count = i + 1;
		lpString++;
		while(lpString < pLimit && i > 0)
		{
			i--;
			*lpString = 0x80 | ((CHAR)(Code >> (i * 6)) & 0x3f);
			lpString++;
		}
	}
	if(i != 0)
		Count = 0;
	if(ppNext)
		*ppNext = lpString;
	return Count;
}

// ワイド文字列からコードポイントと次のポインタを取得
// エンコードが不正な場合は0x80000000を返す
DWORD GetNextCharW(LPCWSTR lpString, LPCWSTR pLimit, LPCWSTR* ppNext)
{
	DWORD Code;
	Code = 0x80000000;
	if(!pLimit)
		pLimit = (LPCWSTR)(~0);
	if(lpString < pLimit)
	{
		if((*lpString & 0xf800) == 0xd800)
		{
			if((*lpString & 0x0400) == 0x0400)
			{
				Code = (DWORD)*lpString & 0x03ff;
				lpString++;
				if(lpString < pLimit)
				{
					if((*lpString & 0x0400) == 0x0000)
					{
						Code |= ((DWORD)*lpString & 0x03ff) << 10;
						lpString++;
					}
					else
						Code = 0x80000000;
				}
				else
					Code = 0x80000000;
			}
		}
		else
		{
			Code = (DWORD)*lpString;
			lpString++;
		}
	}
	if(ppNext)
		*ppNext = lpString;
	return Code;
}

// ワイド文字列へコードポイントの文字を追加して次のポインタを取得
// 文字の長さを返す
int PutNextCharW(LPWSTR lpString, LPWSTR pLimit, LPWSTR* ppNext, DWORD Code)
{
	int Count;
	Count = 0;
	if(!pLimit)
		pLimit = (LPWSTR)(~0);
	if(lpString < pLimit)
	{
		if((Code & 0x7fff0000) || (Code & 0x0000f800) == 0x0000d800)
		{
			*lpString = 0xdc00 | ((WCHAR)Code & 0x03ff);
			lpString++;
			if(lpString < pLimit)
			{
				*lpString = 0xd800 | ((WCHAR)(Code >> 10) & 0x03ff);
				lpString++;
				Count = 2;
			}
		}
		else
		{
			*lpString = (WCHAR)Code;
			lpString++;
			Count = 1;
		}
	}
	if(ppNext)
		*ppNext = lpString;
	return Count;
}

// マルチバイト文字列のコードポイントの個数を取得
int GetCodeCountM(LPCSTR lpString, int CharCount)
{
	int Count;
	LPCSTR pLimit;
	DWORD Code;
	Count = 0;
	if(CharCount == -1)
		pLimit = lpString + strlen(lpString);
	else
		pLimit = lpString + CharCount;
	while(lpString < pLimit)
	{
		Code = GetNextCharM(lpString, pLimit, &lpString);
		if(Code == 0x80000000)
			continue;
		Count++;
	}
	return Count;
}

// ワイド文字列のコードポイントの個数を取得
int GetCodeCountW(LPCWSTR lpString, int CharCount)
{
	int Count;
	LPCWSTR pLimit;
	DWORD Code;
	Count = 0;
	if(CharCount == -1)
		pLimit = lpString + wcslen(lpString);
	else
		pLimit = lpString + CharCount;
	while(lpString < pLimit)
	{
		Code = GetNextCharW(lpString, pLimit, &lpString);
		if(Code == 0x80000000)
			continue;
		Count++;
	}
	return Count;
}

// 文字列用に確保したメモリを開放
// リソースIDならば何もしない
void FreeDuplicatedString(void* p)
{
	if(p < (void*)0x00010000 || p == (void*)~0)
		return;
	free(p);
}

// マルチバイト文字列からワイド文字列へ変換
// UTF-8からUTF-16 LEへの変換専用
int MultiByteToWideCharAlternative(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
	int WideCount;
	LPCSTR pMultiLimit;
	LPWSTR pWideLimit;
	DWORD Code;
	int TempCount;
	WCHAR Temp[8];
	if(CodePage != CP_UTF8 || dwFlags != 0)
		return MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
	WideCount = 0;
	if(cbMultiByte == -1)
		pMultiLimit = lpMultiByteStr + strlen(lpMultiByteStr) + 1;
	else
		pMultiLimit = lpMultiByteStr + cbMultiByte;
	pWideLimit = lpWideCharStr + cchWideChar;
	while(lpMultiByteStr < pMultiLimit)
	{
		Code = GetNextCharM(lpMultiByteStr, pMultiLimit, &lpMultiByteStr);
		if(Code == 0x80000000)
			continue;
		if(lpWideCharStr)
		{
			TempCount = PutNextCharW(lpWideCharStr, pWideLimit, &lpWideCharStr, Code);
			WideCount += TempCount;
			if(TempCount == 0 && lpWideCharStr >= pWideLimit)
			{
				WideCount = 0;
				break;
			}
		}
		else
			WideCount += PutNextCharW(Temp, NULL, NULL, Code);
	}
	return WideCount;
}

// ワイド文字列からマルチバイト文字列へ変換
// UTF-16 LEからUTF-8への変換専用
int WideCharToMultiByteAlternative(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
	int MultiCount;
	LPCWSTR pWideLimit;
	LPSTR pMultiLimit;
	DWORD Code;
	int TempCount;
	CHAR Temp[8];
	if(CodePage != CP_UTF8 || dwFlags != 0)
		return WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
	MultiCount = 0;
	if(cchWideChar == -1)
		pWideLimit = lpWideCharStr + wcslen(lpWideCharStr) + 1;
	else
		pWideLimit = lpWideCharStr + cchWideChar;
	pMultiLimit = lpMultiByteStr + cbMultiByte;
	while(lpWideCharStr < pWideLimit)
	{
		Code = GetNextCharW(lpWideCharStr, pWideLimit, &lpWideCharStr);
		if(Code == 0x80000000)
			continue;
		if(lpMultiByteStr)
		{
			TempCount = PutNextCharM(lpMultiByteStr, pMultiLimit, &lpMultiByteStr, Code);
			MultiCount += TempCount;
			if(TempCount == 0 && lpMultiByteStr >= pMultiLimit)
			{
				MultiCount = 0;
				break;
			}
		}
		else
			MultiCount += PutNextCharM(Temp, NULL, NULL, Code);
	}
	if(lpUsedDefaultChar)
		*lpUsedDefaultChar = FALSE;
	return MultiCount;
}

// 以下ラッパー
// 戻り値バッファ r
// ワイド文字バッファ pw%d
// マルチバイト文字バッファ pm%d
// 引数バッファ a%d

#pragma warning(disable:4102)
#define START_ROUTINE					do{
#define END_ROUTINE						}while(0);end_of_routine:
#define QUIT_ROUTINE					goto end_of_routine;

size_t _mbslenM(const unsigned char * _Str)
{
	size_t r = 0;
START_ROUTINE
	while(GetNextCharM((LPCSTR)_Str, NULL, (LPCSTR*)&_Str) > 0)
	{
		r++;
	}
END_ROUTINE
	return r;
}

int _mbsncmpM(const unsigned char * _Str1, const unsigned char * _Str2, size_t _MaxCount)
{
	int r = 0;
	DWORD c1;
	DWORD c2;
START_ROUTINE
	c1 = 0;
	c2 = 0;
	while(_MaxCount > 0)
	{
		c1 = GetNextCharM((LPCSTR)_Str1, NULL, (LPCSTR*)&_Str1);
		c2 = GetNextCharM((LPCSTR)_Str2, NULL, (LPCSTR*)&_Str2);
		if(c1 != c2)
			break;
		_MaxCount--;
		if(c1 == 0 || c2 == 0)
			break;
	}
	r = c1 - c2;
END_ROUTINE
	return r;
}

unsigned char * _mbsnincM(const unsigned char * _Str, size_t _Count)
{
	unsigned char* r = NULL;
START_ROUTINE
	while(_Count > 0 && GetNextCharM((LPCSTR)_Str, NULL, (LPCSTR*)&_Str) > 0)
	{
		_Count--;
	}
	r = (unsigned char*)_Str;
END_ROUTINE
	return r;
}
