
// Copyright (C) 2009-2014  Frode Roxrud Gill
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
#include "../../storage/config_helper.h"
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

	SafeString plaintext_connectionstring;
	SafeString default_value;
	SafeString dsn_salt;
	if (ID_OK!=default_value.SetStr("", NOOP) ||
	    ID_OK!=dsn_salt.StrDup(DSN_SALT))
	{
		return false;
	}

	if (ID_OK != ConfigHelper::ReadEncrypted(CONFIG_CONNECTIONSTRING, plaintext_connectionstring, default_value, dsn_salt))
	{
		return false;
	}

	const char* plaintext_connectionstring_str;
	if (!plaintext_connectionstring.GetStr(plaintext_connectionstring_str))
		return false;

	wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_CONNECTIONSTRING)), wxTextCtrl)->SetValue(plaintext_connectionstring_str);

	return wxDialog::Show(true);
}

void DatasourceDialog::OnOk(wxCommandEvent& WXUNUSED(event))
{
	SafeString plaintext_connectionstring;
	SafeString dsn_salt;
	if (ID_OK!=plaintext_connectionstring.StrDup(wxDynamicCast(FindWindow(wxGetApp().GetWindowID(IDManager::IDD_CONNECTIONSTRING)), wxTextCtrl)->GetValue().ToUTF8()) ||
	    ID_OK!=dsn_salt.StrDup(DSN_SALT))
	{
		return;
	}

	ConfigHelper::WriteEncrypted(CONFIG_CONNECTIONSTRING, plaintext_connectionstring, dsn_salt, true);
	
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

	wxStaticText* connectionstring_text = new wxStaticText(parent, -1, wxString(_("ConnectionString")), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	form_sizer->Add(connectionstring_text, text_flags);

	wxTextCtrl* connectionstring_edit = new wxTextCtrl(parent, wxGetApp().GetWindowID(IDManager::IDD_CONNECTIONSTRING), wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	form_sizer->Add(connectionstring_edit, edit_flags);

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
