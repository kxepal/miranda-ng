////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2009 Adam Strzelecki <ono+miranda@java.pl>
// Copyright (c) 2009-2012 Bartosz Bia�ek
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"

GGPROTO::GGPROTO(const char* pszProtoName, const TCHAR* tszUserName)
{
	// Init mutexes
	InitializeCriticalSection(&sess_mutex);
	InitializeCriticalSection(&ft_mutex);
	InitializeCriticalSection(&img_mutex);
	InitializeCriticalSection(&modemsg_mutex);
	InitializeCriticalSection(&avatar_mutex);
	InitializeCriticalSection(&sessions_mutex);

	// Init instance names
	m_szModuleName = mir_strdup(pszProtoName);
	m_tszUserName = mir_tstrdup(tszUserName);
	m_szProtoName = GGDEF_PROTONAME;
	m_iVersion = 2;

	// Register netlib user
	TCHAR name[128];
	mir_sntprintf(name, SIZEOF(name), TranslateT("%s connection"), m_tszUserName);

	NETLIBUSER nlu = { 0 };
	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_TCHAR | NUF_OUTGOING | NUF_INCOMING | NUF_HTTPCONNS;
	nlu.szSettingsModule = m_szModuleName;
	nlu.ptszDescriptiveName = name;

	netlib = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	// Register services
	createProtoService(PS_GETAVATARCAPS, &GGPROTO::getavatarcaps);
	createProtoService(PS_GETAVATARINFOT, &GGPROTO::getavatarinfo);
	createProtoService(PS_GETMYAVATAR, &GGPROTO::getmyavatar);
	createProtoService(PS_SETMYAVATAR, &GGPROTO::setmyavatar);

	createProtoService(PS_GETMYAWAYMSG, &GGPROTO::getmyawaymsg);
	createProtoService(PS_CREATEACCMGRUI, &GGPROTO::get_acc_mgr_gui);

	createProtoService(PS_LEAVECHAT, &GGPROTO::leavechat);

	// Offline contacts and clear logon time
	setalloffline();
	db_set_dw(NULL, m_szModuleName, GG_KEY_LOGONTIME, 0);

	DWORD dwVersion;
	if ((dwVersion = db_get_dw(NULL, m_szModuleName, GG_PLUGINVERSION, 0)) < pluginInfo.version)
		cleanuplastplugin(dwVersion);

	links_instance_init();
	initavatarrequestthread();

	TCHAR szPath[MAX_PATH];
	TCHAR *tmpPath = Utils_ReplaceVarsT( _T("%miranda_avatarcache%"));
	mir_sntprintf(szPath, MAX_PATH, _T("%s\\%s"), tmpPath, m_szModuleName);
	mir_free(tmpPath);
	hAvatarsFolder = FoldersRegisterCustomPathT(m_szModuleName, "Avatars", szPath);

	tmpPath = Utils_ReplaceVarsT( _T("%miranda_userdata%"));
	mir_sntprintf(szPath, MAX_PATH, _T("%s\\%s\\ImageCache"), tmpPath, m_szModuleName);
	mir_free(tmpPath);
	hImagesFolder = FoldersRegisterCustomPathT(m_szModuleName, "Images", szPath);
}

GGPROTO::~GGPROTO()
{
#ifdef DEBUGMODE
	netlog("gg_proto_uninit(): destroying protocol interface");
#endif

	// Destroy modules
	block_uninit();
	img_destroy();
	keepalive_destroy();
	gc_destroy();

	if (hMenuRoot)
		CallService(MS_CLIST_REMOVEMAINMENUITEM, (WPARAM)hMenuRoot, 0);

	// Close handles
	Netlib_CloseHandle(netlib);

	// Destroy mutexes
	DeleteCriticalSection(&sess_mutex);
	DeleteCriticalSection(&ft_mutex);
	DeleteCriticalSection(&img_mutex);
	DeleteCriticalSection(&modemsg_mutex);
	DeleteCriticalSection(&avatar_mutex);
	DeleteCriticalSection(&sessions_mutex);

	// Free status messages
	if (modemsg.online)    mir_free(modemsg.online);
	if (modemsg.away)      mir_free(modemsg.away);
	if (modemsg.dnd)       mir_free(modemsg.dnd);
	if (modemsg.freechat)  mir_free(modemsg.freechat);
	if (modemsg.invisible) mir_free(modemsg.invisible);
	if (modemsg.offline)   mir_free(modemsg.offline);

	mir_free(m_szModuleName);
	mir_free(m_tszUserName);
}

