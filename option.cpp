/*=============================================================================
*
*								オプション設定
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


static auto GetMultiTextFromList(HWND hdlg, int id) {
	std::wstring lines;
	std::wstring line;
	for (int i = 0, count = (int)SendDlgItemMessageW(hdlg, id, LB_GETCOUNT, 0, 0); i < count; i++) {
		auto length = SendDlgItemMessageW(hdlg, id, LB_GETTEXTLEN, i, 0);
		line.resize(length);
		length = SendDlgItemMessageW(hdlg, id, LB_GETTEXT, i, (LPARAM)data(line));
		lines.append(begin(line), begin(line) + length);
		lines += L'\0';
	}
	lines += L'\0';
	return lines;
}
static void AddFnameAttrToListView(HWND hDlg, char *Fname, char *Attr);
static void GetFnameAttrFromListView(HWND hDlg, char *Buf);
int GetDecimalText(HWND hDlg, int Ctrl);
void SetDecimalText(HWND hDlg, int Ctrl, int Num);
void CheckRange2(int *Cur, int Max, int Min);
void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize);
void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text);
void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize);


/* 設定値 */
extern char UserMailAdrs[USER_MAIL_LEN+1];
extern char ViewerName[VIEWERS][FMAX_PATH+1];
extern int ConnectOnStart;
extern int SaveWinPos;
extern char AsciiExt[ASCII_EXT_LEN+1];
extern int RecvMode;
extern int SendMode;
extern int MoveMode;
extern char FwallHost[HOST_ADRS_LEN+1];
extern char FwallUser[USER_NAME_LEN+1];
extern char FwallPass[PASSWORD_LEN+1];
extern int FwallPort;
extern int FwallType;
extern int FwallDefault;
extern int FwallSecurity;
extern int FwallResolve;
extern int FwallLower;
extern int FwallDelimiter;
extern int PasvDefault;
extern char DefaultLocalPath[FMAX_PATH+1];
extern int SaveTimeStamp;
extern int DclickOpen;
extern int FnameCnv;
extern int ConnectAndSet;
extern int TimeOut;
extern int RmEOF;
extern int RegType;
extern char MirrorNoTrn[MIRROR_LEN+1];
extern char MirrorNoDel[MIRROR_LEN+1];
extern int MirrorFnameCnv;
extern int RasClose;
extern int RasCloseNotify;
extern int FileHist;
extern char DefAttrList[DEFATTRLIST_LEN+1];
extern int QuickAnonymous;
extern int PassToHist;
extern int VaxSemicolon;
extern int SendQuit;
extern int NoRasControl;
extern int DispDrives;
extern HFONT ListFont;
LOGFONTW ListLogFont;
extern int MirUpDelNotify;
extern int MirDownDelNotify;
extern int FolderAttr;
extern int FolderAttrNum;
// ファイルアイコン表示対応
extern int DispFileIcon;
// タイムスタンプのバグ修正
extern int DispTimeSeconds;
// ファイルの属性を数字で表示
extern int DispPermissionsNumber;
// ディレクトリ自動作成
extern int MakeAllDir;
// UPnP対応
extern int UPnPEnabled;
// 全設定暗号化対応
extern int EncryptAllSettings;
// ローカル側自動更新
extern int AutoRefreshFileList;
// 古い処理内容を消去
extern int RemoveOldLog;
// ファイル一覧バグ修正
extern int AbortOnListError;
// ミラーリング設定追加
extern int MirrorNoTransferContents;
// FireWall設定追加
extern int FwallNoSaveUser;
// ゾーンID設定追加
extern int MarkAsInternet;


// ファイル属性設定ウインドウ
struct DefAttr {
	using result_t = bool;
	std::wstring filename;
	std::wstring attr;
	INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, DEFATTR_FNAME, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessageW(hDlg, DEFATTR_ATTR, EM_LIMITTEXT, 4, 0);
		return TRUE;
	}
	void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case IDOK:
			filename = GetText(hDlg, DEFATTR_FNAME);
			attr = GetText(hDlg, DEFATTR_ATTR);
			EndDialog(hDlg, true);
			break;
		case IDCANCEL:
			EndDialog(hDlg, false);
			break;
		case DEFATTR_ATTR_BR:
			if (auto newattr = ChmodDialog(GetText(hDlg, DEFATTR_ATTR)))
				SetText(hDlg, DEFATTR_ATTR, newattr->c_str());
			break;
		}
	}
};


struct User {
	static constexpr WORD dialogId = opt_user_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, USER_ADRS, EM_LIMITTEXT, PASSWORD_LEN, 0);
		SetText(hDlg, USER_ADRS, u8(UserMailAdrs));
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			strncpy_s(UserMailAdrs, USER_MAIL_LEN + 1, u8(GetText(hDlg, USER_ADRS)).c_str(), _TRUNCATE);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000041);
			break;
		}
		return 0;
	}
};

