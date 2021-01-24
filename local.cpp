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
	SetTaskMsg(IDS_MSGJPN145);
	return FFFTP_FAIL;
}


// ローカル側のディレクトリ作成
void DoLocalMKD(const char* Path) {
	SetTaskMsg(">>MKDIR %s", Path);
	if (std::error_code ec; !fs::create_directory(fs::u8path(Path), ec))
		SetTaskMsg(IDS_MSGJPN146);
}


// ローカル側のカレントディレクトリ取得
void DoLocalPWD(char *Buf) {
	strcpy(Buf, fs::current_path().u8string().c_str());
}


// ローカル側のディレクトリ削除
void DoLocalRMD(const char* Path) {
	SetTaskMsg(">>RMDIR %s", Path);
	if (MoveFileToTrashCan(Path) != 0)
		SetTaskMsg(IDS_MSGJPN148);
}


// ローカル側のファイル削除
void DoLocalDELE(const char* Path) {
	SetTaskMsg(">>DEL %s", Path);
	if (MoveFileToTrashCan(Path) != 0)
		SetTaskMsg(IDS_MSGJPN150);
}


// ローカル側のファイル名変更
void DoLocalRENAME(const char *Src, const char *Dst) {
	SetTaskMsg(">>REN %s %s", Src, Dst);
	std::error_code ec;
	fs::rename(fs::u8path(Src), fs::u8path(Dst), ec);
	if (ec)
		SetTaskMsg(IDS_MSGJPN151);
}


// ファイルのプロパティを表示する
void DispFileProperty(const char* Fname) {
	char Fname2[FMAX_PATH + 1];
	MakeDistinguishableFileName(Fname2, Fname);
	auto wFname2 = u8(Fname2);
	SHELLEXECUTEINFOW info{ sizeof(SHELLEXECUTEINFOW), SEE_MASK_INVOKEIDLIST, 0, L"Properties", wFname2.c_str(), nullptr, nullptr, SW_NORMAL };
	ShellExecuteExW(&info);
}
