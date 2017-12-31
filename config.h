
/* OPENVMS用のコードを有効にする。樋口殿作成のパッチを組み込みました。 */
#define HAVE_OPENVMS

// 全体に影響する設定はここに記述する予定
// 使用するCPUを1個に限定する（マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策）
//#define DISABLE_MULTI_CPUS
// ファイル転送用のネットワークバッファを無効にする（通信中止後にリモートのディレクトリが表示されないバグ対策）
//#define DISABLE_TRANSFER_NETWORK_BUFFERS
// コントロール用のネットワークバッファを無効にする（フリーズ対策）
#define DISABLE_CONTROL_NETWORK_BUFFERS
// JRE32.DLLを無効にする（UTF-8に非対応のため）
#define DISABLE_JRE32DLL
// ファイル転送の同時接続数
#define MAX_DATA_CONNECTION 4
// 現在のコードページをShift_JISで置換する
#define FORCE_SJIS_ON_ACTIVE_CODE_PAGE
// UTF-8 UTF-16 LE間の変換処理でWindows XPのエミュレーションを行う
#define EMULATE_UTF8_WCHAR_CONVERSION

/* HP NonStop Server 用のコードを有効にする */
#define HAVE_TANDEM
