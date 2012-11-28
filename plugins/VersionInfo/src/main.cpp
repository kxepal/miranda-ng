/*
Version information plugin for Miranda IM

Copyright � 2002-2006 Luca Santarelli, Cristian Libotean

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

#include "common.h"

#include "CVersionInfo.h"

HINSTANCE hInst;

int hLangpack;

HICON hiVIIcon;

DWORD EnglishLocale;

BOOL bFoldersAvailable = FALSE;
HANDLE hOutputLocation = NULL; //for folders plugin

void * (* MirandaMalloc)(size_t);
void * (* MirandaRealloc)(void *, size_t);
void (* MirandaFree)(void *);

char ModuleName[] = "VersionInfo";

#ifdef _DEBUG
	BOOL verbose = FALSE;//TRUE;
#else
	BOOL verbose = FALSE;
#endif

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	__PLUGIN_DISPLAY_NAME,
	VERSION,
	__DESC,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {2f174488-489c-4fe1-940d-596cf0f35e65}
	{0x2f174488, 0x489c, 0x4fe1, {0x94, 0x0d, 0x59, 0x6c, 0xf0, 0xf3, 0x5e, 0x65}}
};

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_VERSIONINFO, MIID_SERVICEMODE, MIID_LAST};

bool WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	if (fdwReason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(hinstDLL);
	EnglishLocale = MAKELCID(MAKELANGID(0x09, 0x01), SORT_DEFAULT);	//create our english locale and use it everywhere it's needed
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP(&pluginInfo);

	InitServices();
	HookEvents();
	
	hiVIIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_MAIN));
	
	//get the name of the dll itself
	TCHAR filePath[512] = {0};
	GetModuleFileName(hInst, filePath, SIZEOF(filePath));
	TCHAR *fileName = NULL;
	size_t i = _tcslen(filePath) - 1;
	_tcslwr(filePath);

	//Menu item
	CLISTMENUITEM mi = { sizeof(mi) };
	mi.position = mi.popupPosition = 2000089999;
	mi.flags = 0;
	mi.hIcon = hiVIIcon;
	mi.pszName = Translate("Version Information");
	mi.pszService = MS_VERSIONINFO_MENU_COMMAND;
	Menu_AddMainMenuItem(&mi);

	if (LoadLibraryA("RichEd32.dll") == NULL)
		MessageBoxA(NULL, "d'oh", "d'oh", MB_OK);
	
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	UnhookEvents();
	DestroyServices();
	return 0;
}