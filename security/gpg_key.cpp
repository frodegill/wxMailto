
// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "gpg_key.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include "gpgme.h"
#endif

#include "gpg_key.h"


using namespace wxMailto;

# include <wx/listimpl.cpp>
WX_DEFINE_LIST(GPGKeyList);


GPGKey::GPGKey(const wxString& keyid, const wxString& fingerprint,
               const wxString& name, const wxString& email,
               wxBool is_revoked, wxBool is_expired, wxBool is_disabled,
               wxBool is_invalid, wxBool can_encrypt, wxBool can_sign,
               wxBool is_secret)
:	m_keyid(keyid),
  m_fingerprint(fingerprint),
  m_name(name),
  m_email(email),
  m_is_revoked(is_revoked!=false),
  m_is_expired(is_expired!=false),
  m_is_disabled(is_disabled!=false),
  m_is_invalid(is_invalid!=false),
  m_can_encrypt(can_encrypt!=false),
  m_can_sign(can_sign!=false),
  m_is_secret(is_secret!=false)
{
}

GPGKey::~GPGKey()
{
}