struct Transfer1 {
	static constexpr WORD dialogId = opt_trmode1_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using ModeButton = RadioButton<TRMODE_AUTO, TRMODE_ASCII, TRMODE_BIN>;
	static INT_PTR OnInit(HWND hDlg) {
		SetMultiTextToList(hDlg, TRMODE_EXT_LIST, AsciiExt);
		ModeButton::Set(hDlg, AskTransferType());
		SendDlgItemMessageW(hDlg, TRMODE_TIME, BM_SETCHECK, SaveTimeStamp, 0);
		SendDlgItemMessageW(hDlg, TRMODE_EOF, BM_SETCHECK, RmEOF, 0);
		SendDlgItemMessageW(hDlg, TRMODE_SEMICOLON, BM_SETCHECK, VaxSemicolon, 0);
		SendDlgItemMessageW(hDlg, TRMODE_MAKEDIR, BM_SETCHECK, MakeAllDir, 0);
		SendDlgItemMessageW(hDlg, TRMODE_LISTERROR, BM_SETCHECK, AbortOnListError, 0);
		SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(TRMODE_EXT_LIST, 0), 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SetTransferTypeImm(ModeButton::Get(hDlg));
			SaveTransferType();
			GetMultiTextFromList(hDlg, TRMODE_EXT_LIST, AsciiExt, ASCII_EXT_LEN + 1);
			SaveTimeStamp = (int)SendDlgItemMessageW(hDlg, TRMODE_TIME, BM_GETCHECK, 0, 0);
			RmEOF = (int)SendDlgItemMessageW(hDlg, TRMODE_EOF, BM_GETCHECK, 0, 0);
			VaxSemicolon = (int)SendDlgItemMessageW(hDlg, TRMODE_SEMICOLON, BM_GETCHECK, 0, 0);
			MakeAllDir = (int)SendDlgItemMessageW(hDlg, TRMODE_MAKEDIR, BM_GETCHECK, 0, 0);
			AbortOnListError = (int)SendDlgItemMessageW(hDlg, TRMODE_LISTERROR, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000042);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case TRMODE_ASCII:
		case TRMODE_BIN:
			EnableWindow(GetDlgItem(hDlg, TRMODE_EXT_LIST), FALSE);
			EnableWindow(GetDlgItem(hDlg, TRMODE_ADD), FALSE);
			EnableWindow(GetDlgItem(hDlg, TRMODE_DEL), FALSE);
			break;
		case TRMODE_AUTO:
			EnableWindow(GetDlgItem(hDlg, TRMODE_EXT_LIST), TRUE);
			EnableWindow(GetDlgItem(hDlg, TRMODE_ADD), TRUE);
			EnableWindow(GetDlgItem(hDlg, TRMODE_DEL), TRUE);
			break;
		case TRMODE_ADD:
			if (char Tmp[FMAX_PATH + 1] = ""; InputDialog(fname_in_dlg, hDlg, MSGJPN199, Tmp, FMAX_PATH))
				AddTextToListBox(hDlg, Tmp, TRMODE_EXT_LIST, ASCII_EXT_LEN + 1);
			break;
		case TRMODE_DEL:
			if (auto Num = (int)SendDlgItemMessageW(hDlg, TRMODE_EXT_LIST, LB_GETCURSEL, 0, 0); Num != LB_ERR)
				SendDlgItemMessageW(hDlg, TRMODE_EXT_LIST, LB_DELETESTRING, Num, 0);
			break;
		}
	}
};

struct Transfer2 {
	static constexpr WORD dialogId = opt_trmode2_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using CnvButton = RadioButton<TRMODE2_NOCNV, TRMODE2_LOWER, TRMODE2_UPPER>;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, TRMODE2_LOCAL, EM_LIMITTEXT, FMAX_PATH, 0);
		SetText(hDlg, TRMODE2_LOCAL, u8(DefaultLocalPath));
		CnvButton::Set(hDlg, FnameCnv);
		SendDlgItemMessageW(hDlg, TRMODE2_TIMEOUT, EM_LIMITTEXT, (WPARAM)5, 0);
		char Tmp[FMAX_PATH + 1];
		sprintf(Tmp, "%d", TimeOut);
		SetText(hDlg, TRMODE2_TIMEOUT, u8(Tmp));
		SendDlgItemMessageW(hDlg, TRMODE2_TIMEOUT_SPN, UDM_SETRANGE, 0, MAKELONG(300, 0));
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			strncpy_s(DefaultLocalPath, FMAX_PATH + 1, u8(GetText(hDlg, TRMODE2_LOCAL)).c_str(), _TRUNCATE);
			FnameCnv = CnvButton::Get(hDlg);
			TimeOut = GetDecimalText(hDlg, TRMODE2_TIMEOUT);
			CheckRange2(&TimeOut, 300, 0);
			return PSNRET_NOERROR;
		}
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000043);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case TRMODE2_LOCAL_BR:
			if (char Tmp[FMAX_PATH + 1]; SelectDir(hDlg, Tmp, FMAX_PATH) == TRUE)
				SetText(hDlg, TRMODE2_LOCAL, u8(Tmp));
			break;
		}
	}
};

struct Transfer3 {
	static constexpr WORD dialogId = opt_trmode3_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		RECT Rect;
		GetClientRect(GetDlgItem(hDlg, TRMODE3_LIST), &Rect);

