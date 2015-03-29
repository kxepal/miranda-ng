#include "common.h"

HANDLE CToxProto::hProfileFolderPath;

std::tstring CToxProto::GetToxProfilePath()
{
	return GetToxProfilePath(m_tszUserName);
}

std::tstring CToxProto::GetToxProfilePath(const TCHAR *accountName)
{
	TCHAR profilePath[MAX_PATH];
	TCHAR profileRootPath[MAX_PATH];
	FoldersGetCustomPathT(hProfileFolderPath, profileRootPath, SIZEOF(profileRootPath), VARST(_T("%miranda_userdata%")));
	mir_sntprintf(profilePath, MAX_PATH, _T("%s\\%s.tox"), profileRootPath, accountName);
	return profilePath;
}

bool CToxProto::LoadToxProfile(Tox_Options *options)
{
	debugLogA(__FUNCTION__": loading tox profile");

	size_t size = 0;
	uint8_t *data = NULL;
	std::tstring profilePath = GetToxProfilePath();
	if (IsFileExists(profilePath))
	{
		FILE *profile = _tfopen(profilePath.c_str(), _T("rb"));
		if (profile == NULL)
		{
			debugLogA(__FUNCTION__": failed to open tox profile");
			return false;
		}

		fseek(profile, 0, SEEK_END);
		size = ftell(profile);
		rewind(profile);
		if (size == 0)
		{
			fclose(profile);
			debugLogA(__FUNCTION__": tox profile is empty");
			return true;
		}

		data = (uint8_t*)mir_calloc(size);
		if (fread((char*)data, sizeof(char), size, profile) != size)
		{
			fclose(profile);
			debugLogA(__FUNCTION__": failed to read tox profile");
			mir_free(data);
			return false;
		}
		fclose(profile);
	}
	
	TOX_ERR_NEW coreError;
	if (data != NULL && tox_is_data_encrypted(data))
	{
		password = mir_utf8encodeW(ptrT(getTStringA("Password")));
		if (password == NULL || mir_strlen(password) == 0)
		{
			if (!DialogBoxParam(
				g_hInstance,
				MAKEINTRESOURCE(IDD_PASSWORD),
				NULL,
				ToxProfilePasswordProc,
				(LPARAM)this))
			{
				mir_free(data);
				return false;
			}
		}
		tox = tox_encrypted_new(options, data, size, (uint8_t*)password, mir_strlen(password), &coreError);
	}
	else
		tox = tox_new(options, data, size, &coreError);

	if (coreError != TOX_ERR_NEW_OK)
	{
		debugLogA(__FUNCTION__": failed to load tox profile (%d)", coreError);
		mir_free(data);
		return false;
	}

	debugLogA(__FUNCTION__": tox profile load successfully");
	return true;
}

void CToxProto::SaveToxProfile()
{
	size_t size = 0;
	uint8_t *data = NULL;

	{
		mir_cslock lock(toxLock);

		if (password && strlen(password))
		{
			size = tox_encrypted_size(tox);
			data = (uint8_t*)mir_calloc(size);
			if (tox_encrypted_save(tox, data, (uint8_t*)password, strlen(password)) == TOX_ERROR)
			{
				debugLogA(__FUNCTION__": failed to encrypt tox profile");
				mir_free(data);
				return;
			}
		}
		else
		{
			size = tox_get_savedata_size(tox);
			data = (uint8_t*)mir_calloc(size);
			tox_get_savedata(tox, data);
		}
	}

	std::tstring profilePath = GetToxProfilePath();
	FILE *profile = _tfopen(profilePath.c_str(), _T("wb"));
	if (profile == NULL)
	{
		debugLogA(__FUNCTION__": failed to open tox profile");
		return;
	}

	size_t written = fwrite(data, sizeof(char), size, profile);
	if (size != written)
	{
		fclose(profile);
		debugLogA(__FUNCTION__": failed to write tox profile");
	}

	fclose(profile);
	mir_free(data);
}

INT_PTR CToxProto::OnCopyToxID(WPARAM, LPARAM)
{
	ptrA address(getStringA(TOX_SETTINGS_ID));
	size_t length = mir_strlen(address) + 1;
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_FIXED, length);
		memcpy(GlobalLock(hMem), address, length);
		GlobalUnlock(hMem);
		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
	}
	return 0;
}

INT_PTR CToxProto::ToxProfilePasswordProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CToxProto *proto = (CToxProto*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwnd);
		{
			proto = (CToxProto*)lParam;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				TCHAR password[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PASSWORD, password, SIZEOF(password));
				if (IsDlgButtonChecked(hwnd, IDC_SAVEPERMANENTLY))
					proto->setTString("Password", password);
				if (proto->password != NULL)
					mir_free(proto->password);
				proto->password = mir_utf8encodeW(password);

				EndDialog(hwnd, 1);
			}
			break;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		}
		break;
	}

	return FALSE;
}