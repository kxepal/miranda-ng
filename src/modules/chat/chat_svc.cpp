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

#include "..\..\core\commonheaders.h"

#include "chat.h"

BOOL SmileyAddInstalled, PopupInstalled, IEviewInstalled;

HANDLE            hChatSendEvent;
HANDLE            hBuildMenuEvent;
HGENMENU          hJoinMenuItem, hLeaveMenuItem;
SESSION_INFO		g_TabSession;
CRITICAL_SECTION	cs;

void RegisterFonts( void );

static HANDLE
   hServiceRegister = NULL,
   hServiceNewChat = NULL,
   hServiceAddEvent = NULL,
   hServiceGetAddEventPtr = NULL,
   hServiceGetInfo = NULL,
   hServiceGetCount = NULL,
   hEventPrebuildMenu = NULL,
   hEventDoubleclicked = NULL,
   hEventJoinChat = NULL,
   hEventLeaveChat = NULL;

void ShowRoom(SESSION_INFO *si, WPARAM wp, BOOL bSetForeground)
{
	if (!si)
		return;

	if (ci.pSettings->TabsEnable) {
		// the session is not the current tab, so we copy the necessary
		// details into the SESSION_INFO for the tabbed window
		if (!si->hWnd) {
			g_TabSession.iEventCount = si->iEventCount;
			g_TabSession.iStatusCount = si->iStatusCount;
			g_TabSession.iType = si->iType;
			g_TabSession.nUsersInNicklist = si->nUsersInNicklist;
			g_TabSession.pLog = si->pLog;
			g_TabSession.pLogEnd = si->pLogEnd;
			g_TabSession.pMe = si->pMe;
			g_TabSession.dwFlags = si->dwFlags;
			g_TabSession.pStatuses = si->pStatuses;
			g_TabSession.ptszID = si->ptszID;
			g_TabSession.pszModule = si->pszModule;
			g_TabSession.ptszName = si->ptszName;
			g_TabSession.ptszStatusbarText = si->ptszStatusbarText;
			g_TabSession.ptszTopic = si->ptszTopic;
			g_TabSession.pUsers = si->pUsers;
			g_TabSession.hContact = si->hContact;
			g_TabSession.wStatus = si->wStatus;
			g_TabSession.lpCommands = si->lpCommands;
			g_TabSession.lpCurrentCommand = NULL;
		}

		//Do we need to create a tabbed window?
//		if (g_TabSession.hWnd == NULL) !!!!!!!!!!!!!!!!!!!!!!!
//			g_TabSession.hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_CHANNEL), NULL, RoomWndProc, (LPARAM)&g_TabSession);

		SetWindowLongPtr(g_TabSession.hWnd, GWL_EXSTYLE, GetWindowLongPtr(g_TabSession.hWnd, GWL_EXSTYLE) | WS_EX_APPWINDOW);

		// if the session was not the current tab we need to tell the window to
		// redraw to show the contents of the current SESSION_INFO
		if (!si->hWnd) {
			ci.SM_SetTabbedWindowHwnd(si, g_TabSession.hWnd);
			SendMessage(g_TabSession.hWnd, GC_ADDTAB, -1, (LPARAM)si);
			SendMessage(g_TabSession.hWnd, GC_TABCHANGE, 0, (LPARAM)&g_TabSession);
		}

		ci.SetActiveSession(si->ptszID, si->pszModule);

		if (!IsWindowVisible(g_TabSession.hWnd) || wp == WINDOW_HIDDEN)
			SendMessage(g_TabSession.hWnd, GC_EVENT_CONTROL + WM_USER + 500, wp, 0);
		else {
			if (IsIconic(g_TabSession.hWnd))
				ShowWindow(g_TabSession.hWnd, SW_NORMAL);

			PostMessage(g_TabSession.hWnd, WM_SIZE, 0, 0);
			if (si->iType != GCW_SERVER)
				SendMessage(g_TabSession.hWnd, GC_UPDATENICKLIST, 0, 0);
			else
				SendMessage(g_TabSession.hWnd, GC_UPDATETITLE, 0, 0);
			SendMessage(g_TabSession.hWnd, GC_REDRAWLOG, 0, 0);
			SendMessage(g_TabSession.hWnd, GC_UPDATESTATUSBAR, 0, 0);
			ShowWindow(g_TabSession.hWnd, SW_SHOW);
			if (bSetForeground)
				SetForegroundWindow(g_TabSession.hWnd);
		}
		SendMessage(g_TabSession.hWnd, WM_MOUSEACTIVATE, 0, 0);
		SetFocus(GetDlgItem(g_TabSession.hWnd, IDC_MESSAGE));
		return;
	}

	//Do we need to create a window?
//	if (si->hWnd == NULL) !!!!!!!!!!!!!!!!!!!!!!!s
//		si->hWnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_CHANNEL), NULL, RoomWndProc, (LPARAM)si);

	SetWindowLongPtr(si->hWnd, GWL_EXSTYLE, GetWindowLongPtr(si->hWnd, GWL_EXSTYLE) | WS_EX_APPWINDOW);
	if (!IsWindowVisible(si->hWnd) || wp == WINDOW_HIDDEN)
		SendMessage(si->hWnd, GC_EVENT_CONTROL + WM_USER + 500, wp, 0);
	else {
		if (IsIconic(si->hWnd))
			ShowWindow(si->hWnd, SW_NORMAL);
		ShowWindow(si->hWnd, SW_SHOW);
		SetForegroundWindow(si->hWnd);
	}

	SendMessage(si->hWnd, WM_MOUSEACTIVATE, 0, 0);
	SetFocus(GetDlgItem(si->hWnd, IDC_MESSAGE));
}

