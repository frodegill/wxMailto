
// Copyright (C) 2009-2011  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "mimeglue.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "mimeglue.h"

#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"

using namespace wxMailto;


wxmailto_status MimeGlue::Initialize()
{
	wxGetApp().GetAppModuleManager()->RegisterModule(this);
	return ID_OK;
}

wxmailto_status MimeGlue::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}
