// protectprocess.c
// Copyright (C) 2011 Suguru Kawamoto
// プロセスの保護

// 次の中から1個のみ有効にする
// フック先の関数のコードを書き換える
#define USE_CODE_HOOK
// フック先の関数のインポートアドレステーブルを書き換える
//#define USE_IAT_HOOK

#define _CRT_SECURE_NO_WARNINGS
#include <tchar.h>
#include <windows.h>
#include <ntsecapi.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <aclapi.h>
#include <sfc.h>
#include <tlhelp32.h>
#include <imagehlp.h>

#define DO_NOT_REPLACE
#include "protectprocess.h"
#include "mbswrapper.h"

#ifdef USE_IAT_HOOK
#pragma comment(lib, "dbghelp.lib")
#endif

#ifdef USE_CODE_HOOK
#if defined(_M_IX86)
//#define HOOK_JUMP_CODE_LENGTH 5
#define HOOK_JUMP_CODE_LENGTH 7
#elif defined(_M_AMD64)
#define HOOK_JUMP_CODE_LENGTH 14
#endif
typedef struct
{
	void* pCode;
	size_t CodeLength;
	BYTE PatchCode[HOOK_JUMP_CODE_LENGTH];
	BYTE BackupCode[HOOK_JUMP_CODE_LENGTH];
} HOOK_JUMP_CODE_PATCH;
#endif
typedef struct
{
	DWORD Flags;
	LPCTSTR ModuleName;
	HMODULE hModule;
	LPCSTR ProcName;
	FARPROC Proc;
	FARPROC Hook;
	FARPROC Unhook;
#ifdef USE_CODE_HOOK
	HOOK_JUMP_CODE_PATCH Patch;
#endif
} HOOK_FUNCTION_INFO;

#define HOOK_INITIALIZED 0x00000001
#define HOOK_ENABLED 0x00000002
#define HOOK_USE_GETMODULEHANDLE 0x00000004
#define HOOK_USE_LOADLIBRARY 0x00000008
#define HOOK_USE_GETPROCADDRESS 0x00000010

typedef HMODULE (WINAPI* _LoadLibraryA)(LPCSTR);
typedef HMODULE (WINAPI* _LoadLibraryW)(LPCWSTR);
typedef HMODULE (WINAPI* _LoadLibraryExA)(LPCSTR, HANDLE, DWORD);
typedef HMODULE (WINAPI* _LoadLibraryExW)(LPCWSTR, HANDLE, DWORD);

HOOK_FUNCTION_INFO g_LoadLibraryA;
HOOK_FUNCTION_INFO g_LoadLibraryW;
HOOK_FUNCTION_INFO g_LoadLibraryExA;
HOOK_FUNCTION_INFO g_LoadLibraryExW;

typedef NTSTATUS (NTAPI* _LdrLoadDll)(LPCWSTR, DWORD*, UNICODE_STRING*, HMODULE*);
typedef NTSTATUS (NTAPI* _LdrGetDllHandle)(LPCWSTR, DWORD*, UNICODE_STRING*, HMODULE*);
typedef PIMAGE_NT_HEADERS (NTAPI* _RtlImageNtHeader)(PVOID);
typedef BOOL (WINAPI* _CryptCATAdminCalcHashFromFileHandle)(HANDLE, DWORD*, BYTE*, DWORD);

_LdrLoadDll p_LdrLoadDll;
_LdrGetDllHandle p_LdrGetDllHandle;
_RtlImageNtHeader p_RtlImageNtHeader;
_CryptCATAdminCalcHashFromFileHandle p_CryptCATAdminCalcHashFromFileHandle;

#define MAX_LOCKED_THREAD 16
#define MAX_TRUSTED_FILENAME_TABLE 16
#define MAX_TRUSTED_SHA1_HASH_TABLE 16

DWORD g_ProcessProtectionLevel;
DWORD g_LockedThread[MAX_LOCKED_THREAD];
WCHAR* g_pTrustedFilenameTable[MAX_TRUSTED_FILENAME_TABLE];
BYTE g_TrustedSHA1HashTable[MAX_TRUSTED_SHA1_HASH_TABLE][20];
WNDPROC g_PasswordEditControlProc;

