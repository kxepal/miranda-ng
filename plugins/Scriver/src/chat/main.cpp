/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson
Copyright 2003-2009 Miranda ICQ/IM project,

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

#include "../commonheaders.h"

void RegisterChatFonts( void );

//globals
CHAT_MANAGER *pci;
HMENU g_hMenu = NULL;

HBRUSH hListBkgBrush = NULL;
HBRUSH hListSelectedBkgBrush = NULL;

GlobalLogSettings g_Settings;

static void OnAddLog(SESSION_INFO *si, int isOk)
{
	if (isOk && si->hWnd)
		SendMessage(si->hWnd, GC_ADDLOG, 0, 0);
	else if (si->hWnd)
		SendMessage(si->hWnd, GC_REDRAWLOG2, 0, 0);
}

static void OnSessionDblClick(SESSION_INFO *si)
{
	PostMessage(si->hWnd, GC_CLOSEWINDOW, 0, 0);
}

static void OnSessionRemove(SESSION_INFO *si)
{
	if (si->hWnd)
		SendMessage(si->hWnd, GC_EVENT_CONTROL + WM_USER + 500, SESSION_TERMINATE, 0);
}

static void OnSessionRename(SESSION_INFO *si)
{
	if (si->hWnd)
		SendMessage(si->hWnd, DM_UPDATETITLEBAR, 0, 0);
}

static void OnSessionReplace(SESSION_INFO *si)
{
	if (si->hWnd)
		RedrawWindow(GetDlgItem(si->hWnd, IDC_CHAT_LIST), NULL, NULL, RDW_INVALIDATE);
}

static void OnEventBroadcast(SESSION_INFO *si, GCEVENT *gce)
{
	if (pci->SM_AddEvent(si->ptszID, si->pszModule, gce, FALSE) && si->hWnd && si->bInitDone)
		SendMessage(si->hWnd, GC_ADDLOG, 0, 0);
	else if (si->hWnd && si->bInitDone)
		SendMessage(si->hWnd, GC_REDRAWLOG2, 0, 0);
}

static void OnSetStatusBar(SESSION_INFO *si)
{
	if (si->hWnd)
		SendMessage(si->hWnd, DM_UPDATETITLEBAR, 0, 0);
}

static void OnNewUser(SESSION_INFO *si, USERINFO*)
{
	if (si->hWnd)
		SendMessage(si->hWnd, GC_UPDATENICKLIST, 0, 0);
}

static void OnSetStatus(SESSION_INFO *si, int wStatus)
{
	PostMessage(si->hWnd, GC_FIXTABICONS, 0, 0);
}

static void OnLoadSettings()
{
	if (hListBkgBrush != NULL)
		DeleteObject(hListBkgBrush);
	hListBkgBrush = CreateSolidBrush(db_get_dw(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));

	if (hListSelectedBkgBrush != NULL)
		DeleteObject(hListSelectedBkgBrush);
	hListSelectedBkgBrush = CreateSolidBrush(db_get_dw(NULL, "Chat", "ColorNicklistSelectedBG", GetSysColor(COLOR_HIGHLIGHT)));
}

static void OnFlashWindow(SESSION_INFO *si, int bInactive)
{
	if (bInactive && si->hWnd && db_get_b(NULL, "Chat", "FlashWindowHighlight", 0) != 0)
		SendMessage(GetParent(si->hWnd), CM_STARTFLASHING, 0, 0);
	if (bInactive && si->hWnd)
		SendMessage(si->hWnd, GC_SETMESSAGEHIGHLIGHT, 0, 0);
}

int Chat_Load()
{
	mir_getCI(&g_Settings);
	pci->cbModuleInfo = sizeof(MODULEINFO);
	pci->cbSession = sizeof(SESSION_INFO);
	pci->OnNewUser = OnNewUser;

	pci->OnSetStatus = OnSetStatus;

	pci->OnAddLog = OnAddLog;
	pci->OnLoadSettings = OnLoadSettings;

	pci->OnSessionRemove = OnSessionRemove;
	pci->OnSessionRename = OnSessionRename;
	pci->OnSessionReplace = OnSessionReplace;
	pci->OnSessionDblClick = OnSessionDblClick;

	pci->OnEventBroadcast = OnEventBroadcast;
	pci->OnSetStatusBar = OnSetStatusBar;
	pci->OnFlashWindow = OnFlashWindow;
	pci->ShowRoom = ShowRoom;

	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU));
	TranslateMenu(g_hMenu);
	return 0;
}

int Chat_Unload(void)
{
	db_set_w(NULL, "Chat", "SplitterX", (WORD)g_Settings.iSplitterX);

	DestroyMenu(g_hMenu);
	OptionsUnInit();
	return 0;
}

int Chat_ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	OptionsInit();
 	return 0;
}
