// protectprocess.c
// Copyright (C) 2011 Suguru Kawamoto
// プロセスの保護

// 次の中から1個のみ有効にする
// フック先の関数のコードを書き換える
// 全ての呼び出しをフック可能だが原理的に二重呼び出しに対応できない
#define USE_CODE_HOOK
// フック先の関数のインポートアドレステーブルを書き換える
// 二重呼び出しが可能だが呼び出し方法によってはフックを回避される
//#define USE_IAT_HOOK

// フック対象の関数名 %s
// フック対象の型 _%s
// フック対象のポインタ p_%s
// フック用の関数名 h_%s
// フック対象のコードのバックアップ c_%s

#define _WIN32_WINNT 0x0600

#include <tchar.h>
#include <windows.h>
#include <ntsecapi.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <aclapi.h>
#include <sfc.h>
#ifdef USE_IAT_HOOK
#include <tlhelp32.h>
#include <dbghelp.h>
#endif

#define DO_NOT_REPLACE
#include "protectprocess.h"
#include "mbswrapper.h"

#ifdef USE_IAT_HOOK
#pragma comment(lib, "dbghelp.lib")
#endif

#ifdef USE_CODE_HOOK
#if defined(_X86_)
#define HOOK_JUMP_CODE_LENGTH 5
#elif defined(_AMD64_)
#define HOOK_JUMP_CODE_LENGTH 14
#endif
#endif

BOOL HookFunctionInCode(void* pOriginal, void* pNew, void* pBackupCode, BOOL bRestore);

// 変数の宣言
#ifdef USE_CODE_HOOK
#define HOOK_FUNCTION_VAR(name) _##name p_##name;BYTE c_##name[HOOK_JUMP_CODE_LENGTH * 2];
#endif
#ifdef USE_IAT_HOOK
#define HOOK_FUNCTION_VAR(name) _##name p_##name;
#endif
// 関数ポインタを取得
#define GET_FUNCTION(h, name) p_##name = (_##name)GetProcAddress(h, #name)
// フック対象のコードを置換してフックを開始
#define SET_HOOK_FUNCTION(name) HookFunctionInCode(p_##name, h_##name, &c_##name, FALSE)
// フック対象を呼び出す前に対象のコードを復元
#define START_HOOK_FUNCTION(name) HookFunctionInCode(p_##name, h_##name, &c_##name, TRUE)
// フック対象を呼び出した後に対象のコードを置換
#define END_HOOK_FUNCTION(name) HookFunctionInCode(p_##name, h_##name, NULL, FALSE)

HOOK_FUNCTION_VAR(LoadLibraryA)
HOOK_FUNCTION_VAR(LoadLibraryW)
HOOK_FUNCTION_VAR(LoadLibraryExA)
HOOK_FUNCTION_VAR(LoadLibraryExW)

// ドキュメントが無いため原因は不明だが第2引数はポインタでないとエラーになる場合がある
//typedef NTSTATUS (WINAPI* _LdrLoadDll)(LPCWSTR, DWORD, UNICODE_STRING*, HMODULE*);
typedef NTSTATUS (WINAPI* _LdrLoadDll)(LPCWSTR, DWORD*, UNICODE_STRING*, HMODULE*);
// ドキュメントが無いため原因は不明だが第2引数はポインタでないとエラーになる場合がある
//typedef NTSTATUS (WINAPI* _LdrGetDllHandle)(LPCWSTR, DWORD, UNICODE_STRING*, HMODULE*);
typedef NTSTATUS (WINAPI* _LdrGetDllHandle)(LPCWSTR, DWORD*, UNICODE_STRING*, HMODULE*);
typedef NTSTATUS (WINAPI* _LdrAddRefDll)(DWORD, HMODULE);

_LdrLoadDll p_LdrLoadDll;
_LdrGetDllHandle p_LdrGetDllHandle;
_LdrAddRefDll p_LdrAddRefDll;

#define MAX_MD5_HASH_TABLE 16

BYTE g_MD5HashTable[MAX_MD5_HASH_TABLE][16];

// 以下フック関数
// フック対象を呼び出す場合は前後でSTART_HOOK_FUNCTIONとEND_HOOK_FUNCTIONを実行する必要がある

