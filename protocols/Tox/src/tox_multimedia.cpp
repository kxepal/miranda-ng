#include "common.h"

/* AUDIO RECEIVING */

CToxAudioCall::CToxAudioCall(CToxProto *proto, MCONTACT hContact) :
	CToxDlgBase(proto, IDD_AUDIO, false),
	hContact(hContact), isCallStarted(false),
	ok(this, IDOK), cancel(this, IDCANCEL)
{
	m_autoClose = CLOSE_ON_CANCEL;
	ok.OnClick = Callback(this, &CToxAudioCall::OnOk);
	cancel.OnClick = Callback(this, &CToxAudioCall::OnCancel);
}

void CToxAudioCall::SetIcon(const char *name)
{
	char iconName[100];
	mir_snprintf(iconName, SIZEOF(iconName), "%s_%s", MODULE, name);
	SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)Skin_GetIcon(iconName, 16));
	SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)Skin_GetIcon(iconName, 32));
}

void CToxAudioCall::OnInitDialog()
{
	Utils_RestoreWindowPosition(m_hwnd, NULL, m_proto->m_szModuleName, "AudioCallWindow");
}

void CToxAudioCall::OnClose()
{
	WindowList_Remove(m_proto->hAudioDialogs, m_hwnd);
	Utils_SaveWindowPosition(m_hwnd, NULL, m_proto->m_szModuleName, "AudioCallWindow");
}

INT_PTR CToxAudioCall::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_AUDIO_END)
		if (wParam == hContact)
			Close();

	return CToxDlgBase::DlgProc(msg, wParam, lParam);
}

void CToxAudioCall::OnStartCall()
{
	ok.Enable(FALSE);
	isCallStarted = true;
	SetIcon("audio_start");
}

//

ToxAvCSettings* CToxProto::GetAudioCSettings()
{
	ToxAvCSettings *cSettings = (ToxAvCSettings*)mir_calloc(sizeof(ToxAvCSettings));
	cSettings->audio_frame_duration = 20;

	DWORD deviceId = getDword("AudioInputDeviceID", WAVE_MAPPER);

	WAVEINCAPS wic;
	MMRESULT error = waveInGetDevCaps(deviceId, &wic, sizeof(WAVEINCAPS));
	if (error != MMSYSERR_NOERROR)
	{
		debugLogA(__FUNCTION__": failed to get input device caps (%d)", error);

		TCHAR errorMessage[MAX_PATH];
		waveInGetErrorText(error, errorMessage, SIZEOF(errorMessage));
		CToxProto::ShowNotification(
			TranslateT("Unable to find input audio device"),
			errorMessage);

		mir_free(cSettings);
		return NULL;
	}

	cSettings->audio_channels = wic.wChannels;
	if ((wic.dwFormats & WAVE_FORMAT_96S16) || (wic.dwFormats & WAVE_FORMAT_96M16))
	{
		cSettings->audio_bitrate = 16 * 1000;
		cSettings->audio_sample_rate = 48000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_96S08) || (wic.dwFormats & WAVE_FORMAT_96M08))
	{
		cSettings->audio_bitrate = 8 * 1000;
		cSettings->audio_sample_rate = 48000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_4S16) || (wic.dwFormats & WAVE_FORMAT_4M16))
	{
		cSettings->audio_bitrate = 16 * 1000;
		cSettings->audio_sample_rate = 24000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_4S08) || (wic.dwFormats & WAVE_FORMAT_4S08))
	{
		cSettings->audio_bitrate = 8 * 1000;
		cSettings->audio_sample_rate = 24000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_2M16) || (wic.dwFormats & WAVE_FORMAT_2S16))
	{
		cSettings->audio_bitrate = 16 * 1000;
		cSettings->audio_sample_rate = 16000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_2M08) || (wic.dwFormats & WAVE_FORMAT_2S08))
	{
		cSettings->audio_bitrate = 8 * 1000;
		cSettings->audio_sample_rate = 16000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_1M16) || (wic.dwFormats & WAVE_FORMAT_1S16))
	{
		cSettings->audio_bitrate = 16 * 1000;
		cSettings->audio_sample_rate = 8000;
	}
	else if ((wic.dwFormats & WAVE_FORMAT_1M08) || (wic.dwFormats & WAVE_FORMAT_1S08))
	{
		cSettings->audio_bitrate = 8 * 1000;
		cSettings->audio_sample_rate = 8000;
	}
	else
	{
		debugLogA(__FUNCTION__": failed to parse input device caps");
		mir_free(cSettings);
		return NULL;
	}

	return cSettings;
}