/////////////////////////////////////////////////////////////////////////////////////////
// Post-load event hooks

static int FontsChanged(WPARAM wParam, LPARAM lParam)
{
	LoadLogFonts();
	{
		LOGFONT lf;
		HFONT hFont;
		int iText;

		LoadMsgDlgFont(0, &lf, NULL);
		hFont = CreateFontIndirect(&lf);
		iText = GetTextPixelSize(MakeTimeStamp(ci.pSettings->pszTimeStamp, time(NULL)), hFont, TRUE);
		DeleteObject(hFont);
		ci.pSettings->LogTextIndent = iText;
		ci.pSettings->LogTextIndent = ci.pSettings->LogTextIndent * 12 / 10;
		ci.pSettings->LogIndentEnabled = (db_get_b(NULL, "Chat", "LogIndentEnabled", 1) != 0) ? TRUE : FALSE;
	}
	ci.MM_FontsChanged();
	ci.MM_FixColors();
	ci.SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
	return 0;
}

static int IconsChanged(WPARAM wParam, LPARAM lParam)
{
	FreeMsgLogBitmaps();

	LoadMsgLogBitmaps();
	ci.MM_IconsChanged();
	ci.SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, FALSE);
	return 0;
}

static int PreShutdown(WPARAM wParam, LPARAM lParam)
{
	ci.SM_BroadcastMessage(NULL, GC_CLOSEWINDOW, 0, 1, FALSE);

	ci.SM_RemoveAll();
	ci.MM_RemoveAll();
	ci.TabM_RemoveAll();
	return 0;
}

static int SmileyOptionsChanged(WPARAM wParam, LPARAM lParam)
{
	ci.SM_BroadcastMessage(NULL, GC_REDRAWLOG, 0, 1, FALSE);
	return 0;
}

static INT_PTR Service_GetCount(WPARAM wParam, LPARAM lParam)
{
	if (!lParam)
		return -1;

	mir_cslock lck(cs);
	return ci.SM_GetCount((char *)lParam);
}

