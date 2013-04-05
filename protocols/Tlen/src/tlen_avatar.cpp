/*

Tlen Protocol Plugin for Miranda NG
Copyright (C) 2004-2007  Piotr Piastucki

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


#include "jabber.h"
#include "jabber_list.h"
#include "tlen_avatar.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* TlenGetAvatarFileName() - gets a file name for the avatar image */

void TlenGetAvatarFileName(TlenProtocol *proto, JABBER_LIST_ITEM *item, TCHAR* ptszDest, int cbLen)
{
	DWORD dwAttributes;
	int tPathLen;
	int format = PA_FORMAT_PNG;
	TCHAR* tszFileType;
	TCHAR* tmpPath = Utils_ReplaceVarsT( TEXT("%miranda_avatarcache%") );
	tPathLen = mir_sntprintf( ptszDest, cbLen, TEXT("%s\\Tlen"), tmpPath );
	mir_free(tmpPath);
	dwAttributes = GetFileAttributes( ptszDest );
	if (dwAttributes == 0xffffffff || ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0)
		CreateDirectoryTreeT(ptszDest);

	ptszDest[ tPathLen++ ] = '\\';
	if (item != NULL) {
		format = item->avatarFormat;
	} else if (proto->threadData != NULL) {
		format = proto->threadData->avatarFormat;
	} else {
		format = db_get_dw(NULL, proto->m_szModuleName, "AvatarFormat", PA_FORMAT_UNKNOWN);
	}
	tszFileType = TEXT("png");
	switch(format) {
		case PA_FORMAT_JPEG: tszFileType = TEXT("jpg");   break;
		case PA_FORMAT_ICON: tszFileType = TEXT("ico");   break;
		case PA_FORMAT_PNG:  tszFileType = TEXT("png");   break;
		case PA_FORMAT_GIF:  tszFileType = TEXT("gif");   break;
		case PA_FORMAT_BMP:  tszFileType = TEXT("bmp");   break;
	}
	if ( item != NULL ) {
		char* hash;
		hash = JabberSha1(item->jid);
		TCHAR* hashT = mir_a2t(hash);
		mir_free( hash );
		mir_sntprintf( ptszDest + tPathLen, MAX_PATH - tPathLen, TEXT("%s.%s"), hashT, tszFileType );
		mir_free( hashT );
	} else {
		TCHAR* m_tszModuleName = mir_a2t(proto->m_szModuleName);
		mir_sntprintf( ptszDest + tPathLen, MAX_PATH - tPathLen, TEXT("%s_avatar.%s"), m_tszModuleName, tszFileType );
		mir_free( m_tszModuleName );
	}
}

static void RemoveAvatar(TlenProtocol *proto, HANDLE hContact) {
	TCHAR tFileName[ MAX_PATH ];
	if (hContact == NULL) {
		proto->threadData->avatarHash[0] = '\0';
	}
	TlenGetAvatarFileName( proto, NULL, tFileName, sizeof tFileName );
	DeleteFile(tFileName);
	db_unset(hContact, "ContactPhoto", "File");
	db_unset(hContact, proto->m_szModuleName, "AvatarHash");
	db_unset(hContact, proto->m_szModuleName, "AvatarFormat");
	ProtoBroadcastAck(proto->m_szModuleName, NULL, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0);
}

static void SetAvatar(TlenProtocol *proto, HANDLE hContact, JABBER_LIST_ITEM *item, char *data, int len, DWORD format) {
	FILE* out;
	TCHAR filename[MAX_PATH];
	char md5[33];
	mir_md5_state_t ctx;
	DWORD digest[4];

	if (format == PA_FORMAT_UNKNOWN && len > 4) {
		format = JabberGetPictureType(data);
	}

	mir_md5_init( &ctx );
	mir_md5_append( &ctx, ( BYTE* )data, len);
	mir_md5_finish( &ctx, ( BYTE* )digest );

	sprintf( md5, "%08x%08x%08x%08x", (int)htonl(digest[0]), (int)htonl(digest[1]), (int)htonl(digest[2]), (int)htonl(digest[3]));
	if (item != NULL) {
		char *hash = item->avatarHash;
		item->avatarFormat = format;
		item->avatarHash = mir_strdup(md5);
		mir_free(hash);
	} else {
		proto->threadData->avatarFormat = format;
		strcpy(proto->threadData->avatarHash, md5);
	}
	TlenGetAvatarFileName(proto, item, filename, sizeof filename );
	DeleteFile(filename);
	out = _tfopen( filename, TEXT("wb") );
	if ( out != NULL ) {
		fwrite( data, len, 1, out );
		fclose( out );
		db_set_ts(hContact, "ContactPhoto", "File", filename );
		db_set_s(hContact, proto->m_szModuleName, "AvatarHash",  md5);
		db_set_dw(hContact, proto->m_szModuleName, "AvatarFormat",  format);
	}
	ProtoBroadcastAck( proto->m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL , 0);
}

