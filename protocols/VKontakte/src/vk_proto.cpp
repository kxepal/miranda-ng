/*
Copyright (C) 2013 Miranda NG Project (http://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"

CVkProto::CVkProto(const char *szModuleName, const TCHAR *ptszUserName) :
	PROTO<CVkProto>(szModuleName, ptszUserName),
	m_arRequestsQueue(10)
{
	InitQueue();

	CreateProtoService(PS_CREATEACCMGRUI, &CVkProto::SvcCreateAccMgrUI);

	TCHAR descr[512];
	mir_sntprintf(descr, SIZEOF(descr), TranslateT("%s server connection"), m_tszUserName);

	NETLIBUSER nlu = {sizeof(nlu)};
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS | NUF_TCHAR;
	nlu.szSettingsModule = m_szModuleName;
	nlu.szSettingsModule = m_szModuleName;
	nlu.ptszDescriptiveName = descr;
	m_hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	mir_sntprintf(descr, SIZEOF(descr), _T("%%miranda_avatarcache%%\\%s"), m_tszUserName);
	hAvatarFolder = FoldersRegisterCustomPathT(LPGEN("Avatars"), m_szModuleName, descr, m_tszUserName);

	m_defaultGroup = getTStringA("ProtoGroup");
	if (m_defaultGroup == NULL)
		m_defaultGroup = mir_tstrdup( TranslateT("VKontakte"));
	Clist_CreateGroup(NULL, m_defaultGroup);

	db_set_resident(m_szModuleName, "Status");

	// Set all contacts offline -- in case we crashed
	SetAllContactStatuses(ID_STATUS_OFFLINE);
}

CVkProto::~CVkProto()
{
	UninitQueue();
}

int CVkProto::OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

int CVkProto::OnPreShutdown(WPARAM wParam, LPARAM lParam)
{
	m_bTerminated = true;
	SetEvent(m_evRequestsQueue);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

DWORD_PTR CVkProto::GetCaps(int type, HANDLE hContact)
{
	switch(type) {
	case PFLAGNUM_1:
		return PF1_IM | PF1_CHAT | PF1_SERVERCLIST | PF1_AUTHREQ | PF1_BASICSEARCH | PF1_SEARCHBYEMAIL | PF1_SEARCHBYNAME | PF1_MODEMSG;

	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_INVISIBLE | PF2_ONTHEPHONE | PF2_IDLE; // | PF2_SHORTAWAY;

	case PFLAGNUM_3:
		return PF2_ONLINE; // | PF2_SHORTAWAY;

	case PFLAGNUM_4:
		return PF4_NOCUSTOMAUTH | PF4_FORCEADDED | PF4_IMSENDUTF | PF4_AVATARS | PF4_SUPPORTTYPING | PF4_NOAUTHDENYREASON | PF4_IMSENDOFFLINE;

	case PFLAGNUM_5:
		return PF2_ONTHEPHONE;

	case PFLAG_MAXLENOFMESSAGE:
		return 2000;

	case PFLAG_UNIQUEIDTEXT:
		return (DWORD_PTR)"VK ID";

	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR)"ID";
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int CVkProto::SetStatus(int iNewStatus)
{
	if (m_iDesiredStatus == iNewStatus)
		return 0;

	int oldStatus = m_iStatus;
	m_iDesiredStatus = iNewStatus;

	if (iNewStatus == ID_STATUS_OFFLINE) {
		if ( IsOnline())
			ShutdownSession();

		m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)oldStatus, m_iStatus);
	}
	else if ( !(m_iStatus >= ID_STATUS_CONNECTING && m_iStatus < ID_STATUS_CONNECTING + MAX_CONNECT_RETRIES)) {
		m_iStatus = ID_STATUS_CONNECTING;
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)oldStatus, m_iStatus);
		m_hWorkerThread = ForkThreadEx(&CVkProto::WorkerThread, 0, NULL);
	}
	else if ( IsOnline())
		SetServerStatus(iNewStatus);
	else
		ProtoBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)oldStatus, m_iStatus);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int CVkProto::OnEvent(PROTOEVENTTYPE event, WPARAM wParam, LPARAM lParam)
{
	switch(event) {
	case EV_PROTO_ONLOAD:
		return OnModulesLoaded(wParam,lParam);

	case EV_PROTO_ONEXIT:
		return OnPreShutdown(wParam,lParam);

	case EV_PROTO_ONOPTIONS:
		return OnOptionsInit(wParam,lParam);
	}

	return 1;
}

//////////////////////////////////////////////////////////////////////////////

HANDLE CVkProto::SearchBasic(const PROTOCHAR* id)
{
	return 0;
}

HANDLE CVkProto::SearchByEmail(const PROTOCHAR* email)
{
	return 0;
}

HANDLE CVkProto::SearchByName(const PROTOCHAR* nick, const PROTOCHAR* firstName, const PROTOCHAR* lastName)
{
	return 0;
}

HANDLE CVkProto::AddToList(int flags, PROTOSEARCHRESULT* psr)
{
	return NULL;
}

int CVkProto::AuthRequest(HANDLE hContact,const PROTOCHAR *message)
{
	return 0;
}

int CVkProto::Authorize(HANDLE hDbEvent)
{
	//if (!hDbEvent)
	return 1;
}

int CVkProto::AuthDeny(HANDLE hDbEvent, const PROTOCHAR *reason)
{
	//if (!hDbEvent || isOffline())
	return 1;
}

int CVkProto::RecvMsg(HANDLE hContact, PROTORECVEVENT *pre)
{ 
	return 0;
}

int CVkProto::SendMsg(HANDLE hContact, int flags, const char *msg)
{ 
	return 0;
}

int CVkProto::UserIsTyping(HANDLE hContact, int type)
{ 
	return 0;
}

HANDLE CVkProto::AddToListByEvent(int flags,int iContact,HANDLE hDbEvent)
{
	return NULL;
}

int CVkProto::AuthRecv(HANDLE hContact,PROTORECVEVENT *)
{
	return 1;
}

HANDLE CVkProto::ChangeInfo(int type,void *info_data)
{
	MessageBoxA(0,"ChangeInfo","",0);
	return NULL;
}

HANDLE CVkProto::FileAllow(HANDLE hContact,HANDLE hTransfer,const PROTOCHAR *path)
{
	return NULL;
}

int CVkProto::FileCancel(HANDLE hContact,HANDLE hTransfer)
{
	return 1;
}

int CVkProto::FileDeny(HANDLE hContact,HANDLE hTransfer,const PROTOCHAR *reason)
{
	return 1;
}

int CVkProto::FileResume(HANDLE hTransfer,int *action,const PROTOCHAR **filename)
{
	return 1;
}

int CVkProto::GetInfo(HANDLE hContact, int infoType)
{
	// TODO: Most probably some ProtoAck should be here instead
	return 1;
}

HWND CVkProto::SearchAdvanced(HWND owner)
{
	return NULL;
}

HWND CVkProto::CreateExtendedSearchUI(HWND owner)
{
	return NULL;
}

int CVkProto::RecvContacts(HANDLE hContact,PROTORECVEVENT *)
{
	return 1;
}

int CVkProto::RecvFile(HANDLE hContact,PROTORECVFILET *)
{
	return 1;
}

int CVkProto::RecvUrl(HANDLE hContact,PROTORECVEVENT *)
{
	return 1;
}

int CVkProto::SendContacts(HANDLE hContact,int flags,int nContacts,HANDLE *hContactsList)
{
	return 1;
}

HANDLE CVkProto::SendFile(HANDLE hContact,const PROTOCHAR *desc, PROTOCHAR **files)
{
	return NULL;
}

int CVkProto::SendUrl(HANDLE hContact,int flags,const char *url)
{
	return 1;
}

int CVkProto::SetApparentMode(HANDLE hContact,int mode)
{
	return 1;
}

int CVkProto::RecvAwayMsg(HANDLE hContact,int mode,PROTORECVEVENT *evt)
{
	return 1;
}

HANDLE CVkProto::GetAwayMsg(HANDLE hContact)
{
	return 0; // Status messages are disabled
}

int CVkProto::SetAwayMsg(int status, const PROTOCHAR *msg)
{
	return 0; // Status messages are disabled
}
