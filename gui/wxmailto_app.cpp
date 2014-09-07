
// Copyright (C) 2008-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "wxmailto_app.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/config.h>
# include <wx/dir.h>
# include <wx/image.h>
# include <wx/socket.h>
# include <wx/stdpaths.h>
#endif

#include "wxmailto_app.h"

#include "app_module_manager.h"

IMPLEMENT_APP(wxMailto::wxMailto_App);


using namespace wxMailto;

IMPLEMENT_CLASS(wxMailto_App, wxApp)

wxMailto_App::wxMailto_App()
: wxApp(),
  m_main_frame(NULL),
  m_module_manager(NULL),
  m_lockers(NULL),
  m_id_mgr(NULL),
  m_block_exit_refcount(0),
  m_exit_requested(false)
{
}

wxMailto_App::~wxMailto_App()
{
//	delete m_main_frame; //Deleted by wxWidgets
	delete m_id_mgr;
	delete m_lockers;
}

bool wxMailto_App::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	m_lockers = new GlobalLockers();
	if (!m_lockers)
		return false;

	if (ID_OK != InitializeWxModules())
		return false;

	m_id_mgr = new IDManager();
	if (!m_id_mgr || !m_id_mgr->OnInit())
	{
		delete m_id_mgr;
		return false;
	}
	
	//Initialize main frame
	m_main_frame = new wxMailto_Frame(_(WXMAILTO_TITLE));
	if (!m_main_frame)
	{
		wxASSERT_MSG(false, _("wxMailto_Frame is NULL"));
		return false;
	}

	m_main_frame->Show(true);

	return true;
}

int wxMailto_App::OnRun()
{
	InitializeAppModules();

#if WXMAILTO_VERSION_BETA
# if !defined(_DEBUG) || defined(FORCE_VERSION_CHECK)
	m_main_frame->CheckVersion();
# endif
#endif

	return wxApp::OnRun();
}

int wxMailto_App::OnExit()
{
	if (m_module_manager)
	{
		m_module_manager->PrepareShutdown();
		delete m_module_manager;
	}

	wxConfigBase* config = wxConfigBase::Set(NULL);
	delete config;
	
	return wxApp::OnExit();
}

wxmailto_status wxMailto_App::InitializeWxModules()
{
	srand(time(NULL));

	::wxInitAllImageHandlers();
	if (!wxSocketBase::Initialize())
	{
		wxASSERT_MSG(false, _("wxSocketBase::Initialize() failed"));
		return LOGERROR(ID_GENERIC_ERROR);
	}

	//Create app settings directory if not already present
	wxString config_dir = wxStandardPaths::Get().GetUserLocalDataDir();
	if (!wxDir::Exists(config_dir)) 
	{
		::wxMkdir(config_dir);
	}

	//Init config
	wxConfig* config = new wxConfig(GetAppName(), wxEmptyString, ".settings", wxEmptyString, wxCONFIG_USE_SUBDIR|wxCONFIG_USE_LOCAL_FILE);
	if (!config)
	{
		wxASSERT_MSG(false, _("Could not initialize ConfigBase"));
		return LOGERROR(ID_GENERIC_ERROR);
	}
	wxConfigBase::Set(config);
	return ID_OK;
}

wxmailto_status wxMailto_App::InitializeAppModules()
{
	if (NULL==(m_module_manager=new AppModuleManager()))
		return LOGERROR(ID_OUT_OF_MEMORY);

	return m_module_manager->Initialize();
}

wxmailto_status wxMailto_App::LogError(const wxmailto_status status) const
{
	return status;
}

wxmailto_status wxMailto_App::LogErrorMsg(const wxmailto_status status, const wxString& msg) const
{
	wxFAIL_MSG(msg);
	return status;
}

wxmailto_status wxMailto_App::LogWarning(const wxmailto_status status) const
{
	return status;
}

wxmailto_status wxMailto_App::LogDebug(const wxString& msg) const
{
	{
		wxCriticalSectionLocker locker(GetGlobalLockers()->m_log_lock);

		fprintf(stderr, "%s\n", msg.ToUTF8().data());
		fflush(stderr);
	}
	return ID_OK;
}

void wxMailto_App::RequestExit()
{
	{
		wxCriticalSectionLocker locker(GetGlobalLockers()->m_block_exit_lock);
		m_exit_requested=true;
		if (0==m_block_exit_refcount)
			GetTopWindow()->Close();
	}
}

void wxMailto_App::AddExitBlocker()
{
	{
		wxCriticalSectionLocker locker(GetGlobalLockers()->m_block_exit_lock);
		m_block_exit_refcount++;
	}
}

void wxMailto_App::RemoveExitBlocker()
{
	{
		wxCriticalSectionLocker locker(GetGlobalLockers()->m_block_exit_lock);
		m_block_exit_refcount--;
		if (m_exit_requested && 0==m_block_exit_refcount)
			GetTopWindow()->Close();
	}
}
