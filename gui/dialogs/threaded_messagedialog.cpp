
// Copyright (C) 2011-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "threaded_messagedialog.h"
#endif

#ifdef WX_PRECOMP
# include "../../pch.h"
#else
# include <wx/sizer.h>
# include <wx/textctrl.h>
#endif

#include "threaded_messagedialog.h"

#include "../wxmailto_app.h"
#include "../../wxmailto_rc.h"


using namespace wxMailto;


IMPLEMENT_CLASS(ThreadedModelessMessageDialog, wxDialog)

BEGIN_EVENT_TABLE(ThreadedModelessMessageDialog, wxDialog)
  EVT_BUTTON(wxID_OK, ThreadedModelessMessageDialog::OnOk)
END_EVENT_TABLE()

ThreadedModelessMessageDialog::ThreadedModelessMessageDialog(wxWindow* parent)
: wxDialog(parent, wxGetApp().GetWindowID(IDManager::ID_MODELESS_MESSAGE_DIALOG), "ModelessMessageDialog", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, "ModelessMessageDialog")
{
}

ThreadedModelessMessageDialog::~ThreadedModelessMessageDialog()
{
	wxGetApp().RemoveExitBlocker(); //Blocker was added in ThreadedModelessDialogLauncher::Show
}

bool ThreadedModelessMessageDialog::Show(const wxString& message, const wxString& caption)
{
	ThreadedModelessMessageDialogFunc(this, true, true);

	SetTitle(caption);
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_MESSAGE)), wxTextCtrl)->SetValue(message);

	return wxDialog::Show(true);
}

void ThreadedModelessMessageDialog::OnOk(wxCommandEvent& WXUNUSED(event))
{
	Destroy();
}

wxSizer* ThreadedModelessMessageDialog::ThreadedModelessMessageDialogFunc(wxWindow* parent, wxBool call_fit, wxBool set_sizer)
{
	wxSizerFlags edit_flags = wxSizerFlags(1).Center().Expand().Border(wxALL,5);

	wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);

	wxTextCtrl* message_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_MESSAGE), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
	message_edit->Enable(false);
	dialog_sizer->Add(message_edit, edit_flags);

	wxSizer* ok_sizer = CreateStdDialogButtonSizer(wxOK|wxCANCEL);
	dialog_sizer->Add(ok_sizer, wxSizerFlags(1).Center().Expand().Border(wxLEFT|wxBOTTOM|wxRIGHT, 5));

	if (set_sizer)
	{
			parent->SetSizer(dialog_sizer);
			if (call_fit)
					dialog_sizer->SetSizeHints(parent);
	}

	return dialog_sizer;
}



IMPLEMENT_CLASS(ThreadedModalMessageDialog, ThreadedModalDialog)

BEGIN_EVENT_TABLE(ThreadedModalMessageDialog, ThreadedModalDialog)
  EVT_BUTTON(wxID_OK, ThreadedModalMessageDialog::OnOk)
END_EVENT_TABLE()


ThreadedModalMessageDialog::ThreadedModalMessageDialog(wxWindow* parent)
: ThreadedModalDialog(parent, NULL, wxGetApp().GetWindowID(IDManager::ID_MODAL_MESSAGE_DIALOG), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, "ModalMessageDialog")
{
}

bool ThreadedModalMessageDialog::Show(const wxString& message, const wxString& caption)
{
	ThreadedModalMessageDialogFunc(this, true, true);

	SetTitle(caption);
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_MESSAGE)), wxTextCtrl)->SetValue(message);

	return wxDialog::Show(true);
}

void ThreadedModalMessageDialog::OnOk(wxCommandEvent& WXUNUSED(event))
{
	{
		wxCriticalSectionLocker locker(*m_dialog_critical_section);
		
    ThreadedModalDialogLauncherDataList::iterator iter;
    for (iter=m_launcher_list.begin(); iter!=m_launcher_list.end(); ++iter)
    {
      if (*iter)
				(*iter)->SetStatus(ID_OK);
    }
	}
	
	Destroy();
}

wxSizer* ThreadedModalMessageDialog::ThreadedModalMessageDialogFunc(wxWindow* parent, wxBool call_fit, wxBool set_sizer)
{
	wxSizerFlags edit_flags = wxSizerFlags(1).Center().Expand().Border(wxALL,5);

	wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);

	wxTextCtrl* message_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_MESSAGE), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
	message_edit->Enable(false);
	dialog_sizer->Add(message_edit, edit_flags);

	wxSizer* ok_sizer = CreateStdDialogButtonSizer(wxOK|wxCANCEL);
	dialog_sizer->Add(ok_sizer, wxSizerFlags(1).Center().Expand().Border(wxLEFT|wxBOTTOM|wxRIGHT, 5));

	if (set_sizer)
	{
			parent->SetSizer(dialog_sizer);
			if (call_fit)
					dialog_sizer->SetSizeHints(parent);
	}

	return dialog_sizer;
}