static INT_PTR Service_GetInfo(WPARAM wParam, LPARAM lParam)
{
	GC_INFO *gci = (GC_INFO *)lParam;
	SESSION_INFO *si = NULL;

	if (!gci || !gci->pszModule)
		return 1;

	mir_cslock lck(cs);

	if (gci->Flags&BYINDEX)
		si = ci.SM_FindSessionByIndex(gci->pszModule, gci->iItem);
	else
		si = ci.SM_FindSession(gci->pszID, gci->pszModule);

	if (si) {
		if (gci->Flags & DATA)     gci->dwItemData = si->dwItemData;
		if (gci->Flags & HCONTACT) gci->hContact = si->hContact;
		if (gci->Flags & TYPE)     gci->iType = si->iType;
		if (gci->Flags & COUNT)    gci->iCount = si->nUsersInNicklist;
		if (gci->Flags & USERS)    gci->pszUsers = ci.SM_GetUsers(si);
		if (gci->Flags & ID)       gci->pszID = si->ptszID;
		if (gci->Flags & NAME)     gci->pszName = si->ptszName;
		return 0;
	}

	return 1;
}

static INT_PTR Service_Register(WPARAM wParam, LPARAM lParam)
{
	GCREGISTER *gcr = (GCREGISTER *)lParam;
	if (gcr == NULL)
		return GC_REGISTER_ERROR;

	if (gcr->cbSize != sizeof(GCREGISTER))
		return GC_REGISTER_WRONGVER;

	mir_cslock lck(cs);
	MODULEINFO *mi = ci.MM_AddModule(gcr->pszModule);
	if (mi == NULL)
		return GC_REGISTER_ERROR;

	mi->ptszModDispName = mir_tstrdup(gcr->ptszDispName);
	mi->bBold = gcr->dwFlags & GC_BOLD;
	mi->bUnderline = gcr->dwFlags & GC_UNDERLINE;
	mi->bItalics = gcr->dwFlags & GC_ITALICS;
	mi->bColor = gcr->dwFlags & GC_COLOR;
	mi->bBkgColor = gcr->dwFlags & GC_BKGCOLOR;
	mi->bAckMsg = gcr->dwFlags & GC_ACKMSG;
	mi->bChanMgr = gcr->dwFlags & GC_CHANMGR;
	mi->iMaxText = gcr->iMaxText;
	mi->nColorCount = gcr->nColors;
	if (gcr->nColors > 0) {
		mi->crColors = (COLORREF *)mir_alloc(sizeof(COLORREF)* gcr->nColors);
		memcpy(mi->crColors, gcr->pColors, sizeof(COLORREF)* gcr->nColors);
	}

	mi->OnlineIconIndex = ImageList_AddIcon(ci.hIconsList, LoadSkinnedProtoIcon(gcr->pszModule, ID_STATUS_ONLINE));
	mi->hOnlineIcon = ImageList_GetIcon(ci.hIconsList, mi->OnlineIconIndex, ILD_TRANSPARENT);

	mi->hOnlineTalkIcon = ImageList_GetIcon(ci.hIconsList, mi->OnlineIconIndex, ILD_TRANSPARENT | INDEXTOOVERLAYMASK(1));
	ImageList_AddIcon(ci.hIconsList, mi->hOnlineTalkIcon);

	mi->OfflineIconIndex = ImageList_AddIcon(ci.hIconsList, LoadSkinnedProtoIcon(gcr->pszModule, ID_STATUS_OFFLINE));
	mi->hOfflineIcon = ImageList_GetIcon(ci.hIconsList, mi->OfflineIconIndex, ILD_TRANSPARENT);

	mi->hOfflineTalkIcon = ImageList_GetIcon(ci.hIconsList, mi->OfflineIconIndex, ILD_TRANSPARENT | INDEXTOOVERLAYMASK(1));
	ImageList_AddIcon(ci.hIconsList, mi->hOfflineTalkIcon);

	mi->pszHeader = Log_CreateRtfHeader(mi);

	CheckColorsInModule((char*)gcr->pszModule);
	CList_SetAllOffline(TRUE, gcr->pszModule);
	return 0;
}

