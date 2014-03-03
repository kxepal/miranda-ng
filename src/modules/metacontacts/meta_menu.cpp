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

#include <m_nudge.h>
#include "metacontacts.h"

static HGENMENU hMenuContact[MAX_CONTACTS];

static HGENMENU
	hMenuRoot,			 // root menu item of all subs
	hMenuConvert,      // HANDLE to the convert menu item.
	hMenuAdd,          // HANDLE to the add to menu item.
	hMenuEdit,         // HANDLE to the edit menu item.
	hMenuDelete,       // HANDLE to the delete menu item.
	hMenuDefault,      // HANDLE to the delete menu item.
	hMenuForceDefault, // HANDLE to the delete menu item.
	hMenuOnOff;	       // HANDLE to the enable/disable menu item.

/** Convert the contact chosen into a MetaContact.
*
* Create a new MetaContact, remove the selected contact from the \c CList
* and attach it to the MetaContact.
*
* @param wParam : HANDLE to the contact that has been chosen.
* @param lParam :	Allways set to 0.
*/

INT_PTR Meta_Convert(WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;
	char *group = 0;

	// Get some information about the selected contact.
	if (!db_get_utf(wParam, "CList", "Group", &dbv)) {
		group = _strdup(dbv.pszVal);
		db_free(&dbv);
	}

	// Create a new metacontact
	MCONTACT hMetaContact = (MCONTACT)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (hMetaContact) {
		db_set_dw(hMetaContact, META_PROTO, "NumContacts", 0);

		// Add the MetaContact protocol to the new meta contact
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hMetaContact, (LPARAM)META_PROTO);

		if (group)
			db_set_utf(hMetaContact, "CList", "Group", group);

		// Assign the contact to the MetaContact just created (and make default).
		if (!Meta_Assign(wParam, hMetaContact, TRUE)) {
			MessageBox(0, TranslateT("There was a problem in assigning the contact to the MetaContact"), TranslateT("Error"), MB_ICONEXCLAMATION);
			CallService(MS_DB_CONTACT_DELETE, (WPARAM)hMetaContact, 0);
			return 0;
		}

		// hide the contact if clist groups disabled (shouldn't create one anyway since menus disabled)
		if (!Meta_IsEnabled())
			db_set_b(hMetaContact, "CList", "Hidden", 1);
	}

	free(group);
	return (INT_PTR)hMetaContact;
}

/** Display the <b>'Add to'</b> Dialog
*
* Present a dialog in which the user can choose to which MetaContact this
* contact will be added
*
* @param wParam : HANDLE to the contact that has been chosen.
* @param lParam :	Allways set to 0.
*/

INT_PTR Meta_AddTo(WPARAM wParam, LPARAM lParam)
{
	HWND clui = (HWND)CallService(MS_CLUI_GETHWND, 0, 0);
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_METASELECT), clui, &Meta_SelectDialogProc, (LPARAM)wParam);
	return 0;
}

/** Display the <b>'Edit'</b> Dialog
*
* Present a dialog in which the user can edit some properties of the MetaContact.
*
* @param wParam : HANDLE to the MetaContact to be edited.
* @param lParam :	Allways set to 0.
*/

INT_PTR Meta_Edit(WPARAM wParam, LPARAM lParam)
{
	HWND clui = (HWND)CallService(MS_CLUI_GETHWND, 0, 0);
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_METAEDIT), clui, Meta_EditDialogProc, (LPARAM)wParam);
	return 0;
}