#ifdef USE_CODE_HOOK
BOOL HookFunctionInCode(void* pProc, void* pHook, void** ppUnhook, HOOK_JUMP_CODE_PATCH* pPatch, BOOL bRestore)
{
	BOOL bResult;
	bResult = FALSE;
#if defined(_M_IX86)
	{
		DWORD Protect;
		BYTE* pCode;
		CHAR c;
		LONG l;
		bResult = FALSE;
		if(bRestore)
		{
			if(VirtualProtect(pPatch->pCode, pPatch->CodeLength, PAGE_EXECUTE_READWRITE, &Protect))
			{
				memcpy(pPatch->pCode, &pPatch->BackupCode, pPatch->CodeLength);
				VirtualProtect(pPatch->pCode, pPatch->CodeLength, Protect, &Protect);
				FlushInstructionCache(GetCurrentProcess(), pPatch->pCode, pPatch->CodeLength);
				bResult = TRUE;
			}
		}
		else
		{
			if(!pPatch->pCode)
			{
				pCode = (BYTE*)pProc;
				while(pCode[0] == 0xeb)
				{
					memcpy(&c, pCode + 1, 1);
					pCode = pCode + 2 + c;
				}
				if(pCode[0] == 0x8b && pCode[1] == 0xff)
				{
					pCode = pCode - 5;
					pPatch->pCode = pCode;
					pPatch->CodeLength = 7;
					memcpy(&pPatch->BackupCode, pPatch->pCode, pPatch->CodeLength);
					pPatch->PatchCode[0] = 0xe9;
					l = (long)pHook - ((long)pCode + 5);
					memcpy(&pPatch->PatchCode[1], &l, 4);
					pPatch->PatchCode[5] = 0xeb;
					pPatch->PatchCode[6] = 0xf9;
					*ppUnhook = pCode + 7;
				}
				else if(pCode[0] == 0xe9)
				{
					pPatch->pCode = pCode + 1;
					pPatch->CodeLength = 4;
					memcpy(&pPatch->BackupCode, pPatch->pCode, pPatch->CodeLength);
					l = (long)pHook - ((long)pCode + 5);
					memcpy(&pPatch->PatchCode[0], &l, 4);
					memcpy(&l, pCode + 1, 4);
					*ppUnhook = pCode + 5 + l;
				}
				else
				{
					pPatch->pCode = pCode;
					pPatch->CodeLength = 5;
					memcpy(&pPatch->BackupCode, pPatch->pCode, pPatch->CodeLength);
					pPatch->PatchCode[0] = 0xe9;
					l = (long)pHook - ((long)pCode + 5);
					memcpy(&pPatch->PatchCode[1], &l, 4);
					*ppUnhook = NULL;
				}
			}
			if(VirtualProtect(pPatch->pCode, pPatch->CodeLength, PAGE_EXECUTE_READWRITE, &Protect))
			{
				memcpy(pPatch->pCode, &pPatch->PatchCode, pPatch->CodeLength);
				VirtualProtect(pPatch->pCode, pPatch->CodeLength, Protect, &Protect);
				FlushInstructionCache(GetCurrentProcess(), pPatch->pCode, pPatch->CodeLength);
				bResult = TRUE;
			}
		}
	}
#elif defined(_M_AMD64)
	{
		DWORD Protect;
		BYTE* pCode;
		CHAR c;
		LONG l;
		LONGLONG ll;
		bResult = FALSE;
		if(bRestore)
		{
			if(VirtualProtect(pPatch->pCode, pPatch->CodeLength, PAGE_EXECUTE_READWRITE, &Protect))
			{
				memcpy(pPatch->pCode, &pPatch->BackupCode, pPatch->CodeLength);
				VirtualProtect(pPatch->pCode, pPatch->CodeLength, Protect, &Protect);
				FlushInstructionCache(GetCurrentProcess(), pPatch->pCode, pPatch->CodeLength);
				bResult = TRUE;
			}
		}
		else
		{
			if(!pPatch->pCode)
			{
				pCode = (BYTE*)pProc;
				if(pCode[0] == 0x48)
					pCode = pCode + 1;
				while(pCode[0] == 0xeb || pCode[0] == 0xe9)
				{
					if(pCode[0] == 0xeb)
					{
						memcpy(&c, pCode + 1, 1);
						pCode = pCode + 2 + c;
					}
					else
					{
						memcpy(&l, pCode + 1, 4);
						pCode = pCode + 5 + l;
					}
					if(pCode[0] == 0x48)
						pCode++;
				}
				if(pCode[0] == 0xff && pCode[1] == 0x25)
				{
					memcpy(&l, pCode + 2, 4);
					pPatch->pCode = pCode + 6 + l;
					pPatch->CodeLength = 8;
					memcpy(&pPatch->BackupCode, pPatch->pCode, pPatch->CodeLength);
					memcpy(&pPatch->PatchCode[0], &pHook, 8);
					memcpy(&ll, pCode + 6 + l, 8);
					*ppUnhook = (void*)ll;
				}
				else
				{
					pPatch->pCode = pCode;
					pPatch->CodeLength = 14;
					memcpy(&pPatch->BackupCode, pPatch->pCode, pPatch->CodeLength);
					pPatch->PatchCode[0] = 0xff;
					pPatch->PatchCode[1] = 0x25;
					l = 0;
					memcpy(&pPatch->PatchCode[2], &l, 4);
					memcpy(&pPatch->PatchCode[6], &pHook, 8);
					*ppUnhook = NULL;
				}
			}
			if(VirtualProtect(pPatch->pCode, pPatch->CodeLength, PAGE_EXECUTE_READWRITE, &Protect))
			{
				memcpy(pPatch->pCode, &pPatch->PatchCode, pPatch->CodeLength);
				VirtualProtect(pPatch->pCode, pPatch->CodeLength, Protect, &Protect);
				FlushInstructionCache(GetCurrentProcess(), pPatch->pCode, pPatch->CodeLength);
				bResult = TRUE;
			}
		}
	}
#endif
	return bResult;
}
#endif