HMODULE WINAPI h_LoadLibraryA(LPCSTR lpLibFileName)
{
	HMODULE r = NULL;
	if(GetModuleHandleA(lpLibFileName) || IsModuleTrustedA(lpLibFileName))
	{
		wchar_t* pw0 = NULL;
		pw0 = DuplicateAtoW(lpLibFileName, -1);
		r = System_LoadLibrary(pw0, NULL, 0);
		FreeDuplicatedString(pw0);
	}
	return r;
}

HMODULE WINAPI h_LoadLibraryW(LPCWSTR lpLibFileName)
{
	HMODULE r = NULL;
	if(GetModuleHandleW(lpLibFileName) || IsModuleTrustedW(lpLibFileName))
		r = System_LoadLibrary(lpLibFileName, NULL, 0);
	return r;
}

HMODULE WINAPI h_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE r = NULL;
	if(GetModuleHandleA(lpLibFileName) || IsModuleTrustedA(lpLibFileName))
	{
		wchar_t* pw0 = NULL;
		pw0 = DuplicateAtoW(lpLibFileName, -1);
		r = System_LoadLibrary(pw0, hFile, dwFlags);
		FreeDuplicatedString(pw0);
	}
	return r;
}

HMODULE WINAPI h_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE r = NULL;
	if(GetModuleHandleW(lpLibFileName) || IsModuleTrustedW(lpLibFileName))
		r = System_LoadLibrary(lpLibFileName, hFile, dwFlags);
	return r;
}

// 以下ヘルパー関数

BOOL GetMD5HashOfFile(LPCWSTR Filename, void* pHash)
{
	BOOL bResult;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HANDLE hFile;
	DWORD Size;
	void* pData;
	DWORD dw;
	bResult = FALSE;
	if(CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, 0) || CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		if(CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
		{
			if((hFile = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
			{
				Size = GetFileSize(hFile, NULL);
				if(pData = VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE))
				{
					VirtualLock(pData, Size);
					if(ReadFile(hFile, pData, Size, &dw, NULL))
					{
						if(CryptHashData(hHash, (BYTE*)pData, Size, 0))
						{
							dw = 16;
							if(CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)pHash, &dw, 0))
								bResult = TRUE;
						}
					}
					VirtualUnlock(pData, Size);
					VirtualFree(pData, Size, MEM_DECOMMIT);
				}
				CloseHandle(hFile);
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
	return bResult;
}

#ifdef USE_CODE_HOOK
BOOL HookFunctionInCode(void* pOriginal, void* pNew, void* pBackupCode, BOOL bRestore)
{
	BOOL bResult;
	DWORD Protect;
#if defined(_X86_)
	BYTE JumpCode[HOOK_JUMP_CODE_LENGTH] = {0xe9, 0x00, 0x00, 0x00, 0x00};
	size_t Relative;
	Relative = (size_t)pNew - (size_t)pOriginal - HOOK_JUMP_CODE_LENGTH;
	memcpy(&JumpCode[1], &Relative, 4);
	bResult = FALSE;
	if(bRestore)
	{
		if(VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, PAGE_EXECUTE_READWRITE, &Protect))
		{
			memcpy(pOriginal, pBackupCode, HOOK_JUMP_CODE_LENGTH);
			VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, Protect, &Protect);
			bResult = TRUE;
		}
	}
	else
	{
		if(pBackupCode)
			memcpy(pBackupCode, pOriginal, HOOK_JUMP_CODE_LENGTH);
		if(VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, PAGE_EXECUTE_READWRITE, &Protect))
		{
			memcpy(pOriginal, &JumpCode, HOOK_JUMP_CODE_LENGTH);
			VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, Protect, &Protect);
			bResult = TRUE;
		}
	}
