#include "conversation.h"

CConversation::CConversation(unsigned int oid, SERootObject* root, CSkypeProto *proto) : Conversation(oid, root)
{ 
	this->proto = proto;
}

void CConversation::OnMessage(const MessageRef & message)
{
	/*if (this->proto)
		(proto->*onMessageCallback)(this->ref(), message->ref());*/
}

CConversation::Ref CConversation::FindBySid(Skype *skype, SEString sid)
{
	SEStringList participants;
	participants.append(sid);
	
	CConversation::Ref conversation;
	skype->GetConversationByParticipants(participants, conversation);

	return conversation;
}

//void CConversation::SetOnMessageCallback(OnMessaged callback, CSkypeProto* proto)
//{
//	this->proto = proto;
//	this->onMessageCallback = callback;
//}
