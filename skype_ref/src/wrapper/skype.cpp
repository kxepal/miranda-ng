#include "skype.h"

#include <win2k.h>
#include <shlwapi.h>
#include <ShellAPI.h>
#include <tlhelp32.h>

#include "..\..\..\skypekit\key.h"
#include "..\base64\base64.h"

//extern "C" 
//{ 
#include "..\aes\aes.h" 
//}

//#include "group.h"
//#include "search.h"
//#include "account.h"
//#include "contact.h"
//#include "message.h"
//#include "transfer.h"
//#include "participant.h"
//#include "conversation.h"

// CSkype

CSkype::CSkype(int num_threads) : Skype(num_threads) { }

CAccount* CSkype::newAccount(int oid) 
{ 
	return new CAccount(oid, this, this->proto); 
}

CGroup* CSkype::newContactGroup(int oid)
{ 
	return new CGroup(oid, this, this->proto); 
}

CContact* CSkype::newContact(int oid) 
{ 
	return new CContact(oid, this, this->proto); 
}

CConversation* CSkype::newConversation(int oid) 
{ 
	return new CConversation(oid, this, this->proto); 
}

CParticipant* CSkype::newParticipant(int oid) 
{ 
	return new CParticipant(oid, this, this->proto); 
}

CMessage* CSkype::newMessage(int oid) 
{ 
	return new CMessage(oid, this, this->proto); 
}

CTransfer* CSkype::newTransfer(int oid) 
{ 
	return new CTransfer(oid, this, this->proto); 
}

CSearch* CSkype::newContactSearch(int oid)
{
	return new CSearch(oid, this, this->proto);
}

void CSkype::OnMessage (
	const MessageRef & message,
	const bool & changesInboxTimestamp,
	const MessageRef & supersedesHistoryMessage,
	const ConversationRef & conversation)
{
    /*uint now;
    skype->GetUnixTimestamp(now);
    conversation->SetConsumedHorizon(now);*/

	/*if (this->proto)
		(proto->*onMessageCallback)(conversation->ref(), message->ref());*/
	//this->proto->OnMessage(conversation->ref(), message->ref());
}


BOOL CSkype::IsRunAsAdmin()
{
	BOOL fIsRunAsAdmin = FALSE;
	DWORD dwError = ERROR_SUCCESS;
	PSID pAdministratorsGroup = NULL;

	// Allocate and initialize a SID of the administrators group.
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	if ( !AllocateAndInitializeSid(
		&NtAuthority, 
		2, 
		SECURITY_BUILTIN_DOMAIN_RID, 
		DOMAIN_ALIAS_RID_ADMINS, 
		0, 0, 0, 0, 0, 0, 
		&pAdministratorsGroup))
	{
		dwError = GetLastError();
		goto Cleanup;
	}

	// Determine whether the SID of administrators group is enabled in 
	// the primary access token of the process.
	if ( !CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin))
	{
		dwError = GetLastError();
		goto Cleanup;
	}

Cleanup:
	// Centralized cleanup for all allocated resources.
	if (pAdministratorsGroup)
	{
		FreeSid(pAdministratorsGroup);
		pAdministratorsGroup = NULL;
	}

	// Throw the error if something failed in the function.
	if (ERROR_SUCCESS != dwError)
	{
		throw dwError;
	}

	return fIsRunAsAdmin;
}

char *CSkype::LoadKeyPair(HINSTANCE hInstance)
{
	HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(/*IDR_KEY*/107), L"BIN");
	if (hRes) 
	{
		HGLOBAL hResource = LoadResource(hInstance, hRes);
		if (hResource) 
		{
			aes_context ctx;
			unsigned char key[128];

			int basedecoded = Base64::Decode(MY_KEY, (char *)key, MAX_PATH);
			::aes_set_key(&ctx, key, 128);

			int dwResSize = ::SizeofResource(hInstance, hRes);
			char *pData = (char *)::GlobalLock(hResource);
			basedecoded = dwResSize;
			::GlobalUnlock(hResource);

			unsigned char *bufD = (unsigned char*)::malloc(basedecoded + 1);
			unsigned char *tmpD = (unsigned char*)::malloc(basedecoded + 1);
			basedecoded = Base64::Decode(pData, (char *)tmpD, basedecoded);

			for (int i = 0; i < basedecoded; i += 16)
				aes_decrypt(&ctx, tmpD+i, bufD+i);

			::free(tmpD);
			bufD[basedecoded] = 0; //cert should be null terminated
			return (char *)bufD;
		}
		return NULL;
	}
	return NULL;
}

