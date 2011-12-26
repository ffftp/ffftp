// iowrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// 標準入出力APIラッパー

#ifndef __IOWRAPPER_H__
#define __IOWRAPPER_H__

#include <stdio.h>
#include <windows.h>

#ifndef DO_NOT_REPLACE

#undef GetStdHandle
#define GetStdHandle GetStdHandleX
HANDLE GetStdHandleX(DWORD nStdHandle);
#undef GetConsoleMode
#define GetConsoleMode GetConsoleModeX
BOOL GetConsoleModeX(HANDLE hConsoleHandle, LPDWORD lpMode);
#undef SetConsoleMode
#define SetConsoleMode SetConsoleModeX
BOOL SetConsoleModeX(HANDLE hConsoleHandle, DWORD dwMode);
#undef ReadFile
#define ReadFile ReadFileX
BOOL ReadFileX(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
#undef WriteFile
#define WriteFile WriteFileX
BOOL WriteFileX(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
#undef printf
#define printf printfX
int printfX(const char * _Format, ...);
#undef gets
#define gets getsX
char * getsX(char * _Buffer);
#undef puts
#define puts putsX
int putsX(const char * _Str);
#undef fprintf
#define fprintf fprintfX
int fprintfX(FILE * _File, const char * _Format, ...);
#undef fgets
#define fgets fgetsX
char * fgetsX(char * _Buf, int _MaxCount, FILE * _File);
#undef fputs
#define fputs fputsX
int fputsX(const char * _Str, FILE * _File);
#undef exit
#define exit exitX
void exitX(int _Code);

#endif

typedef BOOL (__stdcall* LPSFTPTIMEOUTCALLBACK)(BOOL*);

typedef struct
{
	CRITICAL_SECTION Lock;
	BYTE* pBuffer;
	size_t Length;
	size_t Written;
	size_t Read;
	BOOL bAborted;
	LPSFTPTIMEOUTCALLBACK pCallback;
} SFTPIOBUFFER;

typedef struct
{
	DWORD ThreadId;
	BOOL bExit;
	LPSFTPTIMEOUTCALLBACK pCallback;
	SFTPIOBUFFER InBuffer;
	SFTPIOBUFFER OutBuffer;
	SFTPIOBUFFER DataInBuffer;
	SFTPIOBUFFER DataOutBuffer;
	LARGE_INTEGER FilePosition;
} SFTPSTATUS;

DWORD WINAPI SFTP_ThreadProc(LPVOID lpParameter);
SFTPSTATUS* FindSFTPStatus(DWORD ThreadId);
BOOL SFTP_InitializeIOBuffer(SFTPIOBUFFER* pIO, size_t Length);
void SFTP_UninitializeIOBuffer(SFTPIOBUFFER* pIO);
void SFTP_AbortIOBuffer(SFTPIOBUFFER* pIO, BOOL bAborted);
size_t SFTP_PeekIOBuffer(SFTPIOBUFFER* pIO, void* pBuffer, size_t Size);
size_t SFTP_ReadIOBuffer(SFTPIOBUFFER* pIO, void* pBuffer, size_t Size);
size_t SFTP_WriteIOBuffer(SFTPIOBUFFER* pIO, const void* pBuffer, size_t Size);
size_t SFTP_ReadIOBufferLine(SFTPIOBUFFER* pIO, void* pBuffer, size_t Size);
size_t SFTP_PeekThreadIO(void* pBuffer, size_t Size);
size_t SFTP_ReadThreadIO(void* pBuffer, size_t Size);
size_t SFTP_WriteThreadIO(const void* pBuffer, size_t Size);
size_t SFTP_ReadThreadIOLine(void* pBuffer, size_t Size);
size_t SFTP_PeekThreadDataIO(void* pBuffer, size_t Size);
size_t SFTP_ReadThreadDataIO(void* pBuffer, size_t Size);
size_t SFTP_WriteThreadDataIO(const void* pBuffer, size_t Size);
BOOL SFTP_GetThreadFilePositon(DWORD* pLow, LONG* pHigh);

#endif

