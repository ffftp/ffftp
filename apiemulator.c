// apiemulator.c
// Copyright (C) 2014 Suguru Kawamoto
// APIエミュレータ

#include <windows.h>

#define DO_NOT_REPLACE
#include "apiemulator.h"

BOOL IsUserAnAdminAlternative()
{
	BOOL bResult;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID pSID;
	BOOL b;
	bResult = FALSE;
	if(AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSID))
	{
		if(CheckTokenMembership(NULL, pSID, &b))
		{
			if(b)
				bResult = TRUE;
		}
		FreeSid(pSID);
	}
	return bResult;
}

