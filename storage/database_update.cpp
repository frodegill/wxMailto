
// Copyright (C) 2008-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "database_update.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "database_update.h"
#include "persistent_object.h"
#include "../wxmailto_errors.h"
#include "../glue/pocoglue.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"

using namespace wxMailto;


static DatabaseUpdateItem update_items[] = {
//"CREATE TABLE property" is done in Database::GetDatabaseVersion
{ 1, "CREATE TABLE IF NOT EXISTS credentials "\
     "(id INT UNSIGNED NOT NULL,"\
     " location BLOB,"\
     " username BLOB,"\
     " password BLOB,"\
     " PRIMARY KEY credentials_pk1 (id))"\
     " engine = InnoDB;"},
{1,  "INSERT INTO property "\
     "(property_key, property_value)"\
     " VALUES "\
     "('next_credential_id', 1);"},
#if 0
{ 1, "CREATE TABLE IF NOT EXISTS walletitem "\
     "(item_id SMALLINT UNSIGNED NOT NULL,"\
     " content TEXT CHARACTER SET utf8,"\
     " PRIMARY KEY walletitem_pk1 (item_id))"\
     " engine = InnoDB;"},
{ 2, "INSERT INTO walletitem "\
     "(item_id, content)"\
     " VALUES"\
     "(1, 'secret');"},
{ 2, "INSERT INTO walletitem "\
     "(item_id, content)"\
     " VALUES"\
     "(2, 'secret');"},
{ 2, "CREATE TABLE IF NOT EXISTS accountitem "\
     "(account_id SMALLINT UNSIGNED NOT NULL,"\
     " account_name TEXT CHARACTER SET utf8 NOT NULL,"\
     " protocol SMALLINT UNSIGNED,"\
     " server_name TEXT CHARACTER SET utf8 NOT NULL,"\
     " port SMALLINT UNSIGNED,"\
     " poll_interval INT UNSIGNED,"\
     " secure_connection INTEGER,"\
     " authentication TEXT CHARACTER SET utf8,"\
     " wallet_id SMALLINT UNSIGNED,"\
     " outgoing_account_id SMALLINT UNSIGNED,"\
     " PRIMARY KEY accountitem_pk1 (account_id),"\
     " FOREIGN KEY accountitem_fk1 (wallet_id) REFERENCES walletitem(item_id) ON DELETE CASCADE)"\
     " engine = InnoDB;"},
{ 3, "INSERT INTO accountitem "\
     "(account_id, account_name, protocol, server_name, port, poll_interval,"\
     " secure_connection, authentication, wallet_id, outgoing_account_id)"\
     " VALUES"\
     "(1, 'test-out', 2, 'localhost', 25, 0,"\
     " 1, 'CRAM-MD5', 1, 0);"},
{ 3, "INSERT INTO accountitem "\
     "(account_id, account_name, protocol, server_name, port, poll_interval,"\
     " secure_connection, authentication, wallet_id, outgoing_account_id)"\
     " VALUES"\
     "(2, 'test-pop', 1, 'localhost', 110, 300,"\
     " 1, 'AUTO', 2, 1);"},
{ 3, "CREATE TABLE IF NOT EXISTS pop_uidl "\
     "(account_id SMALLINT UNSIGNED NOT NULL,"\
     " uidl TEXT CHARACTER SET utf8 NOT NULL,"\
     " PRIMARY KEY pop_uidl_pk1 (account_id,uidl(200)),"\
     " FOREIGN KEY pop_uidl_fk1 (account_id) REFERENCES accountitem(account_id) ON DELETE CASCADE"\
     ")"\
     " engine = InnoDB;"},
{ 4, "CREATE TABLE IF NOT EXISTS gpg_key "\
     "(key_id INT UNSIGNED NOT NULL,"\
     " key_value TEXT CHARACTER SET utf8,"\
     "PRIMARY KEY gpg_key_pk1 (key_id))"\
     " engine = InnoDB;"},
{ 5, "CREATE TABLE IF NOT EXISTS raw_message "\
     "(message_id INT UNSIGNED NOT NULL,"\
     " body TEXT CHARACTER SET utf8,"\
     " PRIMARY KEY message_pk1 (message_id)"\
     ")"\
     " engine = InnoDB;"},
{6,  "INSERT INTO property "\
     "(property_key, property_value)"\
     " VALUES"\
     "('enable_persistent_mime_parse', 0);"}, //Default is security before speed
{6,  "CREATE TABLE IF NOT EXISTS message "\
     "(message_id INT UNSIGNED NOT NULL,"\
     " PRIMARY KEY message_pk1 (message_id)"\
     ")"\
     " engine = InnoDB;"},
{7,  "INSERT INTO property "\
     "(property_key, property_value)"\
     " VALUES"\
     "('next_message_id', 1);"},
{7,  "INSERT INTO property "\
     "(property_key, property_value)"\
     " VALUES"\
     "('next_multipart_id', 1);"},
#endif
{0, wxEmptyString}
};

