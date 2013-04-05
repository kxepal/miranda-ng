/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2012 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "aim.h"

void CAimProto::broadcast_status(int status)
{
	LOG("Broadcast Status: %d",status);
	int old_status = m_iStatus;
	m_iStatus = status;
	if (m_iStatus == ID_STATUS_OFFLINE)
	{
		shutdown_file_transfers();
		shutdown_chat_conn();

		if (hServerConn)
		{
			aim_sendflap(hServerConn,0x04,0,NULL,seqno);
			Netlib_Shutdown(hServerConn);
		}

		if (hMailConn && hMailConn != (HANDLE)1)
		{
			aim_sendflap(hMailConn,0x04,0,NULL,mail_seqno);
			Netlib_Shutdown(hMailConn);
		}
		else if (hMailConn == (HANDLE)1)
			hMailConn = NULL;

		if (hAvatarConn && hAvatarConn != (HANDLE)1)
		{
			aim_sendflap(hAvatarConn,0x04,0,NULL,avatar_seqno);
			Netlib_Shutdown(hAvatarConn);
		}
		else if (hAvatarConn == (HANDLE)1)
			hAvatarConn = NULL;

		if (hChatNavConn && hChatNavConn != (HANDLE)1)
		{
			aim_sendflap(hChatNavConn,0x04,0,NULL,chatnav_seqno);
			Netlib_Shutdown(hChatNavConn);
		}
		else if (hChatNavConn == (HANDLE)1)
			hChatNavConn = NULL;

		idle = false;
		instantidle = false;
		list_received = false;
		state = 0;
		m_iDesiredStatus = ID_STATUS_OFFLINE;
		mir_free(last_status_msg); last_status_msg = NULL;
		
		avatar_id_lg = 0;
		avatar_id_sm = 0;
		mir_free(hash_lg); hash_lg = NULL;
		mir_free(hash_sm); hash_sm = NULL;

		pd_flags = 0;
		pd_info_id = 0;
		pd_mode = 0;

		seqno = 0;
		mail_seqno = 0;
		avatar_seqno = 0;
		chatnav_seqno = 0;
		admin_seqno = 0;

	}
	sendBroadcast(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)old_status, m_iStatus);	
}

void CAimProto::start_connection(void *arg)
{
	int status = (int)arg;

	if (m_iStatus<=ID_STATUS_OFFLINE)
	{
		offline_contacts();
		DBVARIANT dbv;
		if (!getString(AIM_KEY_SN, &dbv))
			db_free(&dbv);
		else
		{
			ShowPopup(LPGEN("Please, enter a username in the options dialog."), 0);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}
		if (!getString(AIM_KEY_PW, &dbv))
			db_free(&dbv);
		else
		{
			ShowPopup(LPGEN("Please, enter a password in the options dialog."), 0);
			broadcast_status(ID_STATUS_OFFLINE);
			return;
		}

		bool use_ssl = !getByte(AIM_KEY_DSSL, 0);

		char* login_url = getSetting(AIM_KEY_HN);
		if (login_url == NULL) login_url = mir_strdup(use_ssl ? AIM_DEFAULT_SERVER : AIM_DEFAULT_SERVER_NS);

		hServerConn = aim_connect(login_url, get_default_port(), use_ssl, login_url);

		mir_free(login_url);

		pref1_flags = 0x77ffff;
		pref1_set_flags = 0x77ffff;
		mir_free(pref2_flags); pref2_flags = NULL; pref2_len = 0;
		mir_free(pref2_set_flags); pref2_set_flags = NULL; pref2_set_len = 0;

		if (hServerConn)
			aim_connection_authorization();
		else 
			broadcast_status(ID_STATUS_OFFLINE);
	}
}

