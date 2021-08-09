/*=============================================================================
*
*									ＲＡＳ関係
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
#include <Ras.h>
#include <RasDlg.h>
#include <RasError.h>
// これらはリンクオプションで遅延ロードに設定する
#pragma comment(lib, "Rasapi32.lib")
#pragma comment(lib, "Rasdlg.lib")

static auto GetConnections() {
	std::vector<RASCONNW> connections;
	if (DWORD size = 0, count = 0, result = RasEnumConnectionsW(nullptr, &size, &count); result == ERROR_BUFFER_TOO_SMALL) {
		connections.resize(size / sizeof(RASCONNW));
		size = sizeof(RASCONNW) * size_as<DWORD>(connections);
		connections[0].dwSize = sizeof(RASCONNW);
		result = RasEnumConnectionsW(data(connections), &size, &count);
		connections.resize(count);
	}
	return connections;
}

static void DisconnectAll(std::vector<RASCONNW> const& connections) {
	if (empty(connections))
		return;
	Notice(IDS_MSGJPN220);
	for (auto const& connection : connections) {
		RasHangUpW(connection.hrasconn);
		for (RASCONNSTATUSW status{ sizeof(RASCONNSTATUSW) }; RasGetConnectStatusW(connection.hrasconn, &status) != ERROR_INVALID_HANDLE;)
			Sleep(10);
	}
}

// RASを切断する
//   confirm: 切断前に確認を行う
void DisconnectRas(bool confirm) {
	if (auto connections = GetConnections(); !empty(connections) && (!confirm || Dialog(GetFtpInst(), rasnotify_dlg, GetMainHwnd())))
		DisconnectAll(connections);
}

// RASのエントリ一覧をコンボボックスにセットする
void SetRasEntryToComboBox(HWND hdlg, int item, std::wstring const& selected) {
	if (DWORD size = 0, count = 0, result = RasEnumEntriesW(nullptr, nullptr, nullptr, &size, &count); result == ERROR_BUFFER_TOO_SMALL) {
		std::vector<RASENTRYNAMEW> entries{ size / sizeof(RASENTRYNAMEW) };
		size = sizeof(RASENTRYNAMEW) * size_as<DWORD>(entries);
		result = RasEnumEntriesW(nullptr, nullptr, data(entries), &size, &count);
		for (DWORD i = 0; i < count; i++)
			SendDlgItemMessageW(hdlg, item, CB_ADDSTRING, 0, (LPARAM)entries[i].szEntryName);
		SendDlgItemMessageW(hdlg, item, CB_SELECTSTRING, -1, (LPARAM)selected.c_str());
	}
}

// RASの接続を行う
//   dialup: ダイアルアップ接続を行う
//   explicitly: nameで指定した接続先へ接続する
//   confirm: 既存の接続を切断する前に確認を行う
//   name: 接続先名
bool ConnectRas(bool dialup, bool explicitly, bool confirm, std::wstring const& name) {
	if (!dialup)
		return true;

	/* 現在の接続を確認 */
	if (auto connections = GetConnections(); !empty(connections)) {
		if (!explicitly)
			return true;
		for (auto const& connection : connections)
			if (name == connection.szEntryName)
				return true;
		if (!confirm || Dialog(GetFtpInst(), rasreconnect_dlg, GetMainHwnd()))
			DisconnectAll(connections);
		else
			return true;
	}

	/* 接続する */
	Notice(IDS_MSGJPN221);
	RASDIALDLG info{ sizeof(RASDIALDLG), GetMainHwnd() };
	return RasDialDlgW(nullptr, const_cast<LPWSTR>(name.c_str()), nullptr, &info);
}
