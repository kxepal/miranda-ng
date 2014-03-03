/*
former MetaContacts Plugin for Miranda IM.

Copyright � 2014 Miranda NG Team
Copyright � 2004-07 Scott Ellis
Copyright � 2004 Universite Louis PASTEUR, STRASBOURG.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "..\..\core\commonheaders.h"

#include "metacontacts.h"

HANDLE invisiGroup;
POINT menuMousePoint;

INT_PTR Mydb_get(MCONTACT hContact, const char *szModule, const char *szSetting, DBVARIANT *dbv)
{
	memset(dbv, 0, sizeof(DBVARIANT));
	return db_get_s(hContact, szModule, szSetting, dbv, 0);
}

int Meta_EqualDBV(DBVARIANT *dbv, DBVARIANT *id) {
	DWORD res = 1;
	int i;
	if (dbv->type == id->type)
	{	// If the id parameter and the value returned by the db_get
		// are the same, this is the correct HANDLE, return it.
		switch(dbv->type)
		{
			case DBVT_DELETED:
				return 1;
			case DBVT_BYTE:
				if (dbv->bVal == id->bVal)
					return 1;
			case DBVT_WORD:
				if (dbv->wVal == id->wVal)
					return 1;
			case DBVT_DWORD:
				if (dbv->dVal == id->dVal)
					return 1;
				break;
			case DBVT_ASCIIZ:
			case DBVT_UTF8:
				if ( !strcmp(dbv->pszVal,id->pszVal))
					return 1;
			case DBVT_WCHAR:
				if ( !wcscmp(dbv->pwszVal,id->pwszVal))
					return 1;
			case DBVT_BLOB:
				if (dbv->cpbVal == id->cpbVal)
				{
					for (i=dbv->cpbVal;res && i<=0;i--)
						res = (dbv->pbVal[i] == id->pbVal[i]);
					if (res)
						return 1;
				}
				break;
		}
	}

	return 0;
}

/** Retrieve a \c HANDLE from a protocol and an identifier
*
* @param protocol :	Name of the protocol
* @param id :		Unique field identifiying the contact searched
*
* @return			a \c HANDLE to the specified contact or \c NULL
					if no contact has been found.
*/
MCONTACT Meta_GetHandle(const char *protocol, DBVARIANT *id)
{
	char *field;
	DBVARIANT dbv;
	DWORD i,res = 1;

	// Get the field identifying the contact in the database.
	if ( !ProtoServiceExists(protocol, PS_GETCAPS))
		return NULL;
	
	field = (char *)CallProtoService(protocol,PS_GETCAPS,PFLAG_UNIQUEIDSETTING,0);

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		// Scan the database and retrieve the field for each contact
		if ( !db_get(hContact, protocol, field, &dbv)) {
			if (dbv.type == id->type) {
				// If the id parameter and the value returned by the db_get
				// are the same, this is the correct HANDLE, return it.
				switch(dbv.type) {
				case DBVT_DELETED:
					break;
				case DBVT_BYTE:
					if (dbv.bVal == id->bVal)
						return hContact;
					break;
				case DBVT_WORD:
					if (dbv.wVal == id->wVal)
						return hContact;
					break;
				case DBVT_DWORD:
					if (dbv.dVal == id->dVal)
						return hContact;
					break;
				case DBVT_ASCIIZ:
				case DBVT_UTF8:
					if ( !strcmp(dbv.pszVal,id->pszVal)) {
						db_free(&dbv);
						return hContact;
					}
					db_free(&dbv);
					break;

				case DBVT_WCHAR:
					if ( !wcscmp(dbv.pwszVal,id->pwszVal)) {
						db_free(&dbv);
						return hContact;
					}
					db_free(&dbv);
					break;

				case DBVT_BLOB:
					if (dbv.cpbVal == id->cpbVal) {
						for (i=dbv.cpbVal;res && i<=0;i--)
							res = (dbv.pbVal[i] == id->pbVal[i]);
						if (res) {
							db_free(&dbv);
							return hContact;
						}
					}
					db_free(&dbv);
					break;
				} // end switch
			}
			else db_free(&dbv);
		}
	}
	return NULL;
}

