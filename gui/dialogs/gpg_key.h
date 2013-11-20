#ifndef _GPG_KEY_DLG_H_
#define _GPG_KEY_DLG_H_

// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "gpg_key.h"
#endif
 
#include "threaded_dialog.h"
#include "../../defines.h"

namespace wxMailto
{

class GPGKeyDialog : public ThreadedModalDialog
{
DECLARE_DYNAMIC_CLASS(GPGKeyDialog)
public:
	GPGKeyDialog(wxWindow* parent, wxCriticalSection* dialog_critical_section);

	bool Show();

	void OnGenerateNewKeyClicked(wxCommandEvent& event);
	void OnOk(wxCommandEvent& event);

private:
	wxSizer* GPGKeyDialogFunc(wxWindow* parent, wxBool call_fit=true, wxBool set_sizer=true);
	wxmailto_status PopulateKeyListCtrl(wxListCtrl* key_listctrl);

private:
	wxString* m_server;
	wxString* m_port;
	wxString* m_database;
	wxString* m_username;
	wxString* m_password;

private:
    DECLARE_EVENT_TABLE()
};

}

#endif // _GPG_KEY_DLG_H_
