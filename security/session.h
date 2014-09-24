#ifndef _SESSION_H_
#define _SESSION_H_

// Copyright (C) 2010-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "session.h"
#endif

#include <wx/string.h>

#include "../gui/wxmailto_module.h"


namespace wxMailto
{

class SessionManager : public wxMailto_Module
{
public:
	SessionManager();
	virtual ~SessionManager();

	wxString GetName() const {return "SessionManager";}
	ModuleType GetType() const {return wxMailto_Module::SESSIONMANAGER;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
//	wxmailto_status 
};

}

#endif // _SESSION_H_
