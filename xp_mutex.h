// Copyright(C) 2020 Kurata Sayuri. All rights reserved.

#pragma once

#ifdef _MUTEX_
#error include "xp_mutex.h" before <mutex>.
#endif

// include from <mutex>.
#include <yvals_core.h>
#include <chrono>
#include <stdlib.h>
#include <system_error>
#include <thread>
#include <utility>
#include <xcall_once.h>

// for _WIN32_WINNT_VISTA.
#include <Windows.h>
#if _WIN32_WINNT < _WIN32_WINNT_VISTA

#define dllimport /* do not dllimport */

static void* const PV_INITIAL = reinterpret_cast<void*>(static_cast<uintptr_t>(0));
static void* const PV_WORKING = reinterpret_cast<void*>(static_cast<uintptr_t>(1));
static void* const PV_SUCCESS = reinterpret_cast<void*>(static_cast<uintptr_t>(2));

extern "C" [[nodiscard]] int __stdcall __std_init_once_begin_initialize(void** _LpInitOnce, unsigned long _DwFlags, int* _FPending, void** _LpContext) noexcept {
	// _DwFlags = 0, _LpContext = nullptrでしか使われていないので最低限の実装とする。
	if (_LpInitOnce == nullptr || _DwFlags != 0 || _FPending == nullptr || _LpContext != nullptr)
		return FALSE;
	for (;;) {
		auto prev = InterlockedCompareExchangePointer(_LpInitOnce, PV_WORKING, PV_INITIAL);
		if (prev == PV_INITIAL) {
			*_FPending = 1;
			return TRUE;
		}
		if (prev == PV_SUCCESS) {
			*_FPending = 0;
			return TRUE;
		}
		SwitchToThread();
	}
}

extern "C" [[nodiscard]] int __stdcall __std_init_once_complete(void** _LpInitOnce, unsigned long _DwFlags, void* _LpContext) noexcept {
	// _DwFlags = INIT_ONCE_INIT_FAILED or 0, _LpContext = nullptrでしか使われていないので最低限の実装とする。
	if (_LpInitOnce == nullptr || _DwFlags & ~INIT_ONCE_INIT_FAILED || _LpContext != nullptr)
		return FALSE;
	auto prev = InterlockedCompareExchangePointer(_LpInitOnce, _DwFlags & INIT_ONCE_INIT_FAILED ? PV_INITIAL : PV_SUCCESS, PV_WORKING);
	return prev == PV_WORKING ? TRUE : FALSE;
}

#include <mutex>

#undef dllimport

#else
#include <mutex>
#endif