//////////////////////////////////////////////////////////
// Dummies for function that have to be implemented

HANDLE GGPROTO::AddToListByEvent(int flags, int iContact, HANDLE hDbEvent) { return NULL; }
int    GGPROTO::Authorize(HANDLE hContact) { return 0; }
int    GGPROTO::AuthDeny(HANDLE hContact, const TCHAR *szReason) { return 0; }
int    GGPROTO::AuthRecv(HANDLE hContact, PROTORECVEVENT *pre) { return 0; }
int    GGPROTO::AuthRequest(HANDLE hContact, const TCHAR *szMessage) { return 0; }
HANDLE GGPROTO::ChangeInfo(int iInfoType, void *pInfoData) { return NULL; }
int    GGPROTO::FileResume(HANDLE hTransfer, int *action, const PROTOCHAR** szFilename) { return 0; }
HANDLE GGPROTO::SearchByEmail(const PROTOCHAR *email) { return NULL; }
int    GGPROTO::RecvContacts(HANDLE hContact, PROTORECVEVENT *pre) { return 0; }
int    GGPROTO::RecvUrl(HANDLE hContact, PROTORECVEVENT *pre) { return 0; }
int    GGPROTO::SendContacts(HANDLE hContact, int flags, int nContacts, HANDLE *hContactsList) { return 0; }
int    GGPROTO::SendUrl(HANDLE hContact, int flags, const char *url) { return 0; }
int    GGPROTO::RecvAwayMsg(HANDLE hContact, int mode, PROTORECVEVENT *evt) { return 0; }
int    GGPROTO::SendAwayMsg(HANDLE hContact, HANDLE hProcess, const char *msg) { return 0; }

//////////////////////////////////////////////////////////
// when contact is added to list

HANDLE GGPROTO::AddToList(int flags, PROTOSEARCHRESULT *psr)
{
	GGSEARCHRESULT *sr = (GGSEARCHRESULT *)psr;
	uin_t uin;

	if (psr->cbSize == sizeof(GGSEARCHRESULT))
		uin = sr->uin;
	else
		uin = _ttoi(psr->id);

	return getcontact(uin, 1, flags & PALF_TEMPORARY ? 0 : 1, sr->nick);
}

//////////////////////////////////////////////////////////
// checks proto capabilities

DWORD_PTR GGPROTO::GetCaps(int type, HANDLE hContact)
{
	switch (type) {
		case PFLAGNUM_1:
			return PF1_IM | PF1_BASICSEARCH | PF1_EXTSEARCH | PF1_EXTSEARCHUI | PF1_SEARCHBYNAME |
				   PF1_MODEMSG | PF1_NUMERICUSERID | PF1_VISLIST | PF1_FILE;
		case PFLAGNUM_2:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_HEAVYDND | PF2_FREECHAT | PF2_INVISIBLE |
				   PF2_LONGAWAY;
		case PFLAGNUM_3:
			return PF2_ONLINE | PF2_SHORTAWAY | PF2_HEAVYDND | PF2_FREECHAT | PF2_INVISIBLE;
		case PFLAGNUM_4:
			return PF4_NOCUSTOMAUTH | PF4_SUPPORTTYPING | PF4_AVATARS | PF4_IMSENDOFFLINE;
		case PFLAGNUM_5:
			return PF2_LONGAWAY;
		case PFLAG_UNIQUEIDTEXT:
			return (DWORD_PTR) Translate("Gadu-Gadu Number");
		case PFLAG_UNIQUEIDSETTING:
			return (DWORD_PTR) GG_KEY_UIN;
	}
	return 0;
}