bool CAimProto::wait_conn(HANDLE& hConn, HANDLE& hEvent, unsigned short service)
{
	if (m_iStatus == ID_STATUS_OFFLINE) 
		return false;

	EnterCriticalSection(&connMutex);
	if (hConn == NULL && hServerConn) 
	{
		LOG("Starting Connection.");
		hConn = (HANDLE)1;    //set so no additional service request attempts are made while aim is still processing the request
		aim_new_service_request(hServerConn, seqno, service) ;//general service connection!
	}
	LeaveCriticalSection(&connMutex);

	if (WaitForSingleObjectEx(hEvent, 10000, TRUE) != WAIT_OBJECT_0)
		return false;

	if (Miranda_Terminated() || m_iStatus == ID_STATUS_OFFLINE) 
		return false;

	return true;
}


unsigned short CAimProto::get_default_port(void)
{
	return getWord(AIM_KEY_PN, getByte(AIM_KEY_DSSL, 0) ? AIM_DEFAULT_PORT : AIM_DEFAULT_SSL_PORT);
}

bool CAimProto::is_my_contact(HANDLE hContact)
{
	const char* szProto = GetContactProto(hContact);
	return szProto != NULL && strcmp(m_szModuleName, szProto) == 0;
}

HANDLE CAimProto::find_chat_contact(const char* room)
{
	HANDLE hContact = db_find_first();
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, "ChatRoomID", &dbv))
			{
				bool found = !strcmp(room, dbv.pszVal); 
				db_free(&dbv);
				if (found) return hContact; 
			}
		}
		hContact = db_find_next(hContact);
	}
	return NULL;
}

HANDLE CAimProto::contact_from_sn(const char* sn, bool addIfNeeded, bool temporary)
{
	char* norm_sn = normalize_name(sn);

	HANDLE hContact = db_find_first();
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			DBVARIANT dbv;
			if (!getString(hContact, AIM_KEY_SN, &dbv))
			{
				bool found = !strcmp(norm_sn, dbv.pszVal); 
				db_free(&dbv);
				if (found)
				{
					mir_free(norm_sn);
					return hContact; 
				}
			}
		}
		hContact = db_find_next(hContact);
	}

	if (addIfNeeded)
	{
		hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		if (hContact)
		{
			if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM) hContact, (LPARAM) m_szModuleName) == 0)
			{
				setString(hContact, AIM_KEY_SN, norm_sn);
				setString(hContact, AIM_KEY_NK, sn);
				LOG("Adding contact %s to client side list.",norm_sn);
				if (temporary)
					db_set_b(hContact, "CList", "NotOnList", 1);
				mir_free(norm_sn);
				return hContact;
			}
			else
				CallService(MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
		}
	}

	mir_free(norm_sn);
	return NULL;
}

void CAimProto::update_server_group(const char* group, unsigned short group_id)
{
	unsigned short user_id_array_size;
	unsigned short* user_id_array;

	if (group_id)
		user_id_array = get_members_of_group(group_id, user_id_array_size);
	else
	{
		user_id_array_size = (unsigned short)group_list.getCount();
		user_id_array = (unsigned short*)mir_alloc(user_id_array_size * sizeof(unsigned short));
		for (unsigned short i=0; i<user_id_array_size; ++i)
			user_id_array[i] = _htons(group_list[i].item_id);
	}

	LOG("Modifying group %s:%u on the serverside list",group, group_id);
	aim_mod_group(hServerConn, seqno, group, group_id, (char*)user_id_array, 
		user_id_array_size * sizeof(unsigned short));

	mir_free(user_id_array);
}

