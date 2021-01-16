/*=============================================================================
*
*								タスクウインドウ
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

#define TASK_BUFSIZE	(16*1024)
extern int ClientWidth;
extern int SepaWidth;
extern int ListHeight;
extern int TaskHeight;
extern HFONT ListFont;
extern int RemoveOldLog;
int DebugConsole = NO;
static HWND hWndTask = NULL;
static Concurrency::concurrent_queue<std::wstring> queue;

static VOID CALLBACK Writer(HWND hwnd, UINT, UINT_PTR, DWORD) {
	std::wstring local;
	for (std::wstring temp; queue.try_pop(temp);) {
		local += temp;
		local += L"\r\n"sv;
	}
	if (empty(local))
		return;
	if (auto length = GetWindowTextLengthW(hwnd); RemoveOldLog == YES) {
		if (TASK_BUFSIZE <= size_as<int>(local))
			SendMessageW(hwnd, EM_SETSEL, 0, -1);
		else {
			for (; TASK_BUFSIZE <= length + size_as<int>(local); length = GetWindowTextLengthW(hwnd)) {
				SendMessageW(hwnd, EM_SETSEL, 0, SendMessageW(hwnd, EM_LINEINDEX, 1, 0));
				SendMessageW(hwnd, EM_REPLACESEL, false, (LPARAM)L"");
			}
			SendMessageW(hwnd, EM_SETSEL, length, length);
		}
	} else
		SendMessageW(hwnd, EM_SETSEL, length, length);
	SendMessageW(hwnd, EM_REPLACESEL, false, (LPARAM)local.c_str());
}

// タスクウインドウを作成する
int MakeTaskWindow() {
	constexpr DWORD style = WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_CLIPSIBLINGS;
	hWndTask = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, nullptr, style, 0, AskToolWinHeight() * 2 + ListHeight + SepaWidth, ClientWidth, TaskHeight, GetMainHwnd(), 0, GetFtpInst(), nullptr);
	if (hWndTask == NULL)
		return FFFTP_FAIL;

	SendMessageW(hWndTask, EM_LIMITTEXT, 0x7fffffff, 0);
	if (ListFont != NULL)
		SendMessageW(hWndTask, WM_SETFONT, (WPARAM)ListFont, MAKELPARAM(TRUE, 0));
	SetTimer(hWndTask, 1, USER_TIMER_MINIMUM, Writer);
	ShowWindow(hWndTask, SW_SHOW);
	return FFFTP_SUCCESS;
}


// タスクウインドウを削除
void DeleteTaskWindow() {
	DestroyWindow(hWndTask);
}


// タスクウインドウのウインドウハンドルを返す
HWND GetTaskWnd() {
	return hWndTask;
}


// タスクメッセージを表示する
void SetTaskMsg(_In_z_ _Printf_format_string_ const char* format, ...) {
	char buffer[10240 + 3];
	va_list args;
	va_start(args, format);
	int result = vsprintf(buffer, format, args);
	va_end(args);
	if (0 < result)
		queue.push(u8(buffer));
}


// タスク内容をビューワで表示
void DispTaskMsg() {
	auto temp = tempDirectory() / L"_ffftp.tsk";
	if (auto text = u8(GetText(hWndTask)); std::ofstream{ temp, std::ofstream::binary }.write(data(text), size(text)).bad()) {
		fs::remove(temp);
		return;
	}
	auto path = temp.u8string();
	AddTempFileList(temp);
	ExecViewer(data(path), 0);
}


// デバッグコンソールにメッセージを表示する
void DoPrintf(_In_z_ _Printf_format_string_ const char* format, ...) {
	if (DebugConsole != YES)
		return;
	struct {
		char prefix[3];
		char message[10240];
	} buffer = { { '#', '#', ' ' } };
	static_assert(std::is_same_v<decltype(buffer.prefix[0]), decltype(buffer.message[0])>);
	static_assert(sizeof buffer == std::size(buffer.prefix) * sizeof buffer.prefix[0] + std::size(buffer.message) * sizeof buffer.message[0]);
	va_list args;
	va_start(args, format);
	int result = vsprintf_s(buffer.message, format, args);
	va_end(args);
	if (0 < result)
		queue.push(u8(buffer.prefix, static_cast<size_t>(result) + 3));
}

void DoPrintf(_In_z_ _Printf_format_string_ const wchar_t* format, ...) {
	if (DebugConsole != YES)
		return;
	struct {
		wchar_t prefix[3];
		wchar_t message[10240];
	} buffer = { { L'#', L'#', L' ' } };
	static_assert(std::is_same_v<decltype(buffer.prefix[0]), decltype(buffer.message[0])>);
	static_assert(sizeof buffer == std::size(buffer.prefix) * sizeof buffer.prefix[0] + std::size(buffer.message) * sizeof buffer.message[0]);
	va_list args;
	va_start(args, format);
	int result = vswprintf_s(buffer.message, format, args);
	va_end(args);
	if (0 < result)
		queue.push({ reinterpret_cast<const wchar_t*>(&buffer), static_cast<size_t>(result) + 3 });
}
