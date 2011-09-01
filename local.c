/*=============================================================================
*
*						ローカル側のファイルアクセス
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

#define	STRICT
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <windowsx.h>

#include "common.h"
#include "resource.h"



/*----- ローカル側のディレクトリ変更 -------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		int ステータス
*			SUCCESS/FAIL
*----------------------------------------------------------------------------*/

int DoLocalCWD(char *Path)
{
	int Sts;

	Sts = SUCCESS;
	SetTaskMsg(">>CD %s", Path);
	if(SetCurrentDirectory(Path) != TRUE)
	{
		SetTaskMsg(MSGJPN145);
		Sts = FAIL;
	}
	return(Sts);
}


/*----- ローカル側のディレクトリ作成 -------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoLocalMKD(char *Path)
{
	SetTaskMsg(">>MKDIR %s", Path);
	if(_mkdir(Path) != 0)
		SetTaskMsg(MSGJPN146);
	return;
}


/*----- ローカル側のカレントディレクトリ取得 -----------------------------------
*
*	Parameter
*		char *Buf : パス名を返すバッファ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoLocalPWD(char *Buf)
{
	if(GetCurrentDirectory(FMAX_PATH, Buf) == 0)
		strcpy(Buf, "");
	return;
}


/*----- ローカル側のディレクトリ削除 ------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoLocalRMD(char *Path)
{
#if 0
	SetTaskMsg(">>RMDIR %s", Path);
	if(rmdir(Path) != 0)
		SetTaskMsg(MSGJPN147);
#else
	SetTaskMsg(">>RMDIR %s", Path);

	if(MoveFileToTrashCan(Path) != 0)
		SetTaskMsg(MSGJPN148);
#endif
	return;
}


/*----- ローカル側のファイル削除 -----------------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoLocalDELE(char *Path)
{
#if 0
	SetTaskMsg(">>DEL %s", Path);
	if(DeleteFile(Path) != TRUE)
		SetTaskMsg(MSGJPN149);
#else
	SetTaskMsg(">>DEL %s", Path);

	if(MoveFileToTrashCan(Path) != 0)
		SetTaskMsg(MSGJPN150);
#endif
	return;
}


/*----- ローカル側のファイル名変更 ---------------------------------------------
*
*	Parameter
*		char *Src : 元ファイル名
*		char *Dst : 変更後のファイル名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DoLocalRENAME(char *Src, char *Dst)
{
	SetTaskMsg(">>REN %s %s", Src, Dst);
	if(MoveFile(Src, Dst) != TRUE)
		SetTaskMsg(MSGJPN151);
	return;
}


/*----- ファイルのプロパティを表示する ----------------------------------------
*
*	Parameter
*		char *Fname : ファイル名
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void DispFileProperty(char *Fname)
{
	SHELLEXECUTEINFO sInfo;

	memset(&sInfo, NUL, sizeof(SHELLEXECUTEINFO));
	sInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	sInfo.fMask = SEE_MASK_INVOKEIDLIST;
	sInfo.hwnd = NULL;		//GetMainHwnd();
	sInfo.lpVerb = "Properties";
	sInfo.lpFile = Fname;
	sInfo.lpParameters = NULL;
	sInfo.lpDirectory = NULL;
	sInfo.nShow = SW_NORMAL;
	sInfo.lpIDList = NULL;
	ShellExecuteEx(&sInfo);
	return;
}


/*----- 属性をチェックする FindFirstFile --------------------------------------
*
*	Parameter
*		char *Fname : ファイル名
*		WIN32_FIND_DATA *FindData : 検索データ
*		int IgnHide : 隠しファイルを無視するかどうか(YES/NO)
*
*	Return Value
*		HANDLE ハンドル
*----------------------------------------------------------------------------*/

HANDLE FindFirstFileAttr(char *Fname, WIN32_FIND_DATA *FindData, int IgnHide)
{
	HANDLE hFind;

	if((hFind = FindFirstFile(Fname, FindData)) != INVALID_HANDLE_VALUE)
	{
		if(IgnHide == YES)
		{
			while(FindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			{
				if(FindNextFile(hFind, FindData) == FALSE)
				{
					FindClose(hFind);
					hFind = INVALID_HANDLE_VALUE;
					break;
				}
			}
		}
	}
	return(hFind);
}


/*----- 属性をチェックする FindNextFile ---------------------------------------
*
*	Parameter
*		HANDLE hFind : ハンドル
*		WIN32_FIND_DATA *FindData : 検索データ
*		int IgnHide : 隠しファイルを無視するかどうか(YES/NO)
*
*	Return Value
*		HANDLE ハンドル
*----------------------------------------------------------------------------*/

BOOL FindNextFileAttr(HANDLE hFind, WIN32_FIND_DATA *FindData, int IgnHide)
{
	BOOL Ret;

	while((Ret = FindNextFile(hFind, FindData)) == TRUE)
	{
		if(IgnHide == NO)
			break;
		if((FindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0)
			break;
	}
	return(Ret);
}