static INT_PTR Service_NewChat(WPARAM wParam, LPARAM lParam)
{
	GCSESSION *gcw = (GCSESSION *)lParam;
	if (gcw == NULL)
		return GC_NEWSESSION_ERROR;

	if (gcw->cbSize != sizeof(GCSESSION))
		return GC_NEWSESSION_WRONGVER;

	mir_cslock lck(cs);
	MODULEINFO* mi = ci.MM_FindModule(gcw->pszModule);
	if (mi == NULL)
		return GC_NEWSESSION_ERROR;

	SESSION_INFO *si = ci.SM_AddSession(gcw->ptszID, gcw->pszModule);

	// create a new session and set the defaults
	if (si != NULL) {
		TCHAR szTemp[256];

		si->dwItemData = gcw->dwItemData;
		if (gcw->iType != GCW_SERVER)
			si->wStatus = ID_STATUS_ONLINE;
		si->iType = gcw->iType;
		si->dwFlags = gcw->dwFlags;
		si->ptszName = mir_tstrdup(gcw->ptszName);
		si->ptszStatusbarText = mir_tstrdup(gcw->ptszStatusbarText);
		si->iSplitterX = ci.pSettings->iSplitterX;
		si->iSplitterY = ci.pSettings->iSplitterY;
		si->iLogFilterFlags = (int)db_get_dw(NULL, "Chat", "FilterFlags", 0x03E0);
		si->bFilterEnabled = db_get_b(NULL, "Chat", "FilterEnabled", 0);
		si->bNicklistEnabled = db_get_b(NULL, "Chat", "ShowNicklist", 1);

		if (mi->bColor) {
			si->iFG = 4;
			si->bFGSet = TRUE;
		}
		if (mi->bBkgColor) {
			si->iBG = 2;
			si->bBGSet = TRUE;
		}
		if (si->iType == GCW_SERVER)
			mir_sntprintf(szTemp, SIZEOF(szTemp), _T("Server: %s"), si->ptszName);
		else
			mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s"), si->ptszName);
		si->hContact = CList_AddRoom(gcw->pszModule, gcw->ptszID, szTemp, si->iType);
		db_set_s(si->hContact, si->pszModule, "Topic", "");
		db_unset(si->hContact, "CList", "StatusMsg");
		if (si->ptszStatusbarText)
			db_set_ts(si->hContact, si->pszModule, "StatusBar", si->ptszStatusbarText);
		else
			db_set_s(si->hContact, si->pszModule, "StatusBar", "");
	}
	else {
		SESSION_INFO* si2 = ci.SM_FindSession(gcw->ptszID, gcw->pszModule);
		if (si2) {
			if (si2->hWnd)
				g_TabSession.nUsersInNicklist = 0;

			ci.UM_RemoveAll(&si2->pUsers);
			ci.TM_RemoveAll(&si2->pStatuses);

			si2->iStatusCount = 0;
			si2->nUsersInNicklist = 0;

			if (!ci.pSettings->TabsEnable) {
				if (si2->hWnd)
					RedrawWindow(GetDlgItem(si2->hWnd, IDC_LIST), NULL, NULL, RDW_INVALIDATE);
			}
			else if (g_TabSession.hWnd)
				RedrawWindow(GetDlgItem(g_TabSession.hWnd, IDC_LIST), NULL, NULL, RDW_INVALIDATE);
		}
	}

	return 0;
}

