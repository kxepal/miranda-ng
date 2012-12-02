/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-04  Santithorn Bunchua
Copyright (C) 2005-12  George Hazan

Idea & portions of code by Artem Shpynov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "jabber.h"
#include "jabber_list.h"

#include <m_icolib.h>

#include <m_cluiframes.h>

#define IDI_ONLINE                      104
#define IDI_OFFLINE                     105
#define IDI_AWAY                        128
#define IDI_FREE4CHAT                   129
#define IDI_INVISIBLE                   130
#define IDI_NA                          131
#define IDI_DND                         158
#define IDI_OCCUPIED                    159
#define IDI_ONTHEPHONE                  1002
#define IDI_OUTTOLUNCH                  1003

HIMAGELIST hAdvancedStatusIcon = NULL;

struct CTransportProtoTableItem
{
	TCHAR* mask;
	char*  proto;
};

static CTransportProtoTableItem TransportProtoTable[] =
{
	{ _T("|*icq*|jit*"),     "ICQ" },
	{ _T("msn*"),            "MSN" },
	{ _T("yahoo*"),          "YAHOO" },
	{ _T("mrim*"),           "MRA" },
	{ _T("aim*"),            "AIM" },
	//request #3094
	{ _T("|gg*|gadu*"),      "GaduGadu" },
	{ _T("tv*"),             "TV" },
	{ _T("dict*"),           "Dictionary" },
	{ _T("weather*"),        "Weather" },
	{ _T("skype*"),          "Skype" }, 
	{ _T("sms*"),            "SMS" },
	{ _T("smtp*"),           "SMTP" },
	//j2j
	{ _T("gtalk.*.*"),       "GTalk" },
	{ _T("|xmpp.*.*|j2j.*.*"),"Jabber2Jabber" },
	//jabbim.cz - services
	{ _T("disk*"),           "Jabber Disk" },
	{ _T("irc*"),            "IRC" },
	{ _T("rss*"),            "RSS" },
	{ _T("tlen*"),           "Tlen" },

	// German social networks
	{ _T("studivz*"),        "StudiVZ" },
	{ _T("schuelervz*"),     "SchuelerVZ" },
	{ _T("meinvz*"),         "MeinVZ" },
	{ _T("|fb*|facebook*"),  "Facebook" },
};

static int skinIconStatusToResourceId[] = {IDI_OFFLINE,IDI_ONLINE,IDI_AWAY,IDI_DND,IDI_NA,IDI_NA,/*IDI_OCCUPIED,*/IDI_FREE4CHAT,IDI_INVISIBLE,IDI_ONTHEPHONE,IDI_OUTTOLUNCH};
static int skinStatusToJabberStatus[] = {0,1,2,3,4,4,6,7,2,2};

///////////////////////////////////////////////////////////////////////////////
// CIconPool class

int CIconPool::CPoolItem::cmp(const CPoolItem *p1, const CPoolItem *p2)
{
	return lstrcmpA(p1->m_name, p2->m_name);
}

CIconPool::CPoolItem::CPoolItem():
	m_name(NULL), m_szIcolibName(NULL), m_hIcolibItem(NULL)
{
}

CIconPool::CPoolItem::~CPoolItem()
{
	if (m_hIcolibItem && m_szIcolibName) {
		CallService(MS_SKIN2_REMOVEICON, 0, (LPARAM)m_szIcolibName);
		mir_free(m_szIcolibName);
	}

	if (m_name) mir_free(m_name);
}

CIconPool::CIconPool() :
	m_items(10, CIconPool::CPoolItem::cmp),
	m_hOnExtraIconsRebuild(NULL)
{
}

CIconPool::~CIconPool()
{
	if (m_hOnExtraIconsRebuild) {
		UnhookEvent(m_hOnExtraIconsRebuild);
		m_hOnExtraIconsRebuild = NULL;
	}
}

