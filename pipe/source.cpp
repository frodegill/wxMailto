
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "source.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "source.h"


using namespace wxMailto;

Source::Source(wxInt id)
: Pipe(id)
{
	LOGDEBUG(wxString::Format("%d source: ctor\n", m_id));
}

Source::~Source()
{
	LOGDEBUG(wxString::Format("%d source: dtor\n", m_id));
}

wxThread::ExitCode Source::Entry()
{
	LOGDEBUG(wxString::Format("%d source: Entry()\n", m_id));
	//1) sleep until terminate or sink wants sink_buffer bytes
	//2) if not terminate
	//3)  if sink wants bytes and we have sink_buffer bytes, move sink_buffer bytes to sink
	//4)  if not eof and we have room for sink_buffer bytes, read into sink_buffer
	//5)  if 3 or 4 did something, goto 2
	//6) goto 1

	wxmailto_status status;
	wxBool processed_something = true; //true initially, to get things rolling..
	while (true)
	{
		//1)
		{
			LOGDEBUG(wxString::Format("%d source Entry(1): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d source Entry(1): aquired pipe lock\n", m_id));
			if (false != m_should_terminate)
			{
				LOGDEBUG(wxString::Format("%d source: terminating==true, terminating\n", m_id));
				LOGDEBUG(wxString::Format("%d source Entry(1.0): releasing pipe lock\n", m_id));
				break;
			}
			LOGDEBUG(wxString::Format("%d source Entry(1.1): releasing pipe lock\n", m_id));
		}
		
		if (!processed_something)
		{
			LOGDEBUG(wxString::Format("%d source: processed_something==false, Wait\n", m_id));
			m_signal_condition->Wait();
		}

		//2)
		{
			LOGDEBUG(wxString::Format("%d source Entry(2): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d source Entry(2): aquired pipe lock\n", m_id));
			if (false != m_should_terminate)
			{
				LOGDEBUG(wxString::Format("%d source: terminating==true, terminating\n", m_id));
				LOGDEBUG(wxString::Format("%d source Entry(2.0): releasing pipe lock\n", m_id));
				break;
			}
			LOGDEBUG(wxString::Format("%d source Entry(2.1): releasing pipe lock\n", m_id));
		}

		processed_something = false;

		//3)
		{
			LOGDEBUG(wxString::Format("%d source Entry(3): aquiring sink_buffer lock\n", m_id));
			wxCriticalSectionLocker sink_locker(m_sink_buffer_lock);
			LOGDEBUG(wxString::Format("%d source Entry(3): aquired sink_buffer lock\n", m_id));
			LOGDEBUG(wxString::Format("%d source Entry(3): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker pipe_locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d source Entry(3): aquired pipe lock\n", m_id));

			wxSizeT buffer_length = UMIN(m_sink_buffer_len, m_requested_bytes);
			while (0 < buffer_length)
			{
				LOGDEBUG(wxString::Format("%d source: giving %d bytes to sink\n", m_id, buffer_length));
				if (ID_OK != (status=m_sink->ReceiveBytes(m_sink_buffer, buffer_length)))
				{
					LOGDEBUG(wxString::Format("%d source: giving bytes failed\n", m_id));
					ReportPipeFailure(status);
					continue;
				}
				
				if (0 < buffer_length)
				{
					processed_something = true;
					memmove(m_sink_buffer, m_sink_buffer+buffer_length, buffer_length);
					m_sink_buffer_pos -= buffer_length;
					m_requested_bytes -= buffer_length;

					LOGDEBUG(wxString::Format("%d source: moving buffer. len:%d pos:%d req:%d\n", m_id, m_sink_buffer_len, m_sink_buffer_pos, m_requested_bytes));
					buffer_length = UMIN(m_sink_buffer_len, m_requested_bytes);
				}
			}
			LOGDEBUG(wxString::Format("%d source Entry(3): releasing sink_buffer lock\n", m_id));
			LOGDEBUG(wxString::Format("%d source Entry(3): releasing pipe lock\n", m_id));
		}

		//5)
		{
			LOGDEBUG(wxString::Format("%d source Entry(4): aquiring sink_buffer lock\n", m_id));
			wxCriticalSectionLocker locker(m_sink_buffer_lock);
			LOGDEBUG(wxString::Format("%d source Entry(4): aquired sink_buffer lock\n", m_id));

			if (!Eof() && (m_sink_buffer_pos+1)<m_sink_buffer_len)
			{
				wxSizeT buffer_length = m_sink_buffer_len-m_sink_buffer_pos;
				LOGDEBUG(wxString::Format("%d source: trying to provide %d bytes\n", m_id, buffer_length));
				if (ID_OK != (status=ProvideBytes(m_sink_buffer+m_sink_buffer_pos, buffer_length)))
				{
					LOGDEBUG(wxString::Format("%d source: providing bytes failed\n", m_id));
					ReportPipeFailure(status);
					LOGDEBUG(wxString::Format("%d source Entry(4.0): releasing sink_buffer lock\n", m_id));
					continue;
				}
				processed_something |= (0 < buffer_length);
			}
			LOGDEBUG(wxString::Format("%d source Entry(4.1): releasing sink_buffer lock\n", m_id));
		}
		LOGDEBUG(wxString::Format("%d source: end of loop. Prosessed_something:%d\n", m_id, processed_something));
	}
	LOGDEBUG(wxString::Format("%d source: The End!\n", m_id));
	return (wxThread::ExitCode)0;
}