		const std::tuple<int, int> columns[]{
			{ Rect.right / 3 * 2, IDS_MSGJPN200 },
			{ Rect.right / 3 * 1, IDS_MSGJPN201 },
		};
		int i = 0;
		for (auto [cx, resourceId] : columns) {
			auto text = GetString(resourceId);
			LVCOLUMNW column{ LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM, 0, cx, data(text), 0, i };
			SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_INSERTCOLUMNW, i++, (LPARAM)&column);
		}

		auto Fname = DefAttrList;
		while (*Fname != NUL) {
			auto Attr = strchr(Fname, NUL) + 1;
			if (*Attr != NUL)
				AddFnameAttrToListView(hDlg, Fname, Attr);
			Fname = strchr(Attr, NUL) + 1;
		}

		SendDlgItemMessageW(hDlg, TRMODE3_FOLDER, BM_SETCHECK, FolderAttr, 0);
		if (FolderAttr == NO)
			EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), FALSE);

		SendDlgItemMessageW(hDlg, TRMODE3_FOLDER_ATTR, EM_LIMITTEXT, (WPARAM)5, 0);
		char TmpStr[10];
		sprintf(TmpStr, "%03d", FolderAttrNum);
		SetText(hDlg, TRMODE3_FOLDER_ATTR, u8(TmpStr));
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			GetFnameAttrFromListView(hDlg, DefAttrList);
			FolderAttrNum = GetDecimalText(hDlg, TRMODE3_FOLDER_ATTR);
			FolderAttr = (int)SendDlgItemMessageW(hDlg, TRMODE3_FOLDER, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		}
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000044);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case TRMODE3_ADD:
			if (DefAttr data; Dialog(GetFtpInst(), def_attr_dlg, hDlg, data) && !empty(data.filename) && !empty(data.attr))
				AddFnameAttrToListView(hDlg, const_cast<char*>(u8(data.filename).c_str()), const_cast<char*>(u8(data.attr).c_str()));
			break;
		case TRMODE3_DEL:
			if (auto Tmp = (long)SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_GETNEXTITEM, -1, LVNI_ALL | LVNI_SELECTED); Tmp != -1)
				SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_DELETEITEM, Tmp, 0);
			break;
		case TRMODE3_FOLDER:
			if (SendDlgItemMessageW(hDlg, TRMODE3_FOLDER, BM_GETCHECK, 0, 0) == 1)
				EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), TRUE);
			else
				EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), FALSE);
			break;
		}
	}
};

struct Transfer4 {
	static constexpr WORD dialogId = opt_trmode4_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using KanjiButton = RadioButton<TRMODE4_SJIS_CNV, TRMODE4_JIS_CNV, TRMODE4_EUC_CNV, TRMODE4_UTF8N_CNV, TRMODE4_UTF8BOM_CNV>;
	static INT_PTR OnInit(HWND hDlg) {
		KanjiButton::Set(hDlg, AskLocalKanjiCode());
		if (IsZoneIDLoaded())
			SendDlgItemMessageW(hDlg, TRMODE4_MARK_INTERNET, BM_SETCHECK, MarkAsInternet, 0);
		else {
			SendDlgItemMessageW(hDlg, TRMODE4_MARK_INTERNET, BM_SETCHECK, BST_UNCHECKED, 0);
			EnableWindow(GetDlgItem(hDlg, TRMODE4_MARK_INTERNET), FALSE);
		}
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SetLocalKanjiCodeImm(KanjiButton::Get(hDlg));
			SaveLocalKanjiCode();
			if (IsZoneIDLoaded())
				MarkAsInternet = (int)SendDlgItemMessageW(hDlg, TRMODE4_MARK_INTERNET, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000067);
			break;
		}
		return 0;
	}
};

struct Mirroring {
	static constexpr WORD dialogId = opt_mirror_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SetMultiTextToList(hDlg, MIRROR_NOTRN_LIST, MirrorNoTrn);
		SetMultiTextToList(hDlg, MIRROR_NODEL_LIST, MirrorNoDel);
		SendDlgItemMessageW(hDlg, MIRROR_LOW, BM_SETCHECK, MirrorFnameCnv, 0);
		SendDlgItemMessageW(hDlg, MIRROR_UPDEL_NOTIFY, BM_SETCHECK, MirUpDelNotify, 0);
		SendDlgItemMessageW(hDlg, MIRROR_DOWNDEL_NOTIFY, BM_SETCHECK, MirDownDelNotify, 0);
		SendDlgItemMessageW(hDlg, MIRROR_NO_TRANSFER, BM_SETCHECK, MirrorNoTransferContents, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			GetMultiTextFromList(hDlg, MIRROR_NOTRN_LIST, MirrorNoTrn, MIRROR_LEN + 1);
			GetMultiTextFromList(hDlg, MIRROR_NODEL_LIST, MirrorNoDel, MIRROR_LEN + 1);
			MirrorFnameCnv = (int)SendDlgItemMessageW(hDlg, MIRROR_LOW, BM_GETCHECK, 0, 0);
			MirUpDelNotify = (int)SendDlgItemMessageW(hDlg, MIRROR_UPDEL_NOTIFY, BM_GETCHECK, 0, 0);
			MirDownDelNotify = (int)SendDlgItemMessageW(hDlg, MIRROR_DOWNDEL_NOTIFY, BM_GETCHECK, 0, 0);
			MirrorNoTransferContents = (int)SendDlgItemMessageW(hDlg, MIRROR_NO_TRANSFER, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000045);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case MIRROR_NOTRN_ADD:
			if (char Tmp[FMAX_PATH + 1] = ""; InputDialog(fname_in_dlg, hDlg, MSGJPN202, Tmp, FMAX_PATH))
				AddTextToListBox(hDlg, Tmp, MIRROR_NOTRN_LIST, MIRROR_LEN + 1);
			break;
		case MIRROR_NODEL_ADD:
			if (char Tmp[FMAX_PATH + 1] = ""; InputDialog(fname_in_dlg, hDlg, MSGJPN203, Tmp, FMAX_PATH))
				AddTextToListBox(hDlg, Tmp, MIRROR_NODEL_LIST, MIRROR_LEN + 1);
			break;
		case MIRROR_NOTRN_DEL:
			if (auto Num = (int)SendDlgItemMessageW(hDlg, MIRROR_NOTRN_LIST, LB_GETCURSEL, 0, 0); Num != LB_ERR)
				SendDlgItemMessageW(hDlg, MIRROR_NOTRN_LIST, LB_DELETESTRING, Num, 0);
			break;
		case MIRROR_NODEL_DEL:
			if (auto Num = (int)SendDlgItemMessageW(hDlg, MIRROR_NODEL_LIST, LB_GETCURSEL, 0, 0); Num != LB_ERR)
				SendDlgItemMessageW(hDlg, MIRROR_NODEL_LIST, LB_DELETESTRING, Num, 0);
			break;
		}
	}
};

