/*
    Variables Plugin for Miranda-IM (www.miranda-im.org)
    Copyright 2003-2006 P. Boon

    This program is mir_free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "variables.h"

static TCHAR *parseUrlEnc(ARGUMENTSINFO *ai) {
	char hex[8];

	if (ai->argc != 2) {
		return NULL;
	}

	char *res = mir_t2a(ai->targv[1]);

	if (res == NULL) {
		return NULL;
	}
	size_t cur = 0;
	while (cur < strlen(res)) {
		if (( (*(res+cur) >= '0') && (*(res+cur) <= '9')) || ( (*(res+cur) >= 'a') && (*(res+cur) <= 'z')) || ( (*(res+cur) >= 'A') && (*(res+cur) <= 'Z')) ) {
			cur++;
			continue;
		}
		res = ( char* )mir_realloc(res, strlen(res)+4);
		if (res == NULL)
			return NULL;

		MoveMemory(res+cur+3, res+cur+1, strlen(res+cur+1)+1);
		mir_snprintf(hex, sizeof(hex), "%%%x", *(res+cur));
		strncpy(res+cur, hex, strlen(hex));
		cur+=strlen(hex);
	}

	TCHAR *tres = mir_a2t(res);

	mir_free(res);

	return tres;
}

static TCHAR *parseUrlDec(ARGUMENTSINFO *ai) {
	char hex[8];

	if (ai->argc != 2) {
		return NULL;
	}

	char *res = mir_t2a(ai->targv[1]);

	if (res == NULL) {
		return NULL;
	}
	unsigned int cur = 0;
	while (cur < strlen(res)) {
		if ((*(res+cur) == '%') && (strlen(res+cur) >= 3)) {
			memset(hex, '\0', sizeof(hex));
			strncpy(hex, res+cur+1, 2);
			*(res+cur) = (char)strtol(hex, NULL, 16);
			MoveMemory(res+cur+1, res+cur+3, strlen(res+cur+3)+1);
		}
		cur++;
	}
	res = ( char* )mir_realloc(res, strlen(res)+1);

	TCHAR *tres = mir_a2t(res);

	mir_free(res);
	
	return tres;
}

static TCHAR *parseNToA(ARGUMENTSINFO *ai) {
	struct in_addr in;

	if (ai->argc != 2) {
		return NULL;
	}
	
	in.s_addr = ttoi(ai->targv[1]);
	char *res = inet_ntoa(in);
	if (res != NULL) {

		return mir_a2t(res);

	}

	return NULL;
}

static TCHAR *parseHToA(ARGUMENTSINFO *ai) {
	struct in_addr in;

	if (ai->argc != 2) {
		return NULL;
	}
	
	in.s_addr = htonl(ttoi(ai->targv[1]));
	char *res = inet_ntoa(in);
	if (res != NULL) {

		return mir_a2t(res);

	}

	return NULL;
}

int registerInetTokens() {

	registerIntToken(_T(URLENC), parseUrlEnc, TRF_FUNCTION, LPGEN("Internet Related")"\t(x)\t"LPGEN("converts each non-html character into hex format"));
	registerIntToken(_T(URLDEC), parseUrlDec, TRF_FUNCTION, LPGEN("Internet Related")"\t(x)\t"LPGEN("converts each hex value into non-html character"));
	registerIntToken(_T(NTOA), parseNToA, TRF_FUNCTION, LPGEN("Internet Related")"\t(x)\t"LPGEN("converts a 32-bit number to IPv4 dotted notation"));
	registerIntToken(_T(HTOA), parseHToA, TRF_FUNCTION, LPGEN("Internet Related")"\t(x)\t"LPGEN("converts a 32-bit number (in host byte order) to IPv4 dotted notation"));

	return 0;
}
