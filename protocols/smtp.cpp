
// Copyright (C) 2008-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "smtp.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/tokenzr.h>
#endif

#include "smtp.h"
#include "../glue/idnaglue.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"


using namespace wxMailto;


void SmtpEhloFeatures::Reset()
{
	m_ehlo_supported = true; //Initially set to true, until proved unsupported
	m_ehlo_SEND_supported = false;
	m_ehlo_SOML_supported = false;
	m_ehlo_SAML_supported = false;
	m_ehlo_EXPN_supported = false;
	m_ehlo_HELP_supported = false;
	m_ehlo_TURN_supported = false;
	m_ehlo_8BITMIME_supported = false;
	SetAllAuthentications(false);
}

void SmtpEhloFeatures::DisableEhloSupport()
{
	m_ehlo_supported = false;
	SetAllAuthentications(true); //Without EHLO, we have to autodetect supported authentication methods
}

void SmtpEhloFeatures::SetAllAuthentications(wxBool enable)
{
	m_auth_ANONYMOUS_supported =
	m_auth_EXTERNAL_supported =
	m_auth_PLAIN_supported =
	m_auth_LOGIN_supported =
	m_auth_SECURID_supported =
	m_auth_NTLM_supported =
	m_auth_CRAMMD5_supported =
	m_auth_DIGESTMD5_supported =
	m_auth_GSSAPI_supported =
	m_auth_KERBEROS5_supported =
	m_auth_SCRAMSHA1_supported = enable;
}

wxString SmtpEhloFeatures::GetSupportedAuthMethods() const
{
	wxString methods;
	wxString separator = " ";

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

wxmailto_status SmtpEhloFeatures::DisableAuthMethod(const wxString& auth_method)
{
	if (auth_method.IsSameAs(AUTH_ANONYMOUS, false))
	{
		m_auth_ANONYMOUS_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_EXTERNAL, false))
	{
		m_auth_EXTERNAL_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_PLAIN, false))
	{
		m_auth_PLAIN_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_LOGIN, false))
	{
		m_auth_LOGIN_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_SECURID, false))
	{
		m_auth_SECURID_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_NTLM, false))
	{
		m_auth_NTLM_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_CRAM_MD5, false))
	{
		m_auth_CRAMMD5_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_DIGEST_MD5, false))
	{
		m_auth_DIGESTMD5_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_GSSAPI, false))
	{
		m_auth_GSSAPI_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_KERBEROS5, false))
	{
		m_auth_KERBEROS5_supported = false;
	}
	else if (auth_method.IsSameAs(AUTH_SCRAM_SHA1, false))
	{
		m_auth_SCRAMSHA1_supported = false;
	}
	else
	{
		wxASSERT_MSG(false, _("Unexpected authentication method \""+auth_method+"\""));
		return ID_OK; //Don't fail because of this
	}
	return ID_OK;
}


Smtp::Smtp(Account* account)
: ProtocolInterface(account)
{
	m_supported_ehlo_features.Reset();
}

Smtp::~Smtp()
{
}

