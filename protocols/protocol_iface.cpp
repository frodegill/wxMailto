
// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "protocol_iface.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "protocol_iface.h"

#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../wxmailto_errors.h"
#include "../wxmailto_rc.h"


AuthenticateCallbackReturn GenericAuthenticateCallback(AuthenticateCallbackParam)
{
	wxMailto::wxmailto_status status;
	wxMailto::ProtocolInterface* clientAPI = static_cast<wxMailto::ProtocolInterface*>(wxGetApp().GetAppModuleManager()->GetAuthenticateGlue()->FindClientAPI(sctx));
	switch(prop)
	{
		case GSASL_AUTHID: gsasl_property_set(sctx, GSASL_AUTHID, "test"); return GSASL_OK;
		case GSASL_AUTHZID: gsasl_property_set(sctx, GSASL_AUTHZID, ""); return GSASL_OK;
		case GSASL_PASSWORD: gsasl_property_set(sctx, GSASL_PASSWORD, "test"); return GSASL_OK;
		case GSASL_SERVICE:
		{
			wxString protocol;
			if (wxMailto::ID_OK!=(status=clientAPI->GetAccount()->GetProtocolString(protocol)))
				return GSASL_MALLOC_ERROR;
			
			gsasl_property_set(sctx, GSASL_SERVICE, protocol.ToUTF8());
			return GSASL_OK;
		}
		default: wxLogDebug(_("Unexpected GSASL property")); return GSASL_NO_CLIENT_CODE;
	}
	return clientAPI ? GSASL_OK : GSASL_NEEDS_MORE;
}

using namespace wxMailto;



ProtocolInterface::ProtocolInterface(Account* account)
: wxThread(),
  AuthenticateClientAPI(),
  m_account(account),
  m_tls_session(NULL),
  m_last_read_ended_in_CR(false)
{
}

wxThread::ExitCode ProtocolInterface::Entry()
{
	wxmailto_status status = Sync();
	m_account->OnSyncComplete(status, m_account->GetPollInterval()>0);
	return (wxThread::ExitCode)status;
}

wxmailto_status ProtocolInterface::InitializeAndConnectSocket()
{
	if (!m_tls_session)
	{
		wxIPV4address address;
		const wxString& server_name = m_account->GetServername();
		if (!address.Hostname(server_name) ||
		    !address.Service(m_account->GetPort()))
		{
			return LOGERROR(ID_UNKNOWN_SERVER);
		}

		wxGetApp().GetAppModuleManager()->GetTLSGlue()->CreateClientSession(address, m_tls_session);
		if (!m_tls_session)
			return LOGERROR(ID_OUT_OF_MEMORY);

		wxmailto_status status;
		if (ID_OK!=(status=m_tls_session->SetServerName(server_name))) {
			DestroySocket();
			return status;
		}
	}

	return ID_OK;
}

wxmailto_status ProtocolInterface::Peek(void* buffer, wxUint32 buffer_bytesize, wxUint32& peeked_bytes)
{
	if (!IsConnected())
		return LOGERROR(ID_NOT_CONNECTED);

	wxmailto_status status = m_tls_session->Peek(buffer, buffer_bytesize, peeked_bytes);
	return status;
}

wxmailto_status ProtocolInterface::ReadSome(char* buffer, wxUint32 buffer_bytesize, wxUint32& read_bytes)
{
	if (!IsConnected())
		return LOGERROR(ID_NOT_CONNECTED);

	wxmailto_status status = m_tls_session->Read(buffer, buffer_bytesize, read_bytes);
	return status;
}

