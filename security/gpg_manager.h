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

#define AES256_KEY_LEN  (32)
#define SHA512_HASH_LEN (64)
#define DERIVED_KEY_ITERATIONS (2048)
#define DERIVED_KEY_LEN (SHA512_HASH_LEN)


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
	wxmailto_status DecryptWithDerivedKey(const wxString& encrypted, const wxUint8* derived_key, wxString& plaintext);
	wxmailto_status DecryptWithDerivedKey(const wxUint8* encrypted, const wxSizeT& encrypted_length, const wxUint8* derived_key, wxString& plaintext);
	wxmailto_status EncryptWithDerivedKey(const wxString& plaintext, const wxUint8* derived_key, wxString& encrypted);
	wxmailto_status EncryptWithDerivedKey(const wxUint8* plain, const wxSizeT& plain_length, const wxUint8* derived_key, wxString& encrypted);
	wxmailto_status Hash(const wxString& plaintext, wxString& hash);
	wxmailto_status Hash(const wxUint8* plain, const wxSizeT& plain_length, wxString& hash);
	wxmailto_status DeriveKey(const wxString& plaintext, const wxString& salt, wxUint8* derived_key);
	wxmailto_status DeriveKey(const wxUint8* plain, const wxSizeT& plain_length,
														const wxUint8* salt, const wxSizeT& salt_length,
														wxUint8* derived_key);
	
private:
	wxmailto_status ConvertStatus(gpgme_error_t err);
	
private:
};

}

#endif // _GPG_MANAGER_H_
