
// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "password_manager.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"
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
	ForgetMasterPassphrase();
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

	return ID_OK;
}

wxmailto_status PasswordManager::SetMasterPassphrase(wxString& passphrase)
{
#ifdef WIPE_AFTER_USE
	passphrase.WipeAfterUse();
#endif

	ForgetMasterPassphrase();

	const wxScopedCharBuffer utf8_buffer = passphrase.ToUTF8();
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
	return ID_OK;
}

void PasswordManager::ForgetMasterPassphrase()
{
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

wxmailto_status PasswordManager::GetSudoPassword(wxString& password)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_master_passphrase;
#ifdef WIPE_AFTER_USE
	password.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_master_passphrase.WipeAfterUse();
#endif
	if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, "sudo@wxMailto", hashed_master_passphrase)))
		return status;

	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	return gpg_manager->Decrypt(m_encrypted_sudo_password, hashed_master_passphrase, password);
}

wxmailto_status PasswordManager::SetSudoPassword(wxString& password)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_master_passphrase;
#ifdef WIPE_AFTER_USE
	password.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_master_passphrase.WipeAfterUse();
#endif
	if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, "sudo@wxMailto", hashed_master_passphrase)))
		return status;

	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	return gpg_manager->Encrypt(password, hashed_master_passphrase, m_encrypted_sudo_password);
}

void PasswordManager::ForgetSudoPassword()
{
	m_encrypted_sudo_password.Clear();
}

wxmailto_status PasswordManager::GetCredential(const wxString& id, wxString& username, wxString& password)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_id_master_passphrase;
	wxString hashed_username_master_passphrase;
	wxString hashed_password_master_passphrase;
#ifdef WIPE_AFTER_USE
	username.WipeAfterUse();
	password.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_id_master_passphrase.WipeAfterUse();
	hashed_username_master_passphrase.WipeAfterUse();
	hashed_password_master_passphrase.WipeAfterUse();
#endif
	if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, "id@wxMailto", hashed_id_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, id+"@username@wxMailto", hashed_username_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, id+"@upassword@wxMailto", hashed_password_master_passphrase)))
		return status;

	wxString encrypted_username;
	wxString encrypted_password;
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (ID_OK != (status=LoadCredential(hashed_id_master_passphrase, encrypted_username, encrypted_password)) ||
	    ID_OK != (status=gpg_manager->Decrypt(encrypted_username, hashed_username_master_passphrase, username)) ||
	    ID_OK != (status=gpg_manager->Decrypt(encrypted_password, hashed_password_master_passphrase, password)))
		return status;

	return ID_OK;
}

wxmailto_status PasswordManager::SetCredential(const wxString& id, const wxString& username, const wxString& password)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_id_master_passphrase;
	wxString hashed_username_master_passphrase;
	wxString hashed_password_master_passphrase;
#ifdef WIPE_AFTER_USE
	username.WipeAfterUse();
	password.WipeAfterUse();
	master_passphrase.WipeAfterUse();
	hashed_id_master_passphrase.WipeAfterUse();
	hashed_username_master_passphrase.WipeAfterUse();
	hashed_password_master_passphrase.WipeAfterUse();
#endif
	if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, "id@wxMailto", hashed_id_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, id+"@username@wxMailto", hashed_username_master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, id+"@upassword@wxMailto", hashed_password_master_passphrase)))
		return status;

	wxString encrypted_username;
	wxString encrypted_password;
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (ID_OK != (status=gpg_manager->Encrypt(username, hashed_username_master_passphrase, encrypted_username)) ||
	    ID_OK != (status=gpg_manager->Encrypt(password, hashed_password_master_passphrase, encrypted_password)))
		return status;

	return SaveCredential(hashed_id_master_passphrase, encrypted_username, encrypted_password);
}

wxmailto_status PasswordManager::ForgetCredential(const wxString& id)
{
	wxmailto_status status;
	wxString master_passphrase;
	wxString hashed_id_master_passphrase;
	wxString encrypted_id;
#ifdef WIPE_AFTER_USE
	master_passphrase.WipeAfterUse();
	hashed_id_master_passphrase.WipeAfterUse();
#endif
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	if (ID_OK != (status=GetMasterPassphrase(master_passphrase)) ||
	    ID_OK != (status=CreateHash(master_passphrase, "id@wxMailto", hashed_id_master_passphrase)) ||
	    ID_OK != (status=gpg_manager->Encrypt(id, hashed_id_master_passphrase, encrypted_id)))
		return status;

	return DeleteCredential(encrypted_id);
}

wxmailto_status PasswordManager::LoadCredential(const wxString& WXUNUSED(id), wxString& WXUNUSED(encrypted_username), wxString& WXUNUSED(encrypted_password))
{
	return ID_NOT_IMPLEMENTED;
}

wxmailto_status PasswordManager::SaveCredential(const wxString& WXUNUSED(id), const wxString& WXUNUSED(encrypted_username), const wxString& WXUNUSED(encrypted_password))
{
	return ID_NOT_IMPLEMENTED;
}

wxmailto_status PasswordManager::DeleteCredential(const wxString& WXUNUSED(id))
{
	return ID_NOT_IMPLEMENTED;
}

wxmailto_status PasswordManager::CreateHash(const wxString& secret, const wxString& salt, wxString& hashed_value)
{
	wxString plaintext;
#ifdef WIPE_AFTER_USE
	plaintext.WipeAfterUse();
#endif
	plaintext = secret+salt;
	
	GPGManager* gpg_manager = wxGetApp().GetAppModuleManager()->GetGPGManager();
	return gpg_manager->Hash(plaintext, hashed_value);
}
