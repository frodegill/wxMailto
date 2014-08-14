
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


wxmailto_status ConfigHelper::ReadEncrypted(const wxString key, SafeString& plaintext_value, const SafeString& default_value, const SafeString& salt)
{
	wxConfigBase* config = wxConfigBase::Get();
	wxASSERT(NULL!=config);
	if (!config)
		return ID_NULL_POINTER;

	wxmailto_status status;

	wxString encrypted_value_hex;
	const char* default_value_str;
	if (ID_OK!=(status=default_value.GetStr(default_value_str)))
		return status;
	
	config->Read(key, &encrypted_value_hex, default_value_str);
	if (encrypted_value_hex.IsEmpty())
	{
		plaintext_value.Clear();
	}
	else
	{
		if (ID_OK!=(status=wxGetApp().GetAppModuleManager()->GetPasswordManager()->
		    GenericDecrypt(encrypted_value_hex, salt, plaintext_value)))
		{
			return status;
		}
	}
	return ID_OK;
}

wxmailto_status ConfigHelper::WriteEncrypted(const wxString key, const SafeString& plaintext_value, const SafeString& salt, wxBool flush)
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
		    GenericEncrypt(plaintext_value, salt, encrypted_value_hex)))
		{
			return status;
		}
	}

	config->Write(key, encrypted_value_hex);
	if (flush)
		config->Flush();

	return ID_OK;
}
