/*
Plugin of miranda IM(ICQ) for Communicating with users of the baseProtocol.
Copyright (C) 2004 Daniel Savi (dss@brturbo.com)

-> Based on J. Lawler BaseProtocol <-

Added:
 - plugin option window
 - make.bat (vc++)
 - tinyclib http://msdn.microsoft.com/msdnmag/issues/01/01/hood/default.aspx
 - C++ version

Miranda ICQ: the free icq client for MS Windows
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede
*/

#include "stdafx.h"

CLIST_INTERFACE *pcli;
HINSTANCE hinstance;

HGENMENU hToggle, hEnableMenu;
BOOL gbVarsServiceExist = FALSE;
INT interval;
int hLangpack;

TCHAR* ptszDefaultMsg[] = {
	TranslateT("I am currently away. I will reply to you when I am back."),
	TranslateT("I am currently very busy and can't spare any time to talk with you. Sorry..."),
	TranslateT("I am not available right now."),
	TranslateT("I am now doing something, I will talk to you later."),
	TranslateT("I am on the phone right now. I will get back to you very soon."),
	TranslateT("I am having meal right now. I will get back to you very soon.")
};

PLUGININFOEX pluginInfoEx = {
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {46BF191F-8DFB-4656-88B2-4C20BE4CFA44}
	{ 0x46bf191f, 0x8dfb, 0x4656, { 0x88, 0xb2, 0x4c, 0x20, 0xbe, 0x4c, 0xfa, 0x44 } }
};

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
	return &pluginInfoEx;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD, LPVOID)
{
	hinstance = hinst;
	return TRUE;
}

INT_PTR ToggleEnable(WPARAM, LPARAM)
{
	BOOL fEnabled = !db_get_b(NULL, protocolname, KEY_ENABLED, 1);
	db_set_b(NULL, protocolname, KEY_ENABLED, fEnabled);

	if (fEnabled)
		Menu_ModifyItem(hEnableMenu, LPGENT("Disable Auto&reply"), iconList[0].hIcolib);
	else
		Menu_ModifyItem(hEnableMenu, LPGENT("Enable Auto&reply"), iconList[1].hIcolib);
	return 0;
}

INT_PTR Toggle(WPARAM hContact, LPARAM)
{
	BOOL on = !db_get_b(hContact, protocolname, "TurnedOn", 0);
	db_set_b(hContact, protocolname, "TurnedOn", on);
	on = !on;

	if (on)
		Menu_ModifyItem(hToggle, LPGENT("Turn off Autoanswer"), iconList[0].hIcolib);
	else
		Menu_ModifyItem(hToggle, LPGENT("Turn on Autoanswer"), iconList[1].hIcolib);
	return 0;
}

INT OnPreBuildContactMenu(WPARAM hContact, LPARAM)
{
	BOOL on = !db_get_b(hContact, protocolname, "TurnedOn", 0);
	if (on)
		Menu_ModifyItem(hToggle, LPGENT("Turn off Autoanswer"), iconList[0].hIcolib);
	else
		Menu_ModifyItem(hToggle, LPGENT("Turn on Autoanswer"), iconList[1].hIcolib);
	return 0;
}

INT CheckDefaults(WPARAM, LPARAM)
{
	DBVARIANT dbv;
	TCHAR *ptszDefault;
	char szStatus[6] = { 0 };

	interval = db_get_w(NULL, protocolname, KEY_REPEATINTERVAL, 300);

	if (db_get_ts(NULL, protocolname, KEY_HEADING, &dbv))
		// Heading not set
		db_set_ts(NULL, protocolname, KEY_HEADING, TranslateT("Dear %user%, the owner left the following message:"));
	else
		db_free(&dbv);

	for (int c = ID_STATUS_ONLINE; c < ID_STATUS_IDLE; c++) {
		mir_snprintf(szStatus, "%d", c);
		if (c == ID_STATUS_ONLINE || c == ID_STATUS_FREECHAT || c == ID_STATUS_INVISIBLE)
			continue;
		else {
			if (db_get_ts(NULL, protocolname, szStatus, &dbv)) {
				if (c < ID_STATUS_FREECHAT)
					// This mode does not have a preset message
					ptszDefault = ptszDefaultMsg[c - ID_STATUS_ONLINE - 1];
				else if (c > ID_STATUS_INVISIBLE)
					ptszDefault = ptszDefaultMsg[c - ID_STATUS_ONLINE - 3];
				if (ptszDefault)
					db_set_ts(NULL, protocolname, szStatus, ptszDefault);
			}
			else
				db_free(&dbv);
		}
	}
	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, OnPreBuildContactMenu);
	if (ServiceExists(MS_VARS_FORMATSTRING))
		gbVarsServiceExist = TRUE;

	BOOL fEnabled = db_get_b(NULL, protocolname, KEY_ENABLED, 1);
	if (fEnabled)
		Menu_ModifyItem(hEnableMenu, LPGENT("Disable Auto&reply"), iconList[0].hIcolib);
	else
		Menu_ModifyItem(hEnableMenu, LPGENT("Enable Auto&reply"), iconList[1].hIcolib);
	return 0;
}

