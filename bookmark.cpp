/*=============================================================================
*
*								ブックマーク
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

extern HFONT ListFont;		/* リストボックスのフォント */
extern SIZE BmarkDlgSize;

struct Bookmark {
	std::wstring local;
	std::wstring remote;
	std::wstring line;
	Bookmark() = default;
	Bookmark(std::wstring const& line) : line{ line } {
		// 旧フォーマットはプレフィックスが付いていない
		static std::wregex re{ LR"(^(?:W (.*?) <> (.*)|L (.*)|(?:H )?(.*))$)" };
		if (std::wsmatch m; std::regex_search(line, m, re)) {
			local = m[1].matched ? m[1] : m[3];
			remote = m[2].matched ? m[2] : m[4];
		} else
			throw std::runtime_error("invalid bookmark.");
	}
	Bookmark(std::wstring const& local, std::wstring const& remote) : local{ local }, remote{ remote } {
		if (empty(local) && empty(remote))
			throw std::runtime_error("empty bookmark.");
		line = empty(remote) ? L"L "s + local : empty(local) ? L"H "s + remote : L"W "s + local + L" <> "s + remote;
	}
};

static std::vector<Bookmark> bookmarks;

// ブックマークをクリアする
void ClearBookMark() {
	auto menu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);
	for (int i = 0; i < size_as<int>(bookmarks); i++)
		DeleteMenu(menu, DEFAULT_BMARK_ITEM, MF_BYPOSITION);
	bookmarks.clear();
}

// ブックマークにパスを登録する
template<class... Args>
static void AddBookMark(Args&&... args) {
	auto menu = GetSubMenu(GetMenu(GetMainHwnd()), BMARK_SUB_MENU);
	auto menuId = MENU_BMARK_TOP + size_as<UINT_PTR>(bookmarks);
	auto const& bookmark = bookmarks.emplace_back(std::forward<Args>(args)...);
	AppendMenuW(menu, MF_STRING, menuId, bookmark.line.c_str());
}

// カレントディレクトリをブックマークに追加
void AddCurDirToBookMark(int Win) {
	char local[FMAX_PATH + 1] = "";
	char remote[FMAX_PATH + 1] = "";
	if (Win != WIN_REMOTE)
		AskLocalCurDir(local, FMAX_PATH);
	if (Win != WIN_LOCAL)
		AskRemoteCurDir(remote, FMAX_PATH);
	AddBookMark(u8(local), u8(remote));
}

// 指定のIDを持つブックマークのパスを返す
std::tuple<std::wstring, std::wstring> AskBookMarkText(int MarkID) {
	auto bookmark = bookmarks[MarkID - MENU_BMARK_TOP];
	return { bookmark.local, bookmark.remote };
}

// ブックマークを接続中のホストリストに保存する
void SaveBookMark() {
	if (AskConnecting() == YES)
		if (auto CurHost = AskCurrentHost(); CurHost != HOSTNUM_NOENTRY) {
			auto u8total = "\0"s;
			if (!empty(bookmarks)) {
				auto total = L""s;
				for (auto const& bookmark : bookmarks) {
					total += bookmark.line;
					total += L'\0';
				}
				u8total = u8(total);
			}
			SetHostBookMark(CurHost, data(u8total), size_as<int>(u8total) + 1);
		}
}

// ホストリストからブックマークを読み込む
void LoadBookMark() {
	if (AskConnecting() == YES)
		if (auto CurHost = AskCurrentHost(); CurHost != HOSTNUM_NOENTRY)
			if (auto p = AskHostBookMark(CurHost)) {
				ClearBookMark();
				for (; *p; p += strlen(p) + 1)
					AddBookMark(u8(p));
			}
}

struct Editor {
	using result_t = bool;
	Bookmark bookmark;
	Editor() = default;
	Editor(Bookmark const& bookmark) : bookmark{ bookmark } {}
	INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, BEDIT_LOCAL, EM_LIMITTEXT, FMAX_PATH - 1, 0);
		SendDlgItemMessageW(hDlg, BEDIT_REMOTE, EM_LIMITTEXT, FMAX_PATH - 1, 0);
		if (!empty(bookmark.remote))
			SendDlgItemMessageW(hDlg, BEDIT_REMOTE, WM_SETTEXT, 0, (LPARAM)bookmark.remote.c_str());
		if (!empty(bookmark.local))
			SendDlgItemMessageW(hDlg, BEDIT_LOCAL, WM_SETTEXT, 0, (LPARAM)bookmark.local.c_str());
		else {
			/* ホスト側にカーソルを移動しておく */
			SetFocus(GetDlgItem(hDlg, BEDIT_REMOTE));
			SendDlgItemMessageW(hDlg, BEDIT_REMOTE, EM_SETSEL, 0, -1);
		}
		return TRUE;
	}
	void OnCommand(HWND hDlg, WORD commandId) {
		switch (commandId) {
		case IDOK:
			if (auto local = GetText(hDlg, BEDIT_LOCAL), remote = GetText(hDlg, BEDIT_REMOTE); !empty(local) || !empty(remote)) {
				bookmark = { local, remote };
				EndDialog(hDlg, true);
				break;
			}
			[[fallthrough]];
		case IDCANCEL:
			EndDialog(hDlg, false);
			break;
		}
	}
};