void CIconPool::RegisterIcon(const char *name, TCHAR *filename, int iconid, TCHAR *szSection, TCHAR *szDescription)
{
	char szSettingName[128];
	mir_snprintf(szSettingName, SIZEOF(szSettingName), "jabber_%s", name);

	CPoolItem *item = new CPoolItem;
	item->m_name = mir_strdup(name);
	item->m_szIcolibName = mir_strdup(szSettingName);

	SKINICONDESC sid = { sizeof(sid) };
	sid.ptszDefaultFile = filename;
	sid.pszName = szSettingName;
	sid.ptszSection = szSection;
	sid.ptszDescription = szDescription;
	sid.flags = SIDF_ALL_TCHAR;
	sid.iDefaultIndex = iconid;
	item->m_hIcolibItem = Skin_AddIcon(&sid);

	m_items.insert(item);
}

HANDLE CIconPool::GetIcolibHandle(const char *name)
{
	if (CPoolItem *item = FindItemByName(name))
		return item->m_hIcolibItem;

	return NULL;
}

char *CIconPool::GetIcolibName(const char *name)
{
	if (CPoolItem *item = FindItemByName(name))
		return item->m_szIcolibName;

	return NULL;
}

HICON CIconPool::GetIcon(const char *name, bool big)
{
	if (CPoolItem *item = FindItemByName(name))
		return Skin_GetIconByHandle(item->m_hIcolibItem, big);

	return NULL;
}

CIconPool::CPoolItem *CIconPool::FindItemByName(const char *name)
{
	CPoolItem item;
	item.m_name = mir_strdup(name);
	return m_items.find(&item);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Icons init

static IconItem iconList[] =
{
	{   LPGEN("%s"),  "main", IDI_JABBER },
};

void CJabberProto::IconsInit(void)
{
	m_transportProtoTableStartIndex = (int *)mir_alloc(sizeof(int) * SIZEOF(TransportProtoTable));
	for (int i = 0; i < SIZEOF(TransportProtoTable); ++i)
		m_transportProtoTableStartIndex[i] = -1;

	IconItemT protoIcon = { m_tszUserName, "main", IDI_JABBER };
	Icon_RegisterT(hInst, _T("Protocols/Jabber/Accounts"), &protoIcon, 1, m_szModuleName);
	m_hProtoIcon = protoIcon.hIcolib;
}

HANDLE CJabberProto::GetIconHandle(int iconId)
{
	if (HANDLE result = g_GetIconHandle(iconId))
		return result;

	if (iconId == IDI_JABBER)
		return m_hProtoIcon;

	return NULL;
}

HICON CJabberProto::LoadIconEx(const char* name, bool big)
{
	if (HICON result = g_LoadIconEx(name, big))
		return result;

	char szSettingName[100];
	mir_snprintf(szSettingName, sizeof(szSettingName), "%s_%s", m_szModuleName, name);
	return Skin_GetIcon(szSettingName, big);
}

/////////////////////////////////////////////////////////////////////////////////////////
// internal functions

static inline TCHAR qtoupper(TCHAR c)
{
	return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c;
}

static BOOL WildComparei(const TCHAR *name, const TCHAR *mask)
{
	const TCHAR *last='\0';
	for (;; mask++, name++) {
		if (*mask != '?' && qtoupper(*mask) != qtoupper(*name))
			break;
		if (*name == '\0')
			return ((BOOL)!*mask);
	}

	if (*mask != '*')
		return FALSE;

	for (;; mask++, name++) {
		while(*mask == '*') {
			last = mask++;
			if (*mask == '\0')
				return ((BOOL)!*mask);   /* true */
		}

		if (*name == '\0')
			return ((BOOL)!*mask);      /* *mask == EOS */
		if (*mask != '?' && qtoupper(*mask) != qtoupper(*name))
			name -= (size_t)(mask - last) - 1, mask = last;
}	}

static BOOL MatchMask(const TCHAR *name, const TCHAR *mask)
{
	if ( !mask || !name)
		return mask == name;

	if (*mask != '|')
		return WildComparei(name, mask);

	TCHAR* temp = NEWTSTR_ALLOCA(mask);
	for (int e=1; mask[e] != '\0'; e++) {
		int s = e;
		while (mask[e] != '\0' && mask[e] != '|')
			e++;

		temp[e]= _T('\0');
		if (WildComparei(name, temp+s))
			return TRUE;

		if (mask[e] == 0)
			return FALSE;
	}

	return FALSE;
}

static HICON ExtractIconFromPath(const char *path, BOOL * needFree)
{
	char *comma;
	char file[MAX_PATH],fileFull[MAX_PATH];
	int n;
	HICON hIcon;
	lstrcpynA(file,path,sizeof(file));
	comma=strrchr(file,',');
	if (comma==NULL) n=0;
	else {n=atoi(comma+1); *comma=0;}
	CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)file, (LPARAM)fileFull);
	hIcon=NULL;
	ExtractIconExA(fileFull,n,NULL,&hIcon,1);
	if (needFree)
		*needFree=(hIcon!=NULL);

	return hIcon;
}

