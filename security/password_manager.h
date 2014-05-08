#ifndef _PASSWORD_MANAGER_H_
#define _PASSWORD_MANAGER_H_

// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "password_manager.h"
#endif

#include "../gui/wxmailto_module.h"

#undef WIPE_AFTER_USE

namespace wxMailto
{

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
	wxmailto_status GetMasterPassphrase(wxString& passphrase); //master passphrase is not stored
	wxmailto_status SetMasterPassphrase(const wxString& old_passphrase, const wxString& new_passphrase);
	void ForgetMasterPassphrase();

	wxmailto_status GetSudoPassword(wxString& password); //sudo password is not stored
	wxmailto_status SetSudoPassword(wxString& password);
	void ForgetSudoPassword();

	wxmailto_status GetLocation(wxUInt id, wxString& location);
	wxmailto_status GetCredential(wxUInt id, wxString& location, wxString& username, wxString& password);
	wxmailto_status SetCredential(wxUInt& id, const wxString& location, const wxString& username, const wxString& password);
	wxmailto_status ForgetCredential(wxUInt id);
private:
	wxmailto_status GetCredential(const wxString& master_passphrase, wxUInt id, wxString& location, wxString& username, wxString& password);
	wxmailto_status SetCredential(const wxString& master_passphrase, wxUInt& id, const wxString& location, const wxString& username, const wxString& password);
	
private:
	wxmailto_status LoadLocation(wxUInt id, wxString& encrypted_location_hex);
	wxmailto_status LoadCredential(wxUInt id, wxString& encrypted_location_hex, wxString& encrypted_username_hex, wxString& encrypted_password_hex);
	wxmailto_status SaveCredential(wxUInt& id, const wxString& encrypted_location_hex, const wxString& encrypted_username_hex, const wxString& encrypted_password_hex);
	wxmailto_status UpdateCredentialPassphrase(const wxString& old_passphrase, const wxString& new_passphrase);
	wxmailto_status DeleteCredential(wxUInt id);

	wxmailto_status CreateHash(const wxString& secret, const wxString& salt, wxString& hashed_value_hex);

	wxmailto_status CreateDerivedKey(const wxString& plaintext, const wxString& salt, wxUint8* derived_key);

public:
	wxmailto_status GenericEncrypt(wxString& plaintext, wxString& encrypted_hex, const wxString& salt = "generic@wxMailto");
	wxmailto_status GenericDecrypt(const wxString& encrypted_hex, wxString& plaintext, const wxString& salt = "generic@wxMailto");

private:
	wxUint8* m_obfuscated_master_password;
	wxUint8* m_master_password_obfuscator;
	size_t m_obfuscated_master_password_length;
	wxString m_encrypted_sudo_password_hex;
};

}

#endif // _PASSWORD_MANAGER_H_