static int DoControl(GCEVENT *gce, WPARAM wp)
{
	SESSION_INFO *si;

	if (gce->pDest->iType == GC_EVENT_CONTROL) {
		switch (wp) {
		case WINDOW_HIDDEN:
			if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule)) {
				si->bInitDone = TRUE;
				ci.SetActiveSession(si->ptszID, si->pszModule);
				if (si->hWnd)
					ShowRoom(si, wp, FALSE);
			}
			return 0;

		case WINDOW_MINIMIZE:
		case WINDOW_MAXIMIZE:
		case WINDOW_VISIBLE:
		case SESSION_INITDONE:
			if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule)) {
				si->bInitDone = TRUE;
				if (wp != SESSION_INITDONE || db_get_b(NULL, "Chat", "PopupOnJoin", 0) == 0)
					ShowRoom(si, wp, TRUE);
				return 0;
			}
			break;

		case SESSION_OFFLINE:
			ci.SM_SetOffline(gce->pDest->ptszID, gce->pDest->pszModule);
			// fall through

		case SESSION_ONLINE:
			ci.SM_SetStatus(gce->pDest->ptszID, gce->pDest->pszModule, wp == SESSION_ONLINE ? ID_STATUS_ONLINE : ID_STATUS_OFFLINE);
			break;

		case WINDOW_CLEARLOG:
			if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule)) {
				ci.LM_RemoveAll(&si->pLog, &si->pLogEnd);
				if (si->hWnd) {
					g_TabSession.pLog = si->pLog;
					g_TabSession.pLogEnd = si->pLogEnd;
				}
				si->iEventCount = 0;
				si->LastTime = 0;
			}
			break;

		case SESSION_TERMINATE:
			return ci.SM_RemoveSession(gce->pDest->ptszID, gce->pDest->pszModule, (gce->dwFlags & GCEF_REMOVECONTACT) != 0);
		}
		ci.SM_SendMessage(gce->pDest->ptszID, gce->pDest->pszModule, GC_EVENT_CONTROL + WM_USER + 500, wp, 0);
	}

	else if (gce->pDest->iType == GC_EVENT_CHUID && gce->ptszText) {
		ci.SM_ChangeUID(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszNick, gce->ptszText);
	}

	else if (gce->pDest->iType == GC_EVENT_CHANGESESSIONAME && gce->ptszText) {
		if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule)) {
			replaceStrT(si->ptszName, gce->ptszText);
			if (si->hWnd)
				SendMessage(si->hWnd, GC_UPDATETITLE, 0, 0);

			if (g_TabSession.hWnd && ci.pSettings->TabsEnable) {
				g_TabSession.ptszName = si->ptszName;
				SendMessage(g_TabSession.hWnd, GC_SESSIONNAMECHANGE, 0, (LPARAM)si);
			}
		}
	}

	else if (gce->pDest->iType == GC_EVENT_SETITEMDATA) {
		if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule))
			si->dwItemData = gce->dwItemData;
	}

	else if (gce->pDest->iType == GC_EVENT_GETITEMDATA) {
		if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule)) {
			gce->dwItemData = si->dwItemData;
			return si->dwItemData;
		}
		return 0;
	}
	else if (gce->pDest->iType == GC_EVENT_SETSBTEXT) {
		if (si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule)) {
			replaceStrT(si->ptszStatusbarText, gce->ptszText);
			if (si->ptszStatusbarText)
				db_set_ts(si->hContact, si->pszModule, "StatusBar", si->ptszStatusbarText);
			else
				db_set_s(si->hContact, si->pszModule, "StatusBar", "");
			if (si->hWnd) {
				g_TabSession.ptszStatusbarText = si->ptszStatusbarText;
				SendMessage(si->hWnd, GC_UPDATESTATUSBAR, 0, 0);
			}
		}
	}
	else if (gce->pDest->iType == GC_EVENT_ACK) {
		ci.SM_SendMessage(gce->pDest->ptszID, gce->pDest->pszModule, GC_ACKMESSAGE, 0, 0);
	}
	else if (gce->pDest->iType == GC_EVENT_SENDMESSAGE && gce->ptszText) {
		ci.SM_SendUserMessage(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszText);
	}
	else if (gce->pDest->iType == GC_EVENT_SETSTATUSEX) {
		ci.SM_SetStatusEx(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszText, gce->dwItemData);
	}
	else return 1;

	return 0;
}