int CSkype::StartSkypeRuntime(HINSTANCE hInstance, const wchar_t *profileName, int &port, const wchar_t *dbPath)
{
	STARTUPINFO cif;
	PROCESS_INFORMATION pi;
	wchar_t param[128];

	::ZeroMemory(&cif, sizeof(STARTUPINFO));	
	cif.cb = sizeof(STARTUPINFO);
	cif.dwFlags = STARTF_USESHOWWINDOW;
	cif.wShowWindow = SW_HIDE;

	//HRSRC 	hRes;
	//HGLOBAL	hResource;
	wchar_t	fileName[MAX_PATH];

	HRSRC hRes = ::FindResource(hInstance, MAKEINTRESOURCE(/*IDR_RUNTIME*/102), L"BIN");
	if (hRes)
	{
		HGLOBAL hResource = ::LoadResource(hInstance, hRes);
		if (hResource)
		{
			HANDLE  hFile;
			char 	*pData = (char *)LockResource(hResource);
			DWORD	dwSize = SizeofResource(hInstance, hRes), written = 0;
			::GetModuleFileName(hInstance, fileName, MAX_PATH);
			
			wchar_t *skypeKitPath = ::wcsrchr(fileName, '\\');
			if (skypeKitPath != NULL)
				*skypeKitPath = 0;
			::swprintf(fileName, SIZEOF(fileName), L"%s\\%s", fileName, L"SkypeKit.exe");
			if ( !::PathFileExists(fileName))
			{
				if ((hFile = ::CreateFile(
					fileName, 
					GENERIC_WRITE, 
					0, 
					NULL, 
					CREATE_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, 
					0)) != INVALID_HANDLE_VALUE) 
				{
					::WriteFile(hFile, (void *)pData, dwSize, &written, NULL);
					::CloseHandle(hFile);
				}
				else
				{
					// Check the current process's "run as administrator" status.
					// Elevate the process if it is not run as administrator.
					if (!CSkype::IsRunAsAdmin())
					{
						wchar_t path[MAX_PATH], cmdLine[100];
						::GetModuleFileName(NULL, path, ARRAYSIZE(path));
						::swprintf(
							cmdLine, 
							SIZEOF(cmdLine), 
							L" /restart:%d /profile=%s", 
							::GetCurrentProcessId(), 
							profileName);

						// Launch itself as administrator.
						SHELLEXECUTEINFO sei = { sizeof(sei) };
						sei.lpVerb = L"runas";
						sei.lpFile = path;
						sei.lpParameters = cmdLine;
						//sei.hwnd = hDlg;
						sei.nShow = SW_NORMAL;

						if ( !::ShellExecuteEx(&sei))
						{
							DWORD dwError = ::GetLastError();
							if (dwError == ERROR_CANCELLED)
							{
								// The user refused to allow privileges elevation.
								// Do nothing ...
							}
						}
						//else
						//{
						//	//DestroyWindow(hDlg);  // Quit itself
						//	::CallService("CloseAction", 0, 0);
						//}
					}
					return 0;
				}
			}
		}
	}

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (::Process32First(snapshot, &entry) == TRUE) {
        while (::Process32Next(snapshot, &entry) == TRUE) {
            if (::wcsicmp(entry.szExeFile, L"SkypeKit.exe") == 0) {  
                HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				port += rand() % 8963 + 1000;
                ::CloseHandle(hProcess);
				break;
            }
        }
    }
    ::CloseHandle(snapshot);

	::swprintf(param, SIZEOF(param), L"-p -P %d -f %s", port, dbPath);
	int startingrt = ::CreateProcess(
		fileName, param, 
		NULL, NULL, FALSE, 
		CREATE_NEW_CONSOLE, 
		NULL, NULL, &cif, &pi);
	DWORD rterr = GetLastError();

	//if (startingrt && rterr == ERROR_SUCCESS)
		//return 1;
	//else
		//return 0;
	return startingrt;
}

HINSTANCE	g_hInstance;

CSkype *CSkype::GetInstance(HINSTANCE hInstance, const wchar_t *profileName, const wchar_t *dbPath, CSkypeProto *proto)
{
	int port = 8963;
	if (!CSkype::StartSkypeRuntime(hInstance, profileName, port, dbPath)) return NULL;

	char *keyPair = CSkype::LoadKeyPair(hInstance);

	CSkype *skype = new CSkype();
	TransportInterface::Status status = skype->init(keyPair, "127.0.0.1", port);
	if (status != TransportInterface::OK)
		return NULL;
	skype->start();

	::free(keyPair);

	//this->skype->SetOnMessageCallback((CSkype::OnMessaged)&CSkypeProto::OnMessage, this);
	skype->proto = proto;
	return skype;
}