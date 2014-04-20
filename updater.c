// updater.c
// Copyright (C) 2014 Suguru Kawamoto
// ソフトウェア自動更新

#include <tchar.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
typedef SOCKADDR_STORAGE_XP SOCKADDR_STORAGE;
typedef SOCKADDR_STORAGE *PSOCKADDR_STORAGE, FAR *LPSOCKADDR_STORAGE;
#include <winhttp.h>

#include "updater.h"
#include "socketwrapper.h"
#include "protectprocess.h"
#include "mbswrapper.h"

typedef struct
{
	BYTE Signature[64];
	BYTE ListHash[64];
} UPDATE_HASH;

BOOL DownloadFileViaHTTP(void* pOut, DWORD Length, DWORD* pLength, LPCWSTR UserAgent, LPCWSTR ServerName, LPCWSTR ObjectName)
{
	BOOL bResult;
	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;
	bResult = FALSE;
	if(hSession = WinHttpOpen(UserAgent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0))
	{
		if(hConnect = WinHttpConnect(hSession, ServerName, INTERNET_DEFAULT_HTTP_PORT, 0))
		{
			if(hRequest = WinHttpOpenRequest(hConnect, L"GET", ObjectName, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0))
			{
				if(WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
				{
					if(WinHttpReceiveResponse(hRequest, NULL))
					{
						if(WinHttpQueryDataAvailable(hRequest, pLength))
						{
							if(*pLength <= Length)
							{
								if(WinHttpReadData(hRequest, pOut, Length, pLength))
									bResult = TRUE;
							}
						}
					}
				}
				WinHttpCloseHandle(hRequest);
			}
			WinHttpCloseHandle(hConnect);
		}
		WinHttpCloseHandle(hSession);
	}
	return bResult;
}

// FFFTPの更新情報を確認
BOOL CheckForUpdates(BOOL bDownload, LPCTSTR DownloadDir)
{
	BOOL bResult;
	DWORD Length;
	BYTE Buf1[4096];
	BYTE Buf2[1024];
	UPDATE_HASH UpdateHash;
	BYTE Hash[64];
	bResult = FALSE;
	if(DownloadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_HASH_PATH))
	{
		if(DecryptSignature(UPDATE_RSA_PUBLIC_KEY, &Buf1, Length, &Buf2, sizeof(Buf2), &Length))
		{
			if(Length == sizeof(UPDATE_HASH))
			{
				memcpy(&UpdateHash, &Buf2, sizeof(UPDATE_HASH));
				if(memcmp(&UpdateHash.Signature, UPDATE_SIGNATURE, 64) == 0)
				{
					if(DownloadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_LIST_PATH))
					{
						GetHashSHA512(&Buf1, Length, &Hash);
						if(memcmp(&Hash, &UpdateHash.ListHash, 64) == 0)
						{
							// TODO: 更新情報を解析
							bResult = TRUE;
							if(bDownload)
								bResult = PrepareUpdates(&Buf1, Length, DownloadDir);
						}
					}
				}
			}
		}
	}
	return bResult;
}

// 更新するファイルをダウンロード
BOOL PrepareUpdates(void* pList, DWORD ListLength, LPCTSTR DownloadDir)
{
	BOOL bResult;
	bResult = FALSE;
	// TODO: 更新情報を解析
	// TODO: 更新するファイルをダウンロード
	return bResult;
}

// FFFTPを更新
BOOL ApplyUpdates(LPCTSTR DestinationDir)
{
	BOOL bResult;
	bResult = FALSE;
	// TODO:
	return bResult;
}

// 更新用のプロセスを起動
BOOL StartUpdateProcess(LPCTSTR Path, LPCTSTR CommandLine)
{
	BOOL bResult;
	bResult = FALSE;
	if(ShellExecute(NULL, "open", Path, CommandLine, NULL, SW_SHOW) > (HINSTANCE)32)
		bResult = TRUE;
	return bResult;
}

// 更新用のプロセスを管理者権限で起動
// Windows XP以前など起動できない場合は現在のプロセスで処理を続行
BOOL StartUpdateProcessAsAdministrator(LPCTSTR CommandLine, LPCTSTR Keyword)
{
	BOOL bResult;
	TCHAR* NewCommandLine;
	TCHAR Path[MAX_PATH];
	SHELLEXECUTEINFO Info;
	bResult = FALSE;
	if(_tcslen(CommandLine) < _tcslen(Keyword) || _tcscmp(CommandLine + _tcslen(CommandLine) - _tcslen(Keyword), Keyword) != 0)
	{
		if(NewCommandLine = (TCHAR*)malloc(sizeof(TCHAR) * (_tcslen(CommandLine) + _tcslen(Keyword) + 1)))
		{
			_tcscpy(NewCommandLine, CommandLine);
			_tcscat(NewCommandLine, Keyword);
			GetModuleFileName(NULL, Path, MAX_PATH);
			memset(&Info, 0, sizeof(SHELLEXECUTEINFO));
			Info.cbSize = sizeof(SHELLEXECUTEINFO);
			Info.fMask = SEE_MASK_NOCLOSEPROCESS;
			Info.lpVerb = "runas";
			Info.lpFile = Path;
			Info.lpParameters = NewCommandLine;
			Info.nShow = SW_SHOW;
			if(ShellExecuteEx(&Info))
			{
				WaitForSingleObject(Info.hProcess, INFINITE);
				CloseHandle(Info.hProcess);
				bResult = TRUE;
			}
			free(NewCommandLine);
		}
	}
	return bResult;
}

