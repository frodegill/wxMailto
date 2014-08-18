
// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "password_manager.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include <Poco/Data/BLOB.h>
#include "Poco/Data/RecordSet.h"

#include "../defines.h"
#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"
#include "../storage/persistent_object.h"
#include "password_manager.h"


using namespace wxMailto;

PasswordManager::PasswordManager()
: m_obfuscated_master_password(NULL),
  m_master_password_obfuscator(NULL),
  m_obfuscated_master_password_length(0)
{
}

PasswordManager::~PasswordManager()
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		ForgetMasterPassphrase();
	}
}

wxmailto_status PasswordManager::Initialize()
{
	wxGetApp().GetAppModuleManager()->RegisterModule(this);
	return ID_OK;
}

wxmailto_status PasswordManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}

wxmailto_status PasswordManager::GetMasterPassphrase(SafeString& passphrase)
{
	wxmailto_status status;

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (!m_obfuscated_master_password || 0==m_obfuscated_master_password_length)
		{
			SafeString old_passphrase;
			SafeString new_passphrase;
			if (ID_OK!=(status=old_passphrase.SetStr("", NOOP)) ||
			    ID_OK!=(status=new_passphrase.StrDup("ToDo: get passphrase")))
			{
				return status;
			}

			SetMasterPassphrase(old_passphrase, new_passphrase);
		}

		wxUint8* utf8_buffer = new wxUint8[m_obfuscated_master_password_length];
		size_t i;
		for (i=0; m_obfuscated_master_password_length>i; i++)
		{
			utf8_buffer[i] = m_obfuscated_master_password[i]^m_master_password_obfuscator[i];
		}
		passphrase.Set(utf8_buffer, m_obfuscated_master_password_length, DELETE);
	}

	return ID_OK;
}

wxmailto_status PasswordManager::SetMasterPassphrase(const SafeString& old_passphrase, const SafeString& new_passphrase)
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		ForgetMasterPassphrase();

		wxmailto_status status;
		const wxUint8* utf8_buffer;
		if (ID_OK!=(status=new_passphrase.Get(utf8_buffer, m_obfuscated_master_password_length)))
			return status;

		if (0<m_obfuscated_master_password_length)
		{
			m_obfuscated_master_password = new wxUint8[m_obfuscated_master_password_length];
			m_master_password_obfuscator = new wxUint8[m_obfuscated_master_password_length];
			size_t i;
			for (i=0; m_obfuscated_master_password_length>i; i++)
			{
				m_master_password_obfuscator[i] = RANDOM(256);
				m_obfuscated_master_password[i] = utf8_buffer[i]^m_master_password_obfuscator[i];
			}
		}
		return UpdateCredentialPassphrase(old_passphrase, new_passphrase);
	}
}

void PasswordManager::ForgetMasterPassphrase()
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (m_obfuscated_master_password)
		{
			memset(m_obfuscated_master_password, 0, m_obfuscated_master_password_length);
			memset(m_master_password_obfuscator, 0, m_obfuscated_master_password_length);
			delete[] m_obfuscated_master_password;
			delete[] m_master_password_obfuscator;
			m_obfuscated_master_password = m_master_password_obfuscator = NULL;
			m_obfuscated_master_password_length = 0;
		}
	}
}

wxmailto_status PasswordManager::GetSudoPassword(SafeString& password)
{
	wxmailto_status status;

	SafeString salt;
	if (ID_OK!=(status=salt.StrDup(SUDO_SALT)))
		return status;
	
	return GenericDecrypt(m_encrypted_sudo_password_hex, salt, password);
}

wxmailto_status PasswordManager::SetSudoPassword(const SafeString& password)
{
	wxmailto_status status;

	SafeString salt;
	if (ID_OK!=(status=salt.StrDup(SUDO_SALT)))
		return status;

	return GenericEncrypt(password, salt, m_encrypted_sudo_password_hex);
}

void PasswordManager::ForgetSudoPassword()
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		m_encrypted_sudo_password_hex.Clear();
	}
}

wxmailto_status PasswordManager::GetLocation(wxUInt id, SafeString& location)
{
	wxmailto_status status;

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

		SafeString master_passphrase;
		SafeString location_salt;
		if (ID_OK!=(status=location_salt.StrDupIndexed(LOCATION_SALT, id)))
			return status;

		SafeString derived_location_master_key;
		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateDerivedKey(master_passphrase, location_salt, derived_location_master_key)))
		{
			return status;
		}

		wxString encrypted_location_hex;
		if (ID_OK != (status=LoadLocation(id, encrypted_location_hex)) ||
		    ID_OK != (status=gcrypt_manager->DecryptWithDerivedKey(encrypted_location_hex, derived_location_master_key, location)))
		{
			return status;
		}
	}

	return ID_OK;
}

