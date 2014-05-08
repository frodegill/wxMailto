
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
#ifdef WIPE_AFTER_USE
	m_encrypted_sudo_password_hex.WipeAfterUse();
#endif
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

wxmailto_status PasswordManager::GetMasterPassphrase(wxString& passphrase)
{
#ifdef WIPE_AFTER_USE
	passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (!m_obfuscated_master_password || 0==m_obfuscated_master_password_length)
		{
			SetMasterPassphrase(wxEmptyString, "ToDo: get passphrase");
		}

		wxUint8* utf8_buffer = new wxUint8[m_obfuscated_master_password_length];
		size_t i;
		for (i=0; m_obfuscated_master_password_length>i; i++)
		{
			utf8_buffer[i] = m_obfuscated_master_password[i]^m_master_password_obfuscator[i];
		}
		passphrase = wxString::FromUTF8(reinterpret_cast<const char*>(utf8_buffer), m_obfuscated_master_password_length);

		memset(utf8_buffer, 0, m_obfuscated_master_password_length);
		delete[] utf8_buffer;
	}

	return ID_OK;
}

wxmailto_status PasswordManager::SetMasterPassphrase(const wxString& old_passphrase, const wxString& new_passphrase)
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		ForgetMasterPassphrase();

		const wxScopedCharBuffer utf8_buffer = new_passphrase.ToUTF8();
		m_obfuscated_master_password_length = utf8_buffer.length();
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

wxmailto_status PasswordManager::GetSudoPassword(wxString& password)
{
	return GenericDecrypt(m_encrypted_sudo_password_hex, password, "sudo@wxMailto");
}

wxmailto_status PasswordManager::SetSudoPassword(wxString& password)
{
	return GenericEncrypt(password, m_encrypted_sudo_password_hex, "sudo@wxMailto");
}

void PasswordManager::ForgetSudoPassword()
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		m_encrypted_sudo_password_hex.Clear();
	}
}

wxmailto_status PasswordManager::GetLocation(wxUInt id, wxString& location)
{
	wxmailto_status status;
	wxString master_passphrase;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

		wxUint8* derived_location_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
		if (!derived_location_master_key)
			return LOGERROR(ID_OUT_OF_MEMORY);

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@location@wxMailto",id), derived_location_master_key)))
		{
			delete[] derived_location_master_key;
			return status;
		}

		wxString encrypted_location_hex;
		if (ID_OK != (status=LoadLocation(id, encrypted_location_hex)) ||
		    ID_OK != (status=gcrypt_manager->DecryptWithDerivedKey(encrypted_location_hex, derived_location_master_key, location)))
		{
			delete[] derived_location_master_key;
			return status;
		}

		delete[] derived_location_master_key;
	}

	return ID_OK;
}

wxmailto_status PasswordManager::GetCredential(wxUInt id, wxString& location, wxString& username, wxString& password)
{
	wxmailto_status status;
	wxString master_passphrase;
#ifdef WIPE_AFTER_USE
	master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)))
			return status;

		return GetCredential(master_passphrase, id, location, username, password);
	}
}

wxmailto_status PasswordManager::GetCredential(const wxString& master_passphrase, wxUInt id, wxString& location, wxString& username, wxString& password)
{
	wxmailto_status status;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	username.WipeAfterUse();
	password.WipeAfterUse();
#endif

	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

	wxUint8* derived_location_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
	wxUint8* derived_username_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
	wxUint8* derived_password_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
	if (!derived_location_master_key || !derived_username_master_key || !derived_password_master_key)
	{
		delete[] derived_location_master_key;
		delete[] derived_username_master_key;
		delete[] derived_password_master_key;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	if (ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@location@wxMailto",id), derived_location_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@username@wxMailto",id), derived_username_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@password@wxMailto",id), derived_password_master_key)))
	{
		delete[] derived_location_master_key;
		delete[] derived_username_master_key;
		delete[] derived_password_master_key;
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
		delete[] derived_location_master_key;
		delete[] derived_username_master_key;
		delete[] derived_password_master_key;
		return status;
	}

	delete[] derived_location_master_key;
	delete[] derived_username_master_key;
	delete[] derived_password_master_key;
	return ID_OK;
}

wxmailto_status PasswordManager::SetCredential(wxUInt& id, const wxString& location, const wxString& username, const wxString& password)
{
	wxmailto_status status;
	wxString master_passphrase;
#ifdef WIPE_AFTER_USE
	master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)))
			return status;

		return SetCredential(master_passphrase, id, location, username, password);
	}
}

