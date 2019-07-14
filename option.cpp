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


/*===== プロトタイプ =====*/

static void AddFnameAttrToListView(HWND hDlg, char *Fname, char *Attr);
static void GetFnameAttrFromListView(HWND hDlg, char *Buf);
static int SelectListFont(HWND hWnd, LOGFONT *lFont);
// hostman.cで使用
//static int GetDecimalText(HWND hDlg, int Ctrl);
//static void SetDecimalText(HWND hDlg, int Ctrl, int Num);
//static void CheckRange2(int *Cur, int Max, int Min);
//static void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize);
//static void SetMultiTextToList(HWND hDlg, int CtrlList, char *Text);
//static void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize);
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
extern int DebugConsole;
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
extern SOUNDFILE Sound[SOUND_TYPES];
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
extern LOGFONT ListLogFont;
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
				SendDlgItemMessageW(hDlg, DEFATTR_ATTR, WM_SETTEXT, 0, (LPARAM)newattr->c_str());
			break;
		}
	}
};


struct User {
	static constexpr WORD dialogId = opt_user_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessage(hDlg, USER_ADRS, EM_LIMITTEXT, PASSWORD_LEN, 0);
		SendDlgItemMessage(hDlg, USER_ADRS, WM_SETTEXT, 0, (LPARAM)UserMailAdrs);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SendDlgItemMessage(hDlg, USER_ADRS, WM_GETTEXT, USER_MAIL_LEN + 1, (LPARAM)UserMailAdrs);
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
		SendDlgItemMessage(hDlg, TRMODE_TIME, BM_SETCHECK, SaveTimeStamp, 0);
		SendDlgItemMessage(hDlg, TRMODE_EOF, BM_SETCHECK, RmEOF, 0);
		SendDlgItemMessage(hDlg, TRMODE_SEMICOLON, BM_SETCHECK, VaxSemicolon, 0);
		SendDlgItemMessage(hDlg, TRMODE_MAKEDIR, BM_SETCHECK, MakeAllDir, 0);
		SendDlgItemMessage(hDlg, TRMODE_LISTERROR, BM_SETCHECK, AbortOnListError, 0);
		SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(TRMODE_EXT_LIST, 0), 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SetTransferTypeImm(ModeButton::Get(hDlg));
			SaveTransferType();
			GetMultiTextFromList(hDlg, TRMODE_EXT_LIST, AsciiExt, ASCII_EXT_LEN + 1);
			SaveTimeStamp = (int)SendDlgItemMessage(hDlg, TRMODE_TIME, BM_GETCHECK, 0, 0);
			RmEOF = (int)SendDlgItemMessage(hDlg, TRMODE_EOF, BM_GETCHECK, 0, 0);
			VaxSemicolon = (int)SendDlgItemMessage(hDlg, TRMODE_SEMICOLON, BM_GETCHECK, 0, 0);
			MakeAllDir = (int)SendDlgItemMessage(hDlg, TRMODE_MAKEDIR, BM_GETCHECK, 0, 0);
			AbortOnListError = (int)SendDlgItemMessage(hDlg, TRMODE_LISTERROR, BM_GETCHECK, 0, 0);
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
			if (auto Num = (int)SendDlgItemMessage(hDlg, TRMODE_EXT_LIST, LB_GETCURSEL, 0, 0); Num != LB_ERR)
				SendDlgItemMessage(hDlg, TRMODE_EXT_LIST, LB_DELETESTRING, Num, 0);
			break;
		}
	}
};

struct Transfer2 {
	static constexpr WORD dialogId = opt_trmode2_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	using CnvButton = RadioButton<TRMODE2_NOCNV, TRMODE2_LOWER, TRMODE2_UPPER>;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessage(hDlg, TRMODE2_LOCAL, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, TRMODE2_LOCAL, WM_SETTEXT, 0, (LPARAM)DefaultLocalPath);
		CnvButton::Set(hDlg, FnameCnv);
		SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT, EM_LIMITTEXT, (WPARAM)5, 0);
		char Tmp[FMAX_PATH + 1];
		sprintf(Tmp, "%d", TimeOut);
		SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT, WM_SETTEXT, 0, (LPARAM)Tmp);
		SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT_SPN, UDM_SETRANGE, 0, MAKELONG(300, 0));
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			SendDlgItemMessage(hDlg, TRMODE2_LOCAL, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)DefaultLocalPath);
			FnameCnv = CnvButton::Get(hDlg);
			char Tmp[FMAX_PATH + 1];
			SendDlgItemMessage(hDlg, TRMODE2_TIMEOUT, WM_GETTEXT, 5 + 1, (LPARAM)Tmp);
			TimeOut = atoi(Tmp);
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
				SendDlgItemMessage(hDlg, TRMODE2_LOCAL, WM_SETTEXT, 0, (LPARAM)Tmp);
			break;
		}
	}
};

