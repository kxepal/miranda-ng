#ifndef _TOX_OPTIONS_H_
#define _TOX_OPTIONS_H_

struct ItemInfo
{
	int iItem;
	HWND hwndList;
};

class CToxOptionsMain : public CToxDlgBase
{
private:
	typedef CToxDlgBase CSuper;

	CCtrlEdit m_toxAddress;
	CCtrlButton m_toxAddressCopy;
	CCtrlButton m_profileCreate;
	CCtrlButton m_profileImport;
	CCtrlButton m_profileExport;

	CCtrlEdit m_nickname;
	CCtrlEdit m_password;
	CCtrlEdit m_group;

	CCtrlCheck m_enableUdp;
	CCtrlCheck m_enableIPv6;

protected:
	void OnInitDialog();

	void ToxAddressCopy_OnClick(CCtrlButton*);
	void ProfileCreate_OnClick(CCtrlButton*);
	void ProfileImport_OnClick(CCtrlButton*);
	void ProfileExport_OnClick(CCtrlButton*);

	void OnApply();

public:
	CToxOptionsMain(CToxProto *proto, int idDialog, HWND hwndParent = NULL);

	static CDlgBase *Create(void *param) { return new CToxOptionsMain((CToxProto *)param, IDD_OPTIONS_MAIN); }
};

#endif //_TOX_OPTIONS_H_