//////////////////////////////////////////////////////////
// loads protocol icon

HICON GGPROTO::GetIcon(int iconIndex)
{
	if (LOWORD(iconIndex) == PLI_PROTOCOL)
	{
		if (iconIndex & PLIF_ICOLIBHANDLE)
			return (HICON)GetIconHandle(IDI_GG);

		BOOL big = (iconIndex & PLIF_SMALL) == 0;
		HICON hIcon = LoadIconEx("main", big);

		if (iconIndex & PLIF_ICOLIB)
			return hIcon;

		hIcon = CopyIcon(hIcon);
		ReleaseIconEx("main", big);
		return hIcon;
	}

	return (HICON)NULL;
}

//////////////////////////////////////////////////////////
// user info request

void __cdecl GGPROTO::cmdgetinfothread(void *hContact)
{
	SleepEx(100, FALSE);
	netlog("gg_cmdgetinfothread(): Failed info retreival.");
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, (HANDLE) 1, 0);
}

int GGPROTO::GetInfo(HANDLE hContact, int infoType)
{
	gg_pubdir50_t req;

	// Custom contact info
	if (hContact)
	{
		if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
		{
			forkthread(&GGPROTO::cmdgetinfothread, hContact);
			return 1;
		}

		// Add uin and search it
		gg_pubdir50_add(req, GG_PUBDIR50_UIN, ditoa((uin_t)db_get_dw(hContact, m_szModuleName, GG_KEY_UIN, 0)));
		gg_pubdir50_seq_set(req, GG_SEQ_INFO);

		netlog("gg_getinfo(): Requesting user info.", req->seq);
		if (isonline())
		{
			EnterCriticalSection(&sess_mutex);
			if (!gg_pubdir50(sess, req))
			{
				LeaveCriticalSection(&sess_mutex);
				forkthread(&GGPROTO::cmdgetinfothread, hContact);
				return 1;
			}
			LeaveCriticalSection(&sess_mutex);
		}
	}
	// Own contact info
	else
	{
		if (!(req = gg_pubdir50_new(GG_PUBDIR50_READ)))
		{
			forkthread(&GGPROTO::cmdgetinfothread, hContact);
			return 1;
		}

		// Add seq
		gg_pubdir50_seq_set(req, GG_SEQ_CHINFO);

		netlog("gg_getinfo(): Requesting owner info.", req->seq);
		if (isonline())
		{
			EnterCriticalSection(&sess_mutex);
			if (!gg_pubdir50(sess, req))
			{
				LeaveCriticalSection(&sess_mutex);
				forkthread(&GGPROTO::cmdgetinfothread, hContact);
				return 1;
			}
			LeaveCriticalSection(&sess_mutex);
		}
	}
	netlog("gg_getinfo(): Seq %d.", req->seq);
	gg_pubdir50_free(req);

	return 1;
}

//////////////////////////////////////////////////////////
// when basic search

void __cdecl GGPROTO::searchthread(void *)
{
	SleepEx(100, FALSE);
	netlog("gg_searchthread(): Failed search.");
	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_FAILED, (HANDLE)1, 0);
}

