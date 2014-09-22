
// Copyright (C) 2008-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "account.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "account.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../protocols/protocol_iface.h"
#include "../protocols/pop.h"
#include "../protocols/smtp.h"
#include "../protocols/imap.h"
#include "../storage/database_update.h"
#include "../wxmailto_rc.h"

#include <Poco/Data/MySQL/MySQLException.h>

using namespace wxMailto;

# include <wx/listimpl.cpp>
WX_DEFINE_LIST(AccountList);
WX_DEFINE_LIST(AccountSyncInfoList);


Account::	Account(const PocoAccount& account)
: DatabaseStatus(),
  m_account_id(account.m_account_id),
  m_account_name(account.m_account_name_string),
  m_protocol(static_cast<Protocol>(account.m_protocol)),
  m_servername(account.m_servername_string),
  m_port(account.m_port),
  m_poll_interval(account.m_poll_interval),
  m_secure_connection(static_cast<SecureConnection>(account.m_secure_connection)),
  m_authentication_method(account.m_authentication_method_string),
  m_wallet_id(account.m_wallet_id),
  m_outgoing_account_id(account.m_outgoing_account_id),
  m_sync_thread(NULL)
{
}

Account::~Account()
{
	{
		wxCriticalSectionLocker locker(m_sync_thread_lock);
		if (m_sync_thread)
		{
			m_sync_thread->Delete();
			m_sync_thread = NULL;
		}
	}
}

wxmailto_status Account::Sync()
{
	if (m_sync_thread)
		return LOGERROR(ID_ALREADY_SYNCING);
	
	switch(GetProtocol())
	{
	case PROTOCOL_POP: m_sync_thread = new Pop(this); break;
	case PROTOCOL_SMTP: m_sync_thread = new Smtp(this); break;
	case PROTOCOL_IMAP: m_sync_thread = new Imap(this); break;
	default: m_sync_thread = NULL; break;
	}
	
	if (!m_sync_thread)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	if (wxTHREAD_NO_ERROR != m_sync_thread->Create())
	{
		wxASSERT_MSG(false, _("Creating sync thread failed"));
		m_sync_thread->Delete();
		OnSyncComplete(ID_GENERIC_ERROR, AccountManager::NO_AUTOMATIC_POLL != GetPollInterval());
		return LOGERROR(ID_GENERIC_ERROR);
	}

	return (wxTHREAD_NO_ERROR==m_sync_thread->Run()) ? ID_OK : ID_GENERIC_ERROR;
}

wxmailto_status Account::OnSyncComplete(wxmailto_status WXUNUSED(result_status), wxBool restart_timer)
{
	{
		wxCriticalSectionLocker locker(m_sync_thread_lock);
		m_sync_thread = NULL;
	}

	if (!restart_timer ||  AccountManager::NO_AUTOMATIC_POLL==GetPollInterval())
		return ID_OK;

	wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD, wxGetApp().GetWindowID(IDManager::IDTE_RESCHEDULE_ACCOUNT_SYNC));
	if (!evt)
		return LOGERROR(ID_OUT_OF_MEMORY);

	evt->SetInt(GetAccountId());
	wxQueueEvent(wxGetApp().GetMainFrame(), evt);
	return ID_OK;
}

wxmailto_status Account::GetProtocolString(wxString& protocol) const
{
	switch(GetProtocol())
	{
		case PROTOCOL_POP: protocol="pop"; return ID_OK;
		case PROTOCOL_SMTP: protocol="smtp"; return ID_OK;
		case PROTOCOL_IMAP: protocol="imap"; return ID_OK;
		default: return LOGERROR(ID_SHOULDNT_GET_HERE);
	}
}

wxString Account::GetFQDN() const
{
	if (!m_fqdn.IsEmpty())
		return m_fqdn;

	wxString fqdn;
	//ToDo: Find fqdn
	return fqdn;
}


AccountManager::AccountManager()
: m_accounts(NULL),
  m_account_sync_list(NULL)
{
	m_sync_timer.SetOwner(wxGetApp().GetMainFrame(), wxGetApp().GetWindowID(IDManager::IDT_SYNC_ACCOUNT));
}

