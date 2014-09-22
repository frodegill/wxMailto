#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

// Copyright (C) 2008-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "account.h"
#endif

#include <wx/list.h>
#include <wx/string.h>
#include <wx/timer.h>
#include "../defines.h"
#include "../glue/pocoglue.h"
#include "../gui/wxmailto_module.h"
#include "../storage/database_status.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

class PocoAccount;
class	ProtocolInterface;
class Account : public DatabaseStatus
{
friend class AccountManager;
friend class Testing;
public:
	enum Protocol {
		PROTOCOL_POP=1,
		PROTOCOL_SMTP=2,
		PROTOCOL_IMAP=3
	};
	enum SecureConnection {
		SECURE_NONE=0,
		SECURE_SSL=1,
		SECURE_TLS=2
	};
	static const wxUint16 NO_ACCOUNT = 0;

private:
	Account(const PocoAccount& account);
public:
	virtual ~Account();

	wxmailto_status Sync();
	wxmailto_status OnSyncComplete(wxmailto_status result_status, wxBool restart_timer);

public:
	wxUint16 GetAccountId() const {return m_account_id;}
	const wxString& GetAccountName() const {return m_account_name;}
	Protocol GetProtocol() const {return m_protocol;}
	wxmailto_status GetProtocolString(wxString& protocol) const;
	const wxString& GetServername() const {return m_servername;}
	wxUint16 GetPort() const {return m_port;}
	wxString GetFQDN() const;
	wxUint32 GetPollInterval() const {return m_poll_interval;} //NO_AUTOMATIC_POLL for no automatic poll interval 
	SecureConnection GetSecureConnection() const {return m_secure_connection;}
	wxString GetAuthenticationMethod() const {return m_authentication_method;}
	wxUint16 GetWalletId() const {return m_wallet_id;}
	wxUint16 GetOutgoingAccountId() const {return m_outgoing_account_id;}

	wxString GetAutodetectedAuthenticationMethod() const {return m_autodetected_authentication_method;}
	void SetAutodetectedAuthenticationMethod(const wxString& authentication_method) {m_autodetected_authentication_method=authentication_method;}

private:
	wxUint16 m_account_id;
	wxString m_account_name;
	Protocol m_protocol;
	wxString m_servername;
	wxUint16 m_port;
	wxString m_fqdn;
	wxUint32 m_poll_interval;
	SecureConnection m_secure_connection;
	wxString m_authentication_method; //"NONE" for no authentication, "AUTO" for autodetection (from most to least secure)
	wxString m_autodetected_authentication_method; //Not stored in database
	wxUint16 m_wallet_id;
	wxUint16 m_outgoing_account_id;

	wxCriticalSection m_sync_thread_lock;
	ProtocolInterface* m_sync_thread;
};
WX_DECLARE_LIST(Account, AccountList);


struct AccountSyncInfo
{
	wxUint16 m_account_id;
	time_t m_sync_time;
};
WX_DECLARE_LIST(AccountSyncInfo, AccountSyncInfoList);


class AccountManager : public wxMailto_Module, public wxTimer
{
public:
	static const wxInt SYNC_IMMEDIATELY = 0;
	static const wxUint32 NO_AUTOMATIC_POLL = 0;

public:
	AccountManager();
	virtual ~AccountManager();

	wxString GetName() const {return "AccountManager";}
	ModuleType GetType() const {return wxMailto_Module::ACCOUNTMANAGER;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status SaveToDB();
	wxmailto_status LoadFromDB();

public:
	const Account* GetAccount(const wxUint16 account_id) const {return GetAccountPtr(account_id);}
private:
	Account* GetAccountPtr(const wxUint16 account_id) const;
	wxmailto_status AddAccount(Account* account);
	wxmailto_status DeleteAccount(Account* account);
	void NotifyModifyAccount(Account* account);

public:
	wxmailto_status OnSyncAccount(); //Timer fired in wxMailto_Frame
	wxmailto_status SyncAccount(const wxUint16 account_id, wxInt delay_in_seconds);
private:
	AccountSyncInfo* GetNextAccountToSync() const;
	AccountSyncInfo* GetAccountSyncInfo(const wxUint16 account_id) const;
	
private:
	AccountList* m_accounts;
	
	AccountSyncInfoList* m_account_sync_list;
	wxTimer m_sync_timer;
};

class PocoAccount
{
public:
	bool operator <(const PocoAccount& account) const {return m_account_id < account.m_account_id;} //For set and multiset support
	Poco::UInt16 operator()() const {return m_account_id;} //Operator to return the key for the map and multimap

public:
	wxUint16 m_account_id;
	std::string m_account_name_string;
	std::string m_authentication_method_string;
	wxUint16 m_outgoing_account_id;
	wxUint32 m_poll_interval;
	wxUint16 m_port;
	wxUint16 m_protocol;
	wxUint16 m_secure_connection;
	std::string m_servername_string;
	wxUint16 m_wallet_id;
};

} // namespace wxMailto


