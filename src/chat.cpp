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
	WAChatInfo *pInfo = m_chats[msg.key.remote_jid];
	if (pInfo == NULL) {
		pInfo = new WAChatInfo(msg.key.remote_jid.c_str(), msg.notifyname.c_str());
		m_chats[msg.key.remote_jid] = pInfo;

		InitChat(pInfo->tszJid, pInfo->tszNick);
		pInfo->bActive = true;

		if (m_pConnection)
			m_pConnection->sendGetParticipants(msg.key.remote_jid);
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

void WhatsAppProto::onGroupInfo(const std::string& gjid, const std::string& ownerJid, const std::string& subject, const std::string& createrJid, int paramInt1, int paramInt2)
{
	debugLogA("'%s', '%s', '%s', '%s'", gjid.c_str(), ownerJid.c_str(), subject.c_str(), createrJid.c_str());
	MCONTACT hContact = ContactIDToHContact(gjid);
	if (!hContact) {
		debugLogA("Group info requested for non existing contact '%s'", gjid.c_str());
		return;
	}
	setByte(hContact, "SimpleChatRoom", ownerJid.compare(m_szJid) == 0 ? 2 : 1);
	if (isOnline())
		m_pConnection->sendGetParticipants(gjid);
}

void WhatsAppProto::onGroupInfoFromList(const std::string& paramString1, const std::string& paramString2, const std::string& paramString3, const std::string& paramString4, int paramInt1, int paramInt2)
{
	// Called before onOwningGroups() or onParticipatingGroups() is called!
}

void WhatsAppProto::onGroupNewSubject(const std::string& from, const std::string& author, const std::string& newSubject, int paramInt)
{
	debugLogA("'%s', '%s', '%s'", from.c_str(), author.c_str(), newSubject.c_str());
	AddToContactList(from, false, newSubject.c_str());
}

void WhatsAppProto::onGroupAddUser(const std::string& paramString1, const std::string& paramString2)
{
	debugLogA("%s - user: %s", paramString1.c_str(), paramString2.c_str());
	MCONTACT hContact = this->AddToContactList(paramString1);
	TCHAR *ptszGroupName = pcli->pfnGetContactDisplayName(hContact, 0);

	if (paramString2.compare(m_szJid) == 0) {
		this->NotifyEvent(ptszGroupName, TranslateT("You have been added to the group"), hContact, WHATSAPP_EVENT_OTHER);
		setByte(hContact, "IsGroupMember", 1);
	}
	else {
		CMString tmp(FORMAT, TranslateT("User '%s' has been added to the group"), this->GetContactDisplayName(paramString2));
		this->NotifyEvent(ptszGroupName, tmp, hContact, WHATSAPP_EVENT_OTHER);
	}

	if (isOnline())
		m_pConnection->sendGetGroupInfo(paramString1);
}

void WhatsAppProto::onGroupRemoveUser(const std::string &paramString1, const std::string &paramString2)
{
	debugLogA("%s - user: %s", paramString1.c_str(), paramString2.c_str());
	MCONTACT hContact = this->ContactIDToHContact(paramString1);
	if (!hContact)
		return;

	TCHAR *ptszGroupName = pcli->pfnGetContactDisplayName(hContact, 0);

	if (paramString2.compare(m_szJid) == 0) {
		//db_set_b(hContact, "CList", "Hidden", 1);
		setByte(hContact, "IsGroupMember", 0);

		this->NotifyEvent(ptszGroupName, TranslateT("You have been removed from the group"), hContact, WHATSAPP_EVENT_OTHER);
	}
	else if (isOnline()) {
		CMString tmp(FORMAT, TranslateT("User '%s' has been removed from the group"), this->GetContactDisplayName(paramString2));
		this->NotifyEvent(ptszGroupName, tmp, hContact, WHATSAPP_EVENT_OTHER);

		m_pConnection->sendGetGroupInfo(paramString1);
	}
}

void WhatsAppProto::onLeaveGroup(const std::string &paramString)
{
	// Won't be called for unknown reasons!
	debugLogA("%s", this->GetContactDisplayName(paramString));
	MCONTACT hContact = this->ContactIDToHContact(paramString);
	if (hContact)
		setByte(hContact, "IsGroupMember", 0);
}

void WhatsAppProto::onGetParticipants(const std::string& gjid, const std::vector<string>& participants)
{
	debugLogA("%s", this->GetContactDisplayName(gjid));

	MCONTACT hUserContact, hContact = this->ContactIDToHContact(gjid);
	if (!hContact)
		return;

	if (db_get_b(hContact, "CList", "Hidden", 0) == 1)
		return;

	bool isHidden = true;
	bool isOwningGroup = getByte(hContact, "SimpleChatRoom", 0) == 2;

	if (isOwningGroup)
		this->isMemberByGroupContact[hContact].clear();

	for (std::vector<string>::const_iterator it = participants.begin(); it != participants.end(); ++it) {
		// Hide, if we are not member of the group
		// Sometimes the group is shown shortly after hiding it again, due to other threads which stored the contact
		//	 in a cache before it has been removed (E.g. picture-id list in processBuddyList)
		if (isHidden && m_szJid.compare(*it) == 0) {
			isHidden = false;
			if (!isOwningGroup) {
				// Break, as we don't need to collect group-members
				break;
			}
		}

		// #TODO Slow for big count of participants
		// #TODO If a group is hidden it has been deleted from the local contact list
		//			 => don't allow to add users anymore
		if (isOwningGroup) {
			hUserContact = this->ContactIDToHContact(*it);
			if (hUserContact)
				this->isMemberByGroupContact[hContact][hUserContact] = true;
		}
	}
	if (isHidden) {
		//db_set_b(hContact, "CList", "Hidden", 1);
		// #TODO Check if it's possible to reach this point at all
		setByte(hContact, "IsGroupMember", 0);
	}
}

void WhatsAppProto::onOwningGroups(const std::vector<string>& paramVector)
{
	HandleReceiveGroups(paramVector, true);
}

void WhatsAppProto::onParticipatingGroups(const std::vector<string>& paramVector)
{
	HandleReceiveGroups(paramVector, false);
}

void WhatsAppProto::HandleReceiveGroups(const std::vector<string>& groups, bool isOwned)
{
	map<MCONTACT, bool> isMember; // at the moment, only members of owning groups are stored

	// This could take long time if there are many new groups which aren't
	//	 yet stored to the database. But that should be a rare case
	for (std::vector<string>::const_iterator it = groups.begin(); it != groups.end(); ++it) {
		MCONTACT hContact = AddToContactList(*it);
		setByte(hContact, "IsGroupMember", 1);
		if (isOwned) {
			this->isMemberByGroupContact[hContact]; // []-operator creates entry, if it doesn't exist
			setByte(hContact, "SimpleChatRoom", 2);
			m_pConnection->sendGetParticipants(*it);
		}
		else isMember[hContact] = true;
	}

	// Mark as non-meber if group only exists locally
	if (!isOwned)
		for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName))
			if (!isChatRoom(hContact) && getByte(hContact, "SimpleChatRoom", 0) > 0)
				setByte(hContact, "IsGroupMember", isMember.find(hContact) == isMember.end() ? 0 : 1);
}

void WhatsAppProto::onGroupCreated(const std::string& paramString1, const std::string& paramString2)
{
	// Must be received after onOwningGroups()
	debugLogA("%s / %s", paramString1.c_str(), paramString2.c_str());
	string jid = paramString2 + string("@") + paramString1;
	MCONTACT hContact = this->AddToContactList(jid);
	setByte(hContact, "SimpleChatRoom", 2);
}
