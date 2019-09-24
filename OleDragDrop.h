/**************************************************************************

	OleDragDrop.h

	(C) Copyright 1996-2002 By Tomoaki Nakashima. All right reserved.	
		http://www.nakka.com/
		nakka@nakka.com

**************************************************************************/
// refactored by Kurata Sayuri.

#pragma once
#include <optional>
#include <tuple>
#include <vector>
#include <Windows.h>
#include <UrlMon.h>			// for CreateFormatEnumerator
#include "wrl/client.h"		// for Microsoft::WRL::Make
#include "wrl/implements.h"	// for Microsoft::WRL::RuntimeClass
#pragma comment(lib, "UrlMon.lib")

namespace OleDragDrop {
	using namespace Microsoft::WRL;

	enum class NotifyType {
		DragEnter,
		DragOver,
		DragLeave,
		Drop,
	};

	struct NotifyData {
		POINTL* ppt;					//マウスの位置
		DWORD dwEffect;					//ドラッグ操作で、ドラッグされる対象で許される効果
		DWORD grfKeyState;				//キーの状態
		UINT cfFormat;					//ドロップされるデータのクリップボードフォーマット
		HANDLE hMem;					//ドロップされるデータ
		IDataObject* pdo;				//IDataObject
	};

	#pragma region Detail
	namespace Detail {
		class Common {
			constexpr DWORD Tymed(CLIPFORMAT clipFormat) {
				switch (clipFormat) {
				case CF_BITMAP:
					return TYMED_GDI;
				case CF_METAFILEPICT:
					return TYMED_MFPICT;
				case CF_ENHMETAFILE:
					return TYMED_ENHMF;
				default:
					return TYMED_HGLOBAL;
				}
			}
		protected:
			HWND hWnd;
			std::vector<FORMATETC> formatEtcs;
			Common(HWND hWnd, CLIPFORMAT* clipFormats, size_t count) : hWnd{ hWnd }, formatEtcs(count) {
				for (size_t i = 0; i < count; i++)
					formatEtcs.push_back({ clipFormats[i], nullptr, DVASPECT_CONTENT, -1, Tymed(clipFormats[i]) });
			}
			LRESULT SendMessageW(UINT Msg, WPARAM wParam, LPARAM lParam) {
				return ::SendMessageW(hWnd, Msg, wParam, lParam);
			}
		};

		class DropTarget : public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, IDropTarget>, public Common {
			UINT msgNotify;
			NotifyData notifyData;
			void Notify(NotifyType notifyType, IDataObject* dataObject, DWORD grfKeyState, POINTL* ppt, DWORD* effect) {
				/* 対応しているクリップボードフォーマットがあるか調べる */
				std::optional<std::tuple<CLIPFORMAT, STGMEDIUM>> found;
				if (dataObject)
					for (auto formatEtc : formatEtcs)
						if (STGMEDIUM sm; dataObject->QueryGetData(&formatEtc) == S_OK && dataObject->GetData(&formatEtc, &sm) == S_OK) {
							found = { formatEtc.cfFormat, sm };
							break;
						}

				/* ウィンドウにイベントを通知する */
				notifyData.ppt = ppt;
				notifyData.grfKeyState = grfKeyState;
				notifyData.cfFormat = found ? std::get<0>(*found) : 0;
				notifyData.hMem = found ? std::get<1>(*found).hGlobal : 0;
				notifyData.pdo = dataObject;
				SendMessageW(msgNotify, static_cast<int>(notifyType), (LPARAM)&notifyData);

				/* クリップボード形式のデータの解放 */
				if (found)
					ReleaseStgMedium(&std::get<1>(*found));

				/* 効果の設定 */
				if (effect) {
					*effect &= notifyData.dwEffect;
					if (*effect & DROPEFFECT_MOVE) {
						*effect &= ~DROPEFFECT_COPY;
						notifyData.dwEffect &= ~DROPEFFECT_COPY;
					}
				}
			}
		public:
			DropTarget(HWND hWnd, UINT msgNotify, CLIPFORMAT* clipFormats, size_t count) : msgNotify{ msgNotify }, Common{ hWnd, clipFormats, count } {}

