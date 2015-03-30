#include "common.h"

CToxOptionsMain::CToxOptionsMain(CToxProto *proto)
	: CSuper(proto, IDD_OPTIONS_MAIN, NULL, false),
	m_toxAddress(this, IDC_TOXID), m_toxAddressCopy(this, IDC_CLIPBOARD),
	m_profileCreate(this, IDC_PROFILE_NEW), m_profileImport(this, IDC_PROFILE_IMPORT),
	m_profileExport(this, IDC_PROFILE_EXPORT), m_nickname(this, IDC_NAME),
	m_password(this, IDC_PASSWORD), m_group(this, IDC_GROUP),
	m_enableUdp(this, IDC_ENABLE_UDP), m_enableIPv6(this, IDC_ENABLE_IPV6)
{
	CreateLink(m_toxAddress, TOX_SETTINGS_ID, _T(""));
	CreateLink(m_nickname, "Nick", _T(""));
	CreateLink(m_password, "Password", _T(""));
	CreateLink(m_group, TOX_SETTINGS_GROUP, _T(""));
	CreateLink(m_enableUdp, "EnableUDP", DBVT_BYTE, TRUE);
	CreateLink(m_enableIPv6, "EnableIPv6", DBVT_BYTE, FALSE);

	m_toxAddressCopy.OnClick = Callback(this, &CToxOptionsMain::ToxAddressCopy_OnClick);
	m_profileCreate.OnClick = Callback(this, &CToxOptionsMain::ProfileCreate_OnClick);
	m_profileImport.OnClick = Callback(this, &CToxOptionsMain::ProfileImport_OnClick);
	m_profileExport.OnClick = Callback(this, &CToxOptionsMain::ProfileExport_OnClick);
}

CToxOptionsMain::CToxOptionsMain(CToxProto *proto, HWND hwndParent)
	: CSuper(proto, IDD_ACCOUNT_MANAGER, hwndParent, false),
	m_toxAddress(this, IDC_TOXID), m_toxAddressCopy(this, IDC_CLIPBOARD),
	m_profileCreate(this, IDC_PROFILE_NEW), m_profileImport(this, IDC_PROFILE_IMPORT),
	m_profileExport(this, IDC_PROFILE_EXPORT), m_nickname(this, IDC_NAME),
	m_password(this, IDC_PASSWORD), m_group(this, IDC_GROUP),
	m_enableUdp(this, IDC_ENABLE_UDP), m_enableIPv6(this, IDC_ENABLE_IPV6)
{
	CreateLink(m_toxAddress, TOX_SETTINGS_ID, _T(""));
	CreateLink(m_nickname, "Nick", _T(""));
	CreateLink(m_password, "Password", _T(""));
	CreateLink(m_group, TOX_SETTINGS_GROUP, _T(""));
	CreateLink(m_enableUdp, "EnableUDP", DBVT_BYTE, TRUE);
	CreateLink(m_enableIPv6, "EnableIPv6", DBVT_BYTE, FALSE);

	m_toxAddressCopy.OnClick = Callback(this, &CToxOptionsMain::ToxAddressCopy_OnClick);
	m_profileCreate.OnClick = Callback(this, &CToxOptionsMain::ProfileCreate_OnClick);
	m_profileImport.OnClick = Callback(this, &CToxOptionsMain::ProfileImport_OnClick);
	m_profileExport.OnClick = Callback(this, &CToxOptionsMain::ProfileExport_OnClick);
}

void CToxOptionsMain::OnInitDialog()
{
	CSuper::OnInitDialog();

	std::tstring profilePath = m_proto->GetToxProfilePath();
	if (CToxProto::IsFileExists(profilePath))
	{
		m_toxAddress.Enable();

		ShowWindow(m_profileCreate.GetHwnd(), FALSE);
		ShowWindow(m_profileImport.GetHwnd(), FALSE);

		ShowWindow(m_toxAddressCopy.GetHwnd(), TRUE);
		//ShowWindow(m_profileExport.GetHwnd(), TRUE);
	}

	if (m_proto->IsOnline())
	{
		EnableWindow(m_nickname.GetHwnd(), TRUE);
		EnableWindow(m_password.GetHwnd(), TRUE);
	}

	SendMessage(m_toxAddress.GetHwnd(), EM_LIMITTEXT, TOX_ADDRESS_SIZE * 2, 0);
	SendMessage(m_nickname.GetHwnd(), EM_LIMITTEXT, TOX_MAX_NAME_LENGTH, 0);
	SendMessage(m_password.GetHwnd(), EM_LIMITTEXT, 32, 0);
	SendMessage(m_group.GetHwnd(), EM_LIMITTEXT, 64, 0);
}

