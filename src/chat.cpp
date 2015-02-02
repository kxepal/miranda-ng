#include "common.h"

static const TCHAR *sttStatuses[] = { LPGENT("Members"), LPGENT("Owners") };

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

			if (m_pConnection)
				m_pConnection->sendGetParticipants(msg.key.remote_jid);
		}
	}

	ptrT tszText(mir_utf8decodeT(msg.data.c_str()));
	ptrT tszUID(mir_utf8decodeT(msg.remote_resource.c_str()));

	GCDEST gcd = { m_szModuleName, pInfo->tszJid, 0 };

	GCEVENT gce = { sizeof(gce), &gcd };
	gce.dwFlags = GCEF_ADDTOLOG;
	gce.ptszUID = tszUID;
	gce.ptszNick = pInfo->tszNick;
	gce.time = msg.timestamp;
	gce.ptszText = tszText;
	gce.bIsMe = m_szJid == msg.remote_resource;
	CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
}

int WhatsAppProto::OnChatOutgoing(WPARAM wParam, LPARAM lParam)
{
	GCHOOK *hook = reinterpret_cast<GCHOOK*>(lParam);
	char *text;

	if (strcmp(hook->pDest->pszModule, m_szModuleName))
		return 0;

	switch (hook->pDest->iType) {
	case GC_USER_MESSAGE:
		text = mir_t2a_cp(hook->ptszText, CP_UTF8);
		{
			std::string msg = text;

			char *id = mir_t2a_cp(hook->pDest->ptszID, CP_UTF8);
			std::string chat_id = id;

			mir_free(text);
			mir_free(id);

			if (isOnline()) {
				MCONTACT hContact = this->ContactIDToHContact(chat_id);
				if (hContact) {
					debugLogA("**Chat - Outgoing message: %s", text);
					this->SendMsg(hContact, IS_CHAT, msg.c_str());

					GCDEST gcd = { m_szModuleName, hook->pDest->ptszID, GC_EVENT_MESSAGE };
					GCEVENT gce = { sizeof(gce), &gcd };
					gce.dwFlags = GCEF_ADDTOLOG;
					gce.ptszNick = mir_a2t(m_szNick.c_str());
					gce.ptszUID = mir_a2t(m_szJid.c_str());
					gce.time = time(NULL);
					gce.ptszText = hook->ptszText;
					gce.bIsMe = TRUE;
					CallServiceSync(MS_GC_EVENT, 0, (LPARAM)&gce);

					mir_free((void*)gce.ptszUID);
					mir_free((void*)gce.ptszNick);
				}
			}
		}
		break;

	case GC_USER_LEAVE:
	case GC_SESSION_TERMINATE:
		break;
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

void WhatsAppProto::SendGetGroupInfoWorker(void* data)
{
	if (isOnline())
		m_pConnection->sendGetGroupInfo(*((std::string*) data));
}

///////////////////////////////////////////////////////////////////////////////
// WAConnection members

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

	if (!subject.empty()) {
		GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_TOPIC };

		pInfo->tszOwner = mir_utf8decodeT(owner.c_str());

		ptrT tszSubject(mir_utf8decodeT(subject.c_str()));
		ptrT tszSubjectOwner(mir_utf8decodeT(subject_owner.c_str()));
		TCHAR *p = _tcschr(tszSubjectOwner, '@');
		if (p) *p = 0;

		GCEVENT gce = { sizeof(gce), &gcd };
		gce.ptszUID = pInfo->tszOwner;
		gce.ptszNick = tszSubjectOwner;
		gce.time = time_subject;
		gce.dwFlags = GCEF_ADDTOLOG;
		gce.ptszText = tszSubject;
		CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}

	if (isOnline())
		m_pConnection->sendGetParticipants(jid);
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

		ptrT ujid(mir_utf8decodeT(curr.c_str())), nick;
		bool bIsOwner = !mir_tstrcmp(ujid, pInfo->tszOwner);
		TCHAR *p = _tcschr(ujid, '@');
		if (p) *p = 0;

		if (m_szNick == curr)
			nick = getTStringA("Nick");
		else {
			MCONTACT hContact = ContactIDToHContact(curr);
			nick = mir_tstrdup((hContact == 0) ? ujid : pcli->pfnGetContactDisplayName(hContact, 0));
		}

		GCDEST gcd = { m_szModuleName, pInfo->tszJid, GC_EVENT_JOIN };

		GCEVENT gce = { sizeof(gce), &gcd };
		gce.ptszNick = nick;
		gce.ptszUID = ujid;
		gce.ptszStatus = (bIsOwner) ? _T("Owners") : _T("Members");
		CallServiceSync(MS_GC_EVENT, NULL, (LPARAM)&gce);
	}
}

void WhatsAppProto::onGroupCreated(const std::string &paramString1, const std::string &paramString2)
{
}
