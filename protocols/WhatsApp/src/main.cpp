#include "common.h"

JSONSERVICEINTERFACE jsi;
CLIST_INTERFACE* pcli;
int hLangpack;

HINSTANCE g_hInstance;
std::string g_strUserAgent;
DWORD g_mirandaVersion;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"WhatsApp Protocol",
	__VERSION_DWORD,
	"Provides basic support for WhatsApp. [Built: "__DATE__" "__TIME__"]",
	"Uli Hecht",
	"uli.hecht@gmail.com",
	"� 2013 Uli Hecht",
	"http://example.com",
	UNICODE_AWARE, //not transient
	// {4f1ff7fa-4d75-44b9-93b0-2ced2e4f9e3e}
	{ 0x4f1ff7fa, 0x4d75, 0x44b9, { 0x93, 0xb0, 0x2c, 0xed, 0x2e, 0x4f, 0x9e, 0x3e } }

};


/////////////////////////////////////////////////////////////////////////////
// Protocol instances
static int compare_protos(const WhatsAppProto *p1, const WhatsAppProto *p2)
{
	return _tcscmp(p1->m_tszUserName, p2->m_tszUserName);
}

OBJLIST<WhatsAppProto> g_Instances(1, compare_protos);

DWORD WINAPI DllMain(HINSTANCE hInstance,DWORD,LPVOID)
{
	g_hInstance = hInstance;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	g_mirandaVersion = mirandaVersion;
	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Interface information

extern "C" __declspec(dllexport) const MUUID MirandaInterfaces[] = {MIID_PROTOCOL, MIID_LAST};

/////////////////////////////////////////////////////////////////////////////////////////
// Load

static PROTO_INTERFACE* protoInit(const char *proto_name,const TCHAR *username )
{
	WhatsAppProto *proto = new WhatsAppProto(proto_name,username);
	g_Instances.insert(proto);
	return proto;
}

static int protoUninit(PROTO_INTERFACE* proto)
{
	g_Instances.remove(( WhatsAppProto* )proto);
	return EXIT_SUCCESS;
}

static HANDLE g_hEvents[1];

extern "C" int __declspec(dllexport) Load(void)
{
	mir_getLP(&pluginInfo);
	mir_getCLI();
	json_getJSI(&jsi);

	PROTOCOLDESCRIPTOR pd = { sizeof(pd) };
	pd.szName = "WhatsApp";
	pd.type = PROTOTYPE_PROTOCOL;
	pd.fnInit = protoInit;
	pd.fnUninit = protoUninit;
	CallService(MS_PROTO_REGISTERMODULE,0,reinterpret_cast<LPARAM>(&pd));
	
	InitIcons();
	//InitContactMenus();

	// Init native User-Agent
	{
		std::stringstream agent;
//		DWORD mir_ver = ( DWORD )CallService( MS_SYSTEM_GETVERSION, NULL, NULL );
		agent << "MirandaNG/";
		agent << (( g_mirandaVersion >> 24) & 0xFF);
		agent << ".";
		agent << (( g_mirandaVersion >> 16) & 0xFF);
		agent << ".";
		agent << (( g_mirandaVersion >>  8) & 0xFF);
		agent << ".";
		agent << (( g_mirandaVersion		) & 0xFF);
	#ifdef _WIN64
		agent << " WhatsApp Protocol x64/";
	#else
		agent << " WhatsApp Protocol/";
	#endif
		agent << __VERSION_STRING;
		g_strUserAgent = agent.str( );
	}

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload

extern "C" int __declspec(dllexport) Unload(void)
{
	//UninitContactMenus();
	for(size_t i=0; i<SIZEOF(g_hEvents); i++)
		UnhookEvent(g_hEvents[i]);

	g_Instances.destroy();

	delete FMessage::generating_lock;
	WASocketConnection::quitNetwork();

	return 0;
}
