/**************************************************************************

	OleDragDrop.h

	(C) Copyright 1996-2002 By Tomoaki Nakashima. All right reserved.	
		http://www.nakka.com/
		nakka@nakka.com

**************************************************************************/

#ifndef _INC_OLEDRAGDROP
#define _INC_OLEDRAGDROP

#define IDROPTARGET_NOTIFY_DRAGENTER	0
#define IDROPTARGET_NOTIFY_DRAGOVER		1
#define IDROPTARGET_NOTIFY_DRAGLEAVE	2
#define IDROPTARGET_NOTIFY_DROP 		3

typedef struct _IDROPTARGET_NOTIFY{
	POINTL *ppt;					//マウスの位置
	DWORD dwEffect;					//ドラッグ操作で、ドラッグされる対象で許される効果
	DWORD grfKeyState;				//キーの状態
	UINT cfFormat;					//ドロップされるデータのクリップボードフォーマット
	HANDLE hMem;					//ドロップされるデータ
	LPVOID pdo;						//IDataObject
}IDROPTARGET_NOTIFY , *LPIDROPTARGET_NOTIFY;




//DragTarget
BOOL APIPRIVATE OLE_IDropTarget_RegisterDragDrop(HWND hWnd, UINT uCallbackMessage, UINT *cFormat, int cfcnt);

//ドラッグ&ドロップのターゲットとして登録します。

//[引数]
//	ドラッグ&ドロップ操作が行われたときに指定のウィンドウの指定のメッセージに通知されます。
//	wParam に操作の種類(IDROPTARGET_NOTIFY_)が設定されています。
//	lParam に IDROPTARGET_NOTIFY 構造体へのポインタが設定されています。

//	cFormat は 受け取ることが可能なクリップボードフォーマットのリストを指定します。
//	cfcnt はクリップボードフォーマットの配列の要素数を指定します。

void APIPRIVATE OLE_IDropTarget_RevokeDragDrop(HWND hWnd);

//ドラッグ＆ドロップのターゲットを解除します。




//DropSource
int APIPRIVATE OLE_IDropSource_Start(HWND hWnd, UINT uCallbackMessage, UINT uCallbackDragOverMessage, UINT *ClipFormtList, int cfcnt, int Effect);

//ドラッグ＆ドロップを開始するときに指定します。
//ドラッグ＆ドロップ操作は自動的に行われますが、データが必要な時は、指定のウィンドウメッセージでデータ要求を行います。

//[引数]
//	hWnd に uCallbackMessage を送ってデータの要求を行います。
//	この時 wParam に要求するクリップボードフォーマットの値が入っています。
//	プログラムは *(HANDLE *)lParam にデータを設定して返します。(NULLでも可)

//	ClipFormtList はサポートしているクリップボードフォーマットの配列を指定します。
//	cfcnt はクリップボードフォーマットの配列の要素数を指定します。

//	Effect は ドラッグ操作でドラッグされる対象で許される効果の組み合わせを指定します。

//[戻り値]
//ドロップが行われた場合は、ドロップ先のアプリケーションが設定した効果を返します。
//キャンセルやエラーの場合は -1 を返します。


#endif
