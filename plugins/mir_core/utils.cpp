/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
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

#include "commonheaders.h"

MIR_CORE_DLL(char*) rtrim(char* str)
{
	if (str == NULL)
		return NULL;

	char* p = strchr(str, 0);
	while (--p >= str) {
		switch (*p) {
		case ' ': case '\t': case '\n': case '\r':
			*p = 0; break;
		default:
			return str;
		}
	}
	return str;
}

MIR_CORE_DLL(WCHAR*) wrtrim(WCHAR *str)
{
	if (str == NULL)
		return NULL;

	WCHAR* p = _tcschr(str, 0);
	while (--p >= str) {
		switch (*p) {
		case ' ': case '\t': case '\n': case '\r':
			*p = 0; break;
		default:
			return str;
		}
	}
	return str;
}

MIR_CORE_DLL(char*) ltrim(char* str)
{
	if (str == NULL)
		return NULL;

	char* p = str;
	for (;;) {
		switch (*p) {
		case ' ': case '\t': case '\n': case '\r':
			++p; break;
		default:
			memmove(str, p, strlen(p) + 1);
			return str;
		}
	}
}

MIR_CORE_DLL(char*) ltrimp(char* str)
{
	if (str == NULL)
		return NULL;

	char* p = str;
	for (;;) {
		switch (*p) {
		case ' ': case '\t': case '\n': case '\r':
			++p; break;
		default:
			return p;
		}
	}
}

MIR_CORE_DLL(int) wildcmp(char * name, char * mask)
{
	char * last='\0';
	for (;; mask++, name++) {
		if (*mask != '?' && *mask != *name) break;
		if (*name == '\0') return ((BOOL)!*mask);
	}
	if (*mask != '*') return FALSE;
	for (;; mask++, name++){
		while (*mask == '*') {
			last = mask++;
			if (*mask == '\0') return ((BOOL)!*mask);   /* true */
		}
		if (*name == '\0') return ((BOOL)!*mask);      /* *mask == EOS */
		if (*mask != '?' && *mask != *name) name -= (size_t)(mask - last) - 1, mask = last;
	}
}

///////////////////////////////////////////////////////////////////////////////

static int sttComparePlugins(const HINSTANCE__* p1, const HINSTANCE__* p2)
{
	if (p1 == p2)
		return 0;

	return (p1 < p2) ? -1 : 1;
}

static LIST<HINSTANCE__> pluginListAddr(10, sttComparePlugins);

MIR_CORE_DLL(void) RegisterModule(HINSTANCE hInst)
{
	pluginListAddr.insert(hInst);
}

MIR_CORE_DLL(void) UnregisterModule(HINSTANCE hInst)
{
	pluginListAddr.remove(hInst);
}

MIR_CORE_DLL(HINSTANCE) GetInstByAddress(void* codePtr)
{
	if (pluginListAddr.getCount() == 0)
		return NULL;

	int idx;
	List_GetIndex((SortedList*)&pluginListAddr, &codePtr, &idx);
	if (idx > 0)
		idx--;

	HINSTANCE result = pluginListAddr[idx];
	if (result < hInst && codePtr > hInst)
		result = hInst;
	else if (idx == 0 && codePtr < (void*)result)
		result = NULL;

	return result;
}
