
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
	m_encrypted_sudo_password.WipeAfterUse();
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
			passphrase.Empty();
			return ID_OK;
		}

		char* utf8_buffer = new char[m_obfuscated_master_password_length];
		size_t i;
		for (i=0; m_obfuscated_master_password_length>i; i++)
		{
			utf8_buffer[i] = m_obfuscated_master_password[i]^m_master_password_obfuscator[i];
		}
		passphrase = wxString::FromUTF8(utf8_buffer, m_obfuscated_master_password_length);

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
			m_obfuscated_master_password = new char[m_obfuscated_master_password_length];
			m_master_password_obfuscator = new char[m_obfuscated_master_password_length];
			size_t i;
			for (i=0; m_obfuscated_master_password_length<i; i++)
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
	return GenericDecrypt(m_encrypted_sudo_password, password, "sudo@wxMailto");
}

wxmailto_status PasswordManager::SetSudoPassword(wxString& password)
{
	return GenericEncrypt(password, m_encrypted_sudo_password, "sudo@wxMailto");
}

void PasswordManager::ForgetSudoPassword()
{
	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		m_encrypted_sudo_password.Clear();
	}
}

wxmailto_status PasswordManager::GetLocation(wxUInt id, wxString& location)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_location_master_passphrase;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_location_master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@location@wxMailto",id), hashed_location_master_passphrase)))
			return status;

		wxString encrypted_location;
		GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
		if (ID_OK != (status=LoadLocation(id, encrypted_location)) ||
		    ID_OK != (status=gpg_manager->DecryptWithPassword(encrypted_location, hashed_location_master_passphrase, location)))
			return status;
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
	wxString hashed_location_master_passphrase;
	wxString hashed_username_master_passphrase;
	wxString hashed_password_master_passphrase;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	username.WipeAfterUse();
	password.WipeAfterUse();
	hashed_location_master_passphrase.WipeAfterUse();
	hashed_username_master_passphrase.WipeAfterUse();
	hashed_password_master_passphrase.WipeAfterUse();
#endif
	if (ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@location@wxMailto",id), hashed_location_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@username@wxMailto",id), hashed_username_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@password@wxMailto",id), hashed_password_master_passphrase)))
		return status;

	wxString encrypted_location;
	wxString encrypted_username;
	wxString encrypted_password;
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (ID_OK != (status=LoadCredential(id, encrypted_location, encrypted_username, encrypted_password)) ||
	    ID_OK != (status=gpg_manager->DecryptWithPassword(encrypted_location, hashed_location_master_passphrase, location)) ||
	    ID_OK != (status=gpg_manager->DecryptWithPassword(encrypted_username, hashed_username_master_passphrase, username)) ||
	    ID_OK != (status=gpg_manager->DecryptWithPassword(encrypted_password, hashed_password_master_passphrase, password)))
		return status;

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
	wxString hashed_location_master_passphrase;
	wxString hashed_username_master_passphrase;
	wxString hashed_password_master_passphrase;
#ifdef WIPE_AFTER_USE
	location.WipeAfterUse();
	username.WipeAfterUse();
	password.WipeAfterUse();
	hashed_location_master_passphrase.WipeAfterUse();
	hashed_username_master_passphrase.WipeAfterUse();
	hashed_password_master_passphrase.WipeAfterUse();
#endif
	if (ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@location@wxMailto",id), hashed_location_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@username@wxMailto",id), hashed_username_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, wxString::Format("%d@password@wxMailto",id), hashed_password_master_passphrase)))
		return status;

	wxString encrypted_location;
	wxString encrypted_username;
	wxString encrypted_password;
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (ID_OK != (status=gpg_manager->EncryptWithPassword(location, hashed_location_master_passphrase, encrypted_location)) ||
	    ID_OK != (status=gpg_manager->EncryptWithPassword(username, hashed_username_master_passphrase, encrypted_username)) ||
	    ID_OK != (status=gpg_manager->EncryptWithPassword(password, hashed_password_master_passphrase, encrypted_password)))
		return status;

	return SaveCredential(id, encrypted_location, encrypted_username, encrypted_password);
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
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	if (!gpg_manager || !poco_glue)
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

wxmailto_status PasswordManager::CreateHash(const wxString& secret, const wxString& salt, wxString& hashed_value)
{
	wxString plaintext;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
#endif
	plaintext = secret+salt;
	
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	return gpg_manager->HashWithPassword(plaintext, hashed_value);
}

wxmailto_status PasswordManager::GenericEncrypt(wxString& plaintext, wxString& encrypted, const wxString& salt)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_master_passphrase;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateHash(master_passphrase, salt, hashed_master_passphrase)))
			return status;

		GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
		return gpg_manager->EncryptWithPassword(plaintext, hashed_master_passphrase, encrypted);
	}
}

wxmailto_status PasswordManager::GenericDecrypt(const wxString& encrypted, wxString& plaintext, const wxString& salt)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_master_passphrase;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_master_passphrase.WipeAfterUse();
#endif

	{
		wxCriticalSectionLocker locker(wxGetApp().GetGlobalLockers()->m_credential_lock);

		if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
		    ID_OK != (status=CreateHash(master_passphrase, salt, hashed_master_passphrase)))
			return status;

		GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
		return gpg_manager->DecryptWithPassword(encrypted, hashed_master_passphrase, plaintext);
	}
}
