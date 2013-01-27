#pragma once

#include "common.h"

#include "group.h"
#include "search.h"
#include "account.h"
#include "contact.h"
#include "message.h"
#include "transfer.h"
#include "participant.h"
#include "conversation.h"

class CSkype : public Skype
{
public:
	CSkype(int num_threads = 1);

	CGroup			*newContactGroup(int oid);
	CSearch			*newContactSearch(int oid);
	CAccount		*newAccount(int oid);
	CContact		*newContact(int oid);
	CMessage		*newMessage(int oid);
	CTransfer		*newTransfer(int oid);
	CParticipant	*newParticipant(int oid);
	CConversation	*newConversation(int oid);

	static CSkype *GetInstance(HINSTANCE hInstance, const wchar_t *profileName, const wchar_t *dbPath, CSkypeProto *proto);

private:
	CSkypeProto *proto;

	void OnMessage(
		const MessageRef &message,
		const bool &changesInboxTimestamp,
		const MessageRef &supersedesHistoryMessage,
		const ConversationRef &conversation);

	static BOOL IsRunAsAdmin();
	static char *LoadKeyPair(HINSTANCE hInstance);
	static int	StartSkypeRuntime(HINSTANCE hInstance, const wchar_t *profileName, int &port, const wchar_t *dbPath);
};