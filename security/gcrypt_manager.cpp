
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

wxmailto_status GcryptManager::DecryptWithDerivedKey(const wxString& encrypted_hex, const wxUint8* derived_key, SafeString& plaintext)
{
	wxmailto_status status;
	wxUint8* encrypted_buffer;
	wxSizeT encrypted_buffer_length;
	if (ID_OK!=(status=StringUtils::HexStringToByteArrayAllocates(encrypted_hex, encrypted_buffer, encrypted_buffer_length)))
	{
		return status;
	}
	status = DecryptWithDerivedKey(encrypted_buffer, encrypted_buffer_length, derived_key, plaintext);
	delete[] encrypted_buffer;
	return status;
}

wxmailto_status GcryptManager::DecryptWithDerivedKey(const wxUint8* encrypted, const wxSizeT& encrypted_length, const wxUint8* derived_key, SafeString& plaintext)
{
	size_t outsize = GetRequiredBuffenLength(encrypted_length, GetCipherAlgorithm());
	wxUint8* out = reinterpret_cast<wxUint8*>(gcry_malloc_secure(outsize));
	if (!out)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	gcry_error_t gcry_err;
	gpg_err_code_t gpg_err;

	gcry_cipher_hd_t handle;
	gcry_err = gcry_cipher_open(&handle, GetCipherAlgorithm(), GetCipherMode(), GCRY_CIPHER_SECURE);
	gpg_err = gcry_err_code(gcry_err);
	if (GPG_ERR_NO_ERROR != gpg_err)
	{
		gcry_free(out);
		return LOGERROR(ConvertStatus(gpg_err));
	}

	if (GPG_ERR_NO_ERROR != gcry_err_code(gcry_err=gcry_cipher_setkey(handle, derived_key, GetDerivedKeyLength())) ||
	    GPG_ERR_NO_ERROR != gcry_err_code(gcry_err=gcry_cipher_setiv(handle, g_iv16, sizeof(g_iv16)/sizeof(g_iv16[0]))) ||
	    GPG_ERR_NO_ERROR != gcry_err_code(gcry_err=gcry_cipher_decrypt(handle, out, outsize, encrypted, encrypted_length)))
	{
		gpg_err = gcry_err_code(gcry_err);
		gcry_free(out);
		gcry_cipher_close(handle);
		return LOGERROR(ConvertStatus(gpg_err));
	}

	gcry_cipher_close(handle);

	plaintext.StrDup(out, outsize);
	gcry_free(out);

	return ID_OK;
}

wxmailto_status GcryptManager::EncryptWithDerivedKey(const SafeString& plaintext, const wxUint8* derived_key, wxString& encrypted_hex)
{
	wxmailto_status status;
	const wxUint8* plaintext_ptr;
	wxSizeT plaintext_length;
	if (ID_OK!=(status=plaintext.Get(plaintext_ptr, plaintext_length)))
	{
		return status;
	}

	size_t outsize = GetRequiredBuffenLength(plaintext_length, GetCipherAlgorithm());
	wxUint8* out = new wxUint8[outsize];
	if (!out)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	gcry_error_t gcry_err;
	gpg_err_code_t gpg_err;

	gcry_cipher_hd_t handle;
	gcry_err = gcry_cipher_open(&handle, GetCipherAlgorithm(), GetCipherMode(), GCRY_CIPHER_SECURE);
	gpg_err = gcry_err_code(gcry_err);
	if (GPG_ERR_NO_ERROR != gpg_err)
	{
		delete[] out;
		return LOGERROR(ConvertStatus(gpg_err));
	}

	if (GPG_ERR_NO_ERROR != gcry_err_code(gcry_err=gcry_cipher_setkey(handle, derived_key, GetDerivedKeyLength())) ||
	    GPG_ERR_NO_ERROR != gcry_err_code(gcry_err=gcry_cipher_setiv(handle, g_iv16, sizeof(g_iv16)/sizeof(g_iv16[0]))) ||
	    GPG_ERR_NO_ERROR != gcry_err_code(gcry_err=gcry_cipher_encrypt(handle, out, outsize, plaintext_ptr, plaintext_length)))
	{
		gpg_err = gcry_err_code(gcry_err);
		gcry_cipher_close(handle);
		delete[] out;
		return LOGERROR(ConvertStatus(gpg_err));
	}

	gcry_cipher_close(handle);

	status = StringUtils::ByteArrayToHexString(out, outsize, encrypted_hex);
	delete[] out;
	return status;
}

wxmailto_status GcryptManager::Hash(const SafeString& plaintext, wxString& hash_hex)
{
	wxmailto_status status;
	const wxUint8* plaintext_ptr;
	wxSizeT plaintext_length;
	if (ID_OK!=(status=plaintext.Get(plaintext_ptr, plaintext_length)))
	{
		return status;
	}
	
	wxUint8* digest = new wxUint8[SHA512_HASH_LEN];
	if (!digest)
		return LOGERROR(ID_OUT_OF_MEMORY);

	gcry_md_hash_buffer(GCRY_MD_SHA512, digest, plaintext_ptr, plaintext_length);
	status = StringUtils::ByteArrayToHexString(digest, SHA512_HASH_LEN, hash_hex);
	delete[] digest;
	return status;
}

wxmailto_status GcryptManager::DeriveKey(const SafeString& plaintext,
                                      const SafeString& salt,
                                      wxUint8* derived_key)
{
	wxmailto_status status;
	const wxUint8* plaintext_ptr;
	wxSizeT plaintext_length;
	const wxUint8* salt_ptr;
	wxSizeT salt_length;
	if (ID_OK!=(status=plaintext.Get(plaintext_ptr, plaintext_length)) ||
	    ID_OK!=(status=salt.Get(salt_ptr, salt_length)))
	{
		return status;
	}
	
	gcry_error_t gcry_err = gcry_kdf_derive(plaintext_ptr, plaintext_length, GCRY_KDF_PBKDF2, GCRY_MD_SHA512,
                                          salt_ptr, salt_length, DERIVED_KEY_ITERATIONS, GetDerivedKeyLength(), derived_key);
	gpg_err_code_t gpg_err = gcry_err_code(gcry_err);
	return ConvertStatus(gpg_err);
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
