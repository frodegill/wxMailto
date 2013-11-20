
// Copyright (C) 2009-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "version_check.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/sizer.h>
# include <wx/protocol/http.h>
# include <wx/xml/xml.h>
#endif

#include "version_check.h"
#include "../wxmailto_rc.h"
#include "wxmailto_app.h"

using namespace wxMailto;


VersionCheck::VersionCheck(wxMailto_Frame* frame)
: wxThread(),
	m_frame(frame)
{
}

wxThread::ExitCode VersionCheck::Entry()
{
	wxHTTP http;
	if (!http.Connect(VERSION_SERVERNAME, 80))
	{
		ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_FAILED));
		return (wxThread::ExitCode)-1;
	}

	http.SetTimeout(60);
	wxXmlDocument version_info;
	wxInputStream* http_stream = http.GetInputStream(wxString("http://")+wxString(VERSION_SERVERNAME)+wxString(VERSION_URI)+wxString("?v=")+wxString(WXMAILTO_VERSION_STR));
	if (!http_stream || !version_info.Load(*http_stream))
	{
		delete http_stream;
		ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_FAILED));
		return (wxThread::ExitCode)-1;
	}
	delete http_stream;

	wxXmlNode* root_node = version_info.GetRoot();
	wxASSERT_MSG(root_node, _("Version-info document lacks root node"));
	if (!root_node || root_node->GetName()!="version")
	{
		ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_FAILED));
		return (wxThread::ExitCode)-1;
	}

	//Parse and find most recent version
	wxUint8 bflag[2]; //0==Stable, 1==BETA
	wxDouble release[2];
	wxString homepage[2];
	wxString changelog[2];
	bflag[0] = bflag[1] = 0;
	release[0] = release[1] = 0.0;
	wxInt index;
	wxXmlNode* child_node = root_node->GetChildren();
#if (wxABI_VERSION < 20900)
	wxXmlProperty* child_property;
#else
	wxXmlAttribute* child_property;
#endif

	while (child_node)
	{
		if ("stable" == child_node->GetName())
		{
			index = 0;
		}
		else if ("beta" == child_node->GetName())
		{
			index = 1;
		}
		else
		{
			wxASSERT_MSG(false, "Unknown XML child node");
			child_node = child_node->GetNext();
			continue;
		}

#if (wxABI_VERSION < 20900)
		child_property = child_node->GetProperties();
#else
		child_property = child_node->GetAttributes();
#endif

		while (child_property)
		{
			if ("version"== child_property->GetName())
			{
				if (child_property->GetValue().ToDouble(&release[index]))
					bflag[index] |= 1;
			}
			else if ("uri"==child_property->GetName() || "homepage"==child_property->GetName())
			{
				homepage[index] = child_property->GetValue();
#if !WXMAILTO_VERSION_BETA
				bflag[index] |= 2;
#endif
			}
			else if ("changelog" == child_property->GetName())
			{
				changelog[index] = child_property->GetValue();
#if WXMAILTO_VERSION_BETA
				bflag[index] |= 2;
#endif
			}
			child_property = child_property->GetNext();
		}
		child_node = child_node->GetNext();
	}

	if (3!=(bflag[0]&3) && 3!=(bflag[1]&3)) //No valid release version information
	{
		ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_FAILED));
		return (wxThread::ExitCode)-1;
	}

	if (3==(bflag[0]&3) && WXMAILTO_VERSION<release[0]) //New Stable release
	{
		ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_NEW_VERSION_AVAILABLE), changelog[0]);
		return (wxThread::ExitCode)0;	
	}

	if (3==(bflag[1]&3) && WXMAILTO_VERSION<release[1]) //New BETA release
	{
#if WXMAILTO_VERSION_BETA
		ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_NEW_BETA_VERSION_AVAILABLE), changelog[1]);
#else
		ReportStatus(IDC_NEW_BETA_VERSION_AVAILABLE, homepage[1]);
#endif
		return (wxThread::ExitCode)0;	
	}

	ReportStatus(wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_OK));
	return (wxThread::ExitCode)0;	
}

void VersionCheck::ReportStatus(wxInt status, const wxString& status_text) const
{
	wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, status);
	event.SetString(status_text);
	::wxPostEvent(m_frame, event);
}


IMPLEMENT_CLASS(NewVersionDialog, wxDialog)

NewVersionDialog::NewVersionDialog(wxWindow* parent, const wxString& uri, wxBool is_beta)
: wxDialog(parent, wxGetApp().GetWindowID(IDManager::ID_NEWVERSION_DIALOG), is_beta ? _("New BETA release available") : _("New release available"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, _("New version")),
  m_uri(uri),
  m_is_beta(is_beta)
{
	Bind(wxEVT_COMMAND_HTML_LINK_CLICKED, &NewVersionDialog::OnLinkClicked, this, wxGetApp().GetWindowID(IDManager::IDD_HTMLWINDOW));
}

NewVersionDialog::~NewVersionDialog()
{
}

bool NewVersionDialog::Show(bool show)
{
	if (show)
	{
		NewVersionDialogFunc(this, true, true);
	}
	
	return wxDialog::Show(show);
}

void NewVersionDialog::OnLinkClicked(wxHtmlLinkEvent& event)
{
	wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref());
}

wxSizer* NewVersionDialog::NewVersionDialogFunc(wxWindow* parent, bool call_fit, bool set_sizer)
{
	wxBoxSizer* dialog_sizer = new wxBoxSizer(wxVERTICAL);

	wxHtmlWindow* html_window = new wxHtmlWindow(parent, wxGetApp().GetWindowID(IDManager::IDD_HTMLWINDOW), wxDefaultPosition, wxSize(480, 320), wxHW_SCROLLBAR_AUTO);
	html_window->SetBorders(5);
	html_window->LoadPage(m_uri);
	html_window->SetSize(html_window->GetInternalRepresentation()->GetWidth(), html_window->GetInternalRepresentation()->GetHeight());
	dialog_sizer->Add(html_window, wxSizerFlags(1).Expand().Border(wxALL, 0));

	wxSizer* buttons_sizer = CreateStdDialogButtonSizer(wxOK);
	dialog_sizer->Add(buttons_sizer, wxSizerFlags().Center().Border(wxALL, 5));

	if (set_sizer)
	{
		parent->SetSizer(dialog_sizer);
		if (call_fit)
			dialog_sizer->SetSizeHints(parent);
	}

	return dialog_sizer;
}