struct Operation {
	static constexpr WORD dialogId = opt_notify_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using DownButton = RadioButton<NOTIFY_D_DLG, NOTIFY_D_OVW>;
	using UpButton = RadioButton<NOTIFY_U_DLG, NOTIFY_U_OVW>;
	using DclickButton = RadioButton<NOTIFY_OPEN, NOTIFY_DOWNLOAD>;
	using MoveButton = RadioButton<NOTIFY_M_NODLG, NOTIFY_M_DLG, NOTIFY_M_DISABLE>;
	static INT_PTR OnInit(HWND hDlg) {
		DownButton::Set(hDlg, RecvMode);
		UpButton::Set(hDlg, SendMode);
		DclickButton::Set(hDlg, DclickOpen);
		MoveButton::Set(hDlg, MoveMode);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			RecvMode = DownButton::Get(hDlg);
			SendMode = UpButton::Get(hDlg);
			DclickOpen = DclickButton::Get(hDlg);
			MoveMode = MoveButton::Get(hDlg);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000046);
			break;
		}
		return 0;
	}
};

struct View1 {
	static constexpr WORD dialogId = opt_disp1_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static inline LOGFONTW TmpFont;
	static INT_PTR OnInit(HWND hDlg) {
		TmpFont = ListLogFont;
		if (ListFont != NULL)
			SetText(hDlg, DISP_FONT, TmpFont.lfFaceName);
		SendDlgItemMessageW(hDlg, DISP_HIDE, BM_SETCHECK, DispIgnoreHide, 0);
		SendDlgItemMessageW(hDlg, DISP_DRIVE, BM_SETCHECK, DispDrives, 0);
		SendDlgItemMessageW(hDlg, DISP_ICON, BM_SETCHECK, DispFileIcon, 0);
		SendDlgItemMessageW(hDlg, DISP_SECOND, BM_SETCHECK, DispTimeSeconds, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			DispIgnoreHide = (int)SendDlgItemMessageW(hDlg, DISP_HIDE, BM_GETCHECK, 0, 0);
			DispDrives = (int)SendDlgItemMessageW(hDlg, DISP_DRIVE, BM_GETCHECK, 0, 0);
			DispFileIcon = (int)SendDlgItemMessageW(hDlg, DISP_ICON, BM_GETCHECK, 0, 0);
			DispTimeSeconds = (int)SendDlgItemMessageW(hDlg, DISP_SECOND, BM_GETCHECK, 0, 0);
			if (wcslen(TmpFont.lfFaceName) > 0) {
				ListLogFont = TmpFont;
				ListFont = CreateFontIndirectW(&ListLogFont);
			}
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000047);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		static CHOOSEFONTW chooseFont;
		switch (id) {
		case DISP_FONT_BR:
			chooseFont = { sizeof(CHOOSEFONTW), hDlg, 0, &TmpFont, 0, CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT, 0, 0, nullptr, nullptr, 0, nullptr, SCREEN_FONTTYPE };
			if (ChooseFontW(&chooseFont))
				SetText(hDlg, DISP_FONT, TmpFont.lfFaceName);
			break;
		}
	}
};

struct View2 {
	static constexpr WORD dialogId = opt_disp2_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, DISP2_PERMIT_NUM, BM_SETCHECK, DispPermissionsNumber, 0);
		SendDlgItemMessageW(hDlg, DISP2_AUTO_REFRESH, BM_SETCHECK, AutoRefreshFileList, 0);
		SendDlgItemMessageW(hDlg, DISP2_REMOVE_OLD_LOG, BM_SETCHECK, RemoveOldLog, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			DispPermissionsNumber = (int)SendDlgItemMessageW(hDlg, DISP2_PERMIT_NUM, BM_GETCHECK, 0, 0);
			AutoRefreshFileList = (int)SendDlgItemMessageW(hDlg, DISP2_AUTO_REFRESH, BM_GETCHECK, 0, 0);
			RemoveOldLog = (int)SendDlgItemMessageW(hDlg, DISP2_REMOVE_OLD_LOG, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000068);
			break;
		}
		return 0;
	}
};

