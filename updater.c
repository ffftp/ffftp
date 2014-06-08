// updater.c
// Copyright (C) 2014 Suguru Kawamoto
// ソフトウェア自動更新
// コードの再利用のため表記はwchar_t型だが実体はchar型でUTF-8

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

#define UPDATE_LIST_FILE_FLAG_DIRECTORY 0x00000001

typedef struct
{
	DWORD Flags;
	CHAR SrcPath[128];
	BYTE SrcHash[64];
	CHAR DstPath[128];
	FILETIME Timestamp;
} UPDATE_LIST_FILE;

typedef struct
{
	DWORD Version;
	CHAR VersionString[32];
	DWORD FileCount;
	UPDATE_LIST_FILE File[1];
} UPDATE_LIST;

BOOL ReadFileViaHTTPW(void* pOut, DWORD Length, DWORD* pLength, LPCWSTR UserAgent, LPCWSTR ServerName, LPCWSTR ObjectName)
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

BOOL ReadFileViaHTTP(void* pOut, DWORD Length, DWORD* pLength, LPCSTR UserAgent, LPCSTR ServerName, LPCSTR ObjectName)
{
	BOOL r;
	wchar_t* pw0;
	wchar_t* pw1;
	wchar_t* pw2;
	pw0 = DuplicateMtoW(UserAgent, -1);
	pw1 = DuplicateMtoW(ServerName, -1);
	pw2 = DuplicateMtoW(ObjectName, -1);
	r = ReadFileViaHTTPW(pOut, Length, pLength, pw0, pw1, pw2);
	FreeDuplicatedString(pw0);
	FreeDuplicatedString(pw1);
	FreeDuplicatedString(pw2);
	return r;
}

