
/* OPENVMS用のコードを有効にする。樋口殿作成のパッチを組み込みました。 */
#define HAVE_OPENVMS

//全体に影響する設定はここに記述する予定
//内部をUTF-8として扱いマルチバイト文字ワイド文字APIラッパーを使用する
#include "mbswrapper.h"
//使用するCPUを1個に限定する（マルチコアCPUの特定環境下でファイル通信中にクラッシュするバグ対策）
#define DISABLE_MULTI_CPUS
//ネットワークバッファを無効にする（通信中止後にリモートのディレクトリが表示されないバグ対策）
//#define DISABLE_NETWORK_BUFFERS