#ifdef USE_IAT_HOOK
BOOL HookFunctionInIAT(void* pProc, void* pHook, void** ppUnhook)
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
							if((void*)pitd->u1.Function == pProc)
							{
								bFound = TRUE;
								if(VirtualProtect(&pitd->u1.Function, sizeof(void*), PAGE_EXECUTE_READWRITE, &Protect))
								{
									memcpy(&pitd->u1.Function, &pHook, sizeof(void*));
									VirtualProtect(&pitd->u1.Function, sizeof(void*), Protect, &Protect);
									*ppUnhook = pProc;
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

BOOL InitializeHookFunction(HOOK_FUNCTION_INFO* pInfo)
{
	BOOL bResult;
	bResult = FALSE;
	if(!(pInfo->Flags & HOOK_INITIALIZED))
	{
		if(pInfo->Flags & HOOK_USE_GETMODULEHANDLE)
			pInfo->hModule = GetModuleHandle(pInfo->ModuleName);
		if(pInfo->Flags & HOOK_USE_LOADLIBRARY)
			pInfo->hModule = LoadLibrary(pInfo->ModuleName);
		if(pInfo->Flags & HOOK_USE_GETPROCADDRESS)
			pInfo->Proc = GetProcAddress(pInfo->hModule, pInfo->ProcName);
		if(pInfo->Proc)
		{
			pInfo->Flags |= HOOK_INITIALIZED;
			bResult = TRUE;
		}
	}
	return bResult;
}

void UninitializeHookFunction(HOOK_FUNCTION_INFO* pInfo)
{
	if(pInfo->Flags & HOOK_INITIALIZED)
	{
		if(pInfo->Flags & HOOK_USE_LOADLIBRARY)
			FreeLibrary(pInfo->hModule);
		pInfo->Flags &= ~HOOK_INITIALIZED;
	}
}

BOOL EnableHookFunction(HOOK_FUNCTION_INFO* pInfo, BOOL bEnable)
{
	BOOL bResult;
	bResult = FALSE;
	if(pInfo->Flags & HOOK_INITIALIZED)
	{
		if(bEnable)
		{
			if(!(pInfo->Flags & HOOK_ENABLED))
			{
#ifdef USE_CODE_HOOK
				if(HookFunctionInCode(pInfo->Proc, pInfo->Hook, (void**)&pInfo->Unhook, &pInfo->Patch, FALSE))
				{
					pInfo->Flags |= HOOK_ENABLED;
					bResult = TRUE;
				}
#endif
#ifdef USE_IAT_HOOK
				if(HookFunctionInIAT(pInfo->Proc, pInfo->Hook, (void**)&pInfo->Unhook))
				{
					pInfo->Flags |= HOOK_ENABLED;
					bResult = TRUE;
				}
#endif
			}
		}
		else
		{
			if(pInfo->Flags & HOOK_ENABLED)
			{
#ifdef USE_CODE_HOOK
				if(HookFunctionInCode(pInfo->Proc, pInfo->Hook, (void**)&pInfo->Unhook, &pInfo->Patch, TRUE))
				{
					pInfo->Flags &= ~HOOK_ENABLED;
					bResult = TRUE;
				}
#endif
#ifdef USE_IAT_HOOK
				if(HookFunctionInIAT(pInfo->Hook, pInfo->Proc, (void**)&pInfo->Unhook))
				{
					pInfo->Flags &= ~HOOK_ENABLED;
					bResult = TRUE;
				}
#endif
			}
		}
	}
	return bResult;
}

BOOL LockThreadLock()
{
	BOOL bResult;
	DWORD ThreadId;
	DWORD i;
	bResult = FALSE;
	ThreadId = GetCurrentThreadId();
	i = 0;
	while(i < MAX_LOCKED_THREAD)
	{
		if(g_LockedThread[i] == ThreadId)
			break;
		i++;
	}
	if(i >= MAX_LOCKED_THREAD)
	{
		i = 0;
		while(i < MAX_LOCKED_THREAD)
		{
			if(g_LockedThread[i] == 0)
			{
				g_LockedThread[i] = ThreadId;
				bResult = TRUE;
				break;
			}
			i++;
		}
	}
	return bResult;
}

BOOL UnlockThreadLock()
{
	BOOL bResult;
	DWORD ThreadId;
	DWORD i;
	bResult = FALSE;
	ThreadId = GetCurrentThreadId();
	i = 0;
	while(i < MAX_LOCKED_THREAD)
	{
		if(g_LockedThread[i] == ThreadId)
		{
			g_LockedThread[i] = 0;
			bResult = TRUE;
			break;
		}
		i++;
	}
	return bResult;
}

// ファイルを変更不能に設定
HANDLE LockExistingFile(LPCWSTR Filename)
{
	HANDLE hResult;
	hResult = NULL;
	if((hResult = CreateFileW(Filename, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL)) == INVALID_HANDLE_VALUE)
		hResult = NULL;
	return hResult;
}

// DLLのハッシュを検索
BOOL FindTrustedModuleSHA1Hash(void* pHash)
{
	BOOL bResult;
	int i;
	bResult = FALSE;
	i = 0;
	while(i < MAX_TRUSTED_SHA1_HASH_TABLE)
	{
		if(memcmp(&g_TrustedSHA1HashTable[i], pHash, 20) == 0)
		{
			bResult = TRUE;
			break;
		}
		i++;
	}
	return bResult;
}

BOOL VerifyFileSignature_Function(LPCWSTR Filename)
{
	BOOL bResult;
	HCERTSTORE hStore;
	PCCERT_CONTEXT pcc;
	CERT_CHAIN_PARA ccp;
	CERT_CHAIN_CONTEXT* pccc;
	CERT_CHAIN_POLICY_PARA ccpp;
	CERT_CHAIN_POLICY_STATUS ccps;
	bResult = FALSE;
	if(CryptQueryObject(CERT_QUERY_OBJECT_FILE, Filename, CERT_QUERY_CONTENT_FLAG_ALL, CERT_QUERY_FORMAT_FLAG_ALL, 0, NULL, NULL, NULL, &hStore, NULL, NULL))
	{
		pcc = NULL;
		while(!bResult && (pcc = CertEnumCertificatesInStore(hStore, pcc)))
		{
			ZeroMemory(&ccp, sizeof(CERT_CHAIN_PARA));
			ccp.cbSize = sizeof(CERT_CHAIN_PARA);
			if(CertGetCertificateChain(NULL, pcc, NULL, NULL, &ccp, 0, NULL, &pccc))
			{
				ZeroMemory(&ccpp, sizeof(CERT_CHAIN_POLICY_PARA));
				ccpp.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);
				if(g_ProcessProtectionLevel & PROCESS_PROTECTION_EXPIRED)
					ccpp.dwFlags |= CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG;
				else if(g_ProcessProtectionLevel & PROCESS_PROTECTION_UNAUTHORIZED)
					ccpp.dwFlags |= CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG;
				ZeroMemory(&ccps, sizeof(CERT_CHAIN_POLICY_STATUS));
				ccps.cbSize = sizeof(CERT_CHAIN_POLICY_STATUS);
				if(CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_AUTHENTICODE, pccc, &ccpp, &ccps))
				{
					if(ccps.dwError == ERROR_SUCCESS)
					{
						bResult = TRUE;
						break;
					}
				}
				CertFreeCertificateChain(pccc);
			}
		}
		while(pcc = CertEnumCertificatesInStore(hStore, pcc))
		{
		}
		CertCloseStore(hStore, 0);
	}
	return bResult;
}

