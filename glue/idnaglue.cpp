
// Copyright (C) 2009-2011  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "idnaglue.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "idnaglue.h"

#include <idna.h>
#include <stringprep.h>

#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"


using namespace wxMailto;

wxmailto_status IdnaGlue::Initialize()
{
	wxGetApp().GetAppModuleManager()->RegisterModule(this);
	return ID_OK;
}

wxmailto_status IdnaGlue::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}

wxmailto_status IdnaGlue::ToASCII(const wxString& unicode_domain, wxString& idna_domain)
{
	char* normalized = stringprep_utf8_nfkc_normalize(unicode_domain.ToUTF8(), -1);
	if (!normalized)
		return LOGERROR(ID_GENERIC_ERROR);

	char* output;
	int to_ascii_ret = idna_to_ascii_8z(normalized, &output, IDNA_USE_STD3_ASCII_RULES);
	free(normalized);
	if (IDNA_SUCCESS!=to_ascii_ret)
		return LOGERROR(ID_GENERIC_ERROR);

	idna_domain = wxString::FromUTF8(output).Lower();
	free(output);
	return ID_OK;
}

wxmailto_status IdnaGlue::ToUnicode(const wxString& idna_domain, wxString& unicode_domain)
{
	char* output;
	if (IDNA_SUCCESS!=idna_to_unicode_8z8z(idna_domain.Lower().ToUTF8(), &output, IDNA_USE_STD3_ASCII_RULES))
		return LOGERROR(ID_GENERIC_ERROR);

	unicode_domain = wxString::FromUTF8(output);
	free(output);
	return ID_OK;
}
