// Copyright (C) 2012-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "pocoglue.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/confbase.h>
#endif

#include "pocoglue.h"

#include <Poco/Data/ODBC/Connector.h>

#include "../gui/app_module_manager.h"
#include "../storage/database_update.h"
#include "../wxmailto_rc.h"


using namespace wxMailto;

PocoGlue::PocoGlue()
: wxMailto_Module(),
  m_pool(NULL)
{
}

PocoGlue::~PocoGlue()
{
	delete m_pool;
}

wxmailto_status PocoGlue::Initialize()
{
	wxmailto_status status;
	wxASSERT(!::wxIsMainThread());
	if (::wxIsMainThread())
	{
		wxASSERT_MSG(false, _("Don't call from main thread!"));
		return LOGERROR(ID_SHOULDNT_GET_HERE);
	}

	std::string connection_string;
	if (ID_OK!=(GetConnectionString(connection_string)))
		return LOGERROR(ID_INVALID_DATASOURCE);

	Poco::Data::ODBC::Connector::registerConnector();
	m_pool = new Poco::Data::SessionPool("ODBC", connection_string);

	//Update database, if needed
	if (ID_OK!=(status=UpdateDatabaseIfNeeded()))
		return status;

	//Register module
	if (ID_OK==status)
		wxGetApp().GetAppModuleManager()->RegisterModule(this);

	return status;
}

wxmailto_status PocoGlue::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	delete m_pool; m_pool=NULL;
	Poco::Data::ODBC::Connector::unregisterConnector();
	
	return ID_OK;
}

wxmailto_status PocoGlue::CreateSession(Poco::Data::Session*& session)
{
	try
	{
		session = new Poco::Data::Session(m_pool->get());
		if (!session)
			return LOGERROR(ID_NULL_POINTER);
		
		session->begin();
		return ID_OK;
	}
	catch (Poco::Exception e)
	{
		LogError(wxString(e.displayText()));
		return LOGERROR(ID_GENERIC_ERROR);
	}
	return LOGERROR(ID_SHOULDNT_GET_HERE);	
}

wxmailto_status PocoGlue::ReleaseSession(Poco::Data::Session* session)
{
	delete session;
	return ID_OK;
}

wxmailto_status PocoGlue::StartTransaction(Poco::Data::Session* session)
{
	if (!session)
		return LOGERROR(ID_NULL_POINTER);

#if 0
#if 1
	//*session << "set autocommit = 0", Poco::Data::now;
	//*session << "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE", Poco::Data::now;
	//*session << "START TRANSACTION WITH CONSISTENT SNAPSHOT", Poco::Data::now;
#else
		session->begin();
#endif
#else
	*session << "set autocommit = 0", Poco::Data::now;
	//*session << "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE", Poco::Data::now;
	//*session << "START TRANSACTION", Poco::Data::now;
	session->begin();
#endif
	return ID_OK;
}

wxmailto_status PocoGlue::CommitTransaction(Poco::Data::Session* session)
{
	if (!session)
		return LOGERROR(ID_NULL_POINTER);

	//*session << "COMMIT AND CHAIN", Poco::Data::now;
	session->commit();
	session->begin();
	return ID_OK;
}

wxmailto_status PocoGlue::RollbackTransaction(Poco::Data::Session* session)
{
	if (!session)
		return LOGERROR(ID_NULL_POINTER);

	//*session << "ROLLBACK AND CHAIN", Poco::Data::now;
	session->rollback();
	session->begin();
	return ID_OK;
}

wxmailto_status PocoGlue::LogError(const wxString& message)
{
	ThreadedModelessDialogLauncherData* data = new ThreadedModelessDialogLauncherData();
	wxThreadEvent* evt = new wxThreadEvent(wxEVT_COMMAND_THREAD, wxGetApp().GetWindowID(IDManager::ID_MODELESS_MESSAGE_DIALOG));
	if (!data || !evt)
	{
		delete data;
		delete evt;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	data->m_message = message;
	data->m_caption = _("Database error");
	evt->SetPayload<ThreadedModelessDialogLauncherData*>(data);

	wxGetApp().AddExitBlocker();
	wxQueueEvent(wxGetApp().GetMainFrame(), evt);
	
	return ID_OK;
}

wxmailto_status PocoGlue::GetConnectionString(std::string& connection_string)
{
	wxmailto_status status;

	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return LOGERROR(ID_NULL_POINTER);

#if 0
	wxString server, port, database, username, password;
	config->Read("Server", &server, wxEmptyString);
	if (server.IsEmpty())
	{
		if (ID_OK!=GetDatasourceInfo(server, port, database, username, password))
			return LOGERROR(ID_INVALID_DATASOURCE);
	}

	config->Read("Port",     &port,     wxEmptyString);
	config->Read("Database", &database, wxEmptyString);
	config->Read("Username", &username, wxEmptyString);
	config->Read("Password", &password, wxEmptyString);
#endif
	wxString encrypted_connectionstring;
	config->Read("connectionstring", &encrypted_connectionstring, wxEmptyString);
	if (encrypted_connectionstring.IsEmpty())
	{
#if 0
		wxString server, port, database, username, password;
		if (ID_OK!=GetDatasourceInfo(server, port, database, username, password))
			return LOGERROR(ID_INVALID_DATASOURCE);
#endif
		wxString plaintext = "DSN=wxMailto";
#ifdef WIPE_AFTER_USE
		plaintext.WipeAfterUse();
#endif
		if (ID_OK != (status=wxGetApp().GetAppModuleManager()->GetPasswordManager()->
			GenericEncrypt(plaintext, encrypted_connectionstring, "dsn@wxMailto")))
		{
			return status;
		}
		config->Write("connectionstring", encrypted_connectionstring);
	}
	else
	{
		wxString plaintext;
#ifdef WIPE_AFTER_USE
		plaintext.WipeAfterUse();
#endif

		if (ID_OK != (status=wxGetApp().GetAppModuleManager()->GetPasswordManager()->
			GenericDecrypt(encrypted_connectionstring, plaintext, "dsn@wxMailto")))
		{
			return status;
		}

		connection_string = plaintext;
	}

	return ID_OK;
}
#if 0
wxmailto_status PocoGlue::GetDatasourceInfo(wxString& server, wxString& port, wxString& database, wxString& username, wxString& password)
{
	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return LOGERROR(ID_NULL_POINTER);

	wxmailto_status status;
	ThreadedModalDialogLauncher datasource_dialog;
	if (ID_OK!=(status=datasource_dialog.Show(wxGetApp().GetWindowID(IDManager::ID_DATASOURCE_DIALOG))))
	{
		return status;
	}
	
	config->Read("Server",   &server,   wxEmptyString);
	config->Read("Server",   &port,     wxEmptyString);
	config->Read("Database", &database, wxEmptyString);
	config->Read("Username", &username, wxEmptyString);
	config->Read("Password", &password, wxEmptyString);

  return ID_OK;
}
#endif
wxmailto_status PocoGlue::UpdateDatabaseIfNeeded()
{
	wxmailto_status status;
	DatabaseUpdate updater;
	Poco::Data::Session* session;
	if (ID_OK!=(status=CreateSession(session)))
		return status;
	
	if (ID_OK!=(status=StartTransaction(session)))
	{
		ReleaseSession(session);
		return status;
	}
	
	if (ID_OK==(status=updater.UpdateIfNeeded(session)))
	{
		CommitTransaction(session);
	}
	else
	{
		RollbackTransaction(session);
	}
	ReleaseSession(session);
	return status;
}
