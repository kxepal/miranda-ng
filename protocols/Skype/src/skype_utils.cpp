#include "skype_proto.h"

int  CSkypeProto::countriesCount;
CountryListEntry*  CSkypeProto::countryList;

void CSkypeProto::FakeAsync(void *param)
{
	::Sleep(100);
	::CallService(MS_PROTO_BROADCASTACK, 0, (LPARAM)param);
	::mir_free(param);
}

int CSkypeProto::GetCountryIdByName(const char* countryName)
{
	for (int i = 0; i < CSkypeProto::countriesCount; i++)
	{
		const char* country = CSkypeProto::countryList[i].szName;
		if (strcmp(CSkypeProto::countryList[i].szName, countryName) == 0)
			return CSkypeProto::countryList[i].id;
	}

	return 0;
}

char* CSkypeProto::GetCountryNameById(int countryId)
{
	return (char*)::CallService(MS_UTILS_GETCOUNTRYBYNUMBER, (WPARAM)countryId, NULL);
}

wchar_t* CSkypeProto::GetAvatarFilePath(wchar_t* skypeName)
{
	wchar_t* path = new wchar_t[MAX_PATH * 2];
	
	FOLDERSGETDATA fgd = {0};
	fgd.cbSize = sizeof(FOLDERSGETDATA);
	fgd.nMaxPathSize = MAX_PATH * 2;
	fgd.szPathT = path;
	fgd.flags = FF_UNICODE;

	HANDLE hAvatarsFolder;
	if (::ServiceExists(MS_FOLDERS_REGISTER_PATH))
	{
		wchar_t tszPath[MAX_PATH * 2];
		::mir_sntprintf(
			tszPath, 
			MAX_PATH * 2, 
			_T("%%miranda_avatarcache%%\\") _T(TCHAR_STR_PARAM) _T("\\"), 
			this->m_szModuleName);

			hAvatarsFolder = ::FoldersRegisterCustomPathT(this->m_szModuleName, "Avatars Cache", tszPath);
		}
	
	if (::CallService(MS_FOLDERS_GET_PATH, (WPARAM)hAvatarsFolder, (LPARAM)&fgd))
	{
		wchar_t *tmpPath = ::Utils_ReplaceVarsT(L"%miranda_avatarcache%");
		::mir_sntprintf(path, MAX_PATH * 2, _T("%s\\") _T(TCHAR_STR_PARAM) _T("\\"), tmpPath, this->m_szModuleName);
		mir_free(tmpPath);
	}
	else
		wcscat(path, L"\\");

	wcscat(path, skypeName);
	wcscat(path, L".jpg");

	// make sure the avatar cache directory exists
	::CallService(MS_UTILS_CREATEDIRTREET, 0, (LPARAM)path);

	return path;
}

int CSkypeProto::CompareProtos(const CSkypeProto *p1, const CSkypeProto *p2)
{
	return wcscmp(p1->m_tszUserName, p2->m_tszUserName);
}

void CSkypeProto::CreateService(const char* szService, SkypeServiceFunc serviceProc)
{
	char moduleName[MAXMODULELABELLENGTH];

	::mir_snprintf(moduleName, sizeof(moduleName), "%s%s", this->m_szModuleName, szService);
	::CreateServiceFunctionObj(moduleName, (MIRANDASERVICEOBJ)*(void**)&serviceProc, this);
}

void CSkypeProto::CreateServiceParam(const char* szService, SkypeServiceFunc serviceProc, LPARAM lParam)
{
	char moduleName[MAXMODULELABELLENGTH];

	::mir_snprintf(moduleName, sizeof(moduleName), "%s%s", this->m_szModuleName, szService);
	::CreateServiceFunctionObjParam(moduleName, (MIRANDASERVICEOBJPARAM)*(void**)&serviceProc, this, lParam);
}

HANDLE CSkypeProto::CreateEvent(const char* szService)
{
	char moduleName[MAXMODULELABELLENGTH];

	::mir_snprintf(moduleName, sizeof(moduleName), "%s%s", this->m_szModuleName, szService);
	return ::CreateHookableEvent(moduleName);
}

