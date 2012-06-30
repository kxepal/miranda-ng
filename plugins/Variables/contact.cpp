/*
    Variables Plugin for Miranda-IM (www.miranda-im.org)
    Copyright 2003-2006 P. Boon

    This program is mir_free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "variables.h"
#include "contact.h"

struct _tagType
{
	int    cnfCode;
	TCHAR* str;
}
static builtinCnfs[] =
{
	{ CNF_FIRSTNAME,   _T(STR_FIRSTNAME)  }, 
	{ CNF_LASTNAME,    _T(STR_LASTNAME)   },
	{ CNF_NICK,        _T(STR_NICK)       },
	{ CNF_CUSTOMNICK,  _T(STR_CUSTOMNICK) },
	{ CNF_EMAIL,       _T(STR_EMAIL)      },
	{ CNF_CITY,        _T(STR_CITY)       },
	{ CNF_STATE,       _T(STR_STATE)      },
	{ CNF_COUNTRY,     _T(STR_COUNTRY)    },
	{ CNF_PHONE,       _T(STR_PHONE)      },
	{ CNF_HOMEPAGE,    _T(STR_HOMEPAGE)   },
	{ CNF_ABOUT,       _T(STR_ABOUT)      },
	{ CNF_GENDER,      _T(STR_GENDER)     },
	{ CNF_AGE,         _T(STR_AGE)        },
	{ CNF_FIRSTLAST,   _T(STR_FIRSTLAST)  },
	{ CNF_UNIQUEID,    _T(STR_UNIQUEID)   },
	{ CNF_DISPLAY,     _T(STR_DISPLAY)    },
	{ CNF_FAX,         _T(STR_FAX)        },
	{ CNF_CELLULAR,    _T(STR_CELLULAR)   },
	{ CNF_TIMEZONE,    _T(STR_TIMEZONE)   },
	{ CNF_MYNOTES,     _T(STR_MYNOTES)    },
	{ CNF_BIRTHDAY,    _T(STR_BIRTHDAY)   },
	{ CNF_BIRTHMONTH,  _T(STR_BIRTHMONTH) },
	{ CNF_BIRTHYEAR,   _T(STR_BIRTHYEAR)  },
	{ CNF_STREET,      _T(STR_STREET)     },
	{ CNF_ZIP,         _T(STR_ZIP)        },
	{ CNF_LANGUAGE1,   _T(STR_LANGUAGE1)  },
	{ CNF_LANGUAGE2,   _T(STR_LANGUAGE2)  },
	{ CNF_LANGUAGE3,   _T(STR_LANGUAGE3)  },
	{ CNF_CONAME,      _T(STR_CONAME)     },
	{ CNF_CODEPT,      _T(STR_CODEPT)     },
	{ CNF_COPOSITION,  _T(STR_COPOSITION) },
	{ CNF_COSTREET,    _T(STR_COSTREET)   },
	{ CNF_COCITY,      _T(STR_COCITY)     },
	{ CNF_COSTATE,     _T(STR_COSTATE)    },
	{ CNF_COZIP,       _T(STR_COZIP)      },
	{ CNF_COCOUNTRY,   _T(STR_COCOUNTRY)  },
	{ CNF_COHOMEPAGE,  _T(STR_COHOMEPAGE) },

	{ CCNF_ACCOUNT,    _T(STR_ACCOUNT)    },
	{ CCNF_PROTOCOL,   _T(STR_PROTOCOL)   },
	{ CCNF_STATUS,     _T(STR_STATUS)     },
	{ CCNF_INTERNALIP, _T(STR_INTERNALIP) },
	{ CCNF_EXTERNALIP, _T(STR_EXTERNALIP) },
	{ CCNF_GROUP,      _T(STR_GROUP)      },
	{ CCNF_PROTOID,    _T(STR_PROTOID)    }
};

#define NEWTSTR_ALLOCA(A) (A==NULL)?NULL:_tcscpy((TCHAR*)alloca(sizeof(TCHAR)*(_tcslen(A)+1)),A)

typedef struct {
	TCHAR* tszContact;
	HANDLE hContact;
	DWORD flags;
} CONTACTCE; /* contact cache entry */

/* cache for 'getcontactfromstring' service */
static CONTACTCE *cce = NULL;
static int cacheSize = 0;
static CRITICAL_SECTION csContactCache;

static HANDLE hContactSettingChangedHook;
static HANDLE hGetContactFromStringService;

