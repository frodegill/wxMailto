
// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "gpg_manager.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <gcrypt.h>
# include <locale.h>
# include "gpgme.h"
#endif

#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"
#include "../string/stringutils.h"
#include "gpg_manager.h"


using namespace wxMailto;

static const wxUint8 g_iv[] = {'w'^0x55,'x'^0x55,'M'^0x55,'a'^0x55,'i'^0x55,'l'^0x55,'t'^0x55,'o'^0x55,
                               'w'^0x01,'x'^0x23,'M'^0x45,'a'^0x67,'i'^0x89,'l'^0xAB,'t'^0xCD,'o'^0xEF};


GPGManager::GPGManager()
: wxMailto_Module()
{
}

wxmailto_status GPGManager::Initialize()
{
	//Initialization copied from GPGME documentation
	setlocale (LC_ALL, "");
	const char* version = gpgme_check_version(NULL);
	if (!version)
		return LOGERROR(ID_GENERIC_ERROR);

	gpgme_set_locale(NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
#ifdef LC_MESSAGES
	gpgme_set_locale(NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
#endif

  if (GPG_ERR_NO_ERROR != gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP))
		return LOGERROR(ID_GENERIC_ERROR);

	if (!gcry_check_version(GCRYPT_VERSION))
	{
		return LOGERROR(ID_GENERIC_ERROR);
	}
	gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
	gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);
	gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

	wxGetApp().GetAppModuleManager()->RegisterModule(this);


	GPGKey key;
	GetDefaultKey(key);

	return ID_OK;
}

wxmailto_status GPGManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	gcry_control(GCRYCTL_TERM_SECMEM);

	return ID_OK;
}

wxmailto_status GPGManager::GetDefaultKey(GPGKey& WXUNUSED(key))
{
	ThreadedModalDialogLauncherData* data = new ThreadedModalDialogLauncherData();
	wxThreadEvent* evt = new wxThreadEvent(wxEVT_COMMAND_THREAD, wxGetApp().GetWindowID(IDManager::ID_GPG_KEY_DIALOG));
	if (!data || !evt)
	{
		delete data;
		delete evt;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	evt->SetPayload<ThreadedModalDialogLauncherData*>(data);

	wxGetApp().AddExitBlocker();
	wxQueueEvent(wxGetApp().GetMainFrame(), evt);
	
	return ID_OK;
}

wxmailto_status GPGManager::GetSecretKeys(GPGKeyList& key_list, wxBool& truncated)
{
	key_list.Clear();

	gpgme_ctx_t ctx;
	gpgme_key_t gpgme_key;

	gpgme_error_t err;
	if (GPG_ERR_NO_ERROR!=(err=gpgme_new(&ctx)))
		return ConvertStatus(err);
	
	if (GPG_ERR_NO_ERROR!=(err=gpgme_op_keylist_start(ctx, NULL, true)))
	{
		gpgme_release(ctx);
		return ConvertStatus(err);
	}

	while (true)
	{
		if (GPG_ERR_NO_ERROR!=(err=gpgme_op_keylist_next(ctx, &gpgme_key)))
			break;

		if (!gpgme_key->revoked && !gpgme_key->expired && !gpgme_key->disabled && !gpgme_key->invalid &&
			  gpgme_key->can_encrypt && gpgme_key->can_sign && gpgme_key->secret)
		{
			GPGKey* key = new GPGKey(gpgme_key->subkeys->keyid, gpgme_key->subkeys->fpr,
               gpgme_key->uids->name, gpgme_key->uids->email,
               gpgme_key->revoked, gpgme_key->expired, gpgme_key->disabled,
               gpgme_key->invalid, gpgme_key->can_encrypt, gpgme_key->can_sign,
               gpgme_key->secret);
			
			key_list.Append(key);

			fprintf(stderr, "%s:", gpgme_key->subkeys->keyid);

			if (gpgme_key->uids && gpgme_key->uids->name)
				fprintf(stderr, " %s", gpgme_key->uids->name);

			if (gpgme_key->uids && gpgme_key->uids->email)
				fprintf(stderr, " <%s>", gpgme_key->uids->email);
		}

		gpgme_key_release(gpgme_key);
	}
	
	if (GPG_ERR_EOF == err)
		err = GPG_ERR_NO_ERROR;

	if (GPG_ERR_NO_ERROR != err)
	{
		fprintf(stderr, "can not list keys: %s\n", gpgme_strerror(err));
	}
	
	gpgme_keylist_result_t list_result = gpgme_op_keylist_result(ctx);
	truncated = list_result->truncated;

	gpgme_release (ctx);
	return ConvertStatus(err);
}

wxmailto_status GPGManager::DecryptWithDerivedKey(const wxString& encrypted, const wxUint8* derived_key, wxString& plaintext)
{
	const wxScopedCharBuffer encrypted_buffer = encrypted.ToUTF8();
	return DecryptWithDerivedKey(reinterpret_cast<const wxUint8*>(encrypted_buffer.data()), encrypted_buffer.length(), derived_key, plaintext);
}

wxmailto_status GPGManager::DecryptWithDerivedKey(const wxUint8* encrypted, const wxSizeT& encrypted_length, const wxUint8* derived_key, wxString& plaintext)
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
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_setiv(handle, g_iv, sizeof(g_iv)/sizeof(g_iv[0]))) ||
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

