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


/*----- 大文字／小文字を区別しないstrstr --------------------------------------
*
*	Parameter
*		char *s1 : 文字列１
*		char *s2 : 文字列２
*
*	Return Value
*		char *文字列１中で文字列２が見つかった位置
*			NULL=見つからなかった
*----------------------------------------------------------------------------*/

const char* stristr(const char* s1, const char* s2)
{
	const char* Ret;

	Ret = NULL;
	while(*s1 != NUL)
	{
		if((tolower(*s1) == tolower(*s2)) &&
		   (_strnicmp(s1, s2, strlen(s2)) == 0))
		{
			Ret = s1;
			break;
		}
		s1++;
	}
	return(Ret);
}


/*----- 文字列中のスペースで区切られた次のフィールドを返す --------------------
*
*	Parameter
*		char *Str : 文字列
*
*	Return Value
*		char *次のフィールド
*			NULL=見つからなかった
*----------------------------------------------------------------------------*/

const char* GetNextField(const char* Str)
{
	if((Str = strchr(Str, ' ')) != NULL)
	{
		while(*Str == ' ')
		{
			if(*Str == NUL)
			{
				Str = NULL;
				break;
			}
			Str++;
		}
	}
	return(Str);
}


/*----- 現在のフィールドの文字列をコピーする ----------------------------------
*
*	Parameter
*		char *Str : 文字列
*		char *Buf : コピー先
*		int Max : 最大文字数
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL=長さが長すぎる
*----------------------------------------------------------------------------*/

int GetOneField(const char* Str, char *Buf, int Max)
{
	int Sts;
	const char* Pos;

	Sts = FFFTP_FAIL;
	if((Pos = strchr(Str, ' ')) == NULL)
	{
		if((int)strlen(Str) <= Max)
		{
			strcpy(Buf, Str);
			Sts = FFFTP_SUCCESS;
		}
	}
	else
	{
		if(Pos - Str <= Max)
		{
			strncpy(Buf, Str, Pos - Str);
			*(Buf + (Pos - Str)) = NUL;
			Sts = FFFTP_SUCCESS;
		}
	}
	return(Sts);
}


/*----- パス名の中のファイル名の先頭を返す ------------------------------------
*
*	Parameter
*		char *Path : パス名
*
*	Return Value
*		char *ファイル名の先頭
*
*	Note
*		ディレクトリの区切り記号は "\" と "/" の両方が有効
*----------------------------------------------------------------------------*/

