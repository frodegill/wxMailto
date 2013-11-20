#ifndef _WXMAILTO_FRAME_H_
#define _WXMAILTO_FRAME_H_

// Copyright (C) 2008-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "wxmailto_frame.h"
#endif

#include <wx/frame.h>
#include <wx/msgdlg.h>
#include <wx/timer.h>

#include "../defines.h"
#include "../wxmailto_errors.h"

#include "dialogs/datasource.h"
#include "dialogs/gpg_key.h"
#include "dialogs/progress.h"

namespace wxMailto
{

class wxMailto_Frame : public wxFrame
{
DECLARE_DYNAMIC_CLASS(wxMailto_Frame)
public:
	wxMailto_Frame(const wxString& title);
	virtual ~wxMailto_Frame();

	void OnQuit(wxCommandEvent& event);

	void OnCheckVersion(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnVersionCheckFailed(wxCommandEvent& event);
	void OnVersionCheckOK(wxCommandEvent& event);
	void OnNewVersionAvailable(wxCommandEvent& event);
	void OnNewBetaVersionAvailable(wxCommandEvent& event);

	//Threaded app events
	void OnNewAccount(wxThreadEvent& event);
	void OnModifyAccount(wxThreadEvent& event);
	void OnDeleteAccount(wxThreadEvent& event);
	void OnRescheduleAccountSync(wxThreadEvent& event);
	
	//Dialogs
	void OnThreadedModelessMessagebox(wxThreadEvent& event);
	void OnThreadedModalMessagebox(wxThreadEvent& event);

	wxCriticalSection m_datasource_dlg_lock;
	DatasourceDialog* m_datasource_dlg;
	void OnDatasourceDialog(wxThreadEvent& event);
	void DestroyDatasourceDialog();

	wxCriticalSection m_gpg_key_dlg_lock;
	GPGKeyDialog* m_gpg_key_dlg;
	void OnGPGKeyDialog(wxThreadEvent& event);
	void DestroyGPGKeyDialog();

	wxCriticalSection m_progress_dlg_lock;
	wxInt m_next_progress_id;
	wxInt GetNextProgressId() {	wxCriticalSectionLocker locker(m_progress_dlg_lock); return ++m_next_progress_id;}
	ProgressDialog* m_progress_dlg;
	void OnUpdateProgress(wxThreadEvent& event);
	void DestroyProgressDialog();
	
private:
	void SetTimeoutStatusText(const wxString& text, wxInt number, wxInt timeout);

public:
	wxmailto_status CheckVersion();

private:
	wxString m_application_title;
	wxTimer m_statusbar_timer0;
	wxTimer m_statusbar_timer1;

private:
	DECLARE_EVENT_TABLE()
};

}

#endif // _WXMAILTO_FRAME_H_
