// updater.c
// Copyright (C) 2014 Suguru Kawamoto
// ソフトウェア自動更新

#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
typedef SOCKADDR_STORAGE_XP SOCKADDR_STORAGE;
typedef SOCKADDR_STORAGE *PSOCKADDR_STORAGE, FAR *LPSOCKADDR_STORAGE;
#include <winhttp.h>

#include "updater.h"
#include "socketwrapper.h"
#include "protectprocess.h"

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
BOOL CheckForUpdates()
{
	BOOL bResult;
	BYTE Buf1[4096];
	BYTE Buf2[1024];
	BYTE Hash[20];
	DWORD Length;
	bResult = FALSE;
	if(DownloadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_HASH_PATH))
	{
		if(DecryptSignature(UPDATE_RSA_PUBLIC_KEY, &Buf1, Length, &Buf2, sizeof(Buf2), &Length))
		{
			if(Length >= 20)
			{
				if(memcmp(&Buf2[0], UPDATE_SIGNATURE, 20) == 0)
				{
					if(DownloadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_LIST_PATH))
					{
						if(GetSHA1HashOfMemory(&Buf1, Length, &Hash))
						{
							if(memcmp(&Hash, &Buf2[20], 20) == 0)
							{
								// TODO: 更新情報を解析
							}
						}
					}
				}
			}
		}
	}
	return bResult;
}

// 別のプロセスでFFFTPを更新
BOOL StartUpdateProcess()
{
	BOOL bResult;
	bResult = FALSE;
	// TODO: CreateProcess()
	return bResult;
}

