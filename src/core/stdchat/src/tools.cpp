/*
Chat module plugin for Miranda IM

Copyright 2000-12 Miranda IM, 2012-14 Miranda NG project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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

#include "chat.h"

int GetRichTextLength(HWND hwnd)
{
	GETTEXTLENGTHEX gtl;
	gtl.flags = GTL_PRECISE;
	gtl.codepage = CP_ACP;
	return (int)SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
}

TCHAR* RemoveFormatting(const TCHAR* pszWord)
{
	static TCHAR szTemp[10000];
	int i = 0;
	int j = 0;

	if (pszWord == 0 || lstrlen(pszWord) == 0)
		return NULL;

	while (j < 9999 && i <= lstrlen(pszWord)) {
		if (pszWord[i] == '%') {
			switch (pszWord[i + 1]) {
			case '%':
				szTemp[j] = '%';
				j++;
				i++; i++;
				break;
			case 'b':
			case 'u':
			case 'i':
			case 'B':
			case 'U':
			case 'I':
			case 'r':
			case 'C':
			case 'F':
				i++; i++;
				break;

			case 'c':
			case 'f':
				i += 4;
				break;

			default:
				szTemp[j] = pszWord[i];
				j++;
				i++;
				break;
			}
		}
		else {
			szTemp[j] = pszWord[i];
			j++;
			i++;
		}
	}

	return (TCHAR*)&szTemp;
}

static void __stdcall ShowRoomFromPopup(void * pi)
{
	SESSION_INFO *si = (SESSION_INFO*)pi;
	pci->ShowRoom(si, WINDOW_VISIBLE, TRUE);
}

static LRESULT CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
		if (HIWORD(wParam) == STN_CLICKED) {
			SESSION_INFO *si = (SESSION_INFO*)PUGetPluginData(hWnd);
			CallFunctionAsync(ShowRoomFromPopup, si);

			PUDeletePopup(hWnd);
			return TRUE;
		}
		break;
	case WM_CONTEXTMENU:
		SESSION_INFO *si = (SESSION_INFO*)PUGetPluginData(hWnd);
		if (si->hContact)
		if (CallService(MS_CLIST_GETEVENT, (WPARAM)si->hContact, 0))
			CallService(MS_CLIST_REMOVEEVENT, (WPARAM)si->hContact, (LPARAM)"chaticon");

		if (si->hWnd && KillTimer(si->hWnd, TIMERID_FLASHWND))
			FlashWindow(si->hWnd, FALSE);

		PUDeletePopup(hWnd);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static int ShowPopup(HANDLE hContact, SESSION_INFO *si, HICON hIcon, char* pszProtoName, TCHAR* pszRoomName, COLORREF crBkg, const TCHAR* fmt, ...)
{
	static TCHAR szBuf[4 * 1024];

	if (!fmt || lstrlen(fmt) == 0 || lstrlen(fmt) > 2000)
		return 0;

	va_list marker;
	va_start(marker, fmt);
	mir_vsntprintf(szBuf, 4096, fmt, marker);
	va_end(marker);

	POPUPDATAT pd = { 0 };
	pd.lchContact = hContact;

	if (hIcon)
		pd.lchIcon = hIcon;
	else
		pd.lchIcon = LoadIconEx("window", FALSE);

	PROTOACCOUNT *pa = ProtoGetAccount(pszProtoName);
	mir_sntprintf(pd.lptzContactName, MAX_CONTACTNAME - 1, _T("%s - %s"),
		(pa == NULL) ? _A2T(pszProtoName) : pa->tszAccountName,
		pcli->pfnGetContactDisplayName(hContact, 0));

	lstrcpyn(pd.lptzText, TranslateTS(szBuf), MAX_SECONDLINE);
	pd.iSeconds = g_Settings.iPopupTimeout;

	if (g_Settings.iPopupStyle == 2) {
		pd.colorBack = 0;
		pd.colorText = 0;
	}
	else if (g_Settings.iPopupStyle == 3) {
		pd.colorBack = g_Settings.crPUBkgColour;
		pd.colorText = g_Settings.crPUTextColour;
	}
	else {
		pd.colorBack = g_Settings.crLogBackground;
		pd.colorText = crBkg;
	}

	pd.PluginWindowProc = PopupDlgProc;
	pd.PluginData = si;
	return PUAddPopupT(&pd);
}

static BOOL DoTrayIcon(SESSION_INFO *si, GCEVENT *gce)
{
	int iEvent = gce->pDest->iType;

	if (iEvent&g_Settings.dwTrayIconFlags) {
		switch (iEvent) {
		case GC_EVENT_MESSAGE | GC_EVENT_HIGHLIGHT:
		case GC_EVENT_ACTION | GC_EVENT_HIGHLIGHT:
			pci->AddEvent(si->hContact, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), "chaticon", 0, TranslateT("%s wants your attention in %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_MESSAGE:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_MESSAGE], "chaticon", CLEF_ONLYAFEW, TranslateT("%s speaks in %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_ACTION:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_ACTION], "chaticon", CLEF_ONLYAFEW, TranslateT("%s speaks in %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_JOIN:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_JOIN], "chaticon", CLEF_ONLYAFEW, TranslateT("%s has joined %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_PART:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_PART], "chaticon", CLEF_ONLYAFEW, TranslateT("%s has left %s"), gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_QUIT:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_QUIT], "chaticon", CLEF_ONLYAFEW, TranslateT("%s has disconnected"), gce->ptszNick);
			break;
		case GC_EVENT_NICK:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_NICK], "chaticon", CLEF_ONLYAFEW, TranslateT("%s is now known as %s"), gce->ptszNick, gce->ptszText);
			break;
		case GC_EVENT_KICK:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_KICK], "chaticon", CLEF_ONLYAFEW, TranslateT("%s kicked %s from %s"), gce->ptszStatus, gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_NOTICE:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_NOTICE], "chaticon", CLEF_ONLYAFEW, TranslateT("Notice from %s"), gce->ptszNick);
			break;
		case GC_EVENT_TOPIC:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_TOPIC], "chaticon", CLEF_ONLYAFEW, TranslateT("Topic change in %s"), si->ptszName);
			break;
		case GC_EVENT_INFORMATION:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_INFO], "chaticon", CLEF_ONLYAFEW, TranslateT("Information in %s"), si->ptszName);
			break;
		case GC_EVENT_ADDSTATUS:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_ADDSTATUS], "chaticon", CLEF_ONLYAFEW, TranslateT("%s enables \'%s\' status for %s in %s"), gce->ptszText, gce->ptszStatus, gce->ptszNick, si->ptszName);
			break;
		case GC_EVENT_REMOVESTATUS:
			pci->AddEvent(si->hContact, pci->hIcons[ICON_REMSTATUS], "chaticon", CLEF_ONLYAFEW, TranslateT("%s disables \'%s\' status for %s in %s"), gce->ptszText, gce->ptszStatus, gce->ptszNick, si->ptszName);
			break;
		}
	}

	return TRUE;
}

static BOOL DoPopup(SESSION_INFO *si, GCEVENT *gce)
{
	int iEvent = gce->pDest->iType;

	if (iEvent & g_Settings.dwPopupFlags) {
		switch (iEvent) {
		case GC_EVENT_MESSAGE | GC_EVENT_HIGHLIGHT:
			ShowPopup(si->hContact, si, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), si->pszModule, si->ptszName, pci->aFonts[16].color, TranslateT("%s says: %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_ACTION | GC_EVENT_HIGHLIGHT:
			ShowPopup(si->hContact, si, LoadSkinnedIcon(SKINICON_EVENT_MESSAGE), si->pszModule, si->ptszName, pci->aFonts[16].color, _T("%s %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_MESSAGE:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_MESSAGE], si->pszModule, si->ptszName, pci->aFonts[9].color, TranslateT("%s says: %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_ACTION:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_ACTION], si->pszModule, si->ptszName, pci->aFonts[15].color, _T("%s %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_JOIN:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_JOIN], si->pszModule, si->ptszName, pci->aFonts[3].color, TranslateT("%s has joined"), gce->ptszNick);
			break;
		case GC_EVENT_PART:
			if (!gce->ptszText)
				ShowPopup(si->hContact, si, pci->hIcons[ICON_PART], si->pszModule, si->ptszName, pci->aFonts[4].color, TranslateT("%s has left"), gce->ptszNick);
			else
				ShowPopup(si->hContact, si, pci->hIcons[ICON_PART], si->pszModule, si->ptszName, pci->aFonts[4].color, TranslateT("%s has left (%s)"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_QUIT:
			if (!gce->ptszText)
				ShowPopup(si->hContact, si, pci->hIcons[ICON_QUIT], si->pszModule, si->ptszName, pci->aFonts[5].color, TranslateT("%s has disconnected"), gce->ptszNick);
			else
				ShowPopup(si->hContact, si, pci->hIcons[ICON_QUIT], si->pszModule, si->ptszName, pci->aFonts[5].color, TranslateT("%s has disconnected (%s)"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_NICK:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_NICK], si->pszModule, si->ptszName, pci->aFonts[7].color, TranslateT("%s is now known as %s"), gce->ptszNick, gce->ptszText);
			break;
		case GC_EVENT_KICK:
			if (!gce->ptszText)
				ShowPopup(si->hContact, si, pci->hIcons[ICON_KICK], si->pszModule, si->ptszName, pci->aFonts[6].color, TranslateT("%s kicked %s"), (char *)gce->ptszStatus, gce->ptszNick);
			else
				ShowPopup(si->hContact, si, pci->hIcons[ICON_KICK], si->pszModule, si->ptszName, pci->aFonts[6].color, TranslateT("%s kicked %s (%s)"), (char *)gce->ptszStatus, gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_NOTICE:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_NOTICE], si->pszModule, si->ptszName, pci->aFonts[8].color, TranslateT("Notice from %s: %s"), gce->ptszNick, RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_TOPIC:
			if (!gce->ptszNick)
				ShowPopup(si->hContact, si, pci->hIcons[ICON_TOPIC], si->pszModule, si->ptszName, pci->aFonts[11].color, TranslateT("The topic is \'%s\'"), RemoveFormatting(gce->ptszText));
			else
				ShowPopup(si->hContact, si, pci->hIcons[ICON_TOPIC], si->pszModule, si->ptszName, pci->aFonts[11].color, TranslateT("The topic is \'%s\' (set by %s)"), RemoveFormatting(gce->ptszText), gce->ptszNick);
			break;
		case GC_EVENT_INFORMATION:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_INFO], si->pszModule, si->ptszName, pci->aFonts[12].color, _T("%s"), RemoveFormatting(gce->ptszText));
			break;
		case GC_EVENT_ADDSTATUS:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_ADDSTATUS], si->pszModule, si->ptszName, pci->aFonts[13].color, TranslateT("%s enables \'%s\' status for %s"), gce->ptszText, (char *)gce->ptszStatus, gce->ptszNick);
			break;
		case GC_EVENT_REMOVESTATUS:
			ShowPopup(si->hContact, si, pci->hIcons[ICON_REMSTATUS], si->pszModule, si->ptszName, pci->aFonts[14].color, TranslateT("%s disables \'%s\' status for %s"), gce->ptszText, (char *)gce->ptszStatus, gce->ptszNick);
			break;
		}
	}

	return TRUE;
}

BOOL DoSoundsFlashPopupTrayStuff(SESSION_INFO *si, GCEVENT *gce, BOOL bHighlight, int bManyFix)
{
	if (!gce || !si || gce->bIsMe || si->iType == GCW_SERVER)
		return FALSE;

	BOOL bInactive = si->hWnd == NULL || GetForegroundWindow() != si->hWnd;
	// bInactive |=  GetActiveWindow() != si->hWnd; // Removed this, because it seemed to be FALSE, even when window was focused, causing incorrect notifications

	int iEvent = gce->pDest->iType;

	if (bHighlight) {
		gce->pDest->iType |= GC_EVENT_HIGHLIGHT;
		if (bInactive || !g_Settings.SoundsFocus)
			SkinPlaySound("ChatHighlight");
		if (!g_Settings.TabsEnable && bInactive && si->hWnd && db_get_b(NULL, "Chat", "FlashWindowHighlight", 0) != 0)
			SetTimer(si->hWnd, TIMERID_FLASHWND, 900, NULL);
		if (db_get_b(si->hContact, "CList", "Hidden", 0) != 0)
			db_unset(si->hContact, "CList", "Hidden");
		if (bInactive)
			DoTrayIcon(si, gce);
		if (bInactive || !g_Settings.PopupInactiveOnly)
			DoPopup(si, gce);
		if (g_Settings.TabsEnable && bInactive && g_TabSession.hWnd)
			SendMessage(g_TabSession.hWnd, GC_SETMESSAGEHIGHLIGHT, 0, (LPARAM)si);
		return TRUE;
	}

	// do blinking icons in tray
	if (bInactive || !g_Settings.TrayIconInactiveOnly)
		DoTrayIcon(si, gce);

	// stupid thing to not create multiple popups for a QUIT event for instance
	if (bManyFix == 0) {
		// do popups
		if (bInactive || !g_Settings.PopupInactiveOnly)
			DoPopup(si, gce);

		// do sounds and flashing
		switch (iEvent) {
		case GC_EVENT_JOIN:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatJoin");
			break;
		case GC_EVENT_PART:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatPart");
			break;
		case GC_EVENT_QUIT:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatQuit");
			break;
		case GC_EVENT_ADDSTATUS:
		case GC_EVENT_REMOVESTATUS:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatMode");
			break;
		case GC_EVENT_KICK:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatKick");
			break;
		case GC_EVENT_MESSAGE:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatMessage");
			if (!g_Settings.TabsEnable && bInactive && g_Settings.FlashWindow && si->hWnd)
				SetTimer(si->hWnd, TIMERID_FLASHWND, 900, NULL);

			if (bInactive && !(si->wState & STATE_TALK)) {
				si->wState |= STATE_TALK;
				db_set_w(si->hContact, si->pszModule, "ApparentMode", (LPARAM)(WORD)40071);
			}
			if (g_Settings.TabsEnable && bInactive && g_TabSession.hWnd)
				SendMessage(g_TabSession.hWnd, GC_SETTABHIGHLIGHT, 0, (LPARAM)si);
			break;
		case GC_EVENT_ACTION:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatAction");
			break;
		case GC_EVENT_NICK:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatNick");
			break;
		case GC_EVENT_NOTICE:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatNotice");
			break;
		case GC_EVENT_TOPIC:
			if (bInactive || !g_Settings.SoundsFocus)
				SkinPlaySound("ChatTopic");
			break;
		}
	}

	return TRUE;
}

int GetColorIndex(const char* pszModule, COLORREF cr)
{
	MODULEINFO * pMod = pci->MM_FindModule(pszModule);
	int i = 0;

	if (!pMod || pMod->nColorCount == 0)
		return -1;

	for (i = 0; i < pMod->nColorCount; i++)
	if (pMod->crColors[i] == cr)
		return i;

	return -1;
}

// obscure function that is used to make sure that any of the colors
// passed by the protocol is used as fore- or background color
// in the messagebox. THis is to vvercome limitations in the richedit
// that I do not know currently how to fix

void CheckColorsInModule(const char* pszModule)
{
	MODULEINFO * pMod = pci->MM_FindModule(pszModule);
	int i = 0;
	COLORREF crBG = (COLORREF)db_get_dw(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW));

	COLORREF crFG;
	pci->LoadMsgDlgFont(17, NULL, &crFG);

	if (!pMod)
		return;

	for (i = 0; i < pMod->nColorCount; i++) {
		if (pMod->crColors[i] == crFG || pMod->crColors[i] == crBG) {
			if (pMod->crColors[i] == RGB(255, 255, 255))
				pMod->crColors[i]--;
			else
				pMod->crColors[i]++;
		}
	}
}

const TCHAR* my_strstri(const TCHAR* s1, const TCHAR* s2)
{
	int i, j, k;
	for (i = 0; s1[i]; i++)
		for (j = i, k = 0; _totlower(s1[j]) == _totlower(s2[k]); j++, k++)
			if (!s2[k + 1])
				return s1 + i;

	return NULL;
}

BOOL IsHighlighted(SESSION_INFO *si, const TCHAR* pszText)
{
	if (g_Settings.HighlightEnabled && g_Settings.pszHighlightWords && pszText && si->pMe) {
		TCHAR* p1 = g_Settings.pszHighlightWords;
		TCHAR* p2 = NULL;
		const TCHAR* p3 = pszText;
		static TCHAR szWord1[1000];
		static TCHAR szWord2[1000];
		static TCHAR szTrimString[] = _T(":,.!?;\'>)");

		// compare word for word
		while (*p1 != '\0') {
			// find the next/first word in the highlight word string
			// skip 'spaces' be4 the word
			while (*p1 == ' ' && *p1 != '\0')
				p1 += 1;

			//find the end of the word
			p2 = _tcschr(p1, ' ');
			if (!p2)
				p2 = _tcschr(p1, '\0');
			if (p1 == p2)
				return FALSE;

			// copy the word into szWord1
			lstrcpyn(szWord1, p1, p2 - p1 > 998 ? 999 : p2 - p1 + 1);
			p1 = p2;

			// replace %m with the users nickname
			p2 = _tcschr(szWord1, '%');
			if (p2 && p2[1] == 'm') {
				TCHAR szTemp[50];

				p2[1] = 's';
				lstrcpyn(szTemp, szWord1, 999);
				mir_sntprintf(szWord1, SIZEOF(szWord1), szTemp, si->pMe->pszNick);
			}

			// time to get the next/first word in the incoming text string
			while (*p3 != '\0') {
				// skip 'spaces' be4 the word
				while (*p3 == ' ' && *p3 != '\0')
					p3 += 1;

				//find the end of the word
				p2 = (TCHAR *)_tcschr(p3, ' ');
				if (!p2)
					p2 = (TCHAR *)_tcschr(p3, '\0');


				if (p3 != p2) {
					// eliminate ending character if needed
					if (p2 - p3 > 1 && _tcschr(szTrimString, p2[-1]))
						p2 -= 1;

					// copy the word into szWord2 and remove formatting
					lstrcpyn(szWord2, p3, p2 - p3 > 998 ? 999 : p2 - p3 + 1);

					// reset the pointer if it was touched because of an ending character
					if (*p2 != '\0' && *p2 != ' ')
						p2 += 1;
					p3 = p2;

					CharLower(szWord1);
					CharLower(szWord2);

					// compare the words, using wildcards
					if (wildcmpt(szWord1, RemoveFormatting(szWord2)))
						return TRUE;
				}
			}

			p3 = pszText;
		}
	}

	return FALSE;
}

UINT CreateGCMenu(HWND hwndDlg, HMENU *hMenu, int iIndex, POINT pt, SESSION_INFO *si, TCHAR* pszUID, TCHAR* pszWordText)
{
	GCMENUITEMS gcmi = { 0 };
	HMENU hSubMenu = 0;

	*hMenu = GetSubMenu(g_hMenu, iIndex);
	TranslateMenu(*hMenu);
	gcmi.pszID = si->ptszID;
	gcmi.pszModule = si->pszModule;
	gcmi.pszUID = pszUID;

	if (iIndex == 1) {
		int i = GetRichTextLength(GetDlgItem(hwndDlg, IDC_LOG));

		EnableMenuItem(*hMenu, ID_CLEARLOG, MF_ENABLED);
		EnableMenuItem(*hMenu, ID_COPYALL, MF_ENABLED);
		ModifyMenu(*hMenu, 4, MF_GRAYED | MF_BYPOSITION, 4, NULL);
		if (!i) {
			EnableMenuItem(*hMenu, ID_COPYALL, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(*hMenu, ID_CLEARLOG, MF_BYCOMMAND | MF_GRAYED);
			if (pszWordText && pszWordText[0])
				ModifyMenu(*hMenu, 4, MF_ENABLED | MF_BYPOSITION, 4, NULL);
		}

		if (pszWordText && pszWordText[0]) {
			TCHAR szMenuText[4096];
			mir_sntprintf(szMenuText, 4096, TranslateT("Look up \'%s\':"), pszWordText);
			ModifyMenu(*hMenu, 4, MF_STRING | MF_BYPOSITION, 4, szMenuText);
		}
		else ModifyMenu(*hMenu, 4, MF_STRING | MF_GRAYED | MF_BYPOSITION, 4, TranslateT("No word to look up"));
		gcmi.Type = MENU_ON_LOG;
	}
	else if (iIndex == 0) {
		TCHAR szTemp[50];
		if (pszWordText)
			mir_sntprintf(szTemp, SIZEOF(szTemp), TranslateT("&Message %s"), pszWordText);
		else
			lstrcpyn(szTemp, TranslateT("&Message"), SIZEOF(szTemp) - 1);

		if (lstrlen(szTemp) > 40)
			lstrcpy(szTemp + 40, _T("..."));
		ModifyMenu(*hMenu, ID_MESS, MF_STRING | MF_BYCOMMAND, ID_MESS, szTemp);
		gcmi.Type = MENU_ON_NICKLIST;
	}

	NotifyEventHooks(pci->hBuildMenuEvent, 0, (WPARAM)&gcmi);

	if (gcmi.nItems > 0)
		AppendMenu(*hMenu, MF_SEPARATOR, 0, 0);

	for (int i = 0; i < gcmi.nItems; i++) {
		TCHAR* ptszText = TranslateTS(gcmi.Item[i].pszDesc);
		DWORD dwState = gcmi.Item[i].bDisabled ? MF_GRAYED : 0;

		if (gcmi.Item[i].uType == MENU_NEWPOPUP) {
			hSubMenu = CreateMenu();
			AppendMenu(*hMenu, dwState | MF_POPUP, (UINT_PTR)hSubMenu, ptszText);
		}
		else if (gcmi.Item[i].uType == MENU_POPUPHMENU)
			AppendMenu(hSubMenu == 0 ? *hMenu : hSubMenu, dwState | MF_POPUP, gcmi.Item[i].dwID, ptszText);
		else if (gcmi.Item[i].uType == MENU_POPUPITEM)
			AppendMenu(hSubMenu == 0 ? *hMenu : hSubMenu, dwState | MF_STRING, gcmi.Item[i].dwID, ptszText);
		else if (gcmi.Item[i].uType == MENU_POPUPCHECK)
			AppendMenu(hSubMenu == 0 ? *hMenu : hSubMenu, dwState | MF_CHECKED | MF_STRING, gcmi.Item[i].dwID, ptszText);
		else if (gcmi.Item[i].uType == MENU_POPUPSEPARATOR)
			AppendMenu(hSubMenu == 0 ? *hMenu : hSubMenu, MF_SEPARATOR, 0, ptszText);
		else if (gcmi.Item[i].uType == MENU_SEPARATOR)
			AppendMenu(*hMenu, MF_SEPARATOR, 0, ptszText);
		else if (gcmi.Item[i].uType == MENU_HMENU)
			AppendMenu(*hMenu, dwState | MF_POPUP, gcmi.Item[i].dwID, ptszText);
		else if (gcmi.Item[i].uType == MENU_ITEM)
			AppendMenu(*hMenu, dwState | MF_STRING, gcmi.Item[i].dwID, ptszText);
		else if (gcmi.Item[i].uType == MENU_CHECK)
			AppendMenu(*hMenu, dwState | MF_CHECKED | MF_STRING, gcmi.Item[i].dwID, ptszText);
	}
	return TrackPopupMenu(*hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
}

void DestroyGCMenu(HMENU *hMenu, int iIndex)
{
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_SUBMENU;
	while (GetMenuItemInfo(*hMenu, iIndex, TRUE, &mi)) {
		if (mi.hSubMenu != NULL)
			DestroyMenu(mi.hSubMenu);
		RemoveMenu(*hMenu, iIndex, MF_BYPOSITION);
	}
}

BOOL DoEventHookAsync(HWND hwnd, const TCHAR *pszID, const char* pszModule, int iType, TCHAR* pszUID, TCHAR* pszText, DWORD dwItem)
{
	SESSION_INFO *si = pci->SM_FindSession(pszID, pszModule);
	if (si == NULL)
		return FALSE;

	GCDEST *gcd = (GCDEST*)mir_calloc(sizeof(GCDEST));
	gcd->pszModule = mir_strdup(pszModule);
	gcd->ptszID = mir_tstrdup(pszID);
	gcd->iType = iType;

	GCHOOK *gch = (GCHOOK*)mir_calloc(sizeof(GCHOOK));
	gch->ptszUID = mir_tstrdup(pszUID);
	gch->ptszText = mir_tstrdup(pszText);
	gch->dwData = dwItem;
	gch->pDest = gcd;
	PostMessage(hwnd, GC_FIREHOOK, 0, (LPARAM)gch);
	return TRUE;
}

BOOL DoEventHook(const TCHAR *pszID, const char* pszModule, int iType, const TCHAR* pszUID, const TCHAR* pszText, DWORD dwItem)
{
	SESSION_INFO *si = pci->SM_FindSession(pszID, pszModule);
	if (si == NULL)
		return FALSE;

	GCDEST gcd = { (char*)pszModule, pszID, iType };
	GCHOOK gch = { 0 };
	gch.ptszUID = (LPTSTR)pszUID;
	gch.ptszText = (LPTSTR)pszText;
	gch.dwData = dwItem;
	gch.pDest = &gcd;
	NotifyEventHooks(pci->hSendEvent, 0, (WPARAM)&gch);
	return TRUE;
}

BOOL IsEventSupported(int eventType)
{
	// Supported events
	switch (eventType) {
	case GC_EVENT_JOIN:
	case GC_EVENT_PART:
	case GC_EVENT_QUIT:
	case GC_EVENT_KICK:
	case GC_EVENT_NICK:
	case GC_EVENT_NOTICE:
	case GC_EVENT_MESSAGE:
	case GC_EVENT_TOPIC:
	case GC_EVENT_INFORMATION:
	case GC_EVENT_ACTION:
	case GC_EVENT_ADDSTATUS:
	case GC_EVENT_REMOVESTATUS:
	case GC_EVENT_CHUID:
	case GC_EVENT_CHANGESESSIONAME:
	case GC_EVENT_ADDGROUP:
	case GC_EVENT_SETITEMDATA:
	case GC_EVENT_GETITEMDATA:
	case GC_EVENT_SETSBTEXT:
	case GC_EVENT_ACK:
	case GC_EVENT_SENDMESSAGE:
	case GC_EVENT_SETSTATUSEX:
	case GC_EVENT_CONTROL:
	case GC_EVENT_SETCONTACTSTATUS:
		return TRUE;
	}

	// Other events
	return FALSE;
}

void ValidateFilename(TCHAR *filename)
{
	TCHAR *p1 = filename;
	TCHAR szForbidden[] = _T("\\/:*?\"<>|");
	while (*p1 != '\0') {
		if (_tcschr(szForbidden, *p1))
			*p1 = '_';
		p1 += 1;
	}
}