/*
	converts a string into a CNF_ type
*/
BYTE getContactInfoType(TCHAR* type) {

	int i;

	if ( type == NULL || _tcslen(type) == 0 )
		return 0;

	for ( i=0; i < SIZEOF(builtinCnfs); i++ )
		if ( !_tcscmp( builtinCnfs[i].str, type ))
			return builtinCnfs[i].cnfCode;

	return 0;
}

/*
	returns info about a contact as a string
*/
static TCHAR* getContactInfo(BYTE type, HANDLE hContact)
{
	/* returns dynamic allocated buffer with info, or NULL if failed */
	CONTACTINFO ci;
	TCHAR *res, *szStatus;
	char *szProto, protoname[128], szVal[16];
	int status;
	BYTE realType;
	DBVARIANT dbv;

	res = NULL;
	if (hContact == NULL)
		return NULL;

	realType = (type & ~CNF_UNICODE);
	switch ( realType ) {
	case CCNF_PROTOID:
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto == NULL)
			return NULL;

		if ( type & CNF_UNICODE )
			return (TCHAR *)mir_a2t(szProto);
		return (TCHAR *)mir_strdup(szProto);

	case CCNF_ACCOUNT:
		if ( g_mirandaVersion < PLUGIN_MAKE_VERSION( 0,8,0,0 ))
			return NULL;

		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto) {
			PROTOACCOUNT* pa = ProtoGetAccount( szProto );
			if ( pa )
				return mir_tstrdup( pa->tszAccountName );
		}
		return NULL;

	case CCNF_PROTOCOL:
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto == NULL)
			return NULL;

		if (CallProtoService(szProto, PS_GETNAME, (WPARAM)sizeof(protoname), (LPARAM)protoname))
			return NULL;

		if ( type & CNF_UNICODE )
			return (TCHAR *)mir_a2t(protoname);

		return (TCHAR *)mir_strdup(protoname);

	case CCNF_STATUS:
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto == NULL)
			return NULL;

		status = DBGetContactSettingWord(hContact, szProto, "Status", 0);
		if (status == 0)
			return NULL;

		if (type & CNF_UNICODE) {
			szStatus = (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)status, (LPARAM)GSMDF_UNICODE);
			if (szStatus == NULL)
				return NULL;

			return mir_tstrdup(szStatus);
		}
		else {
			char *aszStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)status, 0);
			if (aszStatus == NULL)
				return NULL;

			return (TCHAR *)mir_strdup(aszStatus);
		}

	case CCNF_INTERNALIP:
	case CCNF_EXTERNALIP:
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto == NULL)
			return NULL;
		else {
			DWORD ip = DBGetContactSettingDword(hContact, szProto, (realType==CCNF_INTERNALIP)?"RealIP":"IP", 0);
			if (ip == 0)
				return NULL;

			struct in_addr in;
			in.s_addr = htonl(ip);
			char* dotted = inet_ntoa(in);
			if ( dotted == NULL )
				return NULL;

			if ( type & CNF_UNICODE )
				return (TCHAR *)mir_a2t(dotted);
			return (TCHAR *)mir_strdup(dotted);
		}

	case CCNF_GROUP:
		if (!DBGetContactSetting(hContact, "CList", "Group", &dbv)) {
			if ( type & CNF_UNICODE )
				res = (TCHAR *)mir_a2t(dbv.pszVal);
			else
				res = (TCHAR *)mir_strdup(dbv.pszVal);

			DBFreeVariant(&dbv);
			return res;
		}
		if (!DBGetContactSettingW(hContact, "CList", "Group", &dbv)) {
			if ( type & CNF_UNICODE )
				res = (TCHAR *)mir_wstrdup(dbv.pwszVal);
			else
				res = (TCHAR *)mir_u2a(dbv.pwszVal);

			DBFreeVariant(&dbv);
			return res;
		}
		break;

	case CNF_UNIQUEID:
		//UID for ChatRoom
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ( szProto != NULL ) {
			if ( DBGetContactSettingByte(hContact, szProto, "ChatRoom", 0) == 1) {
				DBVARIANT dbv;
				if ( !DBGetContactSettingTString(hContact, szProto, "ChatRoomID", &dbv )) {
					res = mir_tstrdup( dbv.ptszVal );
					DBFreeVariant( &dbv );
					return res;
		}	}	}

		//UID for other contact
		break;
	}

	ZeroMemory(&ci,sizeof(CONTACTINFO));
	ci.cbSize = sizeof(CONTACTINFO);
	ci.hContact = hContact;
	ci.dwFlag = type;
	if (CallService(MS_CONTACT_GETCONTACTINFO,(WPARAM)0,(LPARAM)&ci)) {
		if ( !( type & CNF_UNICODE ))
			return NULL;

		// retrieving the data using UNICODE failed, try without
		ci.dwFlag &= ~CNF_UNICODE;
		if ( CallService( MS_CONTACT_GETCONTACTINFO,(WPARAM)0,(LPARAM)&ci ))
			return NULL;
			
		if (ci.type == CNFT_ASCIIZ) {
			if (type & CNF_UNICODE) {
				TCHAR *ptszVal;

				ptszVal = (TCHAR *)mir_a2t((char *)ci.pszVal);
				mir_free(ci.pszVal);
				return ptszVal;
			}
			else {
				res = (TCHAR *)mir_strdup((char *)ci.pszVal);
				mir_free(ci.pszVal);
				return res;
	}	}	}

	memset(szVal, '\0', sizeof(szVal));
	switch(ci.type){
	case CNFT_BYTE:
		if (realType != CNF_GENDER)
			return itot(ci.bVal);

		szVal[0] = (char)ci.bVal;
		if ( type & CNF_UNICODE )
			return (TCHAR *)mir_a2t(szVal);
		return (TCHAR *)mir_strdup(szVal);

	case CNFT_WORD:
		return itot(ci.wVal);

	case CNFT_DWORD:
		return itot(ci.dVal);

	case CNFT_ASCIIZ:
		if (ci.pszVal != NULL) {
			if (type&CNF_UNICODE)
				res = mir_tstrdup(ci.pszVal);
			else
				res = (TCHAR *)mir_strdup((char *)ci.pszVal);

			mir_free(ci.pszVal);
		}
		break;
	}

	return res;
}

