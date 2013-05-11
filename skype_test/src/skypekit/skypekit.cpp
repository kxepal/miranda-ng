#include "skypekit.h"

CSkypeKit::CSkypeKit(int num_threads) : Skype(num_threads)
{
	this->proto = NULL;
	this->onMessagedCallback = NULL;
}

CAccount* CSkypeKit::newAccount(int oid) 
{ 
	return new CAccount(oid, this); 
}

CContactGroup* CSkypeKit::newContactGroup(int oid)
{ 
	return new CContactGroup(oid, this); 
}

CContact* CSkypeKit::newContact(int oid) 
{ 
	return new CContact(oid, this); 
}

CConversation* CSkypeKit::newConversation(int oid) 
{ 
	return new CConversation(oid, this); 
}

CParticipant* CSkypeKit::newParticipant(int oid) 
{ 
	return new CParticipant(oid, this); 
}

CMessage* CSkypeKit::newMessage(int oid) 
{ 
	return new CMessage(oid, this); 
}

CTransfer* CSkypeKit::newTransfer(int oid) 
{ 
	return new CTransfer(oid, this); 
}

CContactSearch* CSkypeKit::newContactSearch(int oid)
{
	return new CContactSearch(oid, this);
}

bool CSkypeKit::CreateConferenceWithConsumers(ConversationRef &conference, const SEStringList &identities)
{
	if (this->CreateConference(conference))
	{
		conference->SetOption(CConversation::P_OPT_JOINING_ENABLED, true);
		conference->SetOption(CConversation::P_OPT_ENTRY_LEVEL_RANK, CParticipant::WRITER);
		conference->SetOption(CConversation::P_OPT_DISCLOSE_HISTORY, 1);
		conference->AddConsumers(identities);

		return true;
	}

	return false;
}

void CSkypeKit::SetOnMessageCallback(OnMessaged callback, CSkypeProto* proto)
{
	this->proto = proto;
	this->onMessagedCallback = callback;
}

void CSkypeKit::OnMessage (
	const MessageRef & message,
	const bool & changesInboxTimestamp,
	const MessageRef & supersedesHistoryMessage,
	const ConversationRef & conversation)
{
	if (this->proto && this->onMessagedCallback)
		(proto->*onMessagedCallback)(conversation, message);
}
