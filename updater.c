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
#include "apiemulator.h"

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
	CHAR Description[1024];
	DWORD FileCount;
	UPDATE_LIST_FILE File[1];
} UPDATE_LIST;

#define UPDATE_MAX_LIST_SIZE 1048576
#define UPDATE_MAX_FILE_SIZE 16777216

BOOL ReadFileViaHTTPW(void* pOut, DWORD Length, DWORD* pLength, LPCWSTR UserAgent, LPCWSTR ServerName, LPCWSTR ObjectName)
{
	BOOL bResult;
	HINTERNET hSession;
	DWORD Buffer;
	HINTERNET hConnect;
	HINTERNET hRequest;
	bResult = FALSE;
	if(hSession = WinHttpOpen(UserAgent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0))
	{
		Buffer = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
		if(WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &Buffer, sizeof(DWORD)))
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
			if(pTimestamp)
			{
				if(SetFileTime(hFile, NULL, NULL, pTimestamp))
					bResult = TRUE;
			}
			else
				bResult = TRUE;
		}
		CloseHandle(hFile);
	}
	return bResult;
}

BOOL LoadMemoryFromFileWithTimestamp(LPCTSTR FileName, void* pData, DWORD Size, DWORD* pReadSize, FILETIME* pTimestamp)
{
	BOOL bResult;
	HANDLE hFile;
	LARGE_INTEGER li;
	bResult = FALSE;
	if((hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
	{
		if(GetFileSizeEx(hFile, &li))
		{
			if(li.QuadPart <= (LONGLONG)Size)
			{
				if(ReadFile(hFile, pData, Size, pReadSize, NULL))
				{
					if(*pReadSize == li.LowPart)
					{
						if(pTimestamp)
						{
							if(GetFileTime(hFile, NULL, NULL, pTimestamp))
								bResult = TRUE;
						}
						else
							bResult = TRUE;
					}
				}
			}
			CloseHandle(hFile);
		}
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

DWORD ListUpdateFile(UPDATE_LIST* pList, DWORD MaxCount, LPCTSTR ServerPath, LPCTSTR ReferenceDir, LPCTSTR Path)
{
	DWORD Result;
	TCHAR Temp1[MAX_PATH];
	TCHAR Temp2[MAX_PATH];
	TCHAR Temp3[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA Find;
	void* pBuf;
	DWORD Length;
	FILETIME Time;
	BYTE Hash[64];
	Result = 0;
	if(!Path)
		Path = _T("");
	if(_tcslen(ReferenceDir) + _tcslen(Path) + _tcslen(_T("\\*")) < MAX_PATH)
	{
		_tcscpy(Temp1, ReferenceDir);
		_tcscat(Temp1, Path);
		_tcscat(Temp1, _T("\\*"));
		if((hFind = FindFirstFile(Temp1, &Find)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(_tcscmp(Find.cFileName, _T(".")) != 0 && _tcscmp(Find.cFileName, _T("..")) != 0)
				{
//					if(_tcslen(ServerPath) + _tcslen(_T("/")) + _tcslen(Find.cFileName) < 128 && _tcslen(Path) + _tcslen(_T("\\")) + _tcslen(Find.cFileName) < 128)
					if(_tcslen(ServerPath) + _tcslen(Find.cFileName) < 128 && _tcslen(Path) + _tcslen(_T("\\")) + _tcslen(Find.cFileName) < 128)
					{
						_tcscpy(Temp1, ServerPath);
//						_tcscat(Temp1, _T("/"));
						_tcscat(Temp1, Find.cFileName);
						_tcscpy(Temp2, Path);
						_tcscat(Temp2, _T("\\"));
						_tcscat(Temp2, Find.cFileName);
						if(_tcslen(ReferenceDir) + _tcslen(Temp2) < MAX_PATH)
						{
							_tcscpy(Temp3, ReferenceDir);
							_tcscat(Temp3, Temp2);
							if((Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
							{
								if(!(Find.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
								{
									if(pList)
									{
										memset(&pList->File[pList->FileCount], 0, sizeof(UPDATE_LIST_FILE));
										pList->File[pList->FileCount].Flags = UPDATE_LIST_FILE_FLAG_DIRECTORY;
										_tcscpy(pList->File[pList->FileCount].DstPath, Temp2);
										pList->FileCount++;
									}
									Result++;
									if(Result >= MaxCount)
										break;
									Result += ListUpdateFile(pList, MaxCount, Temp1, ReferenceDir, Temp2);
								}
							}
							else
							{
								if(pList)
								{
									if(pBuf = malloc(UPDATE_MAX_FILE_SIZE))
									{
										if(LoadMemoryFromFileWithTimestamp(Temp3, pBuf, UPDATE_MAX_FILE_SIZE, &Length, &Time))
										{
											if(GetHashSHA512(pBuf, Length, &Hash))
											{
												memset(&pList->File[pList->FileCount], 0, sizeof(UPDATE_LIST_FILE));
												_tcscpy(pList->File[pList->FileCount].SrcPath, Temp1);
												memcpy(&pList->File[pList->FileCount].SrcHash, &Hash, 64);
												_tcscpy(pList->File[pList->FileCount].DstPath, Temp2);
												pList->File[pList->FileCount].Timestamp = Time;
												pList->FileCount++;
											}
										}
										free(pBuf);
									}
								}
								Result++;
								if(Result >= MaxCount)
									break;
							}
						}
					}
				}
			}
			while(FindNextFile(hFind, &Find));
			FindClose(hFind);
		}
	}
	return Result;
}

// FFFTPの更新情報を作成
BOOL BuildUpdates(LPCTSTR PrivateKeyFile, LPCTSTR Password, LPCTSTR ServerPath, LPCTSTR HashFile, LPCTSTR ListFile, DWORD Version, LPCTSTR VersionString, LPCTSTR Description)
{
	BOOL bResult;
	char PrivateKey[4096];
	DWORD Length;
	TCHAR Name[MAX_PATH];
	TCHAR* p;
	UPDATE_LIST* pList;
	UPDATE_HASH Hash;
	BYTE Buf[1024];
	bResult = FALSE;
	memset(PrivateKey, 0, sizeof(PrivateKey));
	if(LoadMemoryFromFileWithTimestamp(PrivateKeyFile, &PrivateKey, sizeof(PrivateKey) - sizeof(char), &Length, NULL))
	{
		if(GetModuleFileName(NULL, Name, MAX_PATH) > 0)
		{
			if(p = _tcsrchr(Name, _T('\\')))
				*p = _T('\0');
			if(pList = (UPDATE_LIST*)malloc(UPDATE_MAX_LIST_SIZE))
			{
				memset(pList, 0, UPDATE_MAX_LIST_SIZE);
				pList->Version = Version;
				_tcscpy(pList->VersionString, VersionString);
				_tcscpy(pList->Description, Description);
				ListUpdateFile(pList, (UPDATE_MAX_LIST_SIZE - sizeof(UPDATE_LIST)) / sizeof(UPDATE_LIST_FILE) + 1, ServerPath, Name, NULL);
				Length = (pList->FileCount - 1) * sizeof(UPDATE_LIST_FILE) + sizeof(UPDATE_LIST);
				if(SaveMemoryToFileWithTimestamp(ListFile, pList, Length, NULL))
				{
					memcpy(&Hash.Signature, UPDATE_SIGNATURE, 64);
					if(GetHashSHA512(pList, Length, &Hash.ListHash))
					{
						if(EncryptSignature(PrivateKey, Password, &Hash, sizeof(UPDATE_HASH), &Buf, sizeof(Buf), &Length))
						{
							if(SaveMemoryToFileWithTimestamp(HashFile, &Buf, Length, NULL))
								bResult = TRUE;
						}
					}
				}
				free(pList);
			}
		}
	}
	return bResult;
}

// FFFTPの更新情報を確認
BOOL CheckForUpdates(BOOL bDownload, LPCTSTR DownloadDir, DWORD* pVersion, LPTSTR pVersionString, LPTSTR pDescription)
{
	BOOL bResult;
	DWORD Length;
	BYTE Buf1[1024];
	BYTE Buf2[1024];
	void* pBuf;
	UPDATE_HASH UpdateHash;
	BYTE Hash[64];
	UPDATE_LIST* pUpdateList;
	bResult = FALSE;
	if(ReadFileViaHTTP(&Buf1, sizeof(Buf1), &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_HASH_PATH))
	{
		if(DecryptSignature(UPDATE_RSA_PUBLIC_KEY, NULL, &Buf1, Length, &Buf2, sizeof(Buf2), &Length))
		{
			if(Length == sizeof(UPDATE_HASH))
			{
				memcpy(&UpdateHash, &Buf2, sizeof(UPDATE_HASH));
				if(memcmp(&UpdateHash.Signature, UPDATE_SIGNATURE, 64) == 0)
				{
					if(pBuf = malloc(UPDATE_MAX_LIST_SIZE))
					{
						if(ReadFileViaHTTP(pBuf, UPDATE_MAX_LIST_SIZE, &Length, HTTP_USER_AGENT, UPDATE_SERVER, UPDATE_LIST_PATH))
						{
							if(GetHashSHA512(pBuf, Length, &Hash))
							{
								if(memcmp(&Hash, &UpdateHash.ListHash, 64) == 0)
								{
									if(Length >= sizeof(UPDATE_LIST))
									{
										bResult = TRUE;
										pUpdateList = (UPDATE_LIST*)pBuf;
										if(pUpdateList->Version > *pVersion)
										{
											*pVersion = pUpdateList->Version;
											_tcscpy(pVersionString, pUpdateList->VersionString);
											_tcscpy(pDescription, pUpdateList->Description);
										}
										if(bDownload)
											bResult = PrepareUpdates(pBuf, Length, DownloadDir);
									}
								}
							}
						}
						free(pBuf);
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
	TCHAR LocalDir[MAX_PATH];
	TCHAR* p;
	DWORD i;
	BOOL b;
	TCHAR Path[MAX_PATH];
	DWORD Length;
	BYTE Hash[64];
	bResult = FALSE;
	if(ListLength >= sizeof(UPDATE_LIST))
	{
		pUpdateList = (UPDATE_LIST*)pList;
		if((pUpdateList->FileCount - 1) * sizeof(UPDATE_LIST_FILE) + sizeof(UPDATE_LIST) >= ListLength)
		{
			bResult = TRUE;
			DeleteDirectoryAndContents(DownloadDir);
			CreateDirectory(DownloadDir, NULL);
			if(pBuf = malloc(UPDATE_MAX_FILE_SIZE))
			{
				if(GetModuleFileName(NULL, LocalDir, MAX_PATH) > 0)
				{
					if(p = _tcsrchr(LocalDir, _T('\\')))
						*p = _T('\0');
				}
				else
					_tcscpy(LocalDir, _T("."));
				for(i = 0; i < pUpdateList->FileCount; i++)
				{
					b = FALSE;
					if(pUpdateList->File[i].Flags & UPDATE_LIST_FILE_FLAG_DIRECTORY)
					{
						_tcscpy(Path, DownloadDir);
						_tcscat(Path, pUpdateList->File[i].DstPath);
						if(CreateDirectory(Path, NULL))
							b = TRUE;
					}
					if(strlen(pUpdateList->File[i].SrcPath) > 0)
					{
						_tcscpy(Path, LocalDir);
						_tcscat(Path, pUpdateList->File[i].DstPath);
						if(LoadMemoryFromFileWithTimestamp(Path, pBuf, UPDATE_MAX_FILE_SIZE, &Length, NULL))
						{
							if(GetHashSHA512(pBuf, Length, &Hash))
							{
								if(memcmp(&Hash, &pUpdateList->File[i].SrcHash, 64) == 0)
									b = TRUE;
							}
						}
						if(!b)
						{
							if(ReadFileViaHTTP(pBuf, UPDATE_MAX_FILE_SIZE, &Length, HTTP_USER_AGENT, UPDATE_SERVER, pUpdateList->File[i].SrcPath))
							{
								if(GetHashSHA512(pBuf, Length, &Hash))
								{
									if(memcmp(&Hash, &pUpdateList->File[i].SrcHash, 64) == 0)
									{
										_tcscpy(Path, DownloadDir);
										_tcscat(Path, pUpdateList->File[i].DstPath);
										if(SaveMemoryToFileWithTimestamp(Path, pBuf, Length, &pUpdateList->File[i].Timestamp))
											b = TRUE;
									}
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
		if(CreateDirectory(Backup, NULL))
		{
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
				if(IsUserAnAdmin())
					Info.lpVerb = _T("open");
				else
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

