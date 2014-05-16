
// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "gpg_key.h"
#endif

#ifdef WX_PRECOMP
# include "../../pch.h"
#else
# include <wx/config.h>
# include <wx/listctrl.h>
# include <wx/sizer.h>
# include <wx/stattext.h>
#endif

#include "gpg_key.h"

#include "../app_module_manager.h"
#include "../wxmailto_app.h"
#include "../../security/gpg_key.h"
#include "../../wxmailto_errors.h"
#include "../../wxmailto_rc.h"


using namespace wxMailto;

IMPLEMENT_CLASS(GPGKeyDialog, ThreadedModalDialog)

BEGIN_EVENT_TABLE(GPGKeyDialog, ThreadedModalDialog)
  EVT_BUTTON(wxID_OK, GPGKeyDialog::OnOk)
END_EVENT_TABLE()


GPGKeyDialog::GPGKeyDialog(wxWindow* parent, wxCriticalSection* dialog_critical_section)
: ThreadedModalDialog(parent, dialog_critical_section, wxGetApp().GetWindowID(IDManager::ID_GPG_KEY_DIALOG), _("GPG Key"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, "GPG Key")
{
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, &GPGKeyDialog::OnGenerateNewKeyClicked, this, wxGetApp().GetWindowID(IDManager::IDB_GENERATE_NEW_KEY));
}

bool GPGKeyDialog::Show()
{
	GPGKeyDialogFunc(this, true, true);

	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return false;
#if 0
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
#endif
	return wxDialog::Show(true);
}

void GPGKeyDialog::OnGenerateNewKeyClicked(wxCommandEvent& WXUNUSED(event))
{
}

void GPGKeyDialog::OnOk(wxCommandEvent& WXUNUSED(event))
{
#if 0
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
#endif
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
	wxGetApp().GetMainFrame()->DestroyGPGKeyDialog();
}

wxSizer* GPGKeyDialog::GPGKeyDialogFunc(wxWindow* parent, wxBool call_fit, wxBool set_sizer)
{
#if 0
	wxSizerFlags text_flags = wxSizerFlags(0).Left().Border(wxRIGHT, 5);
	wxSizerFlags edit_flags = wxSizerFlags(1).Left().Expand();
#endif
	wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);

	wxListCtrl* key_listctrl = new wxListCtrl(parent, wxGetApp().GetWindowID(IDManager::IDL_KEYS),
	                            wxDefaultPosition, wxDefaultSize,
	                            wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_SORT_ASCENDING|wxLC_VRULES);
	dialog_sizer->Add(key_listctrl, wxSizerFlags(1).Center().Expand().Border(wxALL, 5));
	PopulateKeyListCtrl(key_listctrl);
#if 0
	wxFlexGridSizer* form_sizer = new wxFlexGridSizer(5, 2, 0, 0);

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
#endif
	
 	wxButton* generate_new_key_button = new wxButton(parent, wxGetApp().GetWindowID(IDManager::IDB_GENERATE_NEW_KEY));
	dialog_sizer->Add(generate_new_key_button, wxSizerFlags(1).Center().Expand().Border(wxALL, 5));
 	
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

wxmailto_status GPGKeyDialog::PopulateKeyListCtrl(wxListCtrl* key_listctrl)
{
	if (!key_listctrl)
		return LOGERROR(ID_NULL_POINTER);

	key_listctrl->DeleteAllItems();
	key_listctrl->InsertColumn(0, _("e-mail"));
	key_listctrl->InsertColumn(1, _("expires"));
	key_listctrl->InsertColumn(2, _("fingerprint"));

	wxmailto_status status = ID_OK;
	wxBool truncated;
	GPGKeyList key_list;
	if (ID_OK!=(status==wxGetApp().GetAppModuleManager()->GetGPGManager()->GetSecretKeys(key_list, truncated)))
		return status;

	long row = 0;
	GPGKeyList::iterator iter;
	for (iter=key_list.begin(); iter!=key_list.end(); ++iter)
	{
		const GPGKey* key = *iter;
		if (!key) continue;

		long index = key_listctrl->InsertItem(row++, key->GetEmail());
    key_listctrl->SetItem(index, 1, _("expires"));
    key_listctrl->SetItem(index, 2, key->GetFingerprint());
	}

	return ID_OK;
}
