#include "common.h"
#include "updater.h"

#include <m_hotkeys.h>
#include "m_toolbar.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;

HANDLE hNetlibUser, hNetlibHttp;
HANDLE hEventOptInit, hEventModulesLoaded, hEventIdleChanged, hToolBarLoaded;

MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;
LIST_INTERFACE li;
int hLangpack;

bool is_idle = false;
//#define TESTING			// defined here to reduce build time blowout caused by changing common.h

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESC,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,		//not transient
	0,						//doesn't replace anything built-in
	{ 0x66dceb80, 0x384, 0x4507, { 0x97, 0x74, 0xcc, 0x20, 0xa7, 0xef, 0x1d, 0x6d } } // {66DCEB80-0384-4507-9774-CC20A7EF1D6D}
};

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_UPDATER, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}



int IdleChanged(WPARAM wParam, LPARAM lParam) {

	is_idle = (lParam & IDF_ISIDLE);

	return 0;
}

void InitNetlib() {
	NETLIBUSER nl_user = {0};
	nl_user.cbSize = sizeof(nl_user);
	nl_user.szSettingsModule = MODULE;
	nl_user.flags = NUF_OUTGOING | NUF_HTTPCONNS | NUF_TCHAR;
	nl_user.ptszDescriptiveName = TranslateT("Updater connection");

	hNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nl_user);
}

int ModulesLoaded(WPARAM wParam, LPARAM lParam) {

	LoadOptions();

	InitOptionsMenuItems();
	
	InitNetlib();
	InitPopups();

#ifdef USE_MY_SERVER
	Update update = {0};
	char szVersion[16];
	update.cbSize = sizeof(Update);

	update.szComponentName = pluginInfo.shortName;
	update.pbVersion = (BYTE *)CreateVersionString(&pluginInfo, szVersion);
	update.cpbVersion = strlen((char *)update.pbVersion);


	update.szUpdateURL = BETA_HOST_URL_PREFIX "/ver_updater_unicode.zip";
	update.szVersionURL = BETA_HOST_URL_PREFIX "/updater_unicode.html";
	update.pbVersionPrefix = (BYTE *)"Updater (Unicode) version ";
	update.cpbVersionPrefix = strlen((char *)update.pbVersionPrefix);


	CallService(MS_UPDATE_REGISTER, 0, (WPARAM)&update);

#else //!USE_MY_SERVER

#ifdef REGISTER_BETA

	Update update = {0};
	char szVersion[16];
	update.cbSize = sizeof(Update);

	update.szComponentName = pluginInfo.shortName;
	update.pbVersion = (BYTE *)CreateVersionStringPluginEx(&pluginInfo, szVersion);
	update.cpbVersion = (int)strlen((char *)update.pbVersion);
	update.szBetaChangelogURL = "https://server.scottellis.com.au/wsvn/mim_plugs/updater/?op=log&rev=0&sc=0&isdir=1";

	


#ifdef REGISTER_AUTO
		update.szUpdateURL = UPDATER_AUTOREGISTER;
#else //!REGISTER_AUTO
		update.szUpdateURL = MIM_DOWNLOAD_URL_PREFIX "2596";
		update.szVersionURL = MIM_VIEW_URL_PREFIX "2596";
		update.pbVersionPrefix = (BYTE *)"<span class=\"fileNameHeader\">Updater (Unicode) ";
		update.cpbVersionPrefix = strlen((char *)update.pbVersionPrefix);
#endif //REGISTER_AUTO

#ifdef _WIN64
		update.szBetaUpdateURL = BETA_HOST_URL_PREFIX "/updater_x64.zip";
#else
		update.szBetaUpdateURL = BETA_HOST_URL_PREFIX "/updater_unicode.zip";
#endif
		update.szBetaVersionURL = BETA_HOST_URL_PREFIX "/ver_updater_unicode.html";
		update.pbBetaVersionPrefix = (BYTE *)"Updater (Unicode) version ";
		update.cpbBetaVersionPrefix = (int)strlen((char *)update.pbBetaVersionPrefix);


	CallService(MS_UPDATE_REGISTER, 0, (WPARAM)&update);
#else // !REGISTER_BETA


	CallService(MS_UPDATE_REGISTERFL, (WPARAM)2596, (LPARAM)&pluginInfo);


#endif // REGISTER_BETA

#endif // USE_MY_SERVER

	hEventIdleChanged = HookEvent(ME_IDLE_CHANGED, IdleChanged);

	if (ServiceExists(MS_TRIGGER_REGISTERACTION)) 
	{
		// create update action for triggerplugin
		ACTIONREGISTER ar = {0};
		ar.cbSize = sizeof(ACTIONREGISTER);
		ar.pszName = Translate("Check for Plugin Updates");
		ar.pszService = MS_UPDATE_CHECKFORUPDATESTRGR;

		CallService(MS_TRIGGER_REGISTERACTION, 0, (LPARAM)&ar);
	}

	// register hotkeys
	HOTKEYDESC shk = {0};
	shk.cbSize = sizeof(shk);
	shk.pszSection = LPGEN("Updater");

	shk.pszDescription = LPGEN("Check for Updates");
	shk.pszName = "Update";
	shk.pszService = MS_UPDATE_CHECKFORUPDATES;
	shk.DefHotKey = HOTKEYCODE(HOTKEYF_ALT, 'U') | HKF_MIRANDA_LOCAL;
	Hotkey_Register(&shk);	

	shk.pszDescription = LPGEN("Restart");
	shk.pszName = "Restart";
	shk.pszService = MS_UPDATE_MENURESTART;
	shk.DefHotKey = HOTKEYCODE(HOTKEYF_ALT, 'R') | HKF_MIRANDA_LOCAL;
	Hotkey_Register(&shk);	

	shk.pszDescription = LPGEN("Update and Exit");
	shk.pszName = "UpdateAndExit";
	shk.pszService = MS_UPDATE_MENUUPDATEANDEXIT;
	shk.DefHotKey = 0;
	Hotkey_Register(&shk);	

	return 0;
}


