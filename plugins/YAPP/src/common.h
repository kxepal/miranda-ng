// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <commctrl.h>
#include <time.h>
#include <malloc.h>

#include <newpluginapi.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_skin.h>
#include <m_clui.h>
#include <m_clist.h>
#include <m_fontservice.h>
#include <m_avatars.h>
#include <m_popup.h>
#include <m_icolib.h>
#include <win2k.h>

#include <m_notify.h>
#include <m_yapp.h>
#include <m_ieview.h> //need this for special renderers

#include "version.h"
#include "message_pump.h"
#include "options.h"
#include "popwin.h"
#include "notify.h"
#include "services.h"
#include "resource.h"
#include "yapp_history.h"

#define MODULE			"YAPP"

extern HMODULE hInst;

extern HFONT hFontFirstLine, hFontSecondLine, hFontTime;
extern COLORREF	colFirstLine, colSecondLine, colBg, colTime, colBorder, colSidebar, colTitleUnderline;

extern MNOTIFYLINK *notifyLink;

// work around a bug in neweventnotify, possibly httpserver
// ignore the address passed to the 'get plugin data' service
extern bool ignore_gpd_passed_addy;
 
// win32 defines for mingw version of windows headers :(
#ifndef LVM_SORTITEMSEX
#define LVM_SORTITEMSEX          (LVM_FIRST + 81)

typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);

#define ListView_SortItemsEx(hwndLV, _pfnCompare, _lPrm) \
  (BOOL)SendMessage((hwndLV), LVM_SORTITEMSEX, (WPARAM)(LPARAM)(_lPrm), (LPARAM)(PFNLVCOMPARE)(_pfnCompare))
#endif


#define PDF_UNICODE  0x0001
#define PDF_ICOLIB   0x0002

#define PDF_TCHAR    PDF_UNICODE

// windowProc messages
#define PM_INIT			(WM_USER + 0x0202)				// message sent to your windowProc after the window has been initialized
#define PM_DIENOTIFY	(WM_USER + 0x0200)				// message sent to your windowProc just before the window is destroyed (can be used e.g. to free your opaque data)

#define PM_DESTROY		(WM_USER + 0x0201)				// send to the popup hWnd (use PostMessage generally, or SendMessage inside your windowProc) to kill it

void ShowPopup(PopupData &pd_in);