AccountManager::~AccountManager()
{
  delete m_accounts;
	delete m_account_sync_list;
}

wxmailto_status AccountManager::Initialize()
{
	m_account_sync_list = new AccountSyncInfoList();
	if (!m_account_sync_list)
		return LOGERROR(ID_NULL_POINTER);

	m_accounts = new AccountList();
	if (!m_accounts)
		return LOGERROR(ID_NULL_POINTER);

	wxmailto_status status;
	if (ID_OK!=(status=LoadFromDB()))
		return status;

	wxGetApp().GetAppModuleManager()->RegisterModule(this);
	return ID_OK;
}

wxmailto_status AccountManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}

wxmailto_status AccountManager::SaveToDB()
{
	Account* account;
	AccountList::iterator iter;
	for (iter=m_accounts->begin(); iter!=m_accounts->end(); ++iter)
	{
		account = *iter;
		if (account)
		{
			switch(account->GetStatus())
			{
			case DatabaseStatus::NEW:
				//ToDo
				account->SetStatus(DatabaseStatus::UNCHANGED);
				break;
			case DatabaseStatus::MODIFIED:
				//ToDo
				account->SetStatus(DatabaseStatus::UNCHANGED);
				break;
			case DatabaseStatus::DELETED:
				//ToDo
				account->SetStatus(DatabaseStatus::UNCHANGED);
				break;
			default: break;
			}
		}
	}
	return ID_OK;
}

wxmailto_status AccountManager::LoadFromDB()
{
	delete m_accounts;
	m_accounts = new AccountList();
	if (!m_accounts)
		return LOGERROR(ID_NULL_POINTER);

	wxmailto_status status;
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!poco_glue)
		return LOGERROR(ID_EXIT_REQUESTED);
	
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	std::vector<PocoAccount> accounts;

 	wxString s;
	try {
	*session << "SELECT account_id, account_name, authentication,"\
	            " outgoing_account_id, poll_interval, port, protocol,"\
	            " secure_connection, server_name, wallet_id "\
	            "FROM accountitem ",
		Poco::Data::into(accounts),
		Poco::Data::now;
	}
	catch(Poco::Data::MySQL::ConnectionException& ce) {
		s = wxString(ce.displayText());
	}
	catch(Poco::Data::MySQL::StatementException& se) {
		s = wxString(se.displayText());
	}
	catch (Poco::Exception e) {
		s = wxString(e.displayText());
		status = ID_GENERIC_ERROR;
	}
	
	poco_glue->ReleaseSession(session);
	
	std::vector<PocoAccount>::const_iterator iter;
	for (iter=accounts.begin(); iter!=accounts.end(); ++iter)
	{
		Account* account = new Account(*iter);
			
		if (NULL == account)
			return LOGERROR(ID_OUT_OF_MEMORY);

		account->SetStatus(DatabaseStatus::UNCHANGED);
		AddAccount(account);

		if (NO_AUTOMATIC_POLL != account->GetPollInterval())
		{
			SyncAccount(account->GetAccountId(), SYNC_IMMEDIATELY);
		}
	}

	return status;
}

Account* AccountManager::GetAccountPtr(const wxUint16 account_id) const
{
	Account* account;
	AccountList::iterator iter;
	for (iter=m_accounts->begin(); iter!=m_accounts->end(); ++iter)
	{
		account = *iter;
		if (account && account_id==account->GetAccountId())
			return account;
	}
	return NULL;
}

wxmailto_status AccountManager::OnSyncAccount()
{
	wxmailto_status status = ID_OK;
	wxmailto_status tmp_status;
	time_t now = wxDateTime::Now().GetTicks();
	AccountSyncInfo* next_to_sync;
	while(true)
	{
		next_to_sync = GetNextAccountToSync();
		if (!next_to_sync)
			break;
		
		if (next_to_sync->m_sync_time > now)
		{
			m_sync_timer.Start((next_to_sync->m_sync_time - now)*1000, wxTIMER_ONE_SHOT);
			break;
		}
		
		Account* account = GetAccountPtr(next_to_sync->m_account_id);
		if (account)
		{
			if (ID_OK!=(tmp_status=account->Sync()) && ID_ALREADY_SYNCING!=tmp_status)
				status = tmp_status;
		}
		m_account_sync_list->remove(next_to_sync);
	}
	return status;
}

