
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "gcrypt_manager.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <gcrypt.h>
#endif

#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"
#include "../string/stringutils.h"


using namespace wxMailto;

static const wxUint8 g_iv16[] = {'w'^0x55,'x'^0x55,'M'^0x55,'a'^0x55,'i'^0x55,'l'^0x55,'t'^0x55,'o'^0x55,
                                 'w'^0x01,'x'^0x23,'M'^0x45,'a'^0x67,'i'^0x89,'l'^0xAB,'t'^0xCD,'o'^0xEF};


GcryptManager::GcryptManager()
: wxMailto_Module()
{
}

wxmailto_status GcryptManager::Initialize()
{
	if (!gcry_check_version(GCRYPT_VERSION))
	{
		return LOGERROR(ID_GENERIC_ERROR);
	}
	gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
	gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);
	gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

	wxGetApp().GetAppModuleManager()->RegisterModule(this);

	return ID_OK;
}

wxmailto_status GcryptManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	gcry_control(GCRYCTL_TERM_SECMEM);

	return ID_OK;
}

wxmailto_status GcryptManager::DecryptWithDerivedKey(const wxString& encrypted, const wxUint8* derived_key, wxString& plaintext)
{
	const wxScopedCharBuffer encrypted_buffer = encrypted.ToUTF8();
	return DecryptWithDerivedKey(reinterpret_cast<const wxUint8*>(encrypted_buffer.data()), encrypted_buffer.length(), derived_key, plaintext);
}

wxmailto_status GcryptManager::DecryptWithDerivedKey(const wxUint8* encrypted, const wxSizeT& encrypted_length, const wxUint8* derived_key, wxString& plaintext)
{
	size_t outsize = GetRequiredBuffenLength(encrypted_length, GetCipherAlgorithm());
	wxUint8* out = reinterpret_cast<wxUint8*>(gcry_malloc_secure(outsize));
	if (!out)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	gcry_error_t err;
	gcry_cipher_hd_t handle;
	if (GPG_ERR_NO_ERROR != (err=gcry_cipher_open(&handle, GetCipherAlgorithm(), GetCipherMode(), GCRY_CIPHER_SECURE)))
	{
		gcry_free(out);
		return LOGERROR(ConvertStatus(err));
	}
	
	if (GPG_ERR_NO_ERROR != (err=gcry_cipher_setkey(handle, derived_key, GetDerivedKeyLength())) ||
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_setiv(handle, g_iv16, sizeof(g_iv16)/sizeof(g_iv16[0]))) ||
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_decrypt(handle, out, outsize, encrypted, encrypted_length)))
	{
		gcry_free(out);
		gcry_cipher_close(handle);
		return LOGERROR(ConvertStatus(err));
	}
	
	gcry_cipher_close(handle);

	plaintext = wxString::FromUTF8(reinterpret_cast<const char*>(out), outsize);
	gcry_free(out);

	return ID_OK;
}

wxmailto_status GcryptManager::EncryptWithDerivedKey(const wxString& plaintext, const wxUint8* derived_key, wxString& encrypted)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	return EncryptWithDerivedKey(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(), derived_key, encrypted);
}

wxmailto_status GcryptManager::EncryptWithDerivedKey(const wxUint8* plain, const wxSizeT& plain_length, const wxUint8* derived_key, wxString& encrypted)
{
	size_t outsize = GetRequiredBuffenLength(plain_length, GetCipherAlgorithm());
	wxUint8* out = new wxUint8[outsize];
	if (!out)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	gcry_error_t err;
	gcry_cipher_hd_t handle;
	if (GPG_ERR_NO_ERROR != (err=gcry_cipher_open(&handle, GetCipherAlgorithm(), GetCipherMode(), GCRY_CIPHER_SECURE)))
	{
		delete[] out;
		return LOGERROR(ConvertStatus(err));
	}

	if (GPG_ERR_NO_ERROR != (err=gcry_cipher_setkey(handle, derived_key, GetDerivedKeyLength())) ||
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_setiv(handle, g_iv16, sizeof(g_iv16)/sizeof(g_iv16[0]))) ||
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_encrypt(handle, out, outsize, plain, plain_length)))
	{
		gcry_cipher_close(handle);
		delete[] out;
		return LOGERROR(ConvertStatus(err));
	}
	
	gcry_cipher_close(handle);

	wxmailto_status status = StringUtils::ByteArrayToHexString(out, outsize, encrypted);
	delete[] out;
	return status;
}

wxmailto_status GcryptManager::Hash(const wxString& plaintext, wxString& hash)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	return Hash(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(), hash);
}

wxmailto_status GcryptManager::Hash(const wxUint8* plain, const wxSizeT& plain_length, wxString& hash)
{
	wxUint8* digest = new wxUint8[SHA512_HASH_LEN];
	if (!digest)
		return LOGERROR(ID_OUT_OF_MEMORY);

	gcry_md_hash_buffer(GCRY_MD_SHA512, digest, plain, plain_length);
	wxmailto_status status = StringUtils::ByteArrayToHexString(digest, SHA512_HASH_LEN, hash);
	delete[] digest;
	return status;
}

wxmailto_status GcryptManager::DeriveKey(const wxString& plaintext, const wxString& salt, wxUint8* derived_key)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	const wxScopedCharBuffer salt_buffer = salt.ToUTF8();
	return DeriveKey(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(),
                   reinterpret_cast<const wxUint8*>(salt_buffer.data()), salt_buffer.length(),
                   derived_key);
}

wxmailto_status GcryptManager::DeriveKey(const wxUint8* plain, const wxSizeT& plain_length,
                                      const wxUint8* salt, const wxSizeT& salt_length,
                                      wxUint8* derived_key)
{
	return ConvertStatus(gcry_kdf_derive(plain, plain_length, GCRY_KDF_PBKDF2, GCRY_MD_SHA512,
                                       salt, salt_length, DERIVED_KEY_ITERATIONS, GetDerivedKeyLength(), derived_key));
}

wxSizeT GcryptManager::GetDerivedKeyLength() const
{
	return gcry_cipher_get_algo_keylen(GetCipherAlgorithm());
}

wxSizeT GcryptManager::GetRequiredBuffenLength(const wxSizeT& length, wxInt algorithm) const
{
	wxSizeT blklen = gcry_cipher_get_algo_blklen(algorithm);
	if (0==blklen)
		return 0;

	wxSizeT blocks = (length+(blklen-1))/blklen;
	return blocks * blklen;
}
