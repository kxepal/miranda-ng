#include "common.h"

void CToxProto::OnFriendRequest(uint8_t *userId, uint8_t *message, uint16_t messageSize, void *arg)
{
}

void CToxProto::OnFriendMessage(Tox *tox, int friendId, uint8_t *message, uint16_t messageSize, void *arg)
{
}

void CToxProto::OnFriendNameChange(Tox *tox, int friendId, uint8_t *name, uint16_t nameSize, void *arg)
{
}

void CToxProto::OnStatusMessageChanged(Tox *tox, int friendId, uint8_t* message, uint16_t messageSize, void *arg)
{
}

void CToxProto::OnUserStatusChanged(Tox *tox, int friendId, TOX_USERSTATUS userStatus, void *arg)
{
}

void CToxProto::OnConnectionStatusChanged(Tox *tox, int friendId, uint8_t status, void *arg)
{
}

void CToxProto::OnAction(Tox *tox, int friendId, uint8_t *message, uint16_t messageSize, void *arg)
{
}