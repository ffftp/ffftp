// protectprocess.h
// Copyright (C) 2011 Suguru Kawamoto
// プロセスの保護

#ifndef __PROTECTPROCESS_H__
#define __PROTECTPROCESS_H__

#define ENABLE_PROCESS_PROTECTION

// 次の中から1個のみ有効にする
// フック先の関数のコードを書き換える
// 全ての呼び出しをフック可能だが原理的に二重呼び出しに対応できない
#define USE_CODE_HOOK
// フック先の関数のインポートアドレステーブルを書き換える
// 二重呼び出しが可能だが呼び出し方法によってはフックを回避される
//#define USE_IAT_HOOK

typedef HMODULE (WINAPI* _LoadLibraryA)(LPCSTR);
typedef HMODULE (WINAPI* _LoadLibraryW)(LPCWSTR);
typedef HMODULE (WINAPI* _LoadLibraryExA)(LPCSTR, HANDLE, DWORD);
typedef HMODULE (WINAPI* _LoadLibraryExW)(LPCWSTR, HANDLE, DWORD);

#ifndef DO_NOT_REPLACE

#ifdef USE_IAT_HOOK

// 変数の宣言
#define EXTERN_HOOK_FUNCTION_VAR(name) extern _##name p_##name;

#undef LoadLibraryA
#define LoadLibraryA p_LoadLibraryA
EXTERN_HOOK_FUNCTION_VAR(LoadLibraryA)
#undef LoadLibraryW
#define LoadLibraryW p_LoadLibraryW
EXTERN_HOOK_FUNCTION_VAR(LoadLibraryW)
#undef LoadLibraryExA
#define LoadLibraryExA p_LoadLibraryExA
EXTERN_HOOK_FUNCTION_VAR(LoadLibraryExA)
#undef LoadLibraryExW
#define LoadLibraryExW p_LoadLibraryExW
EXTERN_HOOK_FUNCTION_VAR(LoadLibraryExW)

#endif

#endif

HMODULE System_LoadLibrary(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
BOOL RegisterModuleMD5Hash(void* pHash);
BOOL UnregisterModuleMD5Hash(void* pHash);
BOOL FindModuleMD5Hash(void* pHash);
BOOL IsModuleTrustedA(LPCSTR Filename);
BOOL IsModuleTrustedW(LPCWSTR Filename);
BOOL InitializeLoadLibraryHook();
BOOL EnableLoadLibraryHook(BOOL bEnable);
BOOL RestartProtectedProcess(LPCTSTR Keyword);

#endif

