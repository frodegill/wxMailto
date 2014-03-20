
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

Pipe::Pipe(wxInt id)
: wxThread(wxTHREAD_DETACHED),
  m_id(id),
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
	LOGDEBUG(wxString::Format("%d pipe: ctor\n", m_id));
	m_signal_condition = new wxCondition(m_signal_lock);
	LOGDEBUG(wxString::Format("%d pipe: ctor signal ready\n", m_id));
}

Pipe::~Pipe()
{
	LOGDEBUG(wxString::Format("%d pipe: dtor\n", m_id));
	delete[] m_source_buffer;
	delete[] m_sink_buffer;

	m_should_terminate = true;
	if (m_signal_condition)
	{
		LOGDEBUG(wxString::Format("%d pipe: dtor signalling\n", m_id));
		m_signal_condition->Signal();
		LOGDEBUG(wxString::Format("%d pipe: dtor signal unavailable\n", m_id));
		delete m_signal_condition;
	}
}

wxmailto_status Pipe::InitializeSourceBuffer(const wxSizeT& initial_size, const wxSizeT& max_size)
{
	{
		LOGDEBUG(wxString::Format("%d pipe InitializeSourceBuffer(1): aquiring source_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_source_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe InitializeSourceBuffer(1): aquired source_buffer lock\n", m_id));

		if (m_source_buffer)
		{
			LOGDEBUG(wxString::Format("%d pipe: source_buffer is non-NULL!\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe InitializeSourceBuffer(1.0): releasing source_buffer lock\n", m_id));
			return LOGERROR(ID_SHOULDNT_GET_HERE);
		}

		LOGDEBUG(wxString::Format("%d pipe: allocating source_buffer. len:%d\n", m_id, initial_size));
		m_source_buffer = new wxUint8[initial_size];
		if (!m_source_buffer)
		{
			LOGDEBUG(wxString::Format("%d pipe: allocating source_buffer failed\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe InitializeSourceBuffer(1.1): releasing source_buffer lock\n", m_id));
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		m_source_buffer_len = initial_size;
		m_source_buffer_pos = 0;
		m_max_source_buffer_len = max_size ? max_size : initial_size;
		LOGDEBUG(wxString::Format("%d pipe: source_buffer ready. len:%d pos=0 max=%d\n", m_id, m_source_buffer_len, m_max_source_buffer_len));
		LOGDEBUG(wxString::Format("%d pipe InitializeSourceBuffer(1.2): releasing source_buffer lock\n", m_id));
	}
	return ID_OK;
}

wxmailto_status Pipe::InitializeSinkBuffer(const wxSizeT& initial_size, const wxSizeT& max_size)
{
	{
		LOGDEBUG(wxString::Format("%d pipe InitializeSinkBuffer(1): aquiring sink_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_sink_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe InitializeSinkBuffer(1): aquired sink_buffer lock\n", m_id));

		if (m_sink_buffer)
		{
			LOGDEBUG(wxString::Format("%d pipe: sink_buffer is non-NULL!\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe InitializeSinkBuffer(1.0): releasing sink_buffer lock\n", m_id));
			return LOGERROR(ID_SHOULDNT_GET_HERE);
		}

		LOGDEBUG(wxString::Format("%d pipe: allocating sink_buffer. len:%d\n", m_id, initial_size));
		m_sink_buffer = new wxUint8[initial_size];
		if (!m_sink_buffer)
		{
			LOGDEBUG(wxString::Format("%d pipe: allocating sink_buffer failed\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe InitializeSinkBuffer(1.1): releasing sink_buffer lock\n", m_id));
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		m_sink_buffer_len = initial_size;
		m_sink_buffer_pos = 0;
		m_max_sink_buffer_len = max_size ? max_size : initial_size;
		LOGDEBUG(wxString::Format("%d pipe: sink_buffer ready. len:%d pos=0 max=%d\n", m_id, m_sink_buffer_len, m_max_sink_buffer_len));
		LOGDEBUG(wxString::Format("%d pipe InitializeSinkBuffer(1.2): releasing sink_buffer lock\n", m_id));
	}
	return ID_OK;
}

Pipe* Pipe::ConnectTo(Pipe* sink)
{
	{
		LOGDEBUG(wxString::Format("%d pipe ConnectTo(1): aquiring pipe lock\n", m_id));
		wxCriticalSectionLocker locker(m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe ConnectTo(1): aquired pipe lock\n", m_id));
		m_sink = sink;
		LOGDEBUG(wxString::Format("%d pipe: connected this to sink\n", m_id));
		LOGDEBUG(wxString::Format("%d pipe ConnectTo(1): releasing pipe lock\n", m_id));
	}
	{
		LOGDEBUG(wxString::Format("%d pipe ConnectTo(2): aquiring sink->pipe lock\n", m_id));
		wxCriticalSectionLocker locker(sink->m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe ConnectTo(2): aquired sink->pipe lock\n", m_id));
		sink->m_source = this;
		LOGDEBUG(wxString::Format("%d pipe: connected sink to this\n", m_id));
		LOGDEBUG(wxString::Format("%d pipe ConnectTo(2): releasing sink->pipe lock\n", m_id));
	}
	return sink;
}

wxThread::ExitCode Pipe::Entry()
{
	LOGDEBUG(wxString::Format("%d pipe: Entry()\n", m_id));

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
			LOGDEBUG(wxString::Format("%d pipe Entry(1): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(1): aquired pipe lock\n", m_id));
			if (false != m_should_terminate)
			{
				LOGDEBUG(wxString::Format("%d pipe: terminating==true, terminating\n", m_id));
				LOGDEBUG(wxString::Format("%d pipe Entry(1.0): releasing pipe lock\n", m_id));
				break;
			}
			LOGDEBUG(wxString::Format("%d pipe Entry(1.1): releasing pipe lock\n", m_id));
		}
		
		if (!processed_something)
		{
			LOGDEBUG(wxString::Format("%d pipe: processed_something==false, Wait\n", m_id));
			m_signal_condition->Wait();
		}

		//2)
		{
			LOGDEBUG(wxString::Format("%d pipe Entry(2): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(2): aquired pipe lock\n", m_id));
			if (false != m_should_terminate)
			{
				LOGDEBUG(wxString::Format("%d pipe: terminating==true, terminating\n", m_id));
				LOGDEBUG(wxString::Format("%d pipe Entry(2.0): releasing pipe lock\n", m_id));
				break;
			}
			LOGDEBUG(wxString::Format("%d pipe Entry(2.1): releasing pipe lock\n", m_id));
		}

		processed_something = false;

		//3)
		{
			LOGDEBUG(wxString::Format("%d pipe Entry(3): aquiring sink_buffer lock\n", m_id));
			wxCriticalSectionLocker sink_locker(m_sink_buffer_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(3): aquired sink_buffer lock\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe Entry(3): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker pipe_locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(3): aquired pipe lock\n", m_id));

			wxSizeT buffer_length = UMIN(m_sink_buffer_len, m_requested_bytes);
			while (0 < buffer_length)
			{
				LOGDEBUG(wxString::Format("%d pipe: giving %d bytes to sink\n", m_id, buffer_length));
				if (ID_OK != (status=m_sink->ReceiveBytes(m_sink_buffer, buffer_length)))
				{
					LOGDEBUG(wxString::Format("%d pipe: giving bytes failed\n", m_id));
					ReportPipeFailure(status);
					continue;
				}
				
				if (0 < buffer_length)
				{
					processed_something = true;
					memmove(m_sink_buffer, m_sink_buffer+buffer_length, buffer_length);
					m_sink_buffer_pos -= buffer_length;
					m_requested_bytes -= buffer_length;

					LOGDEBUG(wxString::Format("%d pipe: moving sink_buffer. len:%d pos:%d req:%d\n", m_id, m_sink_buffer_len, m_sink_buffer_pos, m_requested_bytes));
					buffer_length = UMIN(m_sink_buffer_len, m_requested_bytes);
				}
			}
			LOGDEBUG(wxString::Format("%d pipe Entry(3): releasing sink_buffer lock\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe Entry(3): releasing pipe lock\n", m_id));
		}

		//4)
		{
			LOGDEBUG(wxString::Format("%d pipe Entry(4): aquiring source_buffer lock\n", m_id));
			wxCriticalSectionLocker source_locker(m_source_buffer_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(4): aquired source_buffer lock\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe Entry(4): aquiring sink_buffer lock\n", m_id));
			wxCriticalSectionLocker sink_locker(m_sink_buffer_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(4): aquired sink_buffer lock\n", m_id));

			wxSizeT source_buffer_length = m_source_buffer_pos;
			wxSizeT sink_buffer_length;
			while (0 < source_buffer_length)
			{
				if (0.95 < (static_cast<double>(m_sink_buffer_pos+source_buffer_length) / static_cast<double>(m_sink_buffer_len)))
				{
					wxmailto_status status = GrowSinkBuffer();
					if (ID_OK != status)
					{
						LOGDEBUG(wxString::Format("%d pipe Entry(4.0): releasing source_buffer lock\n", m_id));
						LOGDEBUG(wxString::Format("%d pipe Entry(4.0): releasing sink_buffer lock\n", m_id));
						return (wxThread::ExitCode)status;
					}
				}

				sink_buffer_length = m_sink_buffer_len-m_sink_buffer_pos;

				LOGDEBUG(wxString::Format("%d pipe: processing source_buffer. len:%d sink_pos:%d sink_len:%d\n", m_id, source_buffer_length, m_sink_buffer_pos, sink_buffer_length));

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

					LOGDEBUG(wxString::Format("%d pipe: moving source_buffer. len:%d new source_pos:%d new sink_pos:%d\n", m_id, source_buffer_length, m_source_buffer_pos, m_sink_buffer_pos));
					source_buffer_length = m_source_buffer_pos;
				}
			}
			LOGDEBUG(wxString::Format("%d pipe Entry(4.1): releasing source_buffer lock\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe Entry(4.1): releasing sink_buffer lock\n", m_id));
		}

		//5)
		{
			LOGDEBUG(wxString::Format("%d pipe Entry(5): aquiring source_buffer lock\n", m_id));
			wxCriticalSectionLocker locker(m_source_buffer_lock);
			LOGDEBUG(wxString::Format("%d pipe Entry(5): aquired source_buffer lock\n", m_id));

			if (!Eof() && (m_source_buffer_pos+1)<m_source_buffer_len)
			{
				LOGDEBUG(wxString::Format("%d pipe: requesting %d bytes\n", m_id, m_source_buffer_len-m_source_buffer_pos));
				if (ID_OK != (status=m_source->RequestBytes(m_source_buffer_len-m_source_buffer_pos)))
				{
					LOGDEBUG(wxString::Format("%d pipe: requesting bytes failed\n", m_id));
					ReportPipeFailure(status);
					LOGDEBUG(wxString::Format("%d pipe Entry(5.0): releasing source_buffer lock\n", m_id));
					continue;
				}
			}
			LOGDEBUG(wxString::Format("%d pipe Entry(5.1): releasing source_buffer lock\n", m_id));
		}
	}
	LOGDEBUG(wxString::Format("%d pipe: The End!\n", m_id));
	return (wxThread::ExitCode)0;
}

wxmailto_status Pipe::StartFlow()
{
	LOGDEBUG(wxString::Format("%d pipe: starting flow\n", m_id));
	Pipe* current_pipe = this;
	//Find source
	while (true)
	{
		LOGDEBUG(wxString::Format("%d pipe StartFlow(1): aquiring current_pipe->pipe lock\n", m_id));
		wxCriticalSectionLocker locker(current_pipe->m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe StartFlow(1): aquired current_pipe->pipe lock\n", m_id));
		if (!current_pipe->m_source)
		{
			LOGDEBUG(wxString::Format("%d pipe: found source\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe StartFlow(1.0): releasing current_pipe->pipe lock\n", m_id));
			break;
		}
		
		LOGDEBUG(wxString::Format("%d pipe: updating current_pipe\n", m_id));
		current_pipe = current_pipe->m_source;
		LOGDEBUG(wxString::Format("%d pipe StartFlow(1.1): releasing current_pipe->pipe lock\n", m_id));
	}

	//Start all pipes, from source to sink
	while (current_pipe)
	{
		LOGDEBUG(wxString::Format("%d pipe StartFlow(2): aquiring current_pipe->pipe lock\n", m_id));
		wxCriticalSectionLocker locker(current_pipe->m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe StartFlow(2): aquired current_pipe->pipe lock\n", m_id));
		LOGDEBUG(wxString::Format("%d pipe: current_pipe->Run!\n", m_id));
		current_pipe->Run();
		LOGDEBUG(wxString::Format("%d pipe: updating current_pipe\n", m_id));
		current_pipe = current_pipe->m_sink;
		LOGDEBUG(wxString::Format("%d pipe StartFlow(2): releasing current_pipe->pipe lock\n", m_id));
	}
	return ID_OK;
}

void Pipe::Terminate()
{
	LOGDEBUG(wxString::Format("%d pipe: Terminate\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe Terminate(1): aquiring pipe lock\n", m_id));
		wxCriticalSectionLocker locker(m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe Terminate(1): aquired pipe lock\n", m_id));

		if (m_source)
		{
			m_source->Terminate();
		}

		LOGDEBUG(wxString::Format("%d pipe: m_should_terminate = true\n", m_id));
		m_should_terminate = true;
		LOGDEBUG(wxString::Format("%d pipe Terminate(1): releasing pipe lock\n", m_id));
	}
}

void Pipe::ReportPipeFailure(const wxmailto_status& status, const wxString& msg)
{
	LOGDEBUG(wxString::Format("%d pipe: report pipe failure\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe ReportPipeFailure(1): aquiring pipe lock\n", m_id));
		wxCriticalSectionLocker locker(m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe ReportPipeFailure(1): aquired pipe lock\n", m_id));

		if (m_sink)
		{
			m_source->ReportPipeFailure(status, msg);
		}
		else
		{
			//ToDo, display error message?

			Terminate();
		}
		LOGDEBUG(wxString::Format("%d pipe ReportPipeFailure(1): releasing pipe lock\n", m_id));
	}
}

wxBool Pipe::Eof()
{
	LOGDEBUG(wxString::Format("%d pipe: Eof\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe Eof(1): aquiring source_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_source_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe Eof(1): aquired source_buffer lock\n", m_id));

		if (0 < m_source_buffer_pos)
		{
			LOGDEBUG(wxString::Format("%d pipe: not eof\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe Eof(1.1): releasing source_buffer lock\n", m_id));
			return false;
		}
		LOGDEBUG(wxString::Format("%d pipe Eof(1.1): releasing source_buffer lock\n", m_id));
	}

	{
		LOGDEBUG(wxString::Format("%d pipe Eof(2): aquiring sink_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_sink_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe Eof(2): aquired sink_buffer lock\n", m_id));

		if (0 < m_sink_buffer_pos)
		{
			LOGDEBUG(wxString::Format("%d pipe: not eof\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe Eof(2.0): releasing sink_buffer lock\n", m_id));
			return false;
		}
		LOGDEBUG(wxString::Format("%d pipe Eof(2.1): releasing sink_buffer lock\n", m_id));
	}

	if (!m_source)
	{
		LOGDEBUG(wxString::Format("%d pipe: not eof\n", m_id));
	}

	return m_source ? m_source->Eof() : false;
}

//Notify that this sink_buffer can receive requested_bytes bytes. Should only be called by m_sink
wxmailto_status Pipe::RequestBytes(const wxSizeT& requested_bytes)
{
	LOGDEBUG(wxString::Format("%d pipe: RequestBytes\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe RequestBytes(1): aquiring pipe lock\n", m_id));
		wxCriticalSectionLocker locker(m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe RequestBytes(1): aquired pipe lock\n", m_id));
		
		m_requested_bytes = requested_bytes;
		LOGDEBUG(wxString::Format("%d pipe: new requested bytes: %d\n", m_id, m_requested_bytes));
		LOGDEBUG(wxString::Format("%d pipe RequestBytes(1): releasing pipe lock\n", m_id));
	}

	LOGDEBUG(wxString::Format("%d pipe: signal requested bytes\n", m_id));
	m_signal_condition->Signal();
	return ID_OK;
}

//NOOP-pipe
wxmailto_status Pipe::Process(wxUint8* source_buffer, wxSizeT& source_buffer_len,
                              wxUint8* sink_buffer, wxSizeT& sink_buffer_len)
{
	LOGDEBUG(wxString::Format("%d pipe: Process\n", m_id));
	if (!source_buffer || !sink_buffer)
	{
		LOGDEBUG(wxString::Format("%d pipe: invalid argument\n", m_id));
		return LOGERROR(ID_INVALID_ARGUMENT);
	}

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
	LOGDEBUG(wxString::Format("%d pipe: processed source:%d sink:%d\n", m_id, source_buffer_len, sink_buffer_len));
	return ID_OK;
}

//Read from m_source sink_buffer to this source_buffer. Should only be called by m_source
wxmailto_status Pipe::ReceiveBytes(wxUint8* buffer, wxSizeT& buffer_length)
{
	LOGDEBUG(wxString::Format("%d pipe: ReceiveBytes\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(1): aquiring source_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_source_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(1): aquired source_buffer lock\n", m_id));

		if (0.95 < (static_cast<double>(m_source_buffer_pos+buffer_length) / static_cast<double>(m_source_buffer_len)))
		{
			wxmailto_status status = GrowSourceBuffer();
			if (ID_OK != status)
			{
				LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(1.0): releasing source_buffer lock\n", m_id));
				return status;
			}
		}

		wxSizeT move_length = UMIN(buffer_length, m_source_buffer_len-m_source_buffer_pos);
		if (0 == move_length)
		{
			LOGDEBUG(wxString::Format("%d pipe: nothing to receive\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(1.1): releasing source_buffer lock\n", m_id));
			return ID_OK;
		}

		memcpy(m_source_buffer+m_source_buffer_pos, buffer, move_length);
		m_sink_buffer_pos += move_length;
		buffer_length = move_length;
		LOGDEBUG(wxString::Format("%d pipe: received %d bytes. sink_pos:%d\n", m_id, move_length, m_sink_buffer_pos));
		LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(1.2): releasing source_buffer lock\n", m_id));
	}

	{
		LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(2): aquiring pipe lock\n", m_id));
		wxCriticalSectionLocker locker(m_pipe_lock);
		LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(2): aquired pipe lock\n", m_id));

		m_got_bytes = true;
		LOGDEBUG(wxString::Format("%d pipe ReceiveBytes(2): releasing pipe lock\n", m_id));
	}
	
	LOGDEBUG(wxString::Format("%d pipe: signal received bytes\n", m_id));
	m_signal_condition->Signal();
	return ID_OK;
}

wxmailto_status Pipe::GrowSourceBuffer()
{
	LOGDEBUG(wxString::Format("%d pipe: grow source buffer\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe GrowSourceBuffer(1): aquiring source_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_source_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe GrowSourceBuffer(1): aquired source_buffer lock\n", m_id));
		
		wxSizeT new_buffer_size = UMIN(2*m_source_buffer_len, m_max_source_buffer_len);
		if (new_buffer_size <= m_source_buffer_len)
		{
			LOGDEBUG(wxString::Format("%d pipe: no need to grow. %d < %d\n", m_id, new_buffer_size, m_source_buffer_len));
			LOGDEBUG(wxString::Format("%d pipe GrowSourceBuffer(1.0): releasing source_buffer lock\n", m_id));
			return ID_OK;
		}
		
		wxUint8* new_buffer = new wxUint8[new_buffer_size];
		if (!new_buffer)
		{
			LOGDEBUG(wxString::Format("%d pipe: out of memory\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe GrowSourceBuffer(1.1): releasing source_buffer lock\n", m_id));
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		LOGDEBUG(wxString::Format("%d pipe: growing from %d to %d\n", m_id, m_source_buffer_len, new_buffer_size));
		memcpy(new_buffer, m_source_buffer, m_source_buffer_pos);
		delete[] m_source_buffer;
		m_source_buffer = new_buffer;
		m_source_buffer_len = new_buffer_size;
		LOGDEBUG(wxString::Format("%d pipe GrowSourceBuffer(1.2): releasing source_buffer lock\n", m_id));
	}
	return ID_OK;
}

wxmailto_status Pipe::GrowSinkBuffer()
{
	LOGDEBUG(wxString::Format("%d pipe: grow sink buffer\n", m_id));
	{
		LOGDEBUG(wxString::Format("%d pipe GrowSinkBuffer(1): aquiring sink_buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_sink_buffer_lock);
		LOGDEBUG(wxString::Format("%d pipe GrowSinkBuffer(1): aquired sink_buffer lock\n", m_id));
		
		wxSizeT new_buffer_size = UMIN(2*m_sink_buffer_len, m_max_sink_buffer_len);
		if (new_buffer_size <= m_sink_buffer_len)
		{
			LOGDEBUG(wxString::Format("%d pipe: no need to grow. %d < %d\n", m_id, new_buffer_size, m_sink_buffer_len));
			LOGDEBUG(wxString::Format("%d pipe GrowSinkBuffer(1.0): releasing sink_buffer lock\n", m_id));
			return ID_OK;
		}
		
		wxUint8* new_buffer = new wxUint8[new_buffer_size];
		if (!new_buffer)
		{
			LOGDEBUG(wxString::Format("%d pipe: out of memory\n", m_id));
			LOGDEBUG(wxString::Format("%d pipe GrowSinkBuffer(1.1): releasing sink_buffer lock\n", m_id));
			return LOGERROR(ID_OUT_OF_MEMORY);
		}

		LOGDEBUG(wxString::Format("%d pipe: growing from %d to %d\n", m_id, m_sink_buffer_len, new_buffer_size));
		memcpy(new_buffer, m_sink_buffer, m_sink_buffer_pos);
		delete[] m_sink_buffer;
		m_sink_buffer = new_buffer;
		m_sink_buffer_len = new_buffer_size;
			LOGDEBUG(wxString::Format("%d pipe GrowSinkBuffer(1.2): releasing sink_buffer lock\n", m_id));
	}
	return ID_OK;
}
