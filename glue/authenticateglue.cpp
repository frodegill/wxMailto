
// Copyright (C) 2009-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "authenticateglue.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/string.h>
# include <wx/thread.h>
#endif

#include "authenticateglue.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../string/stringutils.h"

//Useful link: <URL: http://www.howtoforge.com/adding-an-odbc-driver-for-mysql-on-ubuntu >

using namespace wxMailto;

AuthenticateGlue::AuthenticateGlue()
: wxMailto_Module(),
  m_context(NULL)
{
}

AuthenticateGlue::~AuthenticateGlue()
{
	m_sasl_clientAPI_map.clear();

	if (m_context)
	{
		gsasl_done(m_context);
	}
}

wxmailto_status AuthenticateGlue::Initialize()
{
	int result;
	if ((result=gsasl_init(&m_context)) != GSASL_OK)
	{
		wxLogDebug(_("Cannot initialize libgsasl (%d): %s"), result, gsasl_strerror(result));
		return LOGERROR(ID_GENERIC_ERROR);
	}

	wxGetApp().GetAppModuleManager()->RegisterModule(this);
	return ID_OK;
}

wxmailto_status AuthenticateGlue::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}

AuthenticateClientAPI* AuthenticateGlue::FindClientAPI(AutenticateSession* session)
{
	wxCriticalSectionLocker locker(m_clientAPI_lock);
	SessionClientAPIMap::iterator iter = m_sasl_clientAPI_map.find(session);
	return (m_sasl_clientAPI_map.end()!=iter) ? iter->second : NULL;
}

wxmailto_status AuthenticateGlue::SuggestMethod(const wxString& supported_methods, wxString& suggested_method) const
{
	const char* gsasl_suggested_method;
	if (supported_methods.IsEmpty() ||
	    NULL==(gsasl_suggested_method=gsasl_client_suggest_mechanism(m_context, supported_methods.ToUTF8())))
	{
		suggested_method.Empty();
	}
	else
	{
		suggested_method = wxString::FromUTF8(gsasl_suggested_method);
	}
	return ID_OK;
}

wxBool AuthenticateGlue::SupportsMethod(const wxString& method) const
{
	if (method.IsSameAs(AUTH_USER_PASS, false) ||
	    method.IsSameAs(AUTH_APOP, false))
	{
		return true;
	}
	
	return 1==gsasl_client_support_p(m_context, method.ToUTF8());
}

wxmailto_status AuthenticateGlue::Authenticate(const wxString& method, AuthenticateCallbackFunction callback, AuthenticateClientAPI* client)
{
	wxASSERT(NULL!=m_context);
	wxASSERT(NULL!=callback);
	wxASSERT(NULL!=client);

	if (method.IsSameAs(AUTH_USER_PASS, false))
		return AuthenticateUSER(client);

	if (method.IsSameAs(AUTH_APOP, false))
		return AuthenticateAPOP(client);

	if (!m_context)
		return LOGERROR(ID_NULL_POINTER);

	gsasl_callback_set(m_context, callback);

	AutenticateSession* session;
	if (GSASL_OK!=gsasl_client_start(m_context, method.ToUTF8(), &session))
		return LOGERROR(ID_GENERIC_ERROR);

	{
		wxCriticalSectionLocker locker(m_clientAPI_lock);
		m_sasl_clientAPI_map[session] = client;
	}

	SafeString from_server;
	char* to_server_str;
	int rc;
	wxmailto_status status = ID_OK;
	do
	{
		const char* from_server_str;
		if (ID_OK!=(status=from_server.GetStr(from_server_str)))
			break;

		rc = gsasl_step64(session, from_server_str, &to_server_str);

		if (GSASL_OK==rc && to_server_str)
		{
			if (*to_server_str)
			{
				SafeString to_server;
				if (ID_OK!=(status=to_server.SetStr(to_server_str, NOOP)))
					break;

				status = client->WriteAuthenticationNegotiationLine(to_server);
			}
			gsasl_free(to_server_str);
		}
		else if (GSASL_NEEDS_MORE==rc)
		{
			status = client->ReadAuthenticationNegotiationLine(from_server);
		}
	} while (GSASL_NEEDS_MORE==rc && ID_OK==status);

	gsasl_finish(session);

	{
		wxCriticalSectionLocker locker(m_clientAPI_lock);
		m_sasl_clientAPI_map.erase(session);
	}

	if (ID_OK!=status)
		return status;
	
	switch(rc)
	{
		case GSASL_OK: return ID_OK;
		case GSASL_AUTHENTICATION_ERROR: client->WriteAuthenticationAbortedLine(); return LOGERROR(ID_AUTHENTICATION_FAILED);
		default: client->WriteAuthenticationAbortedLine(); return LOGERROR(ID_AUTHENTICATION_NOT_SUPPORTED);
	}
}

wxmailto_status AuthenticateGlue::AuthenticateUSER(AuthenticateClientAPI* client)
{
	wxmailto_status status;
	SafeString username;
	SafeString password;
	if (ID_OK!=(status=client->ReadAuthenticationNegotiationLine(username)) ||
		ID_OK!=(status=client->WriteAuthenticationNegotiationLine(username)) ||
		ID_OK!=(status=client->ReadAuthenticationNegotiationLine(password)) ||
		ID_OK!=(status=client->WriteAuthenticationNegotiationLine(password)))
	{
		return status;
	}
	return ID_OK;
}

wxmailto_status AuthenticateGlue::AuthenticateAPOP(AuthenticateClientAPI* client)
{
	wxmailto_status status;
	SafeString token; //Will become username+" "+digest
	SafeString password;
	SafeString secret; //Will become timestamp+password
	if (ID_OK!=(status=client->ReadAuthenticationNegotiationLine(token)) ||
	    ID_OK!=(status=client->ReadAuthenticationNegotiationLine(password)) ||
	    ID_OK!=(status=client->ReadAuthenticationNegotiationLine(secret)))
	{
		return status;
	}
	
	secret.Append(password);
	wxString digest;
	if (ID_OK!=(status=MD5(secret, digest)))
	{
		return status;
	}

	token.AppendStr(" ");
	token.AppendStr(digest);
	status = client->WriteAuthenticationNegotiationLine(token);
	return status;
}

wxmailto_status AuthenticateGlue::MD5(const SafeString& source, wxString& destination)
{
	wxmailto_status status;
	const wxUint8* source_bytes;
	wxSizeT source_length;
	if (ID_OK!=(status=source.Get(source_bytes, source_length)))
	{
		return status;
	}

	return MD5(source_bytes, source_length, destination);
}

wxmailto_status AuthenticateGlue::MD5(const wxUint8* source_bytes, wxSizeT source_length, wxString& destination)
{
	char* md5sum;
	if (GSASL_OK!=gsasl_md5(reinterpret_cast<const char*>(source_bytes), source_length, &md5sum))
	{
		free(md5sum);
		return LOGERROR(ID_GENERIC_ERROR);
	}
	wxmailto_status status = StringUtils::ByteArrayToHexString(reinterpret_cast<const wxUint8*>(md5sum), 16, destination);
	free(md5sum);
	return status;
}
