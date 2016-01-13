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
#include "../storage/config_helper.h"
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

	{
		SafeString connection_string;
		if (ID_OK!=(GetConnectionString(connection_string)))
			return LOGERROR(ID_INVALID_DATASOURCE);

		const char* connection_string_ptr;
		if (ID_OK!=(status=connection_string.GetStr(connection_string_ptr)))
			return status;

		Poco::Data::ODBC::Connector::registerConnector();
		m_pool = new Poco::Data::SessionPool("ODBC", connection_string_ptr);
	}

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
	//*session << "set autocommit = 0", Poco::Data::Keywords::now;
	//*session << "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE", Poco::Data::Keywords::now;
	//*session << "START TRANSACTION WITH CONSISTENT SNAPSHOT", Poco::Data::Keywords::now;
#else
		session->begin();
#endif
#else
	*session << "set autocommit = 0", Poco::Data::Keywords::now;
	//*session << "SET TRANSACTION ISOLATION LEVEL SERIALIZABLE", Poco::Data::Keywords::now;
	//*session << "START TRANSACTION", Poco::Data::Keywords::now;
	session->begin();
#endif
	return ID_OK;
}

wxmailto_status PocoGlue::CommitTransaction(Poco::Data::Session* session)
{
	if (!session)
		return LOGERROR(ID_NULL_POINTER);

	//*session << "COMMIT AND CHAIN", Poco::Data::Keywords::now;
	session->commit();
	session->begin();
	return ID_OK;
}

wxmailto_status PocoGlue::RollbackTransaction(Poco::Data::Session* session)
{
	if (!session)
		return LOGERROR(ID_NULL_POINTER);

	//*session << "ROLLBACK AND CHAIN", Poco::Data::Keywords::now;
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

wxmailto_status PocoGlue::GetConnectionString(SafeString& plaintext_connectionstring)
{
	wxmailto_status status;

	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return LOGERROR(ID_NULL_POINTER);

	wxString encrypted_connectionstring_hex;
	config->Read(CONFIG_CONNECTIONSTRING, &encrypted_connectionstring_hex, wxEmptyString);
	if (encrypted_connectionstring_hex.IsEmpty())
	{
		ThreadedModalDialogLauncher datasource_dialog;
		if (ID_OK!=(status=datasource_dialog.Show(wxGetApp().GetWindowID(IDManager::ID_DATASOURCE_DIALOG))))
		{
			return status;
		}
	}

	SafeString default_value;
	SafeString salt;
	if (ID_OK!=(status=default_value.SetStr("", NOOP)) ||
	    ID_OK!=(status=salt.StrDup(DSN_SALT)))
	{
		return status;
	}

	return ConfigHelper::ReadEncrypted(CONFIG_CONNECTIONSTRING, plaintext_connectionstring, default_value, salt);
}

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