static HICON LoadTransportIcon(char *filename,int i,char *IconName,TCHAR *SectName,TCHAR *Description,int internalidx, BOOL * needFree)
{
	char szPath[MAX_PATH],szMyPath[MAX_PATH], szFullPath[MAX_PATH],*str;
	BOOL has_proto_icon=FALSE;
	if (needFree) *needFree=FALSE;
	GetModuleFileNameA(NULL, szPath, MAX_PATH);
	str=strrchr(szPath,'\\');
	if (str!=NULL) *str=0;
	_snprintf(szMyPath, sizeof(szMyPath), "%s\\Icons\\%s", szPath, filename);
	_snprintf(szFullPath, sizeof(szFullPath), "%s\\Icons\\%s,%d", szPath, filename, i);
	BOOL nf;
	HICON hi=ExtractIconFromPath(szFullPath,&nf);
	if (hi) has_proto_icon=TRUE;
	if (hi && nf) DestroyIcon(hi);
	if (IconName != NULL && SectName != NULL)  {
		SKINICONDESC sid = { sizeof(sid) };
		sid.hDefaultIcon = (has_proto_icon) ? NULL : LoadSkinnedProtoIcon(0, -internalidx);
		sid.ptszSection = SectName;
		sid.pszName = IconName;
		sid.ptszDescription = Description;
		sid.pszDefaultFile = szMyPath;
		sid.iDefaultIndex = i;
		sid.flags = SIDF_TCHAR;
		Skin_AddIcon(&sid);
	}
	return Skin_GetIcon(IconName);
}

static HICON LoadSmallIcon(HINSTANCE hInstance, LPCTSTR lpIconName)
{
	HICON hIcon=NULL;                 // icon handle
	int index=-(int)lpIconName;
	TCHAR filename[MAX_PATH]={0};
	GetModuleFileName(hInstance,filename,MAX_PATH);
	ExtractIconEx(filename,index,NULL,&hIcon,1);
	return hIcon;
}

int CJabberProto::LoadAdvancedIcons(int iID)
{
	char *proto = TransportProtoTable[iID].proto;
	char defFile[MAX_PATH] = {0};
	TCHAR Group[255];
	char Uname[255];
	int first=-1;
	HICON empty=LoadSmallIcon(NULL,MAKEINTRESOURCE(102));

	mir_sntprintf(Group, SIZEOF(Group), _T("Status Icons/%s/") _T(TCHAR_STR_PARAM) _T(" %s"), m_tszUserName, proto, TranslateT("transport"));
	mir_snprintf(defFile, SIZEOF(defFile), "proto_%s.dll",proto);
	if ( !hAdvancedStatusIcon)
		hAdvancedStatusIcon=(HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST,0,0);

	EnterCriticalSection(&m_csModeMsgMutex);
	for (int i=0; i < ID_STATUS_ONTHEPHONE-ID_STATUS_OFFLINE; i++) {
		HICON hicon;
		BOOL needFree;
		int n=skinStatusToJabberStatus[i];
		TCHAR *descr = (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, n+ID_STATUS_OFFLINE, GSMDF_TCHAR);
		mir_snprintf(Uname, SIZEOF(Uname), "%s_Transport_%s_%d", m_szModuleName, proto, n);
		hicon=(HICON)LoadTransportIcon(defFile,-skinIconStatusToResourceId[i],Uname,Group,descr,-(n+ID_STATUS_OFFLINE),&needFree);
		int index=(m_transportProtoTableStartIndex[iID] == -1)?-1:m_transportProtoTableStartIndex[iID]+n;
		int added=ImageList_ReplaceIcon(hAdvancedStatusIcon,index,hicon?hicon:empty);
		if (first == -1) first=added;
		if (hicon && needFree) DestroyIcon(hicon);
	}

	if (m_transportProtoTableStartIndex[iID] == -1)
		m_transportProtoTableStartIndex[iID] = first;
	LeaveCriticalSection(&m_csModeMsgMutex);
	return 0;
}