static void AddUser(GCEVENT *gce)
{
	SESSION_INFO *si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule);
	if (si == NULL) return;

	WORD status = ci.TM_StringToWord(si->pStatuses, gce->ptszStatus);
	USERINFO *ui = ci.SM_AddUser(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszUID, gce->ptszNick, status);
	if (ui == NULL) return;

	ui->pszNick = mir_tstrdup(gce->ptszNick);

	if (gce->bIsMe)
		si->pMe = ui;

	ui->Status = status;
	ui->Status |= si->pStatuses->Status;

	if (si->hWnd) {
		g_TabSession.pUsers = si->pUsers;
		SendMessage(si->hWnd, GC_UPDATENICKLIST, 0, 0);
	}
}

static INT_PTR Service_AddEvent(WPARAM wParam, LPARAM lParam)
{
	GCEVENT *gce = (GCEVENT*)lParam;
	GCDEST *gcd = NULL;
	BOOL bIsHighlighted = FALSE;
	BOOL bRemoveFlag = FALSE;

	if (gce == NULL)
		return GC_EVENT_ERROR;

	gcd = gce->pDest;
	if (gcd == NULL)
		return GC_EVENT_ERROR;

	if (gce->cbSize != sizeof(GCEVENT))
		return GC_EVENT_WRONGVER;

	if (!IsEventSupported(gcd->iType))
		return GC_EVENT_ERROR;

	mir_cslock lck(cs);

	// Do different things according to type of event
	switch (gcd->iType) {
	case GC_EVENT_ADDGROUP:
		{
			STATUSINFO *si = ci.SM_AddStatus(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszStatus);
			if (si && gce->dwItemData)
				si->hIcon = CopyIcon((HICON)gce->dwItemData);
		}
		return 0;

	case GC_EVENT_CHUID:
	case GC_EVENT_CHANGESESSIONAME:
	case GC_EVENT_SETITEMDATA:
	case GC_EVENT_GETITEMDATA:
	case GC_EVENT_CONTROL:
	case GC_EVENT_SETSBTEXT:
	case GC_EVENT_ACK:
	case GC_EVENT_SENDMESSAGE:
	case GC_EVENT_SETSTATUSEX:
		return DoControl(gce, wParam);

	case GC_EVENT_SETCONTACTSTATUS:
		return ci.SM_SetContactStatus(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszUID, (WORD)gce->dwItemData);

	case GC_EVENT_TOPIC:
		{
			SESSION_INFO *si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule);
			if (si) {
				if (gce->ptszText) {
					replaceStrT(si->ptszTopic, gce->ptszText);
					if (si->hWnd)
						g_TabSession.ptszTopic = si->ptszTopic;
					db_set_ts(si->hContact, si->pszModule, "Topic", RemoveFormatting(si->ptszTopic));
					if (db_get_b(NULL, "Chat", "TopicOnClist", 0))
						db_set_ts(si->hContact, "CList", "StatusMsg", RemoveFormatting(si->ptszTopic));
				}
			}
		}
		break;
	
	case GC_EVENT_ADDSTATUS:
		ci.SM_GiveStatus(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszUID, gce->ptszStatus);
		break;

	case GC_EVENT_REMOVESTATUS:
		ci.SM_TakeStatus(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszUID, gce->ptszStatus);
		break;

	case GC_EVENT_MESSAGE:
	case GC_EVENT_ACTION:
		if (!gce->bIsMe && gce->pDest->ptszID && gce->ptszText) {
			SESSION_INFO *si = ci.SM_FindSession(gce->pDest->ptszID, gce->pDest->pszModule);
			if (si)
			if (IsHighlighted(si, gce->ptszText))
				bIsHighlighted = TRUE;
		}
		break;

	case GC_EVENT_NICK:
		ci.SM_ChangeNick(gce->pDest->ptszID, gce->pDest->pszModule, gce);
		break;

	case GC_EVENT_JOIN:
		AddUser(gce);
		break;

	case GC_EVENT_PART:
	case GC_EVENT_QUIT:
	case GC_EVENT_KICK:
		bRemoveFlag = TRUE;
		break;
	}

	// Decide which window (log) should have the event
	LPCTSTR pWnd = NULL;
	LPCSTR pMod = NULL;
	if (gcd->ptszID) {
		pWnd = gcd->ptszID;
		pMod = gcd->pszModule;
	}
	else if (gcd->iType == GC_EVENT_NOTICE || gcd->iType == GC_EVENT_INFORMATION) {
		SESSION_INFO *si = ci.GetActiveSession();
		if (si && !lstrcmpA(si->pszModule, gcd->pszModule)) {
			pWnd = si->ptszID;
			pMod = si->pszModule;
		}
		else return 0;
	}
	else {
		// Send the event to all windows with a user pszUID. Used for broadcasting QUIT etc
		ci.SM_AddEventToAllMatchingUID(gce);
		if (!bRemoveFlag)
			return 0;
	}

	// add to log
	if (pWnd) {
		SESSION_INFO *si = ci.SM_FindSession(pWnd, pMod);

		// fix for IRC's old stuyle mode notifications. Should not affect any other protocol
		if ((gce->pDest->iType == GC_EVENT_ADDSTATUS || gce->pDest->iType == GC_EVENT_REMOVESTATUS) && !(gce->dwFlags & GCEF_ADDTOLOG))
			return 0;

		if (gce && gce->pDest->iType == GC_EVENT_JOIN && gce->time == 0)
			return 0;

		if (si && (si->bInitDone || gce->pDest->iType == GC_EVENT_TOPIC || (gce->pDest->iType == GC_EVENT_JOIN && gce->bIsMe))) {
			if (ci.SM_AddEvent(pWnd, pMod, gce, bIsHighlighted) && si->hWnd) {
				g_TabSession.pLog = si->pLog;
				g_TabSession.pLogEnd = si->pLogEnd;
				SendMessage(si->hWnd, GC_ADDLOG, 0, 0);
			}
			else if (si->hWnd) {
				g_TabSession.pLog = si->pLog;
				g_TabSession.pLogEnd = si->pLogEnd;
				SendMessage(si->hWnd, GC_REDRAWLOG2, 0, 0);
			}
			if (!(gce->dwFlags & GCEF_NOTNOTIFY))
				DoSoundsFlashPopupTrayStuff(si, gce, bIsHighlighted, 0);
			if ((gce->dwFlags & GCEF_ADDTOLOG) && ci.pSettings->LoggingEnabled)
				LogToFile(si, gce);
		}

		if (!bRemoveFlag)
			return 0;
	}

	if (bRemoveFlag)
		return ci.SM_RemoveUser(gce->pDest->ptszID, gce->pDest->pszModule, gce->ptszUID) == 0;

	return GC_EVENT_ERROR;
}

