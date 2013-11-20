#ifndef _PROGRESS_DLG_H_
#define _PROGRESS_DLG_H_

// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "progress.h"
#endif

#include <wx/gauge.h>
#include <wx/stattext.h>
 
#include "threaded_dialog.h"
#include "../../defines.h"

namespace wxMailto
{

class ProgressInfo {
friend class ProgressDialog;
private:
	ProgressInfo() {}
public:
	ProgressInfo(wxInt progress_id, wxInt current_value, wxInt total_value);
	void SetProgressText(const wxString& text) {m_progress_text = text;} //Only needed in first progress update or when text is changed

private:
	wxInt m_progress_id;
	wxInt m_current_value;
	wxInt m_total_value;
	wxString m_progress_text;
};

class ProgressCtrlInfo {
friend class ProgressDialog;
private:
	ProgressCtrlInfo() {}
public:
	ProgressCtrlInfo(wxInt progress_id, wxSizer* sizer, wxStaticText* static_text, wxGauge* gauge);

private:
	wxInt m_progress_id;
	wxSizer* m_sizer;
	wxStaticText* m_static_text;
	wxGauge* m_gauge;
};
WX_DECLARE_HASH_MAP(wxInt, ProgressCtrlInfo*, wxIntegerHash, wxIntegerEqual, ProgressCtrlInfoMap);


class ProgressDialog : public wxDialog
{
DECLARE_DYNAMIC_CLASS(ProgressDialog)
public:
	ProgressDialog(wxWindow* parent);
	~ProgressDialog();

	bool Show();
	void OnClose(wxCloseEvent& event);

	void UpdateProgress(const ProgressInfo* progress_info);

private:
	void AddProgress(const ProgressInfo* progress_info);
	void DeleteProgress(ProgressCtrlInfo* progress_ctrl_info);

private:
	wxSizer* ProgressDialogFunc(wxWindow* parent, wxBool call_fit=true, wxBool set_sizer=true);

private:
	wxBoxSizer* m_progress_sizer;
	ProgressCtrlInfoMap m_progress_ctrl_map;
	
private:
    DECLARE_EVENT_TABLE()
};

}

#endif // _PROGRESS_DLG_H_
