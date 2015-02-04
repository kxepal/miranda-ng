#include "common.h"

static const TCHAR *sttStatuses[] = { LPGENT("Members"), LPGENT("Owners") };

enum
{
	IDM_CANCEL,

	IDM_DESTROY, IDM_INVITE, IDM_LEAVE, IDM_TOPIC,

	IDM_MESSAGE, IDM_KICK,
	IDM_CPY_NICK, IDM_CPY_TOPIC,
	IDM_ADD_RJID, IDM_CPY_RJID
};

/////////////////////////////////////////////////////////////////////////////////////////
// protocol menu handler - create a new group

INT_PTR __cdecl WhatsAppProto::OnCreateGroup(WPARAM wParam, LPARAM lParam)
{
	ENTER_STRING es = { 0 };
	es.cbSize = sizeof(es);
	es.caption = _T("Enter group subject");
	es.szModuleName = m_szModuleName;
	if (EnterString(&es)) {
		if (isOnline()) {
			std::string groupName(ptrA(mir_utf8encodeT(es.ptszResult)));
			m_pConnection->sendCreateGroupChat(groupName);
		}
		mir_free(es.ptszResult);
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// join/leave services for the standard contact list popup menu

INT_PTR WhatsAppProto::OnJoinChat(WPARAM hContact, LPARAM)
{
	ptrA jid(getStringA(hContact, WHATSAPP_KEY_ID));
	if (jid && isOnline()) {
		setByte(hContact, "IsGroupMember", 0);
		m_pConnection->sendJoinLeaveGroup(jid, true);
	}
	return 0;
}

INT_PTR WhatsAppProto::OnLeaveChat(WPARAM hContact, LPARAM)
{
	ptrA jid(getStringA(hContact, WHATSAPP_KEY_ID));
	if (jid && isOnline()) {
		setByte(hContact, "IsGroupMember", 0);
		m_pConnection->sendJoinLeaveGroup(jid, false);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// handler to pass events from SRMM to WAConnection

int WhatsAppProto::onGroupChatEvent(WPARAM wParam, LPARAM lParam)
{
	GCHOOK *gch = (GCHOOK*)lParam;
	if (mir_strcmp(gch->pDest->pszModule, m_szModuleName))
		return 0;

	std::string chat_id(ptrA(mir_utf8encodeT(gch->pDest->ptszID)));
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[chat_id];
		if (pInfo == NULL)
			return 0;
	}

	switch (gch->pDest->iType) {
	case GC_USER_LEAVE:
	case GC_SESSION_TERMINATE:
		break;

	case GC_USER_LOGMENU:
		ChatLogMenuHook(pInfo, gch);
		break;

	case GC_USER_NICKLISTMENU:
		NickListMenuHook(pInfo, gch);
		break;

	case GC_USER_MESSAGE:
		if (isOnline()) {
			std::string msg(ptrA(mir_utf8encodeT(gch->ptszText)));
			
			try {
				int msgId = GetSerial();
				time_t now = time(NULL);
				std::string id = Utilities::intToStr(now) + "-" + Utilities::intToStr(msgId);

				FMessage fmsg(chat_id, true, id);
				fmsg.timestamp = now;
				fmsg.data = msg;
				m_pConnection->sendMessage(&fmsg);

				pInfo->m_unsentMsgs[id] = gch->ptszText;
			}
			CODE_BLOCK_CATCH_ALL
		}
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// chat log menu event handler

static gc_item sttLogListItems[] =
{
	{ LPGENT("&Invite a user"), IDM_INVITE, MENU_ITEM },
	{ LPGENT("View/change &topic"), IDM_TOPIC, MENU_POPUPITEM },
	{ NULL, 0, MENU_SEPARATOR },
	{ LPGENT("Copy room &JID"), IDM_CPY_RJID, MENU_ITEM },
	{ LPGENT("Copy room topic"), IDM_CPY_TOPIC, MENU_ITEM },
	{ NULL, 0, MENU_SEPARATOR },
	{ LPGENT("&Leave chat session"), IDM_LEAVE, MENU_ITEM }
};

void WhatsAppProto::ChatLogMenuHook(WAChatInfo *pInfo, struct GCHOOK *gch)
{
	switch (gch->dwData) {
	case IDM_INVITE:
		InviteChatUser(pInfo);
		break;

	case IDM_TOPIC:
		SetChatSubject(pInfo);
		break;

	case IDM_CPY_RJID:
		utils::copyText(pcli->hwndContactList, pInfo->tszJid);
		break;

	case IDM_CPY_TOPIC:
		utils::copyText(pcli->hwndContactList, pInfo->tszNick);
		break;

	case IDM_LEAVE:
		if (isOnline())
			m_pConnection->sendJoinLeaveGroup(_T2A(pInfo->tszJid), false);
		break;
	}
}

void WhatsAppProto::InviteChatUser(WAChatInfo *pInfo)
{

}

void WhatsAppProto::SetChatSubject(WAChatInfo *pInfo)
{
	CMString title(FORMAT, TranslateT("Set new subject for %s"), pInfo->tszNick);
	ptrT tszOldValue(getTStringA(pInfo->hContact, "Nick"));

	ENTER_STRING es = { 0 };
	es.cbSize = sizeof(es);
	es.type = ESF_RICHEDIT;
	es.szModuleName = m_szModuleName;
	es.ptszInitVal = tszOldValue;
	es.caption = title;
	es.szDataPrefix = "setSubject_";
	if (EnterString(&es)) {
		ptrA gjid(mir_utf8encodeT(pInfo->tszJid));
		ptrA gsubject(mir_utf8encodeT(es.ptszResult));
		m_pConnection->sendSetNewSubject(std::string(gjid), std::string(gsubject));
		mir_free(es.ptszResult);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// nicklist menu event handler

static gc_item sttListItems[] =
{
	{ LPGENT("&Add to roster"), IDM_ADD_RJID, MENU_POPUPITEM },
	{ NULL, 0, MENU_SEPARATOR },
	{ LPGENT("&Kick"), IDM_KICK, MENU_ITEM },
	{ NULL, 0, MENU_SEPARATOR },
	{ LPGENT("Copy &nickname"), IDM_CPY_NICK, MENU_ITEM },
	{ LPGENT("Copy real &JID"), IDM_CPY_RJID, MENU_ITEM },
};

void WhatsAppProto::NickListMenuHook(WAChatInfo *pInfo, struct GCHOOK *gch)
{
	switch (gch->dwData) {
	case IDM_ADD_RJID:
		AddChatUser(pInfo, gch->ptszUID);
		break;

	case IDM_KICK:
		KickChatUser(pInfo, gch->ptszUID);
		break;

	case IDM_CPY_NICK:
		utils::copyText(pcli->hwndContactList, GetChatUserNick(std::string((char*)_T2A(gch->ptszUID))));
		break;

	case IDM_CPY_RJID:
		utils::copyText(pcli->hwndContactList, gch->ptszUID);
		break;
	}
}

void WhatsAppProto::AddChatUser(WAChatInfo *pInfo, const TCHAR *ptszJid)
{
	std::string jid((char*)_T2A(ptszJid));
	MCONTACT hContact = ContactIDToHContact(jid);
	if (hContact && !db_get_b(hContact, "CList", "NotInList", 0))
		return;

	PROTOSEARCHRESULT sr = { 0 };
	sr.cbSize = sizeof(sr);
	sr.flags = PSR_TCHAR;
	sr.id = (TCHAR*)ptszJid;
	sr.nick = GetChatUserNick(jid);

	ADDCONTACTSTRUCT acs = { 0 };
	acs.handleType = HANDLE_SEARCHRESULT;
	acs.szProto = m_szModuleName;
	acs.psr = (PROTOSEARCHRESULT*)&sr;
	CallService(MS_ADDCONTACT_SHOW, (WPARAM)CallService(MS_CLUI_GETHWND, 0, 0), (LPARAM)&acs);
}

void WhatsAppProto::KickChatUser(WAChatInfo *pInfo, const TCHAR *ptszJid)
{
	if (!isOnline())
		return;

	std::string gjid((char*)_T2A(pInfo->tszJid));
	std::vector<std::string> jids(1);
	jids[0] = (char*)_T2A(ptszJid);
	m_pConnection->sendRemoveParticipants(gjid, jids);
}

/////////////////////////////////////////////////////////////////////////////////////////
// handler to customize chat menus

int WhatsAppProto::OnChatMenu(WPARAM wParam, LPARAM lParam)
{
	GCMENUITEMS *gcmi = (GCMENUITEMS*)lParam;
	if (gcmi == NULL)
		return 0;

	if (mir_strcmpi(gcmi->pszModule, m_szModuleName))
		return 0;

	if (gcmi->Type == MENU_ON_LOG) {
		gcmi->nItems = SIZEOF(sttLogListItems);
		gcmi->Item = sttLogListItems;
	}
	else if (gcmi->Type == MENU_ON_NICKLIST) {
		gcmi->nItems = SIZEOF(sttListItems);
		gcmi->Item = sttListItems;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// chat helpers

WAChatInfo* WhatsAppProto::InitChat(const std::string &jid, const std::string &nick)
{
	TCHAR *ptszJid = str2t(jid), *ptszNick = str2t(nick);

	WAChatInfo *pInfo = new WAChatInfo(ptszJid, ptszNick);
	m_chats[jid] = pInfo;

	GCSESSION gcw = { sizeof(GCSESSION) };
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = m_szModuleName;
	gcw.ptszName = ptszNick;
	gcw.ptszID = ptszJid;
	CallServiceSync(MS_GC_NEWSESSION, NULL, (LPARAM)&gcw);

	pInfo->hContact = ContactIDToHContact(jid);

	GCDEST gcd = { m_szModuleName, ptszJid, GC_EVENT_ADDGROUP };
	GCEVENT gce = { sizeof(gce), &gcd };
	for (int i = SIZEOF(sttStatuses) - 1; i >= 0; i--) {
		gce.ptszStatus = TranslateTS(sttStatuses[i]);
		CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}

	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync(MS_GC_EVENT, (m_pConnection) ? WINDOW_HIDDEN : SESSION_INITDONE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);

	if (m_pConnection)
		m_pConnection->sendGetParticipants(jid);

	return pInfo;
}

TCHAR* WhatsAppProto::GetChatUserNick(const std::string &jid)
{
	if (m_szJid == jid)
		return str2t(m_szNick);

	MCONTACT hContact = ContactIDToHContact(jid);
	return (hContact == 0) ? utils::removeA(str2t(jid)) : mir_tstrdup(pcli->pfnGetContactDisplayName(hContact, 0));
}

///////////////////////////////////////////////////////////////////////////////
// WAGroupListener members

void WhatsAppProto::onGroupInfo(const std::string &jid, const std::string &owner, const std::string &subject, const std::string &subject_owner, int time_subject, int time_created)
{
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[jid];
		if (pInfo == NULL) {
			pInfo = InitChat(jid, subject);
			pInfo->bActive = true;
		}
	}

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, 0 };
	if (!subject.empty()) {
		gcd.iType = GC_EVENT_TOPIC;
		pInfo->tszOwner = str2t(owner);

		ptrT tszSubject(str2t(subject));
		ptrT tszSubjectOwner(str2t(subject_owner));

		GCEVENT gce = { sizeof(gce), &gcd };
		gce.ptszUID = pInfo->tszOwner;
		gce.ptszNick = utils::removeA(tszSubjectOwner);
		gce.time = time_subject;
		gce.dwFlags = GCEF_ADDTOLOG;
		gce.ptszText = tszSubject;
		CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}

	gcd.iType = GC_EVENT_CONTROL;
	GCEVENT gce = { sizeof(gce), &gcd };
	CallServiceSync(MS_GC_EVENT, SESSION_INITDONE, (LPARAM)&gce);
}

void WhatsAppProto::onGroupMessage(const FMessage &msg)
{
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[msg.key.remote_jid];
		if (pInfo == NULL) {
			pInfo = InitChat(msg.key.remote_jid, "");
			pInfo->bActive = true;
		}
	}

	ptrT tszText(str2t(msg.data));
	ptrT tszUID(str2t(msg.remote_resource));
	ptrT tszNick(GetChatUserNick(msg.remote_resource));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_MESSAGE };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = tszNick;
	gce.time = msg.timestamp;
	gce.ptszText = tszText;
	gce.bIsMe = m_szJid == msg.remote_resource;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);

	if (isOnline())
		m_pConnection->sendMessageReceived(msg);
}

void WhatsAppProto::onGroupNewSubject(const std::string &gjid, const std::string &author, const std::string &newSubject, int ts)
{
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[gjid];
		if (pInfo == NULL)
			return;
	}

	ptrT tszUID(str2t(author));
	ptrT tszNick(GetChatUserNick(author));
	ptrT tszText(str2t(newSubject));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_TOPIC };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = tszNick;
	gce.time = ts;
	gce.ptszText = tszText;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);

	setTString(pInfo->hContact, "Nick", tszText);
}

void WhatsAppProto::onGroupAddUser(const std::string &gjid, const std::string &ujid, int ts)
{
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[gjid];
		if (pInfo == NULL)
			return;
	}

	ptrT tszUID(str2t(ujid));
	ptrT tszNick(GetChatUserNick(ujid));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_JOIN };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = tszNick;
	gce.time = ts;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
}