/** Update the MetaContact login, depending on the protocol desired
*
* The login of the "MetaContacts" protocol will be copied from the login
* of the specified protocol.
*
* @param szProto :	The name of the protocol used to get the login that will be
*					affected to the "MetaContacts" protocol.
*
* @return			O on success, 1 otherwise.
*/
int Meta_SetNick(char *szProto)
{
	CONTACTINFO ci;
	ci.cbSize = sizeof(CONTACTINFO);
	ci.dwFlag = CNF_DISPLAY | CNF_TCHAR;
	ci.hContact = NULL;
	ci.szProto = szProto;
	if (CallService(MS_CONTACT_GETCONTACTINFO,0,(LPARAM)&ci))
		return 1;

	switch(ci.type) {
	case CNFT_BYTE:
		if ( db_set_b(NULL, META_PROTO, "Nick", ci.bVal))
			return 1;
		break;
	case CNFT_WORD:
		if ( db_set_w(NULL, META_PROTO, "Nick", ci.wVal))
			return 1;
		break;
	case CNFT_DWORD:
		if ( db_set_dw(NULL, META_PROTO, "Nick", ci.dVal))
			return 1;
		break;
	case CNFT_ASCIIZ:
		if ( db_set_ts(NULL, META_PROTO, "Nick", ci.pszVal))
			return 1;
		mir_free(ci.pszVal);
		break;
	default:
		if ( db_set_s(NULL, META_PROTO, "Nick",(char *)TranslateT("Sender")))
			return 1;
		break;
	}
	return 0;
}

/** Assign a contact (src) to a metacontact (dest)
*
* @param src : HANDLE to a contact that should be assigned
* @param dest : HANDLE to a metacontact that will host the contact
* @param set_as_default : bool flag to indicate whether the new contact becomes the default
*
* @return TRUE on success, FALSE otherwise
*/

