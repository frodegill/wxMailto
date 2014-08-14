#ifndef _CONFIG_HELPER_H_
#define _CONFIG_HELPER_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "config_helper.h"
#endif

#include "../wxmailto_errors.h"
#include "../string/safestring.h"

namespace wxMailto
{

#define CONFIG_CONNECTIONSTRING "ConnectionString"


class ConfigHelper
{
private:
	ConfigHelper() {}

public:
	static wxmailto_status ReadEncrypted(const wxString key, SafeString& plaintext_value, const SafeString& default_value, const SafeString& salt);
	static wxmailto_status WriteEncrypted(const wxString key, const SafeString& plaintext_value, const SafeString& salt, wxBool flush=true);
};

}

#endif // _CONFIG_HELPER_H_
