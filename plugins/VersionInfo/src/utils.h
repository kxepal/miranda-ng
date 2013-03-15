#ifndef _M_VERSIONINFO_UTILS_H
#define _M_VERSIONINFO_UTILS_H

//utils.cpp
void MB(const TCHAR*);
void Log(const TCHAR*);
TCHAR *StrTrim(TCHAR *, const TCHAR *);

//utils.cpp
TCHAR *RelativePathToAbsolute(TCHAR *szRelative, TCHAR *szAbsolute, size_t size);
TCHAR *AbsolutePathToRelative(TCHAR *szAbsolute, TCHAR *szRelative, size_t size);

//returns a string from the database and uses MirandaFree to deallocate the string, leaving only the local copy
//utils.cpp
int GetStringFromDatabase(char *szSettingName, TCHAR *szError, TCHAR *szResult, size_t size);

//a string of the form %s(start) | %s(end) is split into the two strings (start and end)
//utils.cpp
int SplitStringInfo(const TCHAR *szWholeText, TCHAR *szStartText, TCHAR *szEndText);

//utils.cpp
bool DoesDllExist(char *dllName);

//utils.cpp
void GetModuleTimeStamp(TCHAR*, TCHAR*);
void NotifyError(DWORD, const TCHAR*, int);

//utils.cpp
PLUGININFOEX *GetPluginInfo(const char *,HINSTANCE *);
PLUGININFOEX *CopyPluginInfo(PLUGININFOEX *);
void FreePluginInfo(PLUGININFOEX *);

//utils.cpp

BOOL IsCurrentUserLocalAdministrator();

TCHAR *GetLanguageName(LANGID language);
TCHAR *GetLanguageName(LCID locale);

BOOL GetWindowsShell(TCHAR *shellPath, size_t shSize);
BOOL GetInternetExplorerVersion(TCHAR *ieVersion, size_t ieSize);

BOOL UUIDToString(MUUID uuid, TCHAR *str, size_t len);

BOOL IsUUIDNull(MUUID uuid);

#endif