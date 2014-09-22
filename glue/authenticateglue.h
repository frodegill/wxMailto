#ifndef _AUTHENTICATE_GLUE_H_
#define _AUTHENTICATE_GLUE_H_

// Copyright (C) 2009-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "authenticateglue.h"
#endif

#include <gsasl.h>
#include <wx/hashmap.h>
#include "../gui/wxmailto_module.h"
#include "../string/safestring.h"


namespace wxMailto
{

#define AUTH_ANONYMOUS  "ANONYMOUS"
#define AUTH_EXTERNAL   "EXTERNAL"
#define AUTH_USER_PASS  "USER"
#define AUTH_PLAIN      "PLAIN"
#define AUTH_LOGIN      "LOGIN"
#define AUTH_APOP       "APOP"
#define AUTH_SECURID    "SECURID"
#define AUTH_NTLM       "NTLM"
#define AUTH_CRAM_MD5   "CRAM-MD5"
#define AUTH_DIGEST_MD5 "DIGEST-MD5"
#define AUTH_GSSAPI     "GSSAPI"
#define AUTH_KERBEROS5  "KERBEROS5"
#define AUTH_SCRAM_SHA1 "SCRAM-SHA1"


#define AutenticateSession Gsasl_session
#define AuthenticateCallbackFunction Gsasl_callback_function
#define AuthenticateCallbackReturn int
#define AuthenticateCallbackParam Gsasl* WXUNUSED(ctx), Gsasl_session* sctx, Gsasl_property prop


class AuthenticateClientAPI
{
public:
	AuthenticateClientAPI() {}
	virtual ~AuthenticateClientAPI() {}

public:
	virtual wxmailto_status ReadAuthenticationNegotiationLine(SafeString& buffer) = 0;
	virtual wxmailto_status WriteAuthenticationNegotiationLine(const SafeString& buffer) = 0;
	virtual void WriteAuthenticationAbortedLine() = 0;
};


WX_DECLARE_VOIDPTR_HASH_MAP(AuthenticateClientAPI*, SessionClientAPIMap);


class AuthenticateGlue : public wxMailto_Module
{
public:
	AuthenticateGlue();
	virtual ~AuthenticateGlue();

	wxString GetName() const {return "Authenticate";}
	ModuleType GetType() const {return wxMailto_Module::AUTHENTICATE;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

	AuthenticateClientAPI* FindClientAPI(AutenticateSession* session);

	wxmailto_status SuggestMethod(const wxString& supported_methods, wxString& suggested_method) const;
	wxBool SupportsMethod(const wxString& method) const;
	wxmailto_status Authenticate(const wxString& method, AuthenticateCallbackFunction callback, AuthenticateClientAPI* client);

public: //Utils
	static wxmailto_status MD5(const SafeString& source, wxString& destination);
	static wxmailto_status MD5(const wxUint8* source_bytes, wxSizeT source_length, wxString& destination);

private:
	wxmailto_status AuthenticateUSER(AuthenticateClientAPI* client);
	wxmailto_status AuthenticateAPOP(AuthenticateClientAPI* client);

private:
	Gsasl* m_context;

	wxCriticalSection m_clientAPI_lock;
	SessionClientAPIMap m_sasl_clientAPI_map;
};

}

#endif // _AUTHENTICATE_GLUE_H_
