
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
#include "../storage/persistent_object.h"
#include "gpg_manager.h"

#ifdef RUN_TESTS
# include "../test/unit_tests.h"
#endif


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

  if (GPG_ERR_NO_ERROR != gpg_err_code(gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP)))
		return LOGERROR(ID_GENERIC_ERROR);

	wxGetApp().GetAppModuleManager()->RegisterModule(this);


#ifdef RUN_TESTS
	//Great place for testing, this location is called early in startup...
	UnitTests unit_tests;
	unit_tests.RunTests();
#endif //RUN_TESTS

#if 0
	GPGKey key;
	GetDefaultKey(key);
#endif

	return ID_OK;
}

wxmailto_status GPGManager::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}

wxmailto_status GPGManager::CreateContext(GPGContext& context)
{
	switch(gpgme_new(&context))
	{
		case GPG_ERR_NO_ERROR: return ID_OK;
		case GPG_ERR_ENOMEM: return LOGERROR(ID_OUT_OF_MEMORY);
		default: return LOGERROR(ID_GENERIC_ERROR);
	}
}

wxmailto_status GPGManager::ReleaseContext(GPGContext& context)
{
	if (!context)
		return LOGERROR(ID_NULL_POINTER);
	
	gpgme_release(context);
	return ID_OK;
}

wxmailto_status GPGManager::GetDefaultKeyID(wxInt& id)
{
	PersistentProperty* property = new PersistentProperty();
	if (!property)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	wxBool exists;
	wxmailto_status status = property->Initialize(GPG_DEFAULT_KEY, &wxGetApp().GetGlobalLockers()->m_generic_property_lock);
	if (ID_OK == status)
	{
	    status = property->GetIntValue(id, exists);
	}
	delete property;

	if (ID_OK==status && !exists)
	{
		GPGContext context;
		if (ID_OK==(status=CreateContext(context)))
		{
			status = GenerateKey(context, id);
			ReleaseContext(context);
		}
	}
	return status;
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
	gpgme_error_t gpgme_err;
	gpg_err_code_t gpg_err;

	key_list.Clear();

	gpgme_ctx_t ctx;
	gpgme_key_t gpgme_key;

	gpgme_err=gpgme_new(&ctx);
	gpg_err=gpg_err_code(gpgme_err);
	if (GPG_ERR_NO_ERROR!=gpg_err)
		return ConvertStatus(gpg_err);
	
	gpgme_err=gpgme_op_keylist_start(ctx, NULL, true);
	gpg_err=gpg_err_code(gpgme_err);
	if (GPG_ERR_NO_ERROR!=gpg_err)
	{
		gpgme_release(ctx);
		return ConvertStatus(gpg_err);
	}

	while (true)
	{
		gpgme_err=gpgme_op_keylist_next(ctx, &gpgme_key);
		gpg_err=gpg_err_code(gpgme_err);
		if (GPG_ERR_NO_ERROR!=gpg_err)
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
	
	if (GPG_ERR_EOF == gpg_err)
		gpg_err = GPG_ERR_NO_ERROR;

	if (GPG_ERR_NO_ERROR != gpg_err)
	{
		fprintf(stderr, "can not list keys: %s\n", gpgme_strerror(gpgme_err));
	}
	
	gpgme_keylist_result_t list_result = gpgme_op_keylist_result(ctx);
	truncated = list_result->truncated;

	gpgme_release (ctx);
	return ConvertStatus(gpg_err);
}

wxmailto_status GPGManager::GenerateKey(GPGContext& context, wxInt& WXUNUSED(id))
{
	const char* parms = "<GnupgKeyParms format=\"internal\">\r\n"\
                      "Key-Type: default\r\n"\
                      "Subkey-Type: default\r\n"\
                      "Name-Real: Joe Tester\r\n"\
                      "Name-Comment: with stupid passphrase\r\n"\
                      "Name-Email: joe@foo.bar\r\n"\
                      "Expire-Date: 0\r\n"\
                      "Passphrase: abc\r\n"\
                      "</GnupgKeyParms>";
	wxmailto_status status = (GPG_ERR_NO_ERROR==gpgme_op_genkey(context, parms, NULL, NULL)) ? ID_OK : ID_GENERIC_ERROR;
	if (ID_OK==status) {
		GPGGenKeyResult key_result = gpgme_op_genkey_result(context);
		if (!key_result || 0==key_result->primary)
		{
			status = ID_GENERIC_ERROR;
		}
	}
	return status;
}

wxmailto_status GPGManager::ConvertStatus(gpg_err_code_t error_code)
{
 	switch(error_code)
	{
		case GPG_ERR_NO_ERROR: return ID_OK;
		case GPG_ERR_ENOMEM: return LOGERROR(ID_OUT_OF_MEMORY);
		default: return LOGERROR(ID_GENERIC_ERROR);
	}
}
