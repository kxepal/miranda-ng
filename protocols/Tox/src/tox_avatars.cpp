#include "stdafx.h"

TCHAR* CToxProto::GetAvatarFilePath(MCONTACT hContact)
{
	TCHAR *path = (TCHAR*)mir_calloc(MAX_PATH * sizeof(TCHAR) + 1);
	mir_sntprintf(path, MAX_PATH, _T("%s\\%S"), VARST(_T("%miranda_avatarcache%")), m_szModuleName);

	DWORD dwAttributes = GetFileAttributes(path);
	if (dwAttributes == 0xffffffff || (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		CreateDirectoryTreeT(path);

	ptrT address(getTStringA(hContact, TOX_SETTINGS_ID));
	if (address == NULL) {
		mir_free(path);
		return mir_tstrdup(_T(""));
	}

	if (hContact && mir_tstrlen(address) > TOX_PUBLIC_KEY_SIZE * 2)
		address[TOX_PUBLIC_KEY_SIZE * 2] = 0;
	mir_sntprintf(path, MAX_PATH, _T("%s\\%s.png"), path, address);

	return path;
}

void CToxProto::SetToxAvatar(const TCHAR* path)
{
	FILE *hFile = _tfopen(path, L"rb");
	if (!hFile)
	{
		logger->Log(__FUNCTION__": failed to open avatar file");
		return;
	}

	fseek(hFile, 0, SEEK_END);
	long length = ftell(hFile);
	rewind(hFile);
	if (length > TOX_MAX_AVATAR_SIZE)
	{
		fclose(hFile);
		logger->Log(__FUNCTION__": new avatar size is excessive");
		return;
	}

	uint8_t *data = (uint8_t*)mir_alloc(length);
	if (fread(data, sizeof(uint8_t), length, hFile) != length)
	{
		fclose(hFile);
		logger->Log(__FUNCTION__": failed to read avatar file");
		mir_free(data);
		return;
	}
	fclose(hFile);

	DBVARIANT dbv;
	uint8_t hash[TOX_HASH_LENGTH];
	tox_hash(hash, data, TOX_HASH_LENGTH);
	if (!db_get(NULL, m_szModuleName, TOX_SETTINGS_AVATAR_HASH, &dbv))
	{
		if (memcmp(hash, dbv.pbVal, TOX_HASH_LENGTH) == 0)
		{
			db_free(&dbv);
			mir_free(data);
			logger->Log(__FUNCTION__": new avatar is same with old");
			return;
		}
		db_free(&dbv);
	}

	db_set_blob(NULL, m_szModuleName, TOX_SETTINGS_AVATAR_HASH, (void*)hash, TOX_HASH_LENGTH);

	if (IsOnline())
	{
		for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName))
		{
			if (GetContactStatus(hContact) == ID_STATUS_OFFLINE)
				continue;

			int32_t friendNumber = GetToxFriendNumber(hContact);
			if (friendNumber == UINT32_MAX)
			{
				mir_free(data);
				logger->Log(__FUNCTION__": failed to set new avatar");
				return;
			}

			TOX_ERR_FILE_SEND error;
			uint32_t fileNumber = tox_file_send(toxThread->tox, friendNumber, TOX_FILE_KIND_AVATAR, length, hash, NULL, 0, &error);
			if (error != TOX_ERR_FILE_SEND_OK)
			{
				mir_free(data);
				logger->Log(__FUNCTION__": failed to set new avatar");
				return;
			}

			AvatarTransferParam *transfer = new AvatarTransferParam(friendNumber, fileNumber, NULL, length);
			transfer->pfts.flags |= PFTS_SENDING;
			memcpy(transfer->hash, hash, TOX_HASH_LENGTH);
			transfer->pfts.hContact = hContact;
			transfer->hFile = _tfopen(path, L"rb");
			transfers.Add(transfer);
		}
	}

	mir_free(data);
}

INT_PTR CToxProto::GetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case AF_ENABLED:
		return 1;

	case AF_FORMATSUPPORTED:
		return lParam == PA_FORMAT_PNG;

	case AF_MAXSIZE:
		((POINT*)lParam)->x = 300;
		((POINT*)lParam)->y = 300;
		return 0;

	case AF_MAXFILESIZE:
		return TOX_MAX_AVATAR_SIZE;
	}

	return 0;
}