static int ToolbarModulesLoaded(WPARAM, LPARAM)
{
	TBButton tbb = {0};
	tbb.cbSize = sizeof(TBButton);
	tbb.tbbFlags = TBBF_SHOWTOOLTIP;

	tbb.pszButtonID = "updater_checkforupdates";
	tbb.pszButtonName = LPGEN("Check for Updates");
	tbb.pszServiceName = MS_UPDATE_CHECKFORUPDATES;
	tbb.pszTooltipUp = LPGEN("Check for Updates of Plugins");
	tbb.hPrimaryIconHandle = GetIconHandle(I_CHKUPD);
	tbb.defPos = 1000;
	CallService(MS_TB_ADDBUTTON, 0, (LPARAM)&tbb);

	tbb.pszButtonID = "updater_restart";
	tbb.pszButtonName = LPGEN("Restart");
	tbb.pszServiceName = MS_UPDATE_MENURESTART;
	tbb.pszTooltipUp = LPGEN("Restart Miranda IM");
	tbb.hPrimaryIconHandle = GetIconHandle(I_RSTRT);
	tbb.defPos = 1001;
	CallService(MS_TB_ADDBUTTON, 0, (LPARAM)&tbb);

	tbb.pszButtonID = "updater_updateandexit";
	tbb.pszButtonName = LPGEN("Update and Exit");
	tbb.pszServiceName = MS_UPDATE_MENUUPDATEANDEXIT;
	tbb.pszTooltipUp = LPGEN("Update and Exit Miranda IM");
	tbb.hPrimaryIconHandle = GetIconHandle(I_CHKUPDEXT);
	tbb.defPos = 1002;
	CallService(MS_TB_ADDBUTTON,0, (LPARAM)&tbb);

	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;

	mir_getLI(&li);
	mir_getMMI(&mmi);
	mir_getUTFI(&utfi);
	mir_getLP(&pluginInfo);

	// save global status from clist - will be restored after update check if that option is enabled, or in modules loaded if not
	options.start_offline = (DBGetContactSettingByte(0, MODULE, "StartOffline", 0) == 1); // load option here - rest loading in modulesloaded
	if (options.start_offline) 
	{
		WORD saved_status = DBGetContactSettingWord(0, "CList", "Status", ID_STATUS_OFFLINE);
		if (saved_status != ID_STATUS_OFFLINE) 
		{
			DBWriteContactSettingWord(0, MODULE, "SavedGlobalStatus", saved_status);
			DBWriteContactSettingWord(0, "CList", "Status", ID_STATUS_OFFLINE);
		}
	}

	hEventOptInit = HookEvent(ME_OPT_INITIALISE, OptInit);
	hEventModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded); 
	hToolBarLoaded = HookEvent(ME_TB_MODULELOADED, ToolbarModulesLoaded);

	InitServices();
	InitIcons();

	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	UnhookEvent(hEventIdleChanged);
	UnhookEvent(hEventOptInit);
	UnhookEvent(hEventModulesLoaded);
	UnhookEvent(hToolBarLoaded);

	DeinitServices();
	DeinitPopups();

	Netlib_CloseHandle(hNetlibUser);

	return 0;
}
