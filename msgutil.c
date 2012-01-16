
#include <windows.h>

/**
 * アドレス固定のWide文字列とそれに対応するUTF-8文字列を格納する構造体
 */
typedef struct {
	const wchar_t *ws;
	char *u8s;
	size_t u8size;
} StrPair;

static BOOL g_initialized = FALSE;
static CRITICAL_SECTION g_msgUtilLLock;

// Wide文字列 - UTF-8文字列 対応Map
static StrPair *pStrMap = NULL;

//! 現在有効なStrPairの数
static int strMapCount = 0;

//! 確保済みのStrPairの領域数
static int strMapMaxCount = 0;

/**
 * staticなWide文字列に対応するUTF-8バイナリ文字列領域を確保し、その先頭アドレスを返す
 */
const char* const MessageUtil_GetUTF8StaticBinaryBlock(const wchar_t* const ws, size_t ws_area_length)
{
	int i = 0;
	int wcsCount = 0;
	int newSize = 0;
	const char *pResult = NULL;
	if (!g_initialized)
	{
		InitializeCriticalSection(&g_msgUtilLLock);
		g_initialized = TRUE;
	}
	EnterCriticalSection(&g_msgUtilLLock);
	for (i = 0; i < strMapCount; i++)
	{
		if (pStrMap[i].ws == ws)
		{
			pResult = pStrMap[i].u8s;
			break;
		}
	}
	if (pResult == NULL)
	{
		if (strMapMaxCount < strMapCount + 1)
		{
			// 領域が足りなくなったので追加する
			if (strMapMaxCount == 0)
			{
				strMapMaxCount = 100;
				pStrMap = (StrPair*)malloc(sizeof(StrPair) * strMapMaxCount);
			}
			else
			{
				strMapMaxCount += 100;
				pStrMap = (StrPair*)realloc(pStrMap, sizeof(StrPair) * strMapMaxCount);
			}
		}
		newSize = WideCharToMultiByte(CP_UTF8, 0, ws, ws_area_length, 0, 0, NULL, NULL);
		if (newSize > 0)
		{
			int index = strMapCount;
			char *beginPos = 0;
			int postSize = 0;
			strMapCount++;
			pStrMap[index].ws = ws;
			pStrMap[index].u8size = newSize;
			beginPos = (char*)malloc(newSize);
			pStrMap[index].u8s = beginPos;
			postSize = WideCharToMultiByte(CP_UTF8, 0, ws, ws_area_length, beginPos, newSize, NULL, NULL);
			pResult = beginPos;
		}
		else
		{
			static char sEmpty[] = "";
			pResult = sEmpty;
		}
	}
	LeaveCriticalSection(&g_msgUtilLLock);
	return pResult;
}

/**
 * MessageUtil_GetUTF8StaticBinaryBlock() で確保した領域をすべて破棄する
 */
void MessageUtil_FreeUTF8StaticBinaryBlocks()
{
	int i = 0;
	if (!g_initialized)
	{
		InitializeCriticalSection(&g_msgUtilLLock);
		g_initialized = TRUE;
	}
	EnterCriticalSection(&g_msgUtilLLock);
	for (i = 0; i < strMapCount; i++)
	{
		free(pStrMap[i].u8s);
	}
	if (pStrMap)
	{
		free(pStrMap);
		pStrMap = (StrPair*)NULL;
	}
	LeaveCriticalSection(&g_msgUtilLLock);
}
