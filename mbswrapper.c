// mbswrapper.cpp
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー
// マルチバイト文字はUTF-8、ワイド文字はUTF-16であるものとする
// 全ての制御用の文字はASCIIの範囲であるため、Shift_JISとUTF-8間の変換は不要

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#undef _WIN32_IE
#define _WIN32_IE 0x0400

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

// メモリを確保してワイド文字列からShift_JIS文字列へ変換
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

// 文字列用に確保したメモリを開放
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

#define START_ROUTINE					do{
#define END_ROUTINE						}while(0);

/*
#define END_ROUTINE						}while(0);end_of_routine:
#define QUIT_ROUTINE					goto end_of_routine;
*/

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

int MessageBoxM(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	int r = IDOK;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpText, -1);
	pw1 = DuplicateMtoW(lpCaption, -1);
	r = MessageBoxW(hWnd, pw0, pw1, uType);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
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

DWORD GetLogicalDriveStringsM(DWORD nBufferLength, LPSTR lpBuffer)
{
	DWORD r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = AllocateStringW(nBufferLength * 4);
	GetLogicalDriveStringsW(nBufferLength * 4, pw0);
	WtoMMultiString(lpBuffer, nBufferLength, pw0);
	r = TerminateStringM(lpBuffer, nBufferLength);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

ATOM RegisterClassExM(CONST WNDCLASSEXA * v0)
{
	ATOM r = 0;
START_ROUTINE
	// WNDPROCがShift_JIS用であるため
	r = RegisterClassExA(v0);
END_ROUTINE
	return r;
}

HWND CreateWindowExM(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	HWND r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpClassName, -1);
	pw1 = DuplicateMtoW(lpWindowName, -1);
	r = CreateWindowExW(dwExStyle, pw0, pw1, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

LONG GetWindowLongM(HWND hWnd, int nIndex)
{
	LRESULT r = 0;
START_ROUTINE
	// WNDPROCがShift_JIS用であるため
	if(IsWindowUnicode(hWnd))
		r = GetWindowLongW(hWnd, nIndex);
	else
		r = GetWindowLongA(hWnd, nIndex);
END_ROUTINE
	return r;
}

LONG SetWindowLongM(HWND hWnd, int nIndex, LONG dwNewLong)
{
	LRESULT r = 0;
START_ROUTINE
	// WNDPROCがShift_JIS用であるため
	if(IsWindowUnicode(hWnd))
		r = SetWindowLongW(hWnd, nIndex, dwNewLong);
	else
		r = SetWindowLongA(hWnd, nIndex, dwNewLong);
END_ROUTINE
	return r;
}

LRESULT DefWindowProcM(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT r = 0;
START_ROUTINE
	// WNDPROCがShift_JIS用であるため
	if(IsWindowUnicode(hWnd))
		r = DefWindowProcW(hWnd, Msg, wParam, lParam);
	else
		r = DefWindowProcA(hWnd, Msg, wParam, lParam);
END_ROUTINE
	return r;
}

LRESULT CallWindowProcM(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT r = 0;
START_ROUTINE
	// WNDPROCがShift_JIS用であるため
	if(IsWindowUnicode(hWnd))
		r = CallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
	else
		r = CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
END_ROUTINE
	return r;
}

LRESULT SendMessageM(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	int Size;
	LVITEMA* pmLVItem;
	LVITEMW wLVItem;
	LVFINDINFOA* pmLVFindInfo;
	LVFINDINFOW wLVFindInfo;
	LVCOLUMNA* pmLVColumn;
	LVCOLUMNW wLVColumn;
	wchar_t ClassName[MAX_PATH];
START_ROUTINE
	switch(Msg)
	{
	case WM_SETTEXT:
		pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
		r = SendMessageW(hWnd, WM_SETTEXT, wParam, (LPARAM)pw0);
		break;
	case WM_GETTEXT:
		pw0 = AllocateStringW(wParam * 4);
		SendMessageW(hWnd, WM_GETTEXT, wParam * 4, (LPARAM)pw0);
		WtoM((LPSTR)lParam, wParam, pw0, -1);
		r = TerminateStringM((LPSTR)lParam, wParam);
		break;
	case WM_GETTEXTLENGTH:
		Size = SendMessageW(hWnd, WM_GETTEXTLENGTH, wParam, lParam) + 1;
		pw0 = AllocateStringW(Size);
		SendMessageW(hWnd, WM_GETTEXT, (WPARAM)Size, (LPARAM)pw0);
		r = WtoM(NULL, 0, pw0, -1) - 1;
		break;
	default:
		GetClassNameW(hWnd, ClassName, sizeof(ClassName) / sizeof(wchar_t));
		if(_wcsicmp(ClassName, WC_EDITW) == 0)
		{
			switch(Msg)
			{
			case EM_REPLACESEL:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, EM_REPLACESEL, wParam, (LPARAM)pw0);
				break;
			default:
				r = SendMessageW(hWnd, Msg, wParam, lParam);
				break;
			}
		}
		else if(_wcsicmp(ClassName, WC_COMBOBOXW) == 0)
		{
			switch(Msg)
			{
			case CB_ADDSTRING:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, CB_ADDSTRING, wParam, (LPARAM)pw0);
				break;
			case CB_GETLBTEXT:
				Size = SendMessageW(hWnd, CB_GETLBTEXTLEN, wParam, 0) + 1;
				pw0 = AllocateStringW(Size);
				SendMessageW(hWnd, CB_GETLBTEXT, wParam, (LPARAM)pw0);
				// バッファ長不明のためオーバーランの可能性あり
				WtoM((LPSTR)lParam, Size * 4, pw0, -1);
				r = TerminateStringM((LPSTR)lParam, Size * 4);
				break;
			case CB_GETLBTEXTLEN:
				Size = SendMessageW(hWnd, CB_GETLBTEXTLEN, wParam, 0) + 1;
				pw0 = AllocateStringW(Size);
				SendMessageW(hWnd, WM_GETTEXT, wParam, (LPARAM)pw0);
				r = WtoM(NULL, 0, pw0, -1) - 1;
				break;
			case CB_INSERTSTRING:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, CB_INSERTSTRING, wParam, (LPARAM)pw0);
				break;
			case CB_FINDSTRINGEXACT:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, CB_FINDSTRINGEXACT, wParam, (LPARAM)pw0);
				break;
			default:
				r = SendMessageW(hWnd, Msg, wParam, lParam);
				break;
			}
		}
		else if(_wcsicmp(ClassName, WC_LISTBOXW) == 0)
		{
			switch(Msg)
			{
			case LB_ADDSTRING:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, LB_ADDSTRING, wParam, (LPARAM)pw0);
				break;
			case LB_INSERTSTRING:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, LB_INSERTSTRING, wParam, (LPARAM)pw0);
				break;
			case LB_GETTEXT:
				Size = SendMessageW(hWnd, LB_GETTEXTLEN, wParam, 0) + 1;
				pw0 = AllocateStringW(Size);
				SendMessageW(hWnd, LB_GETTEXT, wParam, (LPARAM)pw0);
				// バッファ長不明のためオーバーランの可能性あり
				WtoM((LPSTR)lParam, Size * 4, pw0, -1);
				r = TerminateStringM((LPSTR)lParam, Size * 4);
				break;
			case LB_GETTEXTLEN:
				Size = SendMessageW(hWnd, LB_GETTEXTLEN, wParam, 0) + 1;
				pw0 = AllocateStringW(Size);
				SendMessageW(hWnd, WM_GETTEXT, wParam, (LPARAM)pw0);
				r = WtoM(NULL, 0, pw0, -1) - 1;
				break;
			default:
				r = SendMessageW(hWnd, Msg, wParam, lParam);
				break;
			}
		}
		else if(_wcsicmp(ClassName, WC_LISTVIEWW) == 0)
		{
			switch(Msg)
			{
			case LVM_GETITEMA:
				pmLVItem = (LVITEMA*)lParam;
				wLVItem.mask = pmLVItem->mask;
				wLVItem.iItem = pmLVItem->iItem;
				wLVItem.iSubItem = pmLVItem->iSubItem;
				wLVItem.state = pmLVItem->state;
				wLVItem.stateMask = pmLVItem->stateMask;
				if(pmLVItem->mask & LVIF_TEXT)
				{
					Size = pmLVItem->cchTextMax * 4;
					pw0 = AllocateStringW(Size);
					wLVItem.pszText = pw0;
					wLVItem.cchTextMax = Size;
				}
				wLVItem.iImage = pmLVItem->iImage;
				wLVItem.lParam = pmLVItem->lParam;
				wLVItem.iIndent = pmLVItem->iIndent;
				r = SendMessageW(hWnd, LVM_GETITEMW, wParam, (LPARAM)&wLVItem);
				pmLVItem->mask = wLVItem.mask;
				pmLVItem->iItem = wLVItem.iItem;
				pmLVItem->iSubItem = wLVItem.iSubItem;
				pmLVItem->state = wLVItem.state;
				pmLVItem->stateMask = wLVItem.stateMask;
				if(pmLVItem->mask & LVIF_TEXT)
				{
					WtoM(pmLVItem->pszText, pmLVItem->cchTextMax, wLVItem.pszText, -1);
					TerminateStringM(pmLVItem->pszText, pmLVItem->cchTextMax);
				}
				pmLVItem->iImage = wLVItem.iImage;
				pmLVItem->lParam = wLVItem.lParam;
				pmLVItem->iIndent = wLVItem.iIndent;
				break;
			case LVM_SETITEMA:
				pmLVItem = (LVITEMA*)lParam;
				wLVItem.mask = pmLVItem->mask;
				wLVItem.iItem = pmLVItem->iItem;
				wLVItem.iSubItem = pmLVItem->iSubItem;
				wLVItem.state = pmLVItem->state;
				wLVItem.stateMask = pmLVItem->stateMask;
				if(pmLVItem->mask & LVIF_TEXT)
				{
					pw0 = DuplicateMtoW(pmLVItem->pszText, -1);
					wLVItem.pszText = pw0;
					// TODO: cchTextMaxの確認
					wLVItem.cchTextMax = pmLVItem->cchTextMax;
				}
				wLVItem.iImage = pmLVItem->iImage;
				wLVItem.lParam = pmLVItem->lParam;
				wLVItem.iIndent = pmLVItem->iIndent;
				r = SendMessageW(hWnd, LVM_SETITEMW, wParam, (LPARAM)&wLVItem);
				break;
			case LVM_INSERTITEMA:
				pmLVItem = (LVITEMA*)lParam;
				wLVItem.mask = pmLVItem->mask;
				wLVItem.iItem = pmLVItem->iItem;
				wLVItem.iSubItem = pmLVItem->iSubItem;
				wLVItem.state = pmLVItem->state;
				wLVItem.stateMask = pmLVItem->stateMask;
				if(pmLVItem->mask & LVIF_TEXT)
				{
					pw0 = DuplicateMtoW(pmLVItem->pszText, -1);
					wLVItem.pszText = pw0;
					// TODO: cchTextMaxの確認
					wLVItem.cchTextMax = pmLVItem->cchTextMax;
				}
				wLVItem.iImage = pmLVItem->iImage;
				wLVItem.lParam = pmLVItem->lParam;
				wLVItem.iIndent = pmLVItem->iIndent;
				r = SendMessageW(hWnd, LVM_INSERTITEMW, wParam, (LPARAM)&wLVItem);
				break;
			case LVM_FINDITEMA:
				pmLVFindInfo = (LVFINDINFOA*)lParam;
				wLVFindInfo.flags = pmLVFindInfo->flags;
				if(pmLVFindInfo->flags & (LVFI_STRING | LVFI_PARTIAL))
				{
					pw0 = DuplicateMtoW(pmLVFindInfo->psz, -1);
					wLVFindInfo.psz = pw0;
				}
				wLVFindInfo.lParam = pmLVFindInfo->lParam;
				wLVFindInfo.pt = pmLVFindInfo->pt;
				wLVFindInfo.vkDirection = pmLVFindInfo->vkDirection;
				r = SendMessageW(hWnd, LVM_FINDITEMW, wParam, (LPARAM)&wLVItem);
				break;
			case LVM_GETCOLUMNA:
				pmLVColumn = (LVCOLUMNA*)lParam;
				wLVColumn.mask = pmLVColumn->mask;
				wLVColumn.fmt = pmLVColumn->fmt;
				wLVColumn.cx = pmLVColumn->cx;
				Size = pmLVColumn->cchTextMax * 4;
				if(pmLVColumn->mask & LVCF_TEXT)
				{
					pw0 = AllocateStringW(Size);
					wLVColumn.pszText = pw0;
					wLVColumn.cchTextMax = Size;
				}
				wLVColumn.iSubItem = pmLVColumn->iSubItem;
				wLVColumn.iImage = pmLVColumn->iImage;
				wLVColumn.iOrder = pmLVColumn->iOrder;
				r = SendMessageW(hWnd, LVM_GETCOLUMNW, wParam, (LPARAM)&wLVColumn);
				pmLVColumn->mask = wLVColumn.mask;
				pmLVColumn->fmt = wLVColumn.fmt;
				pmLVColumn->cx = wLVColumn.cx;
				if(pmLVColumn->mask & LVCF_TEXT)
				{
					WtoM(pmLVColumn->pszText, pmLVColumn->cchTextMax, wLVColumn.pszText, -1);
					TerminateStringM(pmLVColumn->pszText, pmLVColumn->cchTextMax);
				}
				pmLVColumn->iSubItem = wLVColumn.iSubItem;
				pmLVColumn->iImage = wLVColumn.iImage;
				pmLVColumn->iOrder = wLVColumn.iOrder;
				break;
			case LVM_INSERTCOLUMNA:
				pmLVColumn = (LVCOLUMNA*)lParam;
				wLVColumn.mask = pmLVColumn->mask;
				wLVColumn.fmt = pmLVColumn->fmt;
				wLVColumn.cx = pmLVColumn->cx;
				if(pmLVColumn->mask & LVCF_TEXT)
				{
					pw0 = DuplicateMtoW(pmLVColumn->pszText, -1);
					wLVColumn.pszText = pw0;
					// TODO: cchTextMaxの確認
					wLVColumn.cchTextMax = pmLVColumn->cchTextMax;
				}
				wLVColumn.iSubItem = pmLVColumn->iSubItem;
				wLVColumn.iImage = pmLVColumn->iImage;
				wLVColumn.iOrder = pmLVColumn->iOrder;
				r = SendMessageW(hWnd, LVM_INSERTCOLUMNW, wParam, (LPARAM)&wLVColumn);
				break;
			case LVM_GETITEMTEXTA:
				pmLVItem = (LVITEMA*)lParam;
				wLVItem.mask = pmLVItem->mask;
				wLVItem.iItem = pmLVItem->iItem;
				wLVItem.iSubItem = pmLVItem->iSubItem;
				wLVItem.state = pmLVItem->state;
				wLVItem.stateMask = pmLVItem->stateMask;
				Size = pmLVItem->cchTextMax * 4;
				pw0 = AllocateStringW(Size);
				wLVItem.pszText = pw0;
				wLVItem.cchTextMax = Size;
				wLVItem.iImage = pmLVItem->iImage;
				wLVItem.lParam = pmLVItem->lParam;
				wLVItem.iIndent = pmLVItem->iIndent;
				r = SendMessageW(hWnd, LVM_GETITEMTEXTW, wParam, (LPARAM)&wLVItem);
				pmLVItem->mask = wLVItem.mask;
				pmLVItem->iItem = wLVItem.iItem;
				pmLVItem->iSubItem = wLVItem.iSubItem;
				pmLVItem->state = wLVItem.state;
				pmLVItem->stateMask = wLVItem.stateMask;
				WtoM(pmLVItem->pszText, pmLVItem->cchTextMax, wLVItem.pszText, -1);
				TerminateStringM(pmLVItem->pszText, pmLVItem->cchTextMax);
				pmLVItem->iImage = wLVItem.iImage;
				pmLVItem->lParam = wLVItem.lParam;
				pmLVItem->iIndent = wLVItem.iIndent;
				break;
			default:
				r = SendMessageW(hWnd, Msg, wParam, lParam);
				break;
			}
		}
		else if(_wcsicmp(ClassName, STATUSCLASSNAMEW) == 0)
		{
			switch(Msg)
			{
			case SB_SETTEXTA:
				pw0 = DuplicateMtoW((LPCSTR)lParam, -1);
				r = SendMessageW(hWnd, SB_SETTEXTW, wParam, (LPARAM)pw0);
				break;
			default:
				r = SendMessageW(hWnd, Msg, wParam, lParam);
				break;
			}
		}
		else
			r = SendMessageW(hWnd, Msg, wParam, lParam);
		break;
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