wxmailto_status Smtp::Sync()
{
	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	wxmailto_status status;
	OutgoingMessageList message_list;
	if (ID_OK!=(status=wxGetApp().GetAppModuleManager()->GetMessageStore()->GetOutgoingMessages(message_list)))
		return status;

	if (message_list.IsEmpty())
		return ID_OK;

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/
	
	if (ID_OK!=(status=InitializeAndConnectSocket()) ||
	    ID_OK!=(status=ReadInitialGreeting()))
	{
		return status;
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	wxString read_string, write_string;

	wxString fqdn_idna_domain;
	wxASSERT(true==IdnaGlue::ToASCII(GetAccount()->GetFQDN(), fqdn_idna_domain));

	if (m_supported_ehlo_features.m_ehlo_supported)
	{
		if (ID_OK!=HandleEHLO(fqdn_idna_domain))
		{
			m_supported_ehlo_features.DisableEhloSupport();
			return Sync(); //Call self, but without EHLO support
		}
	} else {
		if (ID_OK!=(status=HandleHELO(fqdn_idna_domain)))
		{
			HandleQUIT();
			return status;
		}
	}

	/*************************/
	if (TestDestroy())
		return CleanupAndAbort();
	/*************************/

	OutgoingMessageList::iterator iter;
	for (iter=message_list.begin(); iter!=message_list.end(); ++iter)
	{
		/*************************/
		if (TestDestroy())
			return CleanupAndAbort();
		/*************************/

		OutgoingMessage* message = *iter;
		if (!message)
			continue;

		wxASSERT(message->HasTag(wxGetApp().GetAppModuleManager()->GetMessageStore()->GetOutboxTag()));
		wxASSERT(NULL!=message->GetFrom());

		ContactList recipients;
		if (ID_OK!=(status=message->GetAllRecipients(recipients)) ||
		    (iter!=message_list.begin() && ID_OK!=(status=HandleRSET())) || //RSET for all but the first message
		    ID_OK!=(status=HandleMAIL(message->GetFrom())))
		{
			HandleQUIT();
			return status;
		}

		if (recipients.IsEmpty())
			continue;

		ContactList::iterator iter;
		for (iter=recipients.begin(); iter!=recipients.end(); ++iter)
		{
			if (*iter && ID_OK!=(status=HandleRCPT(*iter)))
			{
				HandleQUIT();
				return status;
			}
		}

		/*************************/
		if (TestDestroy())
			return CleanupAndAbort();
		/*************************/

		if (ID_OK!=(status=HandleDATA(message)))
		{
			HandleQUIT();
			return status;
		}

		message->RemoveTag(wxGetApp().GetAppModuleManager()->GetMessageStore()->GetOutboxTag());
		message->AddTag(wxGetApp().GetAppModuleManager()->GetMessageStore()->GetSentTag());
	}

	HandleQUIT();

	return ID_OK;
}

wxmailto_status Smtp::CleanupAndAbort()
{
	HandleQUIT();
	return LOGERROR(ID_EXIT_REQUESTED);
}

wxmailto_status Smtp::ReadAuthenticationNegotiationLine(SafeString& WXUNUSED(buffer))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::WriteAuthenticationNegotiationLine(const SafeString& WXUNUSED(buffer))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

void Smtp::WriteAuthenticationAbortedLine()
{
	wxASSERT_MSG(false, "NOT_IMPLEMENTED");
}

wxInt Smtp::SmtpStatus(const wxString& smtp_string, wxBool& more) const
{
	wxInt smtp_status = 0;
	wxChar c;
	wxSizeT i;
	for (i=0; i<smtp_string.length(); i++)
	{
		c=smtp_string[i];
		if ('0'>c || '9'<c)
		{
			wxASSERT_MSG(' '==c || '-'==c, _("Unexpected SMTP string: ")+smtp_string);
			more = ('-'==c);
			break;
		}

		smtp_status = smtp_status*10 + (c-'0');
	}
	return smtp_status;
}

wxmailto_status Smtp::ReadInitialGreeting()
{
	wxmailto_status status;
	wxString read_string;
	wxInt smtp_status;
	wxBool more=true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;
	
		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC1651 §4.2
	{
	case 220: break;

	case 421: //Failure, fallthrough
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	wxStringTokenizer tokenizer(read_string, s_SP);
	if (2 > tokenizer.CountTokens())
	{
		m_server_fqdn.Empty();
		return ID_OK;
	}
	tokenizer.GetNextToken(); //Skip statuscode
	m_server_fqdn = tokenizer.GetNextToken();
	return ID_OK;
}

wxmailto_status Smtp::HandleHELO(const wxString& fqdn_idna_domain)
{
	wxmailto_status status;
	wxString read_string, write_string;
	//Send HELO, RFC821 §4.1.2
	write_string = "HELO "+fqdn_idna_domain+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 250: break;

	case 500: //Error, fallthrough
	case 501:
	case 504:
	case 421:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleEHLO(const wxString& fqdn_idna_domain)
{
	wxmailto_status status;
	wxString read_string, write_string;
	//Send EHLO, RFC1651 §4.2
	write_string = "EHLO "+fqdn_idna_domain+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	m_supported_ehlo_features.Reset();

	wxBool more=true;
	while(more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
		{
			m_supported_ehlo_features.DisableEhloSupport();
			return status;
		}

		switch(SmtpStatus(read_string, more)) //RFC1651 §4.2
		{
		case 250: if (!more)
		          	m_supported_ehlo_features.m_ehlo_supported = true;

		          ParseEhloKeyword(read_string.Mid(4));
		          break;
	
		case 550: //Failure, fallthrough
		case 500: //Error, fallthrough
		case 501:
		case 502:
		case 504:
		case 421:
		default: if (!more)
		         {
		         	m_supported_ehlo_features.DisableEhloSupport();
		         	return LOGERROR(ID_ERROR_FROM_SERVER);
		         }
		         break;
		}
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleMAIL(const Contact* contact)
{
	wxASSERT(NULL!=contact);
	wxASSERT(Contact::CONTACT_PERSON==contact->GetContactType());

	wxmailto_status status;
	wxString read_string, write_string;
	//Send MAIL FROM, RFC821 §4.1.2
	write_string = "MAIL FROM:<"+contact->GetEmail()+">\r\n";
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 250: break;

	case 552: //Failure, fallthrough
	case 451:
	case 452:
	case 500: //Error, fallthrough
	case 501:
	case 421:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleRCPT(const Contact* contact)
{
	wxASSERT(NULL!=contact);
	wxASSERT(Contact::CONTACT_PERSON==contact->GetContactType());

	wxmailto_status status;
	wxString read_string, write_string;
	//Send RCPT TO, RFC821 §4.1.2
	write_string = "RCPT TO:<"+contact->GetEmail()+">\r\n";
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 250: //Success, fallthrough
	case 251: break;

	case 550: //Failure, fallthrough
	case 551:
	case 552:
	case 553:
	case 450:
	case 451:
	case 452:
	case 500: //Error, fallthrough
	case 501:
	case 503:
	case 421:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleDATA(OutgoingMessage* message)
{
	wxASSERT(NULL!=message);

	wxmailto_status status;
	wxString read_string, write_string;
	//Send DATA, RFC821 §4.1.2
	write_string = "DATA\r\n";
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 354: break;

	case 451: //Failure, fallthrough
	case 554:
	case 500: //Error, fallthrough
	case 501:
	case 503:
	case 421:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	//Send actual message, dot-stuffed. RFC821 §4.1.2
	wxInputStream* message_dot_stuffed_stream;
	if (ID_OK!=(status=message->GetRFC2822Message(false, message_dot_stuffed_stream, true, m_supported_ehlo_features.m_ehlo_8BITMIME_supported)))
		return status;

	if (NULL==message_dot_stuffed_stream)
	{
		return LOGERROR(ID_NULL_POINTER);
	}

	wxUint32 message_bytes_to_write, message_bytes_written;
	const wxSizeT stream_buffer_size = 10*1024;
	char* stream_buffer = new char[stream_buffer_size];
	if (!stream_buffer)
	{
		delete message_dot_stuffed_stream;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	wxStreamError stream_error;
	while (!message_dot_stuffed_stream->Eof())
	{
		message_dot_stuffed_stream->Read(stream_buffer, stream_buffer_size);
		stream_error = message_dot_stuffed_stream->GetLastError();
		if (wxSTREAM_NO_ERROR!=stream_error &&
		    wxSTREAM_EOF!=stream_error)
		{
			delete message_dot_stuffed_stream;
			delete[] stream_buffer;
			return LOGERROR(ID_NETWORK_ERROR);
		}

		message_bytes_to_write = message_dot_stuffed_stream->LastRead();
		while (0 < message_bytes_to_write)
		{
			if (ID_OK!=(status=Write(stream_buffer, message_bytes_to_write, message_bytes_written)))
			{
				delete message_dot_stuffed_stream;
				delete[] stream_buffer;
				return status;
			}
			wxASSERT(0 < message_bytes_written);
			message_bytes_to_write -= message_bytes_written;
		}
	}

	//Read server-response
	more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 250: break;

	case 552: //Failure, fallthrough
	case 554:
	case 451:
	case 452:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleRSET()
{
	wxmailto_status status;
	wxString read_string, write_string;
	//Send RSET, RFC821 §4.1.2
	write_string = "RSET\r\n";
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	switch(smtp_status) //RFC821 §4.3
	{
	case 250: break;

	case 500: //Error, fallthrough
	case 501:
	case 504:
	case 421:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleVRFY(Contact& address)
{
	wxString idna_email = address.GetIdnaEmail();
	if (idna_email.IsEmpty())
		return LOGERROR(ID_INVALID_FORMAT);

	wxmailto_status status;
	wxString read_string, write_string;
	//Send VRFY, RFC821 §4.1.2
	write_string = "VRFY "+idna_email+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	wxInt smtp_status;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		smtp_status = SmtpStatus(read_string, more);
	}

	Contact* found_email = NULL;
	switch(smtp_status) //RFC821 §4.3
	{
	case 251: if (ID_OK!=(status=ParseVRFY_EXPAND(read_string, found_email)))
	          	return status;
	          //else Success, fallthrough
	case 250: break;

	case 550: //Failure, fallthrough
	case 551:
	case 553: if (ID_OK!=(status=ParseVRFY_EXPAND(read_string, found_email)))
	          	return status;

	          break; //If we find an address, consider this a success

	case 500: //Error, fallthrough
	case 501:
	case 502:
	case 504:
	case 421:
	default: return LOGERROR(ID_ERROR_FROM_SERVER);
	}

	if (found_email)
	{
		address.SetEmail(found_email->GetEmail());
		delete found_email;
	}
	return ID_OK;
}

wxmailto_status Smtp::HandleEXPAND(const Contact& address, ContactList& addresses)
{
	wxString idna_email = address.GetIdnaEmail();
	if (idna_email.IsEmpty())
		return LOGERROR(ID_INVALID_FORMAT);

	wxmailto_status status;
	wxString read_string, write_string;
	//Send EXPN, RFC821 §4.1.2
	write_string = "EXPN "+idna_email+s_CRLF;
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
		return status;

	//Read server-response
	Contact* found_email = NULL;
	wxBool more = true;
	while (more)
	{
		if (ID_OK!=(status=ReadLine(read_string, s_raw_encoding)))
			return status;

		switch(SmtpStatus(read_string, more)) //RFC821 §4.3
		{
		case 250: if (ID_OK!=(status=ParseVRFY_EXPAND(read_string, found_email)))
		          	return status;

		          if (found_email)
		          	addresses.Append(found_email);

							break;
	
		case 550: //Failure, fallthrough
		case 500: //Error, fallthrough
		case 501:
		case 502:
		case 504:
		case 421:
		default: return LOGERROR(ID_ERROR_FROM_SERVER);
		}
	}

	return ID_OK;
}

wxmailto_status Smtp::HandleQUIT()
{
	if (!IsConnected())
		return ID_OK; //No need to send QUIT, connection is already down

	wxmailto_status status;
	wxString read_string, write_string;
	//Send QUIT, RFC821 §4.1.2
	write_string = "QUIT\r\n";
	if (ID_OK!=(status=Write(write_string, s_raw_encoding)))
	{
		DestroySocket();
		return status;
	}

	//Read server-response
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

	DestroySocket();
	return ID_OK;
}

wxmailto_status Smtp::HandleSTARTTLS()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE()
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

		wxString supported_methods;
		while (!(supported_methods=m_supported_ehlo_features.GetSupportedAuthMethods()).IsEmpty())
		{
			if (ID_OK!=(status=wxGetApp().GetAppModuleManager()->GetAuthenticateGlue()->SuggestMethod(supported_methods, authentication_method)) ||
			 ID_AUTHENTICATION_NOT_SUPPORTED!=HandleAUTHENTICATE(authentication_method) ||
			 ID_OK!=(status=m_supported_ehlo_features.DisableAuthMethod(authentication_method)))
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

wxmailto_status Smtp::HandleAUTHENTICATE(const wxString& authenticate_method)
{
  if (authenticate_method.IsSameAs(AUTH_ANONYMOUS, false))
	{
		return HandleAUTHENTICATE_ANONYMOUS();
	}
  else if (authenticate_method.IsSameAs(AUTH_EXTERNAL, false))
	{
		return HandleAUTHENTICATE_EXTERNAL();
	}
  else if (authenticate_method.IsSameAs(AUTH_PLAIN, false))
	{
		return HandleAUTHENTICATE_PLAIN();
	}
  else if (authenticate_method.IsSameAs(AUTH_LOGIN, false))
	{
		return HandleAUTHENTICATE_LOGIN();
	}
  else if (authenticate_method.IsSameAs(AUTH_SECURID, false))
	{
		return HandleAUTHENTICATE_SECURID();
	}
  else if (authenticate_method.IsSameAs(AUTH_NTLM, false))
	{
		return HandleAUTHENTICATE_NTLM();
	}
  else if (authenticate_method.IsSameAs(AUTH_CRAM_MD5, false))
	{
		return HandleAUTHENTICATE_CRAM_MD5();
	}
  else if (authenticate_method.IsSameAs(AUTH_DIGEST_MD5, false))
	{
		return HandleAUTHENTICATE_DIGEST_MD5();
	}
  else if (authenticate_method.IsSameAs(AUTH_GSSAPI, false))
	{
		return HandleAUTHENTICATE_GSSAPI();
	}
  else if (authenticate_method.IsSameAs(AUTH_KERBEROS5, false))
	{
		return HandleAUTHENTICATE_KERBEROS5();
	}
  else if (authenticate_method.IsSameAs(AUTH_SCRAM_SHA1, false))
	{
		return HandleAUTHENTICATE_SCRAM_SHA1();
	}

	wxASSERT_MSG(false, _("Unexpected authentication method \""+authenticate_method+"\""));
	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

wxmailto_status Smtp::HandleAUTHENTICATE_ANONYMOUS()
{
	if (!m_supported_ehlo_features.m_auth_ANONYMOUS_supported ||
	    !wxGetApp().GetAppModuleManager()->GetAuthenticateGlue()->SupportsMethod(AUTH_ANONYMOUS))
	{
		return LOGERROR(ID_AUTHENTICATION_NOT_SUPPORTED);
	}

	return wxGetApp().GetAppModuleManager()->GetAuthenticateGlue()->Authenticate(AUTH_ANONYMOUS, &GenericAuthenticateCallback, this);
}

wxmailto_status Smtp::HandleAUTHENTICATE_EXTERNAL()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_PLAIN()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_LOGIN()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_SECURID()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_NTLM()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_CRAM_MD5()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_DIGEST_MD5()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_GSSAPI()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_KERBEROS5()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::HandleAUTHENTICATE_SCRAM_SHA1()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Smtp::ParseEhloKeyword(const wxString& ehlo_keyword_string)
{
	wxStringTokenizer tokenizer(ehlo_keyword_string, s_SP);
	wxASSERT(0 < tokenizer.CountTokens());
	if (0 == tokenizer.CountTokens())
		return LOGERROR(ID_INVALID_FORMAT);

	wxString ehlo_keyword = tokenizer.GetNextToken();

	if (ehlo_keyword.IsSameAs("SEND", false))
		m_supported_ehlo_features.m_ehlo_SEND_supported = true;
	else if (ehlo_keyword.IsSameAs("SOML", false))
		m_supported_ehlo_features.m_ehlo_SOML_supported = true;
	else if (ehlo_keyword.IsSameAs("SAML", false))
		m_supported_ehlo_features.m_ehlo_SAML_supported = true;
	else if (ehlo_keyword.IsSameAs("EXPN", false))
		m_supported_ehlo_features.m_ehlo_EXPN_supported = true;
	else if (ehlo_keyword.IsSameAs("HELP", false))
		m_supported_ehlo_features.m_ehlo_HELP_supported = true;
	else if (ehlo_keyword.IsSameAs("TURN", false))
		m_supported_ehlo_features.m_ehlo_TURN_supported = true;
	else if (ehlo_keyword.IsSameAs("8BITMIME", false))
		m_supported_ehlo_features.m_ehlo_8BITMIME_supported = true;
//ToDo, verify auth method keywords
	else if (ehlo_keyword.IsSameAs(AUTH_ANONYMOUS, false))
		m_supported_ehlo_features.m_auth_ANONYMOUS_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_EXTERNAL, false))
		m_supported_ehlo_features.m_auth_EXTERNAL_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_PLAIN, false))
		m_supported_ehlo_features.m_auth_PLAIN_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_LOGIN, false))
		m_supported_ehlo_features.m_auth_LOGIN_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_SECURID, false))
		m_supported_ehlo_features.m_auth_SECURID_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_NTLM, false))
		m_supported_ehlo_features.m_auth_NTLM_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_CRAM_MD5, false))
		m_supported_ehlo_features.m_auth_CRAMMD5_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_DIGEST_MD5, false))
		m_supported_ehlo_features.m_auth_DIGESTMD5_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_GSSAPI, false))
		m_supported_ehlo_features.m_auth_GSSAPI_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_KERBEROS5, false))
		m_supported_ehlo_features.m_auth_KERBEROS5_supported = true;
	else if (ehlo_keyword.IsSameAs(AUTH_SCRAM_SHA1, false))
		m_supported_ehlo_features.m_auth_SCRAMSHA1_supported = true;

	return ID_OK;
}

wxmailto_status Smtp::ParseVRFY_EXPAND(const wxString& vrfy_string, Contact*& result) const
{
	result = NULL;

	//Guess email adress, by looking for <..>
	wxStringTokenizer tokenizer(vrfy_string, s_SP);
	wxString token;
	while (tokenizer.HasMoreTokens())
	{
		token = tokenizer.GetNextToken();
		if (token.StartsWith("<") &&
		    token.EndsWith(">") )
		{
			result = new Contact();
			if (!result)
				return LOGERROR(ID_OUT_OF_MEMORY);

			result->SetEmail(token.Mid(1, token.length()-2));
			break;
		}
	}
	return ID_OK;
}