struct Connecting {
	static constexpr WORD dialogId = opt_connect_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, CONNECT_CONNECT, BM_SETCHECK, ConnectOnStart, 0);
		SendDlgItemMessageW(hDlg, CONNECT_OLDDLG, BM_SETCHECK, ConnectAndSet, 0);
		SendDlgItemMessageW(hDlg, CONNECT_RASCLOSE, BM_SETCHECK, RasClose, 0);
		if (NoRasControl != NO)
			EnableWindow(GetDlgItem(hDlg, CONNECT_RASCLOSE), FALSE);
		SendDlgItemMessageW(hDlg, CONNECT_CLOSE_NOTIFY, BM_SETCHECK, RasCloseNotify, 0);
		if (RasClose == NO || NoRasControl != NO)
			EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), FALSE);
		SendDlgItemMessageW(hDlg, CONNECT_HIST, EM_LIMITTEXT, (WPARAM)2, 0);
		SetDecimalText(hDlg, CONNECT_HIST, FileHist);
		SendDlgItemMessageW(hDlg, CONNECT_HIST_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(HISTORY_MAX, 0));
		SendDlgItemMessageW(hDlg, CONNECT_QUICK_ANONY, BM_SETCHECK, QuickAnonymous, 0);
		SendDlgItemMessageW(hDlg, CONNECT_HIST_PASS, BM_SETCHECK, PassToHist, 0);
		SendDlgItemMessageW(hDlg, CONNECT_SENDQUIT, BM_SETCHECK, SendQuit, 0);
		SendDlgItemMessageW(hDlg, CONNECT_NORAS, BM_SETCHECK, NoRasControl, 0);
		SendDlgItemMessageW(hDlg, CONNECT_UPNP, BM_SETCHECK, UPnPEnabled, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			ConnectOnStart = (int)SendDlgItemMessageW(hDlg, MISC_CONNECT, BM_GETCHECK, 0, 0);
			ConnectAndSet = (int)SendDlgItemMessageW(hDlg, MISC_OLDDLG, BM_GETCHECK, 0, 0);
			RasClose = (int)SendDlgItemMessageW(hDlg, CONNECT_RASCLOSE, BM_GETCHECK, 0, 0);
			RasCloseNotify = (int)SendDlgItemMessageW(hDlg, CONNECT_CLOSE_NOTIFY, BM_GETCHECK, 0, 0);
			FileHist = GetDecimalText(hDlg, CONNECT_HIST);
			CheckRange2(&FileHist, HISTORY_MAX, 0);
			QuickAnonymous = (int)SendDlgItemMessageW(hDlg, CONNECT_QUICK_ANONY, BM_GETCHECK, 0, 0);
			PassToHist = (int)SendDlgItemMessageW(hDlg, CONNECT_HIST_PASS, BM_GETCHECK, 0, 0);
			SendQuit = (int)SendDlgItemMessageW(hDlg, CONNECT_SENDQUIT, BM_GETCHECK, 0, 0);
			NoRasControl = (int)SendDlgItemMessageW(hDlg, CONNECT_NORAS, BM_GETCHECK, 0, 0);
			UPnPEnabled = (int)SendDlgItemMessageW(hDlg, CONNECT_UPNP, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000048);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case CONNECT_RASCLOSE:
			if (SendDlgItemMessageW(hDlg, CONNECT_RASCLOSE, BM_GETCHECK, 0, 0) == 1)
				EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), TRUE);
			else
				EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), FALSE);
			break;
		}
	}
};

struct Firewall {
	static constexpr WORD dialogId = opt_fire_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static constexpr int firewallTypes[] = { 0, 1, 2, 8, 3, 4, 5, 6, 7, 9 };
	static constexpr int HideTbl[9][6] = {
		{ TRUE,  TRUE,  TRUE,  FALSE, TRUE,  FALSE },
		{ TRUE,  TRUE,  TRUE,  FALSE, FALSE, TRUE  },
		{ TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE },
		{ FALSE, FALSE, FALSE, FALSE, FALSE, TRUE  },
		{ FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE },
		{ TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE },
		{ FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE },
		{ TRUE,  TRUE,  FALSE, TRUE,  FALSE, FALSE },
		{ TRUE,  TRUE,  FALSE, FALSE, TRUE,  TRUE  }
	};
	static INT_PTR OnInit(HWND hDlg) {
		UINT_PTR Type = std::distance(std::begin(firewallTypes), std::find(std::begin(firewallTypes), std::end(firewallTypes), FwallType));
		for (auto resourceId : { IDS_MSGJPN204, IDS_MSGJPN205, IDS_MSGJPN206, IDS_MSGJPN207, IDS_MSGJPN208, IDS_MSGJPN209, IDS_MSGJPN210, IDS_MSGJPN211, IDS_MSGJPN294 })
			SendDlgItemMessageW(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)GetString(resourceId).c_str());
		SendDlgItemMessageW(hDlg, FIRE_TYPE, CB_SETCURSEL, Type - 1, 0);

