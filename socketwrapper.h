// socketwrapper.h
// Copyright (C) 2011 Suguru Kawamoto
// ソケットラッパー

#ifndef __SOCKETWRAPPER_H__
#define __SOCKETWRAPPER_H__

HANDLE WSAAsyncGetHostByNameIPv6(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);
int WSACancelAsyncRequestIPv6(HANDLE hAsyncTaskHandle);
char* AddressToStringIPv4(char* str, void* in);
char* AddressToStringIPv6(char* str, void* in6);
char* inet6_ntoa(struct in6_addr in6);
struct in6_addr inet6_addr(const char* cp);
BOOL ConvertNameToPunycode(LPSTR Output, LPCSTR Input);
HANDLE WSAAsyncGetHostByNameM(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen);
HANDLE WSAAsyncGetHostByNameIPv6M(HWND hWnd, u_int wMsg, const char * name, char * buf, int buflen, short Family);

extern const struct in6_addr IN6ADDR_NONE;

#endif