namespace Poco {
namespace Data {
//Taken from POCO documentation
template <>
class TypeHandler<class wxMailto::PocoAccount>
{
public:
	static std::size_t size() {return 10;}

	static void bind(std::size_t pos, const wxMailto::PocoAccount& obj, AbstractBinder* pBinder)
	{
		poco_assert_dbg(pBinder);
		TypeHandler<Poco::UInt16>::bind(pos++, obj.m_account_id, pBinder);
		TypeHandler<std::string>::bind(pos++, obj.m_account_name_string, pBinder);
		TypeHandler<std::string>::bind(pos++, obj.m_authentication_method_string, pBinder);
		TypeHandler<Poco::UInt16>::bind(pos++, obj.m_outgoing_account_id, pBinder);
		TypeHandler<Poco::UInt32>::bind(pos++, obj.m_poll_interval, pBinder);
		TypeHandler<Poco::UInt16>::bind(pos++, obj.m_port, pBinder);
		TypeHandler<Poco::UInt16>::bind(pos++, obj.m_protocol, pBinder);
		TypeHandler<Poco::UInt16>::bind(pos++, obj.m_secure_connection, pBinder);
		TypeHandler<std::string>::bind(pos++, obj.m_servername_string, pBinder);
		TypeHandler<Poco::UInt16>::bind(pos++, obj.m_wallet_id, pBinder);
	}

	static void prepare(std::size_t pos, const wxMailto::PocoAccount& obj, AbstractPreparation* pPrepare)
	{
		poco_assert_dbg(pBinder);
		TypeHandler<Poco::UInt16>::prepare(pos++, obj.m_account_id, pPrepare);
		TypeHandler<std::string>::prepare(pos++, obj.m_account_name_string, pPrepare);
		TypeHandler<std::string>::prepare(pos++, obj.m_authentication_method_string, pPrepare);
		TypeHandler<Poco::UInt16>::prepare(pos++, obj.m_outgoing_account_id, pPrepare);
		TypeHandler<Poco::UInt32>::prepare(pos++, obj.m_poll_interval, pPrepare);
		TypeHandler<Poco::UInt16>::prepare(pos++, obj.m_port, pPrepare);
		TypeHandler<Poco::UInt16>::prepare(pos++, obj.m_protocol, pPrepare);
		TypeHandler<Poco::UInt16>::prepare(pos++, obj.m_secure_connection, pPrepare);
		TypeHandler<std::string>::prepare(pos++, obj.m_servername_string, pPrepare);
		TypeHandler<Poco::UInt16>::prepare(pos++, obj.m_wallet_id, pPrepare);
	}

	static void extract(std::size_t pos, wxMailto::PocoAccount& obj, const wxMailto::PocoAccount& defVal, AbstractExtractor* pExt)
	{
		poco_assert_dbg(pExt);
		TypeHandler<Poco::UInt16>::extract(pos++, obj.m_account_id, defVal.m_account_id, pExt);
		TypeHandler<std::string>::extract(pos++, obj.m_account_name_string, defVal.m_account_name_string, pExt);
		TypeHandler<std::string>::extract(pos++, obj.m_authentication_method_string, defVal.m_authentication_method_string, pExt);
		TypeHandler<Poco::UInt16>::extract(pos++, obj.m_outgoing_account_id, defVal.m_outgoing_account_id, pExt);
		TypeHandler<Poco::UInt32>::extract(pos++, obj.m_poll_interval, defVal.m_poll_interval, pExt);
		TypeHandler<Poco::UInt16>::extract(pos++, obj.m_port, defVal.m_port, pExt);
		TypeHandler<Poco::UInt16>::extract(pos++, obj.m_protocol, defVal.m_protocol, pExt);
		TypeHandler<Poco::UInt16>::extract(pos++, obj.m_secure_connection, defVal.m_secure_connection, pExt);
		TypeHandler<std::string>::extract(pos++, obj.m_servername_string, defVal.m_servername_string, pExt);
		TypeHandler<Poco::UInt16>::extract(pos++, obj.m_wallet_id, defVal.m_wallet_id, pExt);
	}
};

} } // namespace Poco::Data

#endif // _ACCOUNT_H_