void CAimProto::add_contact_to_group(HANDLE hContact, const char* new_group)
{
	if (new_group == NULL) return;

	if (strcmp(new_group, "MetaContacts Hidden Group") == 0)
		return;

	unsigned short old_group_id = getGroupId(hContact, 1);	
	char* old_group = group_list.find_name(old_group_id);

	if (old_group && strcmp(new_group, old_group) == 0)
		return;
   
	DBVARIANT dbv;
	char *nick = NULL;
	if (!db_get_utf(hContact, MOD_KEY_CL, "MyHandle", &dbv))
	{
		nick = NEWSTR_ALLOCA(dbv.pszVal);
		db_free(&dbv);
	}

	if (getString(hContact, AIM_KEY_SN, &dbv)) return;

	unsigned short item_id = getBuddyId(hContact, 1);
	unsigned short new_item_id = search_for_free_item_id(hContact);
	unsigned short new_group_id = group_list.find_id(new_group);

	if (!item_id)
		LOG("Contact %u not on list.", hContact);

	setGroupId(hContact, 1, new_group_id);
	if (new_group && strcmp(new_group, AIM_DEFAULT_GROUP))
		db_set_utf(hContact, MOD_KEY_CL, OTH_KEY_GP, new_group);
	else
		db_unset(hContact, MOD_KEY_CL, OTH_KEY_GP);

	aim_ssi_update(hServerConn, seqno, true);
		
	if (new_group_id == 0)
	{
		create_group(new_group);	
		LOG("Group %s not on list.", new_group);
		new_group_id = group_list.add(new_group);
		LOG("Adding group %s:%u to the serverside list", new_group, new_group_id);
		aim_add_contact(hServerConn, seqno, new_group, 0, new_group_id, 1);//add the group server-side even if it exist
	}

	LOG("Adding buddy %s:%u %s:%u to the serverside list", dbv.pszVal, new_item_id, new_group, new_group_id);
	aim_add_contact(hServerConn, seqno, dbv.pszVal, new_item_id, new_group_id, 0, nick);
	
	update_server_group(new_group, new_group_id);

	if (old_group_id && item_id)
	{
		bool is_not_in_list = getBool(hContact, AIM_KEY_NIL, false);
		LOG("Removing buddy %s:%u %s:%u from the serverside list", dbv.pszVal, item_id, old_group, old_group_id);
		aim_delete_contact(hServerConn, seqno, dbv.pszVal, item_id, old_group_id, 0, is_not_in_list);
		update_server_group(old_group, old_group_id);
		deleteSetting(hContact, AIM_KEY_NIL);
	}

	aim_ssi_update(hServerConn, seqno, false);

	db_free(&dbv);
}

void CAimProto::offline_contact(HANDLE hContact, bool remove_settings)
{
	if (remove_settings)
	{
		//We need some of this stuff if we are still online.
		for(int i=1;;++i)
		{
			if (deleteBuddyId(hContact, i)) break;
			deleteGroupId(hContact, i);
		}

		db_unset(hContact, MOD_KEY_CL, OTH_KEY_SM);
	}
	setWord(hContact, AIM_KEY_ST, ID_STATUS_OFFLINE);
}

void CAimProto::offline_contacts(void)
{
	HANDLE hContact = db_find_first();
	while (hContact)
	{
		if (is_my_contact(hContact))
			offline_contact(hContact,true);
		hContact = db_find_next(hContact);
	}
	allow_list.destroy();
	block_list.destroy();
	group_list.destroy();
}

char *normalize_name(const char *s)
{
	if (s == NULL) return NULL;
	
	char* buf = mir_strdup(s);
	_strlwr(buf);
/*
	char *p = strchr(buf, ' '); 
	if (p)
	{
		char *q = p;
		while (*p)
		{
			if (*p != ' ') *(q++) = *p;
			++p;
		}
		*q = '\0';
	}
*/
	return buf;
}

char* trim_str(char* s)
{   
	if (s == NULL) return NULL;
	size_t len = strlen(s);

	while (len)
	{
		if (isspace(s[len-1])) --len;
		else break;
	}
	s[len]=0;

	char* sc = s; 
	while (isspace(*sc)) ++sc;
	memcpy(s,sc,strlen(sc)+1);

	return s;
}

