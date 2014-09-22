#ifndef _POP_H_
#define _POP_H_

// Copyright (C) 2008-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "pop.h"
#endif

#include <wx/list.h>
#include "protocol_iface.h"


namespace wxMailto
{

#define POP_NO_LOGIN_DELAY (-1)

#define POP_EXPIRE_NEVER (-1)
#define POP_EXPIRE_IMMEDIATELT (0)


struct PopMessageInfo
{
	wxInt m_server_index;
	wxString m_uidl;
};
WX_DECLARE_LIST(PopMessageInfo, PopMessageInfoList);

struct PopFeatures
{
	void Reset();
	void SetCapaSupport(wxBool supported) {m_capa_supported = supported;}
	void SetSTLSSupport(wxBool supported) {m_stls_supported = supported;}
	void SetTopSupport(wxBool supported) {m_top_supported = supported;}
	void SetUserPassSupport(wxBool supported)	{m_auth_USER_PASS_supported = supported;}
	void SetSaslSupport(wxBool supported) {m_sasl_supported = supported;}
	void SetRespCodesSupport(wxBool supported) {m_resp_codes_supported = supported;}
	void SetLoginDelaySupport(wxBool supported, wxLong login_delay=POP_NO_LOGIN_DELAY) {m_login_delay_supported=supported; m_login_delay=login_delay;}
	void SetPipeliningSupport(wxBool supported) {m_pipelining_supported = supported;}
	void SetExpireSupport(wxBool supported, wxLong retention_days) {m_expire_supported=supported; m_retention_days=retention_days;}
	void SetUidlSupport(wxBool supported) {m_uidl_supported = supported;}
	void SetImplementationSupport(wxBool supported, const wxString& implementation) {m_implementation_supported=supported; m_implementation=implementation;}

	void SetAllAuthentications(wxBool enable=false);
	wxString GetAllSupportedAuthMethods() const;
	wxString GetMostSecureSupportedAuthMethod() const;
	wxmailto_status SetAuthMethodSupport(const wxString& auth_method, wxBool supported);
	wxBool GetAuthMethodSupport(const wxString& auth_method);
	void SetAPOPAuthMethodSupport(wxBool supported, const wxString& apop_timestamp=wxEmptyString);

	wxBool m_capa_supported; //Optional, RFC2449
	wxBool m_stls_supported; //Optional, RFC2595 §4
	wxBool m_top_supported; //Optional, RFC2449 §6.1
	wxBool m_sasl_supported; //Optional, RFC2449 §6.3
	wxBool m_resp_codes_supported; //Optional, RFC2449 §6.4
	wxBool m_login_delay_supported; //Optional, RFC2449 §6.5
	wxLong m_login_delay;
	wxBool m_pipelining_supported; //Optional, RFC2449 §6.6
	wxBool m_expire_supported; //Optional, RFC2449 §6.7
	wxLong m_retention_days;
	wxBool m_uidl_supported; //Optional, RFC1939, RFC2449 §6.8
	wxBool m_implementation_supported; //Optional, RFC2449 §6.9
	wxString m_implementation;

	wxBool m_auth_USER_PASS_supported; //Optional, RFC1939, RFC24494 §6.2
	wxBool m_auth_APOP_supported;
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

	wxString m_auth_APOP_timestamp;
};

class Pop : public ProtocolInterface
{
public:
	Pop(Account* account);
	virtual ~Pop();

public: //ProtocolInterface API
	wxmailto_status Sync();
	wxmailto_status CleanupAndAbort();
	wxmailto_status ReadAuthenticationNegotiationLine(SafeString& buffer);
	wxmailto_status WriteAuthenticationNegotiationLine(const SafeString& buffer);
	void WriteAuthenticationAbortedLine();

public:
	static wxBool IsLinefeed(const char& c) {return ('\r'==c || '\n'==c);}

private:
	wxmailto_status ReadInitialGreeting();
	wxmailto_status HandleCAPA();
	wxmailto_status HandleSTLS();
	wxmailto_status HandleAUTHENTICATE();
	wxmailto_status HandleUIDL(PopMessageInfoList& messages_to_fetch);
	wxmailto_status HandleLIST(PopMessageInfoList& messages_to_fetch);
	wxmailto_status HandleRETR(const PopMessageInfo* message_to_fetch);
	wxmailto_status HandleQUIT();
	wxmailto_status FetchMessages(PopMessageInfoList& messages_to_fetch);

private:
	/*
	 * return: ID_OK (Authenticated successfully)
	 *         ID_AUTHENTICATION_FAILED (Authentication failed)
	 *         ID_AUTHENTICATION_NOT_SUPPORTED (Method was not supported by server. Try another one, or fail)
	*/
	wxmailto_status HandleAUTHENTICATE(const wxString& authenticate_method);

private:
	PopFeatures m_supported_features;
};

}

#endif // _POP_H_
