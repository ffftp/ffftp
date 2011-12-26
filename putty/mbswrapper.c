// mbswrapper.c
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー
// マルチバイト文字はUTF-8、ワイド文字はUTF-16であるものとする
// 全ての制御用の文字はASCIIの範囲であるため、Shift_JISとUTF-8間の変換は不要

#define UNICODE
#define _UNICODE

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
		return GetMultiStringLengthM(pSrc);
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
		return GetMultiStringLengthW(pSrc);
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
		return GetMultiStringLengthA(pSrc);
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
		return GetMultiStringLengthW(pSrc);
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
char* AllocateStringM(int size)
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
wchar_t* AllocateStringW(int size)
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
char* AllocateStringA(int size)
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
		c = strlen(lpString);
	p = AllocateStringW(MtoW(NULL, 0, lpString, c) + 1);
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
		c = strlen(lpString);
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
	count = GetMultiStringLengthM(lpString) + 1;
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
	count = GetMultiStringLengthM(lpString) + 1;
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
		c = wcslen(lpString);
	p = AllocateStringM(WtoM(NULL, 0, lpString, c) + 1);
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
		c = strlen(lpString);
	p = AllocateStringW(AtoW(NULL, 0, lpString, c) + 1);
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
		c = wcslen(lpString);
	p = AllocateStringA(WtoA(NULL, 0, lpString, c) + 1);
	if(p)
	{
		i = WtoA(p, 65535, lpString, c);
		p[i] = L'\0';
	}
	return p;
}

// マルチバイト文字列からコードポイントと次のポインタを取得
// エンコードが不正な場合は0x80000000を返す
DWORD GetNextCharM(LPCSTR lpString, LPCSTR* ppNext)
{
	DWORD Code;
	int i;
	Code = 0;
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
	else
		i = -1;
	lpString++;
	while((*lpString & 0xc0) == 0x80)
	{
		i--;
		Code = Code << 6;
		Code |= (DWORD)*lpString & 0x3f;
		lpString++;
	}
	if(i != 0)
		Code = 0x80000000;
	if(ppNext)
		*ppNext = lpString;
	return Code;
}

// マルチバイト文字列の冗長表現を修正
// 修正があればTRUEを返す
// 修正後の文字列の長さは元の文字列の長さ以下のためpDstとpSrcに同じ値を指定可能
BOOL FixStringM(LPSTR pDst, LPCSTR pSrc)
{
	BOOL bResult;
	char* p;
	DWORD Code;
	int i;
	char c;
	bResult = FALSE;
	p = (char*)pSrc;
	while(*pSrc != '\0')
	{
		Code = GetNextCharM(pSrc, &pSrc);
		if(Code & 0x80000000)
			continue;
		else if(Code & 0x7c000000)
		{
			i = 5;
			c = (char)(0xfc | (Code >> (6 * i)));
		}
		else if(Code & 0x03e00000)
		{
			i = 4;
			c = (char)(0xf8 | (Code >> (6 * i)));
		}
		else if(Code & 0x001f0000)
		{
			i = 3;
			c = (char)(0xf0 | (Code >> (6 * i)));
		}
		else if(Code & 0x0000f800)
		{
			i = 2;
			c = (char)(0xe0 | (Code >> (6 * i)));
		}
		else if(Code & 0x00000780)
		{
			i = 1;
			c = (char)(0xc0 | (Code >> (6 * i)));
		}
		else
		{
			i = 0;
			c = (char)Code;
		}
		if(c != *p)
			bResult = TRUE;
		p++;
		*pDst = c;
		pDst++;
		while(i > 0)
		{
			i--;
			c = (char)(0x80 | ((Code >> (6 * i)) & 0x3f));
			if(c != *p)
				bResult = TRUE;
			p++;
			*pDst = c;
			pDst++;
		}
	}
	if(*p != '\0')
		bResult = TRUE;
	*pDst = '\0';
	return bResult;
}

// NULL区切りマルチバイト文字列の冗長表現を修正
// 修正があればTRUEを返す
// 修正後の文字列の長さは元の文字列の長さ以下のためpDstとpSrcに同じ値を指定可能
BOOL FixMultiStringM(LPSTR pDst, LPCSTR pSrc)
{
	BOOL bResult;
	int Length;
	bResult = FALSE;
	while(*pSrc != '\0')
	{
		Length = strlen(pSrc) + 1;
		bResult = bResult | FixStringM(pDst, pSrc);
		pSrc += Length;
		pDst += strlen(pDst) + 1;
	}
	*pDst = '\0';
	return bResult;
}

// マルチバイト文字列の冗長表現を確認
// 冗長表現があればTRUEを返す
BOOL CheckStringM(LPCSTR lpString)
{
	BOOL bResult;
	char* p;
	bResult = FALSE;
	p = AllocateStringM(strlen(lpString) + 1);
	if(p)
	{
		bResult = FixStringM(p, lpString);
		FreeDuplicatedString(p);
	}
	return bResult;
}

// NULL区切りマルチバイト文字列の冗長表現を確認
// 冗長表現があればTRUEを返す
BOOL CheckMultiStringM(LPCSTR lpString)
{
	BOOL bResult;
	char* p;
	bResult = FALSE;
	p = AllocateStringM(GetMultiStringLengthM(lpString) + 1);
	if(p)
	{
		bResult = FixMultiStringM(p, lpString);
		FreeDuplicatedString(p);
	}
	return bResult;
}

// 文字列用に確保したメモリを開放
// リソースIDならば何もしない
void FreeDuplicatedString(void* p)
{
	if(p < (void*)0x00010000 || p == (void*)~0)
		return;
	free(p);
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

HANDLE CreateFileM(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	HANDLE r = INVALID_HANDLE_VALUE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpFileName, -1);
	r = CreateFileW(pw0, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

HANDLE FindFirstFileM(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
	HANDLE r = INVALID_HANDLE_VALUE;
	wchar_t* pw0 = NULL;
	WIN32_FIND_DATAW a0;
START_ROUTINE
	pw0 = DuplicateMtoW(lpFileName, -1);
	r = FindFirstFileW(pw0, &a0);
	if(r != INVALID_HANDLE_VALUE)
	{
		lpFindFileData->dwFileAttributes = a0.dwFileAttributes;
		lpFindFileData->ftCreationTime = a0.ftCreationTime;
		lpFindFileData->ftLastAccessTime = a0.ftLastAccessTime;
		lpFindFileData->ftLastWriteTime = a0.ftLastWriteTime;
		lpFindFileData->nFileSizeHigh = a0.nFileSizeHigh;
		lpFindFileData->nFileSizeLow = a0.nFileSizeLow;
		lpFindFileData->dwReserved0 = a0.dwReserved0;
		lpFindFileData->dwReserved1 = a0.dwReserved1;
		WtoM(lpFindFileData->cFileName, sizeof(lpFindFileData->cFileName), a0.cFileName, -1);
		WtoM(lpFindFileData->cAlternateFileName, sizeof(lpFindFileData->cAlternateFileName), a0.cAlternateFileName, -1);
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

BOOL FindNextFileM(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
	BOOL r = FALSE;
	WIN32_FIND_DATAW a0;
START_ROUTINE
	r = FindNextFileW(hFindFile, &a0);
	if(r)
	{
		lpFindFileData->dwFileAttributes = a0.dwFileAttributes;
		lpFindFileData->ftCreationTime = a0.ftCreationTime;
		lpFindFileData->ftLastAccessTime = a0.ftLastAccessTime;
		lpFindFileData->ftLastWriteTime = a0.ftLastWriteTime;
		lpFindFileData->nFileSizeHigh = a0.nFileSizeHigh;
		lpFindFileData->nFileSizeLow = a0.nFileSizeLow;
		lpFindFileData->dwReserved0 = a0.dwReserved0;
		lpFindFileData->dwReserved1 = a0.dwReserved1;
		WtoM(lpFindFileData->cFileName, sizeof(lpFindFileData->cFileName), a0.cFileName, -1);
		WtoM(lpFindFileData->cAlternateFileName, sizeof(lpFindFileData->cAlternateFileName), a0.cAlternateFileName, -1);
	}
END_ROUTINE
	return r;
}

DWORD GetCurrentDirectoryM(DWORD nBufferLength, LPSTR lpBuffer)
{
	DWORD r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	// TODO: バッファが不十分な場合に必要なサイズを返す
	pw0 = AllocateStringW(nBufferLength * 4);
	GetCurrentDirectoryW(nBufferLength * 4, pw0);
	WtoM(lpBuffer, nBufferLength, pw0, -1);
	r = TerminateStringM(lpBuffer, nBufferLength);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

BOOL SetCurrentDirectoryM(LPCSTR lpPathName)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpPathName, -1);
	r = SetCurrentDirectoryW(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

DWORD GetTempPathM(DWORD nBufferLength, LPSTR lpBuffer)
{
	DWORD r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = AllocateStringW(nBufferLength * 4);
	GetTempPathW(nBufferLength * 4, pw0);
	WtoM(lpBuffer, nBufferLength, pw0, -1);
	r = TerminateStringM(lpBuffer, nBufferLength);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

DWORD GetFileAttributesM(LPCSTR lpFileName)
{
	DWORD r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpFileName, -1);
	r = GetFileAttributesW(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

DWORD GetModuleFileNameM(HMODULE hModule, LPCH lpFilename, DWORD nSize)
{
	DWORD r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = AllocateStringW(nSize * 4);
	GetModuleFileNameW(hModule, pw0, nSize * 4);
	WtoM(lpFilename, nSize, pw0, -1);
	r = TerminateStringM(lpFilename, nSize);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

BOOL CopyFileM(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpExistingFileName, -1);
	pw1 = DuplicateMtoW(lpNewFileName, -1);
	r = CopyFileW(pw0, pw1, bFailIfExists);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

int mkdirM(const char * _Path)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Path, -1);
	r = _wmkdir(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int _mkdirM(const char * _Path)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Path, -1);
	r = _wmkdir(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int rmdirM(const char * _Path)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Path, -1);
	r = _wrmdir(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int _rmdirM(const char * _Path)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Path, -1);
	r = _wrmdir(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int removeM(const char * _Filename)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Filename, -1);
	r = _wremove(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int _removeM(const char * _Filename)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Filename, -1);
	r = _wremove(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int _unlinkM(const char * _Filename)
{
	int r = -1;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Filename, -1);
	r = _wunlink(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

size_t _mbslenM(const unsigned char * _Str)
{
	size_t r = 0;
START_ROUTINE
	while(GetNextCharM(_Str, &_Str) > 0)
	{
		r++;
	}
END_ROUTINE
	return r;
}

unsigned char * _mbschrM(const unsigned char * _Str, unsigned int _Ch)
{
	unsigned char* r = NULL;
	unsigned int c;
	unsigned char* p;
START_ROUTINE
	while((c = GetNextCharM(_Str, &p)) > 0)
	{
		if(c == _Ch)
			break;
		_Str = p;
	}
	if(c == _Ch)
		r = (unsigned char*)_Str;
END_ROUTINE
	return r;
}

unsigned char * _mbsrchrM(const unsigned char * _Str, unsigned int _Ch)
{
	unsigned char* r = NULL;
	unsigned int c;
	unsigned char* p;
START_ROUTINE
	while((c = GetNextCharM(_Str, &p)) > 0)
	{
		if(c == _Ch)
			r = (unsigned char*)_Str;
		_Str = p;
	}
	if(c == _Ch)
		r = (unsigned char*)_Str;
END_ROUTINE
	return r;
}

unsigned char * _mbsstrM(const unsigned char * _Str, const unsigned char * _Substr)
{
	unsigned char* r = NULL;
START_ROUTINE
	r = strstr(_Str, _Substr);
END_ROUTINE
	return r;
}

int _mbscmpM(const unsigned char * _Str1, const unsigned char * _Str2)
{
	int r = 0;
START_ROUTINE
	r = strcmp(_Str1, _Str2);
END_ROUTINE
	return r;
}

int _mbsicmpM(const unsigned char * _Str1, const unsigned char * _Str2)
{
	int r = 0;
START_ROUTINE
	r = _stricmp(_Str1, _Str2);
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
		c1 = GetNextCharM(_Str1, &_Str1);
		c2 = GetNextCharM(_Str2, &_Str2);
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

unsigned char * _mbslwrM(unsigned char * _String)
{
	unsigned char* r = NULL;
START_ROUTINE
	r = _strlwr(_String);
END_ROUTINE
	return r;
}

unsigned char * _mbsuprM(unsigned char * _String)
{
	unsigned char* r = NULL;
START_ROUTINE
	r = _strupr(_String);
END_ROUTINE
	return r;
}

unsigned char * _mbsnincM(const unsigned char * _Str, size_t _Count)
{
	unsigned char* r = NULL;
START_ROUTINE
	while(_Count > 0 && GetNextCharM(_Str, &_Str) > 0)
	{
		_Count--;
	}
	r = (unsigned char*)_Str;
END_ROUTINE
	return r;
}

FILE * fopenM(const char * _Filename, const char * _Mode)
{
	FILE* r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Filename, -1);
	pw1 = DuplicateMtoW(_Mode, -1);
	r = _wfopen(pw0, pw1);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