int TlenProcessAvatarNode(TlenProtocol *proto, XmlNode *avatarNode, JABBER_LIST_ITEM *item) {
	XmlNode *aNode;
	char *oldHash = NULL;
	char *md5 = NULL, *type = NULL;
	HANDLE hContact;
	hContact = NULL;
	if (item != NULL) {
		if ((hContact=JabberHContactFromJID(proto, item->jid)) == NULL) return 0;
	}
	if (item == NULL) {
		oldHash = proto->threadData->avatarHash;
	} else {
		oldHash = item->avatarHash;
	}
	if (avatarNode != NULL) {
		aNode = JabberXmlGetChild(avatarNode, "a");
		if (aNode != NULL) {
			type = JabberXmlGetAttrValue(aNode, "type");
			md5 = JabberXmlGetAttrValue(aNode, "md5");
		}
	}
	if (md5 != NULL) {
		/* check contact's avatar hash - md5 */
		if (oldHash == NULL || strcmp(oldHash, md5)) {
			if (item != NULL) {
				item->newAvatarDownloading = TRUE;
			}
			TlenGetAvatar(proto, hContact);
			return 1;
		}
	} else {
		/* remove avatar */
		if (oldHash != NULL) {
			if (item != NULL) {
				item->avatarHash = NULL;
				mir_free(oldHash);
				item->newAvatarDownloading = FALSE;
			}
			RemoveAvatar(proto, hContact);
			return 1;
		}
	}
	return 0;
}

void TlenProcessPresenceAvatar(TlenProtocol *proto, XmlNode *node, JABBER_LIST_ITEM *item) {
	HANDLE hContact;
	if ((hContact=JabberHContactFromJID(proto, item->jid)) == NULL) return;
	TlenProcessAvatarNode(proto, JabberXmlGetChild(node, "avatar"), item);
}


static char *replaceTokens(const char *base, const char *uri, const char *login, const char* token, int type, int access) {
	char *result;
	int i, l, size;
	l = (int)strlen(uri);
	size = (int)strlen(base);
	for (i = 0; i < l; ) {
		if (!strncmp(uri + i, "^login^", 7)) {
			size += (int)strlen(login);
			i += 7;
		} else if (!strncmp(uri + i, "^type^", 6)) {
			size++;
			i += 6;
		} else if (!strncmp(uri + i, "^token^", 7)) {
			size += (int)strlen(token);
			i += 7;
		} else if (!strncmp(uri + i, "^access^", 8)) {
			size++;
			i += 8;
		} else {
			size++;
			i++;
		}
	}
	result = (char *)mir_alloc(size +1);
	strcpy(result, base);
	size = (int)strlen(base);
	for (i = 0; i < l; ) {
		if (!strncmp(uri + i, "^login^", 7)) {
			strcpy(result + size, login);
			size += (int)strlen(login);
			i += 7;
		} else if (!strncmp(uri + i, "^type^", 6)) {
			result[size++] = '0' + type;
			i += 6;
		} else if (!strncmp(uri + i, "^token^", 7)) {
			strcpy(result + size, token);
			size += (int)strlen(token);
			i += 7;
		} else if (!strncmp(uri + i, "^access^", 8)) {
			result[size++] = '0' + access;
			i += 8;
		} else {
			result[size++] = uri[i++];
		}
	}
	result[size] = '\0';
	return result;
}


static int getAvatarMutex = 0;

typedef struct {
	TlenProtocol *proto;
	HANDLE hContact;
} TLENGETAVATARTHREADDATA;

static void TlenGetAvatarThread(void *ptr) {

	JABBER_LIST_ITEM *item = NULL;
	NETLIBHTTPREQUEST req;
	NETLIBHTTPREQUEST *resp;
	TLENGETAVATARTHREADDATA *data = (TLENGETAVATARTHREADDATA *)ptr;
	HANDLE hContact = data->hContact;
	char *request;
	char *login = NULL;
	if (hContact != NULL) {
		char *jid = JabberJIDFromHContact(data->proto, hContact);
		login = JabberNickFromJID(jid);
		item = JabberListGetItemPtr(data->proto, LIST_ROSTER, jid);
		mir_free(jid);
	} else {
		login = mir_strdup(data->proto->threadData->username);
	}
	if ((data->proto->threadData != NULL && hContact == NULL) || item != NULL) {
		DWORD format = PA_FORMAT_UNKNOWN;
		if (item != NULL) {
			item->newAvatarDownloading = TRUE;
		}
		request = replaceTokens(data->proto->threadData->tlenConfig.mailBase, data->proto->threadData->tlenConfig.avatarGet, login, data->proto->threadData->avatarToken, 0, 0);
		ZeroMemory(&req, sizeof(req));
		req.cbSize = sizeof(req);
		req.requestType = data->proto->threadData->tlenConfig.avatarGetMthd;
		req.flags = 0;
		req.headersCount = 0;
		req.headers = NULL;
		req.dataLength = 0;
		req.szUrl = request;
		resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)data->proto->hNetlibUser, (LPARAM)&req);
		if (item != NULL) {
			item->newAvatarDownloading = FALSE;
		}
		if (resp != NULL) {
			if (resp->resultCode/100 == 2) {
				if (resp->dataLength > 0) {
					int i;
					for (i=0; i<resp->headersCount; i++ ) {
						if (strcmpi(resp->headers[i].szName, "Content-Type") == 0) {
							if (strcmpi(resp->headers[i].szValue, "image/png") == 0) {
								format = PA_FORMAT_PNG;
							} else if (strcmpi(resp->headers[i].szValue, "image/x-png") == 0) {
								format = PA_FORMAT_PNG;
							} else if (strcmpi(resp->headers[i].szValue, "image/jpeg") == 0) {
								format = PA_FORMAT_JPEG;
							} else if (strcmpi(resp->headers[i].szValue, "image/jpg") == 0) {
								format = PA_FORMAT_JPEG;
							} else if (strcmpi(resp->headers[i].szValue, "image/gif") == 0) {
								format = PA_FORMAT_GIF;
							} else if (strcmpi(resp->headers[i].szValue, "image/bmp") == 0) {
								format = PA_FORMAT_BMP;
							}
							break;
						}
					}
					SetAvatar(data->proto, hContact, item, resp->pData, resp->dataLength, format);
				} else {
					RemoveAvatar(data->proto, hContact);
				}
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);
		}
		mir_free(request);
		mir_free(login);
	}
	if (hContact == NULL) {
		getAvatarMutex = 0;
	}
	mir_free(data);
}

void TlenGetAvatar(TlenProtocol *proto, HANDLE hContact) {
	if (hContact == NULL) {
		if (getAvatarMutex != 0) {
			return;
		}
		getAvatarMutex = 1;
	}
	{
		TLENGETAVATARTHREADDATA *data = (TLENGETAVATARTHREADDATA *)mir_alloc(sizeof(TLENGETAVATARTHREADDATA));
		data->proto = proto;
		data->hContact = hContact;
		JabberForkThread(TlenGetAvatarThread, 0, data);
	}
}

typedef struct {
	TlenProtocol *proto;
	NETLIBHTTPREQUEST *req;
}TLENREMOVEAVATARTHREADDATA;

static void TlenRemoveAvatarRequestThread(void *ptr) {
	NETLIBHTTPREQUEST *resp;
	TLENREMOVEAVATARTHREADDATA *data = (TLENREMOVEAVATARTHREADDATA*)ptr;
	NETLIBHTTPREQUEST *req = (NETLIBHTTPREQUEST *)data->req;
	resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)data->proto->hNetlibUser, (LPARAM)req);
	mir_free(req->szUrl);
	mir_free(req->headers);
	mir_free(req->pData);
	mir_free(req);
	if (resp != NULL) {
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);
		RemoveAvatar(data->proto, NULL);
	}
	mir_free(data);

}

typedef struct {
	TlenProtocol *proto;
	NETLIBHTTPREQUEST *req;
	char *data;
	int  length;
}TLENUPLOADAVATARTHREADDATA;