struct Transfer3 {
	static constexpr WORD dialogId = opt_trmode3_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		auto Tmp = (long)SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
		Tmp |= LVS_EX_FULLROWSELECT;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)Tmp);

		RECT Rect;
		GetClientRect(GetDlgItem(hDlg, TRMODE3_LIST), &Rect);

		LV_COLUMN LvCol;
		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = (Rect.right / 3) * 2;
		LvCol.pszText = MSGJPN200;
		LvCol.iSubItem = 0;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);

		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		LvCol.cx = Rect.right - LvCol.cx;
		LvCol.pszText = MSGJPN201;
		LvCol.iSubItem = 1;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

		auto Fname = DefAttrList;
		while (*Fname != NUL) {
			auto Attr = strchr(Fname, NUL) + 1;
			if (*Attr != NUL)
				AddFnameAttrToListView(hDlg, Fname, Attr);
			Fname = strchr(Attr, NUL) + 1;
		}

		SendDlgItemMessage(hDlg, TRMODE3_FOLDER, BM_SETCHECK, FolderAttr, 0);
		if (FolderAttr == NO)
			EnableWindow(GetDlgItem(hDlg, TRMODE3_FOLDER_ATTR), FALSE);

		SendDlgItemMessage(hDlg, TRMODE3_FOLDER_ATTR, EM_LIMITTEXT, (WPARAM)5, 0);
		char TmpStr[10];
		sprintf(TmpStr, "%03d", FolderAttrNum);
		SendDlgItemMessage(hDlg, TRMODE3_FOLDER_ATTR, WM_SETTEXT, 0, (LPARAM)TmpStr);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			GetFnameAttrFromListView(hDlg, DefAttrList);
			char TmpStr[10];
			SendDlgItemMessage(hDlg, TRMODE3_FOLDER_ATTR, WM_GETTEXT, 5 + 1, (LPARAM)TmpStr);
			FolderAttrNum = atoi(TmpStr);
			FolderAttr = (int)SendDlgItemMessage(hDlg, TRMODE3_FOLDER, BM_GETCHECK, 0, 0);
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
			if (auto Tmp = (long)SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_ALL | LVNI_SELECTED, 0)); Tmp != -1)
				SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_DELETEITEM, Tmp, 0);
			break;
		case TRMODE3_FOLDER:
			if (SendDlgItemMessage(hDlg, TRMODE3_FOLDER, BM_GETCHECK, 0, 0) == 1)
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
			SendDlgItemMessage(hDlg, TRMODE4_MARK_INTERNET, BM_SETCHECK, MarkAsInternet, 0);
		else {
			SendDlgItemMessage(hDlg, TRMODE4_MARK_INTERNET, BM_SETCHECK, BST_UNCHECKED, 0);
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
				MarkAsInternet = (int)SendDlgItemMessage(hDlg, TRMODE4_MARK_INTERNET, BM_GETCHECK, 0, 0);
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
		SendDlgItemMessage(hDlg, MIRROR_LOW, BM_SETCHECK, MirrorFnameCnv, 0);
		SendDlgItemMessage(hDlg, MIRROR_UPDEL_NOTIFY, BM_SETCHECK, MirUpDelNotify, 0);
		SendDlgItemMessage(hDlg, MIRROR_DOWNDEL_NOTIFY, BM_SETCHECK, MirDownDelNotify, 0);
		SendDlgItemMessage(hDlg, MIRROR_NO_TRANSFER, BM_SETCHECK, MirrorNoTransferContents, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			GetMultiTextFromList(hDlg, MIRROR_NOTRN_LIST, MirrorNoTrn, MIRROR_LEN + 1);
			GetMultiTextFromList(hDlg, MIRROR_NODEL_LIST, MirrorNoDel, MIRROR_LEN + 1);
			MirrorFnameCnv = (int)SendDlgItemMessage(hDlg, MIRROR_LOW, BM_GETCHECK, 0, 0);
			MirUpDelNotify = (int)SendDlgItemMessage(hDlg, MIRROR_UPDEL_NOTIFY, BM_GETCHECK, 0, 0);
			MirDownDelNotify = (int)SendDlgItemMessage(hDlg, MIRROR_DOWNDEL_NOTIFY, BM_GETCHECK, 0, 0);
			MirrorNoTransferContents = (int)SendDlgItemMessage(hDlg, MIRROR_NO_TRANSFER, BM_GETCHECK, 0, 0);
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
			if (auto Num = (int)SendDlgItemMessage(hDlg, MIRROR_NOTRN_LIST, LB_GETCURSEL, 0, 0); Num != LB_ERR)
				SendDlgItemMessage(hDlg, MIRROR_NOTRN_LIST, LB_DELETESTRING, Num, 0);
			break;
		case MIRROR_NODEL_DEL:
			if (auto Num = (int)SendDlgItemMessage(hDlg, MIRROR_NODEL_LIST, LB_GETCURSEL, 0, 0); Num != LB_ERR)
				SendDlgItemMessage(hDlg, MIRROR_NODEL_LIST, LB_DELETESTRING, Num, 0);
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
	static inline LOGFONT TmpFont;
	static INT_PTR OnInit(HWND hDlg) {
		memcpy(&TmpFont, &ListLogFont, sizeof(LOGFONT));
		if (ListFont != NULL)
			SendDlgItemMessage(hDlg, DISP_FONT, WM_SETTEXT, 0, (LPARAM)TmpFont.lfFaceName);
		SendDlgItemMessage(hDlg, DISP_HIDE, BM_SETCHECK, DispIgnoreHide, 0);
		SendDlgItemMessage(hDlg, DISP_DRIVE, BM_SETCHECK, DispDrives, 0);
		SendDlgItemMessage(hDlg, DISP_ICON, BM_SETCHECK, DispFileIcon, 0);
		SendDlgItemMessage(hDlg, DISP_SECOND, BM_SETCHECK, DispTimeSeconds, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			DispIgnoreHide = (int)SendDlgItemMessage(hDlg, DISP_HIDE, BM_GETCHECK, 0, 0);
			DispDrives = (int)SendDlgItemMessage(hDlg, DISP_DRIVE, BM_GETCHECK, 0, 0);
			DispFileIcon = (int)SendDlgItemMessage(hDlg, DISP_ICON, BM_GETCHECK, 0, 0);
			DispTimeSeconds = (int)SendDlgItemMessage(hDlg, DISP_SECOND, BM_GETCHECK, 0, 0);
			if (strlen(TmpFont.lfFaceName) > 0) {
				memcpy(&ListLogFont, &TmpFont, sizeof(LOGFONT));
				ListFont = CreateFontIndirect(&ListLogFont);
			}
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000047);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		switch (id) {
		case DISP_FONT_BR:
			if (SelectListFont(hDlg, &TmpFont) == YES)
				SendDlgItemMessage(hDlg, DISP_FONT, WM_SETTEXT, 0, (LPARAM)TmpFont.lfFaceName);
			break;
		}
	}
};

struct View2 {
	static constexpr WORD dialogId = opt_disp2_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessage(hDlg, DISP2_PERMIT_NUM, BM_SETCHECK, DispPermissionsNumber, 0);
		SendDlgItemMessage(hDlg, DISP2_AUTO_REFRESH, BM_SETCHECK, AutoRefreshFileList, 0);
		SendDlgItemMessage(hDlg, DISP2_REMOVE_OLD_LOG, BM_SETCHECK, RemoveOldLog, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			DispPermissionsNumber = (int)SendDlgItemMessage(hDlg, DISP2_PERMIT_NUM, BM_GETCHECK, 0, 0);
			AutoRefreshFileList = (int)SendDlgItemMessage(hDlg, DISP2_AUTO_REFRESH, BM_GETCHECK, 0, 0);
			RemoveOldLog = (int)SendDlgItemMessage(hDlg, DISP2_REMOVE_OLD_LOG, BM_GETCHECK, 0, 0);
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
		SendDlgItemMessage(hDlg, CONNECT_CONNECT, BM_SETCHECK, ConnectOnStart, 0);
		SendDlgItemMessage(hDlg, CONNECT_OLDDLG, BM_SETCHECK, ConnectAndSet, 0);
		SendDlgItemMessage(hDlg, CONNECT_RASCLOSE, BM_SETCHECK, RasClose, 0);
		if (NoRasControl != NO)
			EnableWindow(GetDlgItem(hDlg, CONNECT_RASCLOSE), FALSE);
		SendDlgItemMessage(hDlg, CONNECT_CLOSE_NOTIFY, BM_SETCHECK, RasCloseNotify, 0);
		if (RasClose == NO || NoRasControl != NO)
			EnableWindow(GetDlgItem(hDlg, CONNECT_CLOSE_NOTIFY), FALSE);
		SendDlgItemMessage(hDlg, CONNECT_HIST, EM_LIMITTEXT, (WPARAM)2, 0);
		SetDecimalText(hDlg, CONNECT_HIST, FileHist);
		SendDlgItemMessage(hDlg, CONNECT_HIST_SPN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(HISTORY_MAX, 0));
		SendDlgItemMessage(hDlg, CONNECT_QUICK_ANONY, BM_SETCHECK, QuickAnonymous, 0);
		SendDlgItemMessage(hDlg, CONNECT_HIST_PASS, BM_SETCHECK, PassToHist, 0);
		SendDlgItemMessage(hDlg, CONNECT_SENDQUIT, BM_SETCHECK, SendQuit, 0);
		SendDlgItemMessage(hDlg, CONNECT_NORAS, BM_SETCHECK, NoRasControl, 0);
		SendDlgItemMessage(hDlg, CONNECT_UPNP, BM_SETCHECK, UPnPEnabled, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			ConnectOnStart = (int)SendDlgItemMessage(hDlg, MISC_CONNECT, BM_GETCHECK, 0, 0);
			ConnectAndSet = (int)SendDlgItemMessage(hDlg, MISC_OLDDLG, BM_GETCHECK, 0, 0);
			RasClose = (int)SendDlgItemMessage(hDlg, CONNECT_RASCLOSE, BM_GETCHECK, 0, 0);
			RasCloseNotify = (int)SendDlgItemMessage(hDlg, CONNECT_CLOSE_NOTIFY, BM_GETCHECK, 0, 0);
			FileHist = GetDecimalText(hDlg, CONNECT_HIST);
			CheckRange2(&FileHist, HISTORY_MAX, 0);
			QuickAnonymous = (int)SendDlgItemMessage(hDlg, CONNECT_QUICK_ANONY, BM_GETCHECK, 0, 0);
			PassToHist = (int)SendDlgItemMessage(hDlg, CONNECT_HIST_PASS, BM_GETCHECK, 0, 0);
			SendQuit = (int)SendDlgItemMessage(hDlg, CONNECT_SENDQUIT, BM_GETCHECK, 0, 0);
			NoRasControl = (int)SendDlgItemMessage(hDlg, CONNECT_NORAS, BM_GETCHECK, 0, 0);
			UPnPEnabled = (int)SendDlgItemMessage(hDlg, CONNECT_UPNP, BM_GETCHECK, 0, 0);
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
			if (SendDlgItemMessage(hDlg, CONNECT_RASCLOSE, BM_GETCHECK, 0, 0) == 1)
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
	static constexpr INTCONVTBL TypeTbl[] = {
		{ 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 8 }, 
		{ 4, 3 }, { 5, 4 }, { 6, 5 }, { 7, 6 }, 
		{ 8, 7 }, { 9, 9 }
	};
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
		auto Type = ConvertNum(FwallType, 1, TypeTbl, sizeof(TypeTbl) / sizeof(INTCONVTBL));
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN204);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN205);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN206);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN207);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN208);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN209);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN210);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN211);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_ADDSTRING, 0, (LPARAM)MSGJPN294);
		SendDlgItemMessage(hDlg, FIRE_TYPE, CB_SETCURSEL, Type - 1, 0);

		SendDlgItemMessage(hDlg, FIRE_HOST, EM_LIMITTEXT, HOST_ADRS_LEN, 0);
		SendDlgItemMessage(hDlg, FIRE_USER, EM_LIMITTEXT, USER_NAME_LEN, 0);
		SendDlgItemMessage(hDlg, FIRE_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
		SendDlgItemMessage(hDlg, FIRE_PORT, EM_LIMITTEXT, 5, 0);
		SendDlgItemMessage(hDlg, FIRE_DELIMIT, EM_LIMITTEXT, 1, 0);

		SendDlgItemMessage(hDlg, FIRE_HOST, WM_SETTEXT, 0, (LPARAM)FwallHost);
		SendDlgItemMessage(hDlg, FIRE_USER, WM_SETTEXT, 0, (LPARAM)FwallUser);
		SendDlgItemMessage(hDlg, FIRE_PASS, WM_SETTEXT, 0, (LPARAM)FwallPass);
		char Tmp[10];
		sprintf(Tmp, "%d", FwallPort);
		SendDlgItemMessage(hDlg, FIRE_PORT, WM_SETTEXT, 0, (LPARAM)Tmp);
		sprintf(Tmp, "%c", FwallDelimiter);
		SendDlgItemMessage(hDlg, FIRE_DELIMIT, WM_SETTEXT, 0, (LPARAM)Tmp);

		SendDlgItemMessage(hDlg, FIRE_USEIT, BM_SETCHECK, FwallDefault, 0);
		SendDlgItemMessage(hDlg, FIRE_PASV, BM_SETCHECK, PasvDefault, 0);
		SendDlgItemMessage(hDlg, FIRE_RESOLV, BM_SETCHECK, FwallResolve, 0);
		SendDlgItemMessage(hDlg, FIRE_LOWER, BM_SETCHECK, FwallLower, 0);

		SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN212);
		SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN213);
		SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN214);
		SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN215);
		SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_ADDSTRING, 0, (LPARAM)MSGJPN216);
		SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_SETCURSEL, FwallSecurity, 0);

		SendDlgItemMessage(hDlg, FIRE_SHARED, BM_SETCHECK, FwallNoSaveUser, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY: {
			auto Type = (int)SendDlgItemMessage(hDlg, FIRE_TYPE, CB_GETCURSEL, 0, 0) + 1;
			FwallType = ConvertNum(Type, 0, TypeTbl, sizeof(TypeTbl) / sizeof(INTCONVTBL));
			SendDlgItemMessage(hDlg, FIRE_HOST, WM_GETTEXT, HOST_ADRS_LEN + 1, (LPARAM)FwallHost);
			SendDlgItemMessage(hDlg, FIRE_USER, WM_GETTEXT, USER_NAME_LEN + 1, (LPARAM)FwallUser);
			SendDlgItemMessage(hDlg, FIRE_PASS, WM_GETTEXT, PASSWORD_LEN, (LPARAM)FwallPass);
			char Tmp[10];
			SendDlgItemMessage(hDlg, FIRE_PORT, WM_GETTEXT, 5 + 1, (LPARAM)Tmp);
			FwallPort = atoi(Tmp);
			SendDlgItemMessage(hDlg, FIRE_DELIMIT, WM_GETTEXT, 5, (LPARAM)Tmp);
			FwallDelimiter = Tmp[0];
			FwallDefault = (int)SendDlgItemMessage(hDlg, FIRE_USEIT, BM_GETCHECK, 0, 0);
			PasvDefault = (int)SendDlgItemMessage(hDlg, FIRE_PASV, BM_GETCHECK, 0, 0);
			FwallResolve = (int)SendDlgItemMessage(hDlg, FIRE_RESOLV, BM_GETCHECK, 0, 0);
			FwallLower = (int)SendDlgItemMessage(hDlg, FIRE_LOWER, BM_GETCHECK, 0, 0);
			FwallSecurity = (int)SendDlgItemMessage(hDlg, FIRE_SECURITY, CB_GETCURSEL, 0, 0);
			FwallNoSaveUser = (int)SendDlgItemMessage(hDlg, FIRE_SHARED, BM_GETCHECK, 0, 0);
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
			auto Num = (int)SendDlgItemMessage(hDlg, FIRE_TYPE, CB_GETCURSEL, 0, 0);
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
		SendDlgItemMessage(hDlg, TOOL_EDITOR1, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, TOOL_EDITOR2, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, TOOL_EDITOR3, EM_LIMITTEXT, FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, TOOL_EDITOR1, WM_SETTEXT, 0, (LPARAM)ViewerName[0]);
		SendDlgItemMessage(hDlg, TOOL_EDITOR2, WM_SETTEXT, 0, (LPARAM)ViewerName[1]);
		SendDlgItemMessage(hDlg, TOOL_EDITOR3, WM_SETTEXT, 0, (LPARAM)ViewerName[2]);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SendDlgItemMessage(hDlg, TOOL_EDITOR1, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)ViewerName[0]);
			SendDlgItemMessage(hDlg, TOOL_EDITOR2, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)ViewerName[1]);
			SendDlgItemMessage(hDlg, TOOL_EDITOR3, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)ViewerName[2]);
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
					SendDlgItemMessageW(hDlg, TOOL_EDITOR1, WM_SETTEXT, 0, (LPARAM)path.c_str());
					break;
				case TOOL_EDITOR2_BR:
					SendDlgItemMessageW(hDlg, TOOL_EDITOR2, WM_SETTEXT, 0, (LPARAM)path.c_str());
					break;
				case TOOL_EDITOR3_BR:
					SendDlgItemMessageW(hDlg, TOOL_EDITOR3, WM_SETTEXT, 0, (LPARAM)path.c_str());
					break;
				}
			}
			break;
		}
	}
};

