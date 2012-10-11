#define _CRT_SECURE_NO_WARNINGS
#define MIRANDA_VER 0x0A00

#include <windows.h>

#include <newpluginapi.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_database.h>
#include <win2k.h>

#include "resource.h"
#include "Version.h"

#define SUB_KEY _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define ModuleName "Autorun"

// Plugin UUID for New plugin loader
// req. 0.7.18+ core
// {EB0465E2-CEEE-11DB-83EF-C1BF55D89593}

#define MIID_AUTORUN  {0xeb0465e2, 0xceee, 0x11db, { 0x83, 0xef, 0xc1, 0xbf, 0x55, 0xd8, 0x95, 0x93}}

HKEY ROOT_KEY = HKEY_CURRENT_USER;
