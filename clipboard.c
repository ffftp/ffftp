/*=============================================================================
*
*								クリップボード関係
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

#define  STRICT
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>

#include "common.h"
#include "resource.h"



/*----- 文字列をクリップボードにコピー ----------------------------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

int CopyStrToClipBoard(char *Str)
{
	int Sts;
	void *gBuf;
	HGLOBAL hGlobal;

	Sts = FAIL;
	if(OpenClipboard(GetMainHwnd()))
	{
		if(EmptyClipboard())
		{
			if((hGlobal = GlobalAlloc(GHND, strlen(Str)+1)) != NULL)
			{
				if((gBuf = GlobalLock(hGlobal)) != NULL)
				{
					strcpy(gBuf, Str);

					GlobalUnlock(hGlobal);
					SetClipboardData(CF_TEXT, hGlobal);
					Sts = SUCCESS;
				}
			}
		}
		CloseClipboard();
	}
	return(Sts);
}