void create_group(const char *group)
{
	if (strcmp(group, AIM_DEFAULT_GROUP) == 0) return;

	TCHAR* szGroupName = mir_utf8decodeT(group);
	CallService(MS_CLIST_GROUPCREATE, 0, (LPARAM)szGroupName);
	mir_free(szGroupName);
}

unsigned short CAimProto::search_for_free_item_id(HANDLE hbuddy)//returns a free item id and links the id to the buddy
{
	unsigned short id;

retry:
	id = get_random();

	HANDLE hContact = db_find_first();
	while (hContact)
	{
		if (is_my_contact(hContact))
		{		
			for(int i=1; ;++i)
			{
				unsigned short item_id = getBuddyId(hContact, i);
				if (item_id == 0) break;

				if (item_id == id) goto retry;    //found one no need to look through anymore
			}
		}
		hContact = db_find_next(hContact);
	}

	setBuddyId(hbuddy, 1, id);
	return id;
}

//returns the size of the list array aquired with data
unsigned short* CAimProto::get_members_of_group(unsigned short group_id, unsigned short &size)
{
	unsigned short* list = NULL;
	size = 0;

	HANDLE hContact = db_find_first();
	while (hContact)
	{
		if (is_my_contact(hContact))
		{
			for(int i=1; ;++i)
			{
				unsigned short user_group_id = getGroupId(hContact, i);
				if (user_group_id == 0) break;

				if (group_id == user_group_id)
				{
					unsigned short buddy_id = getBuddyId(hContact, i);
					if (buddy_id)
					{
						list = (unsigned short*)mir_realloc(list, ++size*sizeof(list[0]));
						list[size-1] = _htons(buddy_id);
					}
				}
			}
		}
		hContact = db_find_next(hContact);
	}
	return list;
}

void CAimProto::upload_nicks(void)
{
	HANDLE hContact = db_find_first();
	while (hContact)
	{
		DBVARIANT dbv;
		if (is_my_contact(hContact) && !db_get_utf(hContact, MOD_KEY_CL, "MyHandle", &dbv))
		{
			set_local_nick(hContact, dbv.pszVal, NULL);
			db_free(&dbv);
		}
		hContact = db_find_next(hContact);
	}
}

void CAimProto::set_local_nick(HANDLE hContact, char* nick, char* note)
{
	DBVARIANT dbv;
	if (getString(hContact, AIM_KEY_SN, &dbv)) return;

	for(int i=1; ;++i)
	{
		unsigned short group_id = getGroupId(hContact, i);
		if (group_id == 0) break;

		unsigned short buddy_id = getBuddyId(hContact, i);
		if (buddy_id == 0) break;

		aim_mod_buddy(hServerConn, seqno, dbv.pszVal, group_id, buddy_id, nick, note);
	}
	db_free(&dbv);
}

/////////////////////////////////////////////////////////////////////////////////////////

unsigned short BdList::get_free_id(void)
{
	unsigned short id;

retry:
	id = get_random();

	for (int i=0; i<count; ++i)
		if (items[i]->item_id == id) goto retry;

	return id;
}

unsigned short BdList::find_id(const char* name)
{
	for (int i=0; i<count; ++i)
	{
		if (_stricmp(items[i]->name, name) == 0)
			return items[i]->item_id;
	}
	return 0;
}

char* BdList::find_name(unsigned short id)
{
	for (int i=0; i<count; ++i)
	{
		if (items[i]->item_id == id)
			return items[i]->name;
	}
	return NULL;
}

void BdList::remove_by_id(unsigned short id)
{
	for (int i=0; i<count; ++i)
	{
		if (items[i]->item_id == id)
		{
			remove(i);
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

unsigned short CAimProto::getBuddyId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_BI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_BI)+10, AIM_KEY_BI"%d", i);
	return getWord(hContact, item, 0);
}

void CAimProto::setBuddyId(HANDLE hContact, int i, unsigned short id)
{
	char item[sizeof(AIM_KEY_BI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_BI)+10, AIM_KEY_BI"%d", i);
	setWord(hContact, item, id);
}