struct Sounds {
	static constexpr WORD dialogId = opt_sound_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessage(hDlg, SOUND_CONNECT, BM_SETCHECK, Sound[SND_CONNECT].On, 0);
		SendDlgItemMessage(hDlg, SOUND_TRANS, BM_SETCHECK, Sound[SND_TRANS].On, 0);
		SendDlgItemMessage(hDlg, SOUND_ERROR, BM_SETCHECK, Sound[SND_ERROR].On, 0);

		SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, EM_LIMITTEXT, (WPARAM)FMAX_PATH, 0);
		SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_SETTEXT, 0, (LPARAM)Sound[SND_CONNECT].Fname);
		SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_SETTEXT, 0, (LPARAM)Sound[SND_TRANS].Fname);
		SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_SETTEXT, 0, (LPARAM)Sound[SND_ERROR].Fname);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			Sound[SND_CONNECT].On = (int)SendDlgItemMessage(hDlg, SOUND_CONNECT, BM_GETCHECK, 0, 0);
			Sound[SND_TRANS].On = (int)SendDlgItemMessage(hDlg, SOUND_TRANS, BM_GETCHECK, 0, 0);
			Sound[SND_ERROR].On = (int)SendDlgItemMessage(hDlg, SOUND_ERROR, BM_GETCHECK, 0, 0);
			SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)Sound[SND_CONNECT].Fname);
			SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)Sound[SND_TRANS].Fname);
			SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)Sound[SND_ERROR].Fname);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000051);
			break;
		}
		return 0;
	}
	static void OnCommand(HWND hDlg, WORD id) {
		char Tmp[FMAX_PATH + 1];
		switch (id) {
		case SOUND_CONNECT_BR:
		case SOUND_TRANS_BR:
		case SOUND_ERROR_BR:
			if (auto const path = SelectFile(true, hDlg, IDS_SELECT_AUDIOFILE, L"", nullptr, { FileType::Audio, FileType::All }); !std::empty(path)) {
				switch (id) {
				case SOUND_CONNECT_BR:
					SendDlgItemMessageW(hDlg, SOUND_CONNECT_WAV, WM_SETTEXT, 0, (LPARAM)path.c_str());
					break;
				case SOUND_TRANS_BR:
					SendDlgItemMessageW(hDlg, SOUND_TRANS_WAV, WM_SETTEXT, 0, (LPARAM)path.c_str());
					break;
				case SOUND_ERROR_BR:
					SendDlgItemMessageW(hDlg, SOUND_ERROR_WAV, WM_SETTEXT, 0, (LPARAM)path.c_str());
					break;
				}
			}
			break;
		case SOUND_CONNECT_TEST:
			SendDlgItemMessage(hDlg, SOUND_CONNECT_WAV, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)Tmp);
			sndPlaySound(Tmp, SND_ASYNC | SND_NODEFAULT);
			break;
		case SOUND_TRANS_TEST:
			SendDlgItemMessage(hDlg, SOUND_TRANS_WAV, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)Tmp);
			sndPlaySound(Tmp, SND_ASYNC | SND_NODEFAULT);
			break;
		case SOUND_ERROR_TEST:
			SendDlgItemMessage(hDlg, SOUND_ERROR_WAV, WM_GETTEXT, FMAX_PATH + 1, (LPARAM)Tmp);
			sndPlaySound(Tmp, SND_ASYNC | SND_NODEFAULT);
			break;
		}
	}
};

