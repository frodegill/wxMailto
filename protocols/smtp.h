#ifndef _SMTP_H_
#define _SMTP_H_

// Copyright (C) 2008-2011  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "smtp.h"
#endif

#include <wx/socket.h>

#include "../contacts/contact.h"
#include "../mail/outgoing_message.h"
#include "../storage/account.h"
#include "protocol_iface.h"


namespace wxMailto
{

struct SmtpEhloFeatures
{
	void Reset();
	void DisableEhloSupport();
	void SetAllAuthentications(wxBool enable=false);
	wxString GetSupportedAuthMethods() const;
	wxmailto_status DisableAuthMethod(const wxString& auth_method);

	wxBool m_ehlo_supported; //RFC1651

	wxBool m_ehlo_SEND_supported; //RFC1651 §5
	wxBool m_ehlo_SOML_supported; //RFC1651 §5
	wxBool m_ehlo_SAML_supported; //RFC1651 §5
	wxBool m_ehlo_EXPN_supported; //RFC1651 §5
	wxBool m_ehlo_HELP_supported; //RFC1651 §5
	wxBool m_ehlo_TURN_supported; //RFC1651 §5
	wxBool m_ehlo_8BITMIME_supported; //RFC2821 §2.4

	wxBool m_auth_ANONYMOUS_supported;
	wxBool m_auth_EXTERNAL_supported;
	wxBool m_auth_PLAIN_supported;
	wxBool m_auth_LOGIN_supported;
	wxBool m_auth_SECURID_supported;
	wxBool m_auth_NTLM_supported;
	wxBool m_auth_CRAMMD5_supported;
	wxBool m_auth_DIGESTMD5_supported;
	wxBool m_auth_GSSAPI_supported;
	wxBool m_auth_KERBEROS5_supported;
	wxBool m_auth_SCRAMSHA1_supported;
};


class Smtp : public ProtocolInterface
{
public:
	Smtp(Account* account);
	virtual ~Smtp();

public: //ProtocolInterface API
	wxmailto_status Sync();
	wxmailto_status CleanupAndAbort();
	wxmailto_status ReadAuthenticationNegotiationLine(SafeString& buffer);
	wxmailto_status WriteAuthenticationNegotiationLine(const SafeString& buffer);
	void WriteAuthenticationAbortedLine();

public:
	wxInt SmtpStatus(const wxString& smtp_string, wxBool& more) const;

private:
	wxmailto_status ReadInitialGreeting();
	wxmailto_status HandleHELO(const wxString& fqdn_idna_domain);
	wxmailto_status HandleEHLO(const wxString& fqdn_idna_domain);
	wxmailto_status HandleMAIL(const Contact* contact);
	wxmailto_status HandleRCPT(const Contact* contact);
	wxmailto_status HandleDATA(OutgoingMessage* message);
	wxmailto_status HandleRSET();
	wxmailto_status HandleVRFY(Contact& address);
	wxmailto_status HandleEXPAND(const Contact& address, ContactList& addresses);
	wxmailto_status HandleQUIT();

	wxmailto_status HandleSTARTTLS();
	wxmailto_status HandleAUTHENTICATE();

private:
	/*
	 * return: ID_OK (Authenticated successfully)
	 *         ID_AUTHENTICATION_FAILED (Authentication failed)
	 *         ID_AUTHENTICATION_NOT_SUPPORTED (Method was not supported by server. Try another one, or fail)
	*/
	wxmailto_status HandleAUTHENTICATE(const wxString& authenticate_method);
	wxmailto_status HandleAUTHENTICATE_ANONYMOUS();
	wxmailto_status HandleAUTHENTICATE_EXTERNAL();
	wxmailto_status HandleAUTHENTICATE_PLAIN();
	wxmailto_status HandleAUTHENTICATE_LOGIN();
	wxmailto_status HandleAUTHENTICATE_SECURID();
	wxmailto_status HandleAUTHENTICATE_NTLM();
	wxmailto_status HandleAUTHENTICATE_CRAM_MD5();
	wxmailto_status HandleAUTHENTICATE_DIGEST_MD5();
	wxmailto_status HandleAUTHENTICATE_GSSAPI();
	wxmailto_status HandleAUTHENTICATE_KERBEROS5();
	wxmailto_status HandleAUTHENTICATE_SCRAM_SHA1();

	wxmailto_status ParseEhloKeyword(const wxString& ehlo_keyword_string);
	wxmailto_status ParseVRFY_EXPAND(const wxString& vrfy_string, Contact*& result) const; //If returning true, caller must delete result

private:
	wxString m_server_fqdn;
	SmtpEhloFeatures m_supported_ehlo_features;
};

}

#endif // _SMTP_H_
