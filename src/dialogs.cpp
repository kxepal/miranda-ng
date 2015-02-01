#include "common.h"

#define szAskSendSms  LPGEN("An SMS with registration code will be sent to your mobile phone.\nNotice that you are not able to use the real WhatsApp and this plugin simultaneously!\nContinue?")
#define szPasswordSet LPGEN("Your password has been set automatically. You can proceed with login now")

INT_PTR CALLBACK WhatsAppAccountProc(HWND hwndDlg, UINT message, WPARAM wparam, LPARAM lparam)
{
	WhatsAppProto *proto;

	switch (message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		proto = reinterpret_cast<WhatsAppProto*>(lparam);
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lparam);
		SendDlgItemMessage(hwndDlg, IDC_PW, EM_LIMITTEXT, 3, 0);
		SendDlgItemMessage(hwndDlg, IDC_PW2, EM_LIMITTEXT, 3, 0);
		CheckDlgButton(hwndDlg, IDC_SSL, db_get_b(NULL, proto->m_szModuleName, WHATSAPP_KEY_SSL, 0) ? BST_CHECKED : BST_UNCHECKED);
		{
			ptrA szStr(proto->getStringA(WHATSAPP_KEY_CC));
			if (szStr != NULL)
				SetDlgItemTextA(hwndDlg, IDC_CC, szStr);

			if ((szStr = proto->getStringA(WHATSAPP_KEY_LOGIN)) != NULL)
				SetDlgItemTextA(hwndDlg, IDC_LOGIN, szStr);

			if ((szStr = proto->getStringA(WHATSAPP_KEY_NICK)) != NULL)
				SetDlgItemTextA(hwndDlg, IDC_NICK, szStr);
		}

		EnableWindow(GetDlgItem(hwndDlg, IDC_PW), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_PW2), FALSE);

		if (!proto->isOffline()) {
			SendDlgItemMessage(hwndDlg, IDC_CC, EM_SETREADONLY, 1, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOGIN, EM_SETREADONLY, 1, 0);
			SendDlgItemMessage(hwndDlg, IDC_NICK, EM_SETREADONLY, 1, 0);
			SendDlgItemMessage(hwndDlg, IDC_PW, EM_SETREADONLY, 1, 0);
			SendDlgItemMessage(hwndDlg, IDC_PW2, EM_SETREADONLY, 1, 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SSL), FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wparam) == IDC_BUTTON_REQUEST_CODE || LOWORD(wparam) == IDC_BUTTON_REGISTER) {
			proto = reinterpret_cast<WhatsAppProto*>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));

			string password;
			char cc[5];
			char number[128];
			GetDlgItemTextA(hwndDlg, IDC_CC, cc, SIZEOF(cc));
			GetDlgItemTextA(hwndDlg, IDC_LOGIN, number, SIZEOF(number));

			if (LOWORD(wparam) == IDC_BUTTON_REQUEST_CODE) {
				if (IDYES == MessageBox(NULL, TranslateT(szAskSendSms), PRODUCT_NAME, MB_YESNO)) {
					if (proto->Register(REG_STATE_REQ_CODE, string(cc), string(number), string(), password)) {
						if (!password.empty()) {
							MessageBox(NULL, TranslateT(szPasswordSet), PRODUCT_NAME, MB_ICONWARNING);
							proto->setString(WHATSAPP_KEY_PASS, password.c_str());
						}
						else {
							EnableWindow(GetDlgItem(hwndDlg, IDC_PW), TRUE); // unblock sms code entry field
							EnableWindow(GetDlgItem(hwndDlg, IDC_PW2), TRUE);
						}
					}
				}
			}
			else if (LOWORD(wparam) == IDC_BUTTON_REGISTER) {
				HWND hwnd1 = GetDlgItem(hwndDlg, IDC_PW), hwnd2 = GetDlgItem(hwndDlg, IDC_PW2);
				if (GetWindowTextLength(hwnd1) != 3 || GetWindowTextLength(hwnd2) != 3) {
					MessageBox(NULL, TranslateT("Please correctly specify your registration code received by SMS"), PRODUCT_NAME, MB_ICONEXCLAMATION);
					return TRUE;
				}

				char code[10];
				GetWindowTextA(hwnd1, code, 4);
				GetWindowTextA(hwnd2, code + 3, 4);

				if (proto->Register(REG_STATE_REG_CODE, string(cc), string(number), string(code), password)) {
					proto->setString(WHATSAPP_KEY_PASS, password.c_str());
					MessageBox(NULL, TranslateT(szPasswordSet), PRODUCT_NAME, MB_ICONWARNING);
				}
			}
		}

		if (HIWORD(wparam) == EN_CHANGE && reinterpret_cast<HWND>(lparam) == GetFocus()) {
			switch (LOWORD(wparam)) {
			case IDC_CC:
			case IDC_LOGIN:
			case IDC_NICK:
			case IDC_SSL:
			case IDC_PW:
			case IDC_PW2:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<NMHDR *>(lparam)->code == PSN_APPLY) {
			proto = reinterpret_cast<WhatsAppProto *>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
			char str[128];

			GetDlgItemTextA(hwndDlg, IDC_CC, str, SIZEOF(str));
			proto->setString(WHATSAPP_KEY_CC, str);

			GetDlgItemTextA(hwndDlg, IDC_LOGIN, str, SIZEOF(str));
			proto->setString(WHATSAPP_KEY_LOGIN, str);

			GetDlgItemTextA(hwndDlg, IDC_NICK, str, SIZEOF(str));
			proto->setString(WHATSAPP_KEY_NICK, str);

			proto->setByte(WHATSAPP_KEY_SSL, IsDlgButtonChecked(hwndDlg, IDC_SSL));
			return TRUE;
		}
		break;
	}

	return FALSE;
}
