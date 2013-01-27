#pragma once

#include <windows.h>
#include <m_database.h>

class Settings
{
private:
	char *section;

	inline void Initialize(const char *section)
	{
		int len = ::strlen(section);
		
		this->section = new char[len + 1];
		::strncpy(this->section, section, len);
		this->section[len] = '\0';
	}

	inline char * GetString(HANDLE hContact, const char *name, const char *errValue, bool decode)
	{
		char *result = ::DBGetString(hContact, this->section, name);

		if ( !result)
		{
			result = ::mir_strdup(errValue);
		}

		if (result && decode)
		{
			::CallService(
				MS_DB_CRYPT_DECODESTRING,
				sizeof(result),
				reinterpret_cast<LPARAM>(result));
		}

		return result;
	}

	inline wchar_t * GetWString(HANDLE hContact, const char *name, const wchar_t *errValue, bool decode)
	{
		wchar_t *result = ::DBGetStringW(hContact, this->section, name);

		if ( !result)
		{
			result = ::mir_wstrdup(errValue);
		}

		if (result && decode)
		{
			::CallService(
				MS_DB_CRYPT_DECODESTRING,
				sizeof(result),
				reinterpret_cast<LPARAM>(result));
		}

		return result;
	}

	inline void SetString(HANDLE hContact, const char *name, const char *value, bool encode)
	{
		char *result = ::mir_strdup(value);

		if (result && encode)
		{
			::CallService(
				MS_DB_CRYPT_ENCODESTRING, 
				sizeof(result), 
				reinterpret_cast<LPARAM>(result));
		}
		
		::DBWriteContactSettingString(NULL, this->section, name, value);
		
		::mir_free(result);
	}

	inline void SetWString(HANDLE hContact, const char *name, const wchar_t *value, bool encode)
	{
		wchar_t *result = ::mir_wstrdup(value);

		if (result && encode)
		{
			::CallService(
				MS_DB_CRYPT_ENCODESTRING, 
				sizeof(result), 
				reinterpret_cast<LPARAM>(result));
		}
		
		::DBWriteContactSettingWString(NULL, this->section, name, value);
		
		::mir_free(result);
	}

public:
	inline Settings() { }
	inline Settings(const char *section) { this->Initialize(section); }

	template<typename T> T Get(HANDLE hContact, const char *name, const T &errValue);
	template<typename T> T Get(HANDLE hContact, const char *name, const T &errValue, bool decode);
	template<typename T> T* Get(HANDLE hContact, const char *name, const T *errValue);
	template<typename T> T* Get(HANDLE hContact, const char *name, const T *errValue, bool decode);
	template<typename T> T Get(HANDLE hContact, const char *name);
	template<typename T> T Get(HANDLE hContact, const char *name, bool decode);
	template<typename T> T Get(const char *name, const T &errValue);
	template<typename T> T Get(const char *name, const T &errValue, bool decode);
	template<typename T> T* Get(const char *name, const T *errValue);
	template<typename T> T* Get(const char *name, const T *errValue, bool decode);
	template<typename T> T Get(const char *name);
	template<typename T> T Get(const char *name, bool decode);

	template<> inline bool Get(HANDLE hContact, const char *name, const bool &errValue) { return ::DBGetContactSettingByte(hContact, this->section, name, errValue ? 1 : 0) ? true : false; }
	template<> inline bool Get(HANDLE hContact, const char *name) { return ::DBGetContactSettingByte(hContact, this->section, name, 0) ? true : false; }
	template<> inline bool Get(const char *name, const bool &errValue) { return ::DBGetContactSettingByte(NULL, this->section, name, errValue ? 1 : 0) ? true : false; }
	template<> inline bool Get(const char *name) { return ::DBGetContactSettingByte(NULL, this->section, name, 0) ? true : false; }

	template<> inline WORD Get(HANDLE hContact, const char *name, const WORD &errValue) { return ::DBGetContactSettingWord(hContact, this->section, name, errValue); }
	template<> inline WORD Get(HANDLE hContact, const char *name) { return ::DBGetContactSettingWord(hContact, this->section, name, 0); }
	template<> inline WORD Get(const char *name, const WORD &errValue) { return ::DBGetContactSettingWord(NULL, this->section, name, errValue); }
	template<> inline WORD Get(const char *name) { return ::DBGetContactSettingWord(NULL, this->section, name, 0); }

