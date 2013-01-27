#include "group.h"

CGroup::CGroup(unsigned int oid, SERootObject* root, CSkypeProto *proto) : ContactGroup(oid, root)
{ 
	this->proto = proto;
}

void CGroup::OnChange(const ContactRef& contact)
{
	/*if (this->proto)
		(proto->*callback)(contact);*/
}