/*=============================================================================
*							ＦＦＦＴＰ共通定義ファイル
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

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UMDF_USING_NTSTATUS
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <future>
#include <iterator>
#include <map>
#include <mutex>
#include <numeric>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>
#include <concurrent_queue.h>
#include <boost/regex.hpp>
#include <cassert>
#include <cwctype>
#include <crtdbg.h>
#include <mbstring.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <ObjBase.h>			// for COM interface, define `interface` macro.
#include <bcrypt.h>
#include <windowsx.h>
#include <winsock2.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <MLang.h>
#include <MMSystem.h>
#include <mstcpip.h>
#include <ntstatus.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <ShObjIdl.h>
#include <WS2tcpip.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <comdef.h>
#include "config.h"
#include "dialog.h"
#include "helpid.h"
#include "Resource/resource.ja-JP.h"
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "normaliz.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Ws2_32.lib")
namespace fs = std::filesystem;
using namespace std::literals;
template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
template<class...>
constexpr bool false_v = false;

enum class FileType : UINT {
	All = IDS_FILETYPE_ALL,
	Executable = IDS_FILETYPE_EXECUTABLE,
	Reg = IDS_FILETYPE_REG,
	Ini = IDS_FILETYPE_INI,
	Xml = IDS_FILETYPE_XML,
};

constexpr FileType AllFileTyes[]{ FileType::All, FileType::Executable, FileType::Reg, FileType::Ini, FileType::Xml, };


#define NUL				'\0'

#define IsDigit(n)		(isascii(n) && isdigit(n))
#define IsAlpha(n)		(isascii(n) && isalpha(n))

#define FFFTP_FAIL			0
#define FFFTP_SUCCESS			1

#define NO				0
#define YES				1
#define NO_ALL			2
#define YES_ALL			3
#define YES_LIST		4

/*===== バージョン ======*/

#ifdef _WIN64
#define VER_STR					"5.1 64bit"
#else
#define VER_STR					"5.1 32bit"
#endif
#define VER_NUM					2000		/* 設定バージョン */

/*===== ユーザ定義コマンド =====*/

#define WM_CHANGE_COND	(WM_USER+1)	/* ファイル一覧を変更するコマンド */
#define WM_SET_PACKET	(WM_USER+2)	/* 現在使用している転送パケットのアドレスを通知 */

#define WM_ASYNC_SOCKET	(WM_USER+5)

#define WM_REFRESH_LOCAL_FLG	(WM_USER+7)
#define WM_REFRESH_REMOTE_FLG	(WM_USER+8)
#define WM_RECONNECTSOCKET	(WM_USER+11)
#define WM_MAINTHREADRUNNER	(WM_USER+13)

/*===== ホスト番号 =====*/
/* ホスト番号は 0～ の値を取る */

#define HOSTNUM_NOENTRY	(-1)	/* ホスト一覧に無いときのホスト番号 */

/*===== バッファサイズ =====*/

#define HOST_NAME_LEN	40		/* 一覧に表示するホストの名前 */
#define HOST_ADRS_LEN	80		/* ホスト名 */
#define USER_NAME_LEN	80		/* ユーザ名 */
#define PASSWORD_LEN	80		/* パスワード */
#define ACCOUNT_LEN		80		/* アカウント */
#define INIT_DIR_LEN	(FMAX_PATH-40)	/* 初期ディレクトリ */
#define FILTER_EXT_LEN	400		/* フィルタのファイル名列 */
#define BOOKMARK_SIZE	2048	/* ブックマーク */
#define CHMOD_CMD_LEN	40		/* 属性変更コマンド */
#define NLST_NAME_LEN	40		/* NLSTに付けるファイル名／オプション */
#define INITCMD_LEN		256		/* 初期化コマンド */
#define OWNER_NAME_LEN	40		/* オーナ名 */
#define RAS_NAME_LEN	256		/* RASのエントリ名の長さ */

#define FMAX_PATH		1024

/*===== 初期値 =====*/

#if defined(HAVE_TANDEM)
#define DEF_PRIEXT		4				/* Primary Extents の初期値 */
#define DEF_SECEXT		28				/* Secondary Extents の初期値 */
#define DEF_MAXEXT		978				/* Max Extents の初期値 */
#endif

/*===== 同じ名前のファイルがあった時の処理 =====*/

#define EXIST_OVW		0		/* 上書き */
#define EXIST_NEW		1		/* 新しければ上書き */
#define EXIST_RESUME	2		/* レジューム */
#define EXIST_IGNORE	3		/* 無視 */
#define EXIST_UNIQUE	4		/* ホストが名前を付ける */
#define EXIST_ABORT		5		/* 全て中止 */
// 同じ名前のファイルの処理方法追加
#define EXIST_LARGE		6		/* 大きければ上書き */

/*===== ファイル名の比較モード =====*/

#define COMP_IGNORE		0		/* 大文字/小文字は区別しない */
#define COMP_STRICT		1		/* 大文字/小文字を区別する */
#define COMP_LOWERMATCH	2		/* 大文字/小文字を区別しない（片側は全て小文字） */

/*===== FTPの応答コードの頭１桁 =====*/

#define FTP_PRELIM		1		/* */
#define FTP_COMPLETE	2		/* */
#define FTP_CONTINUE	3		/* */
#define FTP_RETRY		4		/* */
#define FTP_ERROR		5		/* */

/*===== ファイルリストのノード属性 =====*/

#define NODE_DIR		0		/* ディレクトリ */
#define NODE_FILE		1		/* ファイル */
#define NODE_DRIVE		2		/* ドライブ */
#define NODE_NONE		(-1)	/* なし */

/*===== 上書き確認方法の設定値 =====*/

#define TRANS_OVW		0		/* 上書き */
#define TRANS_DLG		1		/* ダイアログを出す */

/*===== ホスト内ファイル移動確認方法の設定値 =====*/

#define MOVE_NODLG		0		/* ダイアログを出さない */
#define MOVE_DLG		1		/* ダイアログを出す */
#define MOVE_DISABLE	2		/* 機能使用禁止 */

/*===== 転送方法 =====*/

#define TYPE_I			'I'		/* バイナリモード */
#define TYPE_A			'A'		/* アスキーモード */
#define TYPE_X			'X'		/* 自動判別 */

#define TYPE_DEFAULT	NUL		/* 設定しない DirectConnectProc()の引数として有効 */

/*===== ソート方法 =====*/
/* 番号はListViewのカラム番号と合わせてある */

#define SORT_NAME		0		/* 名前順 */
#define SORT_DATE		1		/* 日付順 */
#define SORT_SIZE		2		/* サイズ順 */
#define SORT_EXT		3		/* 拡張子順 */

#define SORT_ASCENT		0x00	/* 昇順 */
#define SORT_DESCENT	0x80	/* 降順 */

#define SORT_MASK_ORD	0x7F	/* ｘｘ順を取り出すマスク */
#define SORT_GET_ORD	0x80	/* 昇順／降順を取り出すマスク */

#define SORT_NOTSAVED	0xFFFF'FFFFu	/* ホスト毎のセーブ方法を保存していない時の値 */

/*===== ソートする場所 =====*/

#define ITEM_LFILE		0		/* ローカルの名前 */
#define ITEM_LDIR		1		/* ローカルのディレクトリ */
#define ITEM_RFILE		2		/* ホストの名前 */
#define ITEM_RDIR		3		/* ホストのディレクトリ */

/*===== ウインドウ番号 =====*/

#define WIN_LOCAL		0		/* ローカル */
#define WIN_REMOTE		1		/* ホスト */
#define WIN_BOTH		2		/* 両方 */

/*===== ファイル選択方法 =====*/

#define SELECT_ALL		0		/* 全選択 */
#define SELECT_REGEXP	1		/* 検索式を入力して選択 */
// ローカル側自動更新
#define SELECT_LIST		2		/* リストに含まれるファイルを選択 */