struct Other {
	static constexpr WORD dialogId = opt_misc_dlg;
	static constexpr DWORD flag = PSP_HASHELP;
	static INT_PTR OnInit(HWND hDlg) {
		SendDlgItemMessage(hDlg, MISC_WINPOS, BM_SETCHECK, SaveWinPos, 0);
		SendDlgItemMessage(hDlg, MISC_DEBUG, BM_SETCHECK, DebugConsole, 0);
		SendDlgItemMessage(hDlg, MISC_REGTYPE, BM_SETCHECK, RegType, 0);
		if (AskForceIni() == YES)
			EnableWindow(GetDlgItem(hDlg, MISC_REGTYPE), FALSE);
		SendDlgItemMessage(hDlg, MISC_ENCRYPT_SETTINGS, BM_SETCHECK, EncryptAllSettings, 0);
		return TRUE;
	}
	static INT_PTR OnNotify(HWND hDlg, NMHDR* nmh) {
		switch (nmh->code) {
		case PSN_APPLY:
			SaveWinPos = (int)SendDlgItemMessage(hDlg, MISC_WINPOS, BM_GETCHECK, 0, 0);
			DebugConsole = (int)SendDlgItemMessage(hDlg, MISC_DEBUG, BM_GETCHECK, 0, 0);
			if (AskForceIni() == NO)
				RegType = (int)SendDlgItemMessage(hDlg, MISC_REGTYPE, BM_GETCHECK, 0, 0);
			EncryptAllSettings = (int)SendDlgItemMessage(hDlg, MISC_ENCRYPT_SETTINGS, BM_GETCHECK, 0, 0);
			return PSNRET_NOERROR;
		case PSN_HELP:
			ShowHelp(IDH_HELP_TOPIC_0000052);
			break;
		}
		return 0;
	}
};

