
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

Source::Source()
: Pipe()
{
}

Source::~Source()
{
}

wxThread::ExitCode Source::Entry()
{
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

		//5)
		{
			wxCriticalSectionLocker locker(m_sink_buffer_lock);

			if (!Eof() && (m_sink_buffer_pos+1)<m_sink_buffer_len)
			{
				wxSizeT buffer_length = m_sink_buffer_len-m_sink_buffer_pos;
				if (ID_OK != (status=ProvideBytes(m_sink_buffer+m_sink_buffer_pos, buffer_length)))
				{
					ReportPipeFailure(status);
					continue;
				}
				processed_something |= (0 < buffer_length);
			}
		}
	}
	return (wxThread::ExitCode)0;
}
