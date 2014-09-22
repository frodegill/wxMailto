
// Copyright (C) 2010-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "app_module_manager.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "app_module_manager.h"
#if 0
#include "../storage/database_update.h"
#endif

using namespace wxMailto;


ModuleInitializer::ModuleInitializer(wxMailto_Module* module)
: wxThread(),
  m_module(module)
{
}

wxThread::ExitCode ModuleInitializer::Entry()
{
	if (ID_OK == m_module->Initialize())
		return (wxThread::ExitCode)0;

	wxGetApp().GetAppModuleManager()->OnInitializeModuleFailed(m_module);
	m_module = NULL;
	return (wxThread::ExitCode)-1;
}


ModulePrepareShutdowner::ModulePrepareShutdowner(wxMailto_Module* module)
: wxThread(wxTHREAD_JOINABLE),
  m_module(module)
{
}

wxThread::ExitCode ModulePrepareShutdowner::Entry()
{
	if (ID_OK == m_module->PrepareShutdown())
		return (wxThread::ExitCode)0;

	wxGetApp().GetAppModuleManager()->OnPrepareShutdownModuleFailed(m_module);
	m_module = NULL;
	return (wxThread::ExitCode)-1;
}


AppModuleManager::AppModuleManager()
: m_allocated_modules(NULL),
  m_registered_modules(NULL),
  m_registered_modules_semaphores(NULL)
{
}

AppModuleManager::~AppModuleManager()
{
	wxInt i;
	for (i=0; i<wxMailto_Module::MODULECOUNT; i++)
	{
		if (m_allocated_modules) delete m_allocated_modules[i];
		//delete m_registered_modules[i]; Dn't delete. Objects are only referenced here, they are allocated in m_allocated_modules 
		if (m_registered_modules_semaphores) delete m_registered_modules_semaphores[i];
	}
	delete[] m_allocated_modules;
	delete[] m_registered_modules;
	delete[] m_registered_modules_semaphores;
}

