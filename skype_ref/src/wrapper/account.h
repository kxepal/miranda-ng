#pragma once

#include "common.h"
#include "contact.h"

class CSkype;

class CAccount : public Account
{
public:
	typedef DRef<CAccount, Account> Ref;
	typedef DRefs<CAccount, Account> Refs;

	Transfer::Refs TransferList;
	
	CAccount(unsigned int oid, SERootObject* root, CSkypeProto *proto);

private:
	CSkype *skype;
	CSkypeProto *proto;

	CGroup::Ref		commonGroup;
	CContact::Refs	contactList;

	void LoadContactList();

	void OnChange(int prop);
};