/*===== 検索方法 =====*/

#define FIND_FIRST		0		/* 最初の検索 */
#define FIND_NEXT		1		/* 次を検索 */

/*===== ファイル名の大文字／小文字変換 =====*/

#define FNAME_NOCNV		0		/* 変換しない */
#define FNAME_LOWER		1		/* 小文字に変換 */
#define FNAME_UPPER		2		/* 大文字に変換 */

/*===== 接続ウインドウの形式 =====*/

#define DLG_TYPE_CON	0		/* 簡易（ホスト編集なし） */
#define DLG_TYPE_SET	1		/* ホスト編集あり */

/*===== ファイル一覧取得モード =====*/

#define CACHE_NORMAL	0		/* 通常（キャッシュにあれば使用、なければ読み込み） */
#define CACHE_REFRESH	1		/* 常に読み込み */
#define CACHE_LASTREAD	2		/* 最後に読み込んだものを使用 */

/*===== 漢字コード変換 =====*/

#define KANJI_SJIS		0		/* SJIS */
#define KANJI_JIS		1		/* JIS */
#define KANJI_EUC		2		/* EUC */
#define KANJI_SMB_HEX	3		/* Samba-HEX */
#define KANJI_SMB_CAP	4		/* Samba-CAP */
#define KANJI_UTF8N		5		/* UTF-8 */
// UTF-8対応
#define KANJI_UTF8BOM		6		/* UTF-8 BOM */
// UTF-8 HFS+対応
#define KANJI_UTF8HFSX		7		/* UTF-8 HFS+ */

#define KANJI_NOCNV		-1		/* 漢字コード変換なし */

// UTF-8対応
#define KANJI_AUTO		-1

/*===== ビューワ =====*/

#define VIEWERS			3		/* ビューワの数 */

/*===== ブックマーク =====*/

#define BMARK_SUB_MENU		2		/* ブックマークメニューのサブメニュー番号 */
#define DEFAULT_BMARK_ITEM	5		/* ブックマークメニューにある固定部分の数 */
#define MENU_BMARK_TOP		30000	/* 3000以降(3100くらいまで)は予約する */
									/* resource.h の定義と重ならないように */

/*===== レジストリのタイプ =====*/

#define REGTYPE_REG		0		/* レジストリ */
#define REGTYPE_INI		1		/* INIファイル */

/*===== ホスト設定で明示的に指定するホストのタイプ =====*/

#define	HTYPE_AUTO		0		/* 自動 */
#define	HTYPE_ACOS		1		/* ACOS (待機結合ファイルの指定が必要) */
#define	HTYPE_VMS		2		/* VAX VMS */
#define	HTYPE_IRMX		3		/* IRMX */
#define	HTYPE_ACOS_4	4		/* ACOS ファイル名を('')で括らない */
#define	HTYPE_STRATUS	5		/* Stratus */
#define	HTYPE_AGILENT	6		/* Agilent Logic analyzer */
#define	HTYPE_SHIBASOKU	7		/* Shibasoku LSI test system */
#if defined(HAVE_TANDEM)
#define HTYPE_TANDEM	8		/* HP NonStop Server */
#endif

/*===== ホストのヒストリ =====*/

#define	HISTORY_MAX		20		/* ファイルのヒストリの最大個数 */
#define DEF_FMENU_ITEMS	8		/* Fileメニューにある項目数の初期値 */

/*===== 中断コード =====*/

#define ABORT_NONE			0		/* 転送中断なし */
#define ABORT_USER			1		/* ユーザによる中断 */
#define ABORT_ERROR			2		/* エラーによる中断 */
#define ABORT_DISKFULL		3		/* ディスクフルよる中断 */

/*===== FireWallの種類 =====*/

#define FWALL_NONE			0		/* なし */
#define FWALL_FU_FP_SITE	1		/* FW user → FW pass → SITE host */
#define FWALL_FU_FP_USER	2		/* FW user → FW pass → USER user@host */
#define FWALL_USER			3		/* USER user@host */
#define FWALL_OPEN			4		/* OPEN host */
#define FWALL_SOCKS4		5		/* SOCKS4 */
#define FWALL_SOCKS5_NOAUTH	6		/* SOCKS5 (認証なし) */
#define FWALL_SOCKS5_USER	7		/* SOCKS5 (Username/Password認証) */
#define FWALL_FU_FP			8		/* FW user → FW pass */
#define FWALL_SIDEWINDER	9		/* USER FWuser:FWpass@host */

/*===== ワンタイムパスワード =====*/

/* コードの種類 */
#define SECURITY_DISABLE	0		/* 使用しない */
#define SECURITY_AUTO		1		/* 自動認識 */
#define MD4					2		/* MD4 */
#define MD5					3		/* MD5 */
#define SHA1				4		/* SHA-1 */

#define MAX_SEED_LEN		16		/* Seedの文字数 */

/*===== 再帰的なファイル検索の方法 =====*/

#define RDIR_NONE		0		/* 再帰検索なし */
#define RDIR_NLST		1		/* NLST -R */
#define RDIR_CWD		2		/* CWDで移動 */

/*===== 設定のレベル =====*/

#define SET_LEVEL_GROUP		0x8000		/* グループのフラグ */
#define SET_LEVEL_MASK		0x7FFF
#define SET_LEVEL_SAME		0x7FFF

/*===== 日付、数値の表示形式 =====*/

#define DISPFORM_LEGACY		0		/* 固定書式 */
#define DISPFORM_LOCALE		1		/* コントロールパネルに従う */

/*===== ファイル一覧に存在した情報 =====*/

#define FINFO_DATE			0x01	/* 日付 */
#define FINFO_TIME			0x02	/* 時間 */
#define FINFO_SIZE			0x04	/* サイズ */
#define FINFO_ATTR			0x08	/* 属性 */
#define FINFO_ALL			0xFF	/* 全て */

/*===== GetMasterPasswordStatusで使うコード =====*/
#define PASSWORD_OK 0
#define PASSWORD_UNMATCH 2
#define BAD_PASSWORD_HASH 3

/*===== 暗号化パスワード設定 =====*/
#define DEFAULT_PASSWORD	"DefaultPassword"
#define MAX_PASSWORD_LEN	128

// 暗号化通信対応
#define CRYPT_NONE			0
#define CRYPT_FTPES			1
#define CRYPT_FTPIS			2
#define CRYPT_SFTP			3

// FEAT対応
// UTF-8対応
#define FEATURE_UTF8		0x00000001
// MLSD対応
#define FEATURE_MLSD		0x00000002
// IPv6対応
#define FEATURE_EPRT		0x00000004
#define FEATURE_EPSV		0x00000008
// ホスト側の日時取得
#define FEATURE_MDTM		0x00000010
// ホスト側の日時設定
#define FEATURE_MFMT		0x00000020

// IPv6対応
#define NTYPE_AUTO			0		/* 自動 */
#define NTYPE_IPV4			1		/* TCP/IPv4 */
#define NTYPE_IPV6			2		/* TCP/IPv6 */


