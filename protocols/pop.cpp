
// Copyright (C) 2008-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "pop.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/tokenzr.h>
#endif

#include "pop.h"
#include "../glue/mimeglue.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../mail/message.h"
#include "../storage/database_update.h"


using namespace wxMailto;

# include <wx/listimpl.cpp>
WX_DEFINE_LIST(PopMessageInfoList);


void PopFeatures::Reset()
{
	if (m_capa_supported)
	{
		m_top_supported = false;
		m_stls_supported = false;
		m_auth_USER_PASS_supported = false;
		m_sasl_supported = false;
		m_resp_codes_supported = false;
		m_login_delay_supported = false;
		m_pipelining_supported = false;
		m_expire_supported = false;
		m_uidl_supported = false;
		m_implementation_supported = false;

		wxBool old_apop_supported = m_auth_APOP_supported; //Keep this, it is already set in initial greeting
		wxString old_apop_timestamp = m_auth_APOP_timestamp;
		SetAllAuthentications(false);
		if (old_apop_supported)
			SetAPOPAuthMethodSupport(true, old_apop_timestamp);
	} else { //Have to autodetect
		m_top_supported = true;
		m_stls_supported = true;
		m_auth_USER_PASS_supported = true;
		m_sasl_supported = true;
		m_resp_codes_supported = false;
		m_login_delay_supported = false;
		m_pipelining_supported = false;
		m_expire_supported = false;
		m_uidl_supported = true;
		m_implementation_supported = true;

		SetAllAuthentications(true); //Have to autodetect
	}
}

void PopFeatures::SetAllAuthentications(wxBool enable)
{
	m_auth_SCRAMSHA1_supported =
	m_auth_KERBEROS5_supported =
	m_auth_GSSAPI_supported =
	m_auth_SECURID_supported =
	m_auth_APOP_supported =
	m_auth_DIGESTMD5_supported =
	m_auth_CRAMMD5_supported =
	m_auth_EXTERNAL_supported =	
	m_auth_PLAIN_supported =
	m_auth_LOGIN_supported =
	m_auth_NTLM_supported =
	m_auth_ANONYMOUS_supported = enable;
	
	m_auth_APOP_timestamp.Clear();
}

wxString PopFeatures::GetAllSupportedAuthMethods() const
{
	wxString methods;
	wxString separator = " ";

	if (m_auth_USER_PASS_supported)
		methods += AUTH_USER_PASS+separator;

	if (m_auth_APOP_supported)
		methods += AUTH_APOP+separator;

	if (m_auth_ANONYMOUS_supported)
		methods += AUTH_ANONYMOUS+separator;

  if (m_auth_EXTERNAL_supported)
		methods += AUTH_EXTERNAL+separator;

  if (m_auth_PLAIN_supported)
		methods += AUTH_PLAIN+separator;

  if (m_auth_LOGIN_supported)
		methods += AUTH_LOGIN+separator;

  if (m_auth_SECURID_supported)
		methods += AUTH_SECURID+separator;

  if (m_auth_NTLM_supported)
		methods += AUTH_NTLM+separator;

  if (m_auth_CRAMMD5_supported)
		methods += AUTH_CRAM_MD5+separator;

  if (m_auth_DIGESTMD5_supported)
		methods += AUTH_DIGEST_MD5+separator;

  if (m_auth_GSSAPI_supported)
		methods += AUTH_GSSAPI+separator;

  if (m_auth_KERBEROS5_supported)
		methods += AUTH_KERBEROS5+separator;

  if (m_auth_SCRAMSHA1_supported)
		methods += AUTH_SCRAM_SHA1+separator;

	return methods.Trim();
}

wxString PopFeatures::GetMostSecureSupportedAuthMethod() const
{
	AuthenticateGlue* auth_glue = wxGetApp().GetAppModuleManager()->GetAuthenticateGlue();

	if (m_auth_SCRAMSHA1_supported && auth_glue->SupportsMethod(AUTH_SCRAM_SHA1))
		return AUTH_SCRAM_SHA1;

  if (m_auth_KERBEROS5_supported && auth_glue->SupportsMethod(AUTH_KERBEROS5))
		return AUTH_KERBEROS5;

  if (m_auth_GSSAPI_supported && auth_glue->SupportsMethod(AUTH_GSSAPI))
		return AUTH_GSSAPI;

  if (m_auth_SECURID_supported && auth_glue->SupportsMethod(AUTH_SECURID))
		return AUTH_SECURID;

	if (m_auth_APOP_supported)
		return AUTH_APOP;

  if (m_auth_DIGESTMD5_supported && auth_glue->SupportsMethod(AUTH_DIGEST_MD5))
		return AUTH_DIGEST_MD5;

  if (m_auth_CRAMMD5_supported && auth_glue->SupportsMethod(AUTH_CRAM_MD5))
		return AUTH_CRAM_MD5;

  if (m_auth_EXTERNAL_supported && auth_glue->SupportsMethod(AUTH_EXTERNAL))
		return AUTH_EXTERNAL;

  if (m_auth_PLAIN_supported && auth_glue->SupportsMethod(AUTH_PLAIN))
		return AUTH_PLAIN;

  if (m_auth_LOGIN_supported && auth_glue->SupportsMethod(AUTH_LOGIN))
		return AUTH_LOGIN;

  if (m_auth_NTLM_supported && auth_glue->SupportsMethod(AUTH_NTLM))
		return AUTH_NTLM;

	if (m_auth_USER_PASS_supported)
		return AUTH_USER_PASS;

	if (m_auth_ANONYMOUS_supported && auth_glue->SupportsMethod(AUTH_ANONYMOUS))
		return AUTH_ANONYMOUS;
	
	return wxEmptyString;
}

wxmailto_status PopFeatures::SetAuthMethodSupport(const wxString& auth_method, wxBool supported)
{
	if (auth_method.IsSameAs(AUTH_APOP, false))
	{
		m_auth_APOP_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_ANONYMOUS, false))
	{
		m_auth_ANONYMOUS_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_EXTERNAL, false))
	{
		m_auth_EXTERNAL_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_PLAIN, false))
	{
		m_auth_PLAIN_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_LOGIN, false))
	{
		m_auth_LOGIN_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_SECURID, false))
	{
		m_auth_SECURID_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_NTLM, false))
	{
		m_auth_NTLM_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_CRAM_MD5, false))
	{
		m_auth_CRAMMD5_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_DIGEST_MD5, false))
	{
		m_auth_DIGESTMD5_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_GSSAPI, false))
	{
		m_auth_GSSAPI_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_KERBEROS5, false))
	{
		m_auth_KERBEROS5_supported = supported;
	}
	else if (auth_method.IsSameAs(AUTH_SCRAM_SHA1, false))
	{
		m_auth_SCRAMSHA1_supported = supported;
	}
	else
	{
		wxLogDebug(_("Unexpected authentication method \""+auth_method+"\""));
		return ID_OK; //Don't fail because of this
	}
	return ID_OK;
}

wxBool PopFeatures::GetAuthMethodSupport(const wxString& auth_method)
{
	if (auth_method.IsSameAs(AUTH_APOP, false)) return m_auth_APOP_supported;
	else if (auth_method.IsSameAs(AUTH_ANONYMOUS, false)) return m_auth_ANONYMOUS_supported;
	else if (auth_method.IsSameAs(AUTH_EXTERNAL, false)) return m_auth_EXTERNAL_supported;
	else if (auth_method.IsSameAs(AUTH_PLAIN, false)) return m_auth_PLAIN_supported;
	else if (auth_method.IsSameAs(AUTH_LOGIN, false)) return m_auth_LOGIN_supported;
	else if (auth_method.IsSameAs(AUTH_SECURID, false)) return m_auth_SECURID_supported;
	else if (auth_method.IsSameAs(AUTH_NTLM, false)) return m_auth_NTLM_supported;
	else if (auth_method.IsSameAs(AUTH_CRAM_MD5, false)) return m_auth_CRAMMD5_supported;
	else if (auth_method.IsSameAs(AUTH_DIGEST_MD5, false)) return m_auth_DIGESTMD5_supported;
	else if (auth_method.IsSameAs(AUTH_GSSAPI, false)) return m_auth_GSSAPI_supported;
	else if (auth_method.IsSameAs(AUTH_KERBEROS5, false)) return m_auth_KERBEROS5_supported;
	else if (auth_method.IsSameAs(AUTH_SCRAM_SHA1, false)) return m_auth_SCRAMSHA1_supported;
	else
	{
		wxLogDebug(_("Unexpected authentication method \""+auth_method+"\""));
	}
	return false;
}