wxmailto_status PasswordManager::SetCredential(const wxString& master_passphrase, wxUInt& id, const wxString& location, const wxString& username, const wxString& password)
{
	wxmailto_status status;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	username.WipeAfterUse();
	password.WipeAfterUse();
#endif

	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

	wxUint8* derived_location_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
	wxUint8* derived_username_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
	wxUint8* derived_password_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
	if (!derived_location_master_key || !derived_username_master_key || !derived_password_master_key)
	{
		delete[] derived_location_master_key;
		delete[] derived_username_master_key;
		delete[] derived_password_master_key;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	if (ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@location@wxMailto",id), derived_location_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@username@wxMailto",id), derived_username_master_key)) ||
	    ID_OK != (status=CreateDerivedKey(master_passphrase, wxString::Format("%d@password@wxMailto",id), derived_password_master_key)))
	{
		delete[] derived_location_master_key;
		delete[] derived_username_master_key;
		delete[] derived_password_master_key;
		return status;
	}

	wxString encrypted_location_hex;
	wxString encrypted_username_hex;
	wxString encrypted_password_hex;
	if (ID_OK != (status=gcrypt_manager->EncryptWithDerivedKey(location, derived_location_master_key, encrypted_location_hex)) ||
	    ID_OK != (status=gcrypt_manager->EncryptWithDerivedKey(username, derived_username_master_key, encrypted_username_hex)) ||
	    ID_OK != (status=gcrypt_manager->EncryptWithDerivedKey(password, derived_password_master_key, encrypted_password_hex)))
	{
		delete[] derived_location_master_key;
		delete[] derived_username_master_key;
		delete[] derived_password_master_key;
		return status;
	}

	delete[] derived_location_master_key;
	delete[] derived_username_master_key;
	delete[] derived_password_master_key;
	return SaveCredential(id, encrypted_location_hex, encrypted_username_hex, encrypted_password_hex);
}

wxmailto_status PasswordManager::ForgetCredential(wxUInt id)
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		return DeleteCredential(id);
	}
}

wxmailto_status PasswordManager::LoadLocation(wxUInt id, wxString& encrypted_location)
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
	
	encrypted_location = wxString::FromUTF8(encrypted_location_blob.rawContent(), encrypted_location_blob.size());

	return ID_OK;
}

wxmailto_status PasswordManager::LoadCredential(wxUInt id, wxString& encrypted_location, wxString& encrypted_username, wxString& encrypted_password)
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
	
	encrypted_location = wxString::FromUTF8(encrypted_location_blob.rawContent(), encrypted_location_blob.size());
	encrypted_username = wxString::FromUTF8(encrypted_username_blob.rawContent(), encrypted_username_blob.size());
	encrypted_password = wxString::FromUTF8(encrypted_password_blob.rawContent(), encrypted_password_blob.size());

	return ID_OK;
}

wxmailto_status PasswordManager::SaveCredential(wxUInt& id, const wxString& encrypted_location, const wxString& encrypted_username, const wxString& encrypted_password)
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

	const wxScopedCharBuffer encrypted_location_buffer = encrypted_location.ToUTF8();
	const wxScopedCharBuffer encrypted_username_buffer = encrypted_username.ToUTF8();
	const wxScopedCharBuffer encrypted_password_buffer = encrypted_password.ToUTF8();
	
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

wxmailto_status PasswordManager::UpdateCredentialPassphrase(const wxString& old_passphrase, const wxString& new_passphrase)
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
	wxString location;
	wxString username;
	wxString password;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	username.WipeAfterUse();
	password.WipeAfterUse();
#endif
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

wxmailto_status PasswordManager::CreateHash(const wxString& secret, const wxString& salt, wxString& hashed_value_hex)
{
	wxString plaintext;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
#endif
	plaintext = secret+salt;
	
	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();
	return gcrypt_manager->Hash(plaintext, hashed_value_hex);
}

wxmailto_status PasswordManager::CreateDerivedKey(const wxString& plaintext, const wxString& salt, wxUint8* derived_key)
{
	GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();
	return gcrypt_manager->DeriveKey(plaintext, salt, derived_key);
}

wxmailto_status PasswordManager::GenericEncrypt(wxString& plaintext, wxString& encrypted_hex, const wxString& salt)
{
	wxmailto_status status;
	wxString master_passphrase;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
	master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

		wxUint8* derived_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
		if (!derived_master_key)
		{
			delete[] derived_master_key;
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateDerivedKey(master_passphrase, salt, derived_master_key)))
		{
			delete[] derived_master_key;
			return status;
		}

		status = gcrypt_manager->EncryptWithDerivedKey(plaintext, derived_master_key, encrypted_hex);
		delete[] derived_master_key;
		return status;
	}
}

wxmailto_status PasswordManager::GenericDecrypt(const wxString& encrypted_hex, wxString& plaintext, const wxString& salt)
{
	wxmailto_status status;
	wxString master_passphrase;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
	master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		GcryptManager* gcrypt_manager = wxGetApp().GetAppModuleManager()->GetGcryptManager();

		wxUint8* derived_master_key = new wxUint8[gcrypt_manager->GetDerivedKeyLength()];
		if (!derived_master_key)
		{
			delete[] derived_master_key;
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateDerivedKey(master_passphrase, salt, derived_master_key)))
		{
			delete[] derived_master_key;
			return status;
		}

		status = gcrypt_manager->DecryptWithDerivedKey(encrypted_hex, derived_master_key, plaintext);
		delete[] derived_master_key;
		return status;
	}
}
