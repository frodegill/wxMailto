
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

//Test
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include "../stream/base64_bufferedstream.h"


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

	wxGetApp().GetAppModuleManager()->RegisterModule(this);


	//Great place for testing, this location is called early in startup...

	{
		wxFileOutputStream fos("/tmp/test.txt");

		Base64OutputStream b64s(&fos, 3*1024, 4*1024);
		if (b64s.IsOk())
		{
			char buf[] = {0, 1, 2, 3};
			wxMemoryInputStream input(buf, 4);

			b64s.Write(input);
			b64s.Close();
		}
	}

	{
		wxFileInputStream fis("/tmp/test.txt");

		Base64InputStream b64s(&fis, 4*1024, 3*1024);
		if (b64s.IsOk())
		{
			wxFileOutputStream fos("/tmp/test2.txt");

			b64s.Read(fos);
			b64s.Close();
		}
	}

#if 0
	wxUint8* buffer = new wxUint8[100];
	strcpy((char*)buffer, "Test!\n");
	LOGDEBUG("gpg_manager: Created buffer\n");
	MemorySource* source = new MemorySource(1, buffer, 6, true);
	LOGDEBUG("gpg_manager: Created MemorySource\n");

	SinkResult sink_result(2);
	LOGDEBUG("gpg_manager: Created sink_result\n");
	FileSink* sink = new FileSink(3, &sink_result, "/tmp/test.txt");
	LOGDEBUG("gpg_manager: Created sink /tmp/test.txt\n");
	source->ConnectTo(sink);
	LOGDEBUG("gpg_manager: Connected source to sink\n");
	
	LOGDEBUG("gpg_manager: before flow\n");
	sink->StartFlow();
	LOGDEBUG("gpg_manager: flow started. Wait\n");
	sink_result.Wait();
	LOGDEBUG("gpg_manager: After wait\n");
	delete sink;
	LOGDEBUG("gpg_manager: deleted sink\n");
	delete source;
	LOGDEBUG("gpg_manager: deleted source\n");
#endif
	
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