TCHAR *getContactInfoT(BYTE type, HANDLE hContact, int tchar)
{
	if (tchar)
	{
			return getContactInfo((BYTE)(type | CNF_UNICODE), hContact);
	}

	return getContactInfo(type, hContact);
}

/*
	MS_VARS_GETCONTACTFROMSTRING
*/
int getContactFromString( CONTACTSINFO* ci )
{
	/* service to retrieve a contact's HANDLE from a given string */
	char *szProto;
	TCHAR *szFind, *cInfo, *tszContact, *tszProto;
	BOOL bMatch;
	DBVARIANT dbv;
	HANDLE hContact;
	int count, i;

	if (ci == NULL)
		return -1;

	if (ci->flags&CI_UNICODE) {
		
			tszContact = NEWTSTR_ALLOCA(ci->tszContact);
	
	}
	else {
	
			WCHAR* tmp = mir_a2t(ci->szContact);
			tszContact = NEWTSTR_ALLOCA(tmp);
			mir_free(tmp);
		
	}
	if ( (tszContact == NULL) || (_tcslen(tszContact) == 0))
		return -1;

	ci->hContacts = NULL;
	count = 0;
	/* search the cache */
	EnterCriticalSection(&csContactCache);
	for (i=0;i<cacheSize;i++) {
		if ( (!_tcscmp(cce[i].tszContact, tszContact)) && (ci->flags == cce[i].flags)) {
			/* found in cache */
			ci->hContacts = ( HANDLE* )mir_alloc(sizeof(HANDLE));
			if (ci->hContacts == NULL) {
				LeaveCriticalSection(&csContactCache);
				return -1;
			}
			ci->hContacts[0] = cce[i].hContact;
			LeaveCriticalSection(&csContactCache);
			return 1;
		}
	}

	LeaveCriticalSection(&csContactCache);
	/* contact was not in cache, do a search */
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		szFind = NULL;
		bMatch = FALSE;
		ZeroMemory(&dbv, sizeof(DBVARIANT));
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto == NULL) {
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact, 0);
			continue;
		}
		// <proto:id> (exact)
		if (ci->flags&CI_PROTOID)
		{
			cInfo = getContactInfoT(CNF_UNIQUEID, hContact, ci->flags&CI_TCHAR);
			if (cInfo == NULL)
			{
				// <HANDLE:hContact>
				cInfo = (TCHAR*)mir_alloc(32);
				//cInfo = itot((INT_PTR)hContact);
				_stprintf(cInfo, _T("%p"), hContact);
				szFind = ( TCHAR* )mir_alloc((_tcslen(cInfo) + _tcslen(_T(PROTOID_HANDLE)) + 4)*sizeof(TCHAR));
				if (szFind != NULL)
				{
					wsprintf(szFind, _T("<%s:%s>"), _T(PROTOID_HANDLE), cInfo);
					if (!_tcsncmp(tszContact, szFind, _tcslen(tszContact)))
					{
						bMatch = TRUE;
					}
					mir_free(cInfo);
					mir_free(szFind);
				}
			}
			else
			{
				szFind = ( TCHAR* )mir_alloc((_tcslen(cInfo) + strlen(szProto) + 4)*sizeof(TCHAR));
				if (szFind != NULL)
				{
					tszProto = mir_a2t(szProto);

					if ( (tszProto != NULL) && (szFind != NULL))
					{
						wsprintf(szFind, _T("<%s:%s>"), tszProto, cInfo);
						mir_free(cInfo);
						mir_free(tszProto);
						if (!_tcsncmp(tszContact, szFind, _tcslen(tszContact)))
							bMatch = TRUE;

						mir_free(szFind);
					}
				}
			}
		}
		// id (exact)
		if ( (ci->flags&CI_UNIQUEID) && (!bMatch)) {
			szFind = getContactInfoT(CNF_UNIQUEID, hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind))
					bMatch = TRUE;

				mir_free(szFind);
			}
		}
		// nick (not exact)
		if ( (ci->flags&CI_NICK) && (!bMatch)) {
			szFind = getContactInfoT(CNF_NICK, hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind))
					bMatch = TRUE;

				mir_free(szFind);
			}
		}
		// list name (not exact)
		if ( (ci->flags&CI_LISTNAME) && (!bMatch)) {
			szFind = getContactInfoT(CNF_DISPLAY, hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind))
					bMatch = TRUE;

				mir_free(szFind);
			}
		}
		// firstname (exact)
		if ( (ci->flags&CI_FIRSTNAME) && (!bMatch)) {
			szFind = getContactInfoT(CNF_FIRSTNAME, hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind)) {
					bMatch = TRUE;
				}
				mir_free(szFind);
			}
		}
		// lastname (exact)
		if ( (ci->flags&CI_LASTNAME) && (!bMatch)) {
			szFind = getContactInfoT(CNF_LASTNAME, hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind)) {
					bMatch = TRUE;
				}
				mir_free(szFind);
			}
		}
		// email (exact)
		if ( (ci->flags&CI_EMAIL) && (!bMatch)) {
			szFind = getContactInfoT(CNF_EMAIL, hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind)) {
					bMatch = TRUE;
				}
				mir_free(szFind);
			}
		}
		// CNF_ (exact)
		if ( (ci->flags&CI_CNFINFO) && (!bMatch)) {
			szFind = getContactInfoT((BYTE)(ci->flags&~(CI_CNFINFO|CI_TCHAR)), hContact, ci->flags&CI_TCHAR);
			if (szFind != NULL) {
				if (!_tcscmp(tszContact, szFind)) {
					bMatch = TRUE;
				}
				mir_free(szFind);
			}
		}
		if (bMatch) {
			ci->hContacts = ( HANDLE* )mir_realloc(ci->hContacts, (count+1)*sizeof(HANDLE));
			if (ci->hContacts == NULL) {

				return -1;
			}
			ci->hContacts[count] = hContact;
			count += 1;
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}

	if (count == 1) { /* cache the found result */
		EnterCriticalSection(&csContactCache);
		cce = ( CONTACTCE* )mir_realloc(cce, (cacheSize+1)*sizeof(CONTACTCE));
		if (cce == NULL) {
			LeaveCriticalSection(&csContactCache);
			return count;
		}

		cce[cacheSize].hContact = ci->hContacts[0];
		cce[cacheSize].flags = ci->flags;
		cce[cacheSize].tszContact = mir_tstrdup(tszContact);
		if (cce[cacheSize].tszContact != NULL)
			cacheSize += 1;

		LeaveCriticalSection(&csContactCache);
	}

	return count;
}

