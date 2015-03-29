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
			CToxPasswordEditor passwordEditor(this);
			if (!passwordEditor.DoModal())
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

CToxPasswordEditor::CToxPasswordEditor(CToxProto *proto) :
	CSuper(proto, IDD_PASSWORD, NULL, false), ok(this, IDOK),
	password(this, IDC_PASSWORD), savePermanently(this, IDC_SAVEPERMANENTLY)
{
	ok.OnClick = Callback(this, &CToxPasswordEditor::OnOk);
}

void CToxPasswordEditor::OnInitDialog()
{
	TranslateDialogDefault(m_hwnd);
}

void CToxPasswordEditor::OnOk(CCtrlButton*)
{
	if (savePermanently.Enabled())
		m_proto->setTString("Password", password.GetText());
	if (m_proto->password != NULL)
		mir_free(m_proto->password);
	m_proto->password = mir_utf8encodeW(password.GetText());

	EndDialog(m_hwnd, 1);
}