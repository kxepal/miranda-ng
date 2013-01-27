//#include "account.h"
#include "skype.h"

CAccount::CAccount(unsigned int oid, SERootObject* root, CSkypeProto *proto) : Account(oid, root)
{
	this->skype = (CSkype*)root;
	this->proto = proto;	
}

void CAccount::LoadContactList()
{
	CGroup::Refs customGroups;
	this->skype->GetCustomContactGroups(customGroups);

	this->skype->GetHardwiredContactGroup(CGroup::ALL_BUDDIES, this->commonGroup);
	this->commonGroup.fetch();

	this->commonGroup->GetContacts(this->contactList);
	Sid::fetch(this->contactList);	

	for (unsigned int i = 0; i < this->contactList.size(); i++)
	{
		CContact::Ref contact = this->contactList[i];
		contact->SaveContact();
		
	}
}

void CAccount::OnChange(int prop)
{
	switch(prop)
	{
	case CAccount::P_STATUS:
		CAccount::STATUS loginStatus;
		this->GetPropStatus(loginStatus);
	
		if (loginStatus == CAccount::LOGGED_IN)
		{
			//this->ForkThread(&CSkypeProto::SignInAsync, 0);
			//this->SignInAsync(this);
		}

		if (loginStatus == CAccount::LOGGED_OUT)
		{
			CAccount::LOGOUTREASON whyLogout;
			this->GetPropLogoutreason(whyLogout);
			if (whyLogout != CAccount::LOGOUT_CALLED)
			{
				/*this->m_iStatus = ID_STATUS_OFFLINE;
				this->SendBroadcast(
					ACKTYPE_LOGIN, 
					ACKRESULT_FAILED, 
					NULL, 
					this->SkypeToMirandaLoginError(whyLogout));

				this->ShowNotification(CSkypeProto::LogoutReasons[whyLogout - 1]);

				if (this->rememberPassword && whyLogout == CAccount::INCORRECT_PASSWORD)
				{
					this->rememberPassword = false;
					if (this->password) 
						::mir_free(this->password);
				}*/
			}
		}
		break;

	case CAccount::P_PWDCHANGESTATUS:
		{
			CAccount::PWDCHANGESTATUS status;
			this->GetPropPwdchangestatus(status);
			/*if (status != CAccount::PWD_CHANGING)
				this->ShowNotification(CSkypeProto::PasswordChangeReasons[status]);*/
		}
		break;

	//case CAccount::P_AVATAR_IMAGE:
	case CAccount::P_AVATAR_TIMESTAMP:
		//this->UpdateProfileAvatar(this->account.fetch());
		break;

	//case CAccount::P_MOOD_TEXT:
	case CAccount::P_MOOD_TIMESTAMP:
		//this->UpdateProfileStatusMessage(this->account.fetch());
		break;

	case CAccount::P_PROFILE_TIMESTAMP:
		//this->UpdateProfile(this->account.fetch());
		break;

	case CAccount::P_AVAILABILITY:
		{
			CContact::AVAILABILITY status;
			this->GetPropAvailability(status);
			/*if (status != CContact::CONNECTING && status >= CContact::ONLINE)
				this->SetStatus(this->SkypeToMirandaStatus(status));*/
		}
		break;
	}
}