struct HostExeptPassword {
	static inline auto DefaultChmod = L"SITE CHMOD"s;	/* 属性変更コマンド */
	static inline auto DefaultLsOption = L"-alL"s;		/* NLSTに付けるもの */
	static inline int DefaultTimeZone = [] {
		TIME_ZONE_INFORMATION tzi;
		GetTimeZoneInformation(&tzi);
		return tzi.Bias / -60;
	}();
	std::wstring HostAdrs;								/* ホスト名 */
	std::wstring UserName;								/* ユーザ名 */
	std::wstring Account;								/* アカウント */
	std::wstring LocalInitDir;							/* ローカルの開始ディレクトリ */
	std::wstring RemoteInitDir;							/* ホストの開始ディレクトリ */
	std::wstring ChmodCmd = DefaultChmod;				/* 属性変更コマンド */
	std::wstring LsName = DefaultLsOption;				/* NLSTに付けるファイル名/オプション*/
	std::wstring InitCmd;								/* ホストの初期化コマンド */
	int Port = IPPORT_FTP;								/* ポート番号 */
	int KanjiCode = KANJI_NOCNV;						/* ホストの漢字コード (KANJI_xxx) */
	int KanaCnv = YES;									/* 半角カナを全角に変換(YES/NO) */
	int NameKanjiCode = KANJI_NOCNV;					/* ファイル名の漢字コード (KANJI_xxx) */
	int NameKanaCnv = NO;								/* ファイル名の半角カナを全角に変換(YES/NO) */
	int Pasv = YES;										/* PASVモード (YES/NO) */
	int FireWall = NO;									/* FireWallを使う (YES/NO) */
	int ListCmdOnly = YES;								/* "LIST"コマンドのみ使用する */
	int UseNLST_R = YES;								/* "NLST -R"コマンドを使用する */
	int TimeZone = DefaultTimeZone;						/* タイムゾーン (-12～12) */
	int HostType = HTYPE_AUTO;							/* ホストのタイプ (HTYPE_xxx) */
	int SyncMove = NO;									/* フォルダ同時移動 (YES/NO) */
	int NoFullPath = NO;								/* フルパスでファイルアクセスしない (YES/NO) */
	uint32_t Sort = SORT_NOTSAVED;						/* ソート方法 (0x11223344 : 11=LFsort 22=LDsort 33=RFsort 44=RFsort) */
	int Security = SECURITY_AUTO;						/* セキュリティ (SECURITY_xxx , MDx) */
	int Dialup = NO;									/* ダイアルアップ接続するかどうか (YES/NO) */
	int DialupAlways = NO;								/* 常にこのエントリへ接続するかどうか (YES/NO) */
	int DialupNotify = YES;								/* 再接続の際に確認する (YES/NO) */
	std::wstring DialEntry;								/* ダイアルアップエントリ */
	int UseNoEncryption = YES;							/* 暗号化なしで接続する (YES/NO) */
	int UseFTPES = YES;									/* FTPESで接続する (YES/NO) */
	int UseFTPIS = YES;									/* FTPISで接続する (YES/NO) */
	int UseSFTP = YES;									/* SFTPで接続する (YES/NO) */
	int MaxThreadCount = 1;								/* 同時接続数 */
	int ReuseCmdSkt = YES;								/* メインウィンドウのソケットを再利用する (YES/NO) */
	int UseMLSD = YES;									/* "MLSD"コマンドを使用する */
	int NoopInterval = 60;								/* 無意味なコマンドを送信する間隔（秒数、0で無効）*/
	int TransferErrorMode = EXIST_OVW;					/* 転送エラー時の処理 (EXIST_xxx) */
	int TransferErrorNotify = YES;						/* 転送エラー時に確認ダイアログを出すかどうか (YES/NO) */
	int TransferErrorReconnect = YES;					/* 転送エラー時に再接続する (YES/NO) */
	int NoPasvAdrs = NO;								/* PASVで返されるアドレスを無視する (YES/NO) */
	inline HostExeptPassword();
};

struct Host : HostExeptPassword {
	std::wstring PassWord;
	Host() = default;
	Host(Host const&) = default;
	Host(Host const& that, bool includePassword) : HostExeptPassword{ that }, PassWord{ includePassword ? that.PassWord : L""s } {}
};


struct HOSTDATA : Host {
	int Level = 0;							/* 設定のレベル */
											/* 通常はグループのフラグのみが有効 */
											/* レベル数は設定の登録／呼出時のみで使用 */
	std::wstring HostName;					/* 設定名 */
	std::vector<std::wstring> BookMark;		/* ブックマーク */
	int Anonymous = NO;						/* Anonymousフラグ */
	int CurNameKanjiCode = KANJI_NOCNV;		/* 自動判別後のファイル名の漢字コード (KANJI_xxx) */
	int LastDir = NO;						/* 最後にアクセスしたフォルダを保存 */
	int CryptMode = CRYPT_NONE;				/* 暗号化通信モード (CRYPT_xxx) */
	int NoDisplayUI = NO;					/* UIを表示しない (YES/NO) */
	int Feature = 0;						/* 利用可能な機能のフラグ (FEATURE_xxx) */
	int CurNetType = NTYPE_AUTO;			/* 接続中のネットワークの種類 (NTYPE_xxx) */
	HOSTDATA() = default;
	HOSTDATA(struct HISTORYDATA const& history);
};


struct HISTORYDATA : Host {
	int Type;							/* 転送方法 (TYPE_xx) */
	HISTORYDATA() : Type{ 0 } {}
	HISTORYDATA(HISTORYDATA const&) = default;
	HISTORYDATA(Host const& that, bool includePassword, int type) : Host{ that, includePassword }, Type{ type } {}
};


struct TRANSPACKET {
	SOCKET ctrl_skt = INVALID_SOCKET;	/* Socket */
	std::wstring Command;				/* STOR/RETR/MKD */
	std::wstring Remote;				/* ホスト側のファイル名（フルパス） */
										/* VMSの時は ddd[xxx.yyy]/yyy/zzz のように */
										/* なってるので注意 */
	fs::path Local;						/* ローカル側のファイル名（フルパス） */
	int Type = 0;						/* 転送方法 (TYPE_xx) */
	LONGLONG Size = 0;					/* ファイルのサイズ */
	LONGLONG ExistSize = 0;				/* すでに存在するファイルのサイズ */
										/* 転送中は、転送したファイルのサイズを格納する */
	FILETIME Time = {};					/* ファイルの時間(UTC) */
	int Attr = 0;						/* ファイルの属性 */
	int KanjiCode = 0;					/* 漢字コード (KANJI_xxx) */
	int KanjiCodeDesired = 0;			/* ローカルの漢字コード (KANJI_xxx) */
	int KanaCnv = 0;					/* 半角カナを全角に変換(YES/NO) */
	int Mode = 0;						/* 転送モード (EXIST_xxx) */
#if defined(HAVE_TANDEM)
	int FileCode = 0;					/* ファイルコード */
	int PriExt = 0;						/* Primary Extents */
	int SecExt = 0;						/* Secondary Extents */
	int MaxExt = 0;						/* Max Extents */
#endif
	HWND hWndTrans = nullptr;			/* 転送中ダイアログのウインドウハンドル */
	int Abort = 0;						/* 転送中止フラグ (ABORT_xxx) */
	int NoTransfer = 0;
	int ThreadCount = 0;
};


/*===== ファイルリスト =====*/

struct FILELIST {
	std::string Original;
	std::wstring Name;
	char Node = 0;			/* 種類 (NODE_xxx) */
	char Link = 0;			/* リンクファイルかどうか (YES/NO) */
	LONGLONG Size = 0;		/* ファイルサイズ */
	int Attr = 0;			/* 属性 */
	FILETIME Time = {};		/* 時間(UTC) */
	std::wstring Owner;		/* オーナ名 */
	char InfoExist = 0;		/* ファイル一覧に存在した情報のフラグ (FINFO_xxx) */
	int ImageId = 0;		/* アイコン画像番号 */
	FILELIST() = default;
	FILELIST(std::wstring_view name, char node) : Name{ name }, Node{ node } {}
	FILELIST(std::string_view original, char node) : Original{ original }, Node{ node } {}
	FILELIST(std::wstring_view name, char node, char link, int64_t size, int attr, FILETIME time, char infoExist) : Name{ name }, Node{ node }, Link{ link }, Size{ size }, Attr{ attr }, Time{ time }, InfoExist{ infoExist } {}
	inline FILELIST(std::string_view original, char node, char link, int64_t size, int attr, FILETIME time, std::string_view owner, char infoExist);
	// ディレクトリの階層数を返す
	//  単に '\' と '/'の数を返すだけ
	int DirLevel() const {
		return (int)std::ranges::count_if(Name, [](auto ch) { return ch == L'/' || ch == L'\\'; });
	}
};