// incoming call flow

CToxIncomingAudioCall::CToxIncomingAudioCall(CToxProto *proto, MCONTACT hContact) :
	CToxAudioCall(proto, hContact)
{
}

void CToxIncomingAudioCall::OnInitDialog()
{
	SetIcon("audio_ring");
	CToxAudioCall::OnInitDialog();
}

void CToxIncomingAudioCall::OnOk(CCtrlBase*)
{
	ToxAvCSettings *cSettings = m_proto->GetAudioCSettings();
	if (cSettings == NULL)
	{
		Close();
		return;
	}

	if (toxav_answer(m_proto->toxAv, m_proto->calls[hContact], cSettings) == TOX_ERROR)
	{
		m_proto->debugLogA(__FUNCTION__": failed to start call");
		Close();
	}
}

void CToxIncomingAudioCall::OnCancel(CCtrlBase*)
{
	if (!isCallStarted)
		toxav_reject(m_proto->toxAv, m_proto->calls[hContact], NULL);
	else
		toxav_hangup(m_proto->toxAv, m_proto->calls[hContact]);
}

void CToxProto::OnAvInvite(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		toxav_reject(proto->toxAv, callId, NULL);
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		toxav_reject(proto->toxAv, callId, NULL);
		return;
	}

	ToxAvCSettings cSettings;
	if (toxav_get_peer_csettings(proto->toxAv, callId, 0, &cSettings) != av_ErrorNone)
	{
		proto->debugLogA(__FUNCTION__": failed to get codec settings");
		toxav_reject(proto->toxAv, callId, NULL);
		return;
	}

	if (cSettings.call_type != av_TypeAudio)
	{
		proto->debugLogA(__FUNCTION__": video call is unsupported");
		toxav_reject(proto->toxAv, callId, Translate("Video call is unsupported"));
		return;
	}

	PROTORECVEVENT recv = { 0 };
	recv.timestamp = time(NULL);
	recv.lParam = callId;
	recv.flags = PREF_UTF;
	recv.szMessage = mir_utf8encodeT(TranslateT("Incoming call"));
	ProtoChainRecv(hContact, PSR_AUDIO, hContact, (LPARAM)&recv);
}

