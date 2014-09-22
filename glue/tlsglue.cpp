
// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "tlsglue.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "tlsglue.h"

#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"

using namespace wxMailto;
/*
int (*mutex_init_func) (void **mutex)
{
	*mutex = new wxMutex();
	return (*mutex)?GNUTLS_E_SUCCESS:GNUTLS_E_MEMORY_ERROR;
}

int mutex_lock_func(void **mutex)
{
	return (wxMUTEX_NO_ERROR==reinterpret_cast<wxMutex*>(*mutex)->Lock())?GNUTLS_E_SUCCESS:GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}

int mutex_unlock_func(void **mutex)
{
	return (wxMUTEX_NO_ERROR==reinterpret_cast<wxMutex*>(*mutex)->Unlock())?GNUTLS_E_SUCCESS:GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}

int mutex_deinit_func(void **mutex)
{
	delete reinterpret_cast<wxMutex*>(*mutex);
	return GNUTLS_E_SUCCESS;
}
*/
ssize_t wxsocket_tls_pull_func(gnutls_transport_ptr_t ptr, void* data, size_t len)
{
	wxSocketBase* socket = reinterpret_cast<wxSocketBase*>(ptr);

	if (!socket->IsData() && !socket->WaitForRead(5))
		return 0;

	socket->Read(data, len);
	return socket->Error() ? -1 : socket->LastCount();
}

ssize_t wxsocket_tls_push_func(gnutls_transport_ptr_t ptr, const void* data, size_t len)
{
	wxSocketBase* socket = reinterpret_cast<wxSocketBase*>(ptr);
	
	if (!socket->IsData() && !socket->WaitForWrite(5))
		return 0;

	socket->Write(data, len);
	return socket->Error() ? -1 : socket->LastCount();
}


TLSBaseSession::TLSBaseSession()
: m_socket(NULL),
  m_gnutls_session(NULL),
  m_cert_cred(NULL),
  m_is_secure_connection(false),
  m_status(STATUS_DISCONNECTED),
  m_readahead_buffer(NULL),
  m_readahead_buffer_size(0),
  m_readahead_buffer_length(0)
{
}

TLSBaseSession::~TLSBaseSession()
{
	if (m_socket)
	{
		m_socket->Destroy();
		delete m_socket;
	}
	delete[] m_readahead_buffer;
	gnutls_deinit(m_gnutls_session);
	gnutls_certificate_free_credentials(m_cert_cred);
}

wxmailto_status TLSBaseSession::Initialize(wxSocketBase* socket, wxBool is_client)
{
	m_socket=socket;
	if (!socket)
	{
		SetStatus(STATUS_DISCONNECTED);
		return LOGERROR(ID_NULL_POINTER);
	}

	gnutls_certificate_allocate_credentials(&m_cert_cred);
	gnutls_init(&m_gnutls_session, is_client ? GNUTLS_CLIENT : GNUTLS_SERVER);
	gnutls_credentials_set(m_gnutls_session, GNUTLS_CRD_CERTIFICATE, m_cert_cred);
	

	gnutls_transport_set_ptr(m_gnutls_session, socket);
  gnutls_transport_set_push_function(m_gnutls_session, wxsocket_tls_push_func);
  gnutls_transport_set_pull_function(m_gnutls_session, wxsocket_tls_pull_func);

	gnutls_priority_set_direct(m_gnutls_session, "NORMAL", NULL);

	return ID_OK;
}