/* keep cache consistent */
static int contactSettingChanged(WPARAM wParam, LPARAM lParam)
{
	int i;
	DBCONTACTWRITESETTING *dbw;
	char *szProto, *uid;

	uid = NULL;
	EnterCriticalSection(&csContactCache);
	for (i=0;i<cacheSize;i++) {
		if ((HANDLE)wParam != cce[i].hContact && (cce[i].flags & CI_CNFINFO) == 0 )
			continue;

		dbw = (DBCONTACTWRITESETTING*)lParam;
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam,0);
		if (szProto == NULL)
			continue;

		uid = (char*)CallProtoService(szProto,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);
		if (((!strcmp(dbw->szSetting, "Nick")) && (cce[i].flags&CI_NICK)) ||
			 ((!strcmp(dbw->szSetting, "FirstName")) && (cce[i].flags&CI_FIRSTNAME)) ||
			 ((!strcmp(dbw->szSetting, "LastName")) && (cce[i].flags&CI_LASTNAME)) ||
			 ((!strcmp(dbw->szSetting, "e-mail")) && (cce[i].flags&CI_EMAIL)) ||
			 ((!strcmp(dbw->szSetting, "MyHandle")) && (cce[i].flags&CI_LISTNAME)) ||
			 (cce[i].flags & CI_CNFINFO) != 0 || // lazy; always invalidate CNF info cache entries
			 (( ((INT_PTR)uid != CALLSERVICE_NOTFOUND) && (uid != NULL)) && (!strcmp(dbw->szSetting, uid)) && (cce[i].flags & CI_UNIQUEID)))
		{
			/* remove from cache */
			mir_free(cce[i].tszContact);
			if (cacheSize > 1) {
				MoveMemory(&cce[i], &cce[cacheSize-1], sizeof(CONTACTCE));
				cce = ( CONTACTCE* )mir_realloc(cce, (cacheSize-1)*sizeof(CONTACTCE));
				cacheSize -= 1;
			}
			else {
				mir_free(cce);
				cce = NULL;
				cacheSize = 0;
			}
			break;
		}
	}
	LeaveCriticalSection(&csContactCache);
	return 0;
}