HANDLE GGPROTO::SearchBasic(const PROTOCHAR *id)
{
	if (!isonline())
		return (HANDLE)0;

	gg_pubdir50_t req;
	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH))) {
		forkthread(&GGPROTO::searchthread, NULL);
		return (HANDLE)1;
	}

	TCHAR *ida = mir_tstrdup(id);

	// Add uin and search it
	gg_pubdir50_add(req, GG_PUBDIR50_UIN, _T2A(ida));
	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	mir_free(ida);

	EnterCriticalSection(&sess_mutex);
	if (!gg_pubdir50(sess, req))
	{
		LeaveCriticalSection(&sess_mutex);
		forkthread(&GGPROTO::searchthread, NULL);
		return (HANDLE)1;
	}
	LeaveCriticalSection(&sess_mutex);
	netlog("gg_basicsearch(): Seq %d.", req->seq);
	gg_pubdir50_free(req);

	return (HANDLE)1;
}

//////////////////////////////////////////////////////////
// search by details

HANDLE GGPROTO::SearchByName(const PROTOCHAR *nick, const PROTOCHAR *firstName, const PROTOCHAR *lastName)
{
	gg_pubdir50_t req;
	unsigned long crc;
	char data[512] = "\0";

	// Check if connected and if there's a search data
	if (!isonline())
		return 0;

	if (!nick && !firstName && !lastName)
		return 0;

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{
		forkthread(&GGPROTO::searchthread, NULL);
		return (HANDLE)1;
	}

	// Add uin and search it
	if (nick)
	{
		char *nickA = mir_t2a(nick);
		gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, nickA);
		strncat(data, nickA, sizeof(data) - strlen(data));
		mir_free(nickA);
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	if (firstName)
	{
		char *firstNameA = mir_t2a(firstName);
		gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, firstNameA);
		strncat(data, firstNameA, sizeof(data) - strlen(data));
		mir_free(firstNameA);
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	if (lastName)
	{
		char *lastNameA = mir_t2a(lastName);
		gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, lastNameA);
		strncat(data, lastNameA, sizeof(data) - strlen(data));
		mir_free(lastNameA);
	}
	strncat(data, ".", sizeof(data) - strlen(data));

	// Count crc & check if the data was equal if yes do same search with shift
	crc = crc_get(data);

	if (crc == last_crc && next_uin)
		gg_pubdir50_add(req, GG_PUBDIR50_START, ditoa(next_uin));
	else
		last_crc = crc;

	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	EnterCriticalSection(&sess_mutex);
	if (!gg_pubdir50(sess, req))
	{
		LeaveCriticalSection(&sess_mutex);
		forkthread(&GGPROTO::searchthread, NULL);
		return (HANDLE)1;
	}
	LeaveCriticalSection(&sess_mutex);
	netlog("gg_searchbyname(): Seq %d.", req->seq);
	gg_pubdir50_free(req);

	return (HANDLE)1;
}

//////////////////////////////////////////////////////////
// search by advanced