wxmailto_status TLSBaseSession::Read(void* buffer, wxUint32 buffer_size, wxUint32& read_size)
{
	read_size = 0;
	wxUint8* byte_buffer = reinterpret_cast<wxUint8*>(buffer);
	//Copy any existing content
	if (0<m_readahead_buffer_length) {
		//Enough to fill the entire buffer
		if (buffer_size<=m_readahead_buffer_length) {
			memcpy(byte_buffer, m_readahead_buffer, buffer_size);
			memmove(m_readahead_buffer, m_readahead_buffer+buffer_size, m_readahead_buffer_length-buffer_size);
			m_readahead_buffer_length -= buffer_size;
			read_size = buffer_size;
			return ID_OK;
		}

		//Copy as much as we have, and read to fill rest of buffer
		memcpy(byte_buffer, m_readahead_buffer, m_readahead_buffer_length);
		byte_buffer += m_readahead_buffer_length;
		buffer_size -= m_readahead_buffer_length;
		read_size = m_readahead_buffer_length;
		//Discard now empty buffer
		delete[] m_readahead_buffer;
		m_readahead_buffer = NULL;
		m_readahead_buffer_length = 0;
		m_readahead_buffer_size = 0;
		if (!m_socket->IsData()) //If we don't have more data, return what we have
			return ID_OK;
	}

	ssize_t recv_size;
	if (!m_is_secure_connection) {
		recv_size = wxsocket_tls_pull_func(m_socket, byte_buffer, buffer_size);
		wxLogDebug("S: "+wxString(reinterpret_cast<const char*>(byte_buffer), recv_size));
	} else {
		recv_size = gnutls_record_recv(m_gnutls_session, byte_buffer, buffer_size);
		wxLogDebug("S: "+wxString(reinterpret_cast<const char*>(byte_buffer), recv_size));

		if (0>recv_size) {
			if (GNUTLS_E_REHANDSHAKE==recv_size ||
			    GNUTLS_E_INTERRUPTED==recv_size ||
			    GNUTLS_E_AGAIN==recv_size)
			{
				wxmailto_status status;
				if (GNUTLS_E_REHANDSHAKE==recv_size) {
					if (ID_OK!=(status=Handshake()))
						return status;
				}

				recv_size = gnutls_record_recv(m_gnutls_session, byte_buffer, buffer_size);
			}
		}
	}
	
	if (0>recv_size)
		return LOGERROR(ID_NETWORK_ERROR);
	
	read_size += recv_size;
	return ID_OK;
}

wxmailto_status TLSBaseSession::Unread(const void* buffer, wxUint32 buffer_size, wxUint32& unread_size)
{
	if (0>=buffer_size) {
		unread_size = 0;
		return ID_OK;
	}

	//Is existing buffer large enough?
	if (m_readahead_buffer_size>=(m_readahead_buffer_length+buffer_size)) {
		memcpy(m_readahead_buffer+m_readahead_buffer_length, buffer, buffer_size);
		m_readahead_buffer_length += buffer_size;
		unread_size = buffer_size;
		return ID_OK;
	}

	//Allocate new buffer
	wxUint8* readahead_buffer = new wxUint8[m_readahead_buffer_length+buffer_size];
	if (!readahead_buffer)
		return LOGERROR(ID_OUT_OF_MEMORY);

	//Copy existing content
	if (0<m_readahead_buffer_length) {
		memcpy(readahead_buffer, m_readahead_buffer, m_readahead_buffer_length);
	}
	//Append new content
	memcpy(readahead_buffer+m_readahead_buffer_length, buffer, buffer_size);
	delete[] m_readahead_buffer;
	m_readahead_buffer = readahead_buffer;
	m_readahead_buffer_length += buffer_size;
	m_readahead_buffer_size = m_readahead_buffer_length;
	unread_size = buffer_size;
	return ID_OK;
}

wxmailto_status TLSBaseSession::Peek(void* buffer, wxUint32 buffer_size, wxUint32& peek_size)
{
	wxmailto_status status;
	wxUint32 unread_size = 0;
	if (ID_OK!=(status=Read(buffer, buffer_size, peek_size)) ||
		  ID_OK!=(status=Unread(buffer, peek_size, unread_size))) {
		return status;
	}

	if (unread_size!=peek_size)
		return LOGERROR(ID_NETWORK_ERROR);

 	return ID_OK;
}

wxmailto_status TLSBaseSession::Write(const void* buffer, wxUint32 buffer_size, wxUint32& write_size)
{
	ssize_t send_size;
	if (!m_is_secure_connection) {
		send_size = wxsocket_tls_push_func(m_socket, buffer, buffer_size);
		wxLogDebug("C: "+wxString(static_cast<const char*>(buffer), send_size));
	} else {
		send_size = gnutls_record_send(m_gnutls_session, buffer, buffer_size);
		wxLogDebug("C: "+wxString(static_cast<const char*>(buffer), send_size));

		if (0>send_size) {
			if (GNUTLS_E_INTERRUPTED==send_size ||
			    GNUTLS_E_AGAIN==send_size)
			{
				send_size = gnutls_record_send(m_gnutls_session, buffer, buffer_size);
			}
		}
	}
	
	if (0>send_size)
		return LOGERROR(ID_NETWORK_ERROR);
	
	write_size = send_size;
	return ID_OK;
}

wxmailto_status TLSBaseSession::Handshake()
{
	int tls_status = gnutls_handshake(m_gnutls_session);
	wxmailto_status status = (GNUTLS_E_SUCCESS==tls_status?ID_OK:LOGERROR(ID_NETWORK_ERROR));
	if (ID_OK==status) {
		m_is_secure_connection = true;
	} else {
		wxLogDebug("Handshake failed");
	}
	return status;
}

