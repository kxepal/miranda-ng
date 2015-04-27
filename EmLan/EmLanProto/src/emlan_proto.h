#ifndef _EMLAN_PROTO_H_
#define _EMLAN_PROTO_H_

struct CEmLanProto : public PROTO<CEmLanProto>, public CMLan
{
public:
	//////////////////////////////////////////////////////////////////////////////////////
	//Ctors

	CEmLanProto(const char *protoName, const wchar_t *userName);
	~CEmLanProto();

	//////////////////////////////////////////////////////////////////////////////////////
	// Virtual functions

	virtual	MCONTACT __cdecl AddToList(int flags, PROTOSEARCHRESULT* psr);

	virtual	HANDLE   __cdecl FileAllow(MCONTACT hContact, HANDLE hTransfer, const PROTOCHAR* tszPath);
	virtual	int      __cdecl FileCancel(MCONTACT hContact, HANDLE hTransfer);
	virtual	int      __cdecl FileDeny(MCONTACT hContact, HANDLE hTransfer, const PROTOCHAR* tszReason);
	virtual	int      __cdecl FileResume(HANDLE hTransfer, int* action, const PROTOCHAR** tszFilename);

	virtual	DWORD_PTR __cdecl GetCaps(int type, MCONTACT hContact = NULL);

	virtual	HWND      __cdecl SearchAdvanced(HWND owner);
	virtual	HWND      __cdecl CreateExtendedSearchUI(HWND owner);

	virtual	int       __cdecl RecvMsg(MCONTACT hContact, PROTORECVEVENT*);
	virtual	int       __cdecl SendMsg(MCONTACT hContact, int flags, const char* msg);

	virtual	HANDLE    __cdecl SendFile(MCONTACT hContact, const PROTOCHAR*, PROTOCHAR **ppszFiles);

	virtual	int       __cdecl SetStatus(int iNewStatus);

	virtual	HANDLE    __cdecl GetAwayMsg(MCONTACT hContact);
	virtual	int       __cdecl SetAwayMsg(int iStatus, const PROTOCHAR* msg);

	virtual	int       __cdecl UserIsTyping(MCONTACT hContact, int type);

	virtual	int       __cdecl OnEvent(PROTOEVENTTYPE iEventType, WPARAM wParam, LPARAM lParam);

	// accounts
	static CEmLanProto* InitAccount(const char *protoName, const TCHAR *userName);
	static int         UninitAccount(CEmLanProto *proto);

private:
	// accounts
	static LIST<CEmLanProto> Accounts;
	static int CompareAccounts(const CEmLanProto *p1, const CEmLanProto *p2);
};

#endif //_EMLAN_PROTO_H_
