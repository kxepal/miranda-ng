#pragma once

#include "common.h"

class CContact : public Contact
{
friend class CAccount;

public:
	typedef DRef<CContact, Contact> Ref;
	typedef DRefs<CContact, Contact> Refs;

	CContact(unsigned int oid, SERootObject* root, CSkypeProto *proto);

	void SaveContact();

private:
	CSkypeProto *proto;

	HANDLE hContact;

	bool IsProtoContact(HANDLE hContact);
	bool IsChatRoom(HANDLE hContact);
	HANDLE GetHandleBySid(const char *sid);

	void OnChange(int prop);
};