wxmailto_status GPGManager::EncryptWithDerivedKey(const wxString& plaintext, const wxUint8* derived_key, wxString& encrypted)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	return EncryptWithDerivedKey(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(), derived_key, encrypted);
}

wxmailto_status GPGManager::EncryptWithDerivedKey(const wxUint8* plain, const wxSizeT& plain_length, const wxUint8* derived_key, wxString& encrypted)
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
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_setiv(handle, g_iv, sizeof(g_iv)/sizeof(g_iv[0]))) ||
	    GPG_ERR_NO_ERROR != (err=gcry_cipher_encrypt(handle, out, outsize, plain, plain_length)))
	{
		gcry_cipher_close(handle);
		return LOGERROR(ConvertStatus(err));
	}
	
	gcry_cipher_close(handle);

	encrypted = wxString::FromUTF8(reinterpret_cast<const char*>(out), outsize);
	delete[] out;

	return ID_OK;
}

wxmailto_status GPGManager::Hash(const wxString& plaintext, wxString& hash)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	return Hash(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(), hash);
}

wxmailto_status GPGManager::Hash(const wxUint8* plain, const wxSizeT& plain_length, wxString& hash)
{
	wxUint8* digest = new wxUint8[SHA512_HASH_LEN];
	if (!digest)
		return LOGERROR(ID_OUT_OF_MEMORY);

	gcry_md_hash_buffer(GCRY_MD_SHA512, digest, plain, plain_length);
	wxmailto_status status = StringUtils::ByteArrayToHexString(digest, SHA512_HASH_LEN, hash);
	delete[] digest;
	return status;
}

wxmailto_status GPGManager::DeriveKey(const wxString& plaintext, const wxString& salt, wxUint8* derived_key)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	const wxScopedCharBuffer salt_buffer = salt.ToUTF8();
	return DeriveKey(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(),
                   reinterpret_cast<const wxUint8*>(salt_buffer.data()), salt_buffer.length(),
                   derived_key);
}

wxmailto_status GPGManager::DeriveKey(const wxUint8* plain, const wxSizeT& plain_length,
                                      const wxUint8* salt, const wxSizeT& salt_length,
                                      wxUint8* derived_key)
{
	return ConvertStatus(gcry_kdf_derive(plain, plain_length, GCRY_KDF_PBKDF2, GCRY_MD_SHA512,
                                       salt, salt_length, DERIVED_KEY_ITERATIONS, GetDerivedKeyLength(), derived_key));
}

wxSizeT GPGManager::GetDerivedKeyLength() const
{
	return gcry_cipher_get_algo_keylen(GetCipherAlgorithm());
}

wxSizeT GPGManager::GetRequiredBuffenLength(const wxSizeT& length, wxInt algorithm) const
{
	wxSizeT blklen = gcry_cipher_get_algo_blklen(algorithm);
	if (0==blklen)
		return 0;

	wxSizeT blocks = (length+(blklen-1))/blklen;
	return blocks * blklen;
}

wxmailto_status GPGManager::ConvertStatus(gpgme_error_t err)
{
	gcry_err_code_t error_code = gcry_err_code(err);
 	switch(error_code)
	{
		case GPG_ERR_NO_ERROR: return ID_OK;
		case GPG_ERR_ENOMEM: return ID_OUT_OF_MEMORY;
		default: return LOGERROR(ID_GENERIC_ERROR);
	}
}
