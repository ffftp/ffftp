/*=============================================================================
*
*									ツール
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


// ワンタイムパスワード計算
void OtpCalcTool() {
	struct Data {
		using result_t = int;
		using AlgoButton = RadioButton<OTPCALC_MD4, OTPCALC_MD5, OTPCALC_SHA1>;
		INT_PTR OnInit(HWND hDlg) {
			SendDlgItemMessageW(hDlg, OTPCALC_KEY, EM_LIMITTEXT, 40, 0);
			SendDlgItemMessageW(hDlg, OTPCALC_PASS, EM_LIMITTEXT, PASSWORD_LEN, 0);
			AlgoButton::Set(hDlg, MD4);
			return(TRUE);
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK: {
				static boost::wregex re{ LR"(^ *(\d+)(?=[^ ]* +([^ ]+)))" };
				auto const key = GetText(hDlg, OTPCALC_KEY);
				if (boost::wsmatch m; boost::regex_search(key, m, re)) {
					if (m[2].matched) {
						auto seq = std::stoi(m[1]);
						auto seed = u8(m[2].str());
						auto pass = u8(GetText(hDlg, OTPCALC_PASS));
						char Tmp[41];
						Make6WordPass(seq, seed, pass, AlgoButton::Get(hDlg), Tmp);
						SetText(hDlg, OTPCALC_RES, u8(Tmp));
					} else
						SetText(hDlg, OTPCALC_RES, GetString(IDS_MSGJPN251));
				} else
					SetText(hDlg, OTPCALC_RES, GetString(IDS_MSGJPN253));
				break;
			}
			case IDCANCEL:
				EndDialog(hDlg, NO);
				break;
			case IDHELP:
				ShowHelp(IDH_HELP_TOPIC_0000037);
				break;
			}
		}
	};
	Dialog(GetFtpInst(), otp_calc_dlg, GetMainHwnd(), Data{});
}


void TurnStatefulFTPFilter() {
	if (auto ID = Message(IDS_MANAGE_STATEFUL_FTP, MB_YESNOCANCEL); ID == IDYES || ID == IDNO)
		if (PtrToInt(ShellExecuteW(NULL, L"runas", L"netsh", ID == IDYES ? L"advfirewall set global statefulftp enable" : L"advfirewall set global statefulftp disable", systemDirectory().c_str(), SW_SHOW)) <= 32)
			Message(IDS_FAIL_TO_MANAGE_STATEFUL_FTP, MB_OK | MB_ICONERROR);
}
