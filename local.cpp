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
bool DoLocalCWD(fs::path const& path) {
	Notice(IDS_LOCALCMD, std::format(L"CD {}"sv, path.native()));
	std::error_code ec;
	fs::current_path(path, ec);
	if (!ec)
		return true;
	SetTaskMsg(IDS_MSGJPN145);
	return false;
}


// ローカル側のディレクトリ作成
void DoLocalMKD(fs::path const& path) {
	Notice(IDS_LOCALCMD, std::format(L"MKDIR {}"sv, path.native()));
	if (std::error_code ec; !fs::create_directory(path, ec))
		SetTaskMsg(IDS_MSGJPN146);
}


// ローカル側のカレントディレクトリ取得
fs::path DoLocalPWD() {
	return fs::current_path();
}


// ファイルをゴミ箱に削除
static bool MoveFileToTrashCan(fs::path const& path) {
	auto zzpath = path.native() + L'\0';			// for PCZZSTR
	SHFILEOPSTRUCTW op{ 0, FO_DELETE, zzpath.c_str(), nullptr, FOF_SILENT | FOF_NOCONFIRMATION | FOF_ALLOWUNDO | FOF_NOERRORUI };
	return SHFileOperationW(&op) == 0;
}


// ローカル側のディレクトリ削除
void DoLocalRMD(fs::path const& path) {
	Notice(IDS_LOCALCMD, std::format(L"RMDIR {}"sv, path.native()));
	if (!MoveFileToTrashCan(path))
		SetTaskMsg(IDS_MSGJPN148);
}


// ローカル側のファイル削除
void DoLocalDELE(fs::path const& path) {
	Notice(IDS_LOCALCMD, std::format(L"DEL {}"sv, path.native()));
	if (!MoveFileToTrashCan(path))
		SetTaskMsg(IDS_MSGJPN150);
}


// ローカル側のファイル名変更
void DoLocalRENAME(fs::path const& src, fs::path const& dst) {
	Notice(IDS_LOCALCMD, std::format(L"REN {} {}"sv, src.native(), dst.native()));
	std::error_code ec;
	fs::rename(src, dst, ec);
	if (ec)
		SetTaskMsg(IDS_MSGJPN151);
}