int CAimProto::deleteBuddyId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_BI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_BI)+10, AIM_KEY_BI"%d", i);
	return deleteSetting(hContact, item);
}

unsigned short CAimProto::getGroupId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_GI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_GI)+10, AIM_KEY_GI"%d", i);
	return getWord(hContact, item, 0);
}

void CAimProto::setGroupId(HANDLE hContact, int i, unsigned short id)
{
	char item[sizeof(AIM_KEY_GI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_GI)+10, AIM_KEY_GI"%d", i);
	setWord(hContact, item, id);
}

int CAimProto::deleteGroupId(HANDLE hContact, int i)
{
	char item[sizeof(AIM_KEY_GI)+10];
	mir_snprintf(item, sizeof(AIM_KEY_GI)+10, AIM_KEY_GI"%d", i);
	return deleteSetting(hContact, item);
}

/////////////////////////////////////////////////////////////////////////////////////////

int CAimProto::open_contact_file(const char* sn, const TCHAR* file, const char* mode, TCHAR* &path, bool contact_dir)
{
	path = (TCHAR*)mir_alloc(MAX_PATH * sizeof(TCHAR));

	TCHAR *tmpPath = Utils_ReplaceVarsT(_T("%miranda_userdata%"));
	TCHAR *sztModuleName = mir_a2t(m_szModuleName);
	int pos = mir_sntprintf(path, MAX_PATH, _T("%s\\%s"), tmpPath, sztModuleName);
	mir_free(sztModuleName);
	mir_free(tmpPath);
	if  (contact_dir)
	{
		char* norm_sn = normalize_name(sn);
		TCHAR *norm_snt = mir_a2t(m_szModuleName);
		pos += mir_sntprintf(path + pos, MAX_PATH - pos, _T("\\%s"), norm_snt);
		mir_free(norm_snt);
		mir_free(norm_sn);
	}

	if (_taccess(path, 0))
		CreateDirectoryTreeT(path);

	mir_sntprintf(path + pos, MAX_PATH - pos, _T("\\%s"), file);
	int fid = _topen(path, _O_CREAT | _O_RDWR | _O_BINARY, _S_IREAD);
	if (fid < 0)
	{
		TCHAR errmsg[512];
	#if _MSC_VER > 1200
		mir_sntprintf(errmsg, SIZEOF(errmsg), TranslateT("Failed to open file: %s %s"), path, __tcserror(NULL));
	#else
		TCHAR* err = mir_a2t(_strerror(NULL));
		mir_sntprintf(errmsg, SIZEOF(errmsg), TranslateT("Failed to open file: %s %s"), path, err);
		mir_free(err);
	#endif
		ShowPopup((char*)errmsg, ERROR_POPUP | TCHAR_POPUP);
	}
	return fid;
}

void CAimProto::write_away_message(const char* sn, const char* msg, bool utf)
{
	TCHAR* path;
	int fid = open_contact_file(sn, _T("away.html"), "wb", path, 1);
	if (fid >= 0)
	{
		if (utf) _write(fid, "\xEF\xBB\xBF", 3);
		char* s_msg=process_status_msg(msg, sn);
		_write(fid, "<h3>", 4);
		_write(fid, sn, (unsigned)strlen(sn));
		_write(fid, "'s Away Message:</h3>", 21);
		_write(fid, s_msg, (unsigned)strlen(s_msg));
		_close(fid);
		ShellExecute(NULL, _T("open"), path, NULL, NULL, SW_SHOW);
		mir_free(path);
		mir_free(s_msg);
	}
} 

