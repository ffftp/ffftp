/*=============================================================================
*
*							その他の汎用サブルーチン
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


// 文字列の最後に "/" を付ける
//   Tandem では / の代わりに . を追加
std::wstring SetSlashTail(std::wstring&& path) {
	wchar_t separator = L'/';
#if defined(HAVE_TANDEM)
	if (AskHostType() == HTYPE_TANDEM)
		separator = L'.';
#endif
	if (!path.ends_with(separator))
		path += separator;
	return path;
}


/*----- 文字列内の特定の文字を全て置き換える ----------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char Src : 検索文字
*		char Dst : 置換文字
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void ReplaceAll(char* Str, char Src, char Dst) {
#if defined(HAVE_TANDEM)
	/* Tandem ではノード名の変換を行わない */
	/* 最初の1文字が \ でもそのままにする */
	if (AskRealHostType() == HTYPE_TANDEM && strlen(Str) > 0)
		Str++;
#endif
	for (char* Pos; (Pos = strchr(Str, Src)) != NULL;)
		*Pos = Dst;
}
// 文字列内の特定の文字を全て置き換える
//   Tandem ではノード名の変換を行わないため、最初の1文字が \ でもそのままにする
//   置換対象fromが \ 以外ならstd::replaceでよい
std::wstring ReplaceAll(std::wstring&& str, wchar_t from, wchar_t to) {
	if (!empty(str)) {
		auto it = begin(str);
#if defined(HAVE_TANDEM)
		if (AskRealHostType() == HTYPE_TANDEM)
			++it;
#endif
		std::replace(it, end(str), from, to);
	}
	return str;
}


// パス名の中のファイル名の先頭を返す
//   ディレクトリの区切り記号は "\" と "/" の両方が有効
//   Tandem は . がデリミッタとなる
std::wstring_view GetFileName(std::wstring_view path) {
	if (auto pos = path.find_first_of(L':'); pos != std::wstring_view::npos)
		path = path.substr(pos + 1);
	auto delimiter = L"\\/"sv;
#if defined(HAVE_TANDEM)
	if (AskHostType() == HTYPE_TANDEM)
		delimiter = L"\\/."sv;
#endif
	if (auto pos = path.find_last_of(delimiter); pos != std::wstring_view::npos)
		path = path.substr(pos + 1);
	return path;
}


// パス名からファイル名を取り除く
//   ディレクトリの区切り記号は "\" と "/" の両方が有効
static std::string_view RemoveFileName(std::string_view path) {
	if (auto const p = path.rfind('/'); p != std::string_view::npos)
		return path.substr(0, p);
	if (auto const p = path.rfind('\\'); p != std::string_view::npos)
		if (p == 0 || path[p - 1] != ':')
			return path.substr(0, p);
	return path;
}


// ファイルサイズを文字列に変換する
std::wstring MakeSizeString(double size) {
	if (size < 1024)
		return strprintf(L"%.0lfB", size);
	size /= 1024;
	if (size < 1024)
		return strprintf(L"%.2lfKB", size);
	size /= 1024;
	if (size < 1024)
		return strprintf(L"%.2lfMB", size);
	size /= 1024;
	if (size < 1024)
		return strprintf(L"%.2lfGB", size);
	size /= 1024;
	if (size < 1024)
		return strprintf(L"%.2lfTB", size);
	size /= 1024;
	return strprintf(L"%.2lfPB", size);
}


// StaticTextの領域に収まるようにパス名を整形して表示
void DispStaticText(HWND hWnd, std::wstring text) {
	RECT rect;
	GetClientRect(hWnd, &rect);
	auto font = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
	auto dc = GetDC(hWnd);
	auto previous = SelectObject(dc, font);
	for (;;) {
		if (size(text) <= 4)
			break;
		if (SIZE size; GetTextExtentPoint32W(dc, data(text), size_as<int>(text), &size) && size.cx <= rect.right)
			break;
		auto const pos = text.find_first_of(L"\\/"sv, 4);
		text.erase(0, pos != std::wstring::npos ? pos - 3 : 1);
		std::fill_n(begin(text), 3, L'.');
	}
	SelectObject(dc, previous);
	ReleaseDC(hWnd, dc);
	SetText(hWnd, text);
}


