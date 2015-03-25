#include "common.h"

/* MESSAGE RECEIVING */

// incoming message flow
void CToxProto::OnFriendMessage(Tox *tox, uint32_t friendNumber, TOX_MESSAGE_TYPE type, const uint8_t *message, size_t length, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact)
	{
		ptrA szMessage((char*)mir_alloc(length + 1));
		mir_strncpy(szMessage, (const char*)message, length + 1);

		PROTORECVEVENT recv = { 0 };
		recv.flags = PREF_UTF;
		recv.timestamp = time(NULL);
		recv.szMessage = szMessage;
		recv.lParam = type == TOX_MESSAGE_TYPE_NORMAL
			? EVENTTYPE_MESSAGE : TOX_DB_EVENT_TYPE_ACTION;

		ProtoChainRecvMsg(hContact, &recv);
	}
}

// writing message/even into db
int CToxProto::OnReceiveMessage(MCONTACT hContact, PROTORECVEVENT *pre)
{
	//return Proto_RecvMessage(hContact, pre);
	if (pre->szMessage == NULL)
		return NULL;

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = GetContactProto(hContact);
	dbei.timestamp = pre->timestamp;
	dbei.flags = DBEF_UTF;
	dbei.eventType = pre->lParam;
	dbei.cbBlob = (DWORD)strlen(pre->szMessage) + 1;
	dbei.pBlob = (PBYTE)pre->szMessage;

	return (INT_PTR)db_event_add(hContact, &dbei);
}

/* MESSAGE SENDING */

// outcoming message flow
int CToxProto::OnSendMessage(MCONTACT hContact, int flags, const char *szMessage)
{
	int32_t friendNumber = GetToxFriendNumber(hContact);
	if (friendNumber == UINT32_MAX)
		return 0;

	ptrA message;
	if (flags & PREF_UNICODE)
		message = mir_utf8encodeW((wchar_t*)&szMessage[mir_strlen(szMessage) + 1]);
	else if (flags & PREF_UTF)
		message = mir_strdup(szMessage);
	else
		message = mir_utf8encode(szMessage);

	size_t msgLen = mir_strlen(message);
	uint8_t *msg = (uint8_t*)(char*)message;
	TOX_MESSAGE_TYPE type = TOX_MESSAGE_TYPE_NORMAL;
	if (strncmp(message, "/me ", 4) == 0)
	{
		msg += 4; msgLen -= 4;
		type = TOX_MESSAGE_TYPE_ACTION;
	}
	TOX_ERR_FRIEND_SEND_MESSAGE error;
	int messageId = tox_friend_send_message(tox, friendNumber, type, msg, msgLen, &error);
	if (error != TOX_ERR_FRIEND_SEND_MESSAGE_OK)
	{
		debugLogA(__FUNCTION__": failed to send message (%d)", error);
	}
	return messageId;
}

// message is received by the other side
void CToxProto::OnReadReceipt(Tox *tox, uint32_t friendNumber, uint32_t messageId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact)
		proto->ProtoBroadcastAck(
			hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)messageId, 0);
}

// preparing message/action to writing into db
int CToxProto::OnPreCreateMessage(WPARAM, LPARAM lParam)
{
	MessageWindowEvent *evt = (MessageWindowEvent*)lParam;
	if (mir_strcmp(GetContactProto(evt->hContact), m_szModuleName))
		return 0;

	char *message = (char*)evt->dbei->pBlob;
	if (strncmp(message, "/me ", 4) == 0)
	{
		evt->dbei->cbBlob = evt->dbei->cbBlob - 4;
		PBYTE action = (PBYTE)mir_alloc(evt->dbei->cbBlob);
		memcpy(action, &evt->dbei->pBlob[4], evt->dbei->cbBlob);
		mir_free(evt->dbei->pBlob);
		evt->dbei->pBlob = action;
		evt->dbei->eventType = TOX_DB_EVENT_TYPE_ACTION;
	}

	return 1;
}

/* TYPING */

void CToxProto::OnTypingChanged(Tox *tox, uint32_t friendNumber, bool isTyping, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact)
	{
		CallService(MS_PROTO_CONTACTISTYPING, hContact, (LPARAM)isTyping);
	}
}