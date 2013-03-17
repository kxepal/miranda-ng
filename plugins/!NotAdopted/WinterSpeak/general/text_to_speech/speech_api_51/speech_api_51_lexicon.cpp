//==============================================================================
// General Code, � 2002 Ryan Winter
//==============================================================================

#pragma warning(disable:4786)

#include "speech_api_51_lexicon.h"

#include "speech_api_51.h"

#include <general/debug/debug.h>
#include <general/resource.h>

#include <windows.h>
//#include <sapi.h>
//#include <sphelper.h>

extern HINSTANCE g_hInst;

//------------------------------------------------------------------------------
SpeechApi51Lexicon::SpeechApi51Lexicon(HWND window)
    :
    m_parent_window(window),
    m_window(0)
{
	CLASSCERR("SpeechApi51Lexicon::SpeechApi51Lexicon");
}

//------------------------------------------------------------------------------
SpeechApi51Lexicon::~SpeechApi51Lexicon()
{
	CLASSCERR("SpeechApi51Lexicon::~SpeechApi51Lexicon");
}

//------------------------------------------------------------------------------
bool
SpeechApi51Lexicon::display()
{
	CLASSCERR("SpeechApi51Lexicon::display");

    m_window = CreateDialog(
        g_hInst, 
        MAKEINTRESOURCE(IDD_TTS_LEXICON), 
        m_parent_window, 
        dialogEvent);

    if (!m_window)
    {
        return false;
    }

    ShowWindow(m_window, SW_SHOW);

/*    WNDCLASS wc;
   
    wc.style = 0;
!    wc.lpfnWndProc = (WNDPROC)MainWndProc; // Window procedure for this class.
    wc.cbClsExtra = 0;                  // No per-class extra data.
!    wc.cbWndExtra = 0;                  // No per-window extra data.
    wc.hInstance = hInstance;           // Application that owns the class.
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
    wc.lpszClassName = "Comdlg32WClass"; // Name used in call to CreateWindow.

    return (RegisterClass(&wc));*/

    return true;
}

//------------------------------------------------------------------------------
INT_PTR CALLBACK 
SpeechApi51Lexicon::dialogEvent(HWND hwndDlg, UINT uMsg, WPARAM wParam, 
    LPARAM lParam)
{

    return TRUE;
}

//------------------------------------------------------------------------------
void 
SpeechApi51Lexicon::addLexicon()
{
}

//------------------------------------------------------------------------------
void 
SpeechApi51Lexicon::deleteLexicon()
{
}

//------------------------------------------------------------------------------
void 
SpeechApi51Lexicon::displayLexicon()
{
}

//------------------------------------------------------------------------------