void CAimProto::write_profile(const char* sn, const char* msg, bool utf)
{
	TCHAR* path;
	int fid = open_contact_file(sn, _T("profile.html"),"wb", path, 1);
	if (fid >= 0)
	{
		if (utf) _write(fid, "\xEF\xBB\xBF", 3);
		char* s_msg=process_status_msg(msg, sn);
		_write(fid, "<h3>", 4);
		_write(fid, sn, (unsigned)strlen(sn));
		_write(fid, "'s Profile:</h3>", 16);
		_write(fid, s_msg, (unsigned)strlen(s_msg));
		_close(fid);
		ShellExecute(NULL, _T("open"), path, NULL, NULL, SW_SHOW);
		mir_free(path);
		mir_free(s_msg);
	}
}

unsigned long aim_oft_checksum_chunk(unsigned long dwChecksum, const unsigned char *buffer, int len)
{
	unsigned long checksum = (dwChecksum >> 16) & 0xffff;

	for (int i = 0; i < len; i++)
	{
		unsigned val = buffer[i];

		if ((i & 1) == 0)
			val <<= 8;

		if (checksum < val) ++val;
		checksum -= val;
	}
	checksum = ((checksum & 0x0000ffff) + (checksum >> 16));
	checksum = ((checksum & 0x0000ffff) + (checksum >> 16));
	return checksum << 16;
}

unsigned int aim_oft_checksum_file(TCHAR *filename, unsigned __int64 size) 
{
	unsigned long checksum = 0xffff0000;
	int fid = _topen(filename, _O_RDONLY | _O_BINARY, _S_IREAD);
	if (fid >= 0)  
	{
		unsigned __int64 sz = _filelengthi64(fid);
		if (size > sz) size = sz; 
		while (size)
		{
			unsigned char buffer[8912];
			int bytes = (int)min(size, sizeof(buffer));
			bytes = _read(fid, buffer, bytes);
			size -= bytes;
			checksum = aim_oft_checksum_chunk(checksum, buffer, bytes);
		}
		_close(fid);
	}
	return checksum;
}

char* long_ip_to_char_ip(unsigned long host, char* ip)
{
	host = _htonl(host);
	unsigned char* bytes = (unsigned char*)&host;
	size_t buf_loc = 0;
	for(int i=0; i<4; i++)
	{
		char store[16];
		_itoa(bytes[i], store, 10);
		size_t len = strlen(store);

		memcpy(&ip[buf_loc], store, len);
		buf_loc += len;
		ip[buf_loc++] = '.';
	}
	ip[buf_loc - 1] = '\0';

	return ip;
}

unsigned long char_ip_to_long_ip(char* ip)
{
	unsigned char chost[4] = {0}; 
	char *c = ip;
	for(int i=0; i<4; ++i)
	{
		chost[i] = (unsigned char)atoi(c);
		c = strchr(c, '.');
		if (c) ++c;
		else break;
	}
	return *(unsigned long*)&chost;
}