HWND GGPROTO::SearchAdvanced(HWND hwndDlg)
{
	gg_pubdir50_t req;
	char text[64], data[512] = "\0";
	unsigned long crc;

	// Check if connected
	if (!isonline()) return (HWND)0;

	if (!(req = gg_pubdir50_new(GG_PUBDIR50_SEARCH)))
	{
		forkthread(&GGPROTO::searchthread, NULL);
		return (HWND)1;
	}

	// Fetch search data
	GetDlgItemTextA(hwndDlg, IDC_FIRSTNAME, text, sizeof(text));
	if (strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 1 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemTextA(hwndDlg, IDC_LASTNAME, text, sizeof(text));
	if (strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 2 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemTextA(hwndDlg, IDC_NICKNAME, text, sizeof(text));
	if (strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 3 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemTextA(hwndDlg, IDC_CITY, text, sizeof(text));
	if (strlen(text))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_CITY, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 4 */ strncat(data, ".", sizeof(data) - strlen(data));

	GetDlgItemTextA(hwndDlg, IDC_AGEFROM, text, sizeof(text));
	if (strlen(text))
	{
		int yearTo = atoi(text);
		int yearFrom;
		time_t t = time(NULL);
		struct tm *lt = localtime(&t);
		int ay = lt->tm_year + 1900;
		char age[16];

		GetDlgItemTextA(hwndDlg, IDC_AGETO, age, sizeof(age));
		yearFrom = atoi(age);

		// Count & fix ranges
		if (!yearTo)
			yearTo = ay;
		else
			yearTo = ay - yearTo;
		if (!yearFrom)
			yearFrom = 0;
		else
			yearFrom = ay - yearFrom;
		mir_snprintf(text, sizeof(text), "%d %d", yearFrom, yearTo);

		gg_pubdir50_add(req, GG_PUBDIR50_BIRTHYEAR, text);
		strncat(data, text, sizeof(data) - strlen(data));
	}
	/* 5 */ strncat(data, ".", sizeof(data) - strlen(data));

	switch(SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_GETCURSEL, 0, 0))
	{
		case 1:
			gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_FEMALE);
			strncat(data, GG_PUBDIR50_GENDER_MALE, sizeof(data) - strlen(data));
			break;
		case 2:
			gg_pubdir50_add(req, GG_PUBDIR50_GENDER, GG_PUBDIR50_GENDER_MALE);
			strncat(data, GG_PUBDIR50_GENDER_FEMALE, sizeof(data) - strlen(data));
			break;
	}
	/* 6 */ strncat(data, ".", sizeof(data) - strlen(data));

	if (IsDlgButtonChecked(hwndDlg, IDC_ONLYCONNECTED))
	{
		gg_pubdir50_add(req, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);
		strncat(data, GG_PUBDIR50_ACTIVE_TRUE, sizeof(data) - strlen(data));
	}
	/* 7 */ strncat(data, ".", sizeof(data) - strlen(data));

	// No data entered
	if (strlen(data) <= 7 || (strlen(data) == 8 && IsDlgButtonChecked(hwndDlg, IDC_ONLYCONNECTED))) return (HWND)0;

	// Count crc & check if the data was equal if yes do same search with shift
	crc = crc_get(data);

	if (crc == last_crc && next_uin)
		gg_pubdir50_add(req, GG_PUBDIR50_START, ditoa(next_uin));
	else
		last_crc = crc;

	gg_pubdir50_seq_set(req, GG_SEQ_SEARCH);

	if (isonline())
	{
		EnterCriticalSection(&sess_mutex);
		if (!gg_pubdir50(sess, req))
		{
			LeaveCriticalSection(&sess_mutex);
			forkthread(&GGPROTO::searchthread, NULL);
			return (HWND)1;
		}
		LeaveCriticalSection(&sess_mutex);
	}
	netlog("gg_searchbyadvanced(): Seq %d.", req->seq);
	gg_pubdir50_free(req);

	return (HWND)1;
}

//////////////////////////////////////////////////////////
// create adv search dialog

static INT_PTR CALLBACK gg_advancedsearchdlgproc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)_T(""));				// 0
		SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)TranslateT("Female"));	// 1
		SendDlgItemMessage(hwndDlg, IDC_GENDER, CB_ADDSTRING, 0, (LPARAM)TranslateT("Male"));	// 2
		return TRUE;
		
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
			SendMessage(GetParent(hwndDlg), WM_COMMAND,MAKEWPARAM(IDOK,BN_CLICKED), (LPARAM)GetDlgItem(GetParent(hwndDlg),IDOK));
			break;
		}
		break;
	}
	return FALSE;
}

HWND GGPROTO::CreateExtendedSearchUI(HWND owner)
{
	return CreateDialogParam(hInstance,
		MAKEINTRESOURCE(IDD_GGADVANCEDSEARCH), owner, gg_advancedsearchdlgproc, (LPARAM)this);
}

//////////////////////////////////////////////////////////
// when messsage received

int GGPROTO::RecvMsg(HANDLE hContact, PROTORECVEVENT *pre)
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, ( LPARAM )pre };
	return CallService(MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs);
}