class Sound {
	const wchar_t* keyName;
	const wchar_t* name;
	int id;
	Sound(const wchar_t* keyName, const wchar_t* name, int id) : keyName{ keyName }, name{ name }, id{ id } {}
public:
	static Sound Connected;
	static Sound Transferred;
	static Sound Error;
	void Play(){ PlaySoundW(keyName, 0, SND_ASYNC | SND_NODEFAULT | SND_APPLICATION); }
	static void Register();
};


class MainThreadRunner {
protected:
	~MainThreadRunner() = default;
	virtual int DoWork() = 0;
public:
	int Run();
};


/*=================================================
*		プロトタイプ
*=================================================*/

/*===== main.c =====*/

fs::path const& systemDirectory();
fs::path const& moduleDirectory();
fs::path const& tempDirectory();
void DispWindowTitle();
HWND GetMainHwnd(void);
HWND GetFocusHwnd(void);
void SetFocusHwnd(HWND hWnd);
HINSTANCE GetFtpInst(void);
void DoubleClickProc(int Win, int Mode, int App);
void ExecViewer(fs::path const& path, int App);
void ExecViewer2(fs::path const& path1, fs::path const& path2, int App);
void AddTempFileList(fs::path const& file);
void SoundPlay(int Num);
void ShowHelp(DWORD_PTR helpTopicId);
fs::path const& AskIniFilePath();
int AskForceIni(void);
int BackgrndMessageProc(void);
void ResetAutoExitFlg(void);
int AskAutoExit(void);
// マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策
BOOL IsMainThread();
void Restart();
void Terminate();
// タスクバー進捗表示
int LoadTaskbarList3();
void FreeTaskbarList3();
int IsTaskbarList3Loaded();
void UpdateTaskbarProgress();
// 高DPI対応
int AskToolWinHeight(void);

/*===== filelist.c =====*/

int MakeListWin();
void DeleteListWin(void);
HWND GetLocalHwnd(void);
HWND GetRemoteHwnd(void);
void SetListViewType(void);
void GetRemoteDirForWnd(int Mode, int *CancelCheckWork);
void GetLocalDirForWnd(void);
void ReSortDispList(int Win, int *CancelCheckWork);
bool CheckFname(std::wstring const& file, std::wstring const& spec);
void SelectFileInList(HWND hWnd, int Type, std::vector<FILELIST> const& Base);
void FindFileInList(HWND hWnd, int Type);
int GetCurrentItem(int Win);
int GetItemCount(int Win);
int GetSelectedCount(int Win);
int GetFirstSelected(int Win, int All);
int GetNextSelected(int Win, int Pos, int All);
std::wstring GetHotSelected(int Win);
void SetHotSelected(int Win, std::wstring const& name);
std::wstring GetNodeName(int Win, int Pos);
int GetNodeSize(int Win, int Pos, LONGLONG *Buf);
int GetNodeAttr(int Win, int Pos, int *Buf);
int GetNodeType(int Win, int Pos);
void EraseRemoteDirForWnd(void);
double GetSelectedTotalSize(int Win);
int MakeSelectedFileList(int Win, int Expand, int All, std::vector<FILELIST>& Base, int *CancelCheckWork);
std::tuple<fs::path, std::vector<FILELIST>> MakeDroppedFileList(WPARAM wParam);
fs::path MakeDroppedDir(WPARAM wParam);
void AddRemoteTreeToFileList(int Num, std::wstring const& Path, int IncDir, std::vector<FILELIST>& Base);
const FILELIST* SearchFileList(std::wstring_view Fname, std::vector<FILELIST> const& Base, int Caps);
static inline FILELIST* SearchFileList(std::wstring_view Fname, std::vector<FILELIST>& Base, int Caps) {
	return const_cast<FILELIST*>(SearchFileList(Fname, static_cast<std::vector<FILELIST> const&>(Base), Caps));
}
void SetFilter(int *CancelCheckWork);
void doDeleteRemoteFile(void);


/*===== toolmenu.c =====*/

bool MakeToolBarWindow();
void DeleteToolBarWindow(void);
HWND GetMainTbarWnd(void);
HWND GetLocalHistHwnd(void);
HWND GetRemoteHistHwnd(void);
HWND GetLocalHistEditHwnd(void);
HWND GetRemoteHistEditHwnd(void);
HWND GetLocalTbarWnd(void);
HWND GetRemoteTbarWnd(void);
void MakeButtonsFocus(void);
void DisableUserOpe(void);
void EnableUserOpe(void);
bool AskUserOpeDisabled();
void SetTransferTypeImm(int Mode);
void SetTransferType(int Type);
void DispTransferType(void);
int AskTransferType(void);
int AskTransferTypeAssoc(std::wstring_view path, int Type);
void SaveTransferType(void);
void SetHostKanjiCodeImm(int Mode);
void SetHostKanjiCode(int Type);
void DispHostKanjiCode(void);
int AskHostKanjiCode(void);
void HideHostKanjiButton(void);
// UTF-8対応
void SetLocalKanjiCodeImm(int Mode);
void SetLocalKanjiCode(int Type);
void DispLocalKanjiCode(void);
int AskLocalKanjiCode(void);
void HideLocalKanjiButton(void);
void SaveLocalKanjiCode(void);
void SetHostKanaCnvImm(int Mode);
void SetHostKanaCnv(void);
void DispHostKanaCnv(void);
int AskHostKanaCnv(void);
void SetSortTypeImm(int LFsort, int LDsort, int RFsort, int RDsort);
void SetSortTypeByColumn(int Win, int Tab);
int AskSortType(int Name);
void SetSaveSortToHost(int Sw);
int AskSaveSortToHost(void);
void DispListType(void);
void SetSyncMoveMode(int Mode);
void ToggleSyncMoveMode(void);
void DispSyncMoveMode(void);
int AskSyncMoveMode(void);
void SetRemoteDirHist(std::wstring const&  path);
void SetLocalDirHist(fs::path const& path);
fs::path const& AskLocalCurDir();
std::wstring const& AskRemoteCurDir();
void SetCurrentDirAsDirHist();
void DispDotFileMode(void);
void ShowPopupMenu(int Win, int Pos);

/*===== statuswin.c =====*/

int MakeStatusBarWindow();
void DeleteStatusBarWindow(void);
HWND GetSbarWnd(void);
void UpdateStatusBar();
void DispCurrentWindow(int Win);
void DispSelectedSpace(void);
void DispLocalFreeSpace(fs::path const& directory);
void DispTransferFiles(void);
void DispDownloadSize(LONGLONG Size);
bool NotifyStatusBar(const NMHDR* hdr);

/*===== taskwin.c =====*/

int MakeTaskWindow();
void DeleteTaskWindow(void);
HWND GetTaskWnd(void);
void SetTaskMsg(_In_z_ _Printf_format_string_ const char* format, ...);
void DispTaskMsg(void);
void SetTaskMsg(UINT id, ...);
void DoPrintf(_In_z_ _Printf_format_string_ const char* format, ...);
void DoPrintf(_In_z_ _Printf_format_string_ const wchar_t* format, ...);
namespace detail {
	void Notice(UINT id, std::wformat_args args);
	void Debug(std::wstring_view format, std::wformat_args args);
}
// メッセージを表示する
template<class... Args>
static inline void Notice(UINT id, const Args&... args) {
	detail::Notice(id, std::make_wformat_args(args...));
}
// デバッグメッセージを表示する
template<class... Args>
static inline void Debug(std::wstring_view format, const Args&... args) {
	detail::Debug(format, std::make_wformat_args(args...));
}
void Error(std::wstring_view functionName, int lastError = GetLastError());
static inline void WSAError(std::wstring_view functionName, int lastError = WSAGetLastError()) { Error(functionName, lastError); }

