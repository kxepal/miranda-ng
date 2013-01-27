#include "contact.h"

//#include <m_core.h>
#include <m_database.h>
////#include <m_protoint.h>
//#include <m_protocols.h>

#include "..\skype_proto.h"

CContact::CContact(unsigned int oid, SERootObject* root, CSkypeProto *proto) : Contact(oid, root) 
{ 
	this->proto = proto;
}

//void CContact::SetOnContactChangedCallback(OnContactChanged callback, CSkypeProto* proto)
//{
//	this->proto = proto;
//	this->callback = callback;
//}

//bool CContact::SentAuthRequest(SEString message)
//{
//	this->SetBuddyStatus(Contact::AUTHORIZED_BY_ME);
//	this->SendAuthRequest(message);
//}

bool CContact::IsProtoContact(HANDLE hContact)
{
	/*return ::CallService(
		MS_PROTO_ISPROTOONCONTACT, 
		(WPARAM)hContact, 
		(LPARAM)this->proto->m_szModuleName) < 0;*/
	return false;
}

bool CContact::IsChatRoom(HANDLE hContact)
{
	return ::DBGetContactSettingByte(hContact, this->proto->m_szModuleName, "ChatRoom", 0) > 0;
}

HANDLE CContact::GetHandleBySid(const char *sid)
{
	//HANDLE hContact = ::db_find_first();
	//do
	//{
	//	if  (this->IsProtoContact(hContact) && !this->IsChatRoom(hContact))
	//	{
	//		auto contactSid = ::DBGetString(hContact, this->proto->m_szModuleName, "sid");
	//		if (contactSid && ::stricmp(sid, contactSid) == 0)
	//			return hContact;
	//	}
	//} 
	//while (hContact = ::db_find_next(hContact));

	return 0;
}

void CContact::OnChange(int prop)
{
	/*if (this->proto)
		(proto->*callback)(this->ref(), prop);*/
}

void CContact::SaveContact()
{
	SEString data;
	/*SEString data;

	contact->GetPropSkypename(data);
	char *sid = ::mir_strdup(data);

	contact->GetPropDisplayname(data);
	char *nick = ::mir_utf8decodeA(data);

	contact->GetPropFullname(data);
	char *name = ::mir_utf8decodeA(data);

	DWORD flags = 0;
	CContact::AVAILABILITY availability;
	contact->GetPropAvailability(availability);

	if (availability == CContact::SKYPEOUT)
	{
		flags |= 256;
		contact->GetPropPstnnumber(data);
		::mir_free(sid);
		sid = ::mir_strdup((const char *)data);
	}
	else if (availability == CContact::PENDINGAUTH)
		flags = PALF_TEMPORARY;

	HANDLE hContact = this->AddContactBySid(sid, nick, flags);

	SEObject *obj = contact.fetch();
	this->UpdateContactAuthState(hContact, contact);
	this->UpdateContactStatus(hContact, contact);

	this->UpdateProfile(obj, hContact);
	this->UpdateProfileAvatar(obj, hContact);
	this->UpdateProfileStatusMessage(obj, hContact);*/
}