			// IDropTarget
			STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
				Notify(NotifyType::DragEnter, pDataObj, grfKeyState, &pt, pdwEffect);
				return S_OK;
			}
			STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
				Notify(NotifyType::DragOver, nullptr, grfKeyState, &pt, pdwEffect);
				return S_OK;
			}
			STDMETHODIMP DragLeave() override {
				Notify(NotifyType::DragLeave, nullptr, 0, nullptr, nullptr);
				return S_OK;
			}
			STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
				Notify(NotifyType::Drop, pDataObj, grfKeyState, &pt, pdwEffect);
				return S_OK;
			}
		};


		class DataObject : public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>, IDataObject, IDropSource>, public Common {
			UINT msgGetData;
			UINT msgDragOver;
			DWORD button;
		public:
			DataObject(HWND hWnd, UINT msgGetData, UINT msgDragOver, CLIPFORMAT* clipFormats, size_t count) : msgGetData{ msgGetData }, msgDragOver{ msgDragOver }, Common{ hWnd, clipFormats, count } {
				button = GetKeyState(VK_RBUTTON) & 0x8000 ? MK_RBUTTON : MK_LBUTTON;
			}

			// IDataObject
			STDMETHODIMP GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium) override {
				// マウスのドラッグ中は msgGetData を送らないようにする。(2007.9.3 yutaka)
				if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000))
					return DV_E_FORMATETC;

				/* 要求されたクリップボードフォーマットが存在するか調べる */
				for (auto const& formatEtc : formatEtcs)
					if (formatEtc.cfFormat == pformatetcIn->cfFormat) {
						/* ウィンドウにデータの要求を行う */
						HGLOBAL hMem = nullptr;
						SendMessageW(msgGetData, (WPARAM)formatEtc.cfFormat, (LPARAM)&hMem);
						if (!hMem)
							return STG_E_MEDIUMFULL;
						pmedium->tymed = formatEtc.tymed;
						pmedium->hGlobal = hMem;
						pmedium->pUnkForRelease = nullptr;
						return S_OK;
					}
				return DV_E_FORMATETC;
			}
			STDMETHODIMP GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium) override {
				return E_NOTIMPL;
			}
			STDMETHODIMP QueryGetData(FORMATETC* pformatetc) override {
				for (auto const& formatEtc : formatEtcs)
					if (formatEtc.cfFormat == pformatetc->cfFormat)
						return S_OK;
				return DV_E_FORMATETC;
			}
			STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* pformatectIn, FORMATETC* pformatetcOut) override {
				return E_NOTIMPL;
			}
			STDMETHODIMP SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease) override {
				return E_NOTIMPL;
			}
			STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc) override {
				if (!ppenumFormatEtc)
					return E_INVALIDARG;
				if (dwDirection != DATADIR_GET)
					return E_NOTIMPL;
				return CreateFormatEnumerator(static_cast<UINT>(size(formatEtcs)), data(formatEtcs), ppenumFormatEtc);
			}
			STDMETHODIMP DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection) override {
				return OLE_E_ADVISENOTSUPPORTED;
			}
			STDMETHODIMP DUnadvise(DWORD dwConnection) override {
				return OLE_E_NOCONNECTION;
			}
			STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise) override {
				return OLE_E_ADVISENOTSUPPORTED;
			}

			// IDropSource
			STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override {
				if (fEscapePressed)
					return DRAGDROP_S_CANCEL;
				SendMessageW(msgDragOver, 0, 0);
				return grfKeyState & button ? S_OK : DRAGDROP_S_DROP;
			}
			STDMETHODIMP GiveFeedback(DWORD dwEffect) override {
				return DRAGDROP_S_USEDEFAULTCURSORS;
			}
		};
	}
	#pragma endregion


	// ドラッグ&ドロップのターゲットとして登録します。
	// ドラッグ&ドロップ操作が行われたときに指定のウィンドウの指定のメッセージに通知されます。
	// [引数]
	//  wParam に操作の種類(OleDragDrop::NotifyType)が設定されています。
	//  lParam に OleDragDrop::NotifyData 構造体へのポインタが設定されています。
	//  clipFormts は 受け取ることが可能なクリップボードフォーマットのリストを指定します。
	//  count はクリップボードフォーマットの配列の要素数を指定します。
	static inline bool RegisterDragDrop(HWND hWnd, UINT msgNotify, CLIPFORMAT* clipFormts, int count) {
		auto dropTarget = Make<Detail::DropTarget>(hWnd, msgNotify, clipFormts, count);
		return dropTarget && ::RegisterDragDrop(hWnd, dropTarget.Get()) == S_OK;
	}


	// ドラッグ＆ドロップのターゲットを解除します。
	static inline void RevokeDragDrop(HWND hWnd) {
		::RevokeDragDrop(hWnd);
	}


	// ドラッグ＆ドロップを開始するときに指定します。
	// ドラッグ＆ドロップ操作は自動的に行われますが、データが必要な時は、指定のウィンドウメッセージでデータ要求を行います。
	// [引数]
	//  hWnd に msgGetData を送ってデータの要求を行います。
	//  この時 wParam に要求するクリップボードフォーマットの値が入っています。
	//  プログラムは *(HGLOBAL*)lParam にデータを設定して返します。(NULLでも可)
	//  clipFormts はサポートしているクリップボードフォーマットの配列を指定します。
	//  count はクリップボードフォーマットの配列の要素数を指定します。
	//  okEffects は ドラッグ操作でドラッグされる対象で許される効果の組み合わせを指定します。
	// [戻り値]
	//  ドロップが行われた場合は、ドロップ先のアプリケーションが設定した効果を返します。
	//  キャンセルやエラーの場合は -1 を返します。
	static inline int DoDragDrop(HWND hWnd, UINT msgGetData, UINT msgDragOver, CLIPFORMAT* clipFormts, int count, int okEffects) {
		if (auto dataObject = Make<Detail::DataObject>(hWnd, msgGetData, msgDragOver, clipFormts, count))
			if (DWORD effect = 0; ::DoDragDrop(dataObject.Get(), dataObject.Get(), okEffects, &effect) == DRAGDROP_S_DROP)
				return effect;
		return -1;
	}
}
