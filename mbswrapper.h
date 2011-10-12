// mbswrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー

#ifndef __MBSWRAPPER_H__
#define __MBSWRAPPER_H__

#include <stdio.h>
#include <windows.h>
#include <shlobj.h>

#ifndef DO_NOT_REPLACE

#undef WinMain
#define WinMain WinMainM
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);
#undef LoadLibrary
#define LoadLibrary LoadLibraryM
HMODULE LoadLibraryM(LPCSTR lpLibFileName);
#undef CreateFile
#define CreateFile CreateFileM
HANDLE CreateFileM(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#undef MessageBox
#define MessageBox MessageBoxM
int MessageBoxM(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
#undef FindFirstFile
#define FindFirstFile FindFirstFileM
HANDLE FindFirstFileM(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
#undef FindNextFile
#define FindNextFile FindNextFileM
BOOL FindNextFileM(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
#undef GetLogicalDriveStrings
#define GetLogicalDriveStrings GetLogicalDriveStringsM
DWORD GetLogicalDriveStringsM(DWORD nBufferLength, LPSTR lpBuffer);
#undef RegisterClassEx
#define RegisterClassEx RegisterClassExM
ATOM RegisterClassExM(CONST WNDCLASSEXA * v0);
#undef CreateWindowEx
#define CreateWindowEx CreateWindowExM
HWND CreateWindowExM(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
#undef GetWindowLong
#define GetWindowLong GetWindowLongM
LONG GetWindowLongM(HWND hWnd, int nIndex);
#undef SetWindowLong
#define SetWindowLong SetWindowLongM
LONG SetWindowLongM(HWND hWnd, int nIndex, LONG dwNewLong);
#undef DefWindowProc
#define DefWindowProc DefWindowProcM
LRESULT DefWindowProcM(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#undef CallWindowProc
#define CallWindowProc CallWindowProcM
LRESULT CallWindowProcM(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#undef SendMessage
#define SendMessage SendMessageM
LRESULT SendMessageM(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#undef DefDlgProc
#define DefDlgProc DefDlgProcM
LRESULT DefDlgProcM(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#undef SendDlgItemMessage
#define SendDlgItemMessage SendDlgItemMessageM
LRESULT SendDlgItemMessageM(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
#undef SetWindowText
#define SetWindowText SetWindowTextM
BOOL SetWindowTextM(HWND hWnd, LPCSTR lpString);
#undef DragQueryFile
#define DragQueryFile DragQueryFileM
UINT DragQueryFileM(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cch);
#undef GetCommandLine
#define GetCommandLine GetCommandLineM
LPSTR GetCommandLineM();
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
#undef RegOpenKeyEx
#define RegOpenKeyEx RegOpenKeyExM
LSTATUS RegOpenKeyExM(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
#undef RegCreateKeyEx
#define RegCreateKeyEx RegCreateKeyExM
LSTATUS RegCreateKeyExM(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, CONST LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
#undef RegDeleteValue
#define RegDeleteValue RegDeleteValueM
LSTATUS RegDeleteValueM(HKEY hKey, LPCSTR lpValueName);
#undef RegQueryValueEx
#define RegQueryValueEx RegQueryValueExM
LSTATUS RegQueryValueExM(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
#undef RegSetValueEx
#define RegSetValueEx RegSetValueExM
LSTATUS RegSetValueExM(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE* lpData, DWORD cbData);
#undef TextOut
#define TextOut TextOutM
BOOL TextOutM(HDC hdc, int x, int y, LPCSTR lpString, int c);
#undef GetTextExtentPoint32
#define GetTextExtentPoint32 GetTextExtentPoint32M
BOOL GetTextExtentPoint32M(HDC hdc, LPCSTR lpString, int c, LPSIZE psizl);
#undef PropertySheet
#define PropertySheet PropertySheetM
INT_PTR PropertySheetM(LPCPROPSHEETHEADERA v0);
#undef GetOpenFileName
#define GetOpenFileName GetOpenFileNameM
BOOL GetOpenFileNameM(LPOPENFILENAMEA v0);
#undef GetSaveFileName
#define GetSaveFileName GetSaveFileNameM
BOOL GetSaveFileNameM(LPOPENFILENAMEA v0);
#undef HtmlHelp
#define HtmlHelp HtmlHelpM
HWND HtmlHelpM(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);
#undef CreateProcess
#define CreateProcess CreateProcessM
BOOL CreateProcessM(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
#undef FindExecutable
#define FindExecutable FindExecutableM
HINSTANCE FindExecutableM(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult);
#undef ShellExecute
#define ShellExecute ShellExecuteM
HINSTANCE ShellExecuteM(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
#undef SHBrowseForFolder
#define SHBrowseForFolder SHBrowseForFolderM
PIDLIST_ABSOLUTE SHBrowseForFolderM(LPBROWSEINFOA lpbi);
#undef SHGetPathFromIDList
#define SHGetPathFromIDList SHGetPathFromIDListM
BOOL SHGetPathFromIDListM(PCIDLIST_ABSOLUTE pidl, LPSTR pszPath);
#undef SHFileOperation
#define SHFileOperation SHFileOperationM
int SHFileOperationM(LPSHFILEOPSTRUCTA lpFileOp);
#undef AppendMenu
#define AppendMenu AppendMenuM
BOOL AppendMenuM(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCSTR lpNewItem);
#undef GetMenuItemInfo
#define GetMenuItemInfo GetMenuItemInfoM
BOOL GetMenuItemInfoM(HMENU hmenu, UINT item, BOOL fByPosition, LPMENUITEMINFOA lpmii);
#undef CreateFontIndirect
#define CreateFontIndirect CreateFontIndirectM
HFONT CreateFontIndirectM(CONST LOGFONTA *lplf);
#undef ChooseFont
#define ChooseFont ChooseFontM
BOOL ChooseFontM(LPCHOOSEFONTA v0);
#undef DialogBoxParam
#define DialogBoxParam DialogBoxParamM
INT_PTR DialogBoxParamM(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
#undef CreateDialogParam
#define CreateDialogParam CreateDialogParamM
HWND CreateDialogParamM(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
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

#undef CreateWindow
#define CreateWindow(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) CreateWindowEx(0L, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
#undef DialogBox
#define DialogBox(hInstance, lpTemplate, hWndParent, lpDialogFunc) DialogBoxParam(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)

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
void FreeDuplicatedString(void* p);

int WINAPI WinMainM(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif

