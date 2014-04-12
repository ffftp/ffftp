// updater.h
// Copyright (C) 2014 Suguru Kawamoto
// ソフトウェア自動更新

#ifndef __UPDATER_H__
#define __UPDATER_H__

#include <windows.h>

#define HTTP_USER_AGENT L"Mozilla/4.0"
#define UPDATE_SERVER L"ffftp.sourceforge.jp"
#define UPDATE_HASH_PATH L"/update/hash"
#define UPDATE_LIST_PATH L"/update/list"
#define UPDATE_RSA_PUBLIC_KEY \
	"-----BEGIN PUBLIC KEY-----\n" \
	"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAmJvR902LFeKcP9ldQfVQ\n" \
	"F11f3Ph+KDZRIaMM30lBV77atXKuhJunbrjoiocakoSI7UZ1e7Em/Vx7DKi09Hu5\n" \
	"P0Qr5VU4OZ1CoO0bLsot7kKm2LbvLvVD/D92Ff4nhTMD5jhtsdp/XIbRjcdRj+TI\n" \
	"BmEdGOL62vXZ5XjZbrO3CRis7g0Ft/ojSgH1Qd3QSck5IJ3+L7844uIF9SB73xME\n" \
	"RuL+tG2n+VGajM6Hi6xJ1ssbpr7iLB69QmQ5swIaJSiY8oE950mL+EBNFmI3Md0N\n" \
	"vr4tDG8+fq/VhQB64k5hfgWaBImKYaEfftvg51L7yRX+CttgV6GM85ls41H/NDPM\n" \
	"CwIDAQAB\n" \
	"-----END PUBLIC KEY-----\n"
#define UPDATE_SIGNATURE "\x15\x48\x1D\x36\x13\x9D\xA3\x84\x2F\x06\x73\x40\x74\xAC\xED\xFC\x2D\xED\x75\x86"

BOOL CheckForUpdates();
BOOL StartUpdateProcess();

#endif

