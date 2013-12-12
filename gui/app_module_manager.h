#ifndef _APP_MODULE_MANAGER_H_
#define _APP_MODULE_MANAGER_H_

// Copyright (C) 2010-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "app_module_manager.h"
#endif

#include <wx/thread.h>
#include "wxmailto_app.h"
#include "wxmailto_module.h"
#if 0
#include "../glue/authenticateglue.h"
#endif
#include "../security/gpg_manager.h"
#include "../security/password_manager.h"
#if 0
#include "../glue/idnaglue.h"
#include "../glue/mimeglue.h"
#endif
#include "../glue/pocoglue.h"
#if 0
#include "../glue/tlsglue.h"
#include "../security/session.h"
#include "../storage/account.h"
#include "../storage/messagestore.h"
#endif

namespace wxMailto
{

class ModuleInitializer : public wxThread
{
public:
	ModuleInitializer(wxMailto_Module* module);
	virtual wxThread::ExitCode Entry();

private:
	wxMailto_Module* m_module;
};

class ModulePrepareShutdowner : public wxThread
{
public:
	ModulePrepareShutdowner(wxMailto_Module* module);
	virtual wxThread::ExitCode Entry();
	
	wxString GetModuleName() const {return m_module ? m_module->GetName() : "(NULL)";}

private:
	wxMailto_Module* m_module;
};

class AppModuleManager
{
public:
	AppModuleManager();
	~AppModuleManager();

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
#if 0
	AuthenticateGlue* GetAuthenticateGlue();
#endif
	GPGManager* GetGPGManager();
	PasswordManager* GetPasswordManager();
#if 0
	IdnaGlue* GetIdnaGLue();
	MimeGlue* GetMimeGlue();
#endif
	PocoGlue* GetPocoGlue();
#if 0
	TLSGlue* GetTLSGlue();
	SessionManager* GetSessionManager();
	AccountManager* GetAccountManager();
	MessageStore* GetMessageStore();
#endif
private:
	wxMailto_Module* GetGenericModule(wxMailto_Module::ModuleType type);

private:
	wxmailto_status InitializeModule(wxMailto_Module* module);
	wxmailto_status PrepareShutdownModule(ModulePrepareShutdowner* shutdowner);

private:
#if 0
friend class AuthenticateGlue;
#endif
friend class GPGManager;
friend class PasswordManager;
#if 0
friend class IdnaGlue;
friend class MimeGlue;
#endif
friend class PocoGlue;
#if 0
friend class TLSGlue;
friend class MessageStore;
friend class SessionManager;
friend class AccountManager;
#endif
	void RegisterModule(wxMailto_Module* module);
	void UnregisterModule(wxMailto_Module* module);
  wxMailto_Module* GetModule(wxMailto_Module::ModuleType type);

friend class ModuleInitializer;
	void OnInitializeModuleFailed(const wxMailto_Module* module);
friend class ModulePrepareShutdowner;
	void OnPrepareShutdownModuleFailed(const wxMailto_Module* module);

private:
	wxMailto_Module** m_allocated_modules;

	wxMailto_Module** m_registered_modules;
  wxMutex m_registered_modules_lock;
	wxSemaphore** m_registered_modules_semaphores;
};

}

#endif // _APP_MODULE_MANAGER_H_
