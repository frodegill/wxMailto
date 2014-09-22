#ifndef _TLS_GLUE_H_
#define _TLS_GLUE_H_

// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "tlsglue.h"
#endif

#include <wx/socket.h>
#include <wx/string.h>

#include <gnutls/gnutls.h>

#include "../gui/wxmailto_module.h"


namespace wxMailto
{

class TLSBaseSession
{
public:
	enum Status {
		STATUS_DISCONNECTED,
		STATUS_CONNECTING,
		STATUS_CONNECTED
	};

public:
	TLSBaseSession();
	virtual ~TLSBaseSession();
	wxmailto_status Initialize(wxSocketBase* socket, wxBool is_client);

	virtual bool IsConnected() const {return STATUS_CONNECTED==m_status;}
	virtual bool IsDisconnected() const {return STATUS_DISCONNECTED==m_status;}

protected:
	wxmailto_status Read(void* buffer, wxUint32 buffer_size, wxUint32& read_size);
	wxmailto_status Unread(const void* buffer, wxUint32 buffer_size, wxUint32& unread_size);
	wxmailto_status Peek(void* buffer, wxUint32 buffer_size, wxUint32& peek_size);
	wxmailto_status Write(const void* buffer, wxUint32 buffer_size, wxUint32& write_size);

	wxmailto_status StartTLS() {return Handshake();}
	wxmailto_status Bye();

protected:
	gnutls_session_t GetTLSSession() {return m_gnutls_session;}
	Status					GetStatus() const {return m_status;}
	void						SetStatus(const Status& status) {m_status=status;}

private:
	wxmailto_status Handshake();
	
private:
	wxSocketBase* m_socket;
	gnutls_session_t m_gnutls_session;
	gnutls_certificate_credentials_t m_cert_cred;
	wxBool				m_is_secure_connection;
	Status				m_status;
	wxUint8*			m_readahead_buffer;
	wxUint32			m_readahead_buffer_size;
	wxUint32			m_readahead_buffer_length;
};

class TLSClientSession : public TLSBaseSession
{
private:
friend class TLSGlue;
	TLSClientSession();
	virtual ~TLSClientSession();
	wxmailto_status Initialize(const wxSockAddress& address);

public:
	wxmailto_status Read(void* buffer, wxUint32 buffer_size, wxUint32& read_size) {return TLSBaseSession::Read(buffer, buffer_size, read_size);}
	wxmailto_status Unread(const void* buffer, wxUint32 buffer_size, wxUint32& unread_size) {return TLSBaseSession::Unread(buffer, buffer_size, unread_size);}
	wxmailto_status Peek(void* buffer, wxUint32 buffer_size, wxUint32& peek_size) {return TLSBaseSession::Peek(buffer, buffer_size, peek_size);}
	wxmailto_status Write(const void* buffer, wxUint32 buffer_size, wxUint32& write_size) {return TLSBaseSession::Write(buffer, buffer_size, write_size);}

	wxmailto_status StartTLS() {return TLSBaseSession::StartTLS();}
	wxmailto_status SetServerName(const wxString& server_name);

	wxmailto_status Bye() {return TLSBaseSession::Bye();}
};

class TLSServerSession : public TLSBaseSession
{
private:
friend class TLSGlue;
	TLSServerSession();
	virtual ~TLSServerSession();
	wxmailto_status Initialize(const wxSockAddress& address);

public:
	wxmailto_status StartTLS() {return TLSBaseSession::StartTLS();}

		wxmailto_status Bye() {return TLSBaseSession::Bye();}
};

class TLSGlue : public wxMailto_Module
{
public:
	TLSGlue() : wxMailto_Module() {}
 	virtual ~TLSGlue() {}

	wxString GetName() const {return "TLS";}
	ModuleType GetType() const {return wxMailto_Module::TLS;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status CreateClientSession(const wxSockAddress& address, TLSClientSession*& session);
	wxmailto_status CreateServerSession(const wxSockAddress& address, TLSServerSession*& session);
	wxmailto_status DeleteClientSession(TLSClientSession* session);
	wxmailto_status DeleteServerSession(TLSServerSession* session);

};

}

#endif // _TLS_GLUE_H_
