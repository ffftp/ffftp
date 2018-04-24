// socketwrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー

#ifndef __SOCKETWRAPPER_H__
#define __SOCKETWRAPPER_H__

HANDLE WSAAsyncGetHostByNameIPv6(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);
int WSACancelAsyncRequestIPv6(HANDLE hAsyncTaskHandle);
BOOL ConvertNameToPunycode(LPSTR Output, LPCSTR Input);
HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);

extern const struct in6_addr IN6ADDR_NONE;

#endif