/*----- RECTをクライアント座標からスクリーン座標に変換 ------------------------
*
*	Parameter
*		HWND hWnd : ウインドウハンドル
*		RECT *Rect : RECT
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/

void RectClientToScreen(HWND hWnd, RECT *Rect)
{
	POINT Tmp;

	Tmp.x = Rect->left;
	Tmp.y = Rect->top;
	ClientToScreen(hWnd, &Tmp);
	Rect->left = Tmp.x;
	Rect->top = Tmp.y;

	Tmp.x = Rect->right;
	Tmp.y = Rect->bottom;
	ClientToScreen(hWnd, &Tmp);
	Rect->right = Tmp.x;
	Rect->bottom = Tmp.y;

	return;
}


static auto GetFilterString(std::initializer_list<FileType> fileTypes) {
	static auto const map = [] {
		std::map<FileType, std::wstring> map;
		for (auto fileType : AllFileTyes) {
			// リソース文字列は末尾 '\0' を扱えないので '\t' を利用し、ここで置換する。
			auto text = GetString(static_cast<UINT>(fileType));
			for (auto& ch : text)
				if (ch == L'\t')
					ch = L'\0';
			map.emplace(fileType, text);
		}
		return map;
	}();
	return std::accumulate(begin(fileTypes), end(fileTypes), L""s, [](auto const& result, auto fileType) { return result + map.at(fileType); });
}

// ファイル選択
fs::path SelectFile(bool open, HWND hWnd, UINT titleId, const wchar_t* initialFileName, const wchar_t* extension, std::initializer_list<FileType> fileTypes) {
	auto const filter = GetFilterString(fileTypes);
	wchar_t buffer[FMAX_PATH + 1];
	wcscpy(buffer, initialFileName);
	auto const title = GetString(titleId);
	DWORD flags = (open ? OFN_FILEMUSTEXIST : OFN_EXTENSIONDIFFERENT | OFN_OVERWRITEPROMPT) | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
	OPENFILENAMEW ofn{ sizeof(OPENFILENAMEW), hWnd, 0, filter.c_str(), nullptr, 0, 1, buffer, size_as<DWORD>(buffer), nullptr, 0, nullptr, title.c_str(), flags, 0, 0, extension };
	auto const cwd = fs::current_path();
	int result = (open ? GetOpenFileNameW : GetSaveFileNameW)(&ofn);
	fs::current_path(cwd);
	if (!result)
		return {};
	return buffer;
}


// ディレクトリを選択
fs::path SelectDir(HWND hWnd) {
	fs::path path;
	auto const cwd = fs::current_path();
	auto const title = GetString(IDS_MSGJPN185);
	BROWSEINFOW bi{ hWnd, nullptr, nullptr, title.c_str(), BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE };
	if (auto idlist = SHBrowseForFolderW(&bi)) {
		wchar_t buffer[FMAX_PATH + 1];
		SHGetPathFromIDListEx(idlist, buffer, size_as<DWORD>(buffer), GPFIDL_DEFAULT);
		path = buffer;
		CoTaskMemFree(idlist);
	}
	fs::current_path(cwd);
	return path;
}


#if defined(HAVE_TANDEM)
/*----- ファイルサイズからEXTENTサイズの計算を行う ----------------------------
*
*	Parameter
*		LONGLONG Size : ファイルサイズ
*
*	Return Value
*		なし
*----------------------------------------------------------------------------*/
void CalcExtentSize(TRANSPACKET *Pkt, LONGLONG Size)
{
	LONGLONG extent;

	/* EXTENTS(4,28) MAXEXTENTS 978 */
	if(Size < 56025088) {
		Pkt->PriExt = DEF_PRIEXT;
		Pkt->SecExt = DEF_SECEXT;
		Pkt->MaxExt = DEF_MAXEXT;
	} else {
		/* 増加余地を残すため Used 75% 近辺になるように EXTENT サイズを調整) */
		extent = (LONGLONG)(Size / ((DEF_MAXEXT * 0.75) * 2048LL));
		/* 28未満にすると誤差でFile Fullになる可能性がある */
		if(extent < 28)
			extent = 28;

		Pkt->PriExt = (int)extent;
		Pkt->SecExt = (int)extent;
		Pkt->MaxExt = DEF_MAXEXT;
	}
}
#endif

static auto QueryDisplayDPI() {
	static auto dpi = [] {
		int x = 0, y = 0;
		if (auto dc = GetDC(0)) {
			x = GetDeviceCaps(dc, LOGPIXELSX);
			y = GetDeviceCaps(dc, LOGPIXELSY);
			ReleaseDC(0, dc);
		}
		return std::tuple<int, int>{ x, y };
	}();
	return dpi;
}

int CalcPixelX(int x) {
	auto [dpix, _] = QueryDisplayDPI();
	return (x * dpix + 96 / 2) / 96;
}

int CalcPixelY(int y) {
	auto [_, dpiy] = QueryDisplayDPI();
	return (y * dpiy + 96 / 2) / 96;
}