// オプションのプロパティシート
void SetOption() {
	PropSheet<User, Transfer1, Transfer2, Transfer3, Transfer4, Mirroring, Operation, View1, View2, Connecting, Firewall, Tool, Sounds, Other>(GetMainHwnd(), GetFtpInst(), IDS_OPTION, PSH_NOAPPLYNOW);
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

static void AddFnameAttrToListView(HWND hDlg, char *Fname, char *Attr)
{
	int Num;
	LV_ITEM LvItem;
	char Buf[DEFATTRLIST_LEN+1];

	GetFnameAttrFromListView(hDlg, Buf);
	if(StrMultiLen(Buf) + strlen(Fname) + strlen(Attr) + 2 <= DEFATTRLIST_LEN)
	{
		Num = (int)SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEMCOUNT, 0, 0);

		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Num;
		LvItem.iSubItem = 0;
		LvItem.pszText = Fname;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_INSERTITEM, 0, (LPARAM)&LvItem);

		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = Num;
		LvItem.iSubItem = 1;
		LvItem.pszText = Attr;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_SETITEM, 0, (LPARAM)&LvItem);
	}
	else
		MessageBeep(-1);

	return;
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

static void GetFnameAttrFromListView(HWND hDlg, char *Buf)
{
	int Num;
	int i;
	LV_ITEM LvItem;

	Num = (int)SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEMCOUNT, 0, 0);
	for(i = 0; i < Num; i++)
	{
		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 0;
		LvItem.pszText = Buf;
		LvItem.cchTextMax = FMAX_PATH;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEM, 0, (LPARAM)&LvItem);
		Buf = strchr(Buf, NUL) + 1;

		LvItem.mask = LVIF_TEXT;
		LvItem.iItem = i;
		LvItem.iSubItem = 1;
		LvItem.pszText = Buf;
		LvItem.cchTextMax = FMAX_PATH;
		SendDlgItemMessage(hDlg, TRMODE3_LIST, LVM_GETITEM, 0, (LPARAM)&LvItem);
		Buf = strchr(Buf, NUL) + 1;
	}
	*Buf = NUL;

	return;
}


