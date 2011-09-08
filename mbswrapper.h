// mbswrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// マルチバイト文字ワイド文字APIラッパー

#ifndef __MBSWRAPPER_H__
#define __MBSWRAPPER_H__

#ifndef DO_NOT_REPLACE

#undef CreateFile
#define CreateFile CreateFileM
#undef MessageBox
#define MessageBox MessageBoxM
#undef FindFirstFile
#define FindFirstFile FindFirstFileM
#undef FindNextFile
#define FindNextFile FindNextFileM
#undef GetLogicalDriveStrings
#define GetLogicalDriveStrings GetLogicalDriveStringsM
#undef CreateWindowEx
#define CreateWindowEx CreateWindowExM
#undef SendMessage
#define SendMessage SendMessageM
#undef SendDlgItemMessage
#define SendDlgItemMessage SendDlgItemMessageM
#undef SetWindowText
#define SetWindowText SetWindowTextM
#undef DragQueryFile
#define DragQueryFile DragQueryFileM
#undef GetCurrentDirectory
#define GetCurrentDirectory GetCurrentDirectoryM
#undef SetCurrentDirectory
#define SetCurrentDirectory SetCurrentDirectoryM
#undef SetDllDirectory
#define SetDllDirectory SetDllDirectoryM
#undef GetFileAttributes
#define GetFileAttributes GetFileAttributesM
#undef GetModuleFileName
#define GetModuleFileName GetModuleFileNameM
#undef RegOpenKeyEx
#define RegOpenKeyEx RegOpenKeyExM
#undef RegCreateKeyEx
#define RegCreateKeyEx RegCreateKeyExM
#undef RegDeleteValue
#define RegDeleteValue RegDeleteValueM
#undef RegQueryValueEx
#define RegQueryValueEx RegQueryValueExM
#undef RegSetValueEx
#define RegSetValueEx RegSetValueExM
#undef TextOut
#define TextOut TextOutM
#undef GetTextExtentPoint32
#define GetTextExtentPoint32 GetTextExtentPoint32M
#undef PropertySheet
#define PropertySheet PropertySheetM
#undef GetOpenFileName
#define GetOpenFileName GetOpenFileNameM
#undef GetSaveFileName
#define GetSaveFileName GetSaveFileNameM
#undef HtmlHelp
#define HtmlHelp HtmlHelpM
#undef ShellExecute
#define ShellExecute ShellExecuteM
#undef SHBrowseForFolder
#define SHBrowseForFolder SHBrowseForFolderM
#undef SHGetPathFromIDList
#define SHGetPathFromIDList SHGetPathFromIDListM
#undef SHFileOperation
#define SHFileOperation SHFileOperationM
#undef AppendMenu
#define AppendMenu AppendMenuM
#undef GetMenuItemInfo
#define GetMenuItemInfo GetMenuItemInfoM
#undef CreateFontIndirect
#define CreateFontIndirect CreateFontIndirectM
#undef ChooseFont
#define ChooseFont ChooseFontM

#undef CreateWindow
#define CreateWindow(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) CreateWindowEx(0L, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)

#endif

int MtoW(LPWSTR pDst, int size, LPCSTR pSrc, int count);
int WtoM(LPSTR pDst, int size, LPCWSTR pSrc, int count);
int WtoA(LPSTR pDst, int size, LPCWSTR pSrc, int count);
int TerminateStringM(LPSTR lpString, int size);
int TerminateStringW(LPWSTR lpString, int size);
size_t GetMultiStringLengthM(LPCSTR lpString);
size_t GetMultiStringLengthW(LPCWSTR lpString);
char* AllocateStringM(int size);
wchar_t* AllocateStringW(int size);
char* AllocateStringA(int size);
wchar_t* DuplicateMtoW(LPCSTR lpString, int c);
wchar_t* DuplicateMtoWBuffer(LPCSTR lpString, int c, int size);
wchar_t* DuplicateMtoWMultiString(LPCSTR lpString);
wchar_t* DuplicateMtoWMultiStringBuffer(LPCSTR lpString, int size);
char* DuplicateWtoM(LPCWSTR lpString, int c);
char* DuplicateWtoA(LPCWSTR lpString, int c);
void FreeDuplicatedString(void* p);

#endif

