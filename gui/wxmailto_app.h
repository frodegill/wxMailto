#ifndef _WXMAILTO_APP_H_
#define _WXMAILTO_APP_H_

// Copyright (C) 2008-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "wxmailto_app.h"
#endif

#include <wx/app.h>
#include "../defines.h"
#include "../wxmailto_rc.h"
#include "wxmailto_frame.h"
#include "wxmailto_lockers.h"

namespace wxMailto
{

#define STARTUP_FAILED (1)

class AppModuleManager;
class wxMailto_App : public wxApp
{
DECLARE_DYNAMIC_CLASS(wxMailto_App);
public:
	wxMailto_App();
	virtual ~wxMailto_App();
	
	virtual bool OnInit();
	virtual int OnRun();
	virtual int OnExit();

public:
	wxMailto_Frame* GetMainFrame() const {return m_main_frame;}
	AppModuleManager* GetAppModuleManager() const {return m_module_manager;}
	GlobalLockers* GetGlobalLockers() const {return m_lockers;}

private:
	wxmailto_status InitializeWxModules();
	wxmailto_status InitializeAppModules();
friend class AppModuleManager;
	void OnInitializeAppModulesFailed(const wxString& reason) {wxASSERT_MSG(false, reason); RequestExit();}
	void OnPrepareShutdownAppModulesFailed(const wxString& reason) {wxASSERT_MSG(false, reason);}

public:
	wxmailto_status LogError(const wxmailto_status status) const;
	wxmailto_status LogErrorMsg(const wxmailto_status status, const wxString& msg) const;
	wxmailto_status LogWarning(const wxmailto_status status) const;
	void RequestExit();
	bool IsExitRequested() {wxCriticalSectionLocker locker(GetGlobalLockers()->m_block_exit_lock); return m_exit_requested;}
	void AddExitBlocker();
	void RemoveExitBlocker();

public:
	wxWindowID GetWindowID(IDManager::IDref id_ref) const {return m_id_mgr->GetId(id_ref);}

private:
	wxMailto_Frame* m_main_frame;
	AppModuleManager* m_module_manager;
	GlobalLockers* m_lockers;
	IDManager* m_id_mgr;

	int m_block_exit_refcount;
	bool m_exit_requested;
};

}

DECLARE_APP(wxMailto::wxMailto_App);

#endif // _WXMAILTO_APP_H_
