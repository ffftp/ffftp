// iowrapper.c
// Copyright (C) 2011 Suguru Kawamoto
// 標準入出力APIラッパー

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <windows.h>

#define DO_NOT_REPLACE
#include "iowrapper.h"
#include "psftp.h"

#define MAX_SFTPSTATUS 16

SFTPSTATUS g_SFTPData[MAX_SFTPSTATUS];
HANDLE g_hStdIn;
HANDLE g_hStdOut;
HANDLE g_hStdErr;

BOOL __stdcall DefaultIOBufferCallback(BOOL* pbAborted)
{
	Sleep(1);
	return *pbAborted;
}

DWORD WINAPI SFTP_ThreadProc(LPVOID lpParameter)
{
	char* p[1] = {"PSFTP"};
	SFTPSTATUS* pSFTP;
	psftp_main(1, p);
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		pSFTP->bExit = TRUE;
	return 0;
}

SFTPSTATUS* FindSFTPStatus(DWORD ThreadId)
{
	int i;
	for(i = 0; i < MAX_SFTPSTATUS; i++)
	{
		if(g_SFTPData[i].ThreadId == ThreadId)
			return &g_SFTPData[i];
	}
	return NULL;
}

CRITICAL_SECTION g_DummyLock;

BOOL SFTP_InitializeIOBuffer(SFTPIOBUFFER* pIO, size_t Length)
{
	memset(pIO, 0, sizeof(SFTPIOBUFFER));
//	InitializeCriticalSection(&pIO->Lock);
	if(memcmp(&pIO->Lock, &g_DummyLock, sizeof(CRITICAL_SECTION)) == 0)
	{
		InitializeCriticalSection(&pIO->Lock);
		EnterCriticalSection(&pIO->Lock);
	}
	if(!(pIO->pBuffer = (BYTE*)malloc(Length + 1)))
		return FALSE;
	memset(pIO->pBuffer + Length, 0, 1);
	pIO->Length = Length;
	pIO->Written = 0;
	pIO->Read = 0;
	pIO->bAborted = FALSE;
	pIO->pCallback = DefaultIOBufferCallback;
	LeaveCriticalSection(&pIO->Lock);
	return TRUE;
}

void SFTP_UninitializeIOBuffer(SFTPIOBUFFER* pIO)
{
//	DeleteCriticalSection(&pIO->Lock);
	EnterCriticalSection(&pIO->Lock);
	free(pIO->pBuffer);
//	memset(pIO, 0, sizeof(SFTPIOBUFFER));
}

void SFTP_AbortIOBuffer(SFTPIOBUFFER* pIO, BOOL bAborted)
{
	EnterCriticalSection(&pIO->Lock);
	pIO->bAborted = bAborted;
	LeaveCriticalSection(&pIO->Lock);
}

size_t SFTP_PeekIOBuffer(SFTPIOBUFFER* pIO, void* pBuffer, size_t Size)
{
	size_t Copied;
	size_t Pos;
	size_t Count;
	size_t Read;
	Copied = 0;
	EnterCriticalSection(&pIO->Lock);
	if(pBuffer)
	{
		Read = pIO->Read;
		while(!pIO->bAborted && pIO->Written - Read > 0 && Copied < Size)
		{
			Pos = Read % pIO->Length;
			Count = min(Size - Copied, min(pIO->Written - Read, pIO->Length - Pos));
			memcpy((BYTE*)pBuffer + Copied, pIO->pBuffer + Pos, Count);
			Read += Count;
			Copied += Count;
		}
	}
	else
		Copied = pIO->Written - pIO->Read;
	LeaveCriticalSection(&pIO->Lock);
	return Copied;
}

size_t SFTP_ReadIOBuffer(SFTPIOBUFFER* pIO, void* pBuffer, size_t Size)
{
	size_t Copied;
	size_t Pos;
	size_t Count;
	Copied = 0;
	EnterCriticalSection(&pIO->Lock);
	while(!pIO->bAborted && pIO->Written - pIO->Read > 0 && Copied < Size)
	{
		Pos = pIO->Read % pIO->Length;
		Count = min(Size - Copied, min(pIO->Written - pIO->Read, pIO->Length - Pos));
		memcpy((BYTE*)pBuffer + Copied, pIO->pBuffer + Pos, Count);
		pIO->Read += Count;
		Copied += Count;
	}
	LeaveCriticalSection(&pIO->Lock);
	return Copied;
}

