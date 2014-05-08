#ifndef _GCRYPT_MANAGER_H_
#define _GCRYPT_MANAGER_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "gcrypt_manager.h"
#endif

#include "gpg_manager.h"
#include "../gui/wxmailto_module.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

#define AES256_KEY_LEN  (32)
#define SHA512_HASH_LEN (64)
#define DERIVED_KEY_ITERATIONS (2048)


class GcryptManager : public wxMailto_Module
{
public:
	GcryptManager();

public:  //From wxMailto_Module
	wxString GetName() const {return "Gcrypt";}
	ModuleType GetType() const {return wxMailto_Module::GCRYPT;}
	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status DecryptWithDerivedKey(const wxString& encrypted_hex, const wxUint8* derived_key, wxString& plaintext);
	wxmailto_status DecryptWithDerivedKey(const wxUint8* encrypted, const wxSizeT& encrypted_length, const wxUint8* derived_key, wxString& plaintext);
	wxmailto_status EncryptWithDerivedKey(const wxString& plaintext, const wxUint8* derived_key, wxString& encrypted_hex);
	wxmailto_status EncryptWithDerivedKey(const wxUint8* plain, const wxSizeT& plain_length, const wxUint8* derived_key, wxString& encrypted_hex);

	wxmailto_status Hash(const wxString& plaintext, wxString& hash_hex);
	wxmailto_status Hash(const wxUint8* plain, const wxSizeT& plain_length, wxString& hash_hex);
	wxmailto_status DeriveKey(const wxString& plaintext, const wxString& salt, wxUint8* derived_key);
	wxmailto_status DeriveKey(const wxUint8* plain, const wxSizeT& plain_length,
														const wxUint8* salt, const wxSizeT& salt_length,
														wxUint8* derived_key);

public:
	wxInt GetCipherAlgorithm() const {return GCRY_CIPHER_AES256;}
	wxInt GetCipherMode() const {return GCRY_CIPHER_MODE_CBC;}
	wxSizeT GetDerivedKeyLength() const;
	wxSizeT GetRequiredBuffenLength(const wxSizeT& length, wxInt algorithm) const;

public:
	static wxmailto_status ConvertStatus(gcry_error_t err) {return GPGManager::ConvertStatus(err);}

private:
};

}

#endif // _GCRYPT_MANAGER_H_