#elif defined(_AMD64_)
	BYTE JumpCode[HOOK_JUMP_CODE_LENGTH] = {0xff, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	size_t Absolute;
	Absolute = (size_t)pOriginal;
	memcpy(&JumpCode[6], &Absolute, 8);
	bResult = FALSE;
	if(bRestore)
	{
		if(VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, PAGE_EXECUTE_READWRITE, &Protect))
		{
			memcpy(pOriginal, pBackupCode, HOOK_JUMP_CODE_LENGTH);
			VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, Protect, &Protect);
			bResult = TRUE;
		}
	}
	else
	{
		if(pBackupCode)
			memcpy(pBackupCode, pOriginal, HOOK_JUMP_CODE_LENGTH);
		if(VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, PAGE_EXECUTE_READWRITE, &Protect))
		{
			memcpy(pOriginal, &JumpCode, HOOK_JUMP_CODE_LENGTH);
			VirtualProtect(pOriginal, HOOK_JUMP_CODE_LENGTH, Protect, &Protect);
			bResult = TRUE;
		}
	}
#endif
	return bResult;
}
#endif

#ifdef USE_IAT_HOOK
BOOL HookFunctionInIAT(void* pOriginal, void* pNew)
{
	BOOL bResult;
	HANDLE hSnapshot;
	MODULEENTRY32 me;
	BOOL bFound;
	IMAGE_IMPORT_DESCRIPTOR* piid;
	ULONG Size;
	IMAGE_THUNK_DATA* pitd;
	DWORD Protect;
	bResult = FALSE;
	if((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId())) != INVALID_HANDLE_VALUE)
	{
		me.dwSize = sizeof(MODULEENTRY32);
		if(Module32First(hSnapshot, &me))
		{
			bFound = FALSE;
			do
			{
				if(piid = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToData(me.hModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size))
				{
					while(!bFound && piid->Name != 0)
					{
						pitd = (IMAGE_THUNK_DATA*)((BYTE*)me.hModule + piid->FirstThunk);
						while(!bFound && pitd->u1.Function != 0)
						{
							if((void*)pitd->u1.Function == pOriginal)
							{
								bFound = TRUE;
								if(VirtualProtect(&pitd->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &Protect))
								{
									memcpy(&pitd->u1.Function, &pNew, sizeof(void*));
									VirtualProtect(&pitd->u1.Function, sizeof(void*), Protect, &Protect);
									bResult = TRUE;
								}
							}
							pitd++;
						}
						piid++;
					}
				}
			}
			while(!bFound && Module32Next(hSnapshot, &me));
		}
		CloseHandle(hSnapshot);
	}
	return bResult;
}
#endif

// kernel32.dllのLoadLibraryExW相当の関数
HMODULE System_LoadLibrary(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE r = NULL;
	UNICODE_STRING us;
	us.Length = sizeof(wchar_t) * wcslen(lpLibFileName);
	us.MaximumLength = sizeof(wchar_t) * (wcslen(lpLibFileName) + 1);
	us.Buffer = (PWSTR)lpLibFileName;
	if(dwFlags & LOAD_LIBRARY_AS_DATAFILE)
	{
//		if(p_LdrGetDllHandle(NULL, dwFlags, &us, &r) == 0)
		if(p_LdrGetDllHandle(NULL, &dwFlags, &us, &r) == 0)
		{
			if(p_LdrAddRefDll)
				p_LdrAddRefDll(0, r);
		}
		else
		{
			dwFlags |= DONT_RESOLVE_DLL_REFERENCES;
//			if(p_LdrLoadDll(NULL, dwFlags, &us, &r) == 0)
			if(p_LdrLoadDll(NULL, &dwFlags, &us, &r) == 0)
			{
			}
			else
				r = NULL;
		}
	}
	else
	{
//		if(p_LdrGetDllHandle(NULL, dwFlags, &us, &r) == 0)
		if(p_LdrGetDllHandle(NULL, &dwFlags, &us, &r) == 0)
		{
			if(p_LdrAddRefDll)
				p_LdrAddRefDll(0, r);
		}
//		else if(p_LdrLoadDll(NULL, dwFlags, &us, &r) == 0)
		else if(p_LdrLoadDll(NULL, &dwFlags, &us, &r) == 0)
		{
		}
		else
			r = NULL;
	}
	return r;
}