		SendDlgItemMessageW(hDlg, FIRE_HOST, EM_LIMITTEXT, HOST_ADRS_LEN, 0);
		SendDlgItemMessageW(hDlg, FIRE_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
		SendDlgItemMessageW(hDlg, FIRE_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
		SendDlgItemMessageW(hDlg, FIRE_PORT, EM_LIMITTEXT, 5, 0);
		SendDlgItemMessageW(hDlg, FIRE_DELIMIT, EM_LIMITTEXT, 1, 0);

		SetText(hDlg, FIRE_HOST, u8(FwallHost));
		SetText(hDlg, FIRE_USER, u8(FwallUser));
		SetText(hDlg, FIRE_PASS, u8(FwallPass));
		char Tmp[10];
		sprintf(Tmp, "%d", FwallPort);
		SetText(hDlg, FIRE_PORT, u8(Tmp));
		sprintf(Tmp, "%c", FwallDelimiter);
		SetText(hDlg, FIRE_DELIMIT, u8(Tmp));

		SendDlgItemMessageW(hDlg, FIRE_USEIT, BM_SETCHECK, FwallDefault, 0);
		SendDlgItemMessageW(hDlg, FIRE_PASV, BM_SETCHECK, PasvDefault, 0);
		SendDlgItemMessageW(hDlg, FIRE_RESOLV, BM_SETCHECK, FwallResolve, 0);
		SendDlgItemMessageW(hDlg, FIRE_LOWER, BM_SETCHECK, FwallLower, 0);

		for (auto resourceId : { IDS_MSGJPN212, IDS_MSGJPN213, IDS_MSGJPN214, IDS_MSGJPN215, IDS_MSGJPN216 })
			SendDlgItemMessageW(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)GetString(resourceId).c_str());
		SendDlgItemMessageW(hDlg, FIRE_SECURITY, CB_SETCURSEL, FwallSecurity, 0);

		SendDlgItemMessageW(hDlg, FIRE_SHARED, BM_SETCHECK, FwallNoSaveUser, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			auto Type = (int)SendDlgItemMessageW(hDlg, FIRE_TYPE, CB_GETCURSEL, 0, 0) + 1;
			FwallType = firewallTypes[Type];
			strncpy_s(FwallHost, HOST_ADRS_LEN + 1, u8(GetText(hDlg, FIRE_HOST)).c_str(), _TRUNCATE);
			strncpy_s(FwallUser, USER_NAME_LEN + 1, u8(GetText(hDlg, FIRE_USER)).c_str(), _TRUNCATE);
			strncpy_s(FwallPass, PASSWORD_LEN, u8(GetText(hDlg, FIRE_PASS)).c_str(), _TRUNCATE);
			FwallPort = GetDecimalText(hDlg, FIRE_PORT);
			FwallDelimiter = u8(GetText(hDlg, FIRE_DELIMIT))[0];
			FwallDefault = (int)SendDlgItemMessageW(hDlg, FIRE_USEIT, BM_GETCHECK, 0, 0);
			PasvDefault = (int)SendDlgItemMessageW(hDlg, FIRE_PASV, BM_GETCHECK, 0, 0);
			FwallResolve = (int)SendDlgItemMessageW(hDlg, FIRE_RESOLV, BM_GETCHECK, 0, 0);
			FwallLower = (int)SendDlgItemMessageW(hDlg, FIRE_LOWER, BM_GETCHECK, 0, 0);
			FwallSecurity = (int)SendDlgItemMessageW(hDlg, FIRE_SECURITY, CB_GETCURSEL, 0, 0);
			FwallNoSaveUser = (int)SendDlgItemMessageW(hDlg, FIRE_SHARED, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		}
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000049);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case FIRE_TYPE: {
			auto Num = (int)SendDlgItemMessageW(hDlg, FIRE_TYPE, CB_GETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hDlg, FIRE_USER), HideTbl[Num][0]);
			EnableWindow(GetDlgItem(hDlg, FIRE_PASS), HideTbl[Num][1]);
			EnableWindow(GetDlgItem(hDlg, FIRE_SECURITY), HideTbl[Num][2]);
			EnableWindow(GetDlgItem(hDlg, FIRE_RESOLV), HideTbl[Num][3]);
			EnableWindow(GetDlgItem(hDlg, FIRE_LOWER), HideTbl[Num][4]);
			EnableWindow(GetDlgItem(hDlg, FIRE_DELIMIT), HideTbl[Num][5]);
			break;
		}
		}
	}
};

struct Tool {
	static constexpr WORD dialogId = opt_tool_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, TOOL_EDITOR1, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessageW(hDlg, TOOL_EDITOR2, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessageW(hDlg, TOOL_EDITOR3, EM_LIMITTEXT, FMAX_PATH, 0);
		SetText(hDlg, TOOL_EDITOR1, u8(ViewerName[0]));
		SetText(hDlg, TOOL_EDITOR2, u8(ViewerName[1]));
		SetText(hDlg, TOOL_EDITOR3, u8(ViewerName[2]));
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			strncpy_s(ViewerName[0], FMAX_PATH + 1, u8(GetText(hDlg, TOOL_EDITOR1)).c_str(), _TRUNCATE);
			strncpy_s(ViewerName[1], FMAX_PATH + 1, u8(GetText(hDlg, TOOL_EDITOR2)).c_str(), _TRUNCATE);
			strncpy_s(ViewerName[2], FMAX_PATH + 1, u8(GetText(hDlg, TOOL_EDITOR3)).c_str(), _TRUNCATE);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000050);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case TOOL_EDITOR1_BR:
		case TOOL_EDITOR2_BR:
		case TOOL_EDITOR3_BR:
			if (auto const path = SelectFile(true, hDlg, IDS_SELECT_VIEWER, L"", nullptr, { FileType::Executable, FileType::All }); !std::empty(path)) {
				switch (id) {
				case TOOL_EDITOR1_BR:
					SetText(hDlg, TOOL_EDITOR1, path);
					break;
				case TOOL_EDITOR2_BR:
					SetText(hDlg, TOOL_EDITOR2, path);
					break;
				case TOOL_EDITOR3_BR:
					SetText(hDlg, TOOL_EDITOR3, path);
					break;
				}
			}
			break;
		}
	}
};

