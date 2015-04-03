#ifndef _TOX_DIALOGS_H_
#define _TOX_DIALOGS_H_

class CToxDlgBase : public CProtoDlgBase<CToxProto>
{
private:
	typedef CProtoDlgBase<CToxProto> CSuper;

protected:
	__inline CToxDlgBase(CToxProto *proto, int idDialog, HWND parent, bool show_label = true) :
		CSuper(proto, idDialog, parent, show_label) { }
};

class CToxPasswordEditor : public CToxDlgBase
{
private:
	typedef CToxDlgBase CSuper;

	CCtrlEdit password;
	CCtrlCheck savePermanently;

	CCtrlButton ok;

protected:
	void OnOk(CCtrlButton*);

public:
	CToxPasswordEditor(CToxProto *proto);
};

#endif //_TOX_DIALOGS_H_