/*=============================================================================
*
*							ステータスウインドウ
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

static HWND hWndSbar = NULL;


// ステータスウインドウを作成する
int MakeStatusBarWindow() {
	hWndSbar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, 0, 0, 0, 0, GetMainHwnd(), 0, GetFtpInst(), nullptr);
	if (!hWndSbar)
		return FFFTP_FAIL;
	static int parts[]{ 120, 190, 340, 500, 660, -1 };
	for (auto& part : parts)
		if (part != -1)
			part = CalcPixelX(part);
	SendMessageW(hWndSbar, SB_SETPARTS, size_as<WPARAM>(parts), (LPARAM)parts);
	return FFFTP_SUCCESS;
}


// ステータスウインドウを削除
void DeleteStatusBarWindow() {
	if (hWndSbar)
		DestroyWindow(hWndSbar);
}


// ステータスウインドウのウインドウハンドルを返す
HWND GetSbarWnd() {
	return hWndSbar;
}


void UpdateStatusBar() {
	auto text = AskConnecting() == YES ? GetString(IsSecureConnection() ? IDS_SECURE : IDS_NOTSECURE) : L""s;
	SendMessageW(hWndSbar, SB_SETTEXTW, MAKEWORD(0, 0), (LPARAM)text.c_str());
}


// カレントウインドウを表示
void DispCurrentWindow(int Win) {
	auto resourceId = Win == WIN_LOCAL ? IDS_MSGJPN245 : Win == WIN_REMOTE ? IDS_MSGJPN246 : 0;
	SendMessageW(hWndSbar, SB_SETTEXTW, MAKEWORD(1, 0), (LPARAM)(resourceId == 0 ? L"" : GetString(resourceId).c_str()));
}


// 選択されているファイル数とサイズを表示
void DispSelectedSpace() {
	auto Win = GetFocus() == GetRemoteHwnd() ? WIN_REMOTE : WIN_LOCAL;
	char size[50];
	MakeSizeString(GetSelectedTotalSize(Win), size);
	char text[50];
	sprintf(text, MSGJPN247, GetSelectedCount(Win), size);
	SendMessageW(hWndSbar, SB_SETTEXTW, MAKEWORD(2, 0), (LPARAM)u8(text).c_str());
}


// ローカル側の空き容量を表示
void DispLocalFreeSpace(char *Path) {
	char size[40] = "??";
	if (ULARGE_INTEGER a; GetDiskFreeSpaceExW(fs::u8path(Path).c_str(), &a, nullptr, nullptr) != 0)
		MakeSizeString((double)a.QuadPart, size);
	char text[40];
	sprintf(text, MSGJPN248, size);
	SendMessageW(hWndSbar, SB_SETTEXTW, MAKEWORD(3, 0), (LPARAM)u8(text).c_str());
}


// 転送するファイルの数を表示
void DispTransferFiles() {
	char text[50];
	sprintf(text, MSGJPN249, AskTransferFileNum());
	SendMessageW(hWndSbar, SB_SETTEXTW, MAKEWORD(4, 0), (LPARAM)u8(text).c_str());
}


// 受信中のバイト数を表示
void DispDownloadSize(LONGLONG Size) {
	char text[50]{};
	if (0 <= Size) {
		char Tmp[50];
		MakeSizeString((double)Size, Tmp);
		sprintf(text, MSGJPN250, Tmp);
	}
	SendMessageW(hWndSbar, SB_SETTEXTW, MAKEWORD(5, 0), (LPARAM)u8(text).c_str());
}

bool NotifyStatusBar(const NMHDR* hdr) {
	if (hdr->hwndFrom != hWndSbar)
		return false;
	if (hdr->code == __pragma(warning(suppress:26454)) NM_CLICK)
		if (auto mouse = reinterpret_cast<const NMMOUSE*>(hdr); mouse->dwItemSpec == 0)
			ShowCertificate();
	return true;
}