wxmailto_status PasswordManager::GetCredential(wxUInt id, SafeString& location, SafeString& username, SafeString& password)
{
	wxmailto_status status;

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		SafeString master_passphrase;
		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)))
			return status;

		return GetCredential(master_passphrase, id, location, username, password);
	}
}

wxmailto_status PasswordManager::GetCredential(const SafeString& master_passphrase, wxUInt id, SafeString& location, SafeString& username, SafeString& password)
{
	wxmailto_status status;

	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

	SafeString location_salt;
	SafeString username_salt;
	SafeString password_salt;
	if (ID_OK!=(status=location_salt.StrDupIndexed(LOCATION_SALT, id)) ||
	    ID_OK!=(status=username_salt.StrDupIndexed(USERNAME_SALT, id)) ||
	    ID_OK!=(status=password_salt.StrDupIndexed(PASSWORD_SALT, id)))
	{
		return status;
	}

	SafeString derived_location_master_key;
	SafeString derived_username_master_key;
	SafeString derived_password_master_key;
	if (ID_OK != (status=CreateDerivedKey(master_passphrase, location_salt, derived_location_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, username_salt, derived_username_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, password_salt, derived_password_master_key)))
	{
		return status;
	}

	wxString encrypted_location_hex;
	wxString encrypted_username_hex;
	wxString encrypted_password_hex;
	if (ID_OK != (status=LoadCredential(id, encrypted_location_hex, encrypted_username_hex, encrypted_password_hex)) ||
	    ID_OK != (status=gcrypt_manager->DecryptWithDerivedKey(encrypted_location_hex, derived_location_master_key, location)) ||
	    ID_OK != (status=gcrypt_manager->DecryptWithDerivedKey(encrypted_username_hex, derived_username_master_key, username)) ||
	    ID_OK != (status=gcrypt_manager->DecryptWithDerivedKey(encrypted_password_hex, derived_password_master_key, password)))
	{
		return status;
	}
	return ID_OK;
}

wxmailto_status PasswordManager::SetCredential(wxUInt& id, const SafeString& location, const SafeString& username, const SafeString& password)
{
	wxmailto_status status;
	
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		SafeString master_passphrase;
		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)))
			return status;

		return SetCredential(master_passphrase, id, location, username, password);
	}
}

wxmailto_status PasswordManager::SetCredential(const SafeString& master_passphrase, wxUInt& id, const SafeString& location, const SafeString& username, const SafeString& password)
{
	wxmailto_status status;

	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

	SafeString location_salt;
	SafeString username_salt;
	SafeString password_salt;
	if (ID_OK!=(status=location_salt.StrDupIndexed(LOCATION_SALT, id)) ||
	    ID_OK!=(status=username_salt.StrDupIndexed(USERNAME_SALT, id)) ||
	    ID_OK!=(status=password_salt.StrDupIndexed(PASSWORD_SALT, id)))
	{
		return status;
	}

	SafeString derived_location_master_key;
	SafeString derived_username_master_key;
	SafeString derived_password_master_key;
	if (ID_OK != (status=CreateDerivedKey(master_passphrase, location_salt, derived_location_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, username_salt, derived_username_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, password_salt, derived_password_master_key)))
	{
		return status;
	}

	wxString encrypted_location_hex;
	wxString encrypted_username_hex;
	wxString encrypted_password_hex;
	if (ID_OK != (status=gcrypt_manager->EncryptWithDerivedKey(location, derived_location_master_key, encrypted_location_hex)) ||
	    ID_OK != (status=gcrypt_manager->EncryptWithDerivedKey(username, derived_username_master_key, encrypted_username_hex)) ||
	    ID_OK != (status=gcrypt_manager->EncryptWithDerivedKey(password, derived_password_master_key, encrypted_password_hex)))
	{
		return status;
	}
	return SaveCredential(id, encrypted_location_hex, encrypted_username_hex, encrypted_password_hex);
}

wxmailto_status PasswordManager::ForgetCredential(wxUInt id)
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		return DeleteCredential(id);
	}
}

wxmailto_status PasswordManager::LoadLocation(wxUInt id, wxString& encrypted_location_hex)
{
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!poco_glue)
		return LOGERROR(ID_EXIT_REQUESTED);

	Poco::Data::Session* session;
	wxmailto_status status;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	Poco::Data::BLOB encrypted_location_blob;
	*session << "SELECT location FROM credentials WHERE id=?",
			Poco::Data::into(encrypted_location_blob),
			Poco::Data::use(id),
			Poco::Data::now;

	poco_glue->ReleaseSession(session);
	
	encrypted_location_hex = wxString::FromUTF8(encrypted_location_blob.rawContent(), encrypted_location_blob.size());

	return ID_OK;
}

