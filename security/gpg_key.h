#ifndef _GPG_KEY_H_
#define _GPG_KEY_H_

// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "gpg_key.h"
#endif

#include "../wxmailto_errors.h"


namespace wxMailto
{

class GPGKey;
WX_DECLARE_LIST(GPGKey, GPGKeyList);

class GPGKey
{
public:
	GPGKey(const wxString& keyid=wxEmptyString, const wxString& fingerprint=wxEmptyString,
	       const wxString& name=wxEmptyString, const wxString& email=wxEmptyString,
	       wxBool is_revoked=true, wxBool is_expired=true, wxBool is_disabled=true,
	       wxBool is_invalid=true, wxBool can_encrypt=false, wxBool can_sign=false,
	       wxBool is_secret=false);
	virtual ~GPGKey();

public:
	const wxString& GetKeyId() const {return m_keyid;}
	const wxString& GetFingerprint() const {return m_fingerprint;}
	const wxString& GetName() const {return m_name;}
	const wxString& GetEmail() const {return m_email;}
	wxBool IsRevoked() const {return m_is_revoked;}
	wxBool IsExpired() const {return m_is_expired;}
	wxBool IsDisabled() const {return m_is_disabled;}
	wxBool IsInvalid() const {return m_is_invalid;}
	wxBool CanEncrypt() const {return m_can_encrypt;}
	wxBool CanSign() const {return m_can_sign;}
	wxBool IsSecret() const {return m_is_secret;}

private:
	wxString m_keyid;
	wxString m_fingerprint;
	wxString m_name;
	wxString m_email;
	wxUInt m_is_revoked:1;
	wxUInt m_is_expired:1;
	wxUInt m_is_disabled:1;
	wxUInt m_is_invalid:1;
	wxUInt m_can_encrypt:1;
	wxUInt m_can_sign:1;
	wxUInt m_is_secret:1;
};

}

#endif // _GPG_KEY_H_
