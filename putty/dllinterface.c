// dllinterface.c
// Copyright (C) 2011 Suguru Kawamoto
// 標準入出力APIラッパー

#include <stdio.h>
#include <windows.h>

#include "iowrapper.h"
#include "dllinterface.h"

__declspec(dllexport) SFTPSTATUS* SFTP_Create()
{
	SFTPSTATUS* p;
	if(p = FindSFTPStatus(0))
	{
		p->ThreadId = GetCurrentThreadId();
		p->bExit = FALSE;
		SFTP_InitializeIOBuffer(&p->InBuffer, 65536);
		SFTP_InitializeIOBuffer(&p->OutBuffer, 65536);
		SFTP_InitializeIOBuffer(&p->DataInBuffer, 1048576);
		SFTP_InitializeIOBuffer(&p->DataOutBuffer, 1048576);
		memset(&p->FilePosition, 0, sizeof(LARGE_INTEGER));
		CloseHandle(CreateThread(NULL, 0, SFTP_ThreadProc, NULL, 0, &p->ThreadId));
	}
	return p;
}

__declspec(dllexport) void SFTP_Destroy(SFTPSTATUS* pSFTP)
{
	if(pSFTP)
	{
		SFTP_UninitializeIOBuffer(&pSFTP->InBuffer);
		SFTP_UninitializeIOBuffer(&pSFTP->OutBuffer);
		SFTP_UninitializeIOBuffer(&pSFTP->DataInBuffer);
		SFTP_UninitializeIOBuffer(&pSFTP->DataOutBuffer);
		pSFTP->ThreadId = 0;
	}
}

__declspec(dllexport) BOOL SFTP_IsExited(SFTPSTATUS* pSFTP)
{
	return pSFTP->bExit;
}

__declspec(dllexport) BOOL SFTP_SetTimeoutCallback(SFTPSTATUS* pSFTP, LPSFTPTIMEOUTCALLBACK pCallback)
{
	pSFTP->pCallback = pCallback;
	pSFTP->InBuffer.pCallback = pCallback;
	pSFTP->OutBuffer.pCallback = pCallback;
	pSFTP->DataInBuffer.pCallback = pCallback;
	pSFTP->DataOutBuffer.pCallback = pCallback;
	return TRUE;
}

__declspec(dllexport) size_t SFTP_PeekStdOut(SFTPSTATUS* pSFTP, void* pData, size_t Size)
{
	return SFTP_PeekIOBuffer(&pSFTP->OutBuffer, pData, Size);
}

__declspec(dllexport) size_t SFTP_ReadStdOut(SFTPSTATUS* pSFTP, void* pData, size_t Size)
{
	return SFTP_ReadIOBuffer(&pSFTP->OutBuffer, pData, Size);
}

__declspec(dllexport) size_t SFTP_WriteStdIn(SFTPSTATUS* pSFTP, const void* pData, size_t Size)
{
	return SFTP_WriteIOBuffer(&pSFTP->InBuffer, pData, Size);
}

__declspec(dllexport) size_t SFTP_PeekDataOut(SFTPSTATUS* pSFTP, void* pData, size_t Size)
{
	return SFTP_PeekIOBuffer(&pSFTP->DataOutBuffer, pData, Size);
}

__declspec(dllexport) size_t SFTP_ReadDataOut(SFTPSTATUS* pSFTP, void* pData, size_t Size)
{
	return SFTP_ReadIOBuffer(&pSFTP->DataOutBuffer, pData, Size);
}

__declspec(dllexport) size_t SFTP_WriteDataIn(SFTPSTATUS* pSFTP, const void* pData, size_t Size)
{
	return SFTP_WriteIOBuffer(&pSFTP->DataInBuffer, pData, Size);
}

__declspec(dllexport) BOOL SFTP_SetFilePosition(SFTPSTATUS* pSFTP, LONGLONG Position)
{
	pSFTP->FilePosition.QuadPart = Position;
	return TRUE;
}

