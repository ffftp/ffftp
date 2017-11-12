// protectprocess.h
// Copyright (C) 2011 Suguru Kawamoto
// プロセスの保護

#ifndef __PROTECTPROCESS_H__
#define __PROTECTPROCESS_H__

#define ENABLE_PROCESS_PROTECTION

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
// パスワード入力コントロールを保護
#define PROCESS_PROTECTION_PASSWORD_EDIT 0x00000040

#define PROCESS_PROTECTION_NONE 0
#define PROCESS_PROTECTION_DEFAULT PROCESS_PROTECTION_HIGH
#define PROCESS_PROTECTION_HIGH (PROCESS_PROTECTION_BUILTIN | PROCESS_PROTECTION_SIDE_BY_SIDE | PROCESS_PROTECTION_SYSTEM_FILE | PROCESS_PROTECTION_PASSWORD_EDIT)
#define PROCESS_PROTECTION_MEDIUM (PROCESS_PROTECTION_HIGH | PROCESS_PROTECTION_LOADED | PROCESS_PROTECTION_EXPIRED | PROCESS_PROTECTION_PASSWORD_EDIT)
#define PROCESS_PROTECTION_LOW (PROCESS_PROTECTION_MEDIUM | PROCESS_PROTECTION_UNAUTHORIZED | PROCESS_PROTECTION_PASSWORD_EDIT)

HMODULE System_LoadLibrary(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
void SetProcessProtectionLevel(DWORD Level);
BOOL GetSHA1HashOfMemory(const void* pData, DWORD Size, void* pHash);
BOOL GetSHA1HashOfFile(LPCWSTR Filename, void* pHash);
BOOL RegisterTrustedModuleSHA1Hash(void* pHash);
BOOL UnregisterTrustedModuleSHA1Hash(void* pHash);
BOOL UnloadUntrustedModule();
BOOL InitializeLoadLibraryHook();
BOOL EnableLoadLibraryHook(BOOL bEnable);
BOOL RestartProtectedProcess(LPCTSTR Keyword);
BOOL ProtectPasswordEditControl(HWND hWnd);
BOOL ProtectAllEditControls(HWND hWnd);

#endif