/*===== hostman.c =====*/

int SelectHost(int Type);
int AddHostToList(HOSTDATA *Set, int Pos, int Level);
int CopyHostFromList(int Num, HOSTDATA *Set);
int CopyHostFromListInConnect(int Num, HOSTDATA *Set);
int SetHostBookMark(int Num, std::vector<std::wstring>&& bookmark);
std::optional<std::vector<std::wstring>> AskHostBookMark(int Num);
int SetHostDir(int Num, std::wstring_view LocDir, std::wstring_view HostDir);
int SetHostPassword(int Num, std::wstring const& Pass);
int SetHostSort(int Num, int LFSort, int LDSort, int RFSort, int RDSort);
void DecomposeSortType(uint32_t Sort, int *LFSort, int *LDSort, int *RFSort, int *RDSort);
int AskCurrentHost(void);
void SetCurrentHost(int Num);
void CopyDefaultHost(HOSTDATA *Set);
// ホスト共通設定機能
void ResetDefaultHost(void);
void SetDefaultHost(HOSTDATA *Set);
int SearchHostName(std::wstring_view name);
void ImportFromWSFTP(void);
// 暗号化通信対応
int SetHostEncryption(int Num, int UseNoEncryption, int UseFTPES, int UseFTPIS, int UseSFTP);

/*===== connect.c =====*/

void ConnectProc(int Type, int Num);
void QuickConnectProc(void);
void DirectConnectProc(std::wstring&& unc, int Kanji, int Kana, int Fkanji, int TrMode);
void HistoryConnectProc(int MenuCmd);
std::wstring_view AskHostAdrs();
int AskHostPort(void);
int AskHostNameKanji(void);
int AskHostNameKana(void);
int AskListCmdMode(void);
int AskUseNLST_R(void);
std::wstring AskHostChmodCmd();
int AskHostTimeZone(void);
int AskPasvMode(void);
std::wstring AskHostLsName();
int AskHostType(void);
int AskHostFireWall(void);
int AskNoFullPathMode(void);
void SaveCurrentSetToHost(void);
int ReConnectCmdSkt(void);
// int ReConnectTrnSkt(void);
// 同時接続対応
int ReConnectTrnSkt(SOCKET *Skt, int *CancelCheckWork);
SOCKET AskCmdCtrlSkt(void);
SOCKET AskTrnCtrlSkt(void);
void SktShareProh(void);
int AskShareProh(void);
void DisconnectProc(void);
void DisconnectSet(void);
int AskConnecting(void);
#if defined(HAVE_TANDEM)
int AskRealHostType(void);
int SetOSS(int wkOss);
int AskOSS(void);
#endif
std::optional<sockaddr_storage> SocksReceiveReply(SOCKET s, int* CancelCheckWork);
SOCKET connectsock(std::wstring&& host, int port, UINT prefixId, int *CancelCheckWork);
SOCKET GetFTPListenSocket(SOCKET ctrl_skt, int *CancelCheckWork);
int AskTryingConnect(void);
int AskUseNoEncryption(void);
int AskUseFTPES(void);
int AskUseFTPIS(void);
int AskUseSFTP(void);
// 同時接続対応
int AskMaxThreadCount(void);
int AskReuseCmdSkt(void);
// FEAT対応
int AskHostFeature(void);
// MLSD対応
int AskUseMLSD(void);
// IPv6対応
int AskCurNetType(void);
// 自動切断対策
int AskNoopInterval(void);
// 再転送対応
int AskTransferErrorMode(void);
int AskTransferErrorNotify(void);
// セッションあたりの転送量制限対策
int AskErrorReconnect(void);
// ホスト側の設定ミス対策
int AskNoPasvAdrs(void);

// キャッシュのファイル名を作成する
static inline fs::path MakeCacheFileName(int num) {
	return tempDirectory() / std::format(L"_ffftp.{:03d}"sv, num);
}

/*===== ftpproc.c =====*/

void DownloadProc(int ChName, int ForceFile, int All);
void DirectDownloadProc(std::wstring_view Fname);
void MirrorDownloadProc(int Notify);
void UploadListProc(int ChName, int All);
void UploadDragProc(WPARAM wParam);
void MirrorUploadProc(int Notify);
void DeleteProc(void);
void RenameProc(void);
void MoveRemoteFileProc(int);
void MkdirProc(void);
void ChangeDirComboProc(HWND hWnd);
void ChangeDirBmarkProc(int MarkID);
void ChangeDirDirectProc(int Win);
void ChangeDirDropFileProc(WPARAM wParam);
void ChmodProc(void);
std::optional<std::wstring> ChmodDialog(std::wstring const& text);
void SomeCmdProc(void);
void CalcFileSizeProc(void);
void DispCWDerror(HWND hWnd);
void CopyURLtoClipBoard(void);
int ProcForNonFullpath(SOCKET cSkt, std::wstring& Path, std::wstring& CurDir, HWND hWnd, int* CancelCheckWork);
#if defined(HAVE_OPENVMS)
std::wstring ReformVMSDirName(std::wstring&& dirName);
#endif
// 自動切断対策
void NoopProc(int Force);
// 同時接続対応
void AbortRecoveryProc(void);
void ReconnectProc(void);

/*===== local.c =====*/

bool DoLocalCWD(fs::path const& path);
void DoLocalMKD(fs::path const& path);
fs::path DoLocalPWD();
void DoLocalRMD(fs::path const& path);
void DoLocalDELE(fs::path const& path);
void DoLocalRENAME(fs::path const& src, fs::path const& dst);

/*===== remote.c =====*/

int DoCWD(std::wstring const& Path, int Disp, int ForceGet, int ErrorBell);
int DoCWDStepByStep(std::wstring const& Path, std::wstring const& Cur);
int DoMKD(std::wstring const& Path);
void InitPWDcommand();
int DoRMD(std::wstring const& path);
int DoDELE(std::wstring const& path);
int DoRENAME(std::wstring const& from, std::wstring const& to);
int DoCHMOD(std::wstring const& path, std::wstring const& mode);
int DoSIZE(SOCKET cSkt, std::wstring const& Path, LONGLONG *Size, int *CancelCheckWork);
int DoMDTM(SOCKET cSkt, std::wstring const& Path, FILETIME *Time, int *CancelCheckWork);
int DoMFMT(SOCKET cSkt, std::wstring const&, FILETIME *Time, int *CancelCheckWork);
int DoQUOTE(SOCKET cSkt, std::wstring_view CmdStr, int* CancelCheckWork);
SOCKET DoClose(SOCKET Sock);
// 同時接続対応
//int DoQUIT(SOCKET ctrl_skt);
int DoQUIT(SOCKET ctrl_skt, int *CancelCheckWork);
int DoDirList(std::wstring_view AddOpt, int Num, int* CancelCheckWork);
#if defined(HAVE_TANDEM)
void SwitchOSSProc(void);
#endif
namespace detail {
	std::tuple<int, std::wstring> command(SOCKET cSkt, int* CancelCheckWork, std::wstring&& cmd);
}
template<class... Args>
static inline std::tuple<int, std::wstring> Command(SOCKET socket, int* CancelCheckWork, std::wstring_view format, const Args&... args) {
	return socket == INVALID_SOCKET ? std::tuple{ 429, L""s } : detail::command(socket, CancelCheckWork, std::format(format, args...));
}
std::tuple<int, std::wstring> ReadReplyMessage(SOCKET cSkt, int *CancelCheckWork);
int ReadNchar(SOCKET cSkt, char *Buf, int Size, int *CancelCheckWork);

