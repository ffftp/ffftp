// mbswrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー

#ifndef __MBSWRAPPER_H__
#define __MBSWRAPPER_H__

#include <stdio.h>
#include <windows.h>
#include <shlobj.h>

#ifndef DO_NOT_REPLACE

#undef CreateFile
#define CreateFile CreateFileM
HANDLE CreateFileM(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#undef FindFirstFile
#define FindFirstFile FindFirstFileM
HANDLE FindFirstFileM(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
#undef FindNextFile
#define FindNextFile FindNextFileM
BOOL FindNextFileM(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
#undef GetCurrentDirectory
#define GetCurrentDirectory GetCurrentDirectoryM
DWORD GetCurrentDirectoryM(DWORD nBufferLength, LPSTR lpBuffer);
#undef SetCurrentDirectory
#define SetCurrentDirectory SetCurrentDirectoryM
BOOL SetCurrentDirectoryM(LPCSTR lpPathName);
#undef GetTempPath
#define GetTempPath GetTempPathM
DWORD GetTempPathM(DWORD nBufferLength, LPSTR lpBuffer);
#undef GetFileAttributes
#define GetFileAttributes GetFileAttributesM
DWORD GetFileAttributesM(LPCSTR lpFileName);
#undef GetModuleFileName
#define GetModuleFileName GetModuleFileNameM
DWORD GetModuleFileNameM(HMODULE hModule, LPCH lpFilename, DWORD nSize);
#undef CopyFile
#define CopyFile CopyFileM
BOOL CopyFileM(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
#undef MoveFile
#define MoveFile MoveFileM
BOOL MoveFileM(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
#undef mkdir
#define mkdir _mkdirM
int mkdirM(const char * _Path);
#undef _mkdir
#define _mkdir _mkdirM
int _mkdirM(const char * _Path);
#undef rmdir
#define rmdir rmdirM
int rmdirM(const char * _Path);
#undef _rmdir
#define _rmdir _rmdirM
int _rmdirM(const char * _Path);
#undef remove
#define remove removeM
int removeM(const char * _Filename);
#undef _remove
#define _remove _removeM
int _removeM(const char * _Filename);
#undef _unlink
#define _unlink _unlinkM
int _unlinkM(const char * _Filename);
#undef _mbslen
#define _mbslen _mbslenM
size_t _mbslenM(const unsigned char * _Str);
#undef _mbschr
#define _mbschr _mbschrM
unsigned char * _mbschrM(const unsigned char * _Str, unsigned int _Ch);
#undef _mbsrchr
#define _mbsrchr _mbsrchrM
unsigned char * _mbsrchrM(const unsigned char * _Str, unsigned int _Ch);
#undef _mbsstr
#define _mbsstr _mbsstrM
unsigned char * _mbsstrM(const unsigned char * _Str, const unsigned char * _Substr);
#undef _mbscmp
#define _mbscmp _mbscmpM
int _mbscmpM(const unsigned char * _Str1, const unsigned char * _Str2);
#undef _mbsicmp
#define _mbsicmp _mbsicmpM
int _mbsicmpM(const unsigned char * _Str1, const unsigned char * _Str2);
#undef _mbsncmp
#define _mbsncmp _mbsncmpM
int _mbsncmpM(const unsigned char * _Str1, const unsigned char * _Str2, size_t _MaxCount);
#undef _mbslwr
#define _mbslwr _mbslwrM
unsigned char * _mbslwrM(unsigned char * _String);
#undef _mbsupr
#define _mbsupr _mbsuprM
unsigned char * _mbsuprM(unsigned char * _String);
#undef _mbsninc
#define _mbsninc _mbsnincM
unsigned char * _mbsnincM(const unsigned char * _Str, size_t _Count);
#undef fopen
#define fopen fopenM
FILE * fopenM(const char * _Filename, const char * _Mode);

#endif

#undef CP_ACP
#define CP_ACP 932

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
char* AllocateStringM(int size);
wchar_t* AllocateStringW(int size);
char* AllocateStringA(int size);
wchar_t* DuplicateMtoW(LPCSTR lpString, int c);
wchar_t* DuplicateMtoWBuffer(LPCSTR lpString, int c, int size);
wchar_t* DuplicateMtoWMultiString(LPCSTR lpString);
wchar_t* DuplicateMtoWMultiStringBuffer(LPCSTR lpString, int size);
char* DuplicateWtoM(LPCWSTR lpString, int c);
wchar_t* DuplicateAtoW(LPCSTR lpString, int c);
char* DuplicateWtoA(LPCWSTR lpString, int c);
DWORD GetNextCharM(LPCSTR lpString, LPCSTR* ppNext);
BOOL FixStringM(LPSTR pDst, LPCSTR pSrc);
BOOL FixMultiStringM(LPSTR pDst, LPCSTR pSrc);
BOOL CheckStringM(LPCSTR lpString);
BOOL CheckMultiStringM(LPCSTR lpString);
void FreeDuplicatedString(void* p);

#endif

