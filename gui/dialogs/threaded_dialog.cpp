
// Copyright (C) 2011-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "threaded_dialog.h"
#endif

#ifdef WX_PRECOMP
# include "../../pch.h"
#else
#endif

#include "threaded_dialog.h"
#include "../../wxmailto_rc.h"
#include "../../gui/wxmailto_app.h"


using namespace wxMailto;

ThreadedModelessDialogLauncher::ThreadedModelessDialogLauncher()
{
}

wxmailto_status ThreadedModelessDialogLauncher::Show(wxWindowID id, const wxString& message, const wxString& caption)
{
	ThreadedModelessDialogLauncherData* data = new ThreadedModelessDialogLauncherData();
	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, id);
	if (!data || !evt)
	{
		delete data;
		delete evt;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	data->m_message = message;
	data->m_caption = caption;
	evt->SetPayload<ThreadedModelessDialogLauncherData*>(data);

	wxGetApp().AddExitBlocker();
	wxQueueEvent(wxGetApp().GetMainFrame(), evt);

	return ID_OK;
}

wxmailto_status ThreadedModalDialogLauncherData::InitializeAndLock()
{
	if (NULL==(m_dialog_mutex=new wxMutex()))
	{
		return LOGERROR(ID_OUT_OF_MEMORY);
	}
	if (NULL==(m_dialog_closes_condition=new wxCondition(*m_dialog_mutex)))
	{
		delete m_dialog_mutex;
		m_dialog_mutex = NULL;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	m_dialog_mutex->Lock();
	return ID_OK;
}

wxmailto_status ThreadedModalDialogLauncherData::Signal()
{
	if (!m_dialog_closes_condition)
		return LOGERROR(ID_NULL_POINTER);

	m_dialog_closes_condition->Signal();
	return ID_OK;
}

wxmailto_status ThreadedModalDialogLauncherData::Wait()
{
	if (!m_dialog_closes_condition)
		return LOGERROR(ID_NULL_POINTER);

	m_dialog_closes_condition->Wait();
	return ID_OK;
}


# include <wx/listimpl.cpp>
WX_DEFINE_LIST(ThreadedModalDialogLauncherDataList);


ThreadedModalDialogLauncher::ThreadedModalDialogLauncher()
{
}

wxmailto_status ThreadedModalDialogLauncher::Show(wxWindowID id)
{
	wxmailto_status status;
	ThreadedModalDialogLauncherData* data = new ThreadedModalDialogLauncherData();
	if (!data)
	{
		return LOGERROR(ID_OUT_OF_MEMORY);
	}
	else if (ID_OK!=(status=data->InitializeAndLock()))
	{
		delete data;
		return status;
	}

	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, id);
	if (!evt)
	{
		delete data;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	evt->SetPayload<ThreadedModalDialogLauncherData*>(data);
	wxQueueEvent(wxGetApp().GetMainFrame(), evt);

	return data->Wait();
}


IMPLEMENT_CLASS(ThreadedModalDialog, wxDialog)

ThreadedModalDialog::ThreadedModalDialog(wxWindow* parent,
                                         wxCriticalSection* dialog_critical_section,
                                         wxWindowID id, const wxString &title,
                                         const wxPoint &pos, const wxSize &size,
                                         long style, const wxString &name)
: wxDialog(parent, id, title, pos, size, style, name),
  m_dialog_critical_section(dialog_critical_section)
{
	m_launcher_list.DeleteContents(true);
}

wxmailto_status ThreadedModalDialog::AddLauncher(ThreadedModalDialogLauncherData* helper)
{
	//Only called from wxMailtoFrame, already in a critical section
	m_launcher_list.Append(helper);
	return ID_OK;
}

void ThreadedModalDialog::BeforeDestroyDialog()
{
	wxCriticalSectionLocker* locker = NULL;
	{
		if (m_dialog_critical_section)
			locker = new wxCriticalSectionLocker(*m_dialog_critical_section);

    ThreadedModalDialogLauncherDataList::iterator iter;
    for (iter=m_launcher_list.begin(); iter!=m_launcher_list.end(); ++iter)
    {
      ThreadedModalDialogLauncherData* current = *iter;
      if (!current) continue;
      
      current->Signal();
    }
    
    m_launcher_list.Clear();
		delete locker;
	}
}