void Meta_RemoveContactNumber(DBCachedContact *cc, int number)
{
	if (cc == NULL)
		return;

	if (number < 0 && number >= cc->nSubs)
		return;

	// get the handle
	MCONTACT hContact = Meta_GetContactHandle(cc, number);
	int default_contact = db_get_dw(cc->contactID, META_PROTO, "Default", -1);

	// make sure this contact thinks it's part of this metacontact
	if (hContact == cc->contactID) {
		// stop ignoring, if we were
		if (options.suppress_status)
			CallService(MS_IGNORE_UNIGNORE, hContact, IGNOREEVENT_USERONLINE);
	}

	// each contact from 'number' upwards will be moved down one
	// and the last one will be deleted
	for (int i = number + 1; i < cc->nSubs; i++)
		Meta_SwapContacts(cc, i, i - 1);

	// remove the last one
	char buffer[512], idStr[20];
	_itoa(cc->nSubs - 1, idStr, 10);
	strcpy(buffer, "Protocol"); strcat(buffer, idStr);
	db_unset(cc->contactID, META_PROTO, buffer);

	strcpy(buffer, "Status"); strcat(buffer, idStr);
	db_unset(cc->contactID, META_PROTO, buffer);

	strcpy(buffer, "StatusString"); strcat(buffer, idStr);
	db_unset(cc->contactID, META_PROTO, buffer);

	strcpy(buffer, "Login"); strcat(buffer, idStr);
	db_unset(cc->contactID, META_PROTO, buffer);

	strcpy(buffer, "Nick"); strcat(buffer, idStr);
	db_unset(cc->contactID, META_PROTO, buffer);

	strcpy(buffer, "CListName"); strcat(buffer, idStr);
	db_unset(cc->contactID, META_PROTO, buffer);

	// if the default contact was equal to or greater than 'number', decrement it (and deal with ends)
	if (default_contact >= number) {
		default_contact--;
		if (default_contact < 0)
			default_contact = 0;

		db_set_dw(cc->contactID, META_PROTO, "Default", default_contact);
		NotifyEventHooks(hEventDefaultChanged, cc->contactID, Meta_GetContactHandle(cc, default_contact));
	}
	cc->nSubs--;
	db_set_dw(cc->contactID, META_PROTO, "NumContacts", cc->nSubs);

	// fix nick
	hContact = Meta_GetMostOnline(cc);
	Meta_CopyContactNick(cc, hContact);

	// fix status
	Meta_FixStatus(cc);

	// fix avatar
	hContact = Meta_GetMostOnlineSupporting(cc, PFLAGNUM_4, PF4_AVATARS);
	if (hContact) {
		PROTO_AVATAR_INFORMATIONT AI = { sizeof(AI) };
		AI.hContact = cc->contactID;
		AI.format = PA_FORMAT_UNKNOWN;
		_tcscpy(AI.filename, _T("X"));

		if ((int)CallProtoService(META_PROTO, PS_GETAVATARINFOT, 0, (LPARAM)&AI) == GAIR_SUCCESS)
			db_set_ts(cc->contactID, "ContactPhoto", "File", AI.filename);
	}
}

/** Delete a MetaContact from the database
*
* Delete a MetaContact and remove all the information
* concerning this MetaContact in the contact linked to it.
*
* @param wParam : HANDLE to the MetaContact to be deleted, or to the subcontact to be removed from the MetaContact
* @param lParam : BOOL flag indicating whether to ask 'are you sure' when deleting a MetaContact
*/

