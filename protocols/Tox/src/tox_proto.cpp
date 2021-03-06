#include "stdafx.h"

CToxProto::CToxProto(const char* protoName, const TCHAR* userName)
	: PROTO<CToxProto>(protoName, userName),
	hPollingThread(NULL), toxThread(NULL),
	hOutDevice(NULL), hMessageProcess(1)
{
	InitNetlib();

	accountName = mir_tstrdup(userName);

	CreateProtoService(PS_CREATEACCMGRUI, &CToxProto::OnAccountManagerInit);

	SetAllContactsStatus(ID_STATUS_OFFLINE);

	// services
	CreateProtoService(PSR_AUDIO, &CToxProto::OnRecvAudioCall);
	CreateProtoService("/Audio/Ring", &CToxProto::OnAudioRing);

	// avatars
	CreateProtoService(PS_GETAVATARCAPS, &CToxProto::GetAvatarCaps);
	CreateProtoService(PS_GETAVATARINFO, &CToxProto::GetAvatarInfo);
	CreateProtoService(PS_GETMYAVATAR, &CToxProto::GetMyAvatar);
	CreateProtoService(PS_SETMYAVATAR, &CToxProto::SetMyAvatar);

	// nick
	CreateProtoService(PS_SETMYNICKNAME, &CToxProto::SetMyNickname);

	hAudioDialogs = WindowList_Create();
}

CToxProto::~CToxProto()
{
	WindowList_Destroy(hAudioDialogs);

	mir_free(accountName);
	UninitNetlib();
}

DWORD_PTR CToxProto::GetCaps(int type, MCONTACT)
{
	switch (type)
	{
	case PFLAGNUM_1:
		return PF1_IM | PF1_FILE | PF1_AUTHREQ | PF1_MODEMSG | PF1_EXTSEARCH | PF1_SERVERCLIST;
	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LIGHTDND;
	case PFLAGNUM_3:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LIGHTDND;
	case PFLAGNUM_4:
		return PF4_SINGLEFILEONLY | PF4_SUPPORTTYPING | PF4_AVATARS | PF4_FORCEADDED | PF4_NOAUTHDENYREASON | PF4_FORCEAUTH;
	case PFLAG_UNIQUEIDTEXT:
		return (INT_PTR)"Tox ID";
	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR)TOX_SETTINGS_ID;
	case PFLAG_MAXLENOFMESSAGE:
		return TOX_MAX_MESSAGE_LENGTH;
	}

	return 0;
}

MCONTACT CToxProto::AddToList(int flags, PROTOSEARCHRESULT *psr)
{
	ptrA myAddress(getStringA(NULL, TOX_SETTINGS_ID));
	if (strnicmp(psr->id.a, myAddress, TOX_PUBLIC_KEY_SIZE) == 0)
	{
		ShowNotification(TranslateT("You cannot add yourself to your contact list"), 0);
		return NULL;
	}
	if (MCONTACT hContact = GetContact(psr->id.a))
	{
		ShowNotification(TranslateT("Contact already in your contact list"), 0, hContact);
		return NULL;
	}
	return AddContact(psr->id.a, psr->nick.a, psr->email.a, flags & PALF_TEMPORARY);
}

int CToxProto::Authorize(MEVENT hDbEvent)
{
	MCONTACT hContact = GetContactFromAuthEvent(hDbEvent);
	if (hContact == INVALID_CONTACT_ID)
		return 1;

	return OnGrantAuth(hContact, 0);
}

int CToxProto::AuthRecv(MCONTACT, PROTORECVEVENT* pre)
{
	return Proto_AuthRecv(m_szModuleName, pre);
}

int CToxProto::AuthRequest(MCONTACT hContact, const TCHAR *szMessage)
{
	ptrA reason(mir_utf8encodeW(szMessage));
	return OnRequestAuth(hContact, (LPARAM)reason);
}

