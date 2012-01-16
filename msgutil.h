
// 指定した文字列の UTF-8 バイナリ文字列の先頭アドレスを取得する u8("文字列リテラル") の形で呼び出す
#define u8(x)	MessageUtil_GetUTF8StaticBinaryBlock(L ## x, sizeof(L ## x) / sizeof(wchar_t))

/**
 * UTF-8文字列バイナリを取得する。取得した文字列のアドレスはアプリケーション終了まで有効。
 * 失敗した場合は "" が返る。
 * 必ず u8マクロと組み合わせて、u8("文字列リテラル") の形で呼び出す。引数に文字列変数を渡した場合の動作は不定。
 * 
 * @param[in] ws 文字列
 * @param[in] ws_area_length 文字列の長さ。終端NULL文字を含んだ値であること。ws: "" のとき、countof_ws: 1
 * @return wsで表される文字列のUTF8バイナリの先頭アドレス
 */
const char* const MessageUtil_GetUTF8StaticBinaryBlock(const wchar_t* const ws, size_t ws_area_length);

/**
 * UTF-8文字列群の文字領域を破棄する.
 * 
 * MessageUtil_GetUTF8StaticBinaryBlock()で確保した文字列領域をすべて開放する。アプリケーション終了時に呼び出すこと.
 */
void MessageUtil_FreeUTF8StaticBinaryBlocks();