void CSkypeProto::HookEvent(const char* szEvent, SkypeEventFunc handler)
{
	::HookEventObj(szEvent, (MIRANDAHOOKOBJ)*( void**)&handler, this);
}

int CSkypeProto::SendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam)
{
	ACKDATA ack = {0};
	ack.cbSize = sizeof(ACKDATA);
	ack.szModule = this->m_szModuleName;
	ack.hContact = hContact;
	ack.type = type;
	ack.result = result;
	ack.hProcess = hProcess;
	ack.lParam = lParam;

	return ::CallService(MS_PROTO_BROADCASTACK, 0, (LPARAM)&ack);
}

DWORD CSkypeProto::SendBroadcastAsync(HANDLE hContact, int type, int hResult, HANDLE hProcess, LPARAM lParam, size_t paramSize)
{
	ACKDATA *ack = (ACKDATA *)::mir_calloc(sizeof(ACKDATA) + paramSize);
	ack->cbSize = sizeof(ACKDATA);
	ack->szModule = this->m_szModuleName; 
	ack->hContact = hContact;
	ack->type = type; 
	ack->result = hResult;
	ack->hProcess = hProcess; 
	ack->lParam = lParam;
	if (paramSize)
		::memcpy(ack+1, (void*)lParam, paramSize);
	::mir_forkthread(&CSkypeProto::FakeAsync, ack);
	return 0;
}

void CSkypeProto::ForkThread(SkypeThreadFunc pFunc, void *param)
{
	UINT threadID;
	::CloseHandle((HANDLE)::mir_forkthreadowner(
		(pThreadFuncOwner)*(void**)&pFunc, 
		this, 
		param, 
		&threadID));
}

HANDLE CSkypeProto::ForkThreadEx(SkypeThreadFunc pFunc, void *param, UINT* threadID)
{
	UINT lthreadID;
	return (HANDLE)::mir_forkthreadowner(
		(pThreadFuncOwner)*(void**)&pFunc, 
		this,
		param, 
		threadID ? threadID : &lthreadID);
}

//

int CSkypeProto::SkypeToMirandaLoginError(CAccount::LOGOUTREASON logoutReason)
{
	int loginError = 0;

	// todo: rewrite!!

	switch (logoutReason)
	{
	case CAccount::SERVER_OVERLOADED:
	case CAccount::P2P_CONNECT_FAILED:
	case CAccount::SERVER_CONNECT_FAILED:
		loginError = LOGINERR_NOSERVER;
		break;
	case CAccount::HTTPS_PROXY_AUTH_FAILED:
	case CAccount::SOCKS_PROXY_AUTH_FAILED:
		loginError = LOGINERR_PROXYFAILURE;
		break;
	case CAccount::INCORRECT_PASSWORD:
	case CAccount::UNACCEPTABLE_PASSWORD:
		loginError = LOGINERR_WRONGPASSWORD;
		break;
	}

	return loginError;
}

void CSkypeProto::ShowNotification(const wchar_t *sid, const wchar_t *message, int flags)
{
	if (::Miranda_Terminated()) return;

	if ( !ServiceExists(MS_POPUP_ADDPOPUPEX) || !DBGetContactSettingByte(NULL, "PopUp", "ModuleIsEnabled", 1) )
	{
		MessageBoxW(
			NULL, 
			message, 
			TranslateT("Skype Protocol"), 
			MB_OK);
	}
	else
	{
		POPUPDATAT_V2 ppd = {0};
		ppd.cbSize = sizeof(POPUPDATAT_V2);
		ppd.lchContact = NULL;
		lstrcpyn(ppd.lpwzContactName, sid, MAX_CONTACTNAME);
		lstrcpyn(ppd.lpwzText, message, MAX_SECONDLINE);
		ppd.lchIcon = (HICON)CallService(MS_SKIN2_GETICON, 0, (LPARAM)"Skype_main");
		ppd.colorBack = ppd.colorText = 0;
		ppd.iSeconds = 0;

		::CallService(MS_POPUP_ADDPOPUPT, (WPARAM)&ppd, 0);
	}
}