//////////////////////////////////////////////////////////
// when messsage sent

typedef struct
{
	HANDLE hContact;
	int seq;
} GG_SEQ_ACK;

void __cdecl GGPROTO::sendackthread(void *ack)
{
	SleepEx(100, FALSE);
	ProtoBroadcastAck(m_szModuleName, ((GG_SEQ_ACK *)ack)->hContact,
		ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) ((GG_SEQ_ACK *)ack)->seq, 0);
	mir_free(ack);
}

int GGPROTO::SendMsg(HANDLE hContact, int flags, const char *msg)
{
	uin_t uin;

	if (msg && isonline() && (uin = (uin_t)db_get_dw(hContact, m_szModuleName, GG_KEY_UIN, 0)))
	{
		int seq;
		EnterCriticalSection(&sess_mutex);
		seq = gg_send_message(sess, GG_CLASS_CHAT, uin, (BYTE*)msg);
		LeaveCriticalSection(&sess_mutex);
		if (!db_get_b(NULL, m_szModuleName, GG_KEY_MSGACK, GG_KEYDEF_MSGACK))
		{
			// Auto-ack message without waiting for server ack
			GG_SEQ_ACK *ack = (GG_SEQ_ACK*)mir_alloc(sizeof(GG_SEQ_ACK));
			if (ack)
			{
				ack->seq = seq;
				ack->hContact = hContact;
				forkthread(&GGPROTO::sendackthread, ack);
			}
		}
		return seq;
	}
	return 0;
}

//////////////////////////////////////////////////////////
// visible lists

int GGPROTO::SetApparentMode(HANDLE hContact, int mode)
{
	db_set_w(hContact, m_szModuleName, GG_KEY_APPARENT, (WORD)mode);
	notifyuser(hContact, 1);
	return 0;
}

//////////////////////////////////////////////////////////
// sets protocol status

int GGPROTO::SetStatus(int iNewStatus)
{
	int nNewStatus = gg_normalizestatus(iNewStatus);

	EnterCriticalSection(&modemsg_mutex);
	m_iDesiredStatus = nNewStatus;
	LeaveCriticalSection(&modemsg_mutex);

	// If waiting for connection retry attempt then signal to stop that
	if (hConnStopEvent) SetEvent(hConnStopEvent);

	if (m_iStatus == nNewStatus) return 0;
	netlog("gg_setstatus(): PS_SETSTATUS(%d) normalized to %d.", iNewStatus, nNewStatus);
	refreshstatus(nNewStatus);

	return 0;
}

//////////////////////////////////////////////////////////
// when away message is requested

void __cdecl GGPROTO::getawaymsgthread(void *hContact)
{
	DBVARIANT dbv;

	SleepEx(100, FALSE);
	if (!db_get_s(hContact, "CList", GG_KEY_STATUSDESCR, &dbv, DBVT_TCHAR))
	{
		ProtoBroadcastAck(m_szProtoName, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) dbv.ptszVal);
		netlog("gg_getawaymsg(): Reading away msg <" TCHAR_STR_PARAM ">.", dbv.ptszVal);
		DBFreeVariant(&dbv);
	}
	else
		ProtoBroadcastAck(m_szProtoName, hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE) 1, (LPARAM) NULL);
}

HANDLE GGPROTO::GetAwayMsg(HANDLE hContact)
{
	forkthread(&GGPROTO::getawaymsgthread, hContact);
	return (HANDLE)1;
}

//////////////////////////////////////////////////////////
// when away message is being set

