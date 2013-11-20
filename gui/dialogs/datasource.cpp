
// Copyright (C) 2009-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "datasource.h"
#endif

#ifdef WX_PRECOMP
# include "../../pch.h"
#else
# include <wx/config.h>
# include <wx/sizer.h>
# include <wx/stattext.h>
# include <wx/textctrl.h>
#endif

#include "datasource.h"

#include "../wxmailto_app.h"
#include "../../wxmailto_rc.h"


using namespace wxMailto;

IMPLEMENT_CLASS(DatasourceDialog, ThreadedModalDialog)

BEGIN_EVENT_TABLE(DatasourceDialog, ThreadedModalDialog)
  EVT_BUTTON(wxID_OK, DatasourceDialog::OnOk)
END_EVENT_TABLE()


DatasourceDialog::DatasourceDialog(wxWindow* parent, wxCriticalSection* dialog_critical_section)
: ThreadedModalDialog(parent, dialog_critical_section, wxGetApp().GetWindowID(IDManager::ID_DATASOURCE_DIALOG), _("Datasource"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, "Datasource")
{
}

bool DatasourceDialog::Show()
{
	DatasourceDialogFunc(this, true, true);

	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return false;

	wxString value;
	config->Read("Server", &value, wxEmptyString);
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_SERVER)), wxTextCtrl)->SetValue(value);

	config->Read("Port", &value, "3306");
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_PORT)), wxTextCtrl)->SetValue(value);

	config->Read("Database", &value, wxEmptyString);
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_DATABASE)), wxTextCtrl)->SetValue(value);

	config->Read("Username", &value, wxEmptyString);
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_USERNAME)), wxTextCtrl)->SetValue(value);

	config->Read("Password", &value, wxEmptyString);
	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_PASSWORD)), wxTextCtrl)->SetValue(value);

	return wxDialog::Show(true);
}

void DatasourceDialog::OnOk(wxCommandEvent& WXUNUSED(event))
{
	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (config)
	{
		config->Write("Server", wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_SERVER)), wxTextCtrl)->GetValue());
		config->Write("Port", wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_PORT)), wxTextCtrl)->GetValue());
		config->Write("Database", wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_DATABASE)), wxTextCtrl)->GetValue());
		config->Write("Username", wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_USERNAME)), wxTextCtrl)->GetValue());
		config->Write("Password", wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_PASSWORD)), wxTextCtrl)->GetValue());
		config->Flush();
	}
	
	{
		wxCriticalSectionLocker locker(*m_dialog_critical_section);
		
    ThreadedModalDialogLauncherDataList::iterator iter;
    for (iter=m_launcher_list.begin(); iter!=m_launcher_list.end(); ++iter)
    {
			ThreadedModalDialogLauncherData* current = *iter;
      if (!current) continue;
      
      current->SetStatus(ID_OK);
    }
	}

	BeforeDestroyDialog();
	wxGetApp().GetMainFrame()->DestroyDatasourceDialog();
}

wxSizer* DatasourceDialog::DatasourceDialogFunc(wxWindow* parent, wxBool call_fit, wxBool set_sizer)
{
	wxSizerFlags text_flags = wxSizerFlags(0).Left().Border(wxRIGHT, 5);
	wxSizerFlags edit_flags = wxSizerFlags(1).Left().Expand();

	wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* form_sizer = new wxFlexGridSizer(3, 2, 0, 0);

	wxStaticText* server_text = new wxStaticText(parent, -1, wxString(_("Server")), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	form_sizer->Add(server_text, text_flags);

	wxTextCtrl* server_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_SERVER), wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	form_sizer->Add(server_edit, edit_flags);

	wxStaticText* port_text = new wxStaticText(parent, -1, wxString(_("Port")), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	form_sizer->Add(port_text, text_flags);

	wxTextCtrl* port_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_PORT), wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	form_sizer->Add(port_edit, edit_flags);

	wxStaticText* database_text = new wxStaticText(parent, -1, wxString(_("Database")), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	form_sizer->Add(database_text, text_flags);

	wxTextCtrl* database_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_DATABASE), wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	form_sizer->Add(database_edit, edit_flags);

	wxStaticText* username_text = new wxStaticText(parent, -1, wxString(_("Username")), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	form_sizer->Add(username_text, text_flags);

	wxTextCtrl* username_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_USERNAME), wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	form_sizer->Add(username_edit, edit_flags);

	wxStaticText* password_text = new wxStaticText(parent, -1, wxString(_("Password")), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	form_sizer->Add(password_text, text_flags);

	wxTextCtrl* password_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_PASSWORD), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
	form_sizer->Add(password_edit, edit_flags);

	dialog_sizer->Add(form_sizer, wxSizerFlags(1).Center().Expand().Border(wxALL, 5));

	wxSizer* ok_sizer = CreateStdDialogButtonSizer(wxOK|wxCANCEL);
	dialog_sizer->Add(ok_sizer, wxSizerFlags(1).Center().Expand().Border(wxLEFT|wxBOTTOM|wxRIGHT, 5));

	if (set_sizer)
	{
		parent->SetSizer(dialog_sizer);
		if (call_fit)
			dialog_sizer->SetSizeHints( parent );
	}

	return dialog_sizer;
}
