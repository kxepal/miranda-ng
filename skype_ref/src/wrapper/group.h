#pragma once

#include "common.h"

class CGroup : public ContactGroup
{
public:
	typedef DRef<CGroup, ContactGroup> Ref;
	typedef DRefs<CGroup, ContactGroup> Refs;
	CGroup(unsigned int oid, SERootObject* root, CSkypeProto *proto);

private:
	CSkypeProto *proto;
	void OnChange(const ContactRef& contact);
};