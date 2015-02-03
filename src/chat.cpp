#include "common.h"

static const TCHAR *sttStatuses[] = { LPGENT("Members"), LPGENT("Owners") };

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

int WhatsAppProto::OnChatOutgoing(WPARAM wParam, LPARAM lParam)
{
	GCHOOK *hook = (GCHOOK*)lParam;
	if (mir_strcmp(hook->pDest->pszModule, m_szModuleName))
		return 0;

	std::string chat_id(ptrA(mir_utf8encodeT(hook->pDest->ptszID)));
	WAChatInfo *pInfo;
	{
		mir_cslock lck(m_csChats);
		pInfo = m_chats[chat_id];
		if (pInfo == NULL)
			return 0;
	}

	switch (hook->pDest->iType) {
	case GC_USER_LEAVE:
	case GC_SESSION_TERMINATE:
		break;

	case GC_USER_MESSAGE:
		if (isOnline()) {
			std::string msg(ptrA(mir_utf8encodeT(hook->ptszText)));
			
			try {
				int msgId = GetSerial();
				time_t now = time(NULL);
				std::string id = Utilities::intToStr(now) + "-" + Utilities::intToStr(msgId);

				FMessage fmsg(chat_id, true, id);
				fmsg.timestamp = now;
				fmsg.data = msg;
				m_pConnection->sendMessage(&fmsg);

				pInfo->m_unsentMsgs[id] = hook->ptszText;
			}
			CODE_BLOCK_CATCH_ALL
		}
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// handler to customize chat menus

enum
{
	IDM_CANCEL,

	IDM_DESTROY, IDM_INVITE, IDM_LEAVE, IDM_TOPIC,

	IDM_MESSAGE, IDM_KICK,
	IDM_RJID_ADD, IDM_RJID_COPY,

	IDM_CPY_NICK, IDM_CPY_TOPIC, IDM_CPY_RJID
};

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

static gc_item sttListItems[] =
{
	{ LPGENT("&Add to roster"), IDM_RJID_ADD, MENU_POPUPITEM },
	{ LPGENT("&Copy to clipboard"), IDM_RJID_COPY, MENU_POPUPITEM },
	{ NULL, 0, MENU_SEPARATOR },
	{ LPGENT("&Kick"), IDM_KICK, MENU_ITEM },
	{ NULL, 0, MENU_SEPARATOR },
	{ LPGENT("Copy &nickname"), IDM_CPY_NICK, MENU_ITEM },
	{ LPGENT("Copy real &JID"), IDM_CPY_RJID, MENU_ITEM },
};

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

void __cdecl WhatsAppProto::SendSetGroupNameWorker(void* data)
{
	input_box_ret* ibr(static_cast<input_box_ret*>(data));
	string groupName(ibr->value);
	mir_free(ibr->value);

	ptrA jid(getStringA(*((MCONTACT*)ibr->userData), WHATSAPP_KEY_ID));
	if (jid && isOnline())
		m_pConnection->sendSetNewSubject((char*)jid, groupName);

	delete ibr->userData;
	delete ibr;
}

///////////////////////////////////////////////////////////////////////////////
// chat helpers

void WhatsAppProto::InitChat(const TCHAR *jid, const TCHAR *nick)
{
	GCSESSION gcw = { sizeof(GCSESSION) };
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = m_szModuleName;
	gcw.ptszName = nick;
	gcw.ptszID = jid;
	CallServiceSync(MS_GC_NEWSESSION, NULL, (LPARAM)&gcw);

	GCDEST gcd = { m_szModuleName, jid, GC_EVENT_ADDGROUP };
	GCEVENT gce = { sizeof(gce), &gcd };
	for (int i = SIZEOF(sttStatuses) - 1; i >= 0; i--) {
		gce.ptszStatus = TranslateTS(sttStatuses[i]);
		CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}

	gcd.iType = GC_EVENT_CONTROL;
	CallServiceSync(MS_GC_EVENT, (m_pConnection) ? WINDOW_HIDDEN : SESSION_INITDONE, (LPARAM)&gce);
	CallServiceSync(MS_GC_EVENT, SESSION_ONLINE, (LPARAM)&gce);

	if (m_pConnection)
		m_pConnection->sendGetParticipants(_T2A(jid));
}

TCHAR* WhatsAppProto::GetChatUserNick(const std::string &jid)
{
	if (m_szJid == jid)
		return mir_utf8decodeT(m_szNick.c_str());

	MCONTACT hContact = ContactIDToHContact(jid);
	return (hContact == 0) ? utils::removeA(mir_utf8decodeT(jid.c_str())) : mir_tstrdup(pcli->pfnGetContactDisplayName(hContact, 0));
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
			pInfo = new WAChatInfo(jid.c_str(), subject.c_str());
			m_chats[jid] = pInfo;

			InitChat(pInfo->tszJid, pInfo->tszNick);
			pInfo->bActive = true;
		}
	}

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, 0 };
	if (!subject.empty()) {
		gcd.iType = GC_EVENT_TOPIC;
		pInfo->tszOwner = mir_utf8decodeT(owner.c_str());

		ptrT tszSubject(mir_utf8decodeT(subject.c_str()));
		ptrT tszSubjectOwner(mir_utf8decodeT(subject_owner.c_str()));

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
			pInfo = new WAChatInfo(msg.key.remote_jid.c_str(), msg.notifyname.c_str());
			m_chats[msg.key.remote_jid] = pInfo;

			InitChat(pInfo->tszJid, pInfo->tszNick);
			pInfo->bActive = true;
		}
	}

	ptrT tszText(mir_utf8decodeT(msg.data.c_str()));
	ptrT tszUID(mir_utf8decodeT(msg.remote_resource.c_str()));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_MESSAGE };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = pInfo->tszNick;
	gce.time = msg.timestamp;
	gce.ptszText = tszText;
	gce.bIsMe = m_szJid == msg.remote_resource;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);

	if (isOnline())
		m_pConnection->sendMessageReceived(msg);
}

void WhatsAppProto::onGroupNewSubject(const std::string &from, const std::string &author, const std::string &newSubject, int paramInt)
{
}

void WhatsAppProto::onGroupAddUser(const std::string &paramString1, const std::string &paramString2)
{
}

void WhatsAppProto::onGroupRemoveUser(const std::string &paramString1, const std::string &paramString2)
{
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

		ptrT ujid(mir_utf8decodeT(curr.c_str())), nick(GetChatUserNick(curr));
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

	ptrT tszUID(mir_utf8decodeT(m_szJid.c_str()));
	ptrT tszNick(mir_utf8decodeT(m_szNick.c_str()));

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