INT_PTR Meta_Delete(WPARAM hContact, LPARAM bSkipQuestion)
{
	DBCachedContact *cc = currDb->m_cache->GetCachedContact(hContact);
	if (cc == NULL)
		return 0;

	// The wParam is a metacontact
	if (cc->nSubs != -1) {
		// check from recursion - see second half of this function
		if (!bSkipQuestion && IDYES != 
			 MessageBox((HWND)CallService(MS_CLUI_GETHWND, 0, 0),
				TranslateT("This will remove the MetaContact permanently.\n\nProceed Anyway?"),
				TranslateT("Are you sure?"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2))
			return 0;

		for (int i = 0; i < cc->nSubs; i++) {
			currDb->MetaDetouchSub(cc, i);

			// stop ignoring, if we were
			if (options.suppress_status)
				CallService(MS_IGNORE_UNIGNORE, hContact, IGNOREEVENT_USERONLINE);
		}

		NotifyEventHooks(hSubcontactsChanged, hContact, 0);
		CallService(MS_DB_CONTACT_DELETE, hContact, 0);
	}
	else {
		if (cc->nSubs == 1) {
			if (IDYES == MessageBox(0, TranslateT(szDelMsg), TranslateT("Delete MetaContact?"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1))
				Meta_Delete(hContact, 1);

			return 0;
		}

		Meta_RemoveContactNumber(cc, Meta_GetContactNumber(cc, hContact));
	}
	return 0;
}

/** Set contact as MetaContact default
*
* Set the given contact to be the default one for the metacontact to which it is linked.
*
* @param wParam : 	HANDLE to the MetaContact to be set as default
* @param lParam :	HWND to the clist window
(This means the function has been called via the contact menu).
*/
INT_PTR Meta_Default(WPARAM hMeta, LPARAM wParam)
{
	DBCachedContact *cc = CheckMeta(hMeta);
	if (cc) {
		db_set_dw(hMeta, META_PROTO, "Default", Meta_GetContactNumber(cc, wParam));
		NotifyEventHooks(hEventDefaultChanged, hMeta, wParam);
	}
	return 0;
}

/** Set/unset (i.e. toggle) contact to force use of default contact
*
* Set the given contact to be the default one for the metacontact to which it is linked.
*
* @param wParam : HANDLE to the MetaContact to be set as default
* @param lParam :HWND to the clist window
(This means the function has been called via the contact menu).
*/

INT_PTR Meta_ForceDefault(WPARAM hMeta, LPARAM)
{
	// the wParam is a MetaContact
	DBCachedContact *cc = CheckMeta(hMeta);
	if (cc) {
		BOOL current = db_get_b(hMeta, META_PROTO, "ForceDefault", 0);
		current = !current;
		db_set_b(hMeta, META_PROTO, "ForceDefault", (BYTE)current);

		db_set_dw(hMeta, META_PROTO, "ForceSend", 0);

		if (current)
			NotifyEventHooks(hEventForceSend, hMeta, Meta_GetContactHandle(cc, db_get_dw(hMeta, META_PROTO, "Default", -1)));
		else
			NotifyEventHooks(hEventUnforceSend, hMeta, 0);
	}
	return 0;
}

/** Called when the context-menu of a contact is about to be displayed
*
* This will test which of the 4 menu item should be displayed, depending
* on which contact triggered the event
*
* @param wParam : HANDLE to the contact that triggered the event
* @param lParam :	Always set to 0;
*/

int Meta_ModifyMenu(WPARAM hMeta, LPARAM lParam)
{
	DBCachedContact *cc = currDb->m_cache->GetCachedContact(hMeta);
	if (cc == NULL)
		return 0;
		
	CLISTMENUITEM mi = { sizeof(mi) };
	Menu_ShowItem(hMenuRoot, false);

	if (IsMeta(cc)) {
		// save the mouse pos in case they open a subcontact menu
		GetCursorPos(&menuMousePoint);

		// This is a MetaContact, show the edit, force default, and the delete menu, and hide the others
		Menu_ShowItem(hMenuEdit, true);
		Menu_ShowItem(hMenuAdd, false);
		Menu_ShowItem(hMenuConvert, false);
		Menu_ShowItem(hMenuDefault, false);
		Menu_ShowItem(hMenuDelete, false);

		mi.flags = CMIM_NAME;
		mi.pszName = LPGEN("Remove from metacontact");
		Menu_ModifyItem(hMenuDelete, &mi);

		//show subcontact menu items
		for (int i = 0; i < MAX_CONTACTS; i++) {
			if (i >= cc->nSubs) {
				Menu_ShowItem(hMenuContact[i], false);
				continue;
			}

			MCONTACT hContact = Meta_GetContactHandle(cc, i);
			char *szProto = GetContactProto(hContact);

			if (options.menu_contact_label == DNT_UID) {
				char buf[512], idStr[512];
				strcpy(buf, "Login");
				strcat(buf, _itoa(i, idStr, 10));

				DBVARIANT dbv;
				db_get(hMeta, META_PROTO, buf, &dbv);
				switch (dbv.type) {
				case DBVT_ASCIIZ:
					mir_snprintf(buf, 512, "%s", dbv.pszVal);
					break;
				case DBVT_BYTE:
					mir_snprintf(buf, 512, "%d", dbv.bVal);
					break;
				case DBVT_WORD:
					mir_snprintf(buf, 512, "%d", dbv.wVal);
					break;
				case DBVT_DWORD:
					mir_snprintf(buf, 512, "%d", dbv.dVal);
					break;
				default:
					buf[0] = 0;
				}
				db_free(&dbv);
				mi.pszName = buf;
				mi.flags = 0;
			}
			else {
				mi.ptszName = cli.pfnGetContactDisplayName(hContact, 0);
				mi.flags = CMIF_TCHAR;
			}

			mi.flags |= CMIM_FLAGS | CMIM_NAME | CMIM_ICON;

			int iconIndex = CallService(MS_CLIST_GETCONTACTICON, hContact, 0);
			mi.hIcon = ImageList_GetIcon((HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST, 0, 0), iconIndex, 0);

			Menu_ModifyItem(hMenuContact[i], &mi);
			DestroyIcon(mi.hIcon);
			
			Menu_ShowItem(hMenuRoot, true);
		}

		// show hide nudge menu item
		char serviceFunc[256];
		mir_snprintf(serviceFunc, 256, "%s%s", GetContactProto(Meta_GetMostOnline(cc)), PS_SEND_NUDGE);
		CallService(MS_NUDGE_SHOWMENU, (WPARAM)META_PROTO, ServiceExists(serviceFunc));
		return 0;
	}

	if (!Meta_IsEnabled()) {
		// groups disabled - all meta menu options hidden
		Menu_ShowItem(hMenuDefault, false);
		Menu_ShowItem(hMenuDelete, false);
		Menu_ShowItem(hMenuAdd, false);
		Menu_ShowItem(hMenuConvert, false);
		Menu_ShowItem(hMenuEdit, false);
		return 0;
	}
	
	// the contact is affected to a metacontact
	if (IsSub(cc)) {
		Menu_ShowItem(hMenuDefault, true);

		mi.flags = CMIM_NAME;
		mi.pszName = LPGEN("Remove from metacontact");
		Menu_ModifyItem(hMenuDelete, &mi);

		Menu_ShowItem(hMenuAdd, false);
		Menu_ShowItem(hMenuConvert, false);
		Menu_ShowItem(hMenuEdit, false);
	}
	else {
		// The contact is neutral
		Menu_ShowItem(hMenuAdd, true);
		Menu_ShowItem(hMenuConvert, true);
		Menu_ShowItem(hMenuEdit, false);
		Menu_ShowItem(hMenuDelete, false);
		Menu_ShowItem(hMenuDefault, false);
	}

	for (int i = 0; i < MAX_CONTACTS; i++)
		Menu_ShowItem(hMenuContact[i], false);

	return 0;
}

INT_PTR Meta_OnOff(WPARAM wParam, LPARAM lParam)
{
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.flags = CMIM_NAME | CMIM_ICON;
	// just write to db - the rest is handled in the Meta_SettingChanged function
	if (db_get_b(0, META_PROTO, "Enabled", 1)) {
		db_set_b(0, META_PROTO, "Enabled", 0);
		// modify main mi item
		mi.icolibItem = GetIconHandle(I_MENU);
		mi.pszName = LPGEN("Toggle MetaContacts On");
	}
	else {
		db_set_b(0, META_PROTO, "Enabled", 1);
		// modify main mi item
		mi.icolibItem = GetIconHandle(I_MENUOFF);
		mi.pszName = LPGEN("Toggle MetaContacts Off");
	}
	Menu_ModifyItem(hMenuOnOff, &mi);
	return 0;
}

void InitMenus()
{
	CLISTMENUITEM mi = { sizeof(mi) };

	// main menu item
	mi.icolibItem = GetIconHandle(I_MENUOFF);
	mi.pszName = LPGEN("Toggle MetaContacts Off");
	mi.pszService = "MetaContacts/OnOff";
	mi.position = 500010000;
	hMenuOnOff = Menu_AddMainMenuItem(&mi);

	// contact menu items
	mi.icolibItem = GetIconHandle(I_CONVERT);
	mi.position = -200010;
	mi.pszName = LPGEN("Convert to MetaContact");
	mi.pszService = "MetaContacts/Convert";
	hMenuConvert = Menu_AddContactMenuItem(&mi);

	mi.icolibItem = GetIconHandle(I_ADD);
	mi.position = -200009;
	mi.pszName = LPGEN("Add to existing MetaContact...");
	mi.pszService = "MetaContacts/AddTo";
	hMenuAdd = Menu_AddContactMenuItem(&mi);

	mi.icolibItem = GetIconHandle(I_EDIT);
	mi.position = -200010;
	mi.pszName = LPGEN("Edit MetaContact...");
	mi.pszService = "MetaContacts/Edit";
	hMenuEdit = Menu_AddContactMenuItem(&mi);

	mi.icolibItem = GetIconHandle(I_SETDEFAULT);
	mi.position = -200009;
	mi.pszName = LPGEN("Set as MetaContact default");
	mi.pszService = "MetaContacts/Default";
	hMenuDefault = Menu_AddContactMenuItem(&mi);

	mi.icolibItem = GetIconHandle(I_REMOVE);
	mi.position = -200008;
	mi.pszName = LPGEN("Delete MetaContact");
	mi.pszService = "MetaContacts/Delete";
	hMenuDelete = Menu_AddContactMenuItem(&mi);

	mi.position = -99000;
	mi.flags = CMIF_HIDDEN | CMIF_ROOTPOPUP;
	mi.icolibItem = 0;
	mi.pszName = LPGEN("Subcontacts");
	hMenuRoot = Menu_AddContactMenuItem(&mi);

	char buffer[512], buffer2[512];

	mi.flags = CMIF_HIDDEN | CMIF_CHILDPOPUP;
	mi.hParentMenu = hMenuRoot;
	for (int i = 0; i < MAX_CONTACTS; i++) {
		mi.position--;
		mi.pszName = "";

		strcpy(buffer, "MetaContacts/MenuFunc");
		strcat(buffer, _itoa(i, buffer2, 10));
		mi.pszService = buffer;

		hMenuContact[i] = Menu_AddContactMenuItem(&mi);
	}

	// loop and copy data from subcontacts
	if (options.copydata) {
		for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
			DBCachedContact *cc = CheckMeta(hContact);
			if (cc != NULL)
				Meta_CopyData(cc);
		}
	}

	Meta_HideLinkedContacts();

	if (!Meta_IsEnabled()) {
		// modify main menu item
		mi.flags = CMIM_NAME | CMIM_ICON;
		mi.icolibItem = GetIconHandle(I_MENU);
		mi.pszName = LPGEN("Toggle MetaContacts On");
		Menu_ModifyItem(hMenuOnOff, &mi);

		Meta_HideMetaContacts(TRUE);
	}
	else Meta_SuppressStatus(options.suppress_status);
}