void WhatsAppProto::onGroupRemoveUser(const std::string &gjid, const std::string &ujid, int ts)
{
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[gjid];
		if (pInfo == NULL)
			return;
	}

	ptrT tszUID(str2t(ujid));
	ptrT tszNick(GetChatUserNick(ujid));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_PART };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = tszNick;
	gce.time = ts;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
}

void WhatsAppProto::onLeaveGroup(const std::string &paramString)
{
}

void WhatsAppProto::onGetParticipants(const std::string &gjid, const std::vector<string> &participants)
{
	mir_cslock lck(m_csChats);

	WAChatInfo *pInfo = m_chats[gjid];
	if (pInfo == NULL)
		return;

	for (size_t i = 0; i < participants.size(); i++) {
		std::string curr = participants[i];

		ptrT ujid(str2t(curr)), nick(GetChatUserNick(curr));
		bool bIsOwner = !mir_tstrcmp(ujid, pInfo->tszOwner);

		GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_JOIN };

		GCEVENT gce = { sizeof(gce), &gcd };
		gce.ptszNick = nick;
		gce.ptszUID = utils::removeA(ujid);
		gce.ptszStatus = (bIsOwner) ? _T("Owners") : _T("Members");
		CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}
}

void WhatsAppProto::onGroupCreated(const std::string &paramString1, const std::string &paramString2)
{
}

void WhatsAppProto::onGroupMessageReceived(const FMessage &msg)
{
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[msg.key.remote_jid];
		if (pInfo == NULL)
			return;
	}
	
	auto p = pInfo->m_unsentMsgs.find(msg.key.id);
	if (p == pInfo->m_unsentMsgs.end())
		return;

	ptrT tszUID(str2t(m_szJid));
	ptrT tszNick(str2t(m_szNick));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_MESSAGE };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = tszNick;
	gce.time = msg.timestamp;
	gce.ptszText = p->second.c_str();
	gce.bIsMe = m_szJid == msg.remote_resource;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);

	pInfo->m_unsentMsgs.erase(p);
}