int CJabberProto::GetTransportProtoID(TCHAR* TransportDomain)
{
	for (int i=0; i<SIZEOF(TransportProtoTable); i++)
		if (MatchMask(TransportDomain, TransportProtoTable[i].mask))
			return i;

	return -1;
}

int CJabberProto::GetTransportStatusIconIndex(int iID, int Status)
{
	if (iID < 0 || iID >= SIZEOF(TransportProtoTable))
		return -1;

	//icons not loaded - loading icons
	if (m_transportProtoTableStartIndex[iID] == -1)
		LoadAdvancedIcons(iID);

	//some fault on loading icons
	if (m_transportProtoTableStartIndex[iID] == -1)
		return -1;

	if (Status < ID_STATUS_OFFLINE)
		Status = ID_STATUS_OFFLINE;

	return m_transportProtoTableStartIndex[iID] + skinStatusToJabberStatus[ Status - ID_STATUS_OFFLINE ];
}

/////////////////////////////////////////////////////////////////////////////////////////
// a hook for the IcoLib plugin

int CJabberProto::OnReloadIcons(WPARAM, LPARAM)
{
	for (int i=0; i < SIZEOF(TransportProtoTable); i++)
		if (m_transportProtoTableStartIndex[i] != -1)
			LoadAdvancedIcons(i);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Prototype for Jabber and other protocols to return index of Advanced status
// wParam - HCONTACT of called protocol
// lParam - should be 0 (reserverd for futher usage)
// return value: -1 - no Advanced status
// : other - index of icons in clcimagelist.
// if imagelist require advanced painting status overlay(like xStatus)
// index should be shifted to HIWORD, LOWORD should be 0

INT_PTR __cdecl CJabberProto::JGetAdvancedStatusIcon(WPARAM wParam, LPARAM)
{
	HANDLE hContact=(HANDLE)wParam;
	if ( !hContact)
		return -1;

	if ( !JGetByte(hContact, "IsTransported", 0))
		return -1;

	DBVARIANT dbv;
	if (JGetStringT(hContact, "Transport", &dbv))
		return -1;

	int iID = GetTransportProtoID(dbv.ptszVal);
	db_free(&dbv);
	if (iID >= 0) {
		WORD Status = ID_STATUS_OFFLINE;
		Status = JGetWord(hContact, "Status", ID_STATUS_OFFLINE);
		if (Status < ID_STATUS_OFFLINE)
			Status = ID_STATUS_OFFLINE;
		else if (Status > ID_STATUS_INVISIBLE)
			Status = ID_STATUS_ONLINE;
		return GetTransportStatusIconIndex(iID, Status);
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
//   Transport check functions

BOOL CJabberProto::DBCheckIsTransportedContact(const TCHAR *jid, HANDLE hContact)
{
	// check if transport is already set
	if ( !jid || !hContact)
		return FALSE;

	// strip domain part from jid
	TCHAR* domain  = _tcschr((TCHAR*)jid, '@');
	BOOL   isAgent = (domain == NULL) ? TRUE : FALSE;
	BOOL   isTransported = FALSE;
	if (domain!=NULL)
		domain = NEWTSTR_ALLOCA(domain+1);
	else
		domain = NEWTSTR_ALLOCA(jid);

	TCHAR* resourcepos = _tcschr(domain, '/');
	if (resourcepos != NULL)
		*resourcepos = '\0';

	for (int i=0; i < SIZEOF(TransportProtoTable); i++) {
		if (MatchMask(domain, TransportProtoTable[i].mask)) {
			GetTransportStatusIconIndex(GetTransportProtoID(domain), ID_STATUS_OFFLINE);
			isTransported = TRUE;
			break;
	}	}

	if (m_lstTransports.getIndex(domain) == -1) {
		if (isAgent) {
			m_lstTransports.insert(mir_tstrdup(domain));
			JSetByte(hContact, "IsTransport", 1);
	}	}

	if (isTransported) {
		JSetStringT(hContact, "Transport", domain);
		JSetByte(hContact, "IsTransported", 1);
	}
	return isTransported;
}

void CJabberProto::CheckAllContactsAreTransported()
{
	HANDLE hContact = (HANDLE)db_find_first();
	while (hContact != NULL) {
		char *szProto = GetContactProto(hContact);
		if ( !lstrcmpA(m_szModuleName, szProto)) {
			DBVARIANT dbv;
			if ( !JGetStringT(hContact, "jid", &dbv)) {
				DBCheckIsTransportedContact(dbv.ptszVal, hContact);
				db_free(&dbv);
		}	}

		hContact = db_find_next(hContact);
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Cross-instance shared icons

static IconItem sharedIconList[] =
{
	{   LPGEN("Privacy Lists"),         "privacylists",     IDI_PRIVACY_LISTS      },
	{   LPGEN("Bookmarks"),             "bookmarks",        IDI_BOOKMARKS          },
	{   LPGEN("Notes"),                 "notes",            IDI_NOTES              },
	{   LPGEN("Multi-User Conference"), "group",            IDI_GROUP              },
	{   LPGEN("Agents list"),           "Agents",           IDI_AGENTS             },
	{   LPGEN("Transports"),            "transport",        IDI_TRANSPORT          },
	{   LPGEN("Registered transports"), "transport_loc",    IDI_TRANSPORTL         },
	{   LPGEN("Change password"),       "key",              IDI_KEYS,              },
	{   LPGEN("Personal vCard"),        "vcard",            IDI_VCARD,             },
	{   LPGEN("Request authorization"), "Request",          IDI_REQUEST,           },
	{   LPGEN("Grant authorization"),   "Grant",            IDI_GRANT,             },
	{   LPGEN("Revoke authorization"),  "Revoke",           IDI_AUTHREVOKE,        },
	{   LPGEN("Convert to room"),       "convert",          IDI_USER2ROOM,         },
	{   LPGEN("Add to roster"),         "addroster",        IDI_ADDROSTER,         },
	{   LPGEN("Login/logout"),          "trlogonoff",       IDI_LOGIN,             },
	{   LPGEN("Resolve nicks"),         "trresolve",        IDI_REFRESH,           },
	{   LPGEN("Send note"),             "sendnote",         IDI_SEND_NOTE,         },
	{   LPGEN("Service Discovery"),     "servicediscovery", IDI_SERVICE_DISCOVERY, },
	{   LPGEN("AdHoc Command"),         "adhoc",            IDI_COMMAND,           },
	{   LPGEN("XML Console"),           "xmlconsole",       IDI_CONSOLE,           },
	{   LPGEN("OpenID Request"),        "openid",           IDI_HTTP_AUTH,         },

	{   LPGEN("Discovery succeeded"),   "disco_ok",         IDI_DISCO_OK,          },
	{   LPGEN("Discovery failed"),      "disco_fail",       IDI_DISCO_FAIL,        },
	{   LPGEN("Discovery in progress"), "disco_progress",   IDI_DISCO_PROGRESS,    },
	{   LPGEN("View as tree"),          "sd_view_tree",     IDI_VIEW_TREE,         },
	{   LPGEN("View as list"),          "sd_view_list",     IDI_VIEW_LIST,         },
	{   LPGEN("Apply filter"),          "sd_filter_apply",  IDI_FILTER_APPLY,      },
	{   LPGEN("Reset filter"),          "sd_filter_reset",  IDI_FILTER_RESET,      },

	{   LPGEN("Navigate home"),         "sd_nav_home",      IDI_NAV_HOME,          },
	{   LPGEN("Refresh node"),          "sd_nav_refresh",   IDI_NAV_REFRESH,       },
	{   LPGEN("Browse node"),           "sd_browse",        IDI_BROWSE,            },
	{   LPGEN("RSS service"),           "node_rss",         IDI_NODE_RSS,          },
	{   LPGEN("Server"),                "node_server",      IDI_NODE_SERVER,       },
	{   LPGEN("Storage service"),       "node_store",       IDI_NODE_STORE,        },
	{   LPGEN("Weather service"),       "node_weather",     IDI_NODE_WEATHER,      },

	{   LPGEN("Generic privacy list"),  "pl_list_any",      IDI_PL_LIST_ANY,       },
	{   LPGEN("Active privacy list"),   "pl_list_active",   IDI_PL_LIST_ACTIVE,    },
	{   LPGEN("Default privacy list"),  "pl_list_default",  IDI_PL_LIST_DEFAULT,   },
	{   LPGEN("Move up"),               "arrow_up",         IDI_ARROW_UP,          },
	{   LPGEN("Move down"),             "arrow_down",       IDI_ARROW_DOWN,        },
	{   LPGEN("Allow Messages"),        "pl_msg_allow",     IDI_PL_MSG_ALLOW,      },
	{   LPGEN("Allow Presences (in)"),  "pl_prin_allow",    IDI_PL_PRIN_ALLOW,     },
	{   LPGEN("Allow Presences (out)"), "pl_prout_allow",   IDI_PL_PROUT_ALLOW,    },
	{   LPGEN("Allow Queries"),         "pl_iq_allow",      IDI_PL_QUERY_ALLOW,    },
	{   LPGEN("Deny Messages"),         "pl_msg_deny",      IDI_PL_MSG_DENY,       },
	{   LPGEN("Deny Presences (in)"),   "pl_prin_deny",     IDI_PL_PRIN_DENY,      },
	{   LPGEN("Deny Presences (out)"),  "pl_prout_deny",    IDI_PL_PROUT_DENY,     },
	{   LPGEN("Deny Queries"),          "pl_iq_deny",       IDI_PL_QUERY_DENY,     },
};

static void sttProcessIcons(int iAmount)
{
	Icon_Register(hInst, "Protocols/Jabber", iconList, 21, GLOBAL_SETTING_PREFIX);
	Icon_Register(hInst, "Protocols/Jabber/Dialogs", iconList+21, 7, GLOBAL_SETTING_PREFIX);
	Icon_Register(hInst, "Protocols/Dialogs/Discovery", iconList+28, 7, GLOBAL_SETTING_PREFIX);
	Icon_Register(hInst, "Protocols/Dialogs/Privacy",   iconList+35, 13, GLOBAL_SETTING_PREFIX);
}

void g_IconsInit()
{
	sttProcessIcons(SIZEOF(sharedIconList));
}

HANDLE g_GetIconHandle(int iconId)
{
	for (int i=0; i < SIZEOF(sharedIconList); i++)
		if (sharedIconList[i].defIconID == iconId)
			return sharedIconList[i].hIcolib;

	return NULL;
}

HICON g_LoadIconEx(const char* name, bool big)
{
	char szSettingName[100];
	mir_snprintf(szSettingName, sizeof(szSettingName), "%s_%s", GLOBAL_SETTING_PREFIX, name);
	return Skin_GetIcon(szSettingName, big);
}

void g_ReleaseIcon(HICON hIcon)
{
	if (hIcon)
		Skin_ReleaseIcon(hIcon);
}

void ImageList_AddIcon_Icolib(HIMAGELIST hIml, HICON hIcon)
{
	ImageList_AddIcon(hIml, hIcon);
	g_ReleaseIcon(hIcon);
}

void WindowSetIcon(HWND hWnd, CJabberProto *proto, const char* name)
{
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)proto->LoadIconEx(name, true));
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)proto->LoadIconEx(name));
}

void WindowFreeIcon(HWND hWnd)
{
	g_ReleaseIcon((HICON)SendMessage(hWnd, WM_SETICON, ICON_BIG, 0));
	g_ReleaseIcon((HICON)SendMessage(hWnd, WM_SETICON, ICON_SMALL, 0));
}