/*****************************************************************/

DatabaseUpdate::DatabaseUpdate()
{
}

DatabaseUpdate::~DatabaseUpdate()
{
}

wxmailto_status DatabaseUpdate::GetDatabaseVersion(Poco::Data::Session* session_in_transaction, wxUint32& db_version) const
{
	wxmailto_status status = ID_OK;
	db_version = 0;

	*session_in_transaction << "CREATE TABLE IF NOT EXISTS property "\
	                           "(property_key CHAR(50) CHARACTER SET utf8 NOT NULL PRIMARY KEY," /*PROPERTY_PROPERTY_KEY_LEN*/ \
	                           " property_value TEXT CHARACTER SET utf8)" /*PROPERTY_PROPERTY_VALUE_LEN*/ \
	                           "engine = InnoDB", Poco::Data::now;

	wxUint16 count;
	*session_in_transaction << "SELECT COUNT(*) FROM property WHERE property_key='db_version'",
			Poco::Data::into(count,(const wxUint16)0), Poco::Data::now;

	if (0==count)
	{
		*session_in_transaction << "INSERT INTO property (property_key, property_value) VALUE ('db_version',0);", Poco::Data::now;

		if (ID_OK!=(status=PocoGlue::CommitTransaction(session_in_transaction))) //Only CREATE TABLE, INSERT will get here
			return status;
	}

	*session_in_transaction << "SELECT property_value FROM property WHERE property_key='db_version'",
			Poco::Data::into(db_version,(const wxUint32)0), Poco::Data::now;

	return status;
}

wxmailto_status DatabaseUpdate::UpdateIfNeeded(Poco::Data::Session* session_in_transaction)
{
	wxUint32 db_version;
	wxmailto_status status = GetDatabaseVersion(session_in_transaction, db_version);

	wxUint32 max_version = db_version;
	wxSizeT index = 0;
	while (ID_OK==status && 0!=update_items[index].m_db_version)
	{
		if (db_version<update_items[index].m_db_version)
		{
			if (max_version<update_items[index].m_db_version)
				max_version = update_items[index].m_db_version;

			try
			{
				*session_in_transaction << update_items[index].m_update_query, Poco::Data::now;

				if (update_items[index].m_db_version != update_items[index+1].m_db_version)
				{
					*session_in_transaction << "UPDATE property "\
					                           "SET property_value = ? "\
					                           "WHERE property_key='db_version'",
						Poco::Data::use(max_version), Poco::Data::now;

					status = PocoGlue::CommitTransaction(session_in_transaction);
				}
			}
#if 1
			catch (Poco::Exception e)
			{
				return LOGERROR_MSG(ID_GENERIC_ERROR, wxString(e.displayText()));
			}
#endif
			catch (...)
			{
				return LOGERROR(ID_GENERIC_ERROR);
			}
		}
		index++;
	}

	return status;
}
