#pragma once

#include "common.h"

class CConversation : public Conversation
{
public:
	typedef DRef<CConversation, Conversation> Ref;
	typedef DRefs<CConversation, Conversation> Refs;

	CConversation(unsigned int oid, SERootObject* root, CSkypeProto *proto);

	static CConversation::Ref FindBySid(Skype *skype, SEString sid);

	//void SetOnMessageCallback(OnMessaged callback, CSkypeProto* proto);

private:
	CSkypeProto*	proto;
	//OnMessaged		onMessageCallback;
	
	void OnMessage(const MessageRef & message);
};