int GGPROTO::SetAwayMsg(int iStatus, const PROTOCHAR *msgt)
{
	int status = gg_normalizestatus(iStatus);
	char **szMsg;
	char *msg = mir_t2a(msgt);

	netlog("gg_setawaymsg(): PS_SETAWAYMSG(%d, \"%s\").", iStatus, msg);

	EnterCriticalSection(&modemsg_mutex);
	// Select proper msg
	switch(status)
	{
		case ID_STATUS_ONLINE:
			szMsg = &modemsg.online;
			break;
		case ID_STATUS_AWAY:
			szMsg = &modemsg.away;
			break;
		case ID_STATUS_DND:
			szMsg = &modemsg.dnd;
			break;
		case ID_STATUS_FREECHAT:
			szMsg = &modemsg.freechat;
			break;
		case ID_STATUS_INVISIBLE:
			szMsg = &modemsg.invisible;
			break;
		default:
			LeaveCriticalSection(&modemsg_mutex);
			mir_free(msg);
			return 1;
	}

	// Check if we change status here somehow
	if (*szMsg && msg && !strcmp(*szMsg, msg)
		|| !*szMsg && (!msg || !*msg))
	{
		if (status == m_iDesiredStatus && m_iDesiredStatus == m_iStatus)
		{
			netlog("gg_setawaymsg(): Message hasn't been changed, return.");
			LeaveCriticalSection(&modemsg_mutex);
			mir_free(msg);
			return 0;
		}
	}
	else
	{
		if (*szMsg)
			mir_free(*szMsg);
		*szMsg = msg && *msg ? mir_strdup(msg) : NULL;
#ifdef DEBUGMODE
		netlog("gg_setawaymsg(): Message changed.");
#endif
	}
	LeaveCriticalSection(&modemsg_mutex);

	// Change the status if it was desired by PS_SETSTATUS
	if (status == m_iDesiredStatus)
		refreshstatus(status);

	mir_free(msg);
	return 0;
}

//////////////////////////////////////////////////////////
// sends a notification that the user is typing a message

int GGPROTO::UserIsTyping(HANDLE hContact, int type)
{
	uin_t uin = db_get_dw(hContact, m_szModuleName, GG_KEY_UIN, 0);

	if (!uin || !isonline()) return 0;

	if (type == PROTOTYPE_SELFTYPING_ON || type == PROTOTYPE_SELFTYPING_OFF) {
		EnterCriticalSection(&sess_mutex);
		gg_typing_notification(sess, uin, (type == PROTOTYPE_SELFTYPING_ON));
		LeaveCriticalSection(&sess_mutex);
	}

	return 0;
}

//////////////////////////////////////////////////////////
// Custom protocol event

int GGPROTO::OnEvent(PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam)
{
	switch( eventType ) {
	case EV_PROTO_ONLOAD:
		{
			hookProtoEvent(ME_OPT_INITIALISE, &GGPROTO::options_init);
			hookProtoEvent(ME_USERINFO_INITIALISE, &GGPROTO::details_init);

			// Init misc stuff
			gg_icolib_init();
			initpopups();
			gc_init();
			keepalive_init();
			img_init();
			block_init();

			// Try to fetch user avatar
			getUserAvatar();
			break;
		}
	case EV_PROTO_ONEXIT:
		// Stop avatar request thread
		uninitavatarrequestthread();

		// Stop main connection session thread
		threadwait(&pth_sess);
		img_shutdown();
		sessions_closedlg();
		break;

	case EV_PROTO_ONOPTIONS:
		return options_init(wParam, lParam);

	case EV_PROTO_ONMENU:
		menus_init();
		break;

	case EV_PROTO_ONRENAME:
		if (hMenuRoot) {
			CLISTMENUITEM mi = {0};
			mi.cbSize = sizeof(mi);
			mi.flags = CMIM_NAME | CMIF_TCHAR | CMIF_KEEPUNTRANSLATED;
			mi.ptszName = m_tszUserName;
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuRoot, (LPARAM)&mi);
		}
		break;

	case EV_PROTO_ONCONTACTDELETED:
		return contactdeleted(wParam, lParam);

	case EV_PROTO_DBSETTINGSCHANGED:
		return dbsettingchanged(wParam, lParam);
	}
	return TRUE;
}