// ファイルの署名を確認
BOOL VerifyFileSignature(LPCWSTR Filename)
{
	BOOL bResult;
	GUID g = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_FILE_INFO wfi;
	WINTRUST_DATA wd;
	bResult = FALSE;
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
	else
		bResult = VerifyFileSignature_Function(Filename);
	return bResult;
}

// ファイルの署名をカタログファイルで確認
BOOL VerifyFileSignatureInCatalog(LPCWSTR Catalog, LPCWSTR Filename)
{
	BOOL bResult;
	GUID g = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_CATALOG_INFO wci;
	WINTRUST_DATA wd;
	bResult = FALSE;
	if(VerifyFileSignature(Catalog))
	{
		ZeroMemory(&wci, sizeof(WINTRUST_CATALOG_INFO));
		wci.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
		wci.pcwszCatalogFilePath = Catalog;
		wci.pcwszMemberFilePath = Filename;
		if((wci.hMemberFile = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
		{
			p_CryptCATAdminCalcHashFromFileHandle(wci.hMemberFile, &wci.cbCalculatedFileHash, NULL, 0);
			if(wci.pbCalculatedFileHash = (BYTE*)malloc(wci.cbCalculatedFileHash))
			{
				if(p_CryptCATAdminCalcHashFromFileHandle(wci.hMemberFile, &wci.cbCalculatedFileHash, wci.pbCalculatedFileHash, 0))
				{
					ZeroMemory(&wd, sizeof(WINTRUST_DATA));
					wd.cbStruct = sizeof(WINTRUST_DATA);
					wd.dwUIChoice = WTD_UI_NONE;
					wd.dwUnionChoice = WTD_CHOICE_CATALOG;
					wd.pCatalog = &wci;
					if(WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &g, &wd) == ERROR_SUCCESS)
						bResult = TRUE;
				}
				free(wci.pbCalculatedFileHash);
			}
			CloseHandle(wci.hMemberFile);
		}
	}
	return bResult;
}

BOOL WINAPI GetSHA1HashOfModule_Function(DIGEST_HANDLE refdata, PBYTE pData, DWORD dwLength)
{
	return CryptHashData(*(HCRYPTHASH*)refdata, pData, dwLength, 0);
}

// モジュールのSHA1ハッシュを取得
// マニフェストファイルのfile要素のhash属性は実行可能ファイルの場合にImageGetDigestStreamで算出される
BOOL GetSHA1HashOfModule(LPCWSTR Filename, void* pHash)
{
	BOOL bResult;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HANDLE hFile;
	DWORD dw;
	bResult = FALSE;
	if(CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, 0) || CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		if(CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
		{
			if((hFile = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
			{
				if(ImageGetDigestStream(hFile, CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO, GetSHA1HashOfModule_Function, (DIGEST_HANDLE)&hHash))
				{
					dw = 20;
					if(CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)pHash, &dw, 0))
						bResult = TRUE;
				}
				CloseHandle(hFile);
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
	return bResult;
}

BOOL IsSxsModuleTrusted_Function(LPCWSTR Catalog, LPCWSTR Manifest, LPCWSTR Module)
{
	BOOL bResult;
	HANDLE hLock0;
	HANDLE hLock1;
	BYTE Hash[20];
	int i;
	static char HexTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	char HashHex[41];
	HANDLE hFile;
	DWORD Size;
	char* pData;
	DWORD dw;
	bResult = FALSE;
	if(hLock0 = LockExistingFile(Catalog))
	{
		if(hLock1 = LockExistingFile(Manifest))
		{
			if(VerifyFileSignatureInCatalog(Catalog, Manifest))
			{
				if(GetSHA1HashOfModule(Module, &Hash))
				{
					for(i = 0; i < 20; i++)
					{
						HashHex[i * 2] = HexTable[(Hash[i] >> 4) & 0x0f];
						HashHex[i * 2 + 1] = HexTable[Hash[i] & 0x0f];
					}
					HashHex[i * 2] = '\0';
					if((hFile = CreateFileW(Manifest, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
					{
						Size = GetFileSize(hFile, NULL);
						if(pData = (char*)VirtualAlloc(NULL, Size + 1, MEM_COMMIT, PAGE_READWRITE))
						{
							VirtualLock(pData, Size + 1);
							if(ReadFile(hFile, pData, Size, &dw, NULL))
							{
								pData[dw] = '\0';
								if(strstr(pData, HashHex))
									bResult = TRUE;
							}
							VirtualUnlock(pData, Size + 1);
							VirtualFree(pData, Size + 1, MEM_DECOMMIT);
						}
						CloseHandle(hFile);
					}
				}
			}
			CloseHandle(hLock1);
		}
		CloseHandle(hLock0);
	}
	return bResult;
}

// サイドバイサイドDLLを確認
// パスは"%SystemRoot%\WinSxS"以下を想定
// 以下のファイルが存在するものとする
// "\xxx\yyy.dll"、"\manifests\xxx.cat"、"\manifests\xxx.manifest"のセット（XPの全てのDLL、Vista以降の一部のDLL）
// "\xxx\yyy.dll"、"\catalogs\zzz.cat"、"\manifests\xxx.manifest"のセット（Vista以降のほとんどのDLL）
// 署名されたカタログファイルを用いてマニフェストファイルが改竄されていないことを確認
// ハッシュ値は	マニフェストファイルのfile要素のhash属性に記述されているものを用いる
// マニフェストファイル内にSHA1ハッシュ値の16進数表記を直接検索しているが確率的に問題なし
BOOL IsSxsModuleTrusted(LPCWSTR Filename)
{
	BOOL bResult;
	wchar_t* pw0;
	wchar_t* pw1;
	wchar_t* pw2;
	wchar_t* pw3;
	wchar_t* pw4;
	wchar_t* pw5;
	wchar_t* p;
	HANDLE hFind;
	WIN32_FIND_DATAW wfd;
	bResult = FALSE;
	if(pw0 = AllocateStringW(wcslen(Filename) + 1))
	{
		wcscpy(pw0, Filename);
		if(p = wcsrchr(pw0, L'\\'))
		{
			wcscpy(p, L"");
			if(p = wcsrchr(pw0, L'\\'))
			{
				p++;
				if(pw1 = AllocateStringW(wcslen(p) + 1))
				{
					wcscpy(pw1, p);
					wcscpy(p, L"");
					if(pw2 = AllocateStringW(wcslen(pw0) + wcslen(L"manifests\\") + wcslen(pw1) + wcslen(L".cat") + 1))
					{
						wcscpy(pw2, pw0);
						wcscat(pw2, L"manifests\\");
						wcscat(pw2, pw1);
						if(pw3 = AllocateStringW(wcslen(pw2) + wcslen(L".manifest") + 1))
						{
							wcscpy(pw3, pw2);
							wcscat(pw3, L".manifest");
							wcscat(pw2, L".cat");
							if(IsSxsModuleTrusted_Function(pw2, pw3, Filename))
								bResult = TRUE;
							FreeDuplicatedString(pw3);
						}
						FreeDuplicatedString(pw2);
					}
					if(!bResult)
					{
						if(pw2 = AllocateStringW(wcslen(pw0) + wcslen(L"catalogs\\") + 1))
						{
							if(pw3 = AllocateStringW(wcslen(pw0) + wcslen(L"manifests\\") + wcslen(pw1) + wcslen(L".manifest") + 1))
							{
								wcscpy(pw2, pw0);
								wcscat(pw2, L"catalogs\\");
								wcscpy(pw3, pw0);
								wcscat(pw3, L"manifests\\");
								wcscat(pw3, pw1);
								wcscat(pw3, L".manifest");
								if(pw4 = AllocateStringW(wcslen(pw2) + wcslen(L"*.cat") + 1))
								{
									wcscpy(pw4, pw2);
									wcscat(pw4, L"*.cat");
									if((hFind = FindFirstFileW(pw4, &wfd)) != INVALID_HANDLE_VALUE)
									{
										do
										{
											if(pw5 = AllocateStringW(wcslen(pw2) + wcslen(wfd.cFileName) + 1))
											{
												wcscpy(pw5, pw2);
												wcscat(pw5, wfd.cFileName);
												if(IsSxsModuleTrusted_Function(pw5, pw3, Filename))
													bResult = TRUE;
												FreeDuplicatedString(pw5);
											}
										}
										while(!bResult && FindNextFileW(hFind, &wfd));
										FindClose(hFind);
									}
									FreeDuplicatedString(pw4);
								}
								FreeDuplicatedString(pw3);
							}
							FreeDuplicatedString(pw2);
						}
					}
					FreeDuplicatedString(pw1);
				}
			}
		}
		FreeDuplicatedString(pw0);
	}
	return bResult;
}

// DLLを確認
BOOL IsModuleTrusted(LPCWSTR Filename)
{
	BOOL bResult;
	BYTE Hash[20];
	bResult = FALSE;
	if(LockThreadLock())
	{
		if(GetSHA1HashOfFile(Filename, &Hash))
		{
			if(FindTrustedModuleSHA1Hash(&Hash))
				bResult = TRUE;
		}
		if(!bResult)
		{
			if((g_ProcessProtectionLevel & PROCESS_PROTECTION_BUILTIN) && VerifyFileSignature(Filename))
				bResult = TRUE;
		}
		if(!bResult)
		{
			if((g_ProcessProtectionLevel & PROCESS_PROTECTION_SYSTEM_FILE) && SfcIsFileProtected(NULL, Filename))
				bResult = TRUE;
		}
		if(!bResult)
		{
			if((g_ProcessProtectionLevel & PROCESS_PROTECTION_SIDE_BY_SIDE) && IsSxsModuleTrusted(Filename))
				bResult = TRUE;
		}
		UnlockThreadLock();
	}
	return bResult;
}

// 以下フック関数

HMODULE WINAPI h_LoadLibraryA(LPCSTR lpLibFileName)
{
	HMODULE r = NULL;
	wchar_t* pw0 = NULL;
	if(pw0 = DuplicateAtoW(lpLibFileName, -1))
		r = LoadLibraryExW(pw0, NULL, 0);
	FreeDuplicatedString(pw0);
	return r;
}

HMODULE WINAPI h_LoadLibraryW(LPCWSTR lpLibFileName)
{
	HMODULE r = NULL;
	r = LoadLibraryExW(lpLibFileName, NULL, 0);
	return r;
}

HMODULE WINAPI h_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE r = NULL;
	wchar_t* pw0 = NULL;
	if(pw0 = DuplicateAtoW(lpLibFileName, -1))
		r = LoadLibraryExW(pw0, hFile, dwFlags);
	FreeDuplicatedString(pw0);
	return r;
}

HMODULE WINAPI h_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE r = NULL;
	BOOL bTrusted;
	wchar_t* pw0;
	HANDLE hLock;
	HMODULE hModule;
	DWORD Length;
	bTrusted = FALSE;
	pw0 = NULL;
	hLock = NULL;
//	if(dwFlags & (DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE))
	if(dwFlags & (DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE | 0x00000020 | 0x00000040))
		bTrusted = TRUE;
	if(!bTrusted)
	{
		if(hModule = System_LoadLibrary(lpLibFileName, NULL, DONT_RESOLVE_DLL_REFERENCES))
		{
			Length = MAX_PATH;
			if(pw0 = AllocateStringW(Length))
			{
				if(GetModuleFileNameW(hModule, pw0, Length) > 0)
				{
					while(pw0)
					{
						if(GetModuleFileNameW(hModule, pw0, Length) + 1 <= Length)
						{
							lpLibFileName = pw0;
							break;
						}
						Length = Length * 2;
						FreeDuplicatedString(pw0);
						pw0 = AllocateStringW(Length);
					}
				}
			}
			hLock = LockExistingFile(lpLibFileName);
			FreeLibrary(hModule);
		}
		if((g_ProcessProtectionLevel & PROCESS_PROTECTION_LOADED) && GetModuleHandleW(lpLibFileName))
			bTrusted = TRUE;
	}
	if(!bTrusted)
	{
		if(hLock)
		{
			if(IsModuleTrusted(lpLibFileName))
				bTrusted = TRUE;
		}
	}
	if(bTrusted)
		r = System_LoadLibrary(lpLibFileName, hFile, dwFlags);
	FreeDuplicatedString(pw0);
	if(hLock)
		CloseHandle(hLock);
	return r;
}

// kernel32.dllのLoadLibraryExW相当の関数
// ドキュメントが無いため詳細は不明
// 一部のウィルス対策ソフト（Avast!等）がLdrLoadDllをフックしているためLdrLoadDllを書き換えるべきではない
// カーネルモードのコードに対しては効果なし
// SeDebugPrivilegeが使用可能なユーザーに対しては効果なし
HMODULE System_LoadLibrary(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE r = NULL;
	UNICODE_STRING us;
	HANDLE hDataFile;
	HANDLE hMapping;
	DWORD DllFlags;
	us.Length = sizeof(wchar_t) * (USHORT)wcslen(lpLibFileName);
	us.MaximumLength = sizeof(wchar_t) * ((USHORT)wcslen(lpLibFileName) + 1);
	us.Buffer = (PWSTR)lpLibFileName;
//	if(dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE))
	if(dwFlags & (LOAD_LIBRARY_AS_DATAFILE | 0x00000040))
	{
//		if(p_LdrGetDllHandle(NULL, NULL, &us, &r) == STATUS_SUCCESS)
		if(p_LdrGetDllHandle(NULL, NULL, &us, &r) == 0)
		{
//			dwFlags &= ~(LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
			dwFlags &= ~(LOAD_LIBRARY_AS_DATAFILE | 0x00000040);
			dwFlags |= DONT_RESOLVE_DLL_REFERENCES;
		}
		else
		{
//			if(dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE)
			if(dwFlags & 0x00000040)
				hDataFile = CreateFileW(lpLibFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			else
				hDataFile = CreateFileW(lpLibFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
			if(hDataFile != INVALID_HANDLE_VALUE)
			{
				if(hMapping = CreateFileMappingW(hDataFile, NULL, PAGE_READONLY, 0, 0, NULL))
				{
					if(r = (HMODULE)MapViewOfFileEx(hMapping, FILE_MAP_READ, 0, 0, 0, NULL))
					{
						if(p_RtlImageNtHeader(r))
							r = (HMODULE)((size_t)r | 1);
						else
						{
							UnmapViewOfFile(r);
							r = NULL;
						}
					}
					CloseHandle(hMapping);
				}
				CloseHandle(hDataFile);
			}
			else
			{
//				dwFlags &= ~(LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
				dwFlags &= ~(LOAD_LIBRARY_AS_DATAFILE | 0x00000040);
				dwFlags |= DONT_RESOLVE_DLL_REFERENCES;
			}
		}
	}
//	if(!(dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE)))
	if(!(dwFlags & (LOAD_LIBRARY_AS_DATAFILE | 0x00000040)))
	{
		DllFlags = 0;
//		if(dwFlags & (DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_IMAGE_RESOURCE))
		if(dwFlags & (DONT_RESOLVE_DLL_REFERENCES | 0x00000020))
			DllFlags |= 0x00000002;
//		if(p_LdrLoadDll(NULL, &DllFlags, &us, &r) == STATUS_SUCCESS)
		if(p_LdrLoadDll(NULL, &DllFlags, &us, &r) == 0)
		{
		}
		else
			r = NULL;
	}
	return r;
}

void SetProcessProtectionLevel(DWORD Level)
{
	g_ProcessProtectionLevel = Level;
}

// メモリのSHA1ハッシュを取得
BOOL GetSHA1HashOfMemory(const void* pData, DWORD Size, void* pHash)
{
	BOOL bResult;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	DWORD dw;
	bResult = FALSE;
	if(CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, 0) || CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		if(CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
		{
			if(CryptHashData(hHash, (BYTE*)pData, Size, 0))
			{
				dw = 20;
				if(CryptGetHashParam(hHash, HP_HASHVAL, (BYTE*)pHash, &dw, 0))
					bResult = TRUE;
			}
			CryptDestroyHash(hHash);
		}
		CryptReleaseContext(hProv, 0);
	}
	return bResult;
}

// ファイルのSHA1ハッシュを取得
BOOL GetSHA1HashOfFile(LPCWSTR Filename, void* pHash)
{
	BOOL bResult;
	HANDLE hFile;
	DWORD Size;
	void* pData;
	DWORD dw;
	bResult = FALSE;
	if((hFile = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		Size = GetFileSize(hFile, NULL);
		if(pData = VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE))
		{
			VirtualLock(pData, Size);
			if(ReadFile(hFile, pData, Size, &dw, NULL))
			{
				if(GetSHA1HashOfMemory(pData, Size, pHash))
					bResult = TRUE;
			}
			VirtualUnlock(pData, Size);
			VirtualFree(pData, Size, MEM_DECOMMIT);
		}
		CloseHandle(hFile);
	}
	return bResult;
}

// DLLのハッシュを登録
BOOL RegisterTrustedModuleSHA1Hash(void* pHash)
{
	BOOL bResult;
	BYTE NullHash[20] = {0};
	int i;
	bResult = FALSE;
	if(FindTrustedModuleSHA1Hash(pHash))
		bResult = TRUE;
	else
	{
		i = 0;
		while(i < MAX_TRUSTED_SHA1_HASH_TABLE)
		{
			if(memcmp(&g_TrustedSHA1HashTable[i], &NullHash, 20) == 0)
			{
				memcpy(&g_TrustedSHA1HashTable[i], pHash, 20);
				bResult = TRUE;
				break;
			}
			i++;
		}
	}
	return bResult;
}

// DLLのハッシュの登録を解除
BOOL UnregisterTrustedModuleSHA1Hash(void* pHash)
{
	BOOL bResult;
	BYTE NullHash[20] = {0};
	int i;
	bResult = FALSE;
	i = 0;
	while(i < MAX_TRUSTED_SHA1_HASH_TABLE)
	{
		if(memcmp(&g_TrustedSHA1HashTable[i], pHash, 20) == 0)
		{
			memcpy(&g_TrustedSHA1HashTable[i], &NullHash, 20);
			bResult = TRUE;
			break;
		}
		i++;
	}
	return bResult;
}

// 信頼できないDLLをアンロード
BOOL UnloadUntrustedModule()
{
	BOOL bResult;
	wchar_t* pw0;
	HANDLE hSnapshot;
	MODULEENTRY32 me;
	DWORD Length;
	bResult = FALSE;
	pw0 = NULL;
	if((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId())) != INVALID_HANDLE_VALUE)
	{
		bResult = TRUE;
		me.dwSize = sizeof(MODULEENTRY32);
		if(Module32First(hSnapshot, &me))
		{
			do
			{
				Length = MAX_PATH;
				FreeDuplicatedString(pw0);
				if(pw0 = AllocateStringW(Length))
				{
					if(GetModuleFileNameW(me.hModule, pw0, Length) > 0)
					{
						while(pw0)
						{
							if(GetModuleFileNameW(me.hModule, pw0, Length) + 1 <= Length)
								break;
							Length = Length * 2;
							FreeDuplicatedString(pw0);
							pw0 = AllocateStringW(Length);
						}
					}
				}
				if(pw0)
				{
					if(!IsModuleTrusted(pw0))
					{
						if(me.hModule != GetModuleHandleW(NULL))
						{
							while(FreeLibrary(me.hModule))
							{
							}
							if(GetModuleFileNameW(me.hModule, pw0, Length) > 0)
							{
								bResult = FALSE;
								break;
							}
						}
					}
				}
				else
				{
					bResult = FALSE;
					break;
				}
			}
			while(Module32Next(hSnapshot, &me));
		}
		CloseHandle(hSnapshot);
	}
	FreeDuplicatedString(pw0);
	return bResult;
}

// 関数ポインタを使用可能な状態に初期化
BOOL InitializeLoadLibraryHook()
{
	BOOL bResult;
	HMODULE hModule;
	bResult = TRUE;
	memset(&g_LoadLibraryA, 0, sizeof(HOOK_FUNCTION_INFO));
	g_LoadLibraryA.Flags = HOOK_USE_GETMODULEHANDLE | HOOK_USE_GETPROCADDRESS;
	g_LoadLibraryA.ModuleName = _T("kernel32.dll");
	g_LoadLibraryA.ProcName = "LoadLibraryA";
	g_LoadLibraryA.Hook = (FARPROC)h_LoadLibraryA;
	if(!InitializeHookFunction(&g_LoadLibraryA))
		bResult = FALSE;
	memset(&g_LoadLibraryW, 0, sizeof(HOOK_FUNCTION_INFO));
	g_LoadLibraryW.Flags = HOOK_USE_GETMODULEHANDLE | HOOK_USE_GETPROCADDRESS;
	g_LoadLibraryW.ModuleName = _T("kernel32.dll");
	g_LoadLibraryW.ProcName = "LoadLibraryW";
	g_LoadLibraryW.Hook = (FARPROC)h_LoadLibraryW;
	if(!InitializeHookFunction(&g_LoadLibraryW))
		bResult = FALSE;
	memset(&g_LoadLibraryExA, 0, sizeof(HOOK_FUNCTION_INFO));
	g_LoadLibraryExA.Flags = HOOK_USE_GETMODULEHANDLE | HOOK_USE_GETPROCADDRESS;
	g_LoadLibraryExA.ModuleName = _T("kernel32.dll");
	g_LoadLibraryExA.ProcName = "LoadLibraryExA";
	g_LoadLibraryExA.Hook = (FARPROC)h_LoadLibraryExA;
	if(!InitializeHookFunction(&g_LoadLibraryExA))
		bResult = FALSE;
	memset(&g_LoadLibraryExW, 0, sizeof(HOOK_FUNCTION_INFO));
	g_LoadLibraryExW.Flags = HOOK_USE_GETMODULEHANDLE | HOOK_USE_GETPROCADDRESS;
	g_LoadLibraryExW.ModuleName = _T("kernel32.dll");
	g_LoadLibraryExW.ProcName = "LoadLibraryExW";
	g_LoadLibraryExW.Hook = (FARPROC)h_LoadLibraryExW;
	if(!InitializeHookFunction(&g_LoadLibraryExW))
		bResult = FALSE;
	if(!(hModule = GetModuleHandleW(L"ntdll.dll")))
		bResult = FALSE;
	if(!(p_LdrLoadDll = (_LdrLoadDll)GetProcAddress(hModule, "LdrLoadDll")))
		bResult = FALSE;
	if(!(p_LdrGetDllHandle = (_LdrGetDllHandle)GetProcAddress(hModule, "LdrGetDllHandle")))
		bResult = FALSE;
	if(!(p_RtlImageNtHeader = (_RtlImageNtHeader)GetProcAddress(hModule, "RtlImageNtHeader")))
		bResult = FALSE;
	if(!(hModule = LoadLibraryW(L"wintrust.dll")))
		bResult = FALSE;
	if(!(p_CryptCATAdminCalcHashFromFileHandle = (_CryptCATAdminCalcHashFromFileHandle)GetProcAddress(hModule, "CryptCATAdminCalcHashFromFileHandle")))
		bResult = FALSE;
	// バグ対策
	ImageGetDigestStream(NULL, 0, NULL, NULL);
	return bResult;
}

// SetWindowsHookEx対策
// DLL Injectionされた場合は上のh_LoadLibrary系関数でトラップ可能
BOOL EnableLoadLibraryHook(BOOL bEnable)
{
	BOOL bResult;
	bResult = TRUE;
	if(!EnableHookFunction(&g_LoadLibraryA, bEnable))
		bResult = FALSE;
	if(!EnableHookFunction(&g_LoadLibraryW, bEnable))
		bResult = FALSE;
	if(!EnableHookFunction(&g_LoadLibraryExA, bEnable))
		bResult = FALSE;
	if(!EnableHookFunction(&g_LoadLibraryExW, bEnable))
		bResult = FALSE;
	return bResult;
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

INT_PTR CALLBACK PasswordEditControlWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case EM_GETPASSWORDCHAR:
		break;
	case EM_SETPASSWORDCHAR:
		break;
	default:
		return CallWindowProcW(g_PasswordEditControlProc, hWnd, Msg, wParam, lParam);
	}
	return 0;
}

BOOL ProtectPasswordEditControl(HWND hWnd)
{
	BOOL bResult;
	WCHAR ClassName[MAX_PATH];
	WNDPROC Proc;
	bResult = FALSE;
	if(g_ProcessProtectionLevel & PROCESS_PROTECTION_PASSWORD_EDIT)
	{
		if(GetClassNameW(hWnd, ClassName, MAX_PATH) > 0)
		{
			if(_wcsicmp(ClassName, WC_EDITW) == 0)
			{
				Proc = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_WNDPROC);
				if(Proc != (WNDPROC)PasswordEditControlWndProc)
				{
					g_PasswordEditControlProc = Proc;
					SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)PasswordEditControlWndProc);
					bResult = TRUE;
				}
			}
		}
	}
	return bResult;
}

BOOL CALLBACK ProtectAllEditControlsEnumChildProc(HWND hwnd , LPARAM lParam)
{
	ProtectPasswordEditControl(hwnd);
	return TRUE;
}

BOOL ProtectAllEditControls(HWND hWnd)
{
	if(g_ProcessProtectionLevel & PROCESS_PROTECTION_PASSWORD_EDIT)
		EnumChildWindows(hWnd, ProtectAllEditControlsEnumChildProc, 0);
	return TRUE;
}

