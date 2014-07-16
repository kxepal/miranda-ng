#ifndef _STEAM_CHAT_H_
#define _STEAM_CHAT_H_

namespace SteamWebApi
{
	class GetChatRequest : public HttpGetRequest
	{
	public:
		GetChatRequest(const char *token, const char *steamId, const char *sessionId) :
			HttpGetRequest(STEAM_WEB_URL "/chat")
		{
			char login[MAX_PATH];
			mir_snprintf(login, SIZEOF(login), "%s%s%s", steamId, "%7C%7C", token);

			char cookie[MAX_PATH];
			mir_snprintf(cookie, SIZEOF(cookie), "steamLogin=%s; sessionid=%s; forceMobile=0", login, sessionId);

			AddHeader("Cookie", cookie);
		}
	};

	class GetFriendStateRequest : public HttpsGetRequest
	{
	public:
		GetFriendStateRequest(const char *steamId) :
			HttpsGetRequest(STEAM_WEB_URL "chat/friendstate/%s", steamId)
		{
		}
	};
}

#endif //_STEAM_CHAT_H_