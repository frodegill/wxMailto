
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "pipe.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "../gui/wxmailto_app.h"

#include "pipe.h"


using namespace wxMailto;

Pipe::Pipe()
: wxThread(wxTHREAD_DETACHED),
  m_signal_condition(NULL),
  m_requested_bytes(0),
  m_got_bytes(false),
  m_should_terminate(false),
  m_source(NULL),
  m_source_buffer(NULL),
  m_source_buffer_len(0),
  m_source_buffer_pos(0),
  m_sink(NULL),
  m_sink_buffer(NULL),
  m_sink_buffer_len(0),
  m_sink_buffer_pos(0)
{
	m_signal_condition = new wxCondition(m_signal_lock);
}

Pipe::~Pipe()
{
	delete[] m_source_buffer;
	delete[] m_sink_buffer;

	m_should_terminate = true;
	if (m_signal_condition)
	{
		m_signal_condition->Signal();
		delete m_signal_condition;
	}
}

wxmailto_status Pipe::InitializeSourceBuffer(const wxSizeT& initial_size, const wxSizeT& max_size)
{
	{
		wxCriticalSectionLocker locker(m_source_buffer_lock);

		if (m_source_buffer)
			return LOGERROR(ID_SHOULDNT_GET_HERE);

		m_source_buffer = new wxUint8[initial_size];
		if (!m_source_buffer)
			return LOGERROR(ID_OUT_OF_MEMORY);

		m_source_buffer_len = initial_size;
		m_source_buffer_pos = 0;
		m_max_source_buffer_len = max_size ? max_size : initial_size;
	}
	return ID_OK;
}

wxmailto_status Pipe::InitializeSinkBuffer(const wxSizeT& initial_size, const wxSizeT& max_size)
{
	{
		wxCriticalSectionLocker locker(m_sink_buffer_lock);

		if (m_sink_buffer)
			return LOGERROR(ID_SHOULDNT_GET_HERE);

		m_sink_buffer = new wxUint8[initial_size];
		if (!m_sink_buffer)
			return LOGERROR(ID_OUT_OF_MEMORY);

		m_sink_buffer_len = initial_size;
		m_sink_buffer_pos = 0;
		m_max_sink_buffer_len = max_size ? max_size : initial_size;
	}
	return ID_OK;
}

Pipe* Pipe::ConnectTo(Pipe* sink)
{
	{
		wxCriticalSectionLocker locker(m_pipe_lock);
		m_sink = sink;
	}
	{
		wxCriticalSectionLocker locker(sink->m_pipe_lock);
		sink->m_source = this;
	}
	return sink;
}

wxThread::ExitCode Pipe::Entry()
{
	//1) sleep until terminate, sink wants sink_buffer bytes or we have received source_bytes
	//2) if not terminate and sink wants sink_buffer bytes or we have received source_bytes:
	//3)  if sink wants bytes and we have sink_buffer bytes, move sink_buffer bytes to sink
	//4)  if we have source_buffer bytes and room for sink_buffer bytes, process source_buffer bytes
	//5)  if not eof and we have room for source_buffer bytes, request source_buffer bytes
	//6)  if 3, 4 or 5 did something, goto 2
	//7) goto 1

	wxmailto_status status;
	wxBool processed_something = true; //true initially, to get things rolling..
	while (true)
	{
		//1)
		{
			wxCriticalSectionLocker locker(m_pipe_lock);
			if (false != m_should_terminate)
				break;
		}
		
		if (!processed_something)
		{
			m_signal_condition->Wait();
		}

		//2)
		{
			wxCriticalSectionLocker locker(m_pipe_lock);
			if (false != m_should_terminate)
				break;
		}

		processed_something = false;

		//3)
		{
			wxCriticalSectionLocker sink_locker(m_sink_buffer_lock);
			wxCriticalSectionLocker pipe_locker(m_pipe_lock);

			wxSizeT buffer_length = UMIN(m_sink_buffer_len, m_requested_bytes);
			while (0 < buffer_length)
			{
				if (ID_OK != (status=m_sink->ReceiveBytes(m_sink_buffer, buffer_length)))
				{
					ReportPipeFailure(status);
					continue;
				}
				
				if (0 < buffer_length)
				{
					processed_something = true;
					memmove(m_sink_buffer, m_sink_buffer+buffer_length, buffer_length);
					m_sink_buffer_pos -= buffer_length;
					m_requested_bytes -= buffer_length;

					buffer_length = UMIN(m_sink_buffer_len, m_requested_bytes);
				}
			}
		}

		//4)
		{
			wxCriticalSectionLocker source_locker(m_source_buffer_lock);
			wxCriticalSectionLocker sink_locker(m_sink_buffer_lock);

			wxSizeT source_buffer_length = m_source_buffer_pos;
			wxSizeT sink_buffer_length;
			while (0 < source_buffer_length)
			{
				if (0.95 < (static_cast<double>(m_sink_buffer_pos+source_buffer_length) / static_cast<double>(m_sink_buffer_len)))
				{
					wxmailto_status status = GrowSinkBuffer();
					if (ID_OK != status)
						return (wxThread::ExitCode)status;
				}

				sink_buffer_length = m_sink_buffer_len-m_sink_buffer_pos;

				if (ID_OK != (status=Process(m_source_buffer, source_buffer_length,
				                             m_sink_buffer+m_sink_buffer_pos, sink_buffer_length)))
				{
					ReportPipeFailure(status);
					continue;
				}
				
				if (0<source_buffer_length || 0<sink_buffer_length)
				{
					processed_something = true;
					memmove(m_source_buffer, m_source_buffer+source_buffer_length, source_buffer_length);
					m_source_buffer_pos -= source_buffer_length;
					m_sink_buffer_pos += sink_buffer_length;

					source_buffer_length = m_source_buffer_pos;
				}
			}
		}

		//5)
		{
			wxCriticalSectionLocker locker(m_source_buffer_lock);

			if (!Eof() && (m_source_buffer_pos+1)<m_source_buffer_len)
			{
				if (ID_OK != (status=m_source->RequestBytes(m_source_buffer_len-m_source_buffer_pos)))
				{
					ReportPipeFailure(status);
					continue;
				}
			}
		}
	}
	return (wxThread::ExitCode)0;
}