INT_PTR CToxProto::GetAvatarInfo(WPARAM, LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *)lParam;

	ptrA address(getStringA(pai->hContact, TOX_SETTINGS_ID));
	if (address != NULL)
	{
		ptrT path(GetAvatarFilePath(pai->hContact));
		if (IsFileExists(path))
		{
			mir_tstrncpy(pai->filename, path, _countof(pai->filename));
			pai->format = PA_FORMAT_PNG;

			return GAIR_SUCCESS;
		}
	}

	return GAIR_NOAVATAR;
}

INT_PTR CToxProto::GetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	ptrT path(GetAvatarFilePath());
	if (IsFileExists(path))
		mir_tstrncpy((TCHAR*)wParam, path, (int)lParam);

	return 0;
}

INT_PTR CToxProto::SetMyAvatar(WPARAM, LPARAM lParam)
{
	logger->Log("CToxProto::SetMyAvatar: setting avatar");
	TCHAR *path = (TCHAR*)lParam;
	ptrT avatarPath(GetAvatarFilePath());
	if (path != NULL)
	{
		logger->Log("CToxProto::SetMyAvatar: copy new avatar");
		if (!CopyFile(path, avatarPath, FALSE))
		{
			logger->Log("CToxProto::SetMyAvatar: failed to copy new avatar to avatar cache");
			return 0;
		}

		SetToxAvatar(avatarPath);

		return 0;
	}

	if (IsOnline())
	{
		for (MCONTACT hContact = db_find_first(m_szModuleName); hContact; hContact = db_find_next(hContact, m_szModuleName))
		{
			if (GetContactStatus(hContact) == ID_STATUS_OFFLINE)
				continue;

			int32_t friendNumber = GetToxFriendNumber(hContact);
			if (friendNumber == UINT32_MAX)
				continue;

			TOX_ERR_FILE_SEND error;
			tox_file_send(toxThread->tox, friendNumber, TOX_FILE_KIND_AVATAR, 0, NULL, NULL, 0, &error);
			if (error != TOX_ERR_FILE_SEND_OK)
			{
				logger->Log(__FUNCTION__": failed to unset avatar (%d)", error);
				return 0;
			}
		}
	}

	if (IsFileExists(avatarPath))
		DeleteFile(avatarPath);

	delSetting(TOX_SETTINGS_AVATAR_HASH);

	return 0;
}

void CToxProto::OnGotFriendAvatarInfo(AvatarTransferParam *transfer)
{
	if (transfer->pfts.totalBytes == 0)
	{
		MCONTACT hConact = transfer->pfts.hContact;
		ptrT path(GetAvatarFilePath(hConact));
		if (IsFileExists(path))
			DeleteFile(path);

		transfers.Remove(transfer);
		delSetting(hConact, TOX_SETTINGS_AVATAR_HASH);
		ProtoBroadcastAck(hConact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, 0, 0);
		return;
	}

	DBVARIANT dbv;
	if (!db_get(transfer->pfts.hContact, m_szModuleName, TOX_SETTINGS_AVATAR_HASH, &dbv))
	{
		if (memcmp(transfer->hash, dbv.pbVal, TOX_HASH_LENGTH) == 0)
		{
			db_free(&dbv);
			OnFileCancel(transfer->pfts.hContact, transfer);
			return;
		}
		db_free(&dbv);
	}

	TCHAR path[MAX_PATH];
	mir_sntprintf(path, _T("%s\\%S"), VARST(_T("%miranda_avatarcache%")), m_szModuleName);
	OnFileAllow(transfer->pfts.hContact, transfer, path);
}

void CToxProto::OnGotFriendAvatarData(AvatarTransferParam *transfer)
{
	db_set_blob(transfer->pfts.hContact, m_szModuleName, TOX_SETTINGS_AVATAR_HASH, transfer->hash, TOX_HASH_LENGTH);

	PROTO_AVATAR_INFORMATION ai = { 0 };
	ai.format = PA_FORMAT_PNG;
	ai.hContact = transfer->pfts.hContact;
	mir_tstrcpy(ai.filename, transfer->pfts.tszCurrentFile);

	fclose(transfer->hFile);
	transfer->hFile = NULL;

	ProtoBroadcastAck(transfer->pfts.hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, 0);
	transfers.Remove(transfer);
}