/*----- フォントを選ぶ --------------------------------------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		LOGFONT *lFont : フォント情報
*
*	Return Value
*		なし
*
*	Parameter change
*		HFONT *hFont : フォントのハンドル
*		LOGFONT *lFont : フォント情報
*----------------------------------------------------------------------------*/

static int SelectListFont(HWND hWnd, LOGFONT *lFont)
{
	static CHOOSEFONT cFont;
	int Sts;

	cFont.lStructSize = sizeof(CHOOSEFONT);
	cFont.hwndOwner = hWnd;
	cFont.hDC = 0;
	cFont.lpLogFont = lFont;
	cFont.Flags = CF_FORCEFONTEXIST | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
	cFont.nFontType = SCREEN_FONTTYPE;

	Sts = NO;
	if(ChooseFont(&cFont) == TRUE)
		Sts = YES;

	return(Sts);
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
				if (SendDlgItemMessage(hDlg, SORT_LFILE_REV, BM_GETCHECK, 0, 0) == 1)
					LFsort |= SORT_DESCENT;
				auto LDsort = LDirsortOrdButton::Get(hDlg);
				if (SendDlgItemMessage(hDlg, SORT_LDIR_REV, BM_GETCHECK, 0, 0) == 1)
					LDsort |= SORT_DESCENT;
				auto RFsort = RsortOrdButton::Get(hDlg);
				if (SendDlgItemMessage(hDlg, SORT_RFILE_REV, BM_GETCHECK, 0, 0) == 1)
					RFsort |= SORT_DESCENT;
				auto RDsort = RDirsortOrdButton::Get(hDlg);
				if (SendDlgItemMessage(hDlg, SORT_RDIR_REV, BM_GETCHECK, 0, 0) == 1)
					RDsort |= SORT_DESCENT;
				SetSortTypeImm(LFsort, LDsort, RFsort, RDsort);
				SetSaveSortToHost((int)SendDlgItemMessage(hDlg, SORT_SAVEHOST, BM_GETCHECK, 0, 0));
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


/*----- ダイアログのコントロールから１０進数を取得 ----------------------------
*
*	Parameter
*		HWND hDlg : ウインドウハンドル
*		int Ctrl : コントロールのID
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

// hostman.cで使用
//static int GetDecimalText(HWND hDlg, int Ctrl)
int GetDecimalText(HWND hDlg, int Ctrl)
{
	char Tmp[40];

	SendDlgItemMessage(hDlg, Ctrl, WM_GETTEXT, (WPARAM)39, (LPARAM)Tmp);
	return(atoi(Tmp));
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
	SendDlgItemMessage(hDlg, Ctrl, WM_SETTEXT, 0, (LPARAM)Tmp);
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

// hostman.cで使用
//static void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize)
void AddTextToListBox(HWND hDlg, char *Str, int CtrlList, int BufSize)
{
	char Tmp[FMAX_PATH+1];
	int Num;
	int i;
	int Len;

	Len = (int)strlen(Str);
	if(Len > 0)
	{
		Len++;
		Num = (int)SendDlgItemMessage(hDlg, CtrlList, LB_GETCOUNT, 0, 0);
		for(i = 0; i < Num; i++)
		{
			SendDlgItemMessage(hDlg, CtrlList, LB_GETTEXT, i, (LPARAM)Tmp);
			Len += (int)strlen(Tmp) + 1;
		}

		if(Len > (BufSize-1))
			MessageBeep(-1);
		else
			SendDlgItemMessage(hDlg, CtrlList, LB_ADDSTRING, 0, (LPARAM)Str);
	}
	return;
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
		SendDlgItemMessage(hDlg, CtrlList, LB_ADDSTRING, 0, (LPARAM)Pos);
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

// hostman.cで使用
//static void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize)
void GetMultiTextFromList(HWND hDlg, int CtrlList, char *Buf, int BufSize)
{
	char Tmp[FMAX_PATH+1];
	int Num;
	int i;

	memset(Buf, NUL, BufSize);
	Num = (int)SendDlgItemMessage(hDlg, CtrlList, LB_GETCOUNT, 0, 0);
	for(i = 0; i < Num; i++)
	{
		SendDlgItemMessage(hDlg, CtrlList, LB_GETTEXT, i, (LPARAM)Tmp);
		strcpy(Buf + StrMultiLen(Buf), Tmp);
	}
	return;
}