INT addEvent(WPARAM hContact, LPARAM hDBEvent)
{
	BOOL fEnabled = db_get_b(NULL, protocolname, KEY_ENABLED, 1);
	if (!fEnabled || !hContact || !hDBEvent)
		return FALSE;	/// unspecifyed error

	char* pszProto = GetContactProto(hContact);
	int status = CallProtoService(pszProto, PS_GETSTATUS, 0, 0);
	if (status == ID_STATUS_ONLINE || status == ID_STATUS_FREECHAT || status == ID_STATUS_INVISIBLE)
		return FALSE;

	DBEVENTINFO dbei = { sizeof(dbei) };
	db_event_get(hDBEvent, &dbei); /// detect size of msg

	if ((dbei.eventType != EVENTTYPE_MESSAGE) || (dbei.flags == DBEF_READ))
		return FALSE; /// we need EVENTTYPE_MESSAGE event..
	else {	/// needed event has occured..
		DBVARIANT dbv;

		if (!dbei.cbBlob)	/// invalid size
			return FALSE;

		if (db_get_ts(hContact, "Protocol", "p", &dbv)) // Contact with no protocol ?!!
			return FALSE;
		db_free(&dbv);

		if (db_get_b(hContact, "CList", "NotOnList", 0))
			return FALSE;

		if (db_get_b(hContact, protocolname, "TurnedOn", 0))
			return FALSE;

		if (!(dbei.flags & DBEF_SENT)) {
			int timeBetween = time(NULL) - db_get_dw(hContact, protocolname, "LastReplyTS", 0);
			if (timeBetween > interval || db_get_w(hContact, protocolname, "LastStatus", 0) != status) {
				char szStatus[6] = { 0 };
				int msgLen = 1;
				int isQun = db_get_b(hContact, pszProto, "IsQun", 0);
				if (isQun)
					return FALSE;

				mir_snprintf(szStatus, "%d", status);
				if (!db_get_ts(NULL, protocolname, szStatus, &dbv)) {
					if (*dbv.ptszVal) {
						DBVARIANT dbvHead = { 0 }, dbvNick = { 0 };
						CMString ptszTemp;
						TCHAR *ptszTemp2;

						db_get_ts(hContact, pszProto, "Nick", &dbvNick);
						if (mir_tstrcmp(dbvNick.ptszVal, NULL) == 0) {
							db_free(&dbvNick);
							return FALSE;
						}

						msgLen += (int)mir_tstrlen(dbv.ptszVal);
						if (!db_get_ts(NULL, protocolname, KEY_HEADING, &dbvHead)) {
							ptszTemp = dbvHead.ptszVal;
							ptszTemp.Replace(_T("%user%"), dbvNick.ptszVal);
							msgLen += (int)(mir_tstrlen(ptszTemp));
						}
						ptszTemp2 = (TCHAR*)mir_alloc(sizeof(TCHAR) * (msgLen + 5));
						mir_sntprintf(ptszTemp2, msgLen + 5, _T("%s\r\n\r\n%s"), ptszTemp.c_str(), dbv.ptszVal);
						if (ServiceExists(MS_VARS_FORMATSTRING)) {
							FORMATINFO fi = { 0 };
							fi.cbSize = sizeof(fi);
							fi.flags = FIF_TCHAR;
							fi.tszFormat = ptszTemp2;
							ptszTemp = (TCHAR*)CallService(MS_VARS_FORMATSTRING, (WPARAM)&fi, 0);
						}
						else ptszTemp = Utils_ReplaceVarsT(ptszTemp2);

						T2Utf pszUtf(ptszTemp);
						CallContactService(hContact, PSS_MESSAGE, 0, pszUtf);

						dbei.cbSize = sizeof(dbei);
						dbei.eventType = EVENTTYPE_MESSAGE;
						dbei.flags = DBEF_UTF | DBEF_SENT; //DBEF_READ;
						dbei.szModule = pszProto;
						dbei.timestamp = time(NULL);
						dbei.cbBlob = (int)mir_strlen(pszUtf) + 1;
						dbei.pBlob = (PBYTE)pszUtf;
						db_event_add(hContact, &dbei);

						mir_free(ptszTemp2);
						if (dbvNick.ptszVal)
							db_free(&dbvNick);
						if (dbvHead.ptszVal)
							db_free(&dbvHead);
					}
					db_free(&dbv);
				}
			}
		}

		db_set_dw(hContact, protocolname, "LastReplyTS", time(NULL));
		db_set_w(hContact, protocolname, "LastStatus", status);
	}
	return 0;
}

IconItemT iconList[] =
{
	{ LPGENT("Disable Auto&reply"), "Disable Auto&reply", IDI_OFF },
	{ LPGENT("Enable Auto&reply"), "Enable Auto&reply", IDI_ON }
};

extern "C" int __declspec(dllexport)Load(void)
{
	mir_getLP(&pluginInfoEx);
	mir_getCLI();

	CreateServiceFunction(protocolname"/ToggleEnable", ToggleEnable);
	CreateServiceFunction(protocolname"/ToggleAutoanswer", Toggle);

	CMenuItem mi;

	SET_UID(mi, 0xac1c64a, 0x82ca, 0x4845, 0x86, 0x89, 0x59, 0x76, 0x12, 0x74, 0x72, 0x7b);
	mi.position = 500090000;
	mi.name.t = _T("");
	mi.pszService = protocolname"/ToggleEnable";
	hEnableMenu = Menu_AddMainMenuItem(&mi);

	SET_UID(mi, 0xb290cccd, 0x4ecc, 0x475e, 0x87, 0xcb, 0x51, 0xf4, 0x3b, 0xc3, 0x44, 0x9c);
	mi.position = -0x7FFFFFFF;
	mi.name.t = _T("");
	mi.pszService = protocolname"/ToggleAutoanswer";
	hToggle = Menu_AddContactMenuItem(&mi);

	//add hook
	HookEvent(ME_OPT_INITIALISE, OptInit);
	HookEvent(ME_DB_EVENT_ADDED, addEvent);
	HookEvent(ME_SYSTEM_MODULESLOADED, CheckDefaults);

	Icon_RegisterT(hinstance, _T("Simple Auto Replier"), iconList, _countof(iconList));

	return 0;
}

extern "C" __declspec(dllexport)int Unload(void)
{
	return 0;
}
