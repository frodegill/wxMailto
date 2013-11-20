#ifndef _VERSION_CHECK_H_
#define _VERSION_CHECK_H_

// Copyright (C) 2009-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "version_check.h"
#endif

#include <wx/dialog.h>
#include <wx/thread.h>
#include <wx/html/htmlwin.h>

#include "wxmailto_frame.h"

namespace wxMailto
{

class VersionCheck : public wxThread
{
public:
	VersionCheck(wxMailto_Frame* frame);
	virtual wxThread::ExitCode Entry();

	void ReportStatus(wxInt status, const wxString& status_text = wxEmptyString) const;

private:
	wxMailto_Frame* m_frame;
};


class NewVersionDialog : public wxDialog
{
DECLARE_DYNAMIC_CLASS(NewVersionDialog)
public:
	NewVersionDialog(wxWindow* parent, const wxString& uri, wxBool is_beta=false);
	virtual ~NewVersionDialog();

	virtual bool Show(bool show = true);

	void OnLinkClicked(wxHtmlLinkEvent& event);

private:
	wxSizer* NewVersionDialogFunc(wxWindow* parent, bool call_fit=true, bool set_sizer=true);

private:
	wxString m_uri;
	wxBool m_is_beta;
};

}

#endif // _VERSION_CHECK_H_
