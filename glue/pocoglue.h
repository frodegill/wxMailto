#ifndef _POCO_GLUE_H_
#define _POCO_GLUE_H_

// Copyright (C) 2012-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "pocoglue.h"
#endif

#include <Poco/Data/Common.h>
#include <Poco/Data/SessionPool.h>
#include <wx/string.h>

#include "../gui/wxmailto_module.h"


namespace wxMailto
{

class PocoGlue : public wxMailto_Module
{
public:
	PocoGlue();
 	virtual ~PocoGlue();

	wxString GetName() const {return "Poco";}
	ModuleType GetType() const {return wxMailto_Module::POCO;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status CreateSession(Poco::Data::Session*& session); //Call ReleaseSession when done
	wxmailto_status ReleaseSession(Poco::Data::Session* session);

public:
	static wxmailto_status StartTransaction(Poco::Data::Session* session);
	static wxmailto_status CommitTransaction(Poco::Data::Session* session);
	static wxmailto_status RollbackTransaction(Poco::Data::Session* session);
	static wxmailto_status LogError(const wxString& message);

private:
	wxmailto_status GetConnectionString(std::string& connection_string);
	wxmailto_status UpdateDatabaseIfNeeded();

private:
	Poco::Data::SessionPool* m_pool;
};

}

#endif // _POCO_GLUE_H_