void CToxOptionsMain::ToxAddressCopy_OnClick(CCtrlButton*)
{
	char *toxAddress = m_toxAddress.GetTextA();
	int toxAddressLength = mir_strlen(toxAddress);
	if (OpenClipboard(m_toxAddress.GetHwnd()))
	{
		EmptyClipboard();
		HGLOBAL hMemory = GlobalAlloc(GMEM_FIXED, toxAddressLength);
		memcpy(GlobalLock(hMemory), toxAddress, toxAddressLength);
		GlobalUnlock(hMemory);
		SetClipboardData(CF_TEXT, hMemory);
		CloseClipboard();
	}
}

void CToxOptionsMain::ProfileCreate_OnClick(CCtrlButton*)
{
	if (m_proto->InitToxCore())
	{
		TCHAR *group = m_group.GetText();
		if (mir_tstrlen(group) > 0 && Clist_GroupExists(group))
			Clist_CreateGroup(0, group);

		m_proto->LoadFriendList(NULL);
		m_proto->UninitToxCore();

		m_toxAddress.Enable();
		m_toxAddress.SetTextA(ptrA(m_proto->getStringA(TOX_SETTINGS_ID)));

		m_nickname.SetText(ptrT(m_proto->getTStringA("Nick")));
		m_password.SetText(ptrT(m_proto->getTStringA("Password")));
		m_group.SetText(ptrT(m_proto->getTStringA(TOX_SETTINGS_GROUP)));

		ShowWindow(m_profileCreate.GetHwnd(), FALSE);
		ShowWindow(m_profileImport.GetHwnd(), FALSE);

		ShowWindow(m_toxAddressCopy.GetHwnd(), TRUE);
		//ShowWindow(m_profileExport.GetHwnd(), TRUE);
	}
}

void CToxOptionsMain::ProfileImport_OnClick(CCtrlButton*)
{
	TCHAR filter[MAX_PATH];
	mir_sntprintf(filter, SIZEOF(filter), _T("%s(*.tox)%c*.tox%c%s(*.*)%c*.*%c%c"),
		TranslateT("Tox profile"), 0, 0, TranslateT("All files"), 0, 0, 0);

	TCHAR profilePath[MAX_PATH] = { 0 };

	OPENFILENAME ofn = { sizeof(ofn) };
	ofn.hwndOwner = m_hwnd;
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = profilePath;
	ofn.lpstrTitle = TranslateT("Select tox profile");
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	ofn.lpstrInitialDir = _T("%APPDATA%\\Tox");

	if (!GetOpenFileName(&ofn))
	{
		return;
	}

	std::tstring defaultProfilePath = m_proto->GetToxProfilePath();
	if (_tcsicmp(profilePath, defaultProfilePath.c_str()) != 0)
	{
		CopyFile(profilePath, defaultProfilePath.c_str(), FALSE);
	}

	m_profileCreate.OnClick(&m_profileCreate);
}

void CToxOptionsMain::ProfileExport_OnClick(CCtrlButton*)
{
}

void CToxOptionsMain::OnApply()
{
	TCHAR *group = m_group.GetText();
	if (mir_tstrlen(group) > 0 && Clist_GroupExists(group))
		Clist_CreateGroup(0, group);

	if (m_proto->IsOnline())
	{
		CallProtoService(m_proto->m_szModuleName, PS_SETMYNICKNAME, SMNN_TCHAR, (LPARAM)m_nickname.GetText());

		if (m_proto->password != NULL)
			mir_free(m_proto->password);
		m_proto->password = mir_utf8encodeW(m_password.GetText());

		m_proto->SaveToxProfile();
	}
}

int AddItemToListView(HWND hwndList, UINT mask, int iGroupId, int iItem, int iSubItem, char *pszText, int iImage = -1)
{
	LVITEMA lvi = { 0 };
	lvi.mask = mask;
	lvi.iItem = iItem;
	lvi.iSubItem = iSubItem;
	lvi.iGroupId = iGroupId;
	lvi.iImage = iImage;
	lvi.pszText = mir_strdup(pszText);
	return SendMessage(hwndList, LVM_INSERTITEMA, 0, (LPARAM)&lvi);
}

