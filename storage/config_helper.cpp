
// Copyright (C) 2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "config_helper.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "config_helper.h"

#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"


using namespace wxMailto;


wxmailto_status ConfigHelper::ReadEncrypted(const wxString key, wxString& plaintext_value, const wxString& default_value, const wxString& salt)
{
	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return ID_NULL_POINTER;

	wxString encrypted_value_hex;
	config->Read(key, &encrypted_value_hex, default_value);
	if (encrypted_value_hex.IsEmpty())
	{
		plaintext_value.Clear();
	}
	else
	{
		wxmailto_status status;
		if (ID_OK!=(status=wxGetApp().GetAppModuleManager()->GetPasswordManager()->
		    GenericDecrypt(encrypted_value_hex, plaintext_value, salt)))
		{
			return status;
		}
	}
	return ID_OK;
}

wxmailto_status ConfigHelper::WriteEncrypted(const wxString key, const wxString plaintext_value, const wxString& salt, wxBool flush)
{
	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return ID_NULL_POINTER;

	wxString encrypted_value_hex;
	if (!plaintext_value.IsEmpty())
	{
		wxmailto_status status;
		if (ID_OK!=(status=wxGetApp().GetAppModuleManager()->GetPasswordManager()->
		    GenericEncrypt(plaintext_value, encrypted_value_hex, salt)))
		{
			return status;
		}
	}

	config->Write(key, encrypted_value_hex);
	if (flush)
		config->Flush();

	return ID_OK;
}
