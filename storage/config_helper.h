#ifndef _CONFIG_HELPER_H_
#define _CONFIG_HELPER_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "config_helper.h"
#endif

#include "../wxmailto_errors.h"


namespace wxMailto
{

#define CONFIG_CONNECTIONSTRING "ConnectionString"


class ConfigHelper
{
private:
	ConfigHelper() {}

public:
	static wxmailto_status ReadEncrypted(const wxString key, wxString& plaintext_value, const wxString& default_value, const wxString& salt);
	static wxmailto_status WriteEncrypted(const wxString key, const wxString plaintext_value, const wxString& salt, wxBool flush=true);
};

}

#endif // _CONFIG_HELPER_H_
