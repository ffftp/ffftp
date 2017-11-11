// updater.h
// Copyright (C) 2014 Suguru Kawamoto
// ソフトウェア自動更新

#ifndef __UPDATER_H__
#define __UPDATER_H__

#include <windows.h>

#define HTTP_USER_AGENT "curl/6.0"
#define UPDATE_SERVER "osdn.jp"
#if defined(_M_IX86)
#if !defined(FFFTP_ENGLISH)
#define UPDATE_HASH_PATH "/dl/ffftp/update.jpn.hash"
#define UPDATE_LIST_PATH "/dl/ffftp/update.jpn.list"
#else
#define UPDATE_HASH_PATH "/dl/ffftp/update.eng.hash"
#define UPDATE_LIST_PATH "/dl/ffftp/update.eng.list"
#endif
#elif defined(_M_AMD64)
#if !defined(FFFTP_ENGLISH)
#define UPDATE_HASH_PATH "/dl/ffftp/update.amd64.jpn.hash"
#define UPDATE_LIST_PATH "/dl/ffftp/update.amd64.jpn.list"
#else
#define UPDATE_HASH_PATH "/dl/ffftp/update.amd64.eng.hash"
#define UPDATE_LIST_PATH "/dl/ffftp/update.amd64.eng.list"
#endif
#endif
#define UPDATE_RSA_PUBLIC_KEY \
	"-----BEGIN PUBLIC KEY-----\n" \
	"MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAsVo13yricPHxkQypqiMy\n" \
	"+EVPH5KCTsOz0EAJo9WpdiDrDsATbQ7vbLz0DVVzYKmdBFKF98qTFGqKLt67Z/Y4\n" \
	"7fBmIbkEcA4Ct2oHlyuFtN8sxNAwjZ4k0EC59BXh0bsp+RXdwninliA0zRD30C4Z\n" \
	"Tseul9ZyvllUfVm+cdf7pOPwd1Akh3qdffWRVUHCjAjCeUqTKPSnevKgj4uSP440\n" \
	"ixRnwDSfH0+UPMIOdFT2fD9WG0CRDpg+PM/X34c4qjgnlQtDhfi0dHnZwN1gywkT\n" \
	"CVAXcn3uulVzUO4h61nYcliHhN1C0+mN2cf98C8d65DCkLYamaDFAXn5pxuKV5PM\n" \
	"Vl7O5+UYX7qVPFJih+YP+rf3UVe1kCQFWQ7K4HAz9IytFSNx7uNWbi1OoS5pTXhb\n" \
	"dd7LvwA29XdqFx3pcCqC08wyZnesXqHH828/yetHbXzO6t03CaESVaqmr9V6c9R/\n" \
	"d4c8aagPoG8tlysv4cR1UyAOPZ3ciT3dsn3sJr0HuYZ5S8zFKDybrT4r0hCGp3HS\n" \
	"FfsEoJacyuUJ9WkPul8kW//wdQFstsIisRaBkj/jH6+/aqamIItXR0GkAC7QSM1+\n" \
	"FztlwuPCzs/nJ4piaBBI8NOyWJ5xSSar3kW9arjHzkMDFwRmBVNz+UwgtoOy+jM3\n" \
	"BSnG4aZtcUEB6AZwhG+z9jkCAwEAAQ==\n" \
	"-----END PUBLIC KEY-----\n"
#define UPDATE_SIGNATURE "\x4C\x2A\x8E\x57\xAB\x75\x0C\xB5\xDA\x5F\xFE\xB9\x57\x9A\x1B\xA2\x7A\x61\x32\xF8\xFA\x4B\x61\xE2\xBA\x20\x9C\x37\xD5\x0A\xDC\x94\x10\x4D\x02\x30\x9B\xCD\x01\x9B\xB8\x73\x1E\xDB\xFD\xD7\x45\xCA\xE0\x8E\xF9\xB0\x1F\xB4\x0D\xD8\xFB\xE8\x41\x48\xE7\xF5\xE8\x64"

BOOL BuildUpdates(LPCTSTR PrivateKeyFile, LPCTSTR Password, LPCTSTR ServerPath, LPCTSTR HashFile, LPCTSTR ListFile, DWORD Version, LPCTSTR VersionString, LPCTSTR Description);
BOOL CheckForUpdates(BOOL bDownload, LPCTSTR DownloadDir, DWORD* pVersion, LPTSTR pVersionString, LPTSTR pDescription);
BOOL PrepareUpdates(void* pList, DWORD ListLength, LPCTSTR DownloadDir);
BOOL ApplyUpdates(LPCTSTR DestinationDir, LPCTSTR BackupDirName);
BOOL CleanupUpdates(LPCTSTR DownloadDir);
BOOL StartUpdateProcess(LPCTSTR DownloadDir, LPCTSTR CommandLine);
BOOL RestartUpdateProcessAsAdministrator(LPCTSTR CommandLine, LPCTSTR Keyword);

#endif

