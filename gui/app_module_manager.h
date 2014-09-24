#ifndef _APP_MODULE_MANAGER_H_
#define _APP_MODULE_MANAGER_H_

// Copyright (C) 2010-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "app_module_manager.h"
#endif

#include <wx/thread.h>
#include "wxmailto_app.h"
#include "wxmailto_module.h"
#include "../glue/authenticateglue.h"
#include "../security/gcrypt_manager.h"
#include "../security/gpg_manager.h"
#include "../security/password_manager.h"
#include "../glue/idnaglue.h"
#include "../glue/mimeglue.h"
#include "../glue/pocoglue.h"
#include "../glue/tlsglue.h"
#include "../security/session.h"
#include "../storage/account.h"
#include "../storage/messagestore.h"

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
	AuthenticateGlue* GetAuthenticateGlue();
	GcryptManager* GetGcryptManager();
	GPGManager* GetGPGManager();
	PasswordManager* GetPasswordManager();
	IdnaGlue* GetIdnaGLue();
	MimeGlue* GetMimeGlue();
	PocoGlue* GetPocoGlue();
	TLSGlue* GetTLSGlue();
	SessionManager* GetSessionManager();
	AccountManager* GetAccountManager();
	MessageStore* GetMessageStore();

private:
	wxMailto_Module* GetGenericModule(wxMailto_Module::ModuleType type);

private:
	wxmailto_status InitializeModule(wxMailto_Module* module);
	wxmailto_status PrepareShutdownModule(ModulePrepareShutdowner* shutdowner);

private:
friend class AuthenticateGlue;
friend class GcryptManager;
friend class GPGManager;
friend class PasswordManager;
friend class IdnaGlue;
friend class MimeGlue;
friend class PocoGlue;
friend class TLSGlue;
friend class MessageStore;
friend class SessionManager;
friend class AccountManager;
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