// save event to db
INT_PTR CToxProto::OnRecvAudioCall(WPARAM hContact, LPARAM lParam)
{
	PROTORECVEVENT *pre = (PROTORECVEVENT*)lParam;

	calls[hContact] = pre->lParam;

	char *message = mir_utf8encodeT(TranslateT("Incoming call"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = m_szModuleName;
	dbei.timestamp = pre->timestamp;
	dbei.eventType = DB_EVENT_AUDIO_RING;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	MEVENT hEvent = db_event_add(hContact, &dbei);

	CLISTEVENT cle = { sizeof(cle) };
	cle.flags |= CLEF_TCHAR;
	cle.hContact = hContact;
	cle.hDbEvent = hEvent;
	cle.hIcon = Skin_GetIconByHandle(GetIconHandle("audio_ring"));

	TCHAR szTooltip[256];
	mir_sntprintf(szTooltip, SIZEOF(szTooltip), TranslateT("Incoming call from %s"), pcli->pfnGetContactDisplayName(hContact, 0));
	cle.ptszTooltip = szTooltip;

	char szService[256];
	mir_snprintf(szService, SIZEOF(szService), "%s/Audio/Ring", GetContactProto(hContact));
	cle.pszService = szService;

	CallService(MS_CLIST_ADDEVENT, 0, (LPARAM)&cle);

	return hEvent;
}

// 
INT_PTR CToxProto::OnAudioRing(WPARAM wParam, LPARAM lParam)
{
	CLISTEVENT *cle = (CLISTEVENT*)lParam;
	CToxAudioCall *audioCall = new CToxIncomingAudioCall(this, cle->hContact);
	audioCall->Show();
	WindowList_Add(hAudioDialogs, audioCall->GetHwnd(), cle->hContact);

	return 0;
}

void CToxProto::OnAvCancel(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	CLISTEVENT *cle = (CLISTEVENT*)CallService(MS_CLIST_GETEVENT, hContact, 0);
	if (cle)
		CallService(MS_CLIST_REMOVEEVENT, hContact, cle->hDbEvent);

	char *message = mir_utf8encodeT(TranslateT("Call ended"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_END;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);

	WindowList_Broadcast(proto->hAudioDialogs, WM_AUDIO_END, hContact, 0);
}

/* AUDIO SENDING */

// outcoming audio flow
CToxOutgoingAudioCall::CToxOutgoingAudioCall(CToxProto *proto, MCONTACT hContact) :
	CToxAudioCall(proto, hContact)
{
}

void CToxOutgoingAudioCall::OnInitDialog()
{
	SetIcon("audio_end");
	CToxAudioCall::OnInitDialog();
}

void CToxOutgoingAudioCall::OnOk(CCtrlBase*)
{
	int32_t callId;
	int friendNumber = m_proto->GetToxFriendNumber(hContact);
	if (friendNumber == UINT32_MAX)
	{
		Close();
		return;
	}

	ToxAvCSettings *cSettings = m_proto->GetAudioCSettings();
	if (cSettings == NULL)
	{
		Close();
		return;
	}

	if (toxav_call(m_proto->toxAv, &callId, friendNumber, cSettings, 10) == TOX_ERROR)
	{
		m_proto->debugLogA(__FUNCTION__": failed to start outgoing call");
		return;
	}
	m_proto->calls[hContact] = callId;
	SetIcon("audio_call");

	char *message = mir_utf8encodeT(TranslateT("Outgoing call"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = m_proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_CALL;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);
}

void CToxOutgoingAudioCall::OnCancel(CCtrlBase*)
{
	if (!isCallStarted)
		toxav_cancel(m_proto->toxAv, m_proto->calls[hContact], 0, NULL);
	else
		toxav_hangup(m_proto->toxAv, m_proto->calls[hContact]);
}

INT_PTR CToxProto::OnSendAudioCall(WPARAM hContact, LPARAM)
{
	CToxAudioCall *audioCall = new CToxOutgoingAudioCall(this, hContact);
	audioCall->Show();
	WindowList_Add(hAudioDialogs, audioCall->GetHwnd(), hContact);

	return 0;
}

void CToxProto::OnAvReject(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	char *message = mir_utf8encodeT(TranslateT("Call ended"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_END;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);

	WindowList_Broadcast(proto->hAudioDialogs, WM_AUDIO_END, hContact, 0);
}

void CToxProto::OnAvCallTimeout(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	char *message = mir_utf8encodeT(TranslateT("Call ended"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_END;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);

	WindowList_Broadcast(proto->hAudioDialogs, WM_AUDIO_END, hContact, 0);
}

/* --- */

void CToxProto::OnAvStart(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		toxav_hangup(proto->toxAv, callId);
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		toxav_hangup(proto->toxAv, callId);
		return;
	}

	HWND hwnd = WindowList_Find(proto->hAudioDialogs, hContact);
	CToxAudioCall *audioCall = (CToxAudioCall*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	audioCall->OnStartCall();

	char *message = mir_utf8encodeT(TranslateT("Call started"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_START;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);

	if (toxav_prepare_transmission(proto->toxAv, callId, false) == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to prepare audio transmition");
		toxav_hangup(proto->toxAv, callId);
		return;
	}
}

void CToxProto::OnAvEnd(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	toxav_kill_transmission(proto->toxAv, callId);

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		return;
	}
	
	char *message = mir_utf8encodeT(TranslateT("Call ended"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_END;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);

	WindowList_Broadcast(proto->hAudioDialogs, WM_AUDIO_END, hContact, 0);
}

void CToxProto::OnAvPeerTimeout(void*, int32_t callId, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	int friendNumber = toxav_get_peer_id(proto->toxAv, callId, 0);
	if (friendNumber == TOX_ERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to get friend number");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	MCONTACT hContact = proto->GetContact(friendNumber);
	if (hContact == NULL)
	{
		proto->debugLogA(__FUNCTION__": failed to find contact");
		toxav_stop_call(proto->toxAv, callId);
		return;
	}

	CLISTEVENT *cle = (CLISTEVENT*)CallService(MS_CLIST_GETEVENT, hContact, 0);
	if (cle)
		CallService(MS_CLIST_REMOVEEVENT, hContact, cle->hDbEvent);

	char *message = mir_utf8encodeT(TranslateT("Call ended"));

	DBEVENTINFO dbei = { sizeof(dbei) };
	dbei.szModule = proto->m_szModuleName;
	dbei.timestamp = time(NULL);
	dbei.eventType = DB_EVENT_AUDIO_END;
	dbei.flags = DBEF_UTF;
	dbei.pBlob = (PBYTE)message;
	dbei.cbSize = mir_strlen(message);
	db_event_add(hContact, &dbei);

	WindowList_Broadcast(proto->hAudioDialogs, WM_AUDIO_END, hContact, 0);

	toxav_kill_transmission(proto->toxAv, callId);
}

//////

void CToxProto::OnFriendAudio(void*, int32_t callId, const int16_t *PCM, uint16_t size, void *arg)
{
	CToxProto *proto = (CToxProto*)arg;

	ToxAvCSettings cSettings;
	int cSettingsError = toxav_get_peer_csettings(proto->toxAv, callId, 0, &cSettings);
	if (cSettingsError != av_ErrorNone)
	{
		proto->debugLogA(__FUNCTION__": failed to get codec settings (%d)", cSettingsError);
		return;
	}

	if (cSettings.call_type != av_TypeAudio)
	{
		proto->debugLogA(__FUNCTION__": video call is unsupported");
		return;
	}

	WAVEFORMATEX wfx = { 0 };
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = cSettings.audio_channels;
	wfx.wBitsPerSample = cSettings.audio_bitrate / 1000;
	wfx.nSamplesPerSec = cSettings.audio_sample_rate;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	DWORD deviceId = proto->getDword("AudioOutputDeviceID", WAVE_MAPPER);

	HWAVEOUT hDevice;
	MMRESULT error = waveOutOpen(&hDevice, deviceId, &wfx, 0, 0, CALLBACK_NULL);
	if (error != MMSYSERR_NOERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to open audio device (%d)", error);

		TCHAR errorMessage[MAX_PATH];
		waveInGetErrorText(error, errorMessage, SIZEOF(errorMessage));
		CToxProto::ShowNotification(
			TranslateT("Unable to find output audio device"),
			errorMessage);

		return;
	}

	WAVEHDR header = { 0 };
	header.lpData = (LPSTR)PCM;
	header.dwBufferLength = size * wfx.nChannels * sizeof(int16_t);

	waveOutSetVolume(hDevice, 0xFFFF);

	error = waveOutPrepareHeader(hDevice, &header, sizeof(WAVEHDR));
	if (error != MMSYSERR_NOERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to prepare audio buffer (%d)", error);
		return;
	}

	error = waveOutWrite(hDevice, &header, sizeof(WAVEHDR));
	if (error != MMSYSERR_NOERROR)
	{
		proto->debugLogA(__FUNCTION__": failed to play audio samples (%d)", error);
		return;
	}

	while (waveOutUnprepareHeader(hDevice, &header, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING)
		Sleep(10);

	waveOutClose(hDevice);
}