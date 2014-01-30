
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "sink.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "sink.h"
#include "../gui/wxmailto_app.h"


using namespace wxMailto;


SinkResult::SinkResult()
: m_should_terminate(false)
{
	m_signal_lock.Lock();
	m_signal_condition = new wxCondition(m_signal_lock);
}

SinkResult::~SinkResult()
{
	if (m_signal_condition)
	{
		Signal();
		delete m_signal_condition;
		m_signal_condition = NULL;
	}
}

wxmailto_status SinkResult::Wait()
{
	{
		wxMutexLocker locker(m_signal_lock);
		if (!m_should_terminate)
		{
			m_signal_condition->Wait();
		}
	}
	return m_status;
}

void SinkResult::Signal()
{
	{
		wxMutexLocker locker(m_signal_lock);
		m_should_terminate = true;
		m_signal_condition->Signal();
	}
}



Sink::Sink(SinkResult* sink_result)
: Pipe(),
  m_sink_result(sink_result)
{
}

Sink::~Sink()
{
	if (m_sink_result)
	{
		m_sink_result->SetStatus(ID_OK);
		m_sink_result->Signal();
	}
}

wxThread::ExitCode Sink::Entry()
{
	
	//1) sleep until terminate or source provides sink_buffer bytes
	//2) if not terminate
	//3)  while we got sink_buffer bytes, handle sink_buffer bytes
	//4)  if not eof and we have room for source_buffer bytes, request source_buffer bytes
	//5) goto 1

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
			wxCriticalSectionLocker sink_locker(m_source_buffer_lock);

			wxSizeT buffer_length = m_source_buffer_pos;
			while (0 < buffer_length)
			{
				if (ID_OK != (status=HandleBytes(m_source_buffer, buffer_length)))
				{
					ReportPipeFailure(status);
					continue;
				}
				
				if (0 < buffer_length)
				{
					processed_something = true;
					memmove(m_source_buffer, m_source_buffer+buffer_length, buffer_length);
					m_source_buffer_pos -= buffer_length;

					buffer_length = m_source_buffer_pos;
				}
			}
		}

		//4)
		{
			wxCriticalSectionLocker locker(m_source_buffer_lock);

			if (!Eof() && (m_sink_buffer_pos+1)<m_sink_buffer_len)
			{
				wxSizeT buffer_length = m_sink_buffer_len-m_sink_buffer_pos;
				if (ID_OK != (status=m_source->RequestBytes(buffer_length)))
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
