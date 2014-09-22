#ifndef _MIME_GLUE_H_
#define _MIME_GLUE_H_

// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "mimeglue.h"
#endif

#include <wx/string.h>

#include "../gui/wxmailto_module.h"

#include <mimetic/mimetic.h>
#include <mimetic/rfc822/rfc822.h>


namespace wxMailto
{

#define MimeEntity mimetic::MimeEntity
#define MimeEntityList mimetic::MimeEntityList
#define MimeHeaders mimetic::Header
#define MimeHeader mimetic::Field


class MimeGlue : public wxMailto_Module
{
public:
	MimeGlue() : wxMailto_Module() {}
 	virtual ~MimeGlue() {}

	wxString GetName() const {return "Mime";}
	ModuleType GetType() const {return wxMailto_Module::MIME;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

};

}

#endif // _MIME_GLUE_H_
