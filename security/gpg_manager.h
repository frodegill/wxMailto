#ifndef _GPG_MANAGER_H_
#define _GPG_MANAGER_H_

// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "gpg_manager.h"
#endif

#include "gpg_key.h"
#include "../gui/wxmailto_module.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

class GPGManager : public wxMailto_Module
{
public:
	GPGManager();

public:  //From wxMailto_Module
	wxString GetName() const {return "GPG";}
	ModuleType GetType() const {return wxMailto_Module::GPG;}
	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status GetDefaultKey(GPGKey& key);
	wxmailto_status GetSecretKeys(GPGKeyList& key_list, wxBool& truncated);

public:
	wxmailto_status Decrypt(const wxString& encrypted, const wxString& key, wxString& plaintext);
	wxmailto_status Decrypt(const wxUint8* encrypted, const wxSizeT& encrypted_length, const wxString& key, wxString& plaintext);
	wxmailto_status Encrypt(const wxString& plaintext, const wxString& key, wxString& encrypted);
	wxmailto_status Encrypt(const wxUint8* plain, const wxSizeT& plain_length, const wxString& key, wxString& encrypted);
	wxmailto_status Hash(const wxString& plaintext, wxString& hash);
	wxmailto_status Hash(const wxUint8* plain, const wxSizeT& plain_length, wxString& hash);
	
private:
	wxmailto_status ConvertStatus(gpgme_error_t err);
	
private:
};

}

#endif // _GPG_MANAGER_H_
