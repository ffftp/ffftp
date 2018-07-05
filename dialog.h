// Copyright(C) 2018 Kurata Sayuri. All rights reserved.
#pragma once
#include <type_traits>
#include <Windows.h>
#include <windowsx.h>

template<int... controls>
struct Controls;

template<class RightAnchoredControls, class BottomAnchoredControls, class StretchingControls>
class Resizable;
template<int... anchorRight, int... anchorBottom, int... anchorStretch>
class Resizable<Controls<anchorRight...>, Controls<anchorBottom...>, Controls<anchorStretch...>> {
	static const UINT flags = SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_DEFERERASE | SWP_ASYNCWINDOWPOS;
	SIZE minimum;
	SIZE current;
	static void OnSizeRight(HWND dialog, int id, LONG dx) {
		auto control = GetDlgItem(dialog, id);
		RECT r;
		GetWindowRect(control, &r);
		POINT p{ r.left, r.top };
		ScreenToClient(dialog, &p);
		SetWindowPos(control, 0, p.x + dx, p.y, 0, 0, SWP_NOSIZE | flags);
	}
	static void OnSizeBottom(HWND dialog, int id, LONG dy) {
		auto control = GetDlgItem(dialog, id);
		RECT r;
		GetWindowRect(control, &r);
		POINT p{ r.left, r.top };
		ScreenToClient(dialog, &p);
		SetWindowPos(control, 0, p.x, p.y + dy, 0, 0, SWP_NOSIZE | flags);
	}
	static void OnSizeStretch(HWND dialog, int id, LONG dx, LONG dy) {
		auto control = GetDlgItem(dialog, id);
		RECT r;
		GetWindowRect(control, &r);
		SetWindowPos(control, 0, 0, 0, r.right - r.left + dx, r.bottom - r.top + dy, SWP_NOMOVE | flags);
	}
public:
	Resizable() : current{} {}
	Resizable(SIZE current) : current{ current } {}
	SIZE GetCurrent() const { return current; }
	void OnSize(HWND dialog, LONG cx, LONG cy) {
		LONG dx = cx - current.cx, dy = cy - current.cy;
		if (dx != 0)
			(..., OnSizeRight(dialog, anchorRight, dx));
		if (dy != 0)
			(..., OnSizeBottom(dialog, anchorBottom, dy));
		if (dx != 0 || dy != 0)
			(..., OnSizeStretch(dialog, anchorStretch, dx, dy));
		current = { cx, cy };
		InvalidateRect(dialog, nullptr, FALSE);
	}
	void OnSizing(HWND dialog, RECT* targetSize, int edge) {
		if (targetSize->right - targetSize->left < minimum.cx) {
			if (edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT)
				targetSize->left = targetSize->right - minimum.cx;
			else
				targetSize->right = targetSize->left + minimum.cx;
		}
		if (targetSize->bottom - targetSize->top < minimum.cy) {
			if (edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
				targetSize->top = targetSize->bottom - minimum.cy;
			else
				targetSize->bottom = targetSize->top + minimum.cy;
		}
		OnSize(dialog, targetSize->right - targetSize->left, targetSize->bottom - targetSize->top);
	}
	void Initialize(HWND dialog) {
		RECT r;
		GetWindowRect(dialog, &r);
		minimum = { r.right - r.left, r.bottom - r.top };
		if (current.cx == 0)
			current = minimum;
		else {
			auto copied = current;
			current = minimum;
			SetWindowPos(dialog, 0, 0, 0, copied.cx, copied.cy, SWP_NOMOVE | flags);
		}
	}
};

namespace detail {
	template<class Data>
	class Dialog {
		template<class T> static constexpr auto hasOnInit() -> decltype(std::declval<T>().OnInit(HWND{}), 0) { return true; }
		template<class T> static constexpr auto hasOnInit(...) { return false; }
		template<class T> static constexpr auto hasOnCommand1() -> decltype(std::declval<T>().OnCommand(HWND{}, WORD{}), 0) { return true; }
		template<class T> static constexpr auto hasOnCommand1(...) { return false; }
		template<class T> static constexpr auto hasOnCommand2() -> decltype(std::declval<T>().OnCommand(HWND{}, WORD{}, WORD{}), 0) { return true; }
		template<class T> static constexpr auto hasOnCommand2(...) { return false; }
		template<class T> static constexpr auto hasOnNotify() -> decltype(std::declval<T>().OnNotify(HWND{}, std::declval<NMHDR*>()), 0) { return true; }
		template<class T> static constexpr auto hasOnNotify(...) { return false; }
		template<class T> static constexpr auto hasOnMessage() -> decltype(std::declval<T>().OnMessage(HWND{}, UINT{}, WPARAM{}, LPARAM{}), 0) { return true; }
		template<class T> static constexpr auto hasOnMessage(...) { return false; }
		template<class T> static constexpr auto hasResizable() -> decltype(T::resizable, 0) { return true; }
		template<class T> static constexpr auto hasResizable(...) { return false; }
	public:
		static INT_PTR CALLBACK Proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
			if (uMsg == WM_INITDIALOG) {
				auto ptr = reinterpret_cast<Data*>(lParam);
				SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, lParam);
				INT_PTR result = TRUE;
				if constexpr (hasOnInit<Data>())
					result = ptr->OnInit(hwndDlg);
				if constexpr (hasResizable<Data>())
					ptr->resizable.Initialize(hwndDlg);
				return result;
			}
			auto data = reinterpret_cast<Data*>(GetWindowLongPtrW(hwndDlg, GWLP_USERDATA));
			if constexpr (hasOnCommand1<Data>()) {
				if (uMsg == WM_COMMAND) {
					data->OnCommand(hwndDlg, GET_WM_COMMAND_ID(wParam, lParam));
					return 0;
				}
			} else if constexpr (hasOnCommand2<Data>()) {
				if (uMsg == WM_COMMAND){
					data->OnCommand(hwndDlg, GET_WM_COMMAND_CMD(wParam, lParam), GET_WM_COMMAND_ID(wParam, lParam));
					return 0;
				}
			}
			if constexpr (hasResizable<Data>()) {
				if (uMsg == WM_SIZING) {
					data->resizable.OnSizing(hwndDlg, reinterpret_cast<RECT*>(lParam), static_cast<int>(wParam));
					return TRUE;
				}
				if (uMsg == WM_SIZE) {
					RECT r;
					GetWindowRect(hwndDlg, &r);
					data->resizable.OnSize(hwndDlg, r.right - r.left, r.bottom - r.top);
					return 0;
				}
			}
			if constexpr (hasOnNotify<Data>()) {
				if (uMsg == WM_NOTIFY)
					return data->OnNotify(hwndDlg, reinterpret_cast<NMHDR*>(lParam));
			}
			if constexpr (hasOnMessage<Data>()) {
				return data->OnMessage(hwndDlg, uMsg, wParam, lParam);
			}
			return FALSE;
		}
	};
}