/*===== getput.c =====*/

int MakeTransferThread(void);
void CloseTransferThread(void);
// 同時接続対応
void AbortAllTransfer();
int AddTmpTransFileList(TRANSPACKET const& item, std::forward_list<TRANSPACKET>& list);
int RemoveTmpTransFileListItem(std::forward_list<TRANSPACKET>& list, int Num);

void AddTransFileList(TRANSPACKET *Pkt);
// バグ対策
void AddNullTransFileList();
void AppendTransFileList(std::forward_list<TRANSPACKET>&& list);
void KeepTransferDialog(int Sw);
int AskTransferNow(void);
int AskTransferFileNum(void);
void GoForwardTransWindow(void);
void InitTransCurDir(void);
int DoDownload(SOCKET cSkt, TRANSPACKET& item, int DirList, int *CancelCheckWork);
int CheckPathViolation(TRANSPACKET const& item);
// タスクバー進捗表示
LONGLONG AskTransferSizeLeft(void);
LONGLONG AskTransferSizeTotal(void);
int AskTransferErrorDisplay(void);
// ゾーンID設定追加
int LoadZoneID();
void FreeZoneID();
int IsZoneIDLoaded();
bool MarkFileAsDownloadedFromInternet(fs::path const& path);

/*===== codecnv.c =====*/

class CodeDetector {
	int utf8 = 0;
	int sjis = 0;
	int euc = 0;
	int jis = 0;
	bool nfc = false;
	bool nfd = false;
public:
	void Test(std::string_view str);
	int result() const {
		Debug(L"CodeDetector::result(): utf8 {}, sjis {}, euc {}, jis {}, nfc {}, nfd {}"sv, utf8, sjis, euc, jis, nfc, nfd);
		auto [_, id] = std::max<std::tuple<int, int>>({
			{ utf8, KANJI_UTF8N },
			{ sjis, KANJI_SJIS },
			{ euc, KANJI_EUC },
			{ jis, KANJI_JIS },
			}, [](auto const& l, auto const& r) { return std::get<0>(l) < std::get<0>(r); });
		return id == KANJI_UTF8N && nfd ? KANJI_UTF8HFSX : id;
	}
};

class CodeConverter {
	const int incode;
	const int outcode;
	const bool kana;
	bool first = true;
	std::string rest;
public:
	CodeConverter(int incode, int outcode, bool kana) : incode{ outcode == KANJI_NOCNV || incode == outcode && !kana ? KANJI_NOCNV : incode }, outcode{ outcode }, kana{ kana } {}
	std::string Convert(std::string_view input);
};

std::string ToCRLF(std::string_view source);
std::wstring ConvertFrom(std::string_view str, int kanji);
std::string ConvertTo(std::wstring_view str, int kanji, int kana);

/*===== option.c =====*/

void SetOption();
int SortSetting(void);
// hostman.cで使用
int GetDecimalText(HWND hDlg, int Ctrl);
void CheckRange2(int *Cur, int Max, int Min);

/*===== bookmark.c =====*/

void ClearBookMark();
void AddCurDirToBookMark(int Win);
std::tuple<std::wstring, std::wstring> AskBookMarkText(size_t MarkID);
void SaveBookMark();
void LoadBookMark();
void EditBookMark();

/*===== registry.c =====*/

void SaveRegistry();
bool LoadRegistry();
void ClearRegistry();
// ポータブル版判定
void ClearIni(void);
void SetMasterPassword( const char* );
// セキュリティ強化
void GetMasterPassword(char*);
int GetMasterPasswordStatus(void);
int ValidateMasterPassword(void);
void SaveSettingsToFile(void);
int LoadSettingsFromFile(void);
// ポータブル版判定
int IsRegAvailable();
int IsIniAvailable();
// バージョン確認
int ReadSettingsVersion();
// FileZilla XML形式エクスポート対応
void SaveSettingsToFileZillaXml();
// WinSCP INI形式エクスポート対応
void SaveSettingsToWinSCPIni();

/*===== lvtips.c =====*/

int InitListViewTips();
void DeleteListViewTips(void);
void EraseListViewTips(void);
HWND GetListViewTipsHwnd(void);
void CheckTipsDisplay(HWND hWnd, LPARAM lParam);

/*===== ras.c =====*/

void DisconnectRas(bool confirm);
void SetRasEntryToComboBox(HWND hdlg, int item, std::wstring const& selected);
bool ConnectRas(bool dialup, bool explicitly, bool confirm, std::wstring const& name);

/*===== misc.c =====*/

std::wstring SetSlashTail(std::wstring&& path);
void ReplaceAll(char *Str, char Src, char Dst);
std::wstring ReplaceAll(std::wstring&& str, wchar_t from, wchar_t to);
std::wstring_view GetFileName(std::wstring_view path);
std::wstring MakeSizeString(double size);
void DispStaticText(HWND hWnd, std::wstring text);
void RectClientToScreen(HWND hWnd, RECT *Rect);
fs::path SelectFile(bool open, HWND hWnd, UINT titleId, const wchar_t* initialFileName, const wchar_t* extension, std::initializer_list<FileType> fileTypes);
fs::path SelectDir(HWND hWnd);
fs::path MakeDistinguishableFileName(fs::path&& path);
#if defined(HAVE_TANDEM)
void CalcExtentSize(TRANSPACKET *Pkt, LONGLONG Size);
#endif
int CalcPixelX(int x);
int CalcPixelY(int y);

/*===== opie.c =====*/

std::string Make6WordPass(int seq, std::string_view seed, std::string_view pass, int type);

/*===== tool.c =====*/

void OtpCalcTool(void);
// FTPS対応
void TurnStatefulFTPFilter();

/*===== history.c =====*/

void AddHostToHistory(Host const& host);
void AddHistoryToHistory(HISTORYDATA const& history);
void SetAllHistoryToMenu();
std::optional<HISTORYDATA> GetHistoryByCmd(int menuId);
std::vector<HISTORYDATA> const& GetHistories();

/*===== socket.c =====*/

BOOL LoadSSL();
void FreeSSL();
void ShowCertificate();
BOOL AttachSSL(SOCKET s, SOCKET parent, BOOL* pbAborted, std::wstring_view ServerName);
bool IsSecureConnection();
BOOL IsSSLAttached(SOCKET s);
int MakeSocketWin();
void DeleteSocketWin(void);
void SetAsyncTableData(SOCKET s, std::variant<sockaddr_storage, std::tuple<std::wstring, int>> const& target);
void SetAsyncTableDataMapPort(SOCKET s, int Port);
int GetAsyncTableData(SOCKET s, std::variant<sockaddr_storage, std::tuple<std::wstring, int>>& target);
int GetAsyncTableDataMapPort(SOCKET s, int* Port);
SOCKET do_socket(int af, int type, int protocol);
int do_connect(SOCKET s, const struct sockaddr *name, int namelen, int *CancelCheckWork);
int do_closesocket(SOCKET s);
int do_listen(SOCKET s,	int backlog);
SOCKET do_accept(SOCKET s, struct sockaddr *addr, int *addrlen);
int do_recv(SOCKET s, char *buf, int len, int flags, int *TimeOut, int *CancelCheckWork);
int SendData(SOCKET s, const char* buf, int len, int flags, int* CancelCheckWork);
// 同時接続対応
void RemoveReceivedData(SOCKET s);
// UPnP対応
int LoadUPnP();
void FreeUPnP();
int IsUPnPLoaded();
std::optional<std::wstring> AddPortMapping(std::wstring const& internalAddress, int port);
bool RemovePortMapping(int port);
int CheckClosedAndReconnect(void);
// 同時接続対応
int CheckClosedAndReconnectTrnSkt(SOCKET *Skt, int *CancelCheckWork);


extern int DebugConsole;
extern int DispIgnoreHide;

