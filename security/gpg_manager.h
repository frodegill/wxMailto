#ifndef _GPG_MANAGER_H_
#define _GPG_MANAGER_H_

// Copyright (C) 2013-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "gpg_manager.h"
#endif

#include "gpg_key.h"
#include "../gui/wxmailto_module.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

#define GPGContext gpgme_ctx_t
#define GPGGenKeyResult gpgme_genkey_result_t


class GPGManager : public wxMailto_Module
{
public:
	GPGManager();

public:  //From wxMailto_Module
	wxString GetName() const {return "GPG";}
	ModuleType GetType() const {return wxMailto_Module::GPG;}
	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status CreateContext(GPGContext& context);
	wxmailto_status ReleaseContext(GPGContext& context);

	wxmailto_status GetDefaultKeyID(wxInt& id);
	wxmailto_status GetDefaultKey(GPGKey& key);
	wxmailto_status GetSecretKeys(GPGKeyList& key_list, wxBool& truncated);

	wxmailto_status GenerateKey(GPGContext& context, wxInt& id);

public:
	static wxmailto_status ConvertStatus(gpg_err_code_t error_code);
	
private:
};

}

#endif // _GPG_MANAGER_H_