unsigned short get_random(void)
{
	unsigned short id;
	CallService(MS_UTILS_GETRANDOM, sizeof(id), (LPARAM)&id);
	id &= 0x7fff;
	return id;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Standard functions

int CAimProto::deleteSetting(HANDLE hContact, const char* setting)
{   return db_unset(hContact, m_szModuleName, setting);
}

bool CAimProto::getBool(HANDLE hContact, const char* name, bool defaultValue)
{	return db_get_b(hContact, m_szModuleName, name, defaultValue) != 0;
}

int CAimProto::getByte(const char* name, BYTE defaultValue)
{	return db_get_b(NULL, m_szModuleName, name, defaultValue);
}

int CAimProto::getByte(HANDLE hContact, const char* name, BYTE defaultValue)
{	return db_get_b(hContact, m_szModuleName, name, defaultValue);
}

int CAimProto::getDword(const char* name, DWORD defaultValue)
{	return db_get_dw(NULL, m_szModuleName, name, defaultValue);
}

int CAimProto::getDword(HANDLE hContact, const char* name, DWORD defaultValue)
{	return db_get_dw(hContact, m_szModuleName, name, defaultValue);
}

int CAimProto::getString(const char* name, DBVARIANT* result)
{	return db_get_s(NULL, m_szModuleName, name, result);
}

int CAimProto::getString(HANDLE hContact, const char* name, DBVARIANT* result)
{	return db_get_s(hContact, m_szModuleName, name, result);
}

int CAimProto::getTString(const char* name, DBVARIANT* result)
{	return db_get_ts(NULL, m_szModuleName, name, result);
}

int CAimProto::getTString(HANDLE hContact, const char* name, DBVARIANT* result)
{	return db_get_ts(hContact, m_szModuleName, name, result);
}

WORD CAimProto::getWord(const char* name, WORD defaultValue)
{	return (WORD)db_get_w(NULL, m_szModuleName, name, defaultValue);
}

WORD CAimProto::getWord(HANDLE hContact, const char* name, WORD defaultValue)
{	return (WORD)db_get_w(hContact, m_szModuleName, name, defaultValue);
}

char* CAimProto::getSetting(HANDLE hContact, const char* setting)
{
	DBVARIANT dbv;
	return db_get_s(hContact, m_szModuleName, setting, &dbv) ? 
		NULL : dbv.pszVal;
}

char* CAimProto::getSetting(const char* setting)
{
	DBVARIANT dbv;
	return db_get_s(NULL, m_szModuleName, setting, &dbv) ? 
		NULL : dbv.pszVal;
}

void CAimProto::setByte(const char* name, BYTE value)
{	db_set_b(NULL, m_szModuleName, name, value);
}

void CAimProto::setByte(HANDLE hContact, const char* name, BYTE value)
{	db_set_b(hContact, m_szModuleName, name, value);
}

void CAimProto::setDword(const char* name, DWORD value)
{	db_set_dw(NULL, m_szModuleName, name, value);
}

void CAimProto::setDword(HANDLE hContact, const char* name, DWORD value)
{	db_set_dw(hContact, m_szModuleName, name, value);
}

void CAimProto::setString(const char* name, const char* value)
{	db_set_s(NULL, m_szModuleName, name, value);
}

void CAimProto::setString(HANDLE hContact, const char* name, const char* value)
{	db_set_s(hContact, m_szModuleName, name, value);
}

void CAimProto::setTString(const char* name, const TCHAR* value)
{	db_set_ts(NULL, m_szModuleName, name, value);
}

void CAimProto::setTString(HANDLE hContact, const char* name, const TCHAR* value)
{	db_set_ts(hContact, m_szModuleName, name, value);
}

void CAimProto::setWord(const char* name, WORD value)
{	db_set_w(NULL, m_szModuleName, name, value);
}

void CAimProto::setWord(HANDLE hContact, const char* name, WORD value)
{	db_set_w(hContact, m_szModuleName, name, value);
}

int  CAimProto::sendBroadcast(HANDLE hContact, int type, int result, HANDLE hProcess, LPARAM lParam)
{
	return ProtoBroadcastAck(m_szModuleName, hContact, type, result, hProcess, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////

void CAimProto::CreateProtoService(const char* szService, AimServiceFunc serviceProc)
{
	char temp[MAX_PATH*2];

	mir_snprintf(temp, sizeof(temp), "%s%s", m_szModuleName, szService);
	CreateServiceFunctionObj(temp, (MIRANDASERVICEOBJ)*(void**)&serviceProc, this);
}

void CAimProto::HookProtoEvent(const char* szEvent, AimEventFunc pFunc)
{
	::HookEventObj(szEvent, (MIRANDAHOOKOBJ)*(void**)&pFunc, this);
}

void CAimProto::ForkThread(AimThreadFunc pFunc, void* param)
{
	UINT threadID;
	CloseHandle((HANDLE)mir_forkthreadowner((pThreadFuncOwner)*(void**)&pFunc, this, param, &threadID));
}
