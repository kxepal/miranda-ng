#include "common.h"

INT_PTR __cdecl CToxProto::SetMyNickname(WPARAM wParam, LPARAM lParam)
{
	ptrT nickname((wParam & SMNN_UNICODE) ? mir_u2t((TCHAR*)lParam) : mir_a2t((char*)lParam));

	setTString("Nick", nickname);
	TOX_ERR_SET_INFO error; //todo: use it somehow
	tox_self_set_name(tox, (uint8_t*)(char*)ptrA(mir_utf8encodeT(nickname)), mir_tstrlen(nickname), &error);

	return 0;
}