LRESULT DefDlgProcM(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT r = 0;
START_ROUTINE
	// WNDPROCがShift_JIS用であるため
	if(IsWindowUnicode(hWnd))
		r = DefDlgProcW(hWnd, Msg, wParam, lParam);
	else
		r = DefDlgProcA(hWnd, Msg, wParam, lParam);
END_ROUTINE
	return r;
}

LRESULT SendDlgItemMessageM(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT r = 0;
START_ROUTINE
	r = SendMessageM(GetDlgItem(hDlg, nIDDlgItem), Msg, wParam, lParam);
END_ROUTINE
	return r;
}

BOOL SetWindowTextM(HWND hWnd, LPCSTR lpString)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpString, -1);
	r = SetWindowTextW(hWnd, pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

UINT DragQueryFileM(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cch)
{
	UINT r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	if(iFile == (UINT)-1)
		r = DragQueryFileW(hDrop, iFile, (LPWSTR)lpszFile, cch);
	else
	{
		pw0 = AllocateStringW(cch * 4);
		DragQueryFileW(hDrop, iFile, pw0, cch * 4);
		WtoM(lpszFile, cch, pw0, -1);
		r = TerminateStringM(lpszFile, cch);
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

DWORD GetCurrentDirectoryM(DWORD nBufferLength, LPSTR lpBuffer)
{
	DWORD r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
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

BOOL SetDllDirectoryM(LPCSTR lpPathName)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpPathName, -1);
	r = SetDllDirectoryW(pw0);
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

LSTATUS RegOpenKeyExM(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	LSTATUS r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpSubKey, -1);
	r = RegOpenKeyExW(hKey, pw0, ulOptions, samDesired, phkResult);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

LSTATUS RegCreateKeyExM(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
	LSTATUS r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpSubKey, -1);
	pw1 = DuplicateMtoW(lpClass, -1);
	r = RegCreateKeyExW(hKey, pw0, Reserved, pw1, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

LSTATUS RegDeleteValueM(HKEY hKey, LPCSTR lpValueName)
{
	LSTATUS r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpValueName, -1);
	r = RegDeleteValueW(hKey, pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

LSTATUS RegQueryValueExM(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	LSTATUS r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	DWORD dwType;
	DWORD wcbData;
START_ROUTINE
	pw0 = DuplicateMtoW(lpValueName, -1);
	if(RegQueryValueExW(hKey, pw0, NULL, &dwType, NULL, 0) == ERROR_SUCCESS)
	{
		switch(dwType)
		{
		case REG_SZ:
		case REG_EXPAND_SZ:
		case REG_MULTI_SZ:
			if(lpcbData)
			{
				pw1 = AllocateStringW(*lpcbData / sizeof(char) * 4);
				wcbData = *lpcbData / sizeof(char) * 4;
				r = RegQueryValueExW(hKey, pw0, lpReserved, lpType, (LPBYTE)pw1, &wcbData);
				if(lpData)
					*lpcbData = sizeof(char) * WtoM((char*)lpData, *lpcbData / sizeof(char), pw1, wcbData / sizeof(wchar_t));
				else
					*lpcbData = sizeof(char) * WtoM(NULL, 0, pw1, wcbData / sizeof(wchar_t));
			}
			break;
		default:
			r = RegQueryValueExW(hKey, pw0, lpReserved, lpType, lpData, lpcbData);
			break;
		}
	}
	else
		r = RegQueryValueExW(hKey, pw0, lpReserved, lpType, lpData, lpcbData);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

LSTATUS RegSetValueExM(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE* lpData, DWORD cbData)
{
	LSTATUS r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	DWORD wcbData;
START_ROUTINE
	pw0 = DuplicateMtoW(lpValueName, -1);
	switch(dwType)
	{
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_MULTI_SZ:
		wcbData = MtoW(NULL, 0, (char*)lpData, cbData / sizeof(char));
		pw1 = AllocateStringW(wcbData);
		MtoW(pw1, wcbData, (char*)lpData, cbData / sizeof(char));
		wcbData = sizeof(wchar_t) * wcbData;
		lpData = (BYTE*)pw1;
		cbData = wcbData;
		break;
	}
	r = RegSetValueExW(hKey, pw0, Reserved, dwType, lpData, cbData);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

BOOL TextOutM(HDC hdc, int x, int y, LPCSTR lpString, int c)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpString, c);
	r = TextOutW(hdc, x, y, pw0, wcslen(pw0));
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

BOOL GetTextExtentPoint32M(HDC hdc, LPCSTR lpString, int c, LPSIZE psizl)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpString, c);
	r = GetTextExtentPoint32W(hdc, pw0, wcslen(pw0), psizl);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

INT_PTR PropertySheetM(LPCPROPSHEETHEADERA v0)
{
	INT_PTR r = 0;
	PROPSHEETHEADERW a0;
	PROPSHEETPAGEW* pwPage;
	UINT i;
START_ROUTINE
	a0.dwSize = sizeof(PROPSHEETHEADERW);
	a0.dwFlags = v0->dwFlags;
	a0.hwndParent = v0->hwndParent;
	a0.hInstance = v0->hInstance;
	if(v0->dwFlags & PSH_USEICONID)
		a0.pszIcon = DuplicateMtoW(v0->pszIcon, -1);
	else
		a0.hIcon = v0->hIcon;
	a0.pszCaption = DuplicateMtoW(v0->pszCaption, -1);
	a0.nPages = v0->nPages;
	a0.pStartPage = DuplicateMtoW(v0->pStartPage, -1);
	if(v0->ppsp && (pwPage = (PROPSHEETPAGEW*)malloc(sizeof(PROPSHEETPAGEW) * v0->nPages)))
	{
		for(i = 0; i < v0->nPages; i++)
		{
			pwPage[i].dwSize = sizeof(PROPSHEETPAGEW);
			pwPage[i].dwFlags = v0->ppsp[i].dwFlags;
			pwPage[i].hInstance = v0->ppsp[i].hInstance;
			pwPage[i].pszTemplate = DuplicateMtoW(v0->ppsp[i].pszTemplate, -1);
			if(v0->ppsp[i].dwFlags & PSP_USEICONID)
				pwPage[i].pszIcon = DuplicateMtoW(v0->ppsp[i].pszIcon, -1);
			else
				pwPage[i].hIcon = v0->ppsp[i].hIcon;
			if(v0->ppsp[i].dwFlags & PSP_USETITLE)
				pwPage[i].pszTitle = DuplicateMtoW(v0->ppsp[i].pszTitle, -1);
			pwPage[i].pfnDlgProc = v0->ppsp[i].pfnDlgProc;
			pwPage[i].lParam = v0->ppsp[i].lParam;
			// TODO: pfnCallback
			pwPage[i].pfnCallback = v0->ppsp[i].pfnCallback;
			pwPage[i].pcRefParent = v0->ppsp[i].pcRefParent;
//			pwPage[i].pszHeaderTitle = DuplicateMtoW(v0->ppsp[i].pszHeaderTitle, -1);
//			pwPage[i].pszHeaderSubTitle = DuplicateMtoW(v0->ppsp[i].pszHeaderSubTitle, -1);
			pwPage[i].hActCtx = v0->ppsp[i].hActCtx;
//			pwPage[i].pszbmHeader = DuplicateMtoW(v0->ppsp[i].pszbmHeader, -1);
		}
	}
	else
		pwPage = NULL;
	a0.ppsp = pwPage;
	a0.pfnCallback = v0->pfnCallback;
	r = PropertySheetW(&a0);
	if(a0.dwFlags & PSH_USEICONID)
		FreeDuplicatedString(a0.pszIcon);
	FreeDuplicatedString(a0.pszCaption);
	FreeDuplicatedString(a0.pStartPage);
	if(pwPage)
	{
		for(i = 0; i < v0->nPages; i++)
		{
			FreeDuplicatedString(pwPage[i].pszTemplate);
			if(pwPage[i].dwFlags & PSP_USEICONID)
				FreeDuplicatedString(pwPage[i].pszIcon);
			if(pwPage[i].dwFlags & PSP_USETITLE)
				FreeDuplicatedString(pwPage[i].pszTitle);
//			FreeDuplicatedString(pwPage[i].pszHeaderTitle);
//			FreeDuplicatedString(pwPage[i].pszHeaderSubTitle);
//			FreeDuplicatedString(pwPage[i].pszbmHeader);
		}
		free(pwPage);
	}
END_ROUTINE
	return r;
}

BOOL GetOpenFileNameM(LPOPENFILENAMEA v0)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* pw2 = NULL;
	wchar_t* pw3 = NULL;
	wchar_t* pw4 = NULL;
	wchar_t* pw5 = NULL;
	wchar_t* pw6 = NULL;
	wchar_t* pw7 = NULL;
	wchar_t* pw8 = NULL;
	wchar_t* pw9 = NULL;
	OPENFILENAMEW wofn;
START_ROUTINE
	wofn.lStructSize = sizeof(OPENFILENAMEW);
	wofn.hwndOwner = v0->hwndOwner;
	wofn.hInstance = v0->hInstance;
	pw0 = DuplicateMtoWMultiString(v0->lpstrFilter);
	wofn.lpstrFilter = pw0;
	pw1 = DuplicateMtoWBuffer(v0->lpstrCustomFilter, -1, v0->nMaxCustFilter * 4);
	wofn.lpstrCustomFilter = pw1;
	wofn.nMaxCustFilter = v0->nMaxCustFilter * 4;
	wofn.nFilterIndex = v0->nFilterIndex;
	pw2 = DuplicateMtoWMultiStringBuffer(v0->lpstrFile, v0->nMaxFile * 4);
	wofn.lpstrFile = pw2;
	wofn.nMaxFile = v0->nMaxFile * 4;
	pw3 = DuplicateMtoWBuffer(v0->lpstrFileTitle, -1, v0->nMaxFileTitle * 4);
	wofn.lpstrFileTitle = pw3;
	wofn.nMaxFileTitle = v0->nMaxFileTitle * 4;
	pw4 = DuplicateMtoW(v0->lpstrInitialDir, -1);
	wofn.lpstrInitialDir = pw4;
	pw5 = DuplicateMtoW(v0->lpstrTitle, -1);
	wofn.lpstrTitle = pw5;
	wofn.Flags = v0->Flags;
	wofn.nFileOffset = MtoW(NULL, 0, v0->lpstrFile, v0->nFileOffset);
	wofn.nFileExtension = MtoW(NULL, 0, v0->lpstrFile, v0->nFileExtension);
	pw6 = DuplicateMtoW(v0->lpstrDefExt, -1);
	wofn.lpstrDefExt = pw6;
	wofn.lCustData = v0->lCustData;
	wofn.lpfnHook = v0->lpfnHook;
	wofn.lpTemplateName = DuplicateMtoW(v0->lpTemplateName, -1);
	wofn.pvReserved = v0->pvReserved;
	wofn.FlagsEx = v0->FlagsEx;
	r = GetOpenFileNameW(&wofn);
	WtoM(v0->lpstrFile, v0->nMaxFile, wofn.lpstrFile, -1);
	TerminateStringM(v0->lpstrFile, v0->nMaxFile);
	v0->nFileOffset = WtoM(NULL, 0, wofn.lpstrFile, wofn.nFileOffset);
	v0->nFileExtension = WtoM(NULL, 0, wofn.lpstrFile, wofn.nFileExtension);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	FreeDuplicatedString(pw3);
	FreeDuplicatedString(pw4);
	FreeDuplicatedString(pw5);
	FreeDuplicatedString(pw6);
	return r;
}

BOOL GetSaveFileNameM(LPOPENFILENAMEA v0)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* pw2 = NULL;
	wchar_t* pw3 = NULL;
	wchar_t* pw4 = NULL;
	wchar_t* pw5 = NULL;
	wchar_t* pw6 = NULL;
	wchar_t* pw7 = NULL;
	wchar_t* pw8 = NULL;
	wchar_t* pw9 = NULL;
	OPENFILENAMEW wofn;
START_ROUTINE
	wofn.lStructSize = sizeof(OPENFILENAMEW);
	wofn.hwndOwner = v0->hwndOwner;
	wofn.hInstance = v0->hInstance;
	pw0 = DuplicateMtoWMultiString(v0->lpstrFilter);
	wofn.lpstrFilter = pw0;
	pw1 = DuplicateMtoWBuffer(v0->lpstrCustomFilter, -1, v0->nMaxCustFilter * 4);
	wofn.lpstrCustomFilter = pw1;
	wofn.nMaxCustFilter = v0->nMaxCustFilter * 4;
	wofn.nFilterIndex = v0->nFilterIndex;
	pw2 = DuplicateMtoWMultiStringBuffer(v0->lpstrFile, v0->nMaxFile * 4);
	wofn.lpstrFile = pw2;
	wofn.nMaxFile = v0->nMaxFile * 4;
	pw3 = DuplicateMtoWBuffer(v0->lpstrFileTitle, -1, v0->nMaxFileTitle * 4);
	wofn.lpstrFileTitle = pw3;
	wofn.nMaxFileTitle = v0->nMaxFileTitle * 4;
	pw4 = DuplicateMtoW(v0->lpstrInitialDir, -1);
	wofn.lpstrInitialDir = pw4;
	pw5 = DuplicateMtoW(v0->lpstrTitle, -1);
	wofn.lpstrTitle = pw5;
	wofn.Flags = v0->Flags;
	wofn.nFileOffset = MtoW(NULL, 0, v0->lpstrFile, v0->nFileOffset);
	wofn.nFileExtension = MtoW(NULL, 0, v0->lpstrFile, v0->nFileExtension);
	pw6 = DuplicateMtoW(v0->lpstrDefExt, -1);
	wofn.lpstrDefExt = pw6;
	wofn.lCustData = v0->lCustData;
	wofn.lpfnHook = v0->lpfnHook;
	wofn.lpTemplateName = DuplicateMtoW(v0->lpTemplateName, -1);
	wofn.pvReserved = v0->pvReserved;
	wofn.FlagsEx = v0->FlagsEx;
	r = GetSaveFileNameW(&wofn);
	WtoM(v0->lpstrFile, v0->nMaxFile, wofn.lpstrFile, -1);
	TerminateStringM(v0->lpstrFile, v0->nMaxFile);
	v0->nFileOffset = WtoM(NULL, 0, wofn.lpstrFile, wofn.nFileOffset);
	v0->nFileExtension = WtoM(NULL, 0, wofn.lpstrFile, wofn.nFileExtension);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	FreeDuplicatedString(pw3);
	FreeDuplicatedString(pw4);
	FreeDuplicatedString(pw5);
	FreeDuplicatedString(pw6);
	return r;
}

HWND HtmlHelpM(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	HWND r = NULL;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(pszFile, -1);
	r = HtmlHelpW(hwndCaller, pw0, uCommand, dwData);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

BOOL CreateProcessM(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* pw2 = NULL;
	wchar_t* pw3 = NULL;
	wchar_t* pw4 = NULL;
	wchar_t* pw5 = NULL;
	STARTUPINFOW wStartupInfo;
START_ROUTINE
	pw0 = DuplicateMtoW(lpApplicationName, -1);
	pw1 = DuplicateMtoWBuffer(lpCommandLine, -1, (strlen(lpCommandLine) + 1) * 4);
	pw2 = DuplicateMtoW(lpCurrentDirectory, -1);
	wStartupInfo.cb = sizeof(LPSTARTUPINFOW);
	pw3 = DuplicateMtoW(lpStartupInfo->lpReserved, -1);
	wStartupInfo.lpReserved = pw3;
	pw4 = DuplicateMtoW(lpStartupInfo->lpDesktop, -1);
	wStartupInfo.lpDesktop = pw4;
	pw5 = DuplicateMtoW(lpStartupInfo->lpTitle, -1);
	wStartupInfo.lpTitle = pw5;
	wStartupInfo.dwX = lpStartupInfo->dwX;
	wStartupInfo.dwY = lpStartupInfo->dwY;
	wStartupInfo.dwXSize = lpStartupInfo->dwXSize;
	wStartupInfo.dwYSize = lpStartupInfo->dwYSize;
	wStartupInfo.dwXCountChars = lpStartupInfo->dwXCountChars;
	wStartupInfo.dwYCountChars = lpStartupInfo->dwYCountChars;
	wStartupInfo.dwFillAttribute = lpStartupInfo->dwFillAttribute;
	wStartupInfo.dwFlags = lpStartupInfo->dwFlags;
	wStartupInfo.wShowWindow = lpStartupInfo->wShowWindow;
	wStartupInfo.cbReserved2 = lpStartupInfo->cbReserved2;
	wStartupInfo.lpReserved2 = lpStartupInfo->lpReserved2;
	wStartupInfo.hStdInput = lpStartupInfo->hStdInput;
	wStartupInfo.hStdOutput = lpStartupInfo->hStdOutput;
	wStartupInfo.hStdError = lpStartupInfo->hStdError;
	r = CreateProcessW(pw0, pw1, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, pw2, &wStartupInfo, lpProcessInformation);
	WtoM(lpCommandLine, strlen(lpCommandLine) + 1, pw1, -1);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	FreeDuplicatedString(pw3);
	FreeDuplicatedString(pw4);
	FreeDuplicatedString(pw5);
	return r;
}

HINSTANCE FindExecutableM(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	HINSTANCE r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* pw2 = NULL;
	wchar_t* pw3 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpFile, -1);
	pw1 = DuplicateMtoW(lpDirectory, -1);
	pw2 = AllocateStringW(MAX_PATH * 4);
	r = FindExecutableW(pw0, pw1, pw2);
	// バッファ長不明のためオーバーランの可能性あり
	WtoM(lpResult, MAX_PATH, pw2, -1);
	TerminateStringM(lpResult, MAX_PATH);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	FreeDuplicatedString(pw3);
	return r;
}

HINSTANCE ShellExecuteM(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd)
{
	HINSTANCE r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* pw2 = NULL;
	wchar_t* pw3 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpOperation, -1);
	pw1 = DuplicateMtoW(lpFile, -1);
	pw2 = DuplicateMtoW(lpParameters, -1);
	pw3 = DuplicateMtoW(lpDirectory, -1);
	r = ShellExecuteW(hwnd, pw0, pw1, pw2, pw3, nShowCmd);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	FreeDuplicatedString(pw3);
	return r;
}

PIDLIST_ABSOLUTE SHBrowseForFolderM(LPBROWSEINFOA lpbi)
{
	PIDLIST_ABSOLUTE r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	BROWSEINFOW wbi;
START_ROUTINE
	wbi.hwndOwner = lpbi->hwndOwner;
	wbi.pidlRoot = lpbi->pidlRoot;
	pw0 = DuplicateMtoWBuffer(lpbi->pszDisplayName, -1, MAX_PATH * 4);
	wbi.pszDisplayName = pw0;
	pw1 = DuplicateMtoW(lpbi->lpszTitle, -1);
	wbi.lpszTitle = pw1;
	wbi.ulFlags = lpbi->ulFlags;
	// TODO: lpfn
	wbi.lpfn = lpbi->lpfn;
	wbi.lParam = lpbi->lParam;
	wbi.iImage = lpbi->iImage;
	r = SHBrowseForFolderW(&wbi);
	// バッファ長不明のためオーバーランの可能性あり
	WtoM(lpbi->pszDisplayName, MAX_PATH, wbi.pszDisplayName, -1);
	lpbi->iImage = wbi.iImage;
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

BOOL SHGetPathFromIDListM(PCIDLIST_ABSOLUTE pidl, LPSTR pszPath)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = AllocateStringW(MAX_PATH * 4);
	r = SHGetPathFromIDListW(pidl, pw0);
	// バッファ長不明のためオーバーランの可能性あり
	WtoM(pszPath, MAX_PATH, pw0, -1);
	TerminateStringM(pszPath, MAX_PATH);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int SHFileOperationM(LPSHFILEOPSTRUCTA lpFileOp)
{
	int r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* pw2 = NULL;
	SHFILEOPSTRUCTW wFileOp;
START_ROUTINE
	wFileOp.hwnd = lpFileOp->hwnd;
	wFileOp.wFunc = lpFileOp->wFunc;
	pw0 = DuplicateMtoWMultiString(lpFileOp->pFrom);
	wFileOp.pFrom = pw0;
	pw1 = DuplicateMtoWMultiString(lpFileOp->pTo);
	wFileOp.pTo = pw1;
	wFileOp.fFlags = lpFileOp->fFlags;
	wFileOp.fAnyOperationsAborted = lpFileOp->fAnyOperationsAborted;
	wFileOp.hNameMappings = lpFileOp->hNameMappings;
	pw2 = DuplicateMtoW(lpFileOp->lpszProgressTitle, -1);
	r = SHFileOperationW(&wFileOp);
	lpFileOp->fAnyOperationsAborted = wFileOp.fAnyOperationsAborted;
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	return r;
}

BOOL AppendMenuM(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem)
{
	int r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	if(uFlags & (MF_BITMAP | MF_OWNERDRAW))
		r = AppendMenuW(hMenu, uFlags, uIDNewItem, (LPCWSTR)lpNewItem);
	else
	{
		pw0 = DuplicateMtoW(lpNewItem, -1);
		r = AppendMenuW(hMenu, uFlags, uIDNewItem, pw0);
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

BOOL GetMenuItemInfoM(HMENU hmenu, UINT item, BOOL fByPosition, LPMENUITEMINFOA lpmii)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	MENUITEMINFOW wmii;
START_ROUTINE
	wmii.cbSize = sizeof(MENUITEMINFOW);
	wmii.fMask = lpmii->fMask;
	wmii.fType = lpmii->fType;
	wmii.fState = lpmii->fState;
	wmii.wID = lpmii->wID;
	wmii.hSubMenu = lpmii->hSubMenu;
	wmii.hbmpChecked = lpmii->hbmpChecked;
	wmii.hbmpUnchecked = lpmii->hbmpUnchecked;
	wmii.dwItemData = lpmii->dwItemData;
	pw0 = DuplicateMtoWBuffer(lpmii->dwTypeData, -1, lpmii->cch * 4);
	wmii.dwTypeData = pw0;
	wmii.cch = lpmii->cch * 4;
	r = GetMenuItemInfoW(hmenu, item, fByPosition, &wmii);
	lpmii->fType = wmii.fType;
	lpmii->fState = wmii.fState;
	lpmii->wID = wmii.wID;
	lpmii->hSubMenu = wmii.hSubMenu;
	lpmii->hbmpChecked = wmii.hbmpChecked;
	lpmii->hbmpUnchecked = wmii.hbmpUnchecked;
	lpmii->dwItemData = wmii.dwItemData;
	WtoM(lpmii->dwTypeData, lpmii->cch, wmii.dwTypeData, -1);
	TerminateStringM(lpmii->dwTypeData, lpmii->cch);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

HFONT CreateFontIndirectM(CONST LOGFONTA *lplf)
{
	HFONT r = NULL;
	LOGFONTW wlf;
START_ROUTINE
	wlf.lfHeight = lplf->lfHeight;
	wlf.lfWidth = lplf->lfWidth;
	wlf.lfEscapement = lplf->lfEscapement;
	wlf.lfOrientation = lplf->lfOrientation;
	wlf.lfWeight = lplf->lfWeight;
	wlf.lfItalic = lplf->lfItalic;
	wlf.lfUnderline = lplf->lfUnderline;
	wlf.lfStrikeOut = lplf->lfStrikeOut;
	wlf.lfCharSet = lplf->lfCharSet;
	wlf.lfOutPrecision = lplf->lfOutPrecision;
	wlf.lfClipPrecision = lplf->lfClipPrecision;
	wlf.lfQuality = lplf->lfQuality;
	wlf.lfPitchAndFamily = lplf->lfPitchAndFamily;
	MtoW(wlf.lfFaceName, LF_FACESIZE, lplf->lfFaceName, -1);
	TerminateStringW(wlf.lfFaceName, LF_FACESIZE);
	r = CreateFontIndirect(&wlf);
END_ROUTINE
	return r;
}

BOOL ChooseFontM(LPCHOOSEFONTA v0)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	CHOOSEFONTW a0;
	LOGFONTW* pwlf;
START_ROUTINE
	a0.lStructSize = sizeof(CHOOSEFONTW);
	a0.hwndOwner = v0->hwndOwner;
	a0.hDC = v0->hDC;
	if(v0->lpLogFont && (pwlf = (LOGFONTW*)malloc(sizeof(LOGFONTW))))
	{
		pwlf->lfHeight = v0->lpLogFont->lfHeight;
		pwlf->lfWidth = v0->lpLogFont->lfWidth;
		pwlf->lfEscapement = v0->lpLogFont->lfEscapement;
		pwlf->lfOrientation = v0->lpLogFont->lfOrientation;
		pwlf->lfWeight = v0->lpLogFont->lfWeight;
		pwlf->lfItalic = v0->lpLogFont->lfItalic;
		pwlf->lfUnderline = v0->lpLogFont->lfUnderline;
		pwlf->lfStrikeOut = v0->lpLogFont->lfStrikeOut;
		pwlf->lfCharSet = v0->lpLogFont->lfCharSet;
		pwlf->lfOutPrecision = v0->lpLogFont->lfOutPrecision;
		pwlf->lfClipPrecision = v0->lpLogFont->lfClipPrecision;
		pwlf->lfQuality = v0->lpLogFont->lfQuality;
		pwlf->lfPitchAndFamily = v0->lpLogFont->lfPitchAndFamily;
		MtoW(pwlf->lfFaceName, LF_FACESIZE, v0->lpLogFont->lfFaceName, -1);
		TerminateStringW(pwlf->lfFaceName, LF_FACESIZE);
	}
	else
		pwlf = NULL;
	a0.lpLogFont = pwlf;
	a0.iPointSize = v0->iPointSize;
	a0.Flags = v0->Flags;
	a0.rgbColors = v0->rgbColors;
	a0.lCustData = v0->lCustData;
	a0.lpfnHook = v0->lpfnHook;
	a0.lpTemplateName = DuplicateMtoW(v0->lpTemplateName, -1);
	a0.hInstance = v0->hInstance;
	a0.lpszStyle = DuplicateMtoWBuffer(v0->lpszStyle, -1, LF_FACESIZE * 4);
	a0.nFontType = v0->nFontType;
	a0.nSizeMin = v0->nSizeMin;
	a0.nSizeMax = v0->nSizeMax;
	r = ChooseFontW(&a0);
	if(v0->lpLogFont)
	{
		v0->lpLogFont->lfHeight = pwlf->lfHeight;
		v0->lpLogFont->lfWidth = pwlf->lfWidth;
		v0->lpLogFont->lfEscapement = pwlf->lfEscapement;
		v0->lpLogFont->lfOrientation = pwlf->lfOrientation;
		v0->lpLogFont->lfWeight = pwlf->lfWeight;
		v0->lpLogFont->lfItalic = pwlf->lfItalic;
		v0->lpLogFont->lfUnderline = pwlf->lfUnderline;
		v0->lpLogFont->lfStrikeOut = pwlf->lfStrikeOut;
		v0->lpLogFont->lfCharSet = pwlf->lfCharSet;
		v0->lpLogFont->lfOutPrecision = pwlf->lfOutPrecision;
		v0->lpLogFont->lfClipPrecision = pwlf->lfClipPrecision;
		v0->lpLogFont->lfQuality = pwlf->lfQuality;
		v0->lpLogFont->lfPitchAndFamily = pwlf->lfPitchAndFamily;
		WtoM(v0->lpLogFont->lfFaceName, LF_FACESIZE, pwlf->lfFaceName, -1);
		TerminateStringM(v0->lpLogFont->lfFaceName, LF_FACESIZE);
	}
	v0->rgbColors = a0.rgbColors;
	WtoM(v0->lpszStyle, LF_FACESIZE, a0.lpszStyle, -1);
	TerminateStringM(v0->lpszStyle, LF_FACESIZE);
	v0->nFontType = a0.nFontType;
	if(pwlf)
		free(pwlf);
	FreeDuplicatedString(a0.lpTemplateName);
	FreeDuplicatedString(a0.lpszStyle);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

INT_PTR DialogBoxParamM(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	INT_PTR r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpTemplateName, -1);
	r = DialogBoxParamW(hInstance, pw0, hWndParent, lpDialogFunc, dwInitParam);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

HWND CreateDialogParamM(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	HWND r = NULL;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(lpTemplateName, -1);
	r = CreateDialogParamW(hInstance, pw0, hWndParent, lpDialogFunc, dwInitParam);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

int mkdirM(const char * _Path)
{
	int r = 0;
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
	int r = 0;
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
	int r = 0;
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
	int r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Path, -1);
	r = _wrmdir(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

size_t _mbslenM(const unsigned char * _Str)
{
	size_t r = 0;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str, -1);
	r = wcslen(pw0);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

unsigned char * _mbschrM(const unsigned char * _Str, unsigned int _Ch)
{
	unsigned char* r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* wr;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str, -1);
	// TODO: 非ASCII文字の対応
	wr = wcschr(pw0, _Ch);
	if(wr)
	{
		*wr = L'\0';
		r = _Str + WtoM(NULL, 0, pw0, -1) - 1;
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

unsigned char * _mbsrchrM(const unsigned char * _Str, unsigned int _Ch)
{
	unsigned char* r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* wr;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str, -1);
	// TODO: 非ASCII文字の対応
	wr = wcsrchr(pw0, _Ch);
	if(wr)
	{
		*wr = L'\0';
		r = _Str + WtoM(NULL, 0, pw0, -1) - 1;
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

unsigned char * _mbsstrM(const unsigned char * _Str, const unsigned char * _Substr)
{
	unsigned char* r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
	wchar_t* wr;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str, -1);
	pw1 = DuplicateMtoW(_Substr, -1);
	wr = wcsstr(pw0, pw1);
	if(wr)
	{
		*wr = L'\0';
		r = _Str + WtoM(NULL, 0, pw0, -1) - 1;
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

int _mbscmpM(const unsigned char * _Str1, const unsigned char * _Str2)
{
	int r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str1, -1);
	pw1 = DuplicateMtoW(_Str2, -1);
	r = wcscmp(pw0, pw1);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

int _mbsicmpM(const unsigned char * _Str1, const unsigned char * _Str2)
{
	int r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str1, -1);
	pw1 = DuplicateMtoW(_Str2, -1);
	r = _wcsicmp(pw0, pw1);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

int _mbsncmpM(const unsigned char * _Str1, const unsigned char * _Str2, size_t _MaxCount)
{
	int r = 0;
	wchar_t* pw0 = NULL;
	wchar_t* pw1 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str1, -1);
	pw1 = DuplicateMtoW(_Str2, -1);
	r = wcsncmp(pw0, pw1, _MaxCount);
END_ROUTINE
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	return r;
}

unsigned char * _mbslwrM(unsigned char * _String)
{
	unsigned char* r = NULL;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_String, -1);
	_wcslwr(pw0);
	r = _String;
	WtoM(_String, strlen(_String) + 1, pw0, -1);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

unsigned char * _mbsuprM(unsigned char * _String)
{
	unsigned char* r = NULL;
	wchar_t* pw0 = NULL;
START_ROUTINE
	pw0 = DuplicateMtoW(_String, -1);
	_wcsupr(pw0);
	r = _String;
	WtoM(_String, strlen(_String) + 1, pw0, -1);
END_ROUTINE
	FreeDuplicatedString(pw0);
	return r;
}

unsigned char * _mbsnincM(const unsigned char * _Str, size_t _Count)
{
	unsigned char* r = NULL;
	wchar_t* pw0 = NULL;
	wchar_t* wr;
START_ROUTINE
	pw0 = DuplicateMtoW(_Str, -1);
	wr = _wcsninc(pw0, _Count);
	if(wr)
	{
		*wr = L'\0';
		r = _Str + WtoM(NULL, 0, pw0, -1) - 1;
	}
END_ROUTINE
	FreeDuplicatedString(pw0);
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

