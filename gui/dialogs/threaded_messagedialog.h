#ifndef _THREADED_MESSAGEDIALOG_H_
#define _THREADED_MESSAGEDIALOG_H_

// Copyright (C) 2011  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "threaded_messagedialog.h"
#endif

#include <wx/msgdlg.h>

#include "threaded_dialog.h"
#include "../../defines.h"

namespace wxMailto
{

class ThreadedModelessMessageDialog : public wxDialog
{
DECLARE_DYNAMIC_CLASS(ThreadedModelessMessageDialog)
public:
	ThreadedModelessMessageDialog(wxWindow* parent);
	~ThreadedModelessMessageDialog();

	bool Show(const wxString& message, const wxString& caption=wxMessageBoxCaptionStr);

	void OnOk(wxCommandEvent& event);

private:
	wxSizer* ThreadedModelessMessageDialogFunc(wxWindow* parent, wxBool call_fit=true, wxBool set_sizer=true);

private:
	wxString m_message;
	wxString m_caption;

private:
    DECLARE_EVENT_TABLE()
};


class ThreadedModalMessageDialog : public ThreadedModalDialog
{
DECLARE_DYNAMIC_CLASS(ThreadedModalMessageDialog)
public:
	ThreadedModalMessageDialog(wxWindow* parent);

	bool Show(const wxString& message, const wxString& caption=wxMessageBoxCaptionStr);

	void OnOk(wxCommandEvent& event);

private:
	wxSizer* ThreadedModalMessageDialogFunc(wxWindow* parent, wxBool call_fit=true, wxBool set_sizer=true);

private:
    DECLARE_EVENT_TABLE()
};

}

#endif // _THREADED_MESSAGEDIALOG_H_
