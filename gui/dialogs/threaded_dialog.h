#ifndef _THREADED_DLG_H_
#define _THREADED_DLG_H_

// Copyright (C) 2011-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "threaded_dialog.h"
#endif
 
#include <wx/dialog.h>
#include <wx/list.h>
#include <wx/msgdlg.h>

#include "../../defines.h"
#include "../../wxmailto_errors.h"


namespace wxMailto
{

struct ThreadedModelessDialogLauncherData
{
friend class ThreadedModelessDialogLauncher;

	wxString m_message;
	wxString m_caption;
};

class ThreadedModelessDialogLauncher
{
public:
	ThreadedModelessDialogLauncher();
	wxmailto_status Show(wxWindowID id, const wxString& message, const wxString& caption=wxMessageBoxCaptionStr);
};



struct ThreadedModalDialogLauncherData
{
friend class ThreadedModalDialogLauncher;
	
	ThreadedModalDialogLauncherData() : m_dialog_mutex(NULL), m_dialog_closes_condition(NULL), m_status(ID_UNINITIALIZED) {}
	~ThreadedModalDialogLauncherData() {delete m_dialog_closes_condition; delete m_dialog_mutex;}

	void Signal() {m_dialog_closes_condition->Signal();}
	void SetStatus(const wxmailto_status& status) {m_status = status;}
	wxmailto_status GetStatus() const {return m_status;}

private:
	wxMutex* m_dialog_mutex; //Helper for condition
	wxCondition* m_dialog_closes_condition;
	wxmailto_status m_status;
};

WX_DECLARE_LIST(ThreadedModalDialogLauncherData, ThreadedModalDialogLauncherDataList);

class ThreadedModalDialogLauncher
{
public:
	ThreadedModalDialogLauncher();
	wxmailto_status Show(wxWindowID id);
};


class ThreadedModalDialog : public wxDialog
{
DECLARE_DYNAMIC_CLASS(ThreadedModalDialog)
public:
	ThreadedModalDialog(wxWindow* parent,
	                    wxCriticalSection* dialog_critical_section,
	                    wxWindowID id,
	                    const wxString &title,
	                    const wxPoint &pos = wxDefaultPosition,
	                    const wxSize &size = wxDefaultSize,
	                    long style = wxDEFAULT_DIALOG_STYLE,
	                    const wxString &name = wxDialogNameStr);
	
	wxmailto_status AddLauncher(ThreadedModalDialogLauncherData* launcher);

  virtual void BeforeDestroyDialog();

protected:
	ThreadedModalDialogLauncherDataList m_launcher_list;
  wxCriticalSection* m_dialog_critical_section;
};

}

#endif // _THREADED_DLG_H_
