#include "message.h"

CMessage::CMessage(unsigned int oid, SERootObject* root, CSkypeProto *proto) : Message(oid, root)
{ 
	this->proto = proto;
}