struct Other {
	static constexpr WORD dialogId = opt_misc_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessageW(hDlg, MISC_WINPOS, BM_SETCHECK, SaveWinPos, 0);
		SendDlgItemMessageW(hDlg, MISC_DEBUG, BM_SETCHECK, DebugConsole, 0);
		SendDlgItemMessageW(hDlg, MISC_REGTYPE, BM_SETCHECK, RegType, 0);
		if (AskForceIni() == YES)
			EnableWindow(GetDlgItem(hDlg, MISC_REGTYPE), FALSE);
		SendDlgItemMessageW(hDlg, MISC_ENCRYPT_SETTINGS, BM_SETCHECK, EncryptAllSettings, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SaveWinPos = (int)SendDlgItemMessageW(hDlg, MISC_WINPOS, BM_GETCHECK, 0, 0);
			DebugConsole = (int)SendDlgItemMessageW(hDlg, MISC_DEBUG, BM_GETCHECK, 0, 0);
			if (AskForceIni() == NO)
				RegType = (int)SendDlgItemMessageW(hDlg, MISC_REGTYPE, BM_GETCHECK, 0, 0);
			EncryptAllSettings = (int)SendDlgItemMessageW(hDlg, MISC_ENCRYPT_SETTINGS, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000052);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case IDC_OPENSOUNDS:
		{
			// Executing Control Panel Items <https://docs.microsoft.com/en-us/windows/win32/shell/executing-control-panel-items>
			int index = 2;
#if _WIN32_WINNT < _WIN32_WINNT_VISTA
			if (!IsWindowsVistaOrGreater())
				index = 1;
#endif
			WinExec(strprintf("%s mmsys.cpl,,%d", (systemDirectory() / L"control.exe").string().c_str(), index).c_str(), SW_NORMAL);
			break;
		}
		}
	}
};

// オプションのプロパティシート
void SetOption() {
	PropSheet<User, Transfer1, Transfer2, Transfer3, Transfer4, Mirroring, Operation, View1, View2, Connecting, Firewall, Tool, Other>(GetMainHwnd(), GetFtpInst(), IDS_OPTION, PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP);
}


