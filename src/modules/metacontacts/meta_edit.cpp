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

// Holds the differents changes that have to made
struct
{
	MCONTACT hMeta;                          // HANDLE of the MetaContact that is edited.
	DBCachedContact *cc;
	MCONTACT hDefaultContact;                // HANDLE of the new default contact
	MCONTACT hOfflineContact;
	int num_deleted,                         // DWORD number of deleted contacts
		 num_contacts;                        // DWORD number of contacts
	MCONTACT hDeletedContacts[MAX_CONTACTS]; // HANDLEs of the subcontacts to be removed from this metacontact
	MCONTACT hContact[MAX_CONTACTS];         // HANDLEs of the subcontacts, in the order they should be in
	int force_default;
}
static g_data; // global CHANGES structure

/////////////////////////////////////////////////////////////////////////////////////////

static void FillContactList(HWND hWndDlg)
{
	HWND hList = GetDlgItem(hWndDlg, IDC_LST_CONTACTS);
	TCHAR buff[256];

	SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);

	LVITEM LvItem = { 0 };
	LvItem.mask = LVIF_TEXT;   // Text Style
	LvItem.cchTextMax = 256; // Max size of test

	for (int i = 0; i < g_data.num_contacts; i++) {
		LvItem.iItem = i;

		TCHAR *ptszCDN = cli.pfnGetContactDisplayName(g_data.hContact[i], 0);
		if (ptszCDN == NULL)
			ptszCDN = TranslateT("(Unknown Contact)");

		LvItem.iSubItem = 0; // clist display name
		LvItem.pszText = ptszCDN;
		ListView_InsertItem(hList, &LvItem);

		LvItem.iSubItem = 1; // id
		char *szProto = GetContactProto(g_data.hContact[i]);
		if (szProto) {
			PROTOACCOUNT *pa = ProtoGetAccount(szProto);

			char *szField = (char *)CallProtoService(szProto, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);

			DBVARIANT dbv;
			if (!db_get(g_data.hContact[i], szProto, szField, &dbv)) {
				switch (dbv.type) {
				case DBVT_ASCIIZ:
					_tcsncpy(buff, _A2T(dbv.pszVal), SIZEOF(buff));
					break;
				case DBVT_WCHAR:
					_tcsncpy(buff, dbv.ptszVal, SIZEOF(buff));
					break;
				case DBVT_BYTE:
					_itot(dbv.bVal, buff, 10);
					break;
				case DBVT_WORD:
					_itot(dbv.wVal, buff, 10);
					break;
				case DBVT_DWORD:
					_itot(dbv.dVal, buff, 10);
					break;
				default:
					buff[0] = 0;
				}
				db_free(&dbv);
			}
			else buff[0] = 0;

			LvItem.pszText = buff;
			SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem); // Enter text to SubItems

			LvItem.iSubItem = 2; // protocol
			_tcsncpy_s(buff, SIZEOF(buff), (pa == NULL) ? _A2T(szProto) : pa->tszAccountName, _TRUNCATE);
			ListView_SetItem(hList, &LvItem);
		}
		else {
			LvItem.pszText = TranslateT("Unknown");
			ListView_SetItem(hList, &LvItem);

			LvItem.iSubItem = 2; // protocol
			ListView_SetItem(hList, &LvItem);
		}
		LvItem.iSubItem = 3; // Default (Yes/No)
		LvItem.pszText = (g_data.hContact[i] == g_data.hDefaultContact ? TranslateT("Yes") : TranslateT("No"));
		ListView_SetItem(hList, &LvItem);

		LvItem.iSubItem = 4; // Offline (Yes/No)
		LvItem.pszText = (g_data.hContact[i] == g_data.hOfflineContact ? TranslateT("Yes") : TranslateT("No"));
		ListView_SetItem(hList, &LvItem);
	}
}