	template<> inline DWORD Get(HANDLE hContact, const char *name, const DWORD &errValue) { return ::DBGetContactSettingDword(hContact, this->section, name, errValue); }
	template<> inline DWORD Get(HANDLE hContact, const char *name) { return ::DBGetContactSettingDword(hContact, this->section, name, 0); }
	template<> inline DWORD Get(const char *name, const DWORD &errValue) { return ::DBGetContactSettingDword(NULL, this->section, name, errValue); }
	template<> inline DWORD Get(const char *name) { return ::DBGetContactSettingDword(NULL, this->section, name, 0); }

	template<> inline char * Get(HANDLE hContact, const char *name, const char *errValue) { return this->GetString(hContact, name, errValue, false); }
	template<> inline char * Get(HANDLE hContact, const char *name) { return this->GetString(hContact, name, "", false); }
	template<> inline char * Get(const char *name, const char *errValue) { return this->GetString(NULL, name, errValue, false); }
	template<> inline char * Get(const char *name) { return this->GetString(NULL, name, "", false); }

	template<> inline char * Get(HANDLE hContact, const char *name, const char *errValue, bool decode) { return this->GetString(hContact, name, errValue, decode); }
	template<> inline char * Get(HANDLE hContact, const char *name, bool decode) { return this->GetString(hContact, name, "", decode); }
	template<> inline char * Get(const char *name, const char *errValue, bool decode) { return this->GetString(NULL, name, errValue, decode); }
	template<> inline char * Get(const char *name, bool decode) { return this->GetString(NULL, name, "", decode); }
	
	template<> inline wchar_t * Get(HANDLE hContact, const char *name, const wchar_t *errValue) { return this->GetWString(hContact, name, errValue, false); }
	template<> inline wchar_t * Get(HANDLE hContact, const char *name) { return this->GetWString(hContact, name, L"", false); }
	template<> inline wchar_t * Get(const char *name, const wchar_t *errValue) { return this->GetWString(NULL, name, errValue, false); }
	template<> inline wchar_t * Get(const char *name) { return this->GetWString(NULL, name, L"", false); }

	template<> inline wchar_t * Get(HANDLE hContact, const char *name, const wchar_t *errValue, bool decode) { return this->GetWString(hContact, name, errValue, decode); }
	template<> inline wchar_t * Get(HANDLE hContact, const char *name, bool decode) { return this->GetWString(hContact, name, L"", decode); }
	template<> inline wchar_t * Get(const char *name, const wchar_t *errValue, bool decode) { return this->GetWString(NULL, name, errValue, decode); }
	template<> inline wchar_t * Get(const char *name, bool decode) { return this->GetWString(NULL, name, L"", decode); }

	template<typename T> void Set(HANDLE hContact, const char *name, const T &value);
	template<typename T> void Set(HANDLE hContact, const char *name, const T *value);
	template<typename T> void Set(HANDLE hContact, const char *name, const T *value, bool encode);
	template<typename T> void Set(const char *name, const T &value);
	template<typename T> void Set(const char *name, const T *value);
	template<typename T> void Set(const char *name, const T *value, bool encode);

	template<> inline void Set(HANDLE hContact, const char *name, const bool &value) { ::DBWriteContactSettingByte(hContact, this->section, name, value); }
	template<> inline void Set(const char *name, const bool &value) { ::DBWriteContactSettingByte(NULL, this->section, name, value); }

	template<> inline void Set(HANDLE hContact, const char *name, const WORD &value) { ::DBWriteContactSettingWord(hContact, this->section, name, value); }
	template<> inline void Set(const char *name, const WORD &value) { ::DBWriteContactSettingWord(NULL, this->section, name, value); }

	template<> inline void Set(HANDLE hContact, const char *name, const DWORD &value) { ::DBWriteContactSettingDword(hContact, this->section, name, value); }
	template<> inline void Set(const char *name, const DWORD &value) { ::DBWriteContactSettingDword(NULL, this->section, name, value); }

	template<> inline void Set(HANDLE hContact, const CHAR *name, const char *value) { this->SetString(hContact, name, value, false); }
	template<> inline void Set(HANDLE hContact, const CHAR *name, const char *value, bool encode) { this->SetString(hContact, name, value, encode); }
	template<> inline void Set(const char *name, const CHAR *value) { this->SetString(NULL, name, value, false); }
	template<> inline void Set(const char *name, const CHAR *value, bool encode) { this->SetString(NULL, name, value, encode); }