/*----- ファイル名と属性をリストビューに追加 ----------------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		char *Fname : ファイル名
*		char *Attr : 属性
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void AddFnameAttrToListView(HWND hDlg, char* Fname, char* Attr) {
	char Buf[DEFATTRLIST_LEN + 1];
	GetFnameAttrFromListView(hDlg, Buf);
	if (StrMultiLen(Buf) + strlen(Fname) + strlen(Attr) + 2 <= DEFATTRLIST_LEN) {
		auto wFname = u8(Fname);
		LVITEMW item = { .mask = LVIF_TEXT, .iItem = std::numeric_limits<int>::max(), .pszText = data(wFname) };
		auto index = (int)SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_INSERTITEMW, 0, (LPARAM)&item);
		auto wAttr = u8(Attr);
		item = { .mask = LVIF_TEXT, .iItem = index, .iSubItem = 1, .pszText = data(wAttr) };
		SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_SETITEMW, 0, (LPARAM)&item);
	} else
		MessageBeep(-1);
}


/*----- リストビューの内容をマルチ文字列にする --------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int CtrlList : リストボックスのID
*		char *Buf : 文字列をセットするバッファ
*		int BufSize : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

static void GetFnameAttrFromListView(HWND hDlg, char* Buf) {
	std::wstring lines;
	wchar_t buffer[260 + 1];
	LVITEMW item;
	for (int i = 0, count = (int)SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_GETITEMCOUNT, 0, 0); i < count; i++) {
		item = { .mask = LVIF_TEXT, .iItem = i, .iSubItem = 0, .pszText = buffer, .cchTextMax = size_as<int>(buffer) };
		SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_GETITEMW, 0, (LPARAM)&item);
		lines += buffer;
		lines += L'\0';
		item = { .mask = LVIF_TEXT, .iItem = i, .iSubItem = 1, .pszText = buffer, .cchTextMax = size_as<int>(buffer) };
		SendDlgItemMessageW(hDlg, TRMODE3_LIST, LVM_GETITEMW, 0, (LPARAM)&item);
		lines += buffer;
		lines += L'\0';
	}
	lines += L'\0';
	auto u8lines = u8(lines);
	std::copy(begin(u8lines), end(u8lines), Buf);
}


// ソート設定
int SortSetting() {
	struct Data {
		using result_t = int;
		using LsortOrdButton = RadioButton<SORT_LFILE_NAME, SORT_LFILE_EXT, SORT_LFILE_SIZE, SORT_LFILE_DATE>;
		using LDirsortOrdButton = RadioButton<SORT_LDIR_NAME, SORT_LDIR_DATE>;
		using RsortOrdButton = RadioButton<SORT_RFILE_NAME, SORT_RFILE_EXT, SORT_RFILE_SIZE, SORT_RFILE_DATE>;
		using RDirsortOrdButton = RadioButton<SORT_RDIR_NAME, SORT_RDIR_DATE>;
		INT_PTR OnInit(HWND hDlg) {
			LsortOrdButton::Set(hDlg, AskSortType(ITEM_LFILE) & SORT_MASK_ORD);
			SendDlgItemMessageW(hDlg, SORT_LFILE_REV, BM_SETCHECK, (AskSortType(ITEM_LFILE) & SORT_GET_ORD) != SORT_ASCENT, 0);
			LDirsortOrdButton::Set(hDlg, AskSortType(ITEM_LDIR) & SORT_MASK_ORD);
			SendDlgItemMessageW(hDlg, SORT_LDIR_REV, BM_SETCHECK, (AskSortType(ITEM_LDIR) & SORT_GET_ORD) != SORT_ASCENT, 0);
			RsortOrdButton::Set(hDlg, AskSortType(ITEM_RFILE) & SORT_MASK_ORD);
			SendDlgItemMessageW(hDlg, SORT_RFILE_REV, BM_SETCHECK, (AskSortType(ITEM_RFILE) & SORT_GET_ORD) != SORT_ASCENT, 0);
			RDirsortOrdButton::Set(hDlg, AskSortType(ITEM_RDIR) & SORT_MASK_ORD);
			SendDlgItemMessageW(hDlg, SORT_RDIR_REV, BM_SETCHECK, (AskSortType(ITEM_RDIR) & SORT_GET_ORD) != SORT_ASCENT, 0);
			SendDlgItemMessageW(hDlg, SORT_SAVEHOST, BM_SETCHECK, AskSaveSortToHost(), 0);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK: {
				auto LFsort = LsortOrdButton::Get(hDlg);
				if (SendDlgItemMessageW(hDlg, SORT_LFILE_REV, BM_GETCHECK, 0, 0) == 1)
					LFsort |= SORT_DESCENT;
				auto LDsort = LDirsortOrdButton::Get(hDlg);
				if (SendDlgItemMessageW(hDlg, SORT_LDIR_REV, BM_GETCHECK, 0, 0) == 1)
					LDsort |= SORT_DESCENT;
				auto RFsort = RsortOrdButton::Get(hDlg);
				if (SendDlgItemMessageW(hDlg, SORT_RFILE_REV, BM_GETCHECK, 0, 0) == 1)
					RFsort |= SORT_DESCENT;
				auto RDsort = RDirsortOrdButton::Get(hDlg);
				if (SendDlgItemMessageW(hDlg, SORT_RDIR_REV, BM_GETCHECK, 0, 0) == 1)
					RDsort |= SORT_DESCENT;
				SetSortTypeImm(LFsort, LDsort, RFsort, RDsort);
				SetSaveSortToHost((int)SendDlgItemMessageW(hDlg, SORT_SAVEHOST, BM_GETCHECK, 0, 0));
				EndDialog(hDlg, YES);
				break;
			}
			case IDCANCEL:
				EndDialog(hDlg, NO);
				break;
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000001);
				break;
			}
		}
	};
	return Dialog(GetFtpInst(), sort_dlg, GetMainHwnd(), Data{});
}


// ダイアログのコントロールから１０進数を取得
int GetDecimalText(HWND hDlg, int Ctrl) {
	auto const text = GetText(hDlg, Ctrl);
	return !empty(text) && std::iswdigit(text[0]) ? stoi(text) : 0;
}


/*----- ダイアログのコントロールに１０進数をセット ----------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		int Ctrl : コントロールのID
*		int Num : 数値
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void SetDecimalText(HWND hDlg, int Ctrl, int Num)
void SetDecimalText(HWND hDlg, int Ctrl, int Num)
{
	char Tmp[40];

	sprintf(Tmp, "%d", Num);
	SetText(hDlg, Ctrl, u8(Tmp));
	return;
}


/*----- 設定値の範囲チェック --------------------------------------------------
*
*	Parameter
*		int *Cur : 設定値
*		int Max : 最大値
*		int Min : 最小値
*
*	Return Value
*		なし
*
*	Parameter change
*		int *Cur : 設定値
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void CheckRange2(int *Cur, int Max, int Min)
void CheckRange2(int *Cur, int Max, int Min)
{
	if(*Cur < Min)
		*Cur = Min;
	if(*Cur > Max)
		*Cur = Max;
	return;
}


/*----- 文字列をリストボックスに追加 ------------------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		char *Str : 文字列
*		int CtrlList : リストボックスのID
*		int BufSize : バッファサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
void AddTextToListBox(HWND hDlg, char* Str, int CtrlList, int BufSize) {
	if (int Len = (int)strlen(Str); Len > 0) {
		Len += 1 + size_as<int>(u8(GetMultiTextFromList(hDlg, CtrlList)));
		if (Len > (BufSize - 1))
			MessageBeep(-1);
		else
			SendDlgItemMessageW(hDlg, CtrlList, LB_ADDSTRING, 0, (LPARAM)u8(Str).c_str());
	}
}


/*----- マルチ文字列をリストボックスにセット ----------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int CtrlList : リストボックスのID
*		char *Text : 文字列
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text)
void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text)
{
	char *Pos;

	Pos = Text;
	while(*Pos != NUL)
	{
		SendDlgItemMessageW(hDlg, CtrlList, LB_ADDSTRING, 0, (LPARAM)u8(Pos).c_str());
		Pos += strlen(Pos) + 1;
	}
	return;
}


/*----- リストボックスの内容をマルチ文字列にする ------------------------------
*
*	Parameter
*		HWND hDlg : ダイアログボックスのウインドウハンドル
*		int CtrlList : リストボックスのID
*		char *Buf : 文字列をセットするバッファ
*		int BufSize : バッファのサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
void GetMultiTextFromList(HWND hDlg, int CtrlList, char* Buf, int BufSize) {
	auto u8lines = u8(GetMultiTextFromList(hDlg, CtrlList));
	memset(Buf, 0, BufSize);
	memcpy(Buf, u8lines.c_str(), std::min(size(u8lines), (size_t)BufSize));
}
