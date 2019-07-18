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

#include "common.h"


// ローカル側のディレクトリ変更
int DoLocalCWD(const char *Path) {
	SetTaskMsg(">>CD %s", Path);
	std::error_code ec;
	fs::current_path(fs::u8path(Path), ec);
	if (!ec)
		return FFFTP_SUCCESS;
	SetTaskMsg(MSGJPN145);
	return FFFTP_FAIL;
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


// ローカル側のカレントディレクトリ取得
void DoLocalPWD(char *Buf) {
	strcpy(Buf, fs::current_path().u8string().c_str());
}


// ローカル側のディレクトリ削除
void DoLocalRMD(char *Path) {
	SetTaskMsg(">>RMDIR %s", Path);
	if (MoveFileToTrashCan(Path) != 0)
		SetTaskMsg(MSGJPN148);
}


// ローカル側のファイル削除
void DoLocalDELE(char *Path) {
	SetTaskMsg(">>DEL %s", Path);
	if (MoveFileToTrashCan(Path) != 0)
		SetTaskMsg(MSGJPN150);
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


// ファイルのプロパティを表示する
void DispFileProperty(char *Fname) {
	char Fname2[FMAX_PATH + 1];
	MakeDistinguishableFileName(Fname2, Fname);
	auto wFname2 = u8(Fname2);
	SHELLEXECUTEINFOW info{ sizeof(SHELLEXECUTEINFOW), SEE_MASK_INVOKEIDLIST, 0, L"Properties", wFname2.c_str(), nullptr, nullptr, SW_NORMAL };
	ShellExecuteExW(&info);
}
