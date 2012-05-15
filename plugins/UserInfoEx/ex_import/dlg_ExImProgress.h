/*
UserinfoEx plugin for Miranda IM

Copyright:
� 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

===============================================================================

File name      : $HeadURL: https://userinfoex.googlecode.com/svn/trunk/ex_import/dlg_ExImProgress.h $
Revision       : $Revision: 187 $
Last change on : $Date: 2010-09-08 16:05:54 +0400 (Ср, 08 сен 2010) $
Last change by : $Author: ing.u.horn $

===============================================================================
*/

#ifndef _DLG_EXIMPROGRESS_H_
#define _DLG_EXIMPROGRESS_H_

#pragma once

class CProgress
{
	HWND	_hDlg;
	DWORD	_dwStartTime;

	BOOLEAN	Update();

public:
	CProgress();
	~CProgress();

	VOID Hide();

	VOID SetContactCount(DWORD numContacts);
	VOID SetSettingsCount(DWORD numSettings);
	
	BOOLEAN UpdateContact(LPCTSTR pszFormat, ...);
	BOOLEAN UpdateSetting(LPCTSTR pszFormat, ...);
};

#endif /* _DLG_EXIMPROGRESS_H_ */