wxmailto_status ProtocolInterface::ReadLine(char*& buffer, wxUint32& read_bytes)
{
	if (!IsConnected())
		return LOGERROR(ID_NOT_CONNECTED);

	static const wxSizeT READ_BUFFER_SIZE = 1024; //Possible to read at least this amount each call to ReadSome. Line-oriented protocols seldom allows lines longer than 1Kb
	wxSizeT read_buffer_size = READ_BUFFER_SIZE; //Init to one buffer. May grow
	char* read_buffer = new char[read_buffer_size];
	wxSizeT read_buffer_len = 0; //How much data is read into buffer
	wxUint32 current_read_bytes; //How many bytes were read in call to ReadSome

	if (!read_buffer)
		return LOGERROR(ID_OUT_OF_MEMORY);

	wxmailto_status status;
	if (m_last_read_ended_in_CR)
	{
		if (ID_OK!=(status=ReadSome(read_buffer, 1, current_read_bytes)))
		{
			delete[] read_buffer;
			return status;
		}

		m_last_read_ended_in_CR = false;
		wxASSERT(1==current_read_bytes);
		if ('\r' == *read_buffer) //CRCR(LF?), we're at the start of a new linefeed
		{
			m_last_read_ended_in_CR = true;
			buffer = read_buffer;
			read_bytes = 0;
			return ID_OK;
		} else if ('\n' != *read_buffer) //If we're not within a CRLF, keep byte. If not, we will simply overwrite it later
			read_buffer_len++;
	}

	const char* tmp_ptr;
	wxSizeT tmp_current_offset, tmp_next_offset;
	do
	{
		if (read_buffer_size < (read_buffer_len + READ_BUFFER_SIZE/2)) //realloc if we have less than half a buffer left
		{
			read_buffer_size = 2*read_buffer_size+READ_BUFFER_SIZE; //grow by twice and then some
			char* new_buffer = new char[read_buffer_size];
			if (!new_buffer)
			{
				delete[] read_buffer;
				return LOGERROR(ID_OUT_OF_MEMORY);
			}
			if (read_buffer && 0<read_buffer_len)
			{
				memcpy(new_buffer, read_buffer, read_buffer_len);
			}
			delete[] read_buffer;
			read_buffer = new_buffer;
		}
		
		if (ID_OK!=(status=ReadSome(read_buffer+read_buffer_len, read_buffer_size-read_buffer_len, current_read_bytes)))
		{
			delete[] read_buffer;
			return status;
		}

		tmp_ptr = read_buffer+read_buffer_len;
		for (tmp_current_offset=0; tmp_current_offset<current_read_bytes; tmp_current_offset++)
		{
			if ('\r'!=*(tmp_ptr+tmp_current_offset) && '\n'!=*(tmp_ptr+tmp_current_offset)) //Last test should not be needed (standard says CRLF), but I've seen servers sending only LFs
				continue;

			buffer = read_buffer;
			read_bytes = read_buffer_len+tmp_current_offset; //Return up to, but not including, CRLF

			tmp_next_offset = tmp_current_offset+1;
			if (tmp_next_offset >= current_read_bytes)
			{
				m_last_read_ended_in_CR = true;
			}
			else
			{
				if ('\n' == *(tmp_ptr+tmp_next_offset)) {
					tmp_current_offset++;
					tmp_next_offset++;
				}

				if (current_read_bytes > tmp_next_offset) {
					wxUint32 dummy = 0;
					m_tls_session->Unread(tmp_ptr+tmp_next_offset, current_read_bytes-tmp_next_offset, dummy);
				}
			}

			return ID_OK;
		}
		read_buffer_len += current_read_bytes;
	} while (0<current_read_bytes);

	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

wxmailto_status ProtocolInterface::ReadLine(wxString& buffer, const wxString& charset)
{
	wxCSConv converter(charset);
	wxASSERT(converter.IsOk());
	if (!converter.IsOk())
		return LOGERROR(ID_INVALID_CONVERTER);

	wxmailto_status status;
	char* raw_buffer;
	wxUint32 raw_buffer_length;
	if (ID_OK!=(status=ReadLine(raw_buffer, raw_buffer_length)))
		return status;

	//Convert
	wxSizeT converted_length = converter.ToWChar(NULL, wxNO_LEN, raw_buffer, raw_buffer_length);
	wxChar* converted = new wxChar[converted_length];
	if (!converted)
	{
		delete[] raw_buffer;
		return LOGERROR(ID_OUT_OF_MEMORY);
	}

	converter.ToWChar(converted, converted_length, raw_buffer, raw_buffer_length);
	delete[] raw_buffer;
	buffer = wxString(converted, converted_length);
	delete[] converted;
	return ID_OK;
}

wxmailto_status ProtocolInterface::Write(char* buffer, wxUint32 buffer_bytesize, wxUint32& written_bytes)
{
	if (!IsConnected())
		return LOGERROR(ID_NOT_CONNECTED);

	wxmailto_status status = ID_OK;
	wxUint32 remaining = buffer_bytesize;
	while (ID_OK==status && 0<remaining)
	{
	  status = m_tls_session->Write(buffer, buffer_bytesize, written_bytes);
	  remaining -= written_bytes;
	}
	return status;
}

wxmailto_status ProtocolInterface::Write(const wxString& buffer, const wxString& charset)
{
	wxCSConv converter(charset);
	wxASSERT(converter.IsOk());
	if (!converter.IsOk())
		return LOGERROR(ID_INVALID_CONVERTER);

	//Convert
	wxSizeT converted_length = converter.FromWChar(NULL, wxNO_LEN, buffer.c_str(), wxNO_LEN);
	char* converted = new char[converted_length];
	if (!converted)
		return LOGERROR(ID_OUT_OF_MEMORY);

	converter.FromWChar(converted, converted_length, buffer.c_str(), wxNO_LEN);
	converted_length--; //Don't send zero-terminator

	//Write
	wxUint32 written_bytes = 0;
	wxmailto_status status = Write(converted, converted_length, written_bytes);
	wxASSERT(written_bytes==converted_length);
	delete[] converted;
	return status;
}

wxmailto_status ProtocolInterface::StartTLS()
{
	if (!IsConnected())
		return LOGERROR(ID_NOT_CONNECTED);
	
	return m_tls_session->StartTLS();
}

wxmailto_status ProtocolInterface::DestroySocket()
{
	if (m_tls_session)
	{
		wxGetApp().GetAppModuleManager()->GetTLSGlue()->DeleteClientSession(m_tls_session);
		m_tls_session = NULL;
	}
	return ID_OK;
}

wxmailto_status ProtocolInterface::DoAuthenticate(const wxString& method, AuthenticateCallbackFunction callback)
{
	m_do_authenticate_method = method;
	m_do_authenticate_index = 0;
	m_in_authenticate_mode = false;
	return wxGetApp().GetAppModuleManager()->GetAuthenticateGlue()->Authenticate(method, callback, this);
}