BOOL Meta_Assign(MCONTACT src, MCONTACT dest, BOOL set_as_default)
{
	DWORD num_contacts;
	char buffer[512], szId[40];
	WORD status;
	MCONTACT most_online;

	DBCachedContact *ccDest = CheckMeta(dest);
	if (ccDest == NULL)
		return FALSE;

	if ((num_contacts = db_get_dw(dest, META_PROTO, "NumContacts", INVALID_CONTACT_ID)) == -1) {
		MessageBox(0, TranslateT("Could not retreive MetaContact contact count"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	char *szProto = GetContactProto(src);
	if (szProto == NULL) {
		MessageBox(0, TranslateT("Could not retreive contact protocol"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	// Get the login of the subcontact
	char *field = (char *)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);
	DBVARIANT dbv;
	if (db_get(src, szProto, field, &dbv)) {
		MessageBox(0, TranslateT("Could not get unique ID of contact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	// Check that is is 'on the list'
	if (db_get_b(src, "CList", "NotOnList", 0) == 1) {
		MessageBox(0, TranslateT("Contact is 'Not on List' - please add the contact to your contact list before assigning."), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	num_contacts++;
	if (num_contacts >= MAX_CONTACTS) {
		MessageBox(0, TranslateT("MetaContact is full"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	// write the contact's protocol
	strcpy(buffer, "Protocol");
	strcat(buffer, _itoa(num_contacts - 1, szId, 10));

	if (db_set_s(dest, META_PROTO, buffer, szProto)) {
		MessageBox(0, TranslateT("Could not write contact protocol to MetaContact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	// write the login
	strcpy(buffer, "Login");
	strcat(buffer, szId);

	if (db_set(dest, META_PROTO, buffer, &dbv)) {
		MessageBox(0, TranslateT("Could not write unique ID of contact to MetaContact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
		db_free(&dbv);
		return FALSE;
	}

	db_free(&dbv);

	// If we can get the nickname of the subcontact...
	if (!db_get(src, szProto, "Nick", &dbv)) {
		// write the nickname
		strcpy(buffer, "Nick");
		strcat(buffer, szId);
		if (db_set(dest, META_PROTO, buffer, &dbv)) {
			MessageBox(0, TranslateT("Could not write nickname of contact to MetaContact"), TranslateT("Assignment error"), MB_OK | MB_ICONWARNING);
			return FALSE;
		}

		db_free(&dbv);
	}

	// write the display name
	strcpy(buffer, "CListName");
	strcat(buffer, szId);
	db_set_ts(dest, META_PROTO, buffer, cli.pfnGetContactDisplayName(src, 0));

	// Get the status
	if (!szProto)
		status = ID_STATUS_OFFLINE;
	else
		status = db_get_w(src, szProto, "Status", ID_STATUS_OFFLINE);

	// write the status
	strcpy(buffer, "Status");
	strcat(buffer, szId);
	db_set_w(dest, META_PROTO, buffer, status);

	// write the handle
	strcpy(buffer, "Handle");
	strcat(buffer, szId);
	db_set_dw(dest, META_PROTO, buffer, (DWORD)src);

	// write status string
	strcpy(buffer, "StatusString");
	strcat(buffer, szId);

	TCHAR *szStatus = cli.pfnGetStatusModeDescription(status, 0);
	db_set_ts(dest, META_PROTO, buffer, szStatus);

	// Write the link in the contact
	db_set_dw(src, META_PROTO, "ParentMeta", dest);

	// update count of contacts
	db_set_dw(dest, META_PROTO, "NumContacts", num_contacts);

	if (set_as_default) {
		db_set_dw(dest, META_PROTO, "Default", (WORD)(num_contacts - 1));
		NotifyEventHooks(hEventDefaultChanged, (WPARAM)dest, (LPARAM)src);
	}

	// set nick to most online contact that can message
	most_online = Meta_GetMostOnline(ccDest);
	Meta_CopyContactNick(ccDest, most_online);

	// set status to that of most online contact
	Meta_FixStatus(ccDest);

	// if the new contact is the most online contact with avatar support, get avatar info
	most_online = Meta_GetMostOnlineSupporting(ccDest, PFLAGNUM_4, PF4_AVATARS);
	if (most_online == src) {
		PROTO_AVATAR_INFORMATIONT AI;
		AI.cbSize = sizeof(AI);
		AI.hContact = dest;
		AI.format = PA_FORMAT_UNKNOWN;
		_tcscpy(AI.filename, _T("X"));

		if (CallProtoService(META_PROTO, PS_GETAVATARINFOT, 0, (LPARAM)&AI) == GAIR_SUCCESS)
			db_set_ts(dest, "ContactPhoto", "File", AI.filename);
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!
	// copyHistory(src, dest);

	// Ignore status if the option is on
	if (options.suppress_status)
		CallService(MS_IGNORE_IGNORE, (WPARAM)src, (WPARAM)IGNOREEVENT_USERONLINE);

	// copy other data
	Meta_CopyData(ccDest);

	NotifyEventHooks(hSubcontactsChanged, (WPARAM)dest, 0);
	return TRUE;
}

/**
*	Convenience method - get most online contact supporting messaging
*
*/

MCONTACT Meta_GetMostOnline(DBCachedContact *cc)
{
	return Meta_GetMostOnlineSupporting(cc, PFLAGNUM_1, PF1_IM);
}

/** Get the 'most online' contact for a meta contact (according to above order) which supports the specified
* protocol service, and copies nick from that contact
*
* @param hMetaHANDLE to a metacontact
*
* @return HANDLE to a contact
*/

MCONTACT Meta_GetMostOnlineSupporting(DBCachedContact *cc, int pflagnum, unsigned long capability)
{
	if (cc == NULL)
		return 0;

	int most_online_status = ID_STATUS_OFFLINE;
	MCONTACT hContact;
	int i, default_contact_number, num_contacts;

	// you can't get more online than having the default contact ONLINE - so check that first
	if ((default_contact_number = db_get_dw(cc->contactID, META_PROTO, "Default", INVALID_CONTACT_ID)) == INVALID_CONTACT_ID) {
		// This is a simple contact - return NULL to signify error.
		// (this should normally not happen, since all meta contacts have a default contact)
		return NULL;
	}

	// if the default is beyond the end of the list (eek!) return null
	if (default_contact_number >= (num_contacts = db_get_dw(cc->contactID, META_PROTO, "NumContacts", 0)))
		return NULL;

	MCONTACT most_online_contact = Meta_GetContactHandle(cc, default_contact_number);
	char *szProto = GetContactProto(most_online_contact);
	DWORD caps = szProto ? CallProtoService(szProto, PS_GETCAPS, (WPARAM)pflagnum, 0) : 0;
	if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
	// we are forced to do use default for sending - '-1' capability indicates no specific capability, but respect 'Force Default'
	if (szProto && db_get_b(cc->contactID, META_PROTO, "ForceDefault", 0) && capability != 0 && (capability == -1 || (caps & capability) == capability)) // capability is 0 when we're working out status
		return most_online_contact;

	// a subcontact is being temporarily 'forced' to do sending
	if ((most_online_contact = db_get_dw(cc->contactID, META_PROTO, "ForceSend", 0))) {
		caps = szProto ? CallProtoService(szProto, PS_GETCAPS, (WPARAM)pflagnum, 0) : 0;
		if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
		if (szProto && (caps & capability) == capability && capability != 0) // capability is 0 when we're working out status
			return most_online_contact;
	}

	most_online_contact = Meta_GetContactHandle(cc, default_contact_number);
	szProto = GetContactProto(most_online_contact);
	if (szProto && CallProtoService(szProto, PS_GETSTATUS, 0, 0) >= ID_STATUS_ONLINE) {
		caps = szProto ? CallProtoService(szProto, PS_GETCAPS, (WPARAM)pflagnum, 0) : 0;
		if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
		if (szProto && (capability == -1 || (caps & capability) == capability)) {
			most_online_status = db_get_w(most_online_contact, szProto, "Status", ID_STATUS_OFFLINE);

			// if our default is not offline, and option to use default is set - return default
			// and also if our default is online, return it
			if (most_online_status == ID_STATUS_ONLINE || (most_online_status != ID_STATUS_OFFLINE && options.always_use_default))
				return most_online_contact;
		}
		else most_online_status = ID_STATUS_OFFLINE;
	}
	else most_online_status = ID_STATUS_OFFLINE;

	char *most_online_proto = szProto;
	// otherwise, check all the subcontacts for the one closest to the ONLINE state which supports the required capability
	for (i = 0; i < num_contacts; i++) {
		if (i == default_contact_number) // already checked that (i.e. initial value of most_online_contact and most_online_status are those of the default contact)
			continue;

		hContact = Meta_GetContactHandle(cc, i);
		szProto = GetContactProto(hContact);

		if (!szProto || CallProtoService(szProto, PS_GETSTATUS, 0, 0) < ID_STATUS_ONLINE) // szProto offline or connecting
			continue;

		caps = szProto ? CallProtoService(szProto, PS_GETCAPS, (WPARAM)pflagnum, 0) : 0;
		if (szProto && strcmp(szProto, "IRC") == 0) caps |= PF1_IM;
		if (szProto && (capability == -1 || (caps & capability) == capability)) {
			int status = db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
			if (status == ID_STATUS_ONLINE) {
				most_online_contact = hContact;
				most_online_proto = szProto;
				return most_online_contact;
			}
			if (status <= ID_STATUS_OFFLINE) // status below ID_STATUS_OFFLINE is a connecting status
				continue;

			if (GetRealPriority(szProto, status) < GetRealPriority(most_online_proto, most_online_status)) {
				most_online_status = status;
				most_online_contact = hContact;
				most_online_proto = szProto;
			}
		}
	}

	// no online contacts? if we're trying to message, use 'send offline' contact
	if (most_online_status == ID_STATUS_OFFLINE && capability == PF1_IM) {
		MCONTACT hOffline = Meta_GetContactHandle(cc, db_get_dw(cc->contactID, META_PROTO, "OfflineSend", INVALID_CONTACT_ID));
		if (hOffline)
			most_online_contact = hOffline;
	}

	return most_online_contact;
}

DBCachedContact* CheckMeta(MCONTACT hMeta)
{
	DBCachedContact *cc = currDb->m_cache->GetCachedContact(hMeta);
	return (cc == NULL || cc->nSubs == -1) ? NULL : cc;
}

int Meta_GetContactNumber(DBCachedContact *cc, MCONTACT hContact)
{
	for (int i = 0; i < cc->nSubs; i++)
		if (cc->pSubs[i] == hContact)
			return i;

	return -1;
}

BOOL dbv_same(DBVARIANT *dbv1, DBVARIANT *dbv2)
{
	if (dbv1->type != dbv2->type) return FALSE;

	switch (dbv1->type) {
	case DBVT_BYTE:
		return dbv1->bVal == dbv2->bVal;
	case DBVT_WORD:
		return dbv1->wVal == dbv2->wVal;
	case DBVT_DWORD:
		return dbv1->dVal == dbv2->dVal;
	case DBVT_ASCIIZ:
		return !strcmp(dbv1->pszVal, dbv2->pszVal);
	case DBVT_BLOB:
		return (dbv1->cpbVal == dbv2->cpbVal && !memcmp(dbv1->pbVal, dbv2->pbVal, dbv1->cpbVal));
		break;
	default:
		return FALSE;
	}
}

void copy_settings_array(DBCachedContact *ccMeta, char *module, const char *settings[], int num_settings)
{
	int num_contacts = db_get_dw(ccMeta->contactID, META_PROTO, "NumContacts", INVALID_CONTACT_ID),
		default_contact = db_get_dw(ccMeta->contactID, META_PROTO, "Default", INVALID_CONTACT_ID),
		most_online = Meta_GetContactNumber(ccMeta, Meta_GetMostOnline(ccMeta));

	BOOL use_default = FALSE;
	int source_contact = (use_default ? default_contact : most_online);
	if (source_contact < 0 || source_contact >= num_contacts)
		return;

	for (int i = 0; i < num_settings; i++) {
		BOOL bDataWritten = FALSE;
		for (int j = 0; j < num_contacts && !bDataWritten; j++) {
			// do source (most online) first
			MCONTACT hContact;
			if (j == 0)
				hContact = Meta_GetContactHandle(ccMeta, source_contact);
			else if (j <= source_contact)
				hContact = Meta_GetContactHandle(ccMeta, j - 1);
			else
				hContact = Meta_GetContactHandle(ccMeta, j);

			if (hContact == 0)
				continue;

			char *used_mod;
			if (!module) {
				used_mod = GetContactProto(hContact);
				if (!used_mod)
					continue; // next contact
			}
			else used_mod = module;

			if (j == 0 && strcmp(settings[i], "MirVer") == 0) //Always reset MirVer
				db_unset(ccMeta->contactID, (module ? used_mod : META_PROTO), settings[i]);

			DBVARIANT dbv1, dbv2;
			BOOL bFree, got_val = !Mydb_get(hContact, used_mod, settings[i], &dbv2);
			if (got_val) {
				bFree = !Mydb_get(ccMeta->contactID, (module ? used_mod : META_PROTO), settings[i], &dbv1);

				if (strcmp(settings[i], "MirVer") == 0) {
					if (db_get_w(hContact, used_mod, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
						if (!bFree || (dbv1.pszVal == NULL || strcmp(dbv1.pszVal, "") == 0 || strlen(dbv1.pszVal) < 2)) {
							db_set(ccMeta->contactID, (module ? used_mod : META_PROTO), settings[i], &dbv2);
							bDataWritten = TRUE; //only break if found something to copy
						}
					}
				}
				else {
					if (!bFree || !dbv_same(&dbv1, &dbv2)) {
						db_set(ccMeta->contactID, (module ? used_mod : META_PROTO), settings[i], &dbv2);
						if (dbv2.type == DBVT_ASCIIZ || dbv2.type == DBVT_UTF8) {
							if (dbv2.pszVal != NULL && strcmp(dbv2.pszVal, "") != 0)
								bDataWritten = TRUE; //only break if found something to copy
						}
						else if (dbv2.type == DBVT_WCHAR) {
							if (dbv2.pwszVal != 0 && wcscmp(dbv2.pwszVal, L"") != 0)
								bDataWritten = TRUE; //only break if found something to copy
						}
						else bDataWritten = TRUE; //only break if found something to copy
					}
					else bDataWritten = TRUE;
				}
				db_free(&dbv2);
				if (bFree)
					db_free(&dbv1);
			}
		}
	}
}

const char *ProtoSettings[25] =
{ "BirthDay", "BirthMonth", "BirthYear", "Age", "Cell", "Cellular", "Homepage", "email", "Email", "e-mail",
"FirstName", "MiddleName", "LastName", "Title", "Timezone", "Gender", "MirVer", "ApparentMode", "IdleTS", "LogonTS", "IP", "RealIP",
"Auth", "ListeningTo", "Country" };
const char *UserInfoSettings[71] =
{ "NickName", "FirstName", "MiddleName", "LastName", "Title", "Timezone", "Gender", "DOBd", "DOBm", "DOBy",
"Mye-mail0", "Mye-mail1", "MyPhone0", "MyPhone1", "MyNotes", "PersonalWWW",
"HomePhone", "HomeFax", "HomeMobile", "HomeStreet", "HomeCity", "HomeState", "HomeZip", "HomeCountry",
"WorkPhone", "WorkFax", "WorkMobile", "WorkStreet", "WorkCity", "WorkState", "WorkZip", "WorkCountry", "Company", "Department", "Position",
"Occupation", "Cellular", "Cell", "Phone", "Notes",

"e-mail", "e-mail0", "e-mail1", "Homepage", "MaritalStatus",
"CompanyCellular", "CompanyCity", "CompanyState", "CompanyStreet", "CompanyCountry", "Companye-mail",
"CompanyHomepage", "CompanyDepartment", "CompanyOccupation", "CompanyPosition", "CompanyZip",

"OriginCity", "OriginState", "OriginStreet", "OriginCountry", "OriginZip",
"City", "State", "Street", "Country", "Zip",

"Language1", "Language2", "Language3", "Partner", "Gender" };
const char *ContactPhotoSettings[5] =
{ "File", "Backup", "Format", "ImageHash", "RFile" };
const char *MBirthdaySettings[3] =
{ "BirthDay", "BirthMonth", "BirthYear" };

// special handling for status message
// copy from first subcontact with any of these values that has the same status as the most online contact
// szProto: 
// clist: "StatusMsg"

void CopyStatusData(DBCachedContact *ccMeta)
{
	int num_contacts = db_get_dw(ccMeta->contactID, META_PROTO, "NumContacts", INVALID_CONTACT_ID),
		most_online = Meta_GetContactNumber(ccMeta, Meta_GetMostOnline(ccMeta));
	WORD status = db_get_w(ccMeta->contactID, META_PROTO, "Status", ID_STATUS_OFFLINE);
	MCONTACT hContact;
	BOOL bDoneStatus = FALSE, bDoneXStatus = FALSE;

	for (int i = 0; i < num_contacts; i++) {
		if (i == 0)
			hContact = Meta_GetContactHandle(ccMeta, most_online);
		else if (i <= most_online)
			hContact = Meta_GetContactHandle(ccMeta, i - 1);
		else
			hContact = Meta_GetContactHandle(ccMeta, i);

		char *szProto = GetContactProto(hContact);

		if (szProto && db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE) == status) {
			DBVARIANT dbv;
			if (!bDoneStatus && !Mydb_get(hContact, "CList", "StatusMsg", &dbv)) {
				db_set(ccMeta->contactID, "CList", "StatusMsg", &dbv);
				db_free(&dbv);
				bDoneStatus = TRUE;
			}
			if ((!bDoneXStatus) && (!Mydb_get(hContact, szProto, "XStatusId", &dbv)) && dbv.type != DBVT_DELETED) {
				db_set_s(ccMeta->contactID, META_PROTO, "XStatusProto", szProto);
				db_set(ccMeta->contactID, META_PROTO, "XStatusId", &dbv);

				db_free(&dbv);
				if (!Mydb_get(hContact, szProto, "XStatusMsg", &dbv)) {
					db_set(ccMeta->contactID, META_PROTO, "XStatusMsg", &dbv);
					db_free(&dbv);
				}
				if (!Mydb_get(hContact, szProto, "XStatusName", &dbv)) {
					db_set(ccMeta->contactID, META_PROTO, "XStatusName", &dbv);
					db_free(&dbv);
				}
				bDoneXStatus = TRUE;
			}
		}

		if (bDoneStatus && bDoneXStatus)
			break;
	}

	if (!bDoneStatus)
		db_unset(ccMeta->contactID, "CList", "StatusMsg");

	if (!bDoneXStatus) {
		db_unset(ccMeta->contactID, META_PROTO, "XStatusId");
		db_unset(ccMeta->contactID, META_PROTO, "XStatusMsg");
		db_unset(ccMeta->contactID, META_PROTO, "XStatusName");
	}
}

void Meta_CopyData(DBCachedContact *cc)
{
	if (!options.copydata || cc == NULL)
		return;

	CopyStatusData(cc);

	copy_settings_array(cc, 0, ProtoSettings, 25);
	copy_settings_array(cc, "mBirthday", UserInfoSettings, 3);
	copy_settings_array(cc, "ContactPhoto", ContactPhotoSettings, 5);

	if (options.copy_userinfo)
		copy_settings_array(cc, "UserInfo", UserInfoSettings, 71);
}

MCONTACT Meta_GetContactHandle(DBCachedContact *cc, int contact_number)
{
	if (contact_number >= cc->nSubs || contact_number < 0)
		return 0;

	return cc->pSubs[contact_number];
}

/** Hide all contacts linked to any meta contact, and set handle links
*
* Additionally, set all sub contacts and metacontacts to offline so that status notifications are always sent
*
* and ensure metafilter in place
*/

#define szMsg LPGEN("The 'MetaContacts Hidden Group' has been added to your contact list.\n\
This is most likely due to server-side contact information. To fix this, so that\n\
MetaContacts continues to function correctly, you should:\n\
   - disable MetaContacts using the 'Toggle MetaContacts Off' main menu item\n\
   - move all contacts out of this group\n\
   - synchronize your contacts with the server\n\
   - re-enable MetaContacts")

int Meta_HideLinkedContacts(void)
{
	DBVARIANT dbv, dbv2;
	char buffer[512], buffer2[512];

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		DBCachedContact *cc = currDb->m_cache->GetCachedContact(hContact);
		if (cc == NULL || cc->parentID == 0)
			continue;

		DBCachedContact *ccMeta = CheckMeta(cc->parentID);
		if (ccMeta == NULL)
			continue;

		// get contact number
		int contact_number = Meta_GetContactNumber(cc, hContact);

		// prepare to update metacontact record of subcontat status
		char *szProto = GetContactProto(hContact);

		// find metacontact
		if (contact_number < 0 || contact_number >= ccMeta->nSubs)
			continue;

		// update metacontact's record of status for this contact
		strcpy(buffer, "Status");
		strcat(buffer, _itoa(contact_number, buffer2, 10));

		WORD status = (!szProto) ? ID_STATUS_OFFLINE : db_get_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
		db_set_w(ccMeta->contactID, META_PROTO, buffer, status);

		// update metacontact's record of nick for this contact
		if (szProto && !db_get(hContact, szProto, "Nick", &dbv)) {
			strcpy(buffer, "Nick");
			strcat(buffer, _itoa(contact_number, buffer2, 10));
			db_set(ccMeta->contactID, META_PROTO, buffer, &dbv);

			strcpy(buffer, "CListName");
			strcat(buffer, _itoa(contact_number, buffer2, 10));
			if (db_get(hContact, "CList", "MyHandle", &dbv2))
				db_set(ccMeta->contactID, META_PROTO, buffer, &dbv);
			else {
				db_set(ccMeta->contactID, META_PROTO, buffer, &dbv2);
				db_free(&dbv2);
			}

			db_free(&dbv);
		}
		else {
			if (!db_get(hContact, "CList", "MyHandle", &dbv)) {
				strcpy(buffer, "CListName");
				strcat(buffer, _itoa(contact_number, buffer2, 10));
				db_set(ccMeta->contactID, META_PROTO, buffer, &dbv);
				db_free(&dbv);
			}
		}

		if (options.suppress_status)
			CallService(MS_IGNORE_IGNORE, hContact, (WPARAM)IGNOREEVENT_USERONLINE);
	}

	// do metacontacts after handles set properly above
	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		DBCachedContact *cc = CheckMeta(hContact);
		if (cc == NULL)
			continue;

		MCONTACT hMostOnline = Meta_GetMostOnline(cc); // set nick
		Meta_CopyContactNick(cc, hMostOnline);
		Meta_FixStatus(cc);
	}

	return 0;
}

int Meta_HideMetaContacts(int hide)
{
	// set status suppression
	if (hide)
		Meta_SuppressStatus(FALSE);
	else
		Meta_SuppressStatus(options.suppress_status);

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		if (CheckMeta(hContact) == NULL)
			continue;

		if (hide)
			db_set_b(hContact, "CList", "Hidden", 1);
		else
			db_unset(hContact, "CList", "Hidden");
	}

	return 0;
}

int Meta_SuppressStatus(BOOL suppress)
{
	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact))
		if (db_mc_isSub(hContact))
			CallService((suppress) ? MS_IGNORE_IGNORE : MS_IGNORE_UNIGNORE, hContact, IGNOREEVENT_USERONLINE);

	return 0;
}

int Meta_CopyContactNick(DBCachedContact *ccMeta, MCONTACT hContact)
{
	DBVARIANT dbv, dbv_proto;

	if (options.lockHandle)
		hContact = Meta_GetContactHandle(ccMeta, 0);

	if (!hContact)
		return 1;

	// szProto = GetContactProto(hContact);
	// read szProto direct from db, since we do this on load and other szProto plugins may not be loaded yet
	if (!db_get(hContact, "Protocol", "p", &dbv_proto)) {
		char *szProto = dbv_proto.pszVal;
		if (options.clist_contact_name == CNNT_NICK && szProto) {
			if (!Mydb_get(hContact, szProto, "Nick", &dbv)) {
				db_set(ccMeta->contactID, META_PROTO, "Nick", &dbv);
				db_free(&dbv);
				//CallService(MS_CLIST_INVALIDATEDISPLAYNAME, (WPARAM)hMeta, 0);
				//CallService(MS_CLUI_CONTACTRENAMED, (WPARAM)hMeta, 0);
				db_free(&dbv_proto);
				return 0;
			}
		}
		else if (options.clist_contact_name == CNNT_DISPLAYNAME) {
			TCHAR *name = cli.pfnGetContactDisplayName(hContact, 0);
			if (name && _tcscmp(name, TranslateT("(Unknown Contact)")) != 0) {
				db_set_ts(ccMeta->contactID, META_PROTO, "Nick", name);
				db_free(&dbv_proto);
				return 0;
			}
		}
		db_free(&dbv_proto);
	}
	return 1;
}

int Meta_SetAllNicks()
{
	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		DBCachedContact *cc = CheckMeta(hContact);
		if (cc == NULL)
			continue;
		MCONTACT most_online = Meta_GetMostOnline(cc);
		Meta_CopyContactNick(cc, most_online);
		Meta_FixStatus(cc);
		Meta_CopyData(cc);
	}
	return 0;
}

int Meta_SwapContacts(DBCachedContact *cc, DWORD contact_number1, DWORD contact_number2)
{
	DBVARIANT dbv1, dbv2;

	MCONTACT hContact1 = Meta_GetContactHandle(cc, contact_number1), hContact2 = Meta_GetContactHandle(cc, contact_number2);
	char buff1[512], buff12[512], buff2[512], buff22[512];
	BOOL ok1, ok2;

	// swap the protocol
	strcpy(buff1, "Protocol");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "Protocol");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok2 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	if (ok2) {
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}

	// swap the status
	strcpy(buff1, "Status");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "Status");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	if (ok2) {
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}

	// swap the status string
	strcpy(buff1, "StatusString");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "StatusString");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok2 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	if (ok2) {
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}

	// swap the login
	strcpy(buff1, "Login");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "Login");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok2 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_unset(cc->contactID, META_PROTO, buff2);
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	if (ok2) {
		db_unset(cc->contactID, META_PROTO, buff1);
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}

	// swap the nick
	strcpy(buff1, "Nick");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "Nick");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok2 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	else db_unset(cc->contactID, META_PROTO, buff2);

	if (ok2) {
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}
	else db_unset(cc->contactID, META_PROTO, buff1);

	// swap the clist name
	strcpy(buff1, "CListName");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "CListName");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok2 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	else db_unset(cc->contactID, META_PROTO, buff2);

	if (ok2) {
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}
	else db_unset(cc->contactID, META_PROTO, buff1);

	// swap the handle
	strcpy(buff1, "Handle");
	strcat(buff1, _itoa(contact_number1, buff12, 10));
	strcpy(buff2, "Handle");
	strcat(buff2, _itoa(contact_number2, buff22, 10));
	ok1 = !Mydb_get(cc->contactID, META_PROTO, buff1, &dbv1);
	ok2 = !Mydb_get(cc->contactID, META_PROTO, buff2, &dbv2);
	if (ok1) {
		db_set(cc->contactID, META_PROTO, buff2, &dbv1);
		db_free(&dbv1);
	}
	else db_unset(cc->contactID, META_PROTO, buff2);

	if (ok2) {
		db_set(cc->contactID, META_PROTO, buff1, &dbv2);
		db_free(&dbv2);
	}
	else db_unset(cc->contactID, META_PROTO, buff1);

	return 0;
}

INT_PTR CALLBACK DlgProcNull(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		HWND prog = GetDlgItem(hwndDlg, IDC_PROG);

		TranslateDialogDefault(hwndDlg);

		SendMessage(prog, PBM_SETPOS, 0, 0);
		return TRUE;
	}
	return FALSE;
}

void Meta_FixStatus(DBCachedContact *cc)
{
	MCONTACT most_online = Meta_GetMostOnlineSupporting(cc, PFLAGNUM_1, 0);
	if (most_online) {
		char *szProto = GetContactProto(most_online);
		if (szProto) {
			WORD status = db_get_w(most_online, szProto, "Status", ID_STATUS_OFFLINE);
			db_set_w(cc->contactID, META_PROTO, "Status", status);
		}
		else db_set_w(cc->contactID, META_PROTO, "Status", ID_STATUS_OFFLINE);
	}
	else db_set_w(cc->contactID, META_PROTO, "Status", ID_STATUS_OFFLINE);
}

INT_PTR Meta_IsEnabled()
{
	return db_get_b(0, META_PROTO, "Enabled", 1);
}