wxmailto_status AccountManager::SyncAccount(const wxUint16 account_id, wxInt delay_in_seconds)
{
	wxmailto_status status = ID_OK;
	if (SYNC_IMMEDIATELY == delay_in_seconds) //Sync immediately
	{
		//Remove if already scheduled
		AccountSyncInfo* tmp_account = GetAccountSyncInfo(account_id);
		if (tmp_account)
			m_account_sync_list->remove(tmp_account);

		//Sync account immediately
		Account* account = GetAccountPtr(account_id);
		if (account)
		{
			status = account->Sync();
		}
	}
	else //Schedule sync
	{
		time_t now = wxDateTime::Now().GetTicks();
		time_t requested_sync = now+delay_in_seconds;

		AccountSyncInfo* next_to_sync = GetNextAccountToSync();

		//Find item if already scheduled
		wxBool reschedule_this_sync = false;
		AccountSyncInfo* this_account_sync_info = GetAccountSyncInfo(account_id);
		if (!this_account_sync_info)
		{
			AccountSyncInfo* sync_info = new AccountSyncInfo();
			if (!sync_info)
				return LOGERROR(ID_OUT_OF_MEMORY);

			sync_info->m_account_id = account_id;
			sync_info->m_sync_time = requested_sync;
			m_account_sync_list->Append(sync_info);
			if (!next_to_sync)
			{
				next_to_sync = sync_info;
				reschedule_this_sync = true;
			}
		}

		if (reschedule_this_sync || next_to_sync->m_sync_time > requested_sync)
		{
			next_to_sync->m_sync_time = requested_sync;
			m_sync_timer.Start((next_to_sync->m_sync_time - now)*1000, wxTIMER_ONE_SHOT);
		}
	}
	return status;
}

AccountSyncInfo* AccountManager::GetNextAccountToSync() const
{
		time_t next_sync = -1;
		AccountSyncInfo* found_sync_info = NULL;
		AccountSyncInfo* tmp_sync_info = NULL;
		AccountSyncInfoList::iterator iter;
		for (iter=m_account_sync_list->begin(); iter!=m_account_sync_list->end(); ++iter)
		{
			tmp_sync_info = *iter;
			if (!tmp_sync_info) continue;
			
			if (next_sync==-1 || next_sync > tmp_sync_info->m_sync_time)
			{
				next_sync = tmp_sync_info->m_sync_time;
				found_sync_info = tmp_sync_info;
			}
		}
		return found_sync_info;
}

AccountSyncInfo* AccountManager::GetAccountSyncInfo(const wxUint16 account_id) const
{
	AccountSyncInfo* tmp_account = NULL;
	AccountSyncInfoList::iterator iter;
	for (iter=m_account_sync_list->begin(); iter!=m_account_sync_list->end(); ++iter)
	{
		tmp_account = *iter;
		if (tmp_account && tmp_account->m_account_id==account_id)
			return tmp_account;
	}
	return NULL;
}

wxmailto_status AccountManager::AddAccount(Account* account)
{
	m_accounts->Append(account);
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status AccountManager::DeleteAccount(Account* WXUNUSED(account))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

void AccountManager::NotifyModifyAccount(Account* WXUNUSED(account))
{
	/*
	ThreadedModelessDialogLauncherData* data = new ThreadedModelessDialogLauncherData();
	wxThreadEvent* evt = new wxThreadEvent(wxEVT_COMMAND_THREAD, id);
	if (!data || !evt)
	{
		delete data;
		delete evt;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	data->m_message = message;
	data->m_caption = caption;
	evt->SetPayload<ThreadedModelessDialogLauncherData*>(data);

	wxGetApp().AddExitBlocker();
	wxQueueEvent(wxGetApp().GetMainFrame(), evt);
	*/
}