	template<> inline void Set(HANDLE hContact, const char *name, const wchar_t *value) { this->SetWString(hContact, name, value, false); }
	template<> inline void Set(HANDLE hContact, const char *name, const wchar_t *value, bool encode) { this->SetWString(hContact, name, value, encode); }
	template<> inline void Set(const char *name,  const wchar_t *value) { this->SetWString(NULL, name, value, false); }
	template<> inline void Set(const char *name,  const wchar_t *value, bool encode) { this->SetWString(NULL, name, value, encode); }

	inline void Delete(const char *name) { ::DBDeleteContactSetting(NULL, this->section, name); }
	inline void Delete(HANDLE hContact, const char *name) { ::DBDeleteContactSetting(hContact, this->section, name); }
	inline static void Delete(HANDLE hContact, const char *section, const char *name) { ::DBDeleteContactSetting(hContact, section, name); }
	inline static void Delete(const char *section, const char *name) { Settings::Delete(NULL, section, name); }
};

class SettingBase
{
private:
	inline void Initialize(const char *section, const char *name)
	{
		int len = ::strlen(section);
		
		this->section = new char[len + 1];
		::strncpy(this->section, section, len);
		this->section[len] = '\0';
		
		this->name = new char[len + 1];
		::strncpy(this->name, name, len);
		this->name[len] = '\0';
	}

protected:
	char *name;
	char *section;

public:
	inline SettingBase() { }

	inline SettingBase(const char *section, const char *name)
	{
		this->Initialize(section, name);
	}

	inline virtual ~SettingBase()
	{
		delete [] this->name;
		delete [] this->section;
	}
	
	inline void Delete(HANDLE hContact)
	{
		::DBDeleteContactSetting(hContact, this->section, this->name);
	}

	inline void Delete()
	{
		this->Delete(NULL);
	}

	inline static void Delete(HANDLE hContact, const char *section, const char *name)
	{
		::DBDeleteContactSetting(hContact, section, name);
	}

	inline static void Delete(const char *section, const char *name)
	{
		::DBDeleteContactSetting(NULL, section, name);
	}
};

template<typename T>
class Setting;

template<>
class Setting<bool> : public SettingBase
{
public:
	inline Setting() { }
	inline Setting(const char *section, const char *name) : SettingBase(section, name) { }

	inline virtual ~Setting() { }

	inline bool Get(HANDLE hContact, const bool &errValue = false)
	{
		return ::DBGetContactSettingByte(hContact, this->section, this->name, errValue) ? true : false;
	}

	inline bool Get(const bool &errValue = false)
	{
		return this->Get(NULL, errValue);
	}

	inline void Set(HANDLE hContact, const bool &value = false)
	{
		::DBWriteContactSettingByte(hContact, this->section, this->name, value);
	}

	inline void Set(const bool &value = false)
	{
		this->Set(NULL, value);
	}
};

template<>
class Setting<char *> : public SettingBase
{
private:
	bool isSecure;

public:
	inline Setting() { }
	inline Setting(const char *section, const char *name) : SettingBase(section, name) { }
	inline Setting(const char *section, const char *name, bool isSecure) : 
		SettingBase(section, name) 
	{ 
		this->isSecure = isSecure;
	}

	inline virtual ~Setting() { }

	inline char *Get(HANDLE hContact, const char *errValue = "")
	{
		char *result = ::DBGetString(hContact, this->section, name);

		if ( !result)
		{
			result = ::mir_strdup(errValue);
		}

		if (result && this->isSecure)
		{
			::CallService(
				MS_DB_CRYPT_DECODESTRING,
				sizeof(result),
				reinterpret_cast<LPARAM>(result));
		}

		return result;
	}

	inline char *Get(const char *errValue = "")
	{
		return this->Get(NULL, errValue);
	}

	inline void Set(HANDLE hContact, const char *value = "")
	{
		char *result = ::mir_strdup(value);

		if(result && this->isSecure)
		{
			::CallService(
				MS_DB_CRYPT_ENCODESTRING, 
				sizeof(result), 
				reinterpret_cast<LPARAM>(result));
		}
		
		::DBWriteContactSettingString(NULL, this->section, name, value);
		
		::mir_free(result);
	}

	inline void Set(const char *value = "")
	{
		this->Set(NULL, value);
	}
};