wxmailto_status TLSBaseSession::Bye()
{
	if (m_is_secure_connection) {
		gnutls_bye(m_gnutls_session, GNUTLS_SHUT_RDWR);
	}
	return ID_OK;
}


TLSClientSession::TLSClientSession()
: TLSBaseSession()
{
}

TLSClientSession::~TLSClientSession()
{
}

wxmailto_status TLSClientSession::Initialize(const wxSockAddress& address)
{
	wxSocketClient* socket = new wxSocketClient(wxSOCKET_NOWAIT);
	if (!socket)
			return LOGERROR(ID_OUT_OF_MEMORY);

	wxmailto_status status;
	if (ID_OK!=(status=TLSBaseSession::Initialize(socket, true)))
	{
		delete socket;
		return status;
	}
	
	SetStatus(STATUS_CONNECTING);
	if (!socket->Connect(address, true))
	{
		delete socket;
		return LOGERROR(ID_CONNECTION_FAILED);
	}

	SetStatus(STATUS_CONNECTED);
	return ID_OK;
}

wxmailto_status TLSClientSession::SetServerName(const wxString& server_name)
{
	wxString idna_name;
	wxmailto_status status;
	if (ID_OK!=(status=wxGetApp().GetAppModuleManager()->GetIdnaGLue()->ToASCII(server_name, idna_name)))
		return status;
	
	const char* idna_name_ascii = idna_name.c_str();
	if (!idna_name_ascii)
		return LOGERROR(ID_NULL_POINTER);
	
	const char* domain_separator = strchr(idna_name_ascii, '.');
	if (NULL==domain_separator)
		domain_separator = strchr(idna_name_ascii, '\0');

	gnutls_server_name_set(GetTLSSession(), GNUTLS_NAME_DNS, idna_name_ascii, domain_separator-idna_name_ascii);
	return ID_OK;
}


TLSServerSession::TLSServerSession()
: TLSBaseSession()
{
}

TLSServerSession::~TLSServerSession()
{
}

wxmailto_status TLSServerSession::Initialize(const wxSockAddress& address)
{
	wxSocketServer* socket = new wxSocketServer(address, wxSOCKET_NOWAIT);
	if (!socket)
			return LOGERROR(ID_OUT_OF_MEMORY);
		
	wxmailto_status status;
	if (ID_OK!=(status=TLSBaseSession::Initialize(socket, false)))
	{
		delete socket;
		return status;
	}

	SetStatus(STATUS_DISCONNECTED);

	return ID_OK;
}

/***********************************/

wxmailto_status TLSGlue::Initialize()
{
//	gnutls_global_set_mutex(mutex_init_func, mutex_deinit_func, mutex_lock_func, mutex_unlock_func);

	if (GNUTLS_E_SUCCESS != gnutls_global_init())
		return LOGERROR(ID_GENERIC_ERROR);

	wxGetApp().GetAppModuleManager()->RegisterModule(this);

	IdnaGlue* idna_glue = wxGetApp().GetAppModuleManager()->GetIdnaGLue();
	if (!idna_glue)
		return LOGERROR(ID_NULL_POINTER);
	
	idna_glue->AddModuleDependency();

	return ID_OK;
}

wxmailto_status TLSGlue::PrepareShutdown()
{
	WaitForNoMoreDependencies();

	IdnaGlue* idna_glue = wxGetApp().GetAppModuleManager()->GetIdnaGLue();
	if (idna_glue)
		idna_glue->RemoveModuleDependency();

	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

  gnutls_global_deinit();
	return ID_OK;
}

wxmailto_status TLSGlue::CreateClientSession(const wxSockAddress& address, TLSClientSession*& session)
{
	session = new TLSClientSession;
	if (!session)
		return LOGERROR(ID_OUT_OF_MEMORY);

	wxmailto_status status;
	if (ID_OK!=(status=session->Initialize(address)))
	{
		delete session;
		return status;
	}

	return ID_OK;
}

wxmailto_status TLSGlue::CreateServerSession(const wxSockAddress& address, TLSServerSession*& session)
{
	session = new TLSServerSession;
	if (!session)
		return LOGERROR(ID_OUT_OF_MEMORY);

	wxmailto_status status;
	if (ID_OK!=(status=session->Initialize(address)))
	{
		delete session;
		return status;
	}
	
	return ID_OK;
}

wxmailto_status TLSGlue::DeleteClientSession(TLSClientSession* session)
{
	if (!session)
		return ID_OK;

	session->Bye();

	delete session;
	return ID_OK;
}

wxmailto_status TLSGlue::DeleteServerSession(TLSServerSession* session)
{
	if (!session)
		return ID_OK;

	session->Bye();

	delete session;
	return ID_OK;
}
