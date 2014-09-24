
// Copyright (C) 2010-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "session.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "session.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"

using namespace wxMailto;


SessionManager::SessionManager()
 : wxMailto_Module()
{
}

SessionManager::~SessionManager()
{
}

wxmailto_status SessionManager::Initialize()
{
	wxGetApp().GetAppModuleManager()->RegisterModule(this);

	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (!gpg_manager)
		return LOGERROR(ID_NULL_POINTER);
	
	gpg_manager->AddModuleDependency();
	return ID_OK;
}

wxmailto_status SessionManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();

	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (gpg_manager)
		gpg_manager->RemoveModuleDependency();

	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}