static INT_PTR getContactFromStringSvc( WPARAM wParam, LPARAM lParam)
{
	return getContactFromString(( CONTACTSINFO* )wParam );
}

int initContactModule()
{
	InitializeCriticalSection(&csContactCache);
	hContactSettingChangedHook = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, contactSettingChanged);
	hGetContactFromStringService = CreateServiceFunction(MS_VARS_GETCONTACTFROMSTRING, getContactFromStringSvc);
	return 0;
}

int deinitContactModule()
{
	DestroyServiceFunction(hGetContactFromStringService);
	UnhookEvent(hContactSettingChangedHook);
	DeleteCriticalSection(&csContactCache);
	return 0;
}

// returns a string in the form <PROTOID:UNIQUEID>, cannot be _HANDLE_!
// result must be freed
TCHAR *encodeContactToString(HANDLE hContact)
{
	char *szProto;
	TCHAR *tszUniqueId, *tszResult, *tszProto;
	DBVARIANT dbv;

	ZeroMemory(&dbv, sizeof(DBVARIANT));
	szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	tszUniqueId = getContactInfoT(CNF_UNIQUEID, hContact, CI_TCHAR);
	if ( szProto == NULL || tszUniqueId == NULL )
		return NULL;

	tszResult = ( TCHAR* )mir_alloc((_tcslen(tszUniqueId) + strlen(szProto) + 4) * sizeof(TCHAR));
	if (tszResult == NULL)
		return NULL;

	tszProto = mir_a2t(szProto);
	
	if (tszProto == NULL)
		return NULL;

	wsprintf(tszResult, _T("<%s:%s>"), tszProto, tszUniqueId);

	mir_free(tszProto);

	return tszResult;
}

// returns a contact from a string in the form <PROTOID:UNIQUEID>
// returns INVALID_HANDLE_VALUE in case of an error.
HANDLE *decodeContactFromString(TCHAR *tszContact)
{
	int count;
	HANDLE hContact;
	CONTACTSINFO ci;

	hContact = INVALID_HANDLE_VALUE;
	ZeroMemory(&ci, sizeof(CONTACTSINFO));
	ci.cbSize = sizeof(CONTACTSINFO);
	ci.tszContact = tszContact;
	ci.flags = CI_PROTOID|CI_TCHAR;
	count = getContactFromString( &ci );
	if (count != 1) {
		if (ci.hContacts != NULL)
			mir_free(ci.hContacts);

		return ( HANDLE* )hContact;
	}
	if (ci.hContacts != NULL) {
		hContact = ci.hContacts[0];
		mir_free(ci.hContacts);
	}

	return ( HANDLE* )hContact;
}