int SetSubItemToListView(HWND hwndList, UINT mask, int iGroupId, int iItem, int iSubItem, char *pszText, int iImage = -1)
{
	LVITEMA lvi = { 0 };
	lvi.mask = mask;
	lvi.iItem = iItem;
	lvi.iSubItem = iSubItem;
	lvi.iGroupId = iGroupId;
	lvi.iImage = iImage;
	lvi.pszText = mir_strdup(pszText);
	return SendMessage(hwndList, LVM_SETITEMA, 0, (LPARAM)&lvi);
}

INT_PTR CALLBACK EditNodeDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ItemInfo *itemInfo = (ItemInfo*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			itemInfo = (ItemInfo*)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

			SendDlgItemMessage(hwndDlg, IDC_IPV4, EM_SETLIMITTEXT, MAX_PATH, 0);
			SendDlgItemMessage(hwndDlg, IDC_IPV6, EM_SETLIMITTEXT, 39, 0);
			SendDlgItemMessage(hwndDlg, IDC_PORT, EM_SETLIMITTEXT, 5, 0);
			SendDlgItemMessage(hwndDlg, IDC_PKEY, EM_SETLIMITTEXT, TOX_PUBLIC_KEY_SIZE * 2, 0);

			if (itemInfo->iItem == -1)
			{
				SetWindowText(hwndDlg, TranslateT("Add node"));
			}
			else
			{
				SetWindowText(hwndDlg, TranslateT("Change node"));

				LVITEMA lvi = { 0 };
				lvi.mask = LVIF_TEXT;
				lvi.iItem = itemInfo->iItem;
				lvi.cchTextMax = MAX_PATH;
				lvi.pszText = (char*)mir_alloc(MAX_PATH);

				lvi.iSubItem = 0;
				SendMessage(itemInfo->hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				SetDlgItemTextA(hwndDlg, IDC_IPV4, lvi.pszText);

				lvi.iSubItem = 1;
				SendMessage(itemInfo->hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);

				SetDlgItemTextA(hwndDlg, IDC_IPV6, lvi.pszText);

				lvi.iSubItem = 2;
				SendMessage(itemInfo->hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				SetDlgItemTextA(hwndDlg, IDC_PORT, lvi.pszText);

				lvi.iSubItem = 3;
				SendMessage(itemInfo->hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				SetDlgItemTextA(hwndDlg, IDC_PKEY, lvi.pszText);

				mir_free(lvi.pszText);
			}

			Utils_RestoreWindowPositionNoSize(hwndDlg, NULL, MODULE, "EditNodeDlg");
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			char value[MAX_PATH];
			if (!GetDlgItemTextA(hwndDlg, IDC_IPV4, value, SIZEOF(value)))
			{
				MessageBox(hwndDlg, TranslateT("Enter IPv4"), TranslateT("Error"), MB_OK);
				break;
			}
			if (!GetDlgItemTextA(hwndDlg, IDC_PKEY, value, SIZEOF(value)))
			{
				MessageBox(hwndDlg, TranslateT("Enter public key"), TranslateT("Error"), MB_OK);
				break;
			}

			GetDlgItemTextA(hwndDlg, IDC_IPV4, value, SIZEOF(value));
			int iItem = itemInfo->iItem;
			if (iItem == -1)
			{
				iItem = ListView_GetItemCount(itemInfo->hwndList);
				AddItemToListView(itemInfo->hwndList, LVIF_GROUPID | LVIF_TEXT | LVIF_IMAGE, 1, iItem, 0, value);
				ListView_SetItemState(itemInfo->hwndList, iItem, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_EnsureVisible(itemInfo->hwndList, iItem, TRUE);
			}
			else
			{
				SetSubItemToListView(itemInfo->hwndList, LVIF_TEXT, 1, iItem, 0, value);
			}
			GetDlgItemTextA(hwndDlg, IDC_IPV6, value, SIZEOF(value));
			SetSubItemToListView(itemInfo->hwndList, LVIF_TEXT, 1, iItem, 1, value);
			GetDlgItemTextA(hwndDlg, IDC_PORT, value, SIZEOF(value));
			SetSubItemToListView(itemInfo->hwndList, LVIF_TEXT, 1, iItem, 2, value);
			GetDlgItemTextA(hwndDlg, IDC_PKEY, value, SIZEOF(value));
			SetSubItemToListView(itemInfo->hwndList, LVIF_TEXT, 1, iItem, 3, value);
			SetSubItemToListView(itemInfo->hwndList, LVIF_IMAGE, 1, iItem, 4, value, 0);
			SetSubItemToListView(itemInfo->hwndList, LVIF_IMAGE, 1, iItem, 5, value, 1);

			EndDialog(hwndDlg, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		break;

	case WM_DESTROY:
		Utils_SaveWindowPosition(hwndDlg, NULL, MODULE, "EditNodeDlg");
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK NodeListSubProc(HWND hwndList, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LVITEMA lvi = { 0 };

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		TCHAR *userName = (TCHAR*)lParam;

		HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR32, 2, 0);
		HICON icon = LoadSkinnedIcon(SKINICON_OTHER_TYPING);
		ImageList_AddIcon(hImageList, icon); Skin_ReleaseIcon(icon);
		icon = LoadSkinnedIcon(SKINICON_OTHER_DELETE);
		ImageList_AddIcon(hImageList, icon); Skin_ReleaseIcon(icon);
		ListView_SetImageList(hwndList, hImageList, LVSIL_SMALL);

		LVCOLUMN lvc = { 0 };
		lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.iOrder = 0;
		lvc.pszText = L"IPv4";
		lvc.cx = 100;
		ListView_InsertColumn(hwndList, lvc.iOrder, (LPARAM)&lvc);

		lvc.iOrder = 1;
		lvc.pszText = L"IPv6";
		lvc.cx = 100;
		ListView_InsertColumn(hwndList, lvc.iOrder, (LPARAM)&lvc);

		lvc.iOrder = 2;
		lvc.pszText = TranslateT("Port");
		lvc.cx = 50;
		ListView_InsertColumn(hwndList, lvc.iOrder, (LPARAM)&lvc);

		lvc.iOrder = 3;
		lvc.pszText = TranslateT("Public key");
		lvc.cx = 130;
		ListView_InsertColumn(hwndList, lvc.iOrder, (LPARAM)&lvc);

		lvc.iOrder = 4;
		lvc.pszText = NULL;
		lvc.cx = 32 - GetSystemMetrics(SM_CXVSCROLL);
		ListView_InsertColumn(hwndList, lvc.iOrder, (LPARAM)&lvc);

		lvc.iOrder = 5;
		lvc.cx = 32 - GetSystemMetrics(SM_CXVSCROLL);
		ListView_InsertColumn(hwndList, lvc.iOrder, (LPARAM)&lvc);

		LVGROUP lvg = { sizeof(LVGROUP) };
		lvg.mask = LVGF_HEADER | LVGF_GROUPID;

		lvg.pszHeader = TranslateT("Common nodes");
		lvg.iGroupId = 0;
		ListView_InsertGroup(hwndList, lvg.iGroupId, &lvg);

		TCHAR userGroupName[MAX_PATH];
		mir_sntprintf(userGroupName, SIZEOF(userGroupName), _T("%s %s"), userName, TranslateT("nodes"));
		lvg.pszHeader = mir_tstrdup(userGroupName);
		lvg.iGroupId = 1;
		ListView_InsertGroup(hwndList, lvg.iGroupId, &lvg);

		ListView_EnableGroupView(hwndList, TRUE);

		ListView_SetExtendedListViewStyle(hwndList, LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
		ListView_DeleteAllItems(hwndList);
	}
	break;

	case WM_NOTIFY:
		switch (((NMHDR*)lParam)->code)
		{
		case NM_CLICK:
		{
			lvi.iItem = ((NMITEMACTIVATE*)lParam)->iItem;
			lvi.mask = LVIF_GROUPID;
			SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
			lvi.iSubItem = ((NMITEMACTIVATE*)lParam)->iSubItem;
			if (lvi.iGroupId && lvi.iSubItem == 4)
			{
				ItemInfo itemInfo = { lvi.iItem, hwndList };
				if (DialogBoxParam(
					g_hInstance,
					MAKEINTRESOURCE(IDD_ADDNODE),
					GetParent(hwndList), EditNodeDlgProc,
					(LPARAM)&itemInfo) == IDOK)
				{
					SendMessage(GetParent(GetParent(hwndList)), PSM_CHANGED, 0, 0);
				}
			}
			else if (lvi.iGroupId && lvi.iSubItem == 5)
			{
				if (MessageBox(hwndList, TranslateT("Are you sure?"), TranslateT("Node deleting"), MB_YESNO | MB_ICONWARNING) == IDYES)
				{
					ListView_DeleteItem(hwndList, lvi.iItem);
					SendMessage(GetParent(GetParent(hwndList)), PSM_CHANGED, 0, 0);
				}
			}
		}
		break;

		case NM_DBLCLK:
		{
			lvi.iItem = ((NMITEMACTIVATE*)lParam)->iItem;
			lvi.mask = LVIF_GROUPID;
			SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
			if (lvi.iGroupId || (lvi.iGroupId == 0 && lvi.iItem == -1))
			{
				ItemInfo itemInfo = { lvi.iItem, hwndList };
				if (DialogBoxParam(
					g_hInstance,
					MAKEINTRESOURCE(IDD_ADDNODE),
					GetParent(hwndList), EditNodeDlgProc,
					(LPARAM)&itemInfo) == IDOK)
				{
					SendMessage(GetParent(GetParent(hwndList)), PSM_CHANGED, 0, 0);
				}
			}
		}
		break;

		case LVN_KEYDOWN:
		{
			lvi.iItem = ListView_GetSelectionMark(hwndList);
			lvi.mask = LVIF_GROUPID;
			SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
			if (lvi.iGroupId && lvi.iItem != -1 && ((LPNMLVKEYDOWN)lParam)->wVKey == VK_DELETE)
			{
				if (MessageBox(
					GetParent(hwndList),
					TranslateT("Are you sure?"),
					TranslateT("Node deleting"),
					MB_YESNO | MB_ICONWARNING) == IDYES)
				{
					ListView_DeleteItem(hwndList, lvi.iItem);
					SendMessage(GetParent(GetParent(hwndList)), PSM_CHANGED, 0, 0);
				}
			}
		}
		break;
		}
		break;
	
	default:
		return CallWindowProc((WNDPROC)GetClassLongPtr(hwndList, GCLP_WNDPROC), hwndList, uMsg, wParam, lParam);
	}
	return FALSE;
}

INT_PTR CALLBACK CToxProto::NodesOptionsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CToxProto *proto = (CToxProto*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	HWND hwndList = GetDlgItem(hwndDlg, IDC_NODESLIST);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			proto = (CToxProto*)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

			CallWindowProc((WNDPROC)NodeListSubProc, hwndList, WM_INITDIALOG, wParam, (LPARAM)proto->m_tszUserName);

			int iItem = 0;

			if (IsFileExists((TCHAR*)VARST(_T(TOX_INI_PATH))))
			{
				char fileName[MAX_PATH];
				mir_strcpy(fileName, VARS(TOX_INI_PATH));

				char *section, sections[MAX_PATH], value[MAX_PATH];
				GetPrivateProfileSectionNamesA(sections, SIZEOF(sections), fileName);
				section = sections;
				while (*section != NULL)
				{
					if (strstr(section, TOX_SETTINGS_NODE_PREFIX) == section)
					{
						GetPrivateProfileStringA(section, "IPv4", NULL, value, SIZEOF(value), fileName);
						AddItemToListView(hwndList, LVIF_GROUPID | LVIF_TEXT | LVIF_IMAGE, 0, iItem, 0, value);

						GetPrivateProfileStringA(section, "IPv6", NULL, value, SIZEOF(value), fileName);
						SetSubItemToListView(hwndList, LVIF_TEXT, 0, iItem, 1, value);

						GetPrivateProfileStringA(section, "Port", NULL, value, SIZEOF(value), fileName);
						SetSubItemToListView(hwndList, LVIF_TEXT, 0, iItem, 2, value);

						GetPrivateProfileStringA(section, "PubKey", NULL, value, SIZEOF(value), fileName);
						SetSubItemToListView(hwndList, LVIF_TEXT, 0, iItem, 3, value);

						iItem++;
					}
					section += strlen(section) + 1;
				}
			}

			char module[MAX_PATH], setting[MAX_PATH];
			mir_snprintf(module, SIZEOF(module), "%s_Nodes", proto->m_szModuleName);
			int nodeCount = db_get_w(NULL, module, TOX_SETTINGS_NODE_COUNT, 0);
			for (int i = 0; i < nodeCount; i++, iItem++)
			{
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_IPV4, i);
				ptrA value(db_get_sa(NULL, module, setting));
				AddItemToListView(hwndList, LVIF_GROUPID | LVIF_TEXT | LVIF_IMAGE, 1, iItem, 0, value);

				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_IPV6, i);
				value = db_get_sa(NULL, module, setting);
				SetSubItemToListView(hwndList, LVIF_TEXT, 1, iItem, 1, value);

				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_PORT, i);
				int port = db_get_w(NULL, module, setting, 0);
				if (port > 0)
				{
					char portNum[10];
					itoa(port, portNum, 10);
					SetSubItemToListView(hwndList, LVIF_TEXT, 1, iItem, 2, portNum);
				}

				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_PKEY, i);
				value = db_get_sa(NULL, module, setting);
				SetSubItemToListView(hwndList, LVIF_TEXT, 1, iItem, 3, value);

				SetSubItemToListView(hwndList, LVIF_IMAGE, 1, iItem, 4, value, 0);
				SetSubItemToListView(hwndList, LVIF_IMAGE, 1, iItem, 5, value, 1);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_ADDNODE:
		{
			ItemInfo itemInfo = { -1, hwndList };
			if (DialogBoxParam(
				g_hInstance,
				MAKEINTRESOURCE(IDD_ADDNODE),
				GetParent(hwndList), EditNodeDlgProc,
				(LPARAM)&itemInfo) == IDOK)
			{
				SendMessage(GetParent(GetParent(hwndList)), PSM_CHANGED, 0, 0);
			}
		}
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_CLICK:
		case NM_DBLCLK:
		case LVN_KEYDOWN:
			if (((NMHDR*)lParam)->hwndFrom == hwndList)
			{
				return CallWindowProc((WNDPROC)NodeListSubProc, hwndList, uMsg, wParam, lParam);
			}
			break;

		case PSN_APPLY:
		{
			char setting[MAX_PATH];

			LVITEMA lvi = { 0 };
			lvi.cchTextMax = MAX_PATH;
			lvi.pszText = (char*)mir_alloc(MAX_PATH);

			char module[MAX_PATH];
			mir_snprintf(module, SIZEOF(module), "%s_Nodes", proto->m_szModuleName);

			int iItem = 0;
			int itemCount = ListView_GetItemCount(hwndList);
			for (int i = 0; i < itemCount; i++)
			{
				lvi.iItem = i;
				lvi.mask = LVIF_GROUPID;
				SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				if (lvi.iGroupId == 0)
				{
					continue;
				}

				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = 0;
				SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_IPV4, iItem);
				db_set_s(NULL, module, setting, lvi.pszText);

				lvi.iSubItem = 1;
				SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_IPV6, iItem);
				db_set_s(NULL, module, setting, lvi.pszText);

				lvi.iSubItem = 2;
				SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_PORT, iItem);
				db_set_w(NULL, module, setting, atoi(lvi.pszText));

				lvi.iSubItem = 3;
				SendMessage(hwndList, LVM_GETITEMA, 0, (LPARAM)&lvi);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_PKEY, iItem);
				db_set_s(NULL, module, setting, lvi.pszText);

				iItem++;
			}
			itemCount = iItem;
			int nodeCount = db_get_b(NULL, module, TOX_SETTINGS_NODE_COUNT, 0);
			for (iItem = itemCount; iItem < nodeCount; iItem++)
			{
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_IPV4, iItem);
				db_unset(NULL, module, setting);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_IPV6, iItem);
				db_unset(NULL, module, setting);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_PORT, iItem);
				db_unset(NULL, module, setting);
				mir_snprintf(setting, SIZEOF(setting), TOX_SETTINGS_NODE_PKEY, iItem);
				db_unset(NULL, module, setting);
			}
			db_set_b(NULL, module, TOX_SETTINGS_NODE_COUNT, itemCount);
		}
		return TRUE;
		}
	}
	return FALSE;
}

int CToxProto::OnOptionsInit(WPARAM wParam, LPARAM)
{
	char *title = mir_t2a(m_tszUserName);

	OPTIONSDIALOGPAGE odp = { sizeof(odp) };
	odp.hInstance = g_hInstance;
	odp.pszTitle = title;
	odp.dwInitParam = (LPARAM)this;
	odp.flags = ODPF_BOLDGROUPS;
	odp.pszGroup = LPGEN("Network");

	odp.pszTab = LPGEN("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS_MAIN);
	odp.pfnDlgProc = CDlgBase::DynamicDlgProc;
	odp.dwInitParam = (LPARAM)&ToxMainOptions;
	ToxMainOptions.create = CToxOptionsMain::Create;
	ToxMainOptions.param = this;
	Options_AddPage(wParam, &odp);

	/*odp.pszTab = LPGEN("Audio/Video");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS_AV);
	odp.pfnDlgProc = AVOptionsProc;
	Options_AddPage(wParam, &odp);*/

	odp.pszTab = LPGEN("Nodes");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONS_NODES);
	odp.pfnDlgProc = NodesOptionsProc;
	Options_AddPage(wParam, &odp);

	mir_free(title);

	return 0;
}