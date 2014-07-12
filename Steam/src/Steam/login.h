#ifndef _STEAM_LOGIN_H_
#define _STEAM_LOGIN_H_

namespace SteamWebApi
{
	class DoLoginRequest : public HttpsPostRequest
	{
	private:
		void InitData(const char *username, const char *password, const char *timestamp, const char *guardId = "-1", const char *guardCode = "", const char *captchaId = "-1", const char *captchaText = "")
		{
			char data[1024];
			mir_snprintf(data, SIZEOF(data),
				"username=%s&password=%s&emailsteamid=%s&emailauth=%s&captchagid=%s&captcha_text=%s&rsatimestamp=%s&donotcache=%ld&remember_login=false",
				username,
				ptrA(mir_urlEncode(password)),
				guardId,
				guardCode,
				captchaId,
				ptrA(mir_urlEncode(captchaText)),
				timestamp,
				time(NULL));

			SetData(data, strlen(data));
		}

	public:
		DoLoginRequest(const char *username, const char *password, const char *timestamp) :
			HttpsPostRequest(STEAM_COM_URL "/login/dologin")
		{
			flags = NLHRF_HTTP11 | NLHRF_SSL | NLHRF_NODUMP;

			InitData(username, password, timestamp);
		}

		DoLoginRequest(const char *username, const char *password, const char *timestamp, const char *guardId, const char *guardCode) :
			HttpsPostRequest(STEAM_COM_URL "/login/dologin")
		{
			flags = NLHRF_HTTP11 | NLHRF_SSL | NLHRF_NODUMP;

			InitData(username, password, timestamp, guardId, guardCode);
		}

		DoLoginRequest(const char *username, const char *password, const char *timestamp, const char *guardId, const char *guardCode, const char *captchaId, const char *captchaText) :
			HttpsPostRequest(STEAM_COM_URL "/login/dologin")
		{
			flags = NLHRF_HTTP11 | NLHRF_SSL | NLHRF_NODUMP;

			InitData(username, password, timestamp, guardId, guardCode, captchaId, captchaText);
		}
	};

	class TransferRequest : public HttpsPostRequest
	{
	public:
		TransferRequest(const char *token, const char *steamId, const char *auth, const char *webcookie) :
			HttpsPostRequest(STEAM_COM_URL "/login/transfer")
		{
			char data[256];
			mir_snprintf(data, SIZEOF(data), "token=%s&steamid=%s&auth=%s&webcookie=%s&remember_login=false",
				token,
				steamId,
				auth,
				webcookie);

			SetData(data, strlen(data));
		}
	};

	class LogonRequest : public HttpsPostRequest
	{
	public:
		LogonRequest(const char *token) :
			HttpsPostRequest(STEAM_API_URL "/ISteamWebUserPresenceOAuth/Logon/v0001")
		{
			char data[256];
			mir_snprintf(data, SIZEOF(data), "access_token=%s", token);

			SetData(data, strlen(data));
		}
	};

	class LogoffRequest : public HttpsPostRequest
	{
	public:
		LogoffRequest(const char *token, const char *umqId) :
			HttpsPostRequest(STEAM_API_URL "/ISteamWebUserPresenceOAuth/Logoff/v0001")
		{
			char data[256];
			mir_snprintf(data, SIZEOF(data), "access_token=%s&umqid=%s", token, umqId);

			SetData(data, strlen(data));
		}
	};
}

#endif //_STEAM_LOGIN_H_