// DLLのハッシュを登録
BOOL RegisterModuleMD5Hash(void* pHash)
{
	BOOL bResult;
	BYTE NullHash[16] = {0};
	int i;
	bResult = FALSE;
	if(FindModuleMD5Hash(pHash))
		bResult = TRUE;
	else
	{
		i = 0;
		while(i < MAX_MD5_HASH_TABLE)
		{
			if(memcmp(&g_MD5HashTable[i], &NullHash, 16) == 0)
			{
				memcpy(&g_MD5HashTable[i], pHash, 16);
				bResult = TRUE;
				break;
			}
			i++;
		}
	}
	return bResult;
}

// DLLのハッシュの登録を解除
BOOL UnregisterModuleMD5Hash(void* pHash)
{
	BOOL bResult;
	BYTE NullHash[16] = {0};
	int i;
	bResult = FALSE;
	i = 0;
	while(i < MAX_MD5_HASH_TABLE)
	{
		if(memcmp(&g_MD5HashTable[i], pHash, 16) == 0)
		{
			memcpy(&g_MD5HashTable[i], &NullHash, 16);
			bResult = TRUE;
			break;
		}
		i++;
	}
	return bResult;
}

// DLLのハッシュを検索
BOOL FindModuleMD5Hash(void* pHash)
{
	BOOL bResult;
	int i;
	bResult = FALSE;
	i = 0;
	while(i < MAX_MD5_HASH_TABLE)
	{
		if(memcmp(&g_MD5HashTable[i], pHash, 16) == 0)
		{
			bResult = TRUE;
			break;
		}
		i++;
	}
	return bResult;
}

// DLLを確認
// ハッシュが登録されている、Authenticode署名がされている、またはWFPによる保護下にあることを確認
BOOL IsModuleTrustedA(LPCSTR Filename)
{
	BOOL r = FALSE;
	wchar_t* pw0 = NULL;
	pw0 = DuplicateAtoW(Filename, -1);
	r = IsModuleTrustedW(pw0);
	FreeDuplicatedString(pw0);
	return r;
}

// DLLを確認
// ハッシュが登録されている、Authenticode署名がされている、またはWFPによる保護下にあることを確認
BOOL IsModuleTrustedW(LPCWSTR Filename)
{
	BOOL bResult;
	WCHAR Path[MAX_PATH];
	LPWSTR p;
	BYTE Hash[16];
	GUID g = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_FILE_INFO wfi;
	WINTRUST_DATA wd;
	bResult = FALSE;
	if(wcsrchr(Filename, '.') > wcsrchr(Filename, '\\'))
	{
		if(SearchPathW(NULL, Filename, NULL, MAX_PATH, Path, &p) > 0)
			Filename = Path;
	}
	else
	{
		if(SearchPathW(NULL, Filename, L".dll", MAX_PATH, Path, &p) > 0)
			Filename = Path;
	}
	if(GetMD5HashOfFile(Filename, &Hash))
	{
		if(FindModuleMD5Hash(&Hash))
			bResult = TRUE;
	}
	if(!bResult)
	{
		ZeroMemory(&wfi, sizeof(WINTRUST_FILE_INFO));
		wfi.cbStruct = sizeof(WINTRUST_FILE_INFO);
		wfi.pcwszFilePath = Filename;
		ZeroMemory(&wd, sizeof(WINTRUST_DATA));
		wd.cbStruct = sizeof(WINTRUST_DATA);
		wd.dwUIChoice = WTD_UI_NONE;
		wd.dwUnionChoice = WTD_CHOICE_FILE;
		wd.pFile = &wfi;
		if(WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &g, &wd) == ERROR_SUCCESS)
			bResult = TRUE;
	}
	if(!bResult)
	{
		if(SfcIsFileProtected(NULL, Filename))
			bResult = TRUE;
	}
//	if(!bResult)
//	{
//		WCHAR Temp[MAX_PATH + 128];
//		_swprintf(Temp, L"Untrusted module was detected! \"%s\"\n", Filename);
//		OutputDebugStringW(Temp);
//	}
	return bResult;
}

