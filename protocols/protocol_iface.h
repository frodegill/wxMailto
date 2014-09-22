#ifndef _PROTOCOL_IFACE_H_
#define _PROTOCOL_IFACE_H_

// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "protocol_iface.h"
#endif

#include <wx/thread.h>
#include "../defines.h"
#include "../wxmailto_errors.h"

#include "../glue/authenticateglue.h"
#include "../glue/tlsglue.h"
#include "../storage/account.h"


AuthenticateCallbackReturn GenericAuthenticateCallback(AuthenticateCallbackParam);

namespace wxMailto
{
static const wxString s_raw_encoding = "iso-8859-1"; //Should actually be us-ascii..
static const wxString s_SP = " ";
static const wxString s_CRLF = "\r\n";

class ProtocolInterface : public wxThread, public AuthenticateClientAPI
{
public:
	ProtocolInterface(Account* account);
	virtual ~ProtocolInterface() {}

public:
	virtual wxmailto_status Sync() = 0;
	virtual wxmailto_status CleanupAndAbort() = 0;

public: //AuthenticateClientAPI API
	//virtual wxmailto_status ReadAuthenticationNegotiationLine(SafeString& buffer);
	//virtual wxmailto_status WriteAuthenticationNegotiationLine(const SafeString& buffer);
	//virtual void WriteAuthenticationAbortedLine();

public: //wxThread API
	virtual wxThread::ExitCode Entry();

public:
	virtual wxmailto_status InitializeAndConnectSocket();
	virtual wxBool IsConnected() const {return m_tls_session!=NULL;}
	virtual wxmailto_status Peek(void* buffer, wxUint32 buffer_bytesize, wxUint32& peeked_bytes);
	virtual wxmailto_status ReadSome(char* buffer, wxUint32 buffer_bytesize, wxUint32& read_bytes);
	virtual wxmailto_status ReadLine(char*& buffer, wxUint32& read_bytes); //If returning true, caller must delete[] buffer
	virtual wxmailto_status ReadLine(wxString& buffer, const wxString& charset);
	virtual wxmailto_status Write(char* buffer, wxUint32 buffer_bytesize, wxUint32& written_bytes);
	virtual wxmailto_status Write(const wxString& buffer, const wxString& charset);
	virtual wxmailto_status StartTLS();
	virtual wxmailto_status DestroySocket();

	wxmailto_status DoAuthenticate(const wxString& method, AuthenticateCallbackFunction callback);

	const Account* GetAccount() const {return m_account;}
	
	const wxString& GetDoAuthenticateMethod() const {return m_do_authenticate_method;}
	wxInt GetAndIncrementDoAuthenticateIndex() {return m_do_authenticate_index++;}
	void SetIsInAuthenticateMode(wxBool in_authenticate_mode) {m_in_authenticate_mode=in_authenticate_mode;}
	wxBool IsInAuthenticateMode() const {return m_in_authenticate_mode;}

private:
	Account* m_account;
	TLSClientSession* m_tls_session;

	wxBool m_last_read_ended_in_CR;

	//Variables used in AuthenticateGlue callback
	wxString	m_do_authenticate_method;
	wxInt		m_do_authenticate_index;
	wxBool	m_in_authenticate_mode;
};

}

#endif // _PROTOCOL_IFACE_H_
