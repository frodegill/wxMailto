#ifndef _DATABASE_UPDATE_H_
#define _DATABASE_UPDATE_H_

// Copyright (C) 2008-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "database_update.h"
#endif

#include <wx/string.h>

#include "../defines.h"
#include "../glue/pocoglue.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

#define ACCOUNT_ACCOUNTNAME_LEN 128
#define ACCOUNT_SERVERNAME_LEN 1024
#define ACCOUNT_AUTHENTICATION_LEN 64

#define GPG_KEY_KEY_LEN 4096

#define HEADER_NAME_LEN 997 //RFC2822 §2.2.3

#define HEADERVALUE_TEXT_VALUE_LEN 2048
#define HEADERVALUE_EMAIL_VALUE_LEN 2048

#define POP_UIDL_UIDL_LEN 70 //RFC1939 §7

#define PROPERTY_PROPERTY_KEY_LEN 50
#define PROPERTY_PROPERTY_VALUE_LEN 1024

#define WALLET_CONTENT_LEN 1024



struct DatabaseUpdateItem {
	wxUint32 m_db_version;
	wxString m_update_query;
};


class DatabaseUpdate
{
public:
	DatabaseUpdate();
	virtual ~DatabaseUpdate();

public:
	wxmailto_status GetDatabaseVersion(Poco::Data::Session* session_in_transaction, wxUint32& db_version) const;
	wxmailto_status UpdateIfNeeded(Poco::Data::Session* session_in_transaction);
};

}

#endif // _DATABASE_UPDATE_H_
