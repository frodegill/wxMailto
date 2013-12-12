#ifndef _PASSWORD_MANAGER_H_
#define _PASSWORD_MANAGER_H_

// Copyright (C) 2013  Frode Roxrud Gill
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
	wxmailto_status SetMasterPassphrase(wxString& passphrase);
	void ForgetMasterPassphrase();

	wxmailto_status GetSudoPassword(wxString& password); //sudo password is not stored
	wxmailto_status SetSudoPassword(wxString& password);
	void ForgetSudoPassword();

	wxmailto_status GetCredential(const wxString& id, wxString& username, wxString& password);
	wxmailto_status SetCredential(const wxString& id, const wxString& username, const wxString& password);
	wxmailto_status ForgetCredential(const wxString& id);

private:
	wxmailto_status LoadCredential(const wxString& id, wxString& encrypted_username, wxString& encrypted_password);
	wxmailto_status SaveCredential(const wxString& id, const wxString& encrypted_username, const wxString& encrypted_password);
	wxmailto_status DeleteCredential(const wxString& id);

	wxmailto_status CreateHash(const wxString& secret, const wxString& salt, wxString& hashed_value);

private:
	char* m_obfuscated_master_password;
	char* m_master_password_obfuscator;
	size_t m_obfuscated_master_password_length;
	wxString m_encrypted_sudo_password;
};

}

#endif // _PASSWORD_MANAGER_H_