wxmailto_status PasswordManager::LoadCredential(wxUInt id, wxString& encrypted_location_hex, wxString& encrypted_username_hex, wxString& encrypted_password_hex)
{
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!poco_glue)
		return LOGERROR(ID_EXIT_REQUESTED);

	Poco::Data::Session* session;
	wxmailto_status status;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	Poco::Data::BLOB encrypted_location_blob;
	Poco::Data::BLOB encrypted_username_blob;
	Poco::Data::BLOB encrypted_password_blob;
	*session << "SELECT location,username,password FROM credentials WHERE id=?",
			Poco::Data::into(encrypted_location_blob),
			Poco::Data::into(encrypted_username_blob),
			Poco::Data::into(encrypted_password_blob),
			Poco::Data::use(id),
			Poco::Data::now;

	poco_glue->ReleaseSession(session);
	
	encrypted_location_hex = wxString::FromUTF8(encrypted_location_blob.rawContent(), encrypted_location_blob.size());
	encrypted_username_hex = wxString::FromUTF8(encrypted_username_blob.rawContent(), encrypted_username_blob.size());
	encrypted_password_hex = wxString::FromUTF8(encrypted_password_blob.rawContent(), encrypted_password_blob.size());

	return ID_OK;
}

wxmailto_status PasswordManager::SaveCredential(wxUInt& id, const wxString& encrypted_location_hex, const wxString& encrypted_username_hex, const wxString& encrypted_password_hex)
{
	wxmailto_status status;
	wxBool update = true;
	if (0 == id)
	{
		PersistentProperty* p = new PersistentProperty();
		if (!p)
			return LOGERROR(ID_OUT_OF_MEMORY);

		if (ID_OK != (status=p->Initialize(NEXT_CREDENTIAL_ID, &wxGetApp().GetGlobalLockers()->m_next_credential_id_lock)) ||
		    ID_OK != (status=p->GetNextAvailableId(id)))
		{
			delete p;
			return status;
		}

		delete p;
		update = false;
	}

	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!poco_glue)
		return LOGERROR(ID_EXIT_REQUESTED);

	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)) ||
	    ID_OK!=(status=poco_glue->StartTransaction(session)))
	{
		return status;
	}

	const wxScopedCharBuffer encrypted_location_buffer = encrypted_location_hex.ToUTF8();
	const wxScopedCharBuffer encrypted_username_buffer = encrypted_username_hex.ToUTF8();
	const wxScopedCharBuffer encrypted_password_buffer = encrypted_password_hex.ToUTF8();
	
	Poco::Data::BLOB encrypted_location_blob(encrypted_location_buffer.data(), encrypted_location_buffer.length());
	Poco::Data::BLOB encrypted_username_blob(encrypted_username_buffer.data(), encrypted_username_buffer.length());
	Poco::Data::BLOB encrypted_password_blob(encrypted_password_buffer.data(), encrypted_password_buffer.length());
	if (update)
	{
		*session << "UPDATE credentials SET location=?,username=?,password=? WHERE id=?",
				Poco::Data::use(encrypted_location_blob),
				Poco::Data::use(encrypted_username_blob),
				Poco::Data::use(encrypted_password_blob),
				Poco::Data::use(id),
				Poco::Data::now;
	}
	else
	{
		*session << "INSERT INTO credentials (id,location,username,password) VALUES (?,?,?,?)",
				Poco::Data::use(id),
				Poco::Data::use(encrypted_location_blob),
				Poco::Data::use(encrypted_username_blob),
				Poco::Data::use(encrypted_password_blob),
				Poco::Data::now;
	}

	poco_glue->CommitTransaction(session);
	poco_glue->ReleaseSession(session);

	return ID_OK;
}

