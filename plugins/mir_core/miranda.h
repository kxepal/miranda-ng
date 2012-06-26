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

#define NEWSTR_ALLOCA(A) (A == NULL)?NULL:strcpy((char*)alloca(strlen(A)+1), A)
#define NEWTSTR_ALLOCA(A) (A == NULL)?NULL:_tcscpy((TCHAR*)alloca((_tcslen(A)+1)* sizeof(TCHAR)), A)

struct LangPackMuuid
{
	MUUID muuid;
	PLUGININFOEX* pInfo;
};

MIR_CORE_DLL(int) LangPackMarkPluginLoaded(PLUGININFOEX* pInfo);

MIR_CORE_DLL(LangPackMuuid*) LangPackLookupUuid(WPARAM wParam);

int LoadLangPackModule(void);
void UnloadLangPackModule(void);

int InitialiseModularEngine(void);
void DestroyModularEngine(void);

int InitPathUtils(void);

extern HINSTANCE hInst;

/**** modules.cpp **********************************************************************/

struct THookSubscriber
{
	HINSTANCE hOwner;
	int type;
	union {
		struct {
			union {
				MIRANDAHOOK pfnHook;
				MIRANDAHOOKPARAM pfnHookParam;
				MIRANDAHOOKOBJ pfnHookObj;
				MIRANDAHOOKOBJPARAM pfnHookObjParam;
			};
			void* object;
			LPARAM lParam;
		};
		struct {
			HWND hwnd;
			UINT message;
		};
	};
};

struct THook
{
	char name[ MAXMODULELABELLENGTH ];
	int  id;
	int  subscriberCount;
	THookSubscriber* subscriber;
	MIRANDAHOOK pfnHook;
	CRITICAL_SECTION csHook;
};

/**** langpack.cpp *********************************************************************/

char*  LangPackTranslateString(struct LangPackMuuid* pUuid, const char *szEnglish, const int W);
TCHAR* LangPackTranslateStringT(int hLangpack, const TCHAR* tszEnglish);

/**** options.cpp **********************************************************************/

HTREEITEM FindNamedTreeItemAtRoot(HWND hwndTree, const TCHAR* name);

/**** utils.cpp ************************************************************************/

void HotkeyToName(TCHAR *buf, int size, BYTE shift, BYTE key);
WORD GetHotkeyValue(INT_PTR idHotkey);

HBITMAP ConvertIconToBitmap(HICON hIcon, HIMAGELIST hIml, int iconId);

class StrConvUT
{
private:
	wchar_t* m_body;

public:
	StrConvUT(const char* pSrc) :
		m_body(mir_a2u(pSrc)) {}

    ~StrConvUT() {  mir_free(m_body); }
	operator const wchar_t* () const { return m_body; }
};

class StrConvAT
{
private:
	char* m_body;

public:
	StrConvAT(const wchar_t* pSrc) :
		m_body(mir_u2a(pSrc)) {}

    ~StrConvAT() {  mir_free(m_body); }
	operator const char*  () const { return m_body; }
	operator const wchar_t* () const { return (wchar_t*)m_body; }  // type cast to fake the interface definition
	operator const LPARAM () const { return (LPARAM)m_body; }
};

#define StrConvT(x) StrConvUT(x)
#define StrConvTu(x) x
#define StrConvA(x) StrConvAT(x)
#define StrConvU(x) x