static INT_PTR Service_GetAddEventPtr(WPARAM wParam, LPARAM lParam)
{
	GCPTRS *gp = (GCPTRS *)lParam;

	mir_cslock lck(cs);
	gp->pfnAddEvent = Service_AddEvent;
	return 0;
}

static int ModuleLoad(WPARAM wParam, LPARAM lParam)
{
	IEviewInstalled = ServiceExists(MS_IEVIEW_WINDOW);
	PopupInstalled = ServiceExists(MS_POPUP_ADDPOPUP);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Service creation

static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	char* mods[3] = { "Chat", "ChatFonts" };
	CallService("DBEditorpp/RegisterModule", (WPARAM)mods, 2);

	RegisterFonts();

	CLISTMENUITEM mi = { sizeof(mi) };
	mi.position = -2000090001;
	mi.flags = CMIF_DEFAULT;
	mi.icolibItem = LoadSkinnedIconHandle(SKINICON_CHAT_JOIN);
	mi.pszName = LPGEN("&Join");
	mi.pszService = "GChat/JoinChat";
	hJoinMenuItem = Menu_AddContactMenuItem(&mi);

	mi.position = -2000090000;
	mi.icolibItem = LoadSkinnedIconHandle(SKINICON_CHAT_LEAVE);
	mi.flags = CMIF_NOTOFFLINE;
	mi.pszName = LPGEN("&Leave");
	mi.pszService = "GChat/LeaveChat";
	hLeaveMenuItem = Menu_AddContactMenuItem(&mi);

	HookEvent(ME_FONT_RELOAD, FontsChanged);
	HookEvent(ME_SKIN2_ICONSCHANGED, IconsChanged);

	if (ServiceExists(MS_SMILEYADD_SHOWSELECTION)) {
		SmileyAddInstalled = TRUE;
		HookEvent(ME_SMILEYADD_OPTIONSCHANGED, SmileyOptionsChanged);
	}

	ModuleLoad(0, 0);

	CList_SetAllOffline(TRUE, NULL);
	return 0;
}