const char* GetFileName(const char* Path)
{
	const char* Pos;

	if((Pos = strchr(Path, ':')) != NULL)
		Path = Pos + 1;

	if((Pos = strrchr(Path, '\\')) != NULL)
		Path = Pos + 1;

	if((Pos = strrchr(Path, '/')) != NULL)
		Path = Pos + 1;

#if defined(HAVE_TANDEM)
	/* Tandem は . がデリミッタとなる */
	if((AskHostType() == HTYPE_TANDEM) && ((Pos = strrchr(Path, '.')) != NULL))
		Path = Pos + 1;
#endif
	return(Path);
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


/*----- ＵＮＣ文字列を分解する ------------------------------------------------
*
*	Parameter
*		char *unc : UNC文字列
*		char *Host : ホスト名をコピーするバッファ (サイズは HOST_ADRS_LEN+1)
*		char *Path : パス名をコピーするバッファ (サイズは FMAX_PATH+1)
*		char *File : ファイル名をコピーするバッファ (サイズは FMAX_PATH+1)
*		char *User : ユーザ名をコピーするバッファ (サイズは USER_NAME_LEN+1)
*		char *Pass : パスワードをコピーするバッファ (サイズは PASSWORD_LEN+1)
*		int *Port : ポート番号をコピーするバッファ
*
*	Return Value
*		int ステータス
*			FFFTP_SUCCESS/FFFTP_FAIL
*
*	"\"は全て"/"に置き換える
*----------------------------------------------------------------------------*/

int SplitUNCpath(char *unc, std::wstring& Host, std::wstring& Path, std::wstring& File, std::wstring& User, std::wstring& Pass, int *Port)
{
	char *Pos1;
	char *Pos2;
	char Tmp[FMAX_PATH+1];

	Host.clear();
	Path.clear();
	File.clear();
	User.clear();
	Pass.clear();
	*Port = IPPORT_FTP;

	ReplaceAll(unc, '\\', '/');

	if((Pos1 = strstr(unc, "//")) != NULL)
		Pos1 += 2;
	else
		Pos1 = unc;

	if((Pos2 = strchr(Pos1, '@')) != NULL)
	{
		memset(Tmp, NUL, FMAX_PATH+1);
		memcpy(Tmp, Pos1, Pos2-Pos1);
		Pos1 = Pos2 + 1;

		if ((Pos2 = strchr(Tmp, ':')) != NULL) {
			User = u8({ Tmp, Pos2 });
			Pass = u8(Pos2 + 1);
		} else
			User = u8(Tmp);
	}

	// IPv6対応
	if((Pos2 = strchr(Pos1, '[')) != NULL && Pos2 < strchr(Pos1, ':'))
	{
		Pos1 = Pos2 + 1;
		if((Pos2 = strchr(Pos2, ']')) != NULL)
		{
			Host = u8({ Pos1, Pos2 });
			Pos1 = Pos2 + 1;
		}
	}

	if((Pos2 = strchr(Pos1, ':')) != NULL)
	{
		if (empty(Host))
			Host = u8({ Pos1, Pos2 });
		Pos2++;
		if(IsDigit(*Pos2))
		{
			*Port = atoi(Pos2);
			while(*Pos2 != NUL)
			{
				if(IsDigit(*Pos2) == 0)
					break;
				Pos2++;
			}
		}
		Path = u8(RemoveFileName(Pos2));
		File = u8(GetFileName(Pos2));
	}
	else if((Pos2 = strchr(Pos1, '/')) != NULL)
	{
		if (empty(Host))
			Host = u8({ Pos1, Pos2 });
		Path = u8(RemoveFileName(Pos2));
		File = u8(GetFileName(Pos2));
	}
	else
	{
		if (empty(Host))
			Host = u8(Pos1);
	}

	return empty(Host) ? FFFTP_FAIL : FFFTP_SUCCESS;
}


/*----- 日付文字列(JST)をFILETIME(UTC)に変換 ----------------------------------
*
*	Parameter
*		char *Time : 日付文字列 ("yyyy/mm/dd hh:mm")
*		FILETIME *Buf : ファイルタイムを返すワーク
*
*	Return Value
*		int ステータス
*			YES/NO=日付情報がなかった
*----------------------------------------------------------------------------*/

int TimeString2FileTime(const char *Time, FILETIME *Buf)
{
	SYSTEMTIME sTime;
	FILETIME fTime;
	int Ret;

	Ret = NO;
	Buf->dwLowDateTime = 0;
	Buf->dwHighDateTime = 0;

	if(strlen(Time) >= 16)
	{
		if(IsDigit(Time[0]) && IsDigit(Time[5]) && IsDigit(Time[8]) && 
		   IsDigit(Time[12]) && IsDigit(Time[14]))
		{
			Ret = YES;
		}

		sTime.wYear = atoi(Time);
		sTime.wMonth = atoi(Time + 5);
		sTime.wDay = atoi(Time + 8);
		if(Time[11] != ' ')
			sTime.wHour = atoi(Time + 11);
		else
			sTime.wHour = atoi(Time + 12);
		sTime.wMinute = atoi(Time + 14);
		// タイムスタンプのバグ修正
//		sTime.wSecond = 0;
		if(strlen(Time) >= 19)
			sTime.wSecond = atoi(Time + 17);
		else
			sTime.wSecond = 0;
		sTime.wMilliseconds = 0;

		SystemTimeToFileTime(&sTime, &fTime);
		LocalFileTimeToFileTime(&fTime, Buf);
	}
	return(Ret);
}


/*----- 属性文字列を値に変換 --------------------------------------------------
*
*	Parameter
*		char *Str : 属性文字列 ("rwxrwxrwx")
*
*	Return Value
*		int 値
*----------------------------------------------------------------------------*/

int AttrString2Value(const char* Str) {
	int num = 0;
	if (strlen(Str) >= 9) {
		for (auto bit : { 0x400, 0x200, 0x100, 0x40, 0x20, 0x10, 0x4, 0x2, 0x1 })
			if (*Str++ != '-')
				num |= bit;
	} else if (strlen(Str) >= 3)
		std::from_chars(Str, Str + 3, num, 16);
	return num;
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
