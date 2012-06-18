#include "common.h"
#include <shlobj.h>
#pragma hdrstop

extern TopButtonInt Buttons[MAX_BUTTONS];
extern int nButtonsCount;

extern SortData arrangedbuts[MAX_BUTTONS];

HWND OptionshWnd = 0;

struct OrderData
{
	int dragging;
	HTREEITEM hDragItem;
	HIMAGELIST himlButtonIcons;
};

int BuildTree(HWND hwndDlg)
{
	HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);
	OrderData *dat = (struct OrderData*)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), GWLP_USERDATA);

	dat->himlButtonIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 2, 2);
	TreeView_SetImageList(hTree, dat->himlButtonIcons, TVSIL_NORMAL);
	SetWindowLongPtr(hTree, GWL_STYLE, GetWindowLongPtr(hTree,GWL_STYLE)|TVS_NOHSCROLL);
	TreeView_DeleteAllItems(hTree);

	if (nButtonsCount == 0)
		return FALSE;

	TVINSERTSTRUCT tvis = { 0 };
	tvis.hInsertAfter = TVI_LAST;

	int index;
	TCHAR* tmp;

	for (int i = 0; i < nButtonsCount; i++) {
		TopButtonInt &b = Buttons[arrangedbuts[i].oldpos];

		index = 0;

		if (b.dwFlags & TTBBF_ISSEPARATOR) {
			tvis.item.mask = TVIF_PARAM | TVIF_TEXT;
			tmp = mir_wstrdup(L"------------------");
			index = -1;
		}
		else {
			tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			if (b.dwFlags & TTBBF_ICONBYHANDLE) {
				HICON hIcon = Skin_GetIconByHandle(b.hIconHandleUp);
				index = ImageList_AddIcon(dat->himlButtonIcons, hIcon);
				Skin_ReleaseIcon(hIcon);
			}
			else index = ImageList_AddIcon(dat->himlButtonIcons, b.hIconUp);
			tvis.item.iImage = tvis.item.iSelectedImage = index;

			tmp = mir_a2t( b.name );
		}

		tvis.item.lParam = (LPARAM)&b;
		tvis.item.pszText = TranslateTS(tmp);
		HTREEITEM hti = TreeView_InsertItem(hTree, &tvis);
		mir_free(tmp);

		TreeView_SetCheckState(hTree, hti, (b.dwFlags & TTBBF_VISIBLE) ? TRUE : FALSE);
	}

	return (TRUE);
}

//call this when options opened and buttons added/removed
int OptionsPageRebuild()
{
	if (OptionshWnd)
		BuildTree(OptionshWnd);

	return 0;
}