void HookEvents(void)
{
	InitializeCriticalSection(&cs);

	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, CList_PrebuildContactMenu);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);
	HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);
	HookEvent(ME_SYSTEM_MODULELOAD, ModuleLoad);
	HookEvent(ME_SYSTEM_MODULEUNLOAD, ModuleLoad);
}

void UnhookEvents(void)
{
	DeleteCriticalSection(&cs);
}

void CreateServiceFunctions(void)
{
	CreateServiceFunction(MS_GC_REGISTER, Service_Register);
	CreateServiceFunction(MS_GC_NEWSESSION, Service_NewChat);
	CreateServiceFunction(MS_GC_EVENT, Service_AddEvent);
	CreateServiceFunction(MS_GC_GETEVENTPTR, Service_GetAddEventPtr);
	CreateServiceFunction(MS_GC_GETINFO, Service_GetInfo);
	CreateServiceFunction(MS_GC_GETSESSIONCOUNT, Service_GetCount);

	CreateServiceFunction("GChat/DblClickEvent", CList_EventDoubleclicked);
	CreateServiceFunction("GChat/PrebuildMenuEvent", CList_PrebuildContactMenuSvc);
	CreateServiceFunction("GChat/JoinChat", CList_JoinChat);
	CreateServiceFunction("GChat/LeaveChat", CList_LeaveChat);
}

void CreateHookableEvents(void)
{
	hChatSendEvent = CreateHookableEvent(ME_GC_EVENT);
	hBuildMenuEvent = CreateHookableEvent(ME_GC_BUILDMENU);
}

void DestroyHookableEvents(void)
{
	DestroyHookableEvent(hChatSendEvent);
	DestroyHookableEvent(hBuildMenuEvent);
}

void TabsInit(void)
{
	ZeroMemory(&g_TabSession, sizeof(SESSION_INFO));

	g_TabSession.iType = GCW_TABROOM;
	g_TabSession.iSplitterX = ci.pSettings->iSplitterX;
	g_TabSession.iSplitterY = ci.pSettings->iSplitterY;
	g_TabSession.iLogFilterFlags = (int)db_get_dw(NULL, "Chat", "FilterFlags", 0x03E0);
	g_TabSession.bFilterEnabled = db_get_b(NULL, "Chat", "FilterEnabled", 0);
	g_TabSession.bNicklistEnabled = db_get_b(NULL, "Chat", "ShowNicklist", 1);
	g_TabSession.iFG = 4;
	g_TabSession.bFGSet = TRUE;
	g_TabSession.iBG = 2;
	g_TabSession.bBGSet = TRUE;
}
