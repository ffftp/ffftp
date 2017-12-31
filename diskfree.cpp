/*=============================================================================
*
*						ＤＩＳＫのフリースペースを得る
*
===============================================================================
/ Copyright (C) 1997-2007 Sota. All rights reserved.
/
/ Redistribution and use in source and binary forms, with or without 
/ modification, are permitted provided that the following conditions 
/ are met:
/
/  1. Redistributions of source code must retain the above copyright 
/     notice, this list of conditions and the following disclaimer.
/  2. Redistributions in binary form must reproduce the above copyright 
/     notice, this list of conditions and the following disclaimer in the 
/     documentation and/or other materials provided with the distribution.
/
/ THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
/ IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
/ OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
/ IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
/ INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
/ BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
/ USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
/ ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
/ (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
/ THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
/============================================================================*/

#include "common.h"


typedef DWORD (WINAPI*FUNC_GETDISKFREESPACEEX) (LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

/*===== ローカルなワーク =====*/

static HINSTANCE m_hDll = NULL;

static FUNC_GETDISKFREESPACEEX m_GetDiskFreeSpaceEx = NULL;



/*----- KERNEL32をロードする --------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void LoadKernelLib(void)
{
	OSVERSIONINFO VerInfo;

	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);
	if(((VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (VerInfo.dwBuildNumber > 1000)) ||
	   (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))
	{
		if((m_hDll = LoadLibrary("kernel32.dll")) != NULL)
		{
			m_GetDiskFreeSpaceEx = (FUNC_GETDISKFREESPACEEX)GetProcAddress(m_hDll, "GetDiskFreeSpaceExA");

			if(m_GetDiskFreeSpaceEx == NULL)
			{
				FreeLibrary(m_hDll);
				m_hDll = NULL;
			}
		}
	}
	return;
}


/*----- KERNEL32をリリースする -------------------------------------------------
*
*	Parameter
*		なし
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ReleaseKernelLib(void)
{
	if(m_hDll != NULL)
		FreeLibrary(m_hDll);
	m_hDll = NULL;

	return;
}


/*----- フリーエリアのサイズを表わす文字列を返す-------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		char *文字列
*----------------------------------------------------------------------------*/

char *AskLocalFreeSpace(char *Path)
{
	DWORD SectClus;
	DWORD ByteSect;
	DWORD FreeClus;
	DWORD AllClus;
	ULARGE_INTEGER a;
	ULARGE_INTEGER b;
	ULARGE_INTEGER c;
	double Free;
	static char Buf[40];

	strcpy(Buf, "??");
	if(*(Path+1) == ':')
	{
		strncpy(Buf, Path, 2);
		strcpy(Buf+2, "\\");

		if(m_GetDiskFreeSpaceEx != NULL)
		{
			if((*m_GetDiskFreeSpaceEx)(Buf, &a, &b, &c) != 0)
			{
				_ui64toa(a.QuadPart, Buf, 10);
				Free = atof(Buf);
				MakeSizeString(Free, Buf);
			}
			else
				strcpy(Buf, "??");
		}
		else
		{
			if(GetDiskFreeSpace(Buf, &SectClus, &ByteSect, &FreeClus, &AllClus) == TRUE)
			{
				Free = SectClus * ByteSect * FreeClus;
				MakeSizeString(Free, Buf);
			}
			else
				strcpy(Buf, "??");
		}
	}
	return(Buf);
}