static void SetListSelection(HWND hList, int sel)
{
	ListView_SetItemState(hList, sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

/** Scans the \c CHANGES and call the appropriate function for each change.
*
* @param chg : Structure holding all the change info (See CHANGES).
*/

static void ApplyChanges()
{
	// remove removed contacts
	for (int i = 0; i < g_data.num_deleted; i++) {
		Meta_Delete(g_data.hDeletedContacts[i], 0);
		if (g_data.hDeletedContacts[i] == g_data.hDefaultContact)
			g_data.hDefaultContact = 0;
		if (g_data.hDeletedContacts[i] == g_data.hOfflineContact)
			g_data.hOfflineContact = 0;
	}

	// set contact positions
	for (int i = 0; i < g_data.num_contacts; i++)
		if (Meta_GetContactNumber(g_data.cc, g_data.hContact[i]) != i)
			Meta_SwapContacts(g_data.cc, Meta_GetContactNumber(g_data.cc, g_data.hContact[i]), i);

	NotifyEventHooks(hSubcontactsChanged, g_data.hMeta, g_data.hDefaultContact);

	// set default
	db_mc_setDefaultNum(g_data.hMeta, (g_data.hDefaultContact) ? Meta_GetContactNumber(g_data.cc, g_data.hDefaultContact) : 0);

	// set offline
	if (g_data.hOfflineContact)
		db_set_dw(g_data.hMeta, META_PROTO, "OfflineSend", Meta_GetContactNumber(g_data.cc, g_data.hOfflineContact));
	else
		db_set_dw(g_data.hMeta, META_PROTO, "OfflineSend", INVALID_CONTACT_ID);

	// fix nick
	MCONTACT most_online = Meta_GetMostOnline(g_data.cc);
	Meta_CopyContactNick(g_data.cc, most_online);

	// fix status
	Meta_FixStatus(g_data.cc);

	// fix avatar
	most_online = Meta_GetMostOnlineSupporting(g_data.cc, PFLAGNUM_4, PF4_AVATARS);
	if (most_online) {
		PROTO_AVATAR_INFORMATIONT AI = { sizeof(AI) };
		AI.hContact = g_data.hMeta;
		AI.format = PA_FORMAT_UNKNOWN;
		_tcscpy(AI.filename, _T("X"));
		if (CallProtoService(META_PROTO, PS_GETAVATARINFOT, 0, (LPARAM)&AI) == GAIR_SUCCESS)
			db_set_ts(g_data.hMeta, "ContactPhoto", "File", AI.filename);
	}

	if (MetaAPI_GetForceState(g_data.hMeta, 0) != g_data.force_default)
		MetaAPI_ForceDefault(g_data.hMeta, 0);
}

LRESULT ProcessCustomDraw(LPARAM lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	switch (lplvcd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT: //Before the paint cycle begins
		//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: //Before an item is drawn
		if (g_data.hContact[(int)lplvcd->nmcd.dwItemSpec] == g_data.hDefaultContact)
			lplvcd->clrText = RGB(255, 0, 0);

		return CDRF_NEWFONT;
	}

	return 0;
}

/** Callback function for the <b>'Edit'</b> Dialog.
*
* All the UI is controlled here, from display to functionnalities.
*
* @param hwndDlg : HANDLE to the <b>'Edit'</b> Dialog.
* @param uMsg : Specifies the message received by this dialog.
* @param wParam : Specifies additional message-specific information. 
* @param lParam : Specifies additional message-specific information (handle of MetaContact to edit)
*
* @return TRUE if the dialog processed the message, FALSE if it did not.
*/

#define WMU_SETTITLE		(WM_USER + 1)

static INT_PTR CALLBACK Meta_EditDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd;
	int sel, i;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx(I_EDIT));
		{
			DBCachedContact *cc = currDb->m_cache->GetCachedContact(lParam);
			if (cc == NULL) {
				DestroyWindow(hwndDlg);
				return FALSE;
			}

			// Disable the 'Apply' button.
			EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), FALSE);

			hwnd = GetDlgItem(hwndDlg, IDC_LST_CONTACTS);
			ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

			// Create list columns
			LVCOLUMN LvCol = { 0 };
			LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

			LvCol.pszText = TranslateT("Contact");
			LvCol.cx = 150;
			ListView_InsertColumn(hwnd, 0, &LvCol);

			LvCol.pszText = TranslateT("ID");
			LvCol.cx = 130;
			ListView_InsertColumn(hwnd, 1, &LvCol);

			LvCol.pszText = TranslateT("Protocol");
			LvCol.cx = 100;
			ListView_InsertColumn(hwnd, 2, &LvCol);

			LvCol.pszText = TranslateT("Default");
			LvCol.cx = 60;
			ListView_InsertColumn(hwnd, 3, &LvCol);

			LvCol.pszText = TranslateT("Send Offline");
			LvCol.cx = 85;
			ListView_InsertColumn(hwnd, 4, &LvCol);

			// disable buttons until a selection is made in the list
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_REM), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETDEFAULT), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETOFFLINE), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UP), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_DOWN), FALSE);

			int offline_contact_number = db_get_dw(lParam, META_PROTO, "OfflineSend", INVALID_CONTACT_ID);

			ZeroMemory(&g_data, sizeof(g_data));
			g_data.cc = cc;
			g_data.hMeta = lParam;
			g_data.num_contacts = cc->nSubs;
			g_data.num_deleted = 0;
			g_data.hDefaultContact = Meta_GetContactHandle(g_data.cc, cc->nDefault);
			g_data.hOfflineContact = Meta_GetContactHandle(g_data.cc, offline_contact_number);
			for (i = 0; i < cc->nSubs; i++)
				g_data.hContact[i] = Meta_GetContactHandle(g_data.cc, i);
			g_data.force_default = MetaAPI_GetForceState(lParam, 0);

			SendMessage(hwndDlg, WMU_SETTITLE, 0, lParam);

			CheckDlgButton(hwndDlg, IDC_CHK_FORCEDEFAULT, g_data.force_default);

			FillContactList(hwndDlg);
		}
		return TRUE;

	case WMU_SETTITLE:
		{
			TCHAR *ptszCDN = cli.pfnGetContactDisplayName(lParam, 0);
			if (ptszCDN == NULL)
				ptszCDN = TranslateT("(Unknown Contact)");

			SetWindowText(GetDlgItem(hwndDlg, IDC_ED_NAME), ptszCDN);
		}
		return TRUE;

	case WM_NOTIFY: // the message that is being sent always
		switch (LOWORD(wParam)) { // hit control
		case IDC_LST_CONTACTS:      // did we hit our ListView contorl?
			if (((LPNMHDR)lParam)->code == NM_CLICK) {
				sel = ListView_GetNextItem(GetDlgItem(hwndDlg, IDC_LST_CONTACTS), -1, LVNI_FOCUSED | LVNI_SELECTED); // return item selected

				// enable buttons
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_REM), sel != -1);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETDEFAULT), sel != -1 && g_data.hContact[sel] != g_data.hDefaultContact);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETOFFLINE), sel != -1 && g_data.hContact[sel] != g_data.hOfflineContact);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UP), sel > 0);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_DOWN), (sel != -1 && sel < g_data.num_contacts - 1));
			}
		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:	// A button ('Remove', 'OK', 'Cancel' or 'Apply', normally) has been clicked
			switch (LOWORD(wParam)) {
			case IDC_VALIDATE: // Apply changes, if there is still one contact attached to the metacontact.
				if (g_data.num_contacts == 0) { // Otherwise, delete the metacontact.
					if (IDYES == MessageBox(hwndDlg, TranslateT(szDelMsg), TranslateT("Delete MetaContact?"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1)) {
						Meta_Delete(g_data.hMeta, 0);
						DestroyWindow(hwndDlg);
					}
					return TRUE;
				}
				ApplyChanges();

				// Disable the 'Apply' button.
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), FALSE);
				break;

			case IDOK:
				if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_VALIDATE))) { // If there are changes that could be made,
					if (g_data.num_contacts == 0) { // do the work that would have be done if the 'Apply' button was clicked.
						if (IDYES == MessageBox(hwndDlg, TranslateT(szDelMsg), TranslateT("Delete MetaContact?"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1)) {
							Meta_Delete(g_data.hMeta, 0);
							DestroyWindow(hwndDlg);
						}
						return TRUE;
					}
					ApplyChanges();
				}
				EndDialog(hwndDlg, IDOK);
				return TRUE;

			case IDCANCEL:	// Simply close the dialog
				EndDialog(hwndDlg, IDCANCEL);
				return TRUE;

			case IDC_BTN_SETDEFAULT:
				hwnd = GetDlgItem(hwndDlg, IDC_LST_CONTACTS);
				sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				InvalidateRect(hwnd, 0, TRUE);
				g_data.hDefaultContact = g_data.hContact[sel];
				SendMessage(hwndDlg, WMU_SETTITLE, 0, (LPARAM)g_data.hContact[sel]);

				FillContactList(hwndDlg);
				SetListSelection(hwnd, sel);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETDEFAULT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), TRUE);
				return TRUE;

			case IDC_BTN_SETOFFLINE:
				hwnd = GetDlgItem(hwndDlg, IDC_LST_CONTACTS);
				sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				InvalidateRect(hwnd, 0, TRUE);
				g_data.hOfflineContact = g_data.hContact[sel];

				FillContactList(hwndDlg);
				SetListSelection(hwnd, sel);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETOFFLINE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), TRUE);
				return TRUE;

			case IDC_BTN_REM:
				hwnd = GetDlgItem(hwndDlg, IDC_LST_CONTACTS);
				sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				g_data.num_contacts--;
				g_data.hDeletedContacts[g_data.num_deleted++] = g_data.hContact[sel];
				if (g_data.hDefaultContact == g_data.hContact[sel]) {
					if (g_data.num_contacts > 0) {
						g_data.hDefaultContact = g_data.hContact[0];
						SetWindowText(GetDlgItem(hwndDlg, IDC_ED_DEFAULT), cli.pfnGetContactDisplayName(g_data.hDefaultContact, 0));
					}
					else {
						g_data.hDefaultContact = 0;
						SetWindowText(GetDlgItem(hwndDlg, IDC_ED_DEFAULT), _T("None"));
					}
				}

				for (i = sel; i < g_data.num_contacts; i++)
					g_data.hContact[i] = g_data.hContact[i + 1];
				FillContactList(hwndDlg);

				// disable buttons
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_REM), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETDEFAULT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_SETOFFLINE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UP), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_DOWN), FALSE);

				// Enable the 'Apply' button.
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), TRUE);
				return TRUE;

			case IDC_BTN_UP:
				hwnd = GetDlgItem(hwndDlg, IDC_LST_CONTACTS);
				sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				{
					MCONTACT temp = g_data.hContact[sel];
					g_data.hContact[sel] = g_data.hContact[sel - 1];
					g_data.hContact[sel - 1] = temp;
				}
				FillContactList(hwndDlg);
				sel--;
				SetListSelection(hwnd, sel);

				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UP), (sel > 0));
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_DOWN), (sel < g_data.num_contacts - 1));
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), TRUE);
				return TRUE;

			case IDC_BTN_DOWN:
				hwnd = GetDlgItem(hwndDlg, IDC_LST_CONTACTS);
				sel = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
				{
					MCONTACT temp = g_data.hContact[sel];
					g_data.hContact[sel] = g_data.hContact[sel + 1];
					g_data.hContact[sel + 1] = temp;
				}
				FillContactList(hwndDlg);
				sel++;
				SetListSelection(hwnd, sel);

				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_UP), (sel > 0));
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_DOWN), (sel < g_data.num_contacts - 1));
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), TRUE);
				return TRUE;

			case IDC_CHK_FORCEDEFAULT:
				g_data.force_default = IsDlgButtonChecked(hwndDlg, IDC_CHK_FORCEDEFAULT);
				EnableWindow(GetDlgItem(hwndDlg, IDC_VALIDATE), TRUE);
				return TRUE;
			}
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		return TRUE;

	case WM_DESTROY:
		Skin_ReleaseIcon((HICON)SendMessage(hwndDlg, WM_SETICON, ICON_BIG, 0));
		EndDialog(hwndDlg, IDCANCEL);
		break;
	}

	return FALSE;
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
