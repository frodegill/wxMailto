
// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "gpg_manager.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <locale.h>
# include "gpgme.h"
#endif

#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"
#include "gpg_manager.h"


using namespace wxMailto;


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

	wxGetApp().GetAppModuleManager()->RegisterModule(this);


	GPGKey key;
	GetDefaultKey(key);

	return ID_OK;
}

wxmailto_status GPGManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

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

wxmailto_status GPGManager::Decrypt(const wxString& encrypted, const wxString& key, wxString& plaintext)
{
	const wxScopedCharBuffer encrypted_buffer = encrypted.ToUTF8();
	return Decrypt(reinterpret_cast<const wxUint8*>(encrypted_buffer.data()), encrypted_buffer.length(), key, plaintext);
}

wxmailto_status GPGManager::Decrypt(const wxUint8* WXUNUSED(encrypted), const wxSizeT& WXUNUSED(encrypted_length), const wxString& WXUNUSED(key), wxString& WXUNUSED(plaintext))
{
	return ID_NOT_IMPLEMENTED;
}

wxmailto_status GPGManager::Encrypt(const wxString& plaintext, const wxString& key, wxString& encrypted)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	return Encrypt(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(), key, encrypted);
}

wxmailto_status GPGManager::Encrypt(const wxUint8* WXUNUSED(plain), const wxSizeT& WXUNUSED(plain_length), const wxString& WXUNUSED(key), wxString& WXUNUSED(encrypted))
{
	return ID_NOT_IMPLEMENTED;
}

wxmailto_status GPGManager::Hash(const wxString& plaintext, wxString& hash)
{
	const wxScopedCharBuffer plaintext_buffer = plaintext.ToUTF8();
	return Hash(reinterpret_cast<const wxUint8*>(plaintext_buffer.data()), plaintext_buffer.length(), hash);
}

wxmailto_status GPGManager::Hash(const wxUint8* WXUNUSED(plain), const wxSizeT& WXUNUSED(plain_length), wxString& WXUNUSED(hash))
{
	return ID_NOT_IMPLEMENTED;
}

wxmailto_status GPGManager::ConvertStatus(gpgme_error_t err)
{
	switch(err)
	{
		case GPG_ERR_NO_ERROR: return ID_OK;
		case GPG_ERR_ENOMEM: return ID_OUT_OF_MEMORY;
		default: return LOGERROR(ID_GENERIC_ERROR);
	}
}
