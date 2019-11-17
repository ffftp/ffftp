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

extern int FileHist;
extern int PassToHist;
static std::vector<HISTORYDATA> histories;
static constexpr std::array MenuHistId{
	MENU_HIST_1,  MENU_HIST_2,  MENU_HIST_3,  MENU_HIST_4,  MENU_HIST_5,
	MENU_HIST_6,  MENU_HIST_7,  MENU_HIST_8,  MENU_HIST_9,  MENU_HIST_10,
	MENU_HIST_11, MENU_HIST_12, MENU_HIST_13, MENU_HIST_14, MENU_HIST_15,
	MENU_HIST_16, MENU_HIST_17, MENU_HIST_18, MENU_HIST_19, MENU_HIST_20,
};


// ホスト情報をヒストリリストの先頭に追加する
void AddHostToHistory(HOSTDATA* Host, int TrMode) {
	HISTORYDATA New{ *Host, PassToHist == YES, TrMode };
	AddHistoryToHistory(&New);
}


// ヒストリをヒストリリストの先頭に追加する
void AddHistoryToHistory(HISTORYDATA* Hist) {
	CheckHistoryNum(1);
	if (size_as<int>(histories) < FileHist)
		histories.insert(begin(histories), *Hist);
}


// ヒストリの数を返す
int AskHistoryNum() {
	return size_as<int>(histories);
}


// ヒストリの数をチェックし多すぎたら削除
void CheckHistoryNum(int Space) {
	if (auto newsize = (size_t)FileHist - (size_t)Space; newsize < size(histories))
		histories.resize(newsize);
}


// ヒストリ情報をホスト情報にセット
void CopyHistoryToHost(HISTORYDATA* Hist, HOSTDATA* Host) {
	CopyDefaultHost(Host);
	static_cast<::Host&>(*Host) = ::Host{ *Hist, PassToHist == YES };
}


// ヒストリ情報の初期値を取得
void CopyDefaultHistory(HISTORYDATA* Set) {
	HOSTDATA Host;
	CopyDefaultDefaultHost(&Host);
	static_cast<::Host&>(*Set) = ::Host{ Host, PassToHist == YES };
}


// 全ヒストリをメニューにセット
void SetAllHistoryToMenu() {
	auto menu = GetSubMenu(GetMenu(GetMainHwnd()), 0);
	for (int i = DEF_FMENU_ITEMS, count = GetMenuItemCount(menu); i < count; i++)
		DeleteMenu(menu, DEF_FMENU_ITEMS, MF_BYPOSITION);
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
	int i = 0;
	for (auto history : histories) {
		char buffer[HOST_ADRS_LEN + USER_NAME_LEN + INIT_DIR_LEN + 7 + 1];
		if (i < 9)
			sprintf(buffer, "&%d %s (%s) %s", i + 1, history.HostAdrs, history.UserName, history.RemoteInitDir);
		else if (i == 9)
			sprintf(buffer, "&0 %s (%s) %s", history.HostAdrs, history.UserName, history.RemoteInitDir);
		else
			sprintf(buffer, "&* %s (%s) %s", history.HostAdrs, history.UserName, history.RemoteInitDir);
		AppendMenuW(menu, MF_STRING, MenuHistId[i], u8(buffer).c_str());
		i++;
	}
}


// 指定メニューコマンドに対応するヒストリを返す
//   MenuCmd : 取り出すヒストリに割り当てられたメニューコマンド (MENU_xxx)
int GetHistoryByCmd(int MenuCmd, HISTORYDATA* Buf) {
	auto it = std::find(begin(MenuHistId), end(MenuHistId), MenuCmd);
	return it == end(MenuHistId) ? FFFTP_FAIL : GetHistoryByNum((int)std::distance(begin(MenuHistId), it), Buf);
}


// 指定番号に対応するヒストリを返す
int GetHistoryByNum(int Num, HISTORYDATA* Buf) {
	if (size_as<int>(histories) <= Num)
		return FFFTP_FAIL;
	*Buf = histories[Num];
	return FFFTP_SUCCESS;
}
