#ifndef _WXMAILTO_MODULE_H_
#define _WXMAILTO_MODULE_H_

// Copyright (C) 2010-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "wxmailto_module.h"
#endif

#include <wx/thread.h>

#include "../defines.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

class wxMailto_Module
{
public:
	enum ModuleType { //This enum should really not be here, but it's easier to please the compiler here..
#if 0
		AUTHENTICATE=0,
#endif
		GPG,
		PASSWORD,
		POCO,
#if 0
		IDNA,
		MIME,
		TLS,
		MESSAGESTORE,
		SESSIONMANAGER,
		ACCOUNTMANAGER,
#endif
		MODULECOUNT
	};

public:
	wxMailto_Module();
	virtual ~wxMailto_Module();

public:
	virtual wxString GetName() const = 0;
	virtual ModuleType GetType() const = 0;
	virtual wxmailto_status Initialize() = 0;
	virtual wxmailto_status PrepareShutdown() = 0;

public:
	wxmailto_status AddModuleDependency();
	wxmailto_status RemoveModuleDependency();
	void WaitForNoMoreDependencies();

private:
	wxMutex m_dependency_lock;
	wxCondition* m_dependency_condition;
	wxInt m_dependency_count;
};

}

#endif // _WXMAILTO_MODULE_H_