template<class Target, class Source>
constexpr auto data_as(Source& source) {
	return reinterpret_cast<Target*>(std::data(source));
}
template<class Target, class Source>
constexpr auto data_as(Source const& source) {
	return reinterpret_cast<const Target*>(std::data(source));
}
template<class Size, class Source>
constexpr auto size_as(Source const& source) {
	return static_cast<Size>(std::size(source));
}
template<class T, class Allocator>
constexpr auto before_end(std::forward_list<T, Allocator>& list) {
	for (auto it = list.before_begin();;)
		if (auto prev = it++; it == end(list))
			return prev;
}
template<class DstChar, class SrcChar, class Fn>
static inline auto convert(Fn&& fn, std::basic_string_view<SrcChar> src) {
	auto len1 = fn(data(src), size_as<int>(src), nullptr, 0);
	std::basic_string<DstChar> dst(len1, 0);
	auto len2 = fn(data(src), size_as<int>(src), data(dst), len1);
	dst.resize(len2);
	return dst;
}
template<class DstChar, class Fn>
static inline auto convert(Fn&& fn, std::string_view src) {
	return convert<DstChar, char>(std::forward<Fn>(fn), src);
}
template<class DstChar, class Fn>
static inline auto convert(Fn&& fn, std::wstring_view src) {
	return convert<DstChar, wchar_t>(std::forward<Fn>(fn), src);
}
template<class Char, class... Str>
static inline auto concat(std::basic_string_view<Char> first, Str&&... rest) {
	std::basic_string<Char> result;
	result.reserve((std::size(first) + ... + std::size(rest)));
	((result += first) += ... += rest);
	return result;
}
static inline auto operator+(std::string_view left, std::string_view right) {
	return concat(left, right);
}
static inline auto operator+(std::wstring_view left, std::wstring_view right) {
	return concat(left, right);
}
static inline auto sv(auto const& sm) {
	return std::basic_string_view{ sm.first, sm.second };
}
static inline auto u8(std::string_view utf8) {
	return convert<wchar_t>([](auto src, auto srclen, auto dst, auto dstlen) { return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, srclen, dst, dstlen); }, utf8);
}
static inline auto u8(std::wstring_view wide) {
	return convert<char>([](auto src, auto srclen, auto dst, auto dstlen) { return WideCharToMultiByte(CP_UTF8, 0, src, srclen, dst, dstlen, nullptr, nullptr); }, wide);
}
template<class Char>
static inline auto u8(const Char* str, size_t len) {
	return u8(std::basic_string_view<Char>{ str, len });
}
static auto ieq(std::string_view left, std::string_view right) {
	return std::equal(begin(left), end(left), begin(right), end(right), [](auto const l, auto const r) { return std::toupper(l) == std::toupper(r); });
}
static auto ieq(std::wstring_view left, std::wstring_view right) {
	return std::equal(begin(left), end(left), begin(right), end(right), [](auto const l, auto const r) { return std::towupper(l) == std::towupper(r); });
}
static inline auto lc(std::string&& str) {
	_strlwr(data(str));
	return str;
}
static inline auto lc(std::wstring&& str) {
	_wcslwr(data(str));
	return str;
}
template<class String>
static inline auto lc(String const& src) {
	return lc(std::basic_string(std::begin(src), std::end(src)));
}
static inline auto uc(std::wstring&& str) {
	_wcsupr(data(str));
	return str;
}
static inline auto uc(std::wstring_view sv) {
	return uc(std::wstring{ sv });
}
template<class Char, class Evaluator>
static inline auto replace(std::basic_string_view<Char> input, boost::basic_regex<Char> const& pattern, Evaluator&& evaluator) {
	std::basic_string<Char> replaced;
	auto last = data(input);
	for (boost::regex_iterator<const Char*> it{ data(input), data(input) + size(input), pattern }, end; it != end; ++it) {
		replaced.append(last, (*it)[0].first);
		replaced += evaluator(*it);
		last = (*it)[0].second;
	}
	replaced.append(last, data(input) + size(input));
	return replaced;
}
namespace detail {
	static inline int vscprintf(char const* const format, va_list args) { return _vscprintf_p(format, args); }
	static inline int vscprintf(wchar_t const* const format, va_list args) { return _vscwprintf_p(format, args); }
	static inline int vsprintf(_Out_writes_(size) _Post_z_ char* const buffer, size_t const size, char const* const format, va_list args) { return _vsprintf_p(buffer, size, format, args); }
	static inline int vsprintf(_Out_writes_(size) _Post_z_ wchar_t* const buffer, size_t const size, wchar_t const* const format, va_list args) { return _vswprintf_p(buffer, size, format, args); }
	template<class Char>
	static inline auto vstrprintf(int capacity, Char const* const format, va_list args) {
		if (capacity == -1)
			capacity = vscprintf(format, args);
		assert(0 < capacity);
		std::basic_string buffer(capacity, Char{});
		int length = vsprintf(data(buffer), (size_t)capacity + 1, format, args);
		assert(length <= capacity);
		buffer.resize(length);
		return buffer;
	}
}
template<int capacity = -1, class Char>
static inline auto strprintf(_In_z_ _Printf_format_string_ Char const* const format, ...) {
	va_list args;
	va_start(args, format);
	auto result = detail::vstrprintf(capacity, format, args);
	va_end(args);
	return result;
}
template<int captionId = IDS_APP>
static inline auto Message(HWND owner, int textId, DWORD style) {
	MSGBOXPARAMSW msgBoxParams{ sizeof MSGBOXPARAMSW, owner, GetFtpInst(), MAKEINTRESOURCEW(textId), MAKEINTRESOURCEW(captionId), style, nullptr, 0, nullptr, LANG_NEUTRAL };
	return MessageBoxIndirectW(&msgBoxParams);
}
template<int captionId = IDS_APP>
static inline auto Message(int textId, DWORD style) {
	return Message<captionId>(GetMainHwnd(), textId, style);
}
static auto GetString(UINT id) {
	wchar_t buffer[1024];
	auto length = LoadStringW(GetFtpInst(), id, buffer, size_as<int>(buffer));
	return std::wstring(buffer, length);
}
static auto GetText(HWND hwnd) {
	std::wstring text;
	if (auto const length = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0); 0 < length) {
		text.resize(length);
		auto const result = SendMessageW(hwnd, WM_GETTEXT, length + 1, (LPARAM)text.data());
		text.resize(result);
	}
	return text;
}
static inline auto GetText(HWND hdlg, int id) {
	return GetText(GetDlgItem(hdlg, id));
}
static inline void SetText(HWND hwnd, const wchar_t* text) {
	SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)text);
}
static inline void SetText(HWND hdlg, int id, const wchar_t* text) {
	SendDlgItemMessageW(hdlg, id, WM_SETTEXT, 0, (LPARAM)text);
}
static inline void SetText(HWND hwnd, const std::wstring& text) {
	SetText(hwnd, text.c_str());
}
static inline void SetText(HWND hdlg, int id, const std::wstring& text) {
	SetText(hdlg, id, text.c_str());
}
static inline auto AddressPortToString(const SOCKADDR* sa, size_t salen) {
	std::wstring string(sizeof "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff%4294967295]:65535" - 1, L'\0');
	auto length = size_as<DWORD>(string) + 1;
	auto result = WSAAddressToStringW(const_cast<SOCKADDR*>(sa), static_cast<DWORD>(salen), nullptr, data(string), &length);
	assert(result == 0);
	string.resize(length - 1);
	return string;
}
template<class SockAddr>
static inline auto AddressPortToString(const SockAddr* sa, size_t salen = sizeof(SockAddr)) {
	static_assert(std::is_same_v<SockAddr, sockaddr_in> || std::is_same_v<SockAddr, sockaddr_in6> || std::is_same_v<SockAddr, sockaddr_storage>);
	return AddressPortToString(reinterpret_cast<const SOCKADDR*>(sa), salen);
}
static inline auto AddressToString(sockaddr_storage const& sa) {
	if (sa.ss_family == AF_INET) {
		auto local = reinterpret_cast<sockaddr_in const&>(sa);
		local.sin_port = 0;
		return AddressPortToString(&local);
	} else {
		auto local = reinterpret_cast<sockaddr_in6 const&>(sa);
		local.sin6_port = 0;
		local.sin6_scope_id = 0;
		return AddressPortToString(&local);
	}
}
static inline auto IdnToAscii(std::wstring_view unicode) {
	if (empty(unicode))
		return L""s;
	return convert<wchar_t>([](auto src, auto srclen, auto dst, auto dstlen) { return IdnToAscii(0, src, srclen, dst, dstlen); }, unicode);
}
static inline auto NormalizeString(NORM_FORM form, std::wstring_view src) {
	if (empty(src))
		return std::wstring{ src };
	return convert<wchar_t>([form](auto src, auto srclen, auto dst, auto dstlen) { return NormalizeString(form, src, srclen, dst, dstlen); }, src);
}
static inline auto InputDialog(int dialogId, HWND parent, UINT titleId, std::wstring& text, size_t maxlength = 0, int* flag = nullptr, int helpTopicId = IDH_HELP_TOPIC_0000001) {
	struct Data {
		using result_t = bool;
		UINT titleId;
		std::wstring& text;
		size_t maxlength;
		int* flag;
		int helpTopicId;
		Data(UINT titleId, std::wstring& text, size_t maxlength, int* flag, int helpTopicId) : titleId{ titleId }, text{ text }, maxlength{ maxlength }, flag{ flag }, helpTopicId{ helpTopicId } {}
		INT_PTR OnInit(HWND hDlg) {
			if (titleId != 0)
				SetText(hDlg, GetString(titleId));
			SendDlgItemMessageW(hDlg, INP_INPSTR, EM_LIMITTEXT, maxlength - 1, 0);
			SetText(hDlg, INP_INPSTR, text);
			if (flag)
				SendDlgItemMessageW(hDlg, INP_ANONYMOUS, BM_SETCHECK, *flag, 0);
			return TRUE;
		}
		void OnCommand(HWND hDlg, WORD id) {
			switch (id) {
			case IDOK:
				text = GetText(hDlg, INP_INPSTR);
				if (flag)
					*flag = (int)SendDlgItemMessageW(hDlg, INP_ANONYMOUS, BM_GETCHECK, 0, 0);
				EndDialog(hDlg, true);
				break;
			case IDCANCEL:
				EndDialog(hDlg, false);
				break;
			case IDHELP:
				ShowHelp(helpTopicId);
				break;
			case INP_BROWSE:
				if (auto path = SelectDir(hDlg); !path.empty())
					SetText(hDlg, INP_INPSTR, path);
				break;
			}
		}
	};
	return Dialog(GetFtpInst(), dialogId, parent, Data{ titleId, text, maxlength, flag, helpTopicId });
}
struct ProcessInformation : PROCESS_INFORMATION {
	ProcessInformation() : PROCESS_INFORMATION{ INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE } {}
	~ProcessInformation() {
		if (hThread != INVALID_HANDLE_VALUE)
			CloseHandle(hThread);
		if (hProcess != INVALID_HANDLE_VALUE)
			CloseHandle(hProcess);
	}
};
template<class Fn>
static inline void GetDrives(Fn&& fn) {
	auto drives = GetLogicalDrives();
	DWORD nodrives = 0;
	DWORD size = sizeof(DWORD);
	SHRegGetUSValueW(LR"(Software\Microsoft\Windows\CurrentVersion\Policies\Explorer)", L"NoDrives", nullptr, &nodrives, &size, false, nullptr, 0);
	for (int i = 0; i < sizeof(DWORD) * 8; i++)
		if ((drives & 1 << i) != 0 && (nodrives & 1 << i) == 0) {
			wchar_t drive[] = { wchar_t(L'A' + i), L':', L'\\', 0 };
			fn(drive);
		}
}
static auto GetErrorMessage(int lastError) {
	wchar_t* buffer;
	auto length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr, lastError, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
	std::wstring message(buffer, buffer + length);
	LocalFree(buffer);
	return std::move(message);
}