wxmailto_status PasswordManager::UpdateCredentialPassphrase(const SafeString& old_passphrase, const SafeString& new_passphrase)
{
	if (old_passphrase.IsEmpty())
		return ID_OK;

	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!gcrypt_manager || !poco_glue)
		return LOGERROR(ID_EXIT_REQUESTED);

	Poco::Data::Session* session;
	wxmailto_status status;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	wxUInt count, max;
	*session << "SELECT COUNT(*),MAX(id) FROM credentials",
			Poco::Data::into(count),
			Poco::Data::into(max),
			Poco::Data::now;

	//Reserve new IDs
	PersistentProperty* property = new PersistentProperty();
	if (!property)
	{
		poco_glue->ReleaseSession(session);
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	wxUint32 first_available_id;
	if (ID_OK != (status=property->Initialize(NEXT_CREDENTIAL_ID, &wxGetApp().GetGlobalLockers()->m_next_credential_id_lock)) ||
	    ID_OK != (status=property->GetNextAvailableId(first_available_id, count)))
	{
		delete property;
		poco_glue->ReleaseSession(session);
		return status;
	}
	delete property;

	//Start transaction
	poco_glue->StartTransaction(session);

	//Select all existing IDs
	Poco::Data::Statement* select_ids = new Poco::Data::Statement(*session);
	if (!select_ids)
	{
		poco_glue->RollbackTransaction(session);
		poco_glue->ReleaseSession(session);
		return LOGERROR(ID_OUT_OF_MEMORY);
	}
	*select_ids << "SELECT id FROM credentials WHERE id<=?",
			Poco::Data::use(max);
	select_ids->execute();
	Poco::Data::RecordSet* select_ids_rs = new Poco::Data::RecordSet(*select_ids);
	if (!select_ids_rs)
	{
		delete select_ids;
		poco_glue->RollbackTransaction(session);
		poco_glue->ReleaseSession(session);
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	//iterate
	wxUInt id, new_id;
	wxBool more = select_ids_rs->moveFirst();
	SafeString location;
	SafeString username;
	SafeString password;
	while (more)
	{
		id = select_ids_rs->value(0).convert<wxUInt>();
		new_id = first_available_id++;
		if (ID_OK != (status=GetCredential(old_passphrase, id, location, username, password)) ||
		    ID_OK != (status=SetCredential(new_passphrase, new_id, location, username, password)))
		{
			delete select_ids_rs;
			delete select_ids;
			poco_glue->RollbackTransaction(session);
			poco_glue->ReleaseSession(session);
			return status;
		}

		more = select_ids_rs->moveNext();
	}
	delete select_ids_rs;
	delete select_ids;
	
	*session << "DELETE FROM credentials WHERE id<=?",
			Poco::Data::use(max),
			Poco::Data::now;

	poco_glue->CommitTransaction(session);
	poco_glue->ReleaseSession(session);
	
	return ID_OK;
}

wxmailto_status PasswordManager::DeleteCredential(wxUInt id)
{
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!poco_glue)
		return LOGERROR(ID_EXIT_REQUESTED);

	Poco::Data::Session* session;
	wxmailto_status status;
	if (ID_OK!=(status=poco_glue->CreateSession(session)) ||
	    ID_OK!=(status=poco_glue->StartTransaction(session)))
	{
		return status;
	}

	*session << "DELETE FROM credentials WHERE id=?",
			Poco::Data::use(id),
			Poco::Data::now;

	poco_glue->CommitTransaction(session);
	poco_glue->ReleaseSession(session);

	return ID_OK;
}

wxmailto_status PasswordManager::CreateHash(const SafeString& secret, const SafeString& salt, wxString& hashed_value_hex)
{
	wxmailto_status status;
	
	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

	SafeString plaintext;
	if (ID_OK!=(status=plaintext.Set(secret)) ||
	    ID_OK!=(status=plaintext.Append(salt)))
	{
		return status;
	}
	return gcrypt_manager->Hash(plaintext, hashed_value_hex);
}

wxmailto_status PasswordManager::CreateDerivedKey(const SafeString& plaintext, const SafeString& salt, SafeString& derived_key)
{
	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();
	return gcrypt_manager->DeriveKey(plaintext, salt, derived_key);
}

wxmailto_status PasswordManager::GenericEncrypt(const SafeString& plaintext, const SafeString& salt, wxString& encrypted_hex)
{
	wxmailto_status status;

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

		SafeString derived_master_key;
		SafeString master_passphrase;
		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateDerivedKey(master_passphrase, salt, derived_master_key)))
		{
			return status;
		}
		return gcrypt_manager->EncryptWithDerivedKey(plaintext, derived_master_key, encrypted_hex);
	}
}

wxmailto_status PasswordManager::GenericDecrypt(const wxString& encrypted_hex, const SafeString& salt, SafeString& plaintext)
{
	wxmailto_status status;

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

		SafeString derived_master_key;
		SafeString master_passphrase;
		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateDerivedKey(master_passphrase, salt, derived_master_key)))
		{
			return status;
		}
		return gcrypt_manager->DecryptWithDerivedKey(encrypted_hex, derived_master_key, plaintext);
	}
}