HANDLE CToxProto::FileAllow(MCONTACT hContact, HANDLE hTransfer, const TCHAR *tszPath)
{
	return OnFileAllow(hContact, hTransfer, tszPath);
}

int CToxProto::FileCancel(MCONTACT hContact, HANDLE hTransfer)
{
	return OnFileCancel(hContact, hTransfer);
}

int CToxProto::FileDeny(MCONTACT hContact, HANDLE hTransfer, const TCHAR*)
{
	return FileCancel(hContact, hTransfer);
}

int CToxProto::FileResume(HANDLE hTransfer, int *action, const TCHAR **szFilename)
{
	return OnFileResume(hTransfer, action, szFilename);
}

HWND CToxProto::SearchAdvanced(HWND owner)
{
	return OnSearchAdvanced(owner);
}

HWND CToxProto::CreateExtendedSearchUI(HWND owner)
{
	return OnCreateExtendedSearchUI(owner);
}

int CToxProto::SendMsg(MCONTACT hContact, int, const char *msg)
{
	return OnSendMessage(hContact, msg);
}

HANDLE CToxProto::SendFile(MCONTACT hContact, const TCHAR *msg, TCHAR **ppszFiles)
{
	return OnSendFile(hContact, msg, ppszFiles);
}

int CToxProto::SetStatus(int iNewStatus)
{
	if (iNewStatus == m_iDesiredStatus)
		return 0;

	iNewStatus = MapStatus(iNewStatus);

	logger->Log("CToxProto::SetStatus: changing status from %i to %i", m_iStatus, iNewStatus);

	int old_status = m_iStatus;
	m_iDesiredStatus = iNewStatus;

	if (iNewStatus == ID_STATUS_OFFLINE)
	{
		// logout
		if (toxThread)
		{
			toxThread->Stop();
			toxThread = NULL;
		}

		if (!Miranda_Terminated())
		{
			SetAllContactsStatus(ID_STATUS_OFFLINE);
			CloseAllChatChatSessions();
		}

		m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);
		return 0;
	}

	if (old_status == ID_STATUS_CONNECTING)
		return 0;

	if (old_status == ID_STATUS_OFFLINE && !IsOnline())
	{
		// login
		m_iStatus = ID_STATUS_CONNECTING;

		hPollingThread = ForkThreadEx(&CToxProto::PollingThread, NULL, NULL);
	}
	else
	{
		// set tox status
		m_iStatus = iNewStatus;
		tox_self_set_status(toxThread->tox, MirandaToToxStatus(iNewStatus));
	}

	ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);
	return 0;
}

HANDLE CToxProto::GetAwayMsg(MCONTACT hContact)
{
	if (IsOnline())
	{
		ForkThread(&CToxProto::GetStatusMessageAsync, (void*)hContact);
		return (HANDLE)hContact;
	}

	return 0;
}

int CToxProto::SetAwayMsg(int, const TCHAR *msg)
{
	if (IsOnline())
	{
		T2Utf statusMessage(msg);
		TOX_ERR_SET_INFO error;
		if (!tox_self_set_status_message(toxThread->tox, (uint8_t*)(char*)statusMessage, min(TOX_MAX_STATUS_MESSAGE_LENGTH, mir_strlen(statusMessage)), &error))
			logger->Log("CToxProto::SetAwayMsg: failed to set status status message %s (%d)", msg, error);
	}

	return 0;
}

int CToxProto::UserIsTyping(MCONTACT hContact, int type)
{
	return OnUserIsTyping(hContact, type);
}

int CToxProto::OnEvent(PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam)
{
	switch (iEventType)
	{
	case EV_PROTO_ONLOAD:
		return OnAccountLoaded(wParam, lParam);

	case EV_PROTO_ONRENAME:
		return OnAccountRenamed(wParam, lParam);

	case EV_PROTO_ONCONTACTDELETED:
		return OnContactDeleted(wParam, lParam);

	case EV_PROTO_ONMENU:
		return OnInitStatusMenu();
	}

	return 1;
}