wxmailto_status AppModuleManager::Initialize()
{
	wxmailto_status status;
	//Allocate arrays
	if (!(m_allocated_modules=new wxMailto_Module*[wxMailto_Module::MODULECOUNT]) ||
	    !(m_registered_modules=new wxMailto_Module*[wxMailto_Module::MODULECOUNT]) ||
	    !(m_registered_modules_semaphores=new wxSemaphore*[wxMailto_Module::MODULECOUNT]))
	{
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	wxInt i;
	//Clear arrays
	for (i=0; i<wxMailto_Module::MODULECOUNT; i++)
	{
		m_allocated_modules[i] = NULL;
		m_registered_modules[i] = NULL;
		m_registered_modules_semaphores[i] = NULL;
	}

	//Allocate modules
	if (
			!(m_allocated_modules[wxMailto_Module::AUTHENTICATE]=new AuthenticateGlue()) ||
	    !(m_allocated_modules[wxMailto_Module::GCRYPT]=new GcryptManager()) ||
	    !(m_allocated_modules[wxMailto_Module::GPG]=new GPGManager()) ||
	    !(m_allocated_modules[wxMailto_Module::PASSWORD]=new PasswordManager()) ||
	    !(m_allocated_modules[wxMailto_Module::IDNA]=new IdnaGlue()) ||
#if 0
	    !(m_allocated_modules[wxMailto_Module::MIME]=new MimeGlue()) ||
#endif
	    !(m_allocated_modules[wxMailto_Module::POCO]=new PocoGlue()) ||
	    !(m_allocated_modules[wxMailto_Module::TLS]=new TLSGlue()) ||
	    !(m_allocated_modules[wxMailto_Module::MESSAGESTORE]=new MessageStore())
#if 0
	    !(m_allocated_modules[wxMailto_Module::SESSIONMANAGER]=new SessionManager()) ||
	    !(m_allocated_modules[wxMailto_Module::ACCOUNTMANAGER]=new AccountManager())
#endif
		 )
	{
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	//Allocate one semaphore per module
	for (i=0; i<wxMailto_Module::MODULECOUNT; i++)
	{
		if (!(m_registered_modules_semaphores[i]=new wxSemaphore()))
			return LOGERROR(ID_OUT_OF_MEMORY);
	}

	//Initialize modules, multi-threaded
	for (i=0; i<wxMailto_Module::MODULECOUNT; i++)
	{
		if (ID_OK!=(status=InitializeModule(m_allocated_modules[i])))
			return status;
	}

	return ID_OK;
}

wxmailto_status AppModuleManager::PrepareShutdown()
{
	if (!m_allocated_modules)
		return LOGERROR(ID_NULL_POINTER);

	ModulePrepareShutdowner** modules = new ModulePrepareShutdowner*[wxMailto_Module::MODULECOUNT];
	if (!modules)
		return LOGERROR(ID_OUT_OF_MEMORY);

	wxmailto_status status;
	wxInt i;
	for (i=0; i<wxMailto_Module::MODULECOUNT; i++)
	{
		modules[i] = new ModulePrepareShutdowner(m_allocated_modules[i]);
		if (!modules[i])
			return LOGERROR(ID_OUT_OF_MEMORY);

		if (ID_OK!=(status=PrepareShutdownModule(modules[i])))
			return status;
	}
	
	for (i=0; i<wxMailto_Module::MODULECOUNT; i++)
	{
		modules[i]->Wait();
		delete modules[i];
	}
	delete[] modules;

	return ID_OK;
}

AuthenticateGlue* AppModuleManager::GetAuthenticateGlue()
{
	return static_cast<AuthenticateGlue*>(GetGenericModule(wxMailto_Module::AUTHENTICATE));
}

GcryptManager* AppModuleManager::GetGcryptManager()
{
	return static_cast<GcryptManager*>(GetGenericModule(wxMailto_Module::GCRYPT));
}

GPGManager* AppModuleManager::GetGPGManager()
{
	return static_cast<GPGManager*>(GetGenericModule(wxMailto_Module::GPG));
}

PasswordManager* AppModuleManager::GetPasswordManager()
{
	return static_cast<PasswordManager*>(GetGenericModule(wxMailto_Module::PASSWORD));
}

IdnaGlue* AppModuleManager::GetIdnaGLue()
{
	return static_cast<IdnaGlue*>(GetGenericModule(wxMailto_Module::IDNA));
}

#if 0
MimeGlue* AppModuleManager::GetMimeGlue()
{
	return static_cast<MimeGlue*>(GetGenericModule(wxMailto_Module::MIME));
}
#endif
PocoGlue* AppModuleManager::GetPocoGlue()
{
	return static_cast<PocoGlue*>(GetGenericModule(wxMailto_Module::POCO));
}

TLSGlue* AppModuleManager::GetTLSGlue()
{
	return static_cast<TLSGlue*>(GetGenericModule(wxMailto_Module::TLS));
}
#if 0
SessionManager* AppModuleManager::GetSessionManager()
{
	return static_cast<SessionManager*>(GetGenericModule(wxMailto_Module::SESSIONMANAGER));
}

AccountManager* AppModuleManager::GetAccountManager()
{
	return static_cast<AccountManager*>(GetGenericModule(wxMailto_Module::ACCOUNTMANAGER));
}
#endif

MessageStore* AppModuleManager::GetMessageStore()
{
	return static_cast<MessageStore*>(GetGenericModule(wxMailto_Module::MESSAGESTORE));
}

wxMailto_Module* AppModuleManager::GetGenericModule(wxMailto_Module::ModuleType type)
{
	//If module already registered, return it
	wxMailto_Module* module = GetModule(type);
	if (module)
		return module;

	//If exiting, and module is unregistered, assert and return NULL
	if (wxGetApp().IsExitRequested())
	{
		wxASSERT_MSG(false, _("Trying to get module after unregister at exit"));
		return NULL;
	}

	//Wait for module to register
	m_registered_modules_semaphores[(int)type]->Wait();
	m_registered_modules_semaphores[(int)type]->Post();
	return GetModule(type);
}

wxmailto_status AppModuleManager::InitializeModule(wxMailto_Module* module)
{
	if (!module)
		return LOGERROR(ID_NULL_POINTER);

	ModuleInitializer* module_initializer = new ModuleInitializer(module);
	if (!module_initializer)
		return LOGERROR(ID_OUT_OF_MEMORY);

	if (wxTHREAD_NO_ERROR != module_initializer->Create())
	{
		wxASSERT_MSG(false, module->GetName() + _(" module initialize failed"));
		module_initializer->Delete();
		return LOGERROR(ID_GENERIC_ERROR);
	}

	module_initializer->Run(); //Thread will delete itself
	return ID_OK;
}

wxmailto_status AppModuleManager::PrepareShutdownModule(ModulePrepareShutdowner* shutdowner)
{
	if (!shutdowner)
		return LOGERROR(ID_NULL_POINTER);

	if (wxTHREAD_NO_ERROR != shutdowner->Create())
	{
		wxASSERT_MSG(false, shutdowner->GetModuleName() + _(": module prepare shutdown failed"));
		return LOGERROR(ID_GENERIC_ERROR);
	}

	shutdowner->Run(); //Thread is joinable, and will NOT delete itself
	return ID_OK;
}

void AppModuleManager::RegisterModule(wxMailto_Module* module)
{
	int type = (int)module->GetType();
  {
    wxMutexLocker locker(m_registered_modules_lock);
    m_registered_modules[type]=module;
    m_registered_modules_semaphores[type]->Post();
  }
}

void AppModuleManager::UnregisterModule(wxMailto_Module* module)
{
	int type = (int)module->GetType();
  {
    wxMutexLocker locker(m_registered_modules_lock);
    m_registered_modules[type]=NULL;
  }
}

wxMailto_Module* AppModuleManager::GetModule(wxMailto_Module::ModuleType type)
{
  {
    wxMutexLocker locker(m_registered_modules_lock);
    return m_registered_modules[(int)type];
  }
}

void AppModuleManager::OnInitializeModuleFailed(const wxMailto_Module* module)
{
  int type = (int)module->GetType();
  wxASSERT_MSG(false, module->GetName() + _(" module initialize entry failed"));
  {
    wxMutexLocker locker(m_registered_modules_lock);
    m_registered_modules[type]=NULL;
    m_registered_modules_semaphores[type]->Post();
  }
	wxGetApp().RequestExit();
}

void AppModuleManager::OnPrepareShutdownModuleFailed(const wxMailto_Module* module)
{
  int type = (int)module->GetType();
  wxASSERT_MSG(false, module->GetName() + _(" module shutdown entry failed"));
  {
    wxMutexLocker locker(m_registered_modules_lock);
    m_registered_modules[type]=NULL;
    m_registered_modules_semaphores[type]->Post();
  }
  wxGetApp().RequestExit();
}
