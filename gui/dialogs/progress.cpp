
// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "progress.h"
#endif

#ifdef WX_PRECOMP
# include "../../pch.h"
#else
# include <wx/config.h>
# include <wx/gauge.h>
# include <wx/sizer.h>
# include <wx/stattext.h>
# include <wx/textctrl.h>
#endif

#include "progress.h"

#include "../wxmailto_app.h"
#include "../wxmailto_frame.h"
#include "../../wxmailto_rc.h"


using namespace wxMailto;


ProgressInfo::ProgressInfo(	wxInt progress_id, wxInt current_value, wxInt total_value)
:	m_progress_id(progress_id),
  m_current_value(current_value),
  m_total_value(total_value)
{
}

ProgressCtrlInfo::ProgressCtrlInfo(wxInt progress_id, wxSizer* sizer, wxStaticText* static_text, wxGauge* gauge)
: m_progress_id(progress_id),
  m_sizer(sizer),
  m_static_text(static_text),
  m_gauge(gauge)
{
}

		
IMPLEMENT_CLASS(ProgressDialog, wxDialog)

BEGIN_EVENT_TABLE(ProgressDialog, wxDialog)
	EVT_CLOSE(ProgressDialog::OnClose)
END_EVENT_TABLE()


ProgressDialog::ProgressDialog(wxWindow* parent)
: wxDialog(parent, wxGetApp().GetWindowID(IDManager::ID_PROGRESS_DIALOG), _("Progress"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, "Progress"),
  m_progress_sizer(NULL)
{
}

ProgressDialog::~ProgressDialog()
{
	//delete m_progress_sizer; Deleted by wxWidgets
}

bool ProgressDialog::Show()
{
	ProgressDialogFunc(this, true, true);

	return wxDialog::Show(true);
}

void ProgressDialog::OnClose(wxCloseEvent& event)
{
	wxGetApp().GetMainFrame()->DestroyProgressDialog(); //Let mainframe know that this dialog is closing
	event.Veto();
}

void ProgressDialog::UpdateProgress(const ProgressInfo* progress_info)
{
	if (!progress_info)
		return;

	ProgressCtrlInfo* progress_ctrl_info = m_progress_ctrl_map[progress_info->m_progress_id];
	if (!progress_ctrl_info)
	{
		AddProgress(progress_info);
	}
	else
	{
		if (progress_info->m_current_value >= progress_info->m_total_value)
		{
			DeleteProgress(progress_ctrl_info);
		}
		else
		{
			progress_ctrl_info->m_gauge->SetRange(progress_info->m_total_value);
			progress_ctrl_info->m_gauge->SetValue(progress_info->m_current_value);
			if (progress_info->m_progress_text != wxEmptyString)
			{
				progress_ctrl_info->m_static_text->SetLabel(progress_info->m_progress_text);
			}
		}
	}
}

void ProgressDialog::AddProgress(const ProgressInfo* progress_info)
{
	wxBoxSizer* container_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* text_ctrl = new wxStaticText(this, -1, progress_info->m_progress_text, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	container_sizer->Add(text_ctrl, wxSizerFlags().Center().Expand().Border(wxRIGHT, 5));

	wxGauge* gauge_ctrl = new wxGauge(this, -1, progress_info->m_total_value);
	container_sizer->Add(gauge_ctrl, wxSizerFlags().Center().Expand());

	m_progress_sizer->Add(container_sizer, wxSizerFlags().Center().Expand().Border(wxTOP|wxLEFT|wxRIGHT, 5));
	m_progress_sizer->Layout();
	
	ProgressCtrlInfo* progress_ctrl_info = new ProgressCtrlInfo(progress_info->m_progress_id, container_sizer, text_ctrl, gauge_ctrl);
	m_progress_ctrl_map[progress_ctrl_info->m_progress_id] = progress_ctrl_info;

	gauge_ctrl->SetValue(progress_info->m_current_value);
}

void ProgressDialog::DeleteProgress(ProgressCtrlInfo* progress_ctrl_info)
{
	m_progress_sizer->Remove(progress_ctrl_info->m_sizer);
	m_progress_sizer->Layout();
	m_progress_ctrl_map.erase(progress_ctrl_info->m_progress_id); //ToDo, verify that this does not leak
}

wxSizer* ProgressDialog::ProgressDialogFunc(wxWindow* parent, wxBool call_fit, wxBool set_sizer)
{
	m_progress_sizer = new wxBoxSizer(wxVERTICAL);

	if (set_sizer)
	{
		parent->SetSizer(m_progress_sizer);
		if (call_fit)
			m_progress_sizer->SetSizeHints(parent);
	}

	return m_progress_sizer;
}