// DialogBoxを表示します。
// 次の要件を満たした型を受け入れます。
// struct Data {
//     // 必須。ダイアログプロシージャからEndDialog()を呼び出す際に使われる値の型を宣言します。そのままDialog()関数の戻り値になります。
//     using result_t = ...;
//     // 任意。宣言されている場合はダイアログのサイズを可変にします。第１テンプレートパラメーターは右端に固定されるID、第２は下端に固定されるID、第３は伸縮するIDをそれぞれ指定します。
//     // resizable.GetCurrent()で現在のダイアログサイズを取得できます。
//     Resizable<Controls<ID1, ID2, ...>, Controls<ID3, ID4, ...>, Controls<ID5, ID6, ...>> resizable;
//     // 任意。WM_INITDIALOGメッセージを処理するコールバック。
//     INT_PTR OnInit(HWND);
//     // 任意。WM_COMMANDメッセージを処理するコールバック。第２引数は押されたコマンドのIDです。
//     void OnCommand(HWND, WORD);
//     // 任意。WM_NOTIFYメッセージを処理するコールバック。第２引数はlParamで渡されたNMHDR*です。
//     INT_PTR OnNotify(HWND, NMHDR*);
//     // 任意。残りのメッセージを処理するコールバック。
//     INT_PTR OnMessage(HWND, UNIT, WPARAM, LPARAM);
// };
template<class Data>
static inline auto Dialog(HINSTANCE instance, int resourceId, HWND parent, Data&& data) {
	using T = std::remove_reference_t<Data>;
	return (typename T::result_t)DialogBoxParamW(instance, MAKEINTRESOURCEW(resourceId), parent, detail::Dialog<T>::Proc, (LPARAM)&data);
}