static void TlenUploadAvatarRequestThread(void *ptr) {
	NETLIBHTTPREQUEST *resp;
	TLENUPLOADAVATARTHREADDATA * data = (TLENUPLOADAVATARTHREADDATA *) ptr;
	NETLIBHTTPREQUEST *req = data->req;
	resp = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)data->proto->hNetlibUser, (LPARAM)req);
	if (resp != NULL) {
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)resp);
		SetAvatar(data->proto, NULL, NULL, data->data, data->length, PA_FORMAT_PNG);
	}
	mir_free(req->szUrl);
	mir_free(req->headers);
	mir_free(req->pData);
	mir_free(req);
	mir_free(data->data);
	mir_free(data);
}

void TlenRemoveAvatar(TlenProtocol *proto) {
	NETLIBHTTPREQUEST *req;
	char *request;
	if (proto->threadData != NULL) {
		TLENREMOVEAVATARTHREADDATA *data = (TLENREMOVEAVATARTHREADDATA *)mir_alloc(sizeof(TLENREMOVEAVATARTHREADDATA));
		req = (NETLIBHTTPREQUEST *)mir_alloc(sizeof(NETLIBHTTPREQUEST));
		data->proto =proto;
		data->req = req;
		request = replaceTokens(proto->threadData->tlenConfig.mailBase, proto->threadData->tlenConfig.avatarRemove, "", proto->threadData->avatarToken, 0, 0);
		ZeroMemory(req, sizeof(NETLIBHTTPREQUEST));
		req->cbSize = sizeof(NETLIBHTTPREQUEST);
		req->requestType = proto->threadData->tlenConfig.avatarGetMthd;
		req->szUrl = request;
		JabberForkThread(TlenRemoveAvatarRequestThread, 0, data);
	}
}


void TlenUploadAvatar(TlenProtocol *proto, unsigned char *data, int dataLen, int access) {
	NETLIBHTTPREQUEST *req;
	NETLIBHTTPHEADER *headers;
	TLENUPLOADAVATARTHREADDATA *threadData;
	char *request;
	unsigned char *buffer;
	if (proto->threadData != NULL && dataLen > 0 && data != NULL) {
		char *mpartHead =  "--AaB03x\r\nContent-Disposition: form-data; name=\"filename\"; filename=\"plik.png\"\r\nContent-Type: image/png\r\n\r\n";
		char *mpartTail =  "\r\n--AaB03x--\r\n";
		int size, sizeHead = (int)strlen(mpartHead), sizeTail = (int)strlen(mpartTail);
		request = replaceTokens(proto->threadData->tlenConfig.mailBase, proto->threadData->tlenConfig.avatarUpload, "", proto->threadData->avatarToken, 0, access);
		threadData = (TLENUPLOADAVATARTHREADDATA *)mir_alloc(sizeof(TLENUPLOADAVATARTHREADDATA));
		threadData->proto = proto;
		req = (NETLIBHTTPREQUEST *)mir_alloc(sizeof(NETLIBHTTPREQUEST));
		headers = (NETLIBHTTPHEADER *)mir_alloc(sizeof(NETLIBHTTPHEADER));
		ZeroMemory(req, sizeof(NETLIBHTTPREQUEST));
		req->cbSize = sizeof(NETLIBHTTPREQUEST);
		req->requestType = proto->threadData->tlenConfig.avatarUploadMthd;
		req->szUrl = request;
		req->flags = 0;
		headers[0].szName = "Content-Type";
		headers[0].szValue = "multipart/form-data; boundary=AaB03x";
		req->headersCount = 1;
		req->headers = headers;
		size = dataLen + sizeHead + sizeTail;
		buffer = (unsigned char *)mir_alloc(size);
		memcpy(buffer, mpartHead, sizeHead);
		memcpy(buffer + sizeHead, data, dataLen);
		memcpy(buffer + sizeHead + dataLen, mpartTail, sizeTail);
		req->dataLength = size;
		req->pData = (char*)buffer;
		threadData->req = req;
		threadData->data = (char *) mir_alloc(dataLen);
		memcpy(threadData->data, data, dataLen);
		threadData->length = dataLen;
		JabberForkThread(TlenUploadAvatarRequestThread, 0, threadData);
	}
}

