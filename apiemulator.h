// apiemulator.h
// Copyright (C) 2014 Suguru Kawamoto
// APIエミュレータ

#ifndef __APIEMULATOR_H__
#define __APIEMULATOR_H__

#include <windows.h>

#ifndef DO_NOT_REPLACE

#define IsUserAnAdmin IsUserAnAdminAlternative

#endif

BOOL IsUserAnAdminAlternative();

#endif

