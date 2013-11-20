
// Copyright (C) 2009-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "about.h"
#endif

#ifdef WX_PRECOMP
# include "../../pch.h"
#else
# include <wx/button.h>
# include <wx/hyperlink.h>
# include <wx/sizer.h>
# include <wx/stattext.h>
# include <wx/textctrl.h>
#endif

#include "about.h"

#include "../../wxmailto_rc.h"
#include "../wxmailto_app.h"


using namespace wxMailto;

IMPLEMENT_CLASS(AboutDialog, wxDialog)

AboutDialog::AboutDialog(wxWindow* parent)
: wxDialog(parent, wxGetApp().GetWindowID(IDManager::ID_ABOUT_DIALOG), _("About"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, "About")
{
}

bool AboutDialog::Show(bool show)
{
	if (show)
	{
		AboutDialogFunc(this, true, true);
	}

	return wxDialog::Show(show);
}

wxSizer* AboutDialog::AboutDialogFunc(wxWindow *parent, bool call_fit, bool set_sizer)
{
	wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* version_text = new wxStaticText(parent, -1, wxString(WXMAILTO_TITLE)+wxString(" v")+wxString(WXMAILTO_VERSION_STR), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
	dialog_sizer->Add(version_text, wxSizerFlags().Center().Expand().Border(wxTOP|wxLEFT|wxRIGHT, 5));

	wxStaticText* credit_text = new wxStaticText(parent, -1, wxString(_("by "))+wxString("Frode Roxrud Gill"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
		dialog_sizer->Add(credit_text, wxSizerFlags().Center().Expand().Border(wxTOP|wxLEFT|wxRIGHT, 5));

#if WXMAILTO_VERSION_BETA
	wxDateTime compile_date;
	wxDateSpan expire_period(0, 0, 0, BETA_DAYS_TO_LIVE);
	if ((wxChar*)NULL != compile_date.ParseFormat(wxT(__DATE__), "%b %d %Y", wxDateTime((time_t)0)))
	{
		compile_date += expire_period;
		wxStaticText* beta_text = new wxStaticText(parent, -1, wxString(_("This BETA version expires "))+compile_date.FormatISODate(), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
		dialog_sizer->Add(beta_text, wxSizerFlags().Center().Expand().Border(wxTOP|wxLEFT|wxRIGHT, 5));
	}
	else
	{
		wxASSERT_MSG(false, _("Parsing compile time failed"));
	}
#endif

	wxHyperlinkCtrl* homepage_text = new wxHyperlinkCtrl(parent, -1, WXMAILTO_HOMEPAGE, WXMAILTO_HOMEPAGE, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
	dialog_sizer->Add(homepage_text, wxSizerFlags().Center().Expand().Border(wxTOP|wxLEFT|wxRIGHT, 5));

	wxTextCtrl* license_edit = new wxTextCtrl(parent, -1, WXMAILTO_LICENSE, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_AUTO_URL);
	dialog_sizer->Add(license_edit, wxSizerFlags(1).Center().Expand().Border(wxTOP|wxLEFT|wxRIGHT, 5));

	wxSizer* ok_sizer = CreateStdDialogButtonSizer(wxOK);
	dialog_sizer->Add(ok_sizer, wxSizerFlags().Center().Border(wxALL, 5));

	if (set_sizer)
	{
		parent->SetSizer(dialog_sizer);
		if (call_fit)
			dialog_sizer->SetSizeHints( parent );
	}

	return dialog_sizer;
}