// 関数ポインタを使用可能な状態に初期化
BOOL InitializeLoadLibraryHook()
{
	HMODULE hModule;
	hModule = GetModuleHandleW(L"kernel32.dll");
	GET_FUNCTION(hModule, LoadLibraryA);
	GET_FUNCTION(hModule, LoadLibraryW);
	GET_FUNCTION(hModule, LoadLibraryExA);
	GET_FUNCTION(hModule, LoadLibraryExW);
	hModule = GetModuleHandleW(L"ntdll.dll");
	GET_FUNCTION(hModule, LdrLoadDll);
	GET_FUNCTION(hModule, LdrGetDllHandle);
	GET_FUNCTION(hModule, LdrAddRefDll);
	return TRUE;
}

// SetWindowsHookEx対策
// DLL Injectionされた場合は上のh_LoadLibrary系関数でトラップ可能
BOOL EnableLoadLibraryHook(BOOL bEnable)
{
	if(bEnable)
	{
		// 検証に必要なDLLの遅延読み込み回避
		IsModuleTrustedA("");
#ifdef USE_CODE_HOOK
		SET_HOOK_FUNCTION(LoadLibraryA);
		SET_HOOK_FUNCTION(LoadLibraryW);
		SET_HOOK_FUNCTION(LoadLibraryExA);
		SET_HOOK_FUNCTION(LoadLibraryExW);
#endif
#ifdef USE_IAT_HOOK
		HookFunctionInIAT(p_LoadLibraryA, h_LoadLibraryA);
		HookFunctionInIAT(p_LoadLibraryW, h_LoadLibraryW);
		HookFunctionInIAT(p_LoadLibraryExA, h_LoadLibraryExA);
		HookFunctionInIAT(p_LoadLibraryExW, h_LoadLibraryExW);
#endif
	}
	else
	{
#ifdef USE_CODE_HOOK
		END_HOOK_FUNCTION(LoadLibraryA);
		END_HOOK_FUNCTION(LoadLibraryW);
		END_HOOK_FUNCTION(LoadLibraryExA);
		END_HOOK_FUNCTION(LoadLibraryExW);
#endif
#ifdef USE_IAT_HOOK
		HookFunctionInIAT(h_LoadLibraryA, p_LoadLibraryA);
		HookFunctionInIAT(h_LoadLibraryW, p_LoadLibraryW);
		HookFunctionInIAT(h_LoadLibraryExA, p_LoadLibraryExA);
		HookFunctionInIAT(h_LoadLibraryExW, p_LoadLibraryExW);
#endif
	}
	return TRUE;
}

// ReadProcessMemory、WriteProcessMemory、CreateRemoteThread対策
// TerminateProcessのみ許可
BOOL RestartProtectedProcess(LPCTSTR Keyword)
{
	BOOL bResult;
	ACL* pACL;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
	PSID pSID;
	SECURITY_DESCRIPTOR sd;
	TCHAR* CommandLine;
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	bResult = FALSE;
	if(_tcslen(GetCommandLine()) >= _tcslen(Keyword) && _tcscmp(GetCommandLine() + _tcslen(GetCommandLine()) - _tcslen(Keyword), Keyword) == 0)
		return FALSE;
	if(pACL = (ACL*)malloc(sizeof(ACL) + 1024))
	{
		if(InitializeAcl(pACL, sizeof(ACL) + 1024, ACL_REVISION))
		{
			if(AllocateAndInitializeSid(&sia, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSID))
			{
				if(AddAccessAllowedAce(pACL, ACL_REVISION, PROCESS_TERMINATE, pSID))
				{
					if(InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
					{
						if(SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE))
						{
							if(CommandLine = (TCHAR*)malloc(sizeof(TCHAR) * (_tcslen(GetCommandLine()) + _tcslen(Keyword) + 1)))
							{
								_tcscpy(CommandLine, GetCommandLine());
								_tcscat(CommandLine, Keyword);
								sa.nLength = sizeof(SECURITY_ATTRIBUTES);
								sa.lpSecurityDescriptor = &sd;
								sa.bInheritHandle = FALSE;
								GetStartupInfo(&si);
								if(CreateProcess(NULL, CommandLine, &sa, NULL, FALSE, 0, NULL, NULL, &si, &pi))
								{
									CloseHandle(pi.hThread);
									CloseHandle(pi.hProcess);
									bResult = TRUE;
								}
								free(CommandLine);
							}
						}
					}
				}
				FreeSid(pSID);
			}
		}
		free(pACL);
	}
	return bResult;
}

