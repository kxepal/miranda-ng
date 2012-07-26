/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright 2012 Miranda NG project,
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

int hLangpack;

static PLUGININFOEX pluginInfo =
{
	sizeof(PLUGININFOEX),
	"Miranda NG mmap database driver",
	__VERSION_DWORD,
	"Provides Miranda database support: global settings, contacts, history, settings per contact.",
	"Miranda-NG project",
	"bio@msx.ru; ghazan@miranda.im",
	"Copyright 2012 Miranda NG project",
	"",
	UNICODE_AWARE,
    {0xf7a6b27c, 0x9d9c, 0x4a42, { 0xbe, 0x86, 0xa4, 0x48, 0xae, 0x10, 0x91, 0x61 }} //{F7A6B27C-9D9C-4a42-BE86-A448AE109161}
};

HINSTANCE g_hInst = NULL;

LIST<CDb3Mmap> g_Dbs(1, (LIST<CDb3Mmap>::FTSortFunc)HandleKeySort);

/////////////////////////////////////////////////////////////////////////////////////////

// returns 0 if the profile is created, EMKPRF*
static int makeDatabase(const TCHAR *profile, int *error)
{
	CDb3Mmap *tmp = new CDb3Mmap(profile);
	if (tmp->Create() == ERROR_SUCCESS) {
		tmp->CreateDbHeaders();
		delete tmp;
		return 0;
	}
	delete tmp;
	if (error != NULL) *error = EMKPRF_CREATEFAILED;
	return 1;
}

// returns 0 if the given profile has a valid header
static int grokHeader(const TCHAR *profile, int *error)
{
	CDb3Mmap *tmp = new CDb3Mmap(profile);
	if (tmp->Load(true) != ERROR_SUCCESS) {
		delete tmp;
		if (error != NULL) *error = EGROKPRF_CANTREAD;
		return 1;
	}

	int chk = tmp->CheckDbHeaders();
	delete tmp;
	if ( chk == 0 ) {
		// all the internal tests passed, hurrah
		if (error != NULL) *error = 0;
		return 0;
	}
	
	// didn't pass at all, or some did.
	switch ( chk ) {
	case 1:
		// "Miranda ICQ DB" wasn't present
		if (error != NULL) *error = EGROKPRF_UNKHEADER;
		break;

	case 2:
		// header was present, but version information newer
		if (error != NULL) *error =  EGROKPRF_VERNEWER;
		break;

	case 3:
		// header/version OK, internal data missing
		if (error != NULL) *error = EGROKPRF_DAMAGED;
		break;
	}

	return 1;
}

// returns 0 if all the APIs are injected otherwise, 1
static MIDatabase* LoadDatabase(const TCHAR *profile)
{
	// set the memory, lists & UTF8 manager
	mir_getLP( &pluginInfo );

	CDb3Mmap* db = new CDb3Mmap(profile);
	if (db->Load(false) != ERROR_SUCCESS) {
		delete db;
		return NULL;
	}

	g_Dbs.insert(db);
	return db;
}

static int UnloadDatabase(MIDatabase* db)
{
	g_Dbs.remove((CDb3Mmap*)db);
	delete (CDb3Mmap*)db;
	return 0;
}
/*
static CheckWorker Workers[6] = 
{
	&CDb3Mmap::WorkInitialChecks,
	&CDb3Mmap::WorkModuleChain,
	&CDb3Mmap::WorkUser,
	&CDb3Mmap::WorkContactChain,
	&CDb3Mmap::WorkAggressive,
	&CDb3Mmap::WorkFinalTasks
};

static int CheckDb(DBCHeckCallback *callback, int phase, int firstTime)
{
	if (phase >= SIZEOF(Workers))
		return ERROR_NO_MORE_ITEMS;

	CDb3Mmap* db = (CDb3Mmap*)callback->db;
	return (db->*Workers[phase])(callback, firstTime);
}
*/
static DATABASELINK dblink =
{
	sizeof(DATABASELINK),
	"db3x mmap driver",
	_T("db3x mmap database support"),
	makeDatabase,
	grokHeader,
	LoadDatabase,
	UnloadDatabase,
	//CheckDb
};

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_DATABASE, MIID_LAST};

extern "C" __declspec(dllexport) int Load(void)
{
	RegisterDatabasePlugin(&dblink);
	return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
	g_Dbs.destroy();
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
	g_hInst = hInstDLL;
	return TRUE;
}