size_t SFTP_WriteIOBuffer(SFTPIOBUFFER* pIO, const void* pBuffer, size_t Size)
{
	size_t Copied;
	size_t Pos;
	size_t Count;
	Copied = 0;
	EnterCriticalSection(&pIO->Lock);
	while(!pIO->bAborted && Copied < Size)
	{
		if(pIO->Written - pIO->Read < pIO->Length)
		{
			Pos = pIO->Written % pIO->Length;
			Count = min(Size - Copied, min(pIO->Length + pIO->Read - pIO->Written, pIO->Length - Pos));
			memcpy(pIO->pBuffer + Pos, (BYTE*)pBuffer + Copied, Count);
			pIO->Written += Count;
			Copied += Count;
		}
		else
		{
			LeaveCriticalSection(&pIO->Lock);
			if(pIO->pCallback(&pIO->bAborted))
				Size = 0;
			EnterCriticalSection(&pIO->Lock);
		}
	}
	LeaveCriticalSection(&pIO->Lock);
	return Copied;
}

size_t SFTP_ReadIOBufferLine(SFTPIOBUFFER* pIO, void* pBuffer, size_t Size)
{
	size_t Copied;
	size_t Pos;
	size_t Count;
	char* p;
	Copied = 0;
	EnterCriticalSection(&pIO->Lock);
	while(!pIO->bAborted && Copied < Size)
	{
		if(pIO->Written - pIO->Read > 0)
		{
			Pos = pIO->Read % pIO->Length;
			Count = min(Size - Copied, min(pIO->Written - pIO->Read, pIO->Length - Pos));
			if(p = strchr((char*)(pIO->pBuffer + Pos), '\n'))
			{
				p++;
				Count = min(Count, (size_t)p - (size_t)(pIO->pBuffer + Pos));
				Size = 0;
			}
			memcpy((BYTE*)pBuffer + Copied, pIO->pBuffer + Pos, Count);
			pIO->Read += Count;
			Copied += Count;
		}
		else
		{
			LeaveCriticalSection(&pIO->Lock);
			if(pIO->pCallback(&pIO->bAborted))
				Size = 0;
			EnterCriticalSection(&pIO->Lock);
		}
	}
	LeaveCriticalSection(&pIO->Lock);
	return Copied;
}

size_t SFTP_PeekThreadIO(void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_PeekIOBuffer(&pSFTP->InBuffer, pBuffer, Size);
	return 0;
}

size_t SFTP_ReadThreadIO(void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_ReadIOBuffer(&pSFTP->InBuffer, pBuffer, Size);
	return 0;
}

size_t SFTP_WriteThreadIO(const void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_WriteIOBuffer(&pSFTP->OutBuffer, pBuffer, Size);
	return 0;
}

size_t SFTP_ReadThreadIOLine(void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_ReadIOBufferLine(&pSFTP->InBuffer, pBuffer, Size);
	return 0;
}

size_t SFTP_PeekThreadDataIO(void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_PeekIOBuffer(&pSFTP->DataInBuffer, pBuffer, Size);
	return 0;
}

size_t SFTP_ReadThreadDataIO(void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_ReadIOBuffer(&pSFTP->DataInBuffer, pBuffer, Size);
	return 0;
}

size_t SFTP_WriteThreadDataIO(const void* pBuffer, size_t Size)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
		return SFTP_WriteIOBuffer(&pSFTP->DataOutBuffer, pBuffer, Size);
	return 0;
}

BOOL SFTP_GetThreadFilePositon(DWORD* pLow, LONG* pHigh)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
	{
		*pLow = pSFTP->FilePosition.LowPart;
		*pHigh = pSFTP->FilePosition.HighPart;
		return TRUE;
	}
	return FALSE;
}

// 以下ラッパー

HANDLE GetStdHandleX(DWORD nStdHandle)
{
	HANDLE r = INVALID_HANDLE_VALUE;
	if(!g_hStdIn)
		g_hStdIn = (HANDLE)1;
	if(!g_hStdOut)
		g_hStdOut = (HANDLE)2;
	if(!g_hStdErr)
		g_hStdErr = (HANDLE)3;
	if(nStdHandle == STD_INPUT_HANDLE)
		r = g_hStdIn;
	if(nStdHandle == STD_OUTPUT_HANDLE)
		r = g_hStdOut;
	if(nStdHandle == STD_ERROR_HANDLE)
		r = g_hStdErr;
	return r;
}

BOOL GetConsoleModeX(HANDLE hConsoleHandle, LPDWORD lpMode)
{
	BOOL r = FALSE;
	return r;
}

BOOL SetConsoleModeX(HANDLE hConsoleHandle, DWORD dwMode)
{
	BOOL r = FALSE;
	return r;
}

BOOL ReadFileX(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	BOOL r = FALSE;
	if(hFile == g_hStdIn)
	{
		*lpNumberOfBytesRead = (DWORD)SFTP_ReadThreadIOLine(lpBuffer, nNumberOfBytesToRead);
		if(*lpNumberOfBytesRead > 0)
			r = TRUE;
	}
	else if(hFile == g_hStdOut)
	{
	}
	else if(hFile == g_hStdErr)
	{
	}
	return r;
}

BOOL WriteFileX(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	BOOL r = FALSE;
	if(hFile == g_hStdIn)
	{
	}
	else if(hFile == g_hStdOut)
	{
		*lpNumberOfBytesWritten = (DWORD)SFTP_WriteThreadIO(lpBuffer, nNumberOfBytesToWrite);
		if(*lpNumberOfBytesWritten == nNumberOfBytesToWrite)
			r = TRUE;
	}
	else if(hFile == g_hStdErr)
	{
		*lpNumberOfBytesWritten = (DWORD)SFTP_WriteThreadIO(lpBuffer, nNumberOfBytesToWrite);
		if(*lpNumberOfBytesWritten == nNumberOfBytesToWrite)
			r = TRUE;
	}
	return r;
}

int printfX(const char * _Format, ...)
{
	int r = 0;
	va_list v;
	char Temp[1024];
	va_start(v, _Format);
	vsprintf(Temp, _Format, v);
	r = (int)SFTP_WriteThreadIO(Temp, strlen(Temp));
	va_end(v);
	return r;
}

int putsX(const char * _Str)
{
	int r = 0;
	r = (int)SFTP_WriteThreadIO(_Str, strlen(_Str));
	return r;
}

int fprintfX(FILE * _File, const char * _Format, ...)
{
	int r = 0;
	va_list v;
	char Temp[1024];
	va_start(v, _Format);
	if(_File == stdout)
	{
		vsprintf(Temp, _Format, v);
		r = (int)SFTP_WriteThreadIO(Temp, strlen(Temp));
	}
	else if(_File == stderr)
	{
		vsprintf(Temp, _Format, v);
		r = (int)SFTP_WriteThreadIO(Temp, strlen(Temp));
	}
	else
		r = vfprintf(_File, _Format, v);
	va_end(v);
	return r;
}

char * fgetsX(char * _Buf, int _MaxCount, FILE * _File)
{
	char * r = NULL;
	if(_File == stdin)
	{
		memset(_Buf, 0, _MaxCount);
		SFTP_ReadThreadIOLine(_Buf, _MaxCount - 1);
		r = _Buf;
	}
	else
		r = fgets(_Buf, _MaxCount, _File);
	return r;
}

int fputsX(const char * _Str, FILE * _File)
{
	int r = 0;
	if(_File == stdout)
	{
		r = (int)SFTP_WriteThreadIO(_Str, strlen(_Str));
	}
	else if(_File == stderr)
	{
		r = (int)SFTP_WriteThreadIO(_Str, strlen(_Str));
	}
	else
		r = fputs(_Str, _File);
	return r;
}

int fflushX(FILE * _File)
{
	int r = 0;
	if(_File == stdout)
	{
	}
	else if(_File == stderr)
	{
	}
	else
		r = fflush(_File);
	return r;
}

void exitX(int _Code)
{
	SFTPSTATUS* pSFTP;
	if(pSFTP = FindSFTPStatus(GetCurrentThreadId()))
	{
		pSFTP->bExit = TRUE;
		SFTP_AbortIOBuffer(&pSFTP->InBuffer, TRUE);
		SFTP_AbortIOBuffer(&pSFTP->OutBuffer, TRUE);
		SFTP_AbortIOBuffer(&pSFTP->DataInBuffer, TRUE);
		SFTP_AbortIOBuffer(&pSFTP->DataOutBuffer, TRUE);
	}
	TerminateThread(GetCurrentThread(), (DWORD)_Code);
}