void PopFeatures::SetAPOPAuthMethodSupport(wxBool supported, const wxString& apop_timestamp)
{
	m_auth_APOP_supported = supported;
	m_auth_APOP_timestamp = apop_timestamp;
}


Pop::Pop(Account* account)
: ProtocolInterface(account)
{
	m_supported_features.SetCapaSupport(true);
	m_supported_features.SetAPOPAuthMethodSupport(false, wxEmptyString);
	m_supported_features.Reset();
}

Pop::~Pop()
{
}

wxmailto_status Pop::Sync()
{
	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	wxmailto_status status;
	if (ID_OK!=(status=InitializeAndConnectSocket()) ||
	    ID_OK!=(status=ReadInitialGreeting()))
	{
		return status;
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	if (m_supported_features.m_capa_supported)
	{
		if (ID_OK!=HandleCAPA())
		{
			m_supported_features.SetCapaSupport(false);
		}
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	if (m_supported_features.m_stls_supported)
	{
		if (ID_OK!=HandleSTLS())
		{
			m_supported_features.SetSTLSSupport(false);
		}

		//"Once TLS has been started, the client MUST discard cached
		// information about server capabilities and SHOULD re-issue
		// the CAPA command.", RFC2595 §4
		if (m_supported_features.m_capa_supported)
		{
			if (ID_OK!=HandleCAPA())
			{
				m_supported_features.SetCapaSupport(false);
			}
		}
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	if (ID_OK!=HandleAUTHENTICATE())
	{
		return CleanupAndAbort();
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	PopMessageInfoList messages_to_fetch;
	messages_to_fetch.DeleteContents(true);
	if (!m_supported_features.m_uidl_supported || ID_OK!=HandleUIDL(messages_to_fetch))
	{
		m_supported_features.SetUidlSupport(false);
		if (ID_OK!=HandleLIST(messages_to_fetch))
		{
			CleanupAndAbort();
			return LOGERROR(ID_ERROR_FROM_SERVER);
		}
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	if (ID_OK!=(status=FetchMessages(messages_to_fetch)))
	{
		CleanupAndAbort();
		return status;
	}

	return ID_OK;
}

wxmailto_status Pop::CleanupAndAbort()
{
	HandleQUIT();
	return LOGERROR(ID_EXIT_REQUESTED);
}

wxmailto_status Pop::ReadAuthenticationNegotiationLine(SafeString& buffer)
{
	wxString method = GetDoAuthenticateMethod();
	wxInt index = GetAndIncrementDoAuthenticateIndex();
	if (method.IsSameAs(AUTH_USER_PASS))
	{
		switch(index)
		{
			case 0: return buffer.StrDup("test"); /*GetAccount()->*/ /*TODO*/
			case 2: return buffer.StrDup("test"); return ID_OK; /*TODO*/
			default: return LOGERROR(ID_SHOULDNT_GET_HERE);
		}
	}
	else if (method.IsSameAs(AUTH_APOP))
	{
		switch(index)
		{
			case 0: return buffer.StrDup("test"); /*GetAccount()->*/ /*TODO*/
			case 1: return buffer.StrDup("test"); return ID_OK; /*TODO*/
			case 2: return buffer.StrDup(m_supported_features.m_auth_APOP_timestamp.ToUTF8());
			default: return LOGERROR(ID_SHOULDNT_GET_HERE);
		}
	}
	else if (method.IsSameAs(AUTH_CRAM_MD5) ||
	         method.IsSameAs(AUTH_DIGEST_MD5))
	{
		wxmailto_status status;
		wxString read_string;
		if (ID_OK!=(status=Write("AUTH "+method+s_CRLF, s_raw_encoding)) ||
		    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			return status;
		}

		wxStringTokenizer tokenizer(read_string, s_SP);
		if (2 > tokenizer.CountTokens() ||
		    "+"!=tokenizer.GetNextToken())
		{
			return LOGERROR(ID_ERROR_FROM_SERVER);
		}

		SetIsInAuthenticateMode(true);
		buffer.StrDup(tokenizer.GetNextToken());
		return ID_OK;
	}
	else
	{
		wxLogDebug(_("Unexpected authentication method \""+method+"\""));
		return LOGERROR(ID_AUTHENTICATION_NOT_SUPPORTED);
	}
	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

wxmailto_status Pop::WriteAuthenticationNegotiationLine(const SafeString& buffer)
{
	wxmailto_status status;
	wxString read_string;
	wxString method = GetDoAuthenticateMethod();
	wxInt index = GetAndIncrementDoAuthenticateIndex();

	const char* buffer_str;
	if (ID_OK!=(status=buffer.GetStr(buffer_str)))
		return status;

	if (method.IsSameAs(AUTH_USER_PASS))
	{
		switch(index)
		{
			case 1:
			{
				if (ID_OK!=(status=Write("USER "+wxString::FromUTF8(buffer_str)+s_CRLF, s_raw_encoding)) ||
				    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
				{
					return status;
				}
				
				return (read_string.StartsWith("+OK")) ? ID_OK : LOGERROR(ID_ERROR_FROM_SERVER);
			}
			case 3:
			{
				if (ID_OK!=(status=Write("PASS "+wxString::FromUTF8(buffer_str)+s_CRLF, s_raw_encoding)) ||
				    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
				{
					return status;
				}
				
				return (read_string.StartsWith("+OK")) ? ID_OK : LOGERROR(ID_ERROR_FROM_SERVER);
			}
			default: return LOGERROR(ID_SHOULDNT_GET_HERE);
		}
	}
	else if (method.IsSameAs(AUTH_APOP))
	{
		if (ID_OK!=(status=Write("APOP "+wxString::FromUTF8(buffer_str)+s_CRLF, s_raw_encoding)) ||
		    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			return status;
		}
		
		return (read_string.StartsWith("+OK")) ? ID_OK : LOGERROR(ID_ERROR_FROM_SERVER);
	}
	else if (method.IsSameAs(AUTH_DIGEST_MD5) || //SASL methods, server write first
	         method.IsSameAs(AUTH_CRAM_MD5))
	{
		if (ID_OK!=(status=Write(wxString::FromUTF8(buffer_str)+s_CRLF, s_raw_encoding)) ||
		    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			return status;
		}
		
		status = (read_string.StartsWith("+OK")) ? ID_OK : LOGERROR(ID_ERROR_FROM_SERVER);
		SetIsInAuthenticateMode(ID_OK!=status); //If fail, we still are in authenticate mode
		return status;
	}
	else if (method.IsSameAs(AUTH_PLAIN) || //SASL methods, client write first
	         method.IsSameAs(AUTH_SCRAM_SHA1))
	{
		if (ID_OK!=(status=Write("AUTH "+method+" "+wxString::FromUTF8(buffer_str)+s_CRLF, s_raw_encoding)) ||
		    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			return status;
		}
		
		status = (read_string.StartsWith("+OK")) ? ID_OK : LOGERROR(ID_ERROR_FROM_SERVER);
		SetIsInAuthenticateMode(ID_OK==status); //If success, we are now in authenticate mode
		return status;
	}
	else
	{
		wxLogDebug(_("Unexpected authentication method \""+method+"\""));
		return LOGERROR(ID_AUTHENTICATION_NOT_SUPPORTED);
	}
	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

void Pop::WriteAuthenticationAbortedLine()
{
	if (!IsInAuthenticateMode())
		return;
	
	Write("*"+s_CRLF, s_raw_encoding); //RFC1734 §2

	wxString read_string;
	ReadLine(read_string, s_raw_encoding);

	SetIsInAuthenticateMode(false);
}

wxmailto_status Pop::ReadInitialGreeting()
{
	wxmailto_status status;
	wxString read_string;

	if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		return status;
	
	if (512<read_string.length()) //RFC1939 §3, "Responses may be up to 512 characters long, including the terminating CRLF"
		return LOGERROR(ID_ERROR_FROM_SERVER);

	wxStringTokenizer tokenizer(read_string, s_SP);
	if (1 > tokenizer.CountTokens())
	{
		return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	wxString status_indicator = tokenizer.GetNextToken(); //RFC1939 §3. Should be "+OK" or "-ERR", in uppercase
	if (!status_indicator.IsSameAs("+OK"))
	{
		while (m_supported_features.m_resp_codes_supported && tokenizer.HasMoreTokens())
		{
			wxString extended_response_code = tokenizer.GetNextToken();
			if (extended_response_code.IsSameAs("[IN-USE]"))
				return LOGERROR(ID_POP_IN_USE);
			else if (extended_response_code.IsSameAs("[LOGIN-DELAY]"))
				return LOGERROR(ID_POP_LOGIN_DELAY);
		}
		return LOGERROR(ID_ERROR_FROM_SERVER);
	}
	else
	{
		while (tokenizer.HasMoreTokens())
		{
			wxString apop_timestamp = tokenizer.GetNextToken();
			if (apop_timestamp.StartsWith("<") && apop_timestamp.EndsWith(">"))
			{
				m_supported_features.SetAPOPAuthMethodSupport(true, apop_timestamp);
			}
		}
	}

	return ID_OK;
}

wxmailto_status Pop::HandleCAPA()
{
	wxmailto_status status;
	wxString read_string, write_string;
	//Send CAPA, RFC2449 §5
	write_string = "CAPA"+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Reset capabilities
	m_supported_features.Reset();

	if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
	{
		m_supported_features.SetCapaSupport(false);
		return status;
	}
	
	if (!read_string.StartsWith("+OK"))
	{
		m_supported_features.SetCapaSupport(false);
		return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	while(true)
	{
		/*************************/
		if (TestDestroy())
			return CleanupAndAbort();
		/*************************/

		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			return status;
		}

		if (512 < read_string.length()) //RFC2449 §5, "Each capability line is limited to 512 octets (including the CRLF)"
			return LOGERROR(ID_ERROR_FROM_SERVER);
			
		wxStringTokenizer tokenizer(read_string, s_SP);
		if (!tokenizer.HasMoreTokens())
			return LOGERROR(ID_ERROR_FROM_SERVER);
		
		wxString capability = tokenizer.GetNextToken();
		if (!tokenizer.HasMoreTokens() && "."==capability) //"The capability list is terminated by a line containing a termination octet (".") and a CRLF pair"
			break;

		wxLong login_delay, retention_days;
		if ("STLS" == capability)
		{
			m_supported_features.SetSTLSSupport(true);
		}
		else if ("TOP" == capability)
		{
			m_supported_features.SetTopSupport(true);
		}
		else if ("USER" == capability)
		{
			m_supported_features.SetUserPassSupport(true);
		}
		else if ("SASL" == capability)
		{
			while (tokenizer.HasMoreTokens())
				m_supported_features.SetAuthMethodSupport(tokenizer.GetNextToken(), true);
		}
		else if ("RESP-CODES" == capability)
		{
			m_supported_features.SetRespCodesSupport(true);
		}
		else if ("LOGIN-DELAY" == capability && tokenizer.HasMoreTokens() && tokenizer.GetNextToken().ToLong(&login_delay))
		{
			m_supported_features.SetLoginDelaySupport(true, login_delay);
		}
		else if ("PIPELINING" == capability)
		{
			m_supported_features.SetPipeliningSupport(true);
		}
		else if ("EXPIRE" == capability && tokenizer.HasMoreTokens())
		{
			wxString expire = tokenizer.GetNextToken();

			if (expire.IsSameAs("USER", false) && tokenizer.HasMoreTokens()) //RFC2449 §6.7
				expire  = tokenizer.GetNextToken();

			if (expire.IsSameAs("NEVER", false) || !expire.ToLong(&retention_days)) //RFC2449 §6.7
				retention_days = POP_EXPIRE_NEVER;

			m_supported_features.SetExpireSupport(true, retention_days);
		}
		else if ("UIDL" == capability)
		{
			m_supported_features.SetUidlSupport(true);
		}
		else if ("IMPLEMENTATION" == capability && tokenizer.HasMoreTokens())
		{
			m_supported_features.SetImplementationSupport(true, tokenizer.GetString());
		}
	}

	return ID_OK;
}

wxmailto_status Pop::HandleSTLS()
{
	wxmailto_status status;
	wxString read_string, write_string;
	//Send STLS, RFC2595 §4
	write_string = "STLS"+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
	{
		m_supported_features.SetSTLSSupport(false);
		return status;
	}
	
	if (!read_string.StartsWith("+OK"))
	{
		m_supported_features.SetSTLSSupport(false);
		return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	return StartTLS();
}

wxmailto_status Pop::HandleAUTHENTICATE()
{
	wxmailto_status status;
	wxString authentication_method = GetAccount()->GetAuthenticationMethod();
	if (authentication_method.IsEmpty() || authentication_method.IsSameAs("NONE", false))
	{
		return ID_OK;
	}

	if (authentication_method.IsSameAs("AUTO", false))
	{
		authentication_method = GetAccount()->GetAutodetectedAuthenticationMethod();
		if (!authentication_method.IsEmpty() && ID_OK==HandleAUTHENTICATE(authentication_method))
		{
			return ID_OK;
		}

		//Iterate through all methods, most secure first
		while (wxEmptyString != (authentication_method=m_supported_features.GetMostSecureSupportedAuthMethod()))
		{
			//If OK or ERROR, return
			if (ID_AUTHENTICATION_NOT_SUPPORTED!=(status=HandleAUTHENTICATE(authentication_method)))
				return status;

			//If not supported, disable this method and try next
			if (ID_OK!=(status=m_supported_features.SetAuthMethodSupport(authentication_method, false)))
			{
				return status;
			}
		}
	}
	else {
		return HandleAUTHENTICATE(authentication_method);
	}
	return LOGERROR(ID_AUTHENTICATION_FAILED);
}

wxmailto_status Pop::HandleAUTHENTICATE(const wxString& authenticate_method)
{
	if (m_supported_features.GetAuthMethodSupport(authenticate_method) &&
	    wxGetApp().GetAppModuleManager()->GetAuthenticateGlue()->SupportsMethod(authenticate_method))
	{
		return DoAuthenticate(authenticate_method, &GenericAuthenticateCallback);
	}

	wxLogDebug(_("Unexpected authentication method \""+authenticate_method+"\""));
	return LOGERROR(ID_AUTHENTICATION_NOT_SUPPORTED);
}

wxmailto_status Pop::HandleUIDL(PopMessageInfoList& messages_to_fetch)
{
	messages_to_fetch.Clear();

	wxmailto_status status;
	wxString read_string, write_string;
	//Send UIDL, RFC1939 §7
	write_string = "UIDL"+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
	{
		m_supported_features.SetUidlSupport(false);
		return status;
	}
	
	if (!read_string.StartsWith("+OK"))
	{
		m_supported_features.SetUidlSupport(false);
		return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	wxUint16 account_id = GetAccount()->GetAccountId();
	wxUInt uidl_count = 0; 
	std::string unique_id;

	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
		return status;

	//Prepare statement used in loop
	Poco::Data::Statement uidl_count_statement =
		 (*session << "SELECT COUNT(*) FROM pop_uidl WHERE account_id = ? AND uidl = ?",
			Poco::Data::into(uidl_count), Poco::Data::use(account_id), Poco::Data::use(unique_id));

	while(true)
	{
		/*************************/
		if (TestDestroy()) {
			poco_glue->ReleaseSession(session);
			return CleanupAndAbort();
		}
		/*************************/

		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			poco_glue->ReleaseSession(session);
			return status;
		}

		wxStringTokenizer tokenizer(read_string, s_SP);
		if (!tokenizer.HasMoreTokens())
		{
			poco_glue->ReleaseSession(session);
			return LOGERROR(ID_ERROR_FROM_SERVER);
		}

		wxString message_number = tokenizer.GetNextToken();
		if (!tokenizer.HasMoreTokens() && "."==message_number) //"The uidl list is terminated by a line containing a termination octet (".") and a CRLF pair"
			break;

		unique_id = std::string(tokenizer.GetNextToken().Left(POP_UIDL_UIDL_LEN).ToUTF8());

		uidl_count_statement.execute(); //Already prepared statement

		if (0<uidl_count)
			continue; //Message is already fetched

		PopMessageInfo* message_info = new PopMessageInfo();
		if (!message_info)
		{
			poco_glue->ReleaseSession(session);
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		wxLong tmp_index;
		message_number.ToLong(&tmp_index);
		message_info->m_server_index = tmp_index;
		message_info->m_uidl = wxString::FromUTF8(unique_id.c_str());
		messages_to_fetch.Append(message_info);
	}

	poco_glue->ReleaseSession(session);
	return ID_OK;
}

wxmailto_status Pop::HandleLIST(PopMessageInfoList& WXUNUSED(messages_to_fetch))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Pop::HandleRETR(const PopMessageInfo* message_to_fetch)
{
	if (!message_to_fetch)
		return LOGERROR(ID_NULL_POINTER);

	wxmailto_status status;
	wxString read_string, write_string;
	//Send RETR, RFC1939 §5
	write_string = wxString::Format("RETR %d"+s_CRLF, message_to_fetch->m_server_index);
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)) ||
	    ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
	{
		return status;
	}
	
	if (!read_string.StartsWith("+OK"))
		return LOGERROR(ID_ERROR_FROM_SERVER);

	std::stringstream* input_stream = new std::stringstream;
	char* raw_buffer;
	wxUint32 raw_buffer_length;
	while(true)
	{
		/*************************/
		if (TestDestroy()) {
			delete input_stream;
			return CleanupAndAbort();
		}
		/*************************/

		if (ID_OK!=(status=ReadLine(raw_buffer, raw_buffer_length)))
		{
			delete input_stream;
			return status;
		}

		if (1==raw_buffer_length && '.'==*raw_buffer)
		{
			break;
		}
		else
		{
			//Dot-stuffed?
			if (2==raw_buffer_length && '.'==*raw_buffer && '.'==*(raw_buffer+1))
			{
				raw_buffer++;
				raw_buffer_length--;
			}

			input_stream->write(raw_buffer, raw_buffer_length);
			input_stream->write("\r\n", 2);
		}
	}

	Message* message = new Message();
	if (!message)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	status = message->SetRFC2822Message(input_stream);
	delete input_stream;
	if (ID_OK!=status)
	{
		delete message;
		return status;
	}

	//Persist message

	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		delete message;
		return status;
	}

	if (ID_OK!=(status=PocoGlue::StartTransaction(session)) ||
	    ID_OK!=(status=message->SaveToDB(session)))
	{
		poco_glue->ReleaseSession(session);
		delete message;
		return status;
	}

	wxUint16 account_id = GetAccount()->GetAccountId();
	std::string uidl = std::string(message_to_fetch->m_uidl.Left(POP_UIDL_UIDL_LEN).ToUTF8());
	*session << "INSERT INTO pop_uidl (account_id, uidl) VALUES (?, ?)",
		Poco::Data::use(account_id), Poco::Data::use(uidl),
		Poco::Data::now;

	status = PocoGlue::CommitTransaction(session);
	poco_glue->ReleaseSession(session);
	delete message;
	return ID_OK;
}

wxmailto_status Pop::HandleQUIT()
{
	if (!IsConnected())
		return ID_OK; //No need to send QUIT, connection is already down

	wxmailto_status status;
	wxString read_string, write_string;
	//Send QUIT, RFC1939 §6
	write_string = "QUIT\r\n";
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
	{
		DestroySocket();
		return status;
	}

	//Read server-response
	/*
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			DestroySocket();
			return status;
		}

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 221: break;

	case 500: //Error, fallthrough
	default: 	DestroySocket();
	          return LOGERROR(ID_ERROR_FROM_SERVER);
	}
*/
	DestroySocket();
	return ID_OK;
}

wxmailto_status Pop::FetchMessages(PopMessageInfoList& messages_to_fetch)
{
	wxmailto_status status=ID_OK;

	PopMessageInfo* message_to_fetch;
	wxmailto_status tmp_status;
	PopMessageInfoList::iterator iter;
	for (iter=messages_to_fetch.begin(); iter!=messages_to_fetch.end(); ++iter)
	{
		message_to_fetch = *iter;
		if (ID_OK==status && ID_OK!=(tmp_status=HandleRETR(message_to_fetch)))
		{
			status = tmp_status;
		}
	}
	messages_to_fetch.Clear();
	return status;
}
