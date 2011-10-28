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

// ロード済みのモジュールは検査をパス
#define PROCESS_PROTECTION_LOADED 0x00000001
// モジュールに埋め込まれたAuthenticode署名を検査
#define PROCESS_PROTECTION_BUILTIN 0x00000002
// サイドバイサイドのAuthenticode署名を検査
#define PROCESS_PROTECTION_SIDE_BY_SIDE 0x00000004
// WFPによる保護下にあるかを検査
#define PROCESS_PROTECTION_SYSTEM_FILE 0x00000008
// Authenticode署名の有効期限を無視
#define PROCESS_PROTECTION_EXPIRED 0x00000010
// Authenticode署名の発行元を無視
#define PROCESS_PROTECTION_UNAUTHORIZED 0x00000020

#define PROCESS_PROTECTION_NONE 0
#define PROCESS_PROTECTION_DEFAULT PROCESS_PROTECTION_HIGH
#define PROCESS_PROTECTION_HIGH (PROCESS_PROTECTION_BUILTIN | PROCESS_PROTECTION_SIDE_BY_SIDE | PROCESS_PROTECTION_SYSTEM_FILE)
#define PROCESS_PROTECTION_MEDIUM (PROCESS_PROTECTION_HIGH | PROCESS_PROTECTION_LOADED | PROCESS_PROTECTION_EXPIRED)
#define PROCESS_PROTECTION_LOW (PROCESS_PROTECTION_MEDIUM | PROCESS_PROTECTION_UNAUTHORIZED)

HMODULE System_LoadLibrary(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
void SetProcessProtectionLevel(DWORD Level);
BOOL GetSHA1HashOfFile(LPCWSTR Filename, void* pHash);
BOOL RegisterTrustedModuleSHA1Hash(void* pHash);
BOOL UnregisterTrustedModuleSHA1Hash(void* pHash);
BOOL UnloadUntrustedModule();
BOOL InitializeLoadLibraryHook();
BOOL EnableLoadLibraryHook(BOOL bEnable);
BOOL RestartProtectedProcess(LPCTSTR Keyword);

#endif

