#ifndef _IDNA_GLUE_H_
#define _IDNA_GLUE_H_

// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "idnaglue.h"
#endif

#include <wx/string.h>

#include "../gui/wxmailto_module.h"


namespace wxMailto
{

class IdnaGlue : public wxMailto_Module
{
public:
	IdnaGlue() : wxMailto_Module() {}
 	virtual ~IdnaGlue() {}

	wxString GetName() const {return "Idna";}
	ModuleType GetType() const {return wxMailto_Module::IDNA;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

	static wxmailto_status ToASCII(const wxString& unicode_domain, wxString& idna_domain);
	static wxmailto_status ToUnicode(const wxString& idna_domain, wxString& unicode_domain);
};

}

#endif // _IDNA_GLUE_H_