int SaveTree(HWND hwndDlg)
{
	HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);

	TVITEM tvi = { 0 };
	tvi.hItem = TreeView_GetRoot(hTree);

	int count = 0;
	lockbut();

	while(tvi.hItem != NULL) {
		tvi.stateMask = TVIS_STATEIMAGEMASK;
		tvi.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_STATE;
		TreeView_GetItem(hTree, &tvi);

		TopButtonInt* btn = (TopButtonInt*)tvi.lParam;
		// can use TreeView_GetCheckState(hTree,tvi.hItem);
		// WTF?!
//		if (btn->arrangedpos >= 0 && btn->arrangedpos)
		{
			if ((tvi.state >> 12 ) == 0x2)
				btn->dwFlags |= TTBBF_VISIBLE;
			else
				btn->dwFlags &= ~TTBBF_VISIBLE;
			btn->arrangedpos = count;
		}

		tvi.hItem = TreeView_GetNextSibling(hTree, tvi.hItem);
		count++;
	}

	ulockbut();
	ttbOptionsChanged();
	return (TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Options window: main

static INT_PTR CALLBACK ButOrderOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TopButtonInt* btn;
	struct OrderData *dat = (struct OrderData*)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		dat = (struct OrderData*)malloc(sizeof(struct OrderData));
		SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), GWLP_USERDATA, (LONG)dat);
		dat->dragging = 0;

		SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), GWL_STYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), GWL_STYLE)|TVS_NOHSCROLL);

		SetDlgItemInt(hwndDlg, IDC_BUTTHEIGHT, DBGetContactSettingByte(0, TTB_OPTDIR, "BUTTHEIGHT", 16), FALSE);
		SetDlgItemInt(hwndDlg, IDC_BUTTWIDTH, DBGetContactSettingByte(0, TTB_OPTDIR, "BUTTWIDTH", 20), FALSE);
		CheckDlgButton(hwndDlg, IDC_USEFLAT, DBGetContactSettingByte(0, TTB_OPTDIR, "UseFlatButton", 1));

		BuildTree(hwndDlg);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ENAME), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EPATH), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DELLBUTTON), FALSE);

		OptionshWnd = hwndDlg;
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE && OptionshWnd) {
			switch(LOWORD(wParam)) {
			case IDC_ENAME: case IDC_EPATH:
				break;
			default:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			break;
		}

		if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_DBLCLK)) {
			int ctrlid = LOWORD(wParam);
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			//----- Launch buttons -----

			if (ctrlid == IDC_LBUTTONSET) {
				TVITEM tvi;
				tvi.hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE));
				if (tvi.hItem == NULL)
					break;

				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &tvi);

				btn = (TopButtonInt*)tvi.lParam;
				TCHAR buf [256];
				// probably, condition not needs
				if (btn->dwFlags & TTBBF_ISLBUTTON) {
				  if (btn->name) free(btn->name);
					GetDlgItemText(hwndDlg, IDC_ENAME, buf, 255);
					btn->name = _strdup( _T2A(buf));

					if (btn->program) free(btn->program);
					GetDlgItemText(hwndDlg, IDC_EPATH, buf, 255);
					btn->program = _tcsdup(buf);

				tvi.mask = TVIF_TEXT;
				tvi.pszText =  mir_a2t( btn->name );
				TreeView_SetItem(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &tvi);
				}
				break;
			}

			if (ctrlid == IDC_ADDLBUTTON) {
				InsertLBut(-1);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}

			//----- Separators -----

			if (ctrlid == IDC_ADDSEP) {
				InsertSeparator(-1);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}

			if (ctrlid == IDC_REMOVEBUTTON) {
				TVITEM tvi = {0};
				tvi.hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE));
				if (tvi.hItem == NULL)
					break;

				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &tvi);

				btn = (TopButtonInt*)tvi.lParam;
				if (btn->dwFlags & TTBBF_ISSEPARATOR) {
					DeleteSeparator(btn->wParamDown);
					TTBRemoveButton(btn->lParamDown, 0);
				}
				else if (btn->dwFlags & TTBBF_ISLBUTTON) {
					DeleteLBut(btn->wParamDown);
					TTBRemoveButton(btn->lParamDown, 0);
				}

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}
		}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				DBWriteContactSettingByte(0, TTB_OPTDIR, "BUTTHEIGHT", GetDlgItemInt(hwndDlg, IDC_BUTTHEIGHT, FALSE, FALSE));
				DBWriteContactSettingByte(0, TTB_OPTDIR, "BUTTWIDTH", GetDlgItemInt(hwndDlg, IDC_BUTTWIDTH, FALSE, FALSE));
				DBWriteContactSettingByte(0, TTB_OPTDIR, "UseFlatButton", (BYTE)IsDlgButtonChecked(hwndDlg, IDC_USEFLAT));

				SaveTree(hwndDlg);
				RecreateWindows();
				ArrangeButtons();
			}
			break;

		case IDC_BUTTONORDERTREE:
			switch (((LPNMHDR)lParam)->code) {
			case TVN_BEGINDRAGA:
			case TVN_BEGINDRAGW:
				SetCapture(hwndDlg);
				dat->dragging = 1;
				dat->hDragItem = ((LPNMTREEVIEW)lParam)->itemNew.hItem;
				TreeView_SelectItem(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), dat->hDragItem);
				break;

			case NM_CLICK:
				{
					TVHITTESTINFO hti;
					hti.pt.x = (short)LOWORD(GetMessagePos());
					hti.pt.y = (short)HIWORD(GetMessagePos());
					ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
					if (TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti))
						if (hti.flags & TVHT_ONITEMSTATEICON) {
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
							TreeView_SelectItem(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), hti.hItem);
						}
				}
				break;

			case TVN_SELCHANGEDA:
			case TVN_SELCHANGEDW:
				{
					HTREEITEM hti = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE));
					if (hti == NULL)
						break;

					TopButtonInt *btn = (TopButtonInt*)((LPNMTREEVIEW)lParam)->itemNew.lParam;
					lockbut();

					if (btn->dwFlags & TTBBF_ISLBUTTON) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_ENAME), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EPATH), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEBUTTON), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LBUTTONSET), TRUE);
						if (btn->name != NULL)
							SetDlgItemTextA(hwndDlg, IDC_ENAME, btn->name);
						if (btn->program != NULL)
							SetDlgItemText(hwndDlg, IDC_EPATH, btn->program);
					}
					else
					{
						if (btn->dwFlags & TTBBF_ISSEPARATOR)
							EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEBUTTON), TRUE);
						else
							EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEBUTTON), FALSE);

						EnableWindow(GetDlgItem(hwndDlg, IDC_ENAME), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EPATH), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LBUTTONSET), FALSE);
						SetDlgItemTextA(hwndDlg, IDC_ENAME, "");
						SetDlgItemTextA(hwndDlg, IDC_EPATH, "");
					}
					ulockbut();
				}
			}
			break;
		}
		break;

	case WM_MOUSEMOVE:
		if (dat->dragging) {
			TVHITTESTINFO hti;
			hti.pt.x = (short)LOWORD(lParam);
			hti.pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwndDlg, &hti.pt);
			ScreenToClient(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &hti.pt);
			TreeView_HitTest(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &hti);
			if (hti.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT)) {
				hti.pt.y -= TreeView_GetItemHeight(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &hti);
				TreeView_SetInsertMark(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), hti.hItem, 1);
			}
			else {
				if (hti.flags & TVHT_ABOVE) SendDlgItemMessage(hwndDlg, IDC_BUTTONORDERTREE, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
				if (hti.flags & TVHT_BELOW) SendDlgItemMessage(hwndDlg, IDC_BUTTONORDERTREE, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
				TreeView_SetInsertMark(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), NULL, 0);
			}
		}
		break;

	case WM_DESTROY:
		if (dat) {
			ImageList_Destroy(dat->himlButtonIcons);
			free(dat);
		}
		OptionshWnd = NULL;
		return 0;

	case WM_LBUTTONUP:
		if (dat->dragging) {
			HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);
			TreeView_SetInsertMark(hTree, NULL, 0);
			dat->dragging = 0;
			ReleaseCapture();

			TVHITTESTINFO hti;
			hti.pt.x = (short)LOWORD(lParam);
			hti.pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwndDlg, &hti.pt);
			ScreenToClient(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE), &hti.pt);
			hti.pt.y -= TreeView_GetItemHeight(GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE))/2;
			TreeView_HitTest(hTree, &hti);
			if (dat->hDragItem == hti.hItem)
				break;

			TVITEM tvi;
			tvi.mask = TVIF_HANDLE|TVIF_PARAM;
			tvi.hItem = hti.hItem;
			TreeView_GetItem(hTree, &tvi);
			if (hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)) {
				TVINSERTSTRUCT tvis;
				TCHAR name[128];
				tvis.item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
				tvis.item.stateMask = 0xFFFFFFFF;
				tvis.item.pszText = name;
				tvis.item.cchTextMax = SIZEOF(name);
				tvis.item.hItem = dat->hDragItem;
				TreeView_GetItem(hTree, &tvis.item);

				TreeView_DeleteItem(hTree, dat->hDragItem);
				tvis.hParent = NULL;
				tvis.hInsertAfter = hti.hItem;
				TreeView_SelectItem(hTree, TreeView_InsertItem(hTree, &tvis));
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Options window: background

static INT_PTR CALLBACK DlgProcTTBBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_BITMAP, DBGetContactSettingByte(NULL, TTB_OPTDIR, "UseBitmap", TTBDEFAULT_USEBITMAP)?BST_CHECKED:BST_UNCHECKED);
		SendMessage(hwndDlg, WM_USER+10, 0, 0);
		SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, TTBDEFAULT_BKCOLOUR);
		SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, TTB_OPTDIR, "BkColour", TTBDEFAULT_BKCOLOUR));
		SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_SETDEFAULTCOLOUR, 0, TTBDEFAULT_SELBKCOLOUR);
		SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, TTB_OPTDIR, "SelBkColour", TTBDEFAULT_SELBKCOLOUR));
		{
			DBVARIANT dbv;
			if ( !DBGetContactSetting(NULL, TTB_OPTDIR, "BkBitmap", &dbv)) {
				SetDlgItemText(hwndDlg, IDC_FILENAME, dbv.ptszVal);
				DBFreeVariant(&dbv);
			}

			WORD bmpUse = DBGetContactSettingWord(NULL, TTB_OPTDIR, "BkBmpUse", TTBDEFAULT_BKBMPUSE);
			CheckDlgButton(hwndDlg, IDC_STRETCHH, bmpUse&CLB_STRETCHH?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_STRETCHV, bmpUse&CLB_STRETCHV?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_TILEH, bmpUse&CLBF_TILEH?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_TILEV, bmpUse&CLBF_TILEV?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SCROLL, bmpUse&CLBF_SCROLL?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_PROPORTIONAL, bmpUse&CLBF_PROPORTIONAL?BST_CHECKED:BST_UNCHECKED);

			HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND, DWORD);
			MySHAutoComplete = (HRESULT (STDAPICALLTYPE*)(HWND, DWORD))GetProcAddress(GetModuleHandleA("shlwapi"), "SHAutoComplete");
			if (MySHAutoComplete)
				MySHAutoComplete(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
		}
		return TRUE;

	case WM_USER+10:
		EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_STRETCHH), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_STRETCHV), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_TILEH), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_TILEV), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_SCROLL), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		EnableWindow(GetDlgItem(hwndDlg, IDC_PROPORTIONAL), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BROWSE) {
			TCHAR str[MAX_PATH];
			OPENFILENAME ofn = {0};
			TCHAR filter[512];

			GetDlgItemText(hwndDlg, IDC_FILENAME, str, sizeof(str));
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
			ofn.hwndOwner = hwndDlg;
			ofn.hInstance = NULL;
			CallService(MS_UTILS_GETBITMAPFILTERSTRINGST, SIZEOF(filter), (LPARAM)filter);
			ofn.lpstrFilter = filter;
			ofn.lpstrFile = str;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.nMaxFile = sizeof(str);
			ofn.nMaxFileTitle = MAX_PATH;
			ofn.lpstrDefExt = _T("bmp");
			if (!GetOpenFileName(&ofn))
				break;

			SetDlgItemText(hwndDlg, IDC_FILENAME, str);
		}
		else if (LOWORD(wParam) == IDC_FILENAME && HIWORD(wParam) != EN_CHANGE)
			break;

		if (LOWORD(wParam) == IDC_BITMAP) SendMessage(hwndDlg, WM_USER+10, 0, 0);
		if (LOWORD(wParam) == IDC_FILENAME && (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())) return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				DBWriteContactSettingByte(NULL, TTB_OPTDIR, "UseBitmap", (BYTE)IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
				{
					COLORREF col = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0);
					if (col == TTBDEFAULT_BKCOLOUR)
						DBDeleteContactSetting(NULL, TTB_OPTDIR, "BkColour");
					else
						DBWriteContactSettingDword(NULL, TTB_OPTDIR, "BkColour", col);
					col = SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_GETCOLOUR, 0, 0);
					if (col == TTBDEFAULT_SELBKCOLOUR)
						DBDeleteContactSetting(NULL, TTB_OPTDIR, "SelBkColour");
					else
						DBWriteContactSettingDword(NULL, TTB_OPTDIR, "SelBkColour", col);

					TCHAR str[MAX_PATH];
					GetDlgItemText(hwndDlg, IDC_FILENAME, str, SIZEOF(str));
					DBWriteContactSettingTString(NULL, TTB_OPTDIR, "BkBitmap", str);

					WORD flags = 0;
					if (IsDlgButtonChecked(hwndDlg, IDC_STRETCHH)) flags |= CLB_STRETCHH;
					if (IsDlgButtonChecked(hwndDlg, IDC_STRETCHV)) flags |= CLB_STRETCHV;
					if (IsDlgButtonChecked(hwndDlg, IDC_TILEH)) flags |= CLBF_TILEH;
					if (IsDlgButtonChecked(hwndDlg, IDC_TILEV)) flags |= CLBF_TILEV;
					if (IsDlgButtonChecked(hwndDlg, IDC_SCROLL)) flags |= CLBF_SCROLL;
					if (IsDlgButtonChecked(hwndDlg, IDC_PROPORTIONAL)) flags |= CLBF_PROPORTIONAL;
					DBWriteContactSettingWord(NULL, TTB_OPTDIR, "BkBmpUse", flags);
				}

				ttbOptionsChanged();
				return TRUE;
			}
			break;
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

int TTBOptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.pszGroup = LPGEN("TopToolBar");

	if ( !ServiceExists(MS_BACKGROUNDCONFIG_REGISTER)) {
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_TTBBKG);
		odp.pszTitle = LPGEN("TTBBackground");
		odp.pfnDlgProc = DlgProcTTBBkgOpts;
		odp.flags = ODPF_BOLDGROUPS;
		CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	}

	odp.position = -1000000000;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_BUTORDER);
	odp.pszGroup = LPGEN("TopToolBar");
	odp.pszTitle = LPGEN("Buttons");
	odp.pfnDlgProc = ButOrderOpts;
	odp.flags = ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	return 0;
}
