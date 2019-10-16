// mbswrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー

#ifndef __MBSWRAPPER_H__
#define __MBSWRAPPER_H__

#include <stdio.h>
#include <windows.h>
#include <shlobj.h>

#ifndef DO_NOT_REPLACE


#endif

#ifdef FORCE_SJIS_ON_ACTIVE_CODE_PAGE
#undef CP_ACP
#define CP_ACP 932
#endif

#ifdef EMULATE_UTF8_WCHAR_CONVERSION
#define MultiByteToWideChar MultiByteToWideCharAlternative
#define WideCharToMultiByte WideCharToMultiByteAlternative
#endif

int MtoW(LPWSTR pDst, int size, LPCSTR pSrc, int count);
int WtoM(LPSTR pDst, int size, LPCWSTR pSrc, int count);
int AtoW(LPWSTR pDst, int size, LPCSTR pSrc, int count);
int WtoA(LPSTR pDst, int size, LPCWSTR pSrc, int count);
int TerminateStringM(LPSTR lpString, int size);
int TerminateStringW(LPWSTR lpString, int size);
int TerminateStringA(LPSTR lpString, int size);
size_t GetMultiStringLengthM(LPCSTR lpString);
size_t GetMultiStringLengthW(LPCWSTR lpString);
size_t GetMultiStringLengthA(LPCSTR lpString);
int MtoWMultiString(LPWSTR pDst, int size, LPCSTR pSrc);
int WtoMMultiString(LPSTR pDst, int size, LPCWSTR pSrc);
int AtoWMultiString(LPWSTR pDst, int size, LPCSTR pSrc);
int WtoAMultiString(LPSTR pDst, int size, LPCWSTR pSrc);
char* AllocateStringM(size_t size);
wchar_t* AllocateStringW(size_t size);
char* AllocateStringA(size_t size);
wchar_t* DuplicateMtoW(LPCSTR lpString, int c);
wchar_t* DuplicateMtoWBuffer(LPCSTR lpString, int c, int size);
wchar_t* DuplicateMtoWMultiString(LPCSTR lpString);
wchar_t* DuplicateMtoWMultiStringBuffer(LPCSTR lpString, int size);
char* DuplicateWtoM(LPCWSTR lpString, int c);
wchar_t* DuplicateAtoW(LPCSTR lpString, int c);
char* DuplicateWtoA(LPCWSTR lpString, int c);
DWORD GetNextCharM(LPCSTR lpString, LPCSTR pLimit, LPCSTR* ppNext);
int PutNextCharM(LPSTR lpString, LPSTR pLimit, LPSTR* ppNext, DWORD Code);
DWORD GetNextCharW(LPCWSTR lpString, LPCWSTR pLimit, LPCWSTR* ppNext);
int PutNextCharW(LPWSTR lpString, LPWSTR pLimit, LPWSTR* ppNext, DWORD Code);
int GetCodeCountM(LPCSTR lpString, int CharCount);
int GetCodeCountW(LPCWSTR lpString, int CharCount);
void FreeDuplicatedString(void* p);
int MultiByteToWideCharAlternative(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
int WideCharToMultiByteAlternative(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);

#endif