template<class Fn>
static inline std::invoke_result_t<Fn, BCRYPT_ALG_HANDLE> BCrypt(LPCWSTR algid, Fn&& fn) {
	BCRYPT_ALG_HANDLE alg;
	if (auto status = BCryptOpenAlgorithmProvider(&alg, algid, nullptr, 0); status != STATUS_SUCCESS) {
		Debug(L"BCryptOpenAlgorithmProvider({}) failed: 0x{:08X}."sv, algid, status);
		return {};
	}
	auto result = std::invoke(std::forward<Fn>(fn), alg);
	BCryptCloseAlgorithmProvider(alg, 0);
	return result;
}
template<class Fn>
static inline auto HashOpen(LPCWSTR algid, Fn&& fn) {
	return BCrypt(algid, [fn](BCRYPT_ALG_HANDLE alg) -> std::invoke_result_t<Fn, BCRYPT_ALG_HANDLE, std::vector<UCHAR>&&, std::vector<UCHAR>&&> {
		DWORD objlen, hashlen, resultlen;
		if (auto status = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objlen), sizeof objlen, &resultlen, 0); status != STATUS_SUCCESS || resultlen != sizeof objlen) {
			Debug(L"BCryptGetProperty({}) failed: 0x{:08X} or invalid length: {}."sv, BCRYPT_OBJECT_LENGTH, status, resultlen);
			return {};
		}
		if (auto status = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashlen), sizeof hashlen, &resultlen, 0); status != STATUS_SUCCESS || resultlen != sizeof hashlen) {
			Debug(L"BCryptGetProperty({}) failed: 0x{:08X} or invalid length: {}."sv, BCRYPT_HASH_LENGTH, status, resultlen);
			return {};
		}
		return std::invoke(fn, alg, std::vector<UCHAR>(objlen), std::vector<UCHAR>(hashlen));
	});
}
template<class... Ranges>
static inline auto HashData(BCRYPT_ALG_HANDLE alg, std::vector<UCHAR>& obj, std::vector<UCHAR>& hash, Ranges const&... ranges) {
	NTSTATUS status;
	if (BCRYPT_HASH_HANDLE handle; (status = BCryptCreateHash(alg, &handle, data(obj), size_as<ULONG>(obj), nullptr, 0, 0)) == STATUS_SUCCESS) {
		for (auto bytes : { std::as_bytes(std::span{ ranges }.subspan(0))... })
			if ((status = BCryptHashData(handle, const_cast<PUCHAR>(data_as<const UCHAR>(bytes)), size_as<ULONG>(bytes), 0)) != STATUS_SUCCESS) {
				Debug(L"{}() failed: 0x{:08X}."sv, L"BCryptHashData"sv, status);
				break;
			}
		if (status == STATUS_SUCCESS && (status = BCryptFinishHash(handle, data(hash), size_as<DWORD>(hash), 0)) != STATUS_SUCCESS)
			Debug(L"{}() failed: 0x{:08X}."sv, L"BCryptFinishHash"sv, status);
		BCryptDestroyHash(handle);
	} else
		Debug(L"{}() failed: 0x{:08X}."sv, L"BCryptCreateHash"sv, status);
	return status == STATUS_SUCCESS;
}

FILELIST::FILELIST(std::string_view original, char node, char link, int64_t size, int attr, FILETIME time, std::string_view owner, char infoExist) : Original{ original }, Node{ node }, Link{ link }, Size{ size }, Attr{ attr }, Time{ time }, Owner{ u8(owner) }, InfoExist{ infoExist } {}
