/*=============================================================================
*
*										ヒストリ
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

int FileHist = 5;
int PassToHist = YES;
static std::vector<HISTORYDATA> histories;
static constexpr std::array MenuHistId{
	MENU_HIST_1,  MENU_HIST_2,  MENU_HIST_3,  MENU_HIST_4,  MENU_HIST_5,
	MENU_HIST_6,  MENU_HIST_7,  MENU_HIST_8,  MENU_HIST_9,  MENU_HIST_10,
	MENU_HIST_11, MENU_HIST_12, MENU_HIST_13, MENU_HIST_14, MENU_HIST_15,
	MENU_HIST_16, MENU_HIST_17, MENU_HIST_18, MENU_HIST_19, MENU_HIST_20,
};


// ホスト情報をヒストリリストの先頭に追加する
void AddHostToHistory(Host const& host) {
	AddHistoryToHistory({ host, PassToHist == YES, AskTransferType() });
}


// ヒストリをヒストリリストの先頭に追加する
void AddHistoryToHistory(HISTORYDATA const& history) {
	if (size_as<int>(histories) == FileHist)
		histories.resize((size_t)FileHist - 1);
	histories.insert(begin(histories), history);
}


HOSTDATA::HOSTDATA(struct HISTORYDATA const& history) {
	CopyDefaultHost(this);
	static_cast<Host&>(*this) = Host{ history, PassToHist == YES };
}


// 全ヒストリをメニューにセット
void SetAllHistoryToMenu() {
	auto menu = GetSubMenu(GetMenu(GetMainHwnd()), 0);
	for (int i = DEF_FMENU_ITEMS, count = GetMenuItemCount(menu); i < count; i++)
		DeleteMenu(menu, DEF_FMENU_ITEMS, MF_BYPOSITION);
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	int i = 0;
	for (auto history : histories) {
		auto const text
			= i < 9 ? strprintf(L"&%d %s (%s) %s", i + 1, history.HostAdrs.c_str(), history.UserName.c_str(), history.RemoteInitDir.c_str())
			: i == 9 ? strprintf(L"&0 %s (%s) %s", history.HostAdrs.c_str(), history.UserName.c_str(), history.RemoteInitDir.c_str())
			: strprintf(L"&* %s (%s) %s", history.HostAdrs.c_str(), history.UserName.c_str(), history.RemoteInitDir.c_str());
		AppendMenuW(menu, MF_STRING, MenuHistId[i], text.c_str());
		i++;
	}
}


// 指定メニューコマンドに対応するヒストリを返す
//   MenuCmd : 取り出すヒストリに割り当てられたメニューコマンド (MENU_xxx)
std::optional<HISTORYDATA> GetHistoryByCmd(int menuId) {
	if (auto it = std::find(begin(MenuHistId), end(MenuHistId), menuId); it != end(MenuHistId))
		if (auto index = (size_t)std::distance(begin(MenuHistId), it); index < size(histories))
			return histories[index];
	return {};
}


std::vector<HISTORYDATA> const& GetHistories() {
	return histories;
}
