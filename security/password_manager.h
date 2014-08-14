#ifndef _PASSWORD_MANAGER_H_
#define _PASSWORD_MANAGER_H_

// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "password_manager.h"
#endif

#include "../gui/wxmailto_module.h"


namespace wxMailto
{

#define GENERIC_SALT  "generic@wxMailto"
#define LOCATION_SALT "%d@location@wxMailto"
#define PASSWORD_SALT "%d@password@wxMailto"
#define SUDO_SALT     "sudo@wxMailto"
#define USERNAME_SALT "%d@username@wxMailto"

class PasswordManager : public wxMailto_Module
{
public:
	PasswordManager();
	virtual ~PasswordManager();
	
public:  //From wxMailto_Module
	wxString GetName() const {return "PASSWORD";}
	ModuleType GetType() const {return wxMailto_Module::PASSWORD;}
	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status GetMasterPassphrase(SafeString& passphrase); //master passphrase is not stored
	wxmailto_status SetMasterPassphrase(const SafeString& old_passphrase, const SafeString& new_passphrase);
	void ForgetMasterPassphrase();

	wxmailto_status GetSudoPassword(SafeString& password); //sudo password is not stored
	wxmailto_status SetSudoPassword(const SafeString& password);
	void ForgetSudoPassword();

	wxmailto_status GetLocation(wxUInt id, SafeString& location);
	wxmailto_status GetCredential(wxUInt id, SafeString& location, SafeString& username, SafeString& password);
	wxmailto_status SetCredential(wxUInt& id, const SafeString& location, const SafeString& username, const SafeString& password);
	wxmailto_status ForgetCredential(wxUInt id);
private:
	wxmailto_status GetCredential(const SafeString& master_passphrase, wxUInt id, SafeString& location, SafeString& username, SafeString& password);
	wxmailto_status SetCredential(const SafeString& master_passphrase, wxUInt& id, const SafeString& location, const SafeString& username, const SafeString& password);
	
private:
	wxmailto_status LoadLocation(wxUInt id, wxString& encrypted_location_hex);
	wxmailto_status LoadCredential(wxUInt id, wxString& encrypted_location_hex, wxString& encrypted_username_hex, wxString& encrypted_password_hex);
	wxmailto_status SaveCredential(wxUInt& id, const wxString& encrypted_location_hex, const wxString& encrypted_username_hex, const wxString& encrypted_password_hex);
	wxmailto_status UpdateCredentialPassphrase(const SafeString& old_passphrase, const SafeString& new_passphrase);
	wxmailto_status DeleteCredential(wxUInt id);

	wxmailto_status CreateHash(const SafeString& secret, const SafeString& salt, wxString& hashed_value_hex);

	wxmailto_status CreateDerivedKey(const SafeString& plaintext, const SafeString& salt, wxUint8* derived_key);

public:
	wxmailto_status GenericEncrypt(const SafeString& plaintext, const SafeString& salt, wxString& encrypted_hex);
	wxmailto_status GenericDecrypt(const wxString& encrypted_hex, const SafeString& salt, SafeString& plaintext);

private:
	wxUint8* m_obfuscated_master_password;
	wxUint8* m_master_password_obfuscator;
	size_t m_obfuscated_master_password_length;
	wxString m_encrypted_sudo_password_hex;
};

}

#endif // _PASSWORD_MANAGER_H_