wxmailto_status Pipe::StartFlow()
{
	Pipe* current_pipe = this;
	//Find source
	while (true)
	{
		wxCriticalSectionLocker locker(current_pipe->m_pipe_lock);
		if (!current_pipe->m_source)
			break;
		
		current_pipe = current_pipe->m_source;
	}

	//Start all pipes, from source to sink
	while (current_pipe)
	{
		wxCriticalSectionLocker locker(current_pipe->m_pipe_lock);
		current_pipe->Run();
		current_pipe = current_pipe->m_sink;
	}
	return ID_OK;
}

void Pipe::Terminate()
{
	{
		wxCriticalSectionLocker locker(m_pipe_lock);

		if (m_source)
			m_source->Terminate();

		m_should_terminate = true;
	}
}

void Pipe::ReportPipeFailure(const wxmailto_status& status, const wxString& msg)
{
	{
		wxCriticalSectionLocker locker(m_pipe_lock);

		if (m_sink)
		{
			m_source->ReportPipeFailure(status, msg);
		}
		else
		{
			//ToDo, display error message?

			Terminate();
		}
	}
}

wxBool Pipe::Eof()
{
	{
		wxCriticalSectionLocker locker(m_source_buffer_lock);

		if (0 < m_source_buffer_pos)
			return false;
	}

	{
		wxCriticalSectionLocker locker(m_sink_buffer_lock);

		if (0 < m_sink_buffer_pos)
			return false;
	}

	return m_source ? m_source->Eof() : false;
}

//Notify that this sink_buffer can receive requested_bytes more bytes. Should only be called by m_sink
wxmailto_status Pipe::RequestBytes(const wxSizeT& requested_bytes)
{
	{
		wxCriticalSectionLocker locker(m_pipe_lock);
		
		m_requested_bytes += requested_bytes;
	}

	m_signal_condition->Signal();
	return ID_OK;
}

//NOOP-pipe
wxmailto_status Pipe::Process(wxUint8* source_buffer, wxSizeT& source_buffer_len,
                              wxUint8* sink_buffer, wxSizeT& sink_buffer_len)
{
	if (!source_buffer || !sink_buffer)
		return LOGERROR(ID_INVALID_ARGUMENT);

	wxSizeT process_length = UMIN(source_buffer_len, sink_buffer_len);
	if (0 == process_length)
	{
		source_buffer_len = sink_buffer_len = 0;
	}
	else
	{
		memcpy(sink_buffer, source_buffer, process_length);
		source_buffer_len = process_length;
		sink_buffer_len = process_length;
	}
	return ID_OK;
}

//Read from m_source sink_buffer to this source_buffer. Should only be called by m_source
wxmailto_status Pipe::ReceiveBytes(wxUint8* buffer, wxSizeT& buffer_length)
{
	{
		wxCriticalSectionLocker locker(m_source_buffer_lock);

		if (0.95 < (static_cast<double>(m_source_buffer_pos+buffer_length) / static_cast<double>(m_source_buffer_len)))
		{
			wxmailto_status status = GrowSourceBuffer();
			if (ID_OK != status)
				return status;
		}

		wxSizeT move_length = UMIN(buffer_length, m_source_buffer_len-m_source_buffer_pos);
		if (0 == move_length)
			return ID_OK;

		memcpy(m_source_buffer+m_source_buffer_pos, buffer, move_length);
		m_sink_buffer_pos += move_length;
		buffer_length = move_length;
	}

	{
		wxCriticalSectionLocker locker(m_source_buffer_lock);

		m_got_bytes = true;
	}
	
	m_signal_condition->Signal();
	return ID_OK;
}

wxmailto_status Pipe::GrowSourceBuffer()
{
	{
		wxCriticalSectionLocker locker(m_source_buffer_lock);
		
		wxSizeT new_buffer_size = UMIN(2*m_source_buffer_len, m_max_source_buffer_len);
		if (new_buffer_size <= m_source_buffer_len)
			return ID_OK;
		
		wxUint8* new_buffer = new wxUint8[new_buffer_size];
		if (!new_buffer)
			return LOGERROR(ID_OUT_OF_MEMORY);

		memcpy(new_buffer, m_source_buffer, m_source_buffer_pos);
		delete[] m_source_buffer;
		m_source_buffer = new_buffer;
		m_source_buffer_len = new_buffer_size;
	}
	return ID_OK;
}

wxmailto_status Pipe::GrowSinkBuffer()
{
	{
		wxCriticalSectionLocker locker(m_sink_buffer_lock);
		
		wxSizeT new_buffer_size = UMIN(2*m_sink_buffer_len, m_max_sink_buffer_len);
		if (new_buffer_size <= m_sink_buffer_len)
			return ID_OK;
		
		wxUint8* new_buffer = new wxUint8[new_buffer_size];
		if (!new_buffer)
			return LOGERROR(ID_OUT_OF_MEMORY);

		memcpy(new_buffer, m_sink_buffer, m_sink_buffer_pos);
		delete[] m_sink_buffer;
		m_sink_buffer = new_buffer;
		m_sink_buffer_len = new_buffer_size;
	}
	return ID_OK;
}