BOOL SaveMemoryToFileWithTimestamp(LPCTSTR FileName, void* pData, DWORD Size, FILETIME* pTimestamp)
{
	BOOL bResult;
	HANDLE hFile;
	bResult = FALSE;
	if((hFile = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		if(WriteFile(hFile, pData, Size, &Size, NULL))
		{
			if(SetFileTime(hFile, NULL, NULL, pTimestamp))
				bResult = TRUE;
		}
		CloseHandle(hFile);
	}
	return bResult;
}

BOOL CopyAllFilesInDirectory(LPCTSTR From, LPCTSTR To)
{
	BOOL bResult;
	TCHAR* pFrom;
	TCHAR* pTo;
	SHFILEOPSTRUCT fop;
	bResult = FALSE;
	if(pFrom = (TCHAR*)malloc(sizeof(TCHAR) * (_tcslen(From) + _tcslen(_T("\\*")) + 2)))
	{
		_tcscpy(pFrom, From);
		_tcsncpy(pFrom + _tcslen(pFrom), _T("\\*"), _tcslen(_T("\\*")) + 2);
		if(pTo = (TCHAR*)malloc(sizeof(TCHAR) * (_tcslen(To) + 2)))
		{
			_tcsncpy(pTo, To, _tcslen(To) + 2);
			memset(&fop, 0, sizeof(SHFILEOPSTRUCT));
			fop.wFunc = FO_COPY;
			fop.pFrom = pFrom;
			fop.pTo = pTo;
			fop.fFlags = FOF_NO_UI;
			if(SHFileOperation(&fop) == 0)
				bResult = TRUE;
			free(pTo);
		}
		free(pFrom);
	}
	return bResult;
}

BOOL DeleteDirectoryAndContents(LPCTSTR Path)
{
	BOOL bResult;
	TCHAR* pFrom;
	SHFILEOPSTRUCT fop;
	bResult = FALSE;
	if(pFrom = (TCHAR*)malloc(sizeof(TCHAR) * (_tcslen(Path) + 2)))
	{
		_tcsncpy(pFrom, Path, _tcslen(Path) + 2);
		memset(&fop, 0, sizeof(SHFILEOPSTRUCT));
		fop.wFunc = FO_DELETE;
		fop.pFrom = pFrom;
		fop.fFlags = FOF_NO_UI;
		if(SHFileOperation(&fop) == 0)
			bResult = TRUE;
		free(pFrom);
	}
	return bResult;
}

// FFFTPの更新情報を確認
BOOL CheckForUpdates(BOOL bDownload, LPCTSTR DownloadDir, DWORD* pVersion, LPTSTR pVersionString)
{
	BOOL bResult;
	DWORD Length;
	BYTE Buf1[65536];
	BYTE Buf2[1024];
	UPDATE_HASH UpdateHash;
	BYTE Hash[64];
	UPDATE_LIST* pUpdateList;
	bResult = FALSE;
	if(ReadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_HASH_PATH))
	{
		if(DecryptSignature(UPDATE_RSA_PUBLIC_KEY, &Buf1, Length, &Buf2, sizeof(Buf2), &Length))
		{
			if(Length == sizeof(UPDATE_HASH))
			{
				memcpy(&UpdateHash, &Buf2, sizeof(UPDATE_HASH));
				if(memcmp(&UpdateHash.Signature, UPDATE_SIGNATURE, 64) == 0)
				{
					if(ReadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_LIST_PATH))
					{
						if(GetHashSHA512(&Buf1, Length, &Hash))
						{
							if(memcmp(&Hash, &UpdateHash.ListHash, 64) == 0)
							{
								if(Length >= sizeof(UPDATE_LIST))
								{
									bResult = TRUE;
									pUpdateList = (UPDATE_LIST*)&Buf1;
									if(pUpdateList->Version > *pVersion)
									{
										*pVersion = pUpdateList->Version;
										_tcscpy(pVersionString, pUpdateList->VersionString);
									}
									if(bDownload)
										bResult = PrepareUpdates(&Buf1, Length, DownloadDir);
								}
							}
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
	UPDATE_LIST* pUpdateList;
	void* pBuf;
	DWORD i;
	BOOL b;
	DWORD Length;
	BYTE Hash[64];
	TCHAR Path[MAX_PATH];
	bResult = FALSE;
	if(ListLength >= sizeof(UPDATE_LIST))
	{
		pUpdateList = (UPDATE_LIST*)pList;
		if((pUpdateList->FileCount - 1) * sizeof(UPDATE_LIST_FILE) + sizeof(UPDATE_LIST) >= ListLength)
		{
			bResult = TRUE;
			DeleteDirectoryAndContents(DownloadDir);
			CreateDirectory(DownloadDir, NULL);
			pBuf = malloc(16777216);
			for(i = 0; i < pUpdateList->FileCount; i++)
			{
				b = FALSE;
				if(pUpdateList->File[i].Flags & UPDATE_LIST_FILE_FLAG_DIRECTORY)
				{
					_tcscpy(Path, DownloadDir);
					_tcscat(Path, _T("\\"));
					_tcscat(Path, pUpdateList->File[i].DstPath);
					if(CreateDirectory(Path, NULL))
						b = TRUE;
				}
				if(strlen(pUpdateList->File[i].SrcPath) > 0)
				{
					if(ReadFileViaHTTP(pBuf, 16777216, &Length, HTTP_USER_AGENT, UPDATE_SERVER, pUpdateList->File[i].SrcPath))
					{
						if(GetHashSHA512(pBuf, Length, &Hash))
						{
							if(memcmp(&Hash, &pUpdateList->File[i].SrcHash, 64) == 0)
							{
								_tcscpy(Path, DownloadDir);
								_tcscat(Path, _T("\\"));
								_tcscat(Path, pUpdateList->File[i].DstPath);
								if(SaveMemoryToFileWithTimestamp(Path, pBuf, Length, &pUpdateList->File[i].Timestamp))
									b = TRUE;
							}
						}
					}
				}
				if(!b)
				{
					bResult = FALSE;
					break;
				}
			}
			free(pBuf);
		}
	}
	return bResult;
}

// FFFTPを更新
BOOL ApplyUpdates(LPCTSTR DestinationDir, LPCTSTR BackupDirName)
{
	BOOL bResult;
	TCHAR Source[MAX_PATH];
	TCHAR Backup[MAX_PATH];
	TCHAR DestinationBackup[MAX_PATH];
	TCHAR* p;
	bResult = FALSE;
	if(GetModuleFileName(NULL, Source, MAX_PATH) > 0)
	{
		if(p = _tcsrchr(Source, _T('\\')))
			*p = _T('\0');
		_tcscpy(Backup, Source);
		_tcscat(Backup, _T("\\"));
		_tcscat(Backup, BackupDirName);
		DeleteDirectoryAndContents(Backup);
		if(CopyAllFilesInDirectory(DestinationDir, Backup))
		{
			_tcscpy(DestinationBackup, DestinationDir);
			_tcscat(DestinationBackup, _T("\\"));
			_tcscat(DestinationBackup, BackupDirName);
			if(CopyAllFilesInDirectory(Source, DestinationDir))
			{
				DeleteDirectoryAndContents(DestinationBackup);
				bResult = TRUE;
			}
			else
			{
				DeleteDirectoryAndContents(DestinationBackup);
				CopyAllFilesInDirectory(Backup, DestinationDir);
			}
		}
	}
	return bResult;
}

// 更新するファイルをダウンロード
BOOL CleanupUpdates(LPCTSTR DownloadDir)
{
	BOOL bResult;
	bResult = FALSE;
	if(DeleteDirectoryAndContents(DownloadDir))
		bResult = TRUE;
	return bResult;
}

// 更新用のプロセスを起動
BOOL StartUpdateProcess(LPCTSTR DownloadDir, LPCTSTR CommandLine)
{
	BOOL bResult;
	TCHAR Name[MAX_PATH];
	TCHAR* p;
	TCHAR Path[MAX_PATH];
	bResult = FALSE;
	if(GetModuleFileName(NULL, Name, MAX_PATH) > 0)
	{
		if(p = _tcsrchr(Name, _T('\\')))
			p++;
		else
			p = Name;
		_tcscpy(Path, DownloadDir);
		_tcscat(Path, _T("\\"));
		_tcscat(Path, p);
		if(ShellExecute(NULL, _T("open"), Path, CommandLine, NULL, SW_SHOW) > (HINSTANCE)32)
			bResult = TRUE;
	}
	return bResult;
}

// 更新用のプロセスを管理者権限で起動
// Windows XP以前など起動できない場合は現在のプロセスで処理を続行
BOOL RestartUpdateProcessAsAdministrator(LPCTSTR CommandLine, LPCTSTR Keyword)
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
			if(GetModuleFileName(NULL, Path, MAX_PATH) > 0)
			{
				memset(&Info, 0, sizeof(SHELLEXECUTEINFO));
				Info.cbSize = sizeof(SHELLEXECUTEINFO);
				Info.fMask = SEE_MASK_NOCLOSEPROCESS;
				Info.lpVerb = _T("runas");
				Info.lpFile = Path;
				Info.lpParameters = NewCommandLine;
				Info.nShow = SW_SHOW;
				if(ShellExecuteEx(&Info))
				{
					WaitForSingleObject(Info.hProcess, INFINITE);
					CloseHandle(Info.hProcess);
					bResult = TRUE;
				}
			}
			free(NewCommandLine);
		}
	}
	return bResult;
}