struct List {
	using result_t = LRESULT;
	Resizable<Controls<BMARK_NEW, BMARK_SET, BMARK_DEL, BMARK_DOWN, BMARK_UP, IDHELP, BMARK_SIZEGRIP>, Controls<IDOK, BMARK_JUMP, BMARK_SIZEGRIP>, Controls<BMARK_LIST>> resizable{ BmarkDlgSize };
	std::vector<Bookmark> bookmarks;
	List(std::vector<Bookmark> const& bookmarks) : bookmarks{ bookmarks } {}
	INT_PTR OnInit(HWND hDlg) {
		auto hList = GetDlgItem(hDlg, BMARK_LIST);
		if (ListFont != NULL)
			SendMessageW(hList, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
		for (auto const& bookmark : bookmarks)
			SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)bookmark.line.c_str());
		return TRUE;
	}
	void OnCommand(HWND hDlg, WORD commandId) {
		auto hList = GetDlgItem(hDlg, BMARK_LIST);
		switch (commandId) {
		case BMARK_JUMP:
		case IDCANCEL:
		case IDOK:
			EndDialog(hDlg, commandId == BMARK_JUMP ? SendMessageW(hList, LB_GETCURSEL, 0, 0) : LB_ERR);
			break;
		case BMARK_SET:
			if (auto index = SendMessageW(hList, LB_GETCURSEL, 0, 0); index != LB_ERR)
				if (Editor editor{ bookmarks[index] }; Dialog(GetFtpInst(), bmark_edit_dlg, hDlg, editor)) {
					bookmarks[index] = editor.bookmark;
					SendMessageW(hList, LB_DELETESTRING, index, 0);
					SendMessageW(hList, LB_INSERTSTRING, index, (LPARAM)editor.bookmark.line.c_str());
					SendMessageW(hList, LB_SETCURSEL, index, 0);
				}
			break;
		case BMARK_NEW:
			if (Editor editor; Dialog(GetFtpInst(), bmark_edit_dlg, hDlg, editor)) {
				bookmarks.emplace_back(editor.bookmark);
				auto index = SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)editor.bookmark.line.c_str());
				SendMessageW(hList, LB_SETCURSEL, index, 0);
			}
			break;
		case BMARK_DEL:
			if (auto index = SendMessageW(hList, LB_GETCURSEL, 0, 0); index != LB_ERR) {
				bookmarks.erase(begin(bookmarks) + index);
				SendMessageW(hList, LB_DELETESTRING, index, 0);
				if (!empty(bookmarks))
					SendMessageW(hList, LB_SETCURSEL, std::min(index, size_as<LRESULT>(bookmarks) - 1), 0);
			}
			break;
		case BMARK_UP:
			if (auto index = SendMessageW(hList, LB_GETCURSEL, 0, 0); index != LB_ERR)
				if (0 < index) {
					std::swap(bookmarks[index - 1], bookmarks[index]);
					SendMessageW(hList, LB_DELETESTRING, index, 0);
					SendMessageW(hList, LB_INSERTSTRING, index - 1, (LPARAM)bookmarks[index - 1].line.c_str());
					SendMessageW(hList, LB_SETCURSEL, index - 1, 0);
				}
			break;
		case BMARK_DOWN:
			if (auto index = SendMessageW(hList, LB_GETCURSEL, 0, 0); index != LB_ERR)
				if (index < size_as<LRESULT>(bookmarks) - 1) {
					std::swap(bookmarks[index], bookmarks[index + 1]);
					SendMessageW(hList, LB_DELETESTRING, index, 0);
					SendMessageW(hList, LB_INSERTSTRING, index + 1, (LPARAM)bookmarks[index + 1].line.c_str());
					SendMessageW(hList, LB_SETCURSEL, index + 1, 0);
				}
			break;
		case IDHELP:
			hHelpWin = HtmlHelp(NULL, AskHelpFilePath(), HH_HELP_CONTEXT, IDH_HELP_TOPIC_0000019);
			break;
		}
	}
};

// ブックマーク編集ウインドウ
void EditBookMark() {
	List editor{ bookmarks };
	auto index = Dialog(GetFtpInst(), bmark_dlg, GetMainHwnd(), editor);
	ClearBookMark();
	for (auto const& bookmark : editor.bookmarks)
		AddBookMark(bookmark);
	if (index != LB_ERR)
		PostMessageW(GetMainHwnd(), WM_COMMAND, MAKEWPARAM(MENU_BMARK_TOP + index, 0), 0);
}
