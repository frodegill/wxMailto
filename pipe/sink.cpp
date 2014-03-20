
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


SinkResult::SinkResult(wxInt id)
: m_id(id),
  m_should_terminate(false)
{
	LOGDEBUG(wxString::Format("%d sinkresult: ctor\n", m_id));
	m_signal_lock.Lock();
	m_signal_condition = new wxCondition(m_signal_lock);
	LOGDEBUG(wxString::Format("%d sinkresult: ctor condition active\n", m_id));
}

SinkResult::~SinkResult()
{
	LOGDEBUG(wxString::Format("%d sinkresult: dtor\n", m_id));
	if (m_signal_condition)
	{
		LOGDEBUG(wxString::Format("%d sinkresult: dtor signal\n", m_id));
		Signal();
		LOGDEBUG(wxString::Format("%d sinkresult: dtor signal deactivated\n", m_id));
		delete m_signal_condition;
		m_signal_condition = NULL;
	}
}

wxmailto_status SinkResult::Wait()
{
	{
		LOGDEBUG(wxString::Format("%d sinkresult Wait(1): aquiring signal lock\n", m_id));
		wxMutexLocker locker(m_signal_lock);
		LOGDEBUG(wxString::Format("%d sinkresult Wait(1): aquiried signal lock\n", m_id));
		if (!m_should_terminate)
		{
			LOGDEBUG(wxString::Format("%d sinkresult: wait for signal\n", m_id));
			m_signal_condition->Wait();
			LOGDEBUG(wxString::Format("%d sinkresult: signalled!\n", m_id));
		}
		LOGDEBUG(wxString::Format("%d sinkresult Wait(1): releasing signal lock\n", m_id));
	}
	return m_status;
}

void SinkResult::Signal()
{
	{
		LOGDEBUG(wxString::Format("%d sinkresult Signal(1): aquiring signal lock\n", m_id));
		wxMutexLocker locker(m_signal_lock);
		LOGDEBUG(wxString::Format("%d sinkresult Signal(1): aquired signal lock\n", m_id));
		m_should_terminate = true;
		LOGDEBUG(wxString::Format("%d sinkresult: signalling!\n", m_id));
		m_signal_condition->Signal();
		LOGDEBUG(wxString::Format("%d sinkresult: has signalled\n", m_id));
		LOGDEBUG(wxString::Format("%d sinkresult Signal(1): releasing signal lock\n", m_id));
	}
}



Sink::Sink(wxInt id, SinkResult* sink_result)
: Pipe(id),
  m_sink_result(sink_result)
{
	LOGDEBUG(wxString::Format("%d sink: ctor\n", m_id));
}

Sink::~Sink()
{
	LOGDEBUG(wxString::Format("%d sink: dtor\n", m_id));
	if (m_sink_result)
	{
		m_sink_result->SetStatus(ID_OK);
		LOGDEBUG(wxString::Format("%d sink: dtor signal ok\n", m_id));
		m_sink_result->Signal();
	}
}

wxThread::ExitCode Sink::Entry()
{
	LOGDEBUG(wxString::Format("%d sink: Entry()\n", m_id));
	
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
			LOGDEBUG(wxString::Format("%d sink Entry(1): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d sink Entry(1): aquired pipe lock\n", m_id));
			if (false != m_should_terminate)
			{
				LOGDEBUG(wxString::Format("%d sink: should terminate\n", m_id));
				break;
			}
			LOGDEBUG(wxString::Format("%d sink Entry(1): releasing pipe lock\n", m_id));
		}
		
		if (!processed_something)
		{
			LOGDEBUG(wxString::Format("%d sink: prepare to wait\n", m_id));
			m_signal_condition->Wait();
			LOGDEBUG(wxString::Format("%d sink: waited\n", m_id));
		}

		//2)
		{
			LOGDEBUG(wxString::Format("%d sink Entry(2): aquiring pipe lock\n", m_id));
			wxCriticalSectionLocker locker(m_pipe_lock);
			LOGDEBUG(wxString::Format("%d sink Entry(2): aquired pipe lock\n", m_id));
			if (false != m_should_terminate)
			{
				LOGDEBUG(wxString::Format("%d sink: should terminate 2\n", m_id));
				LOGDEBUG(wxString::Format("%d sink Entry(2.0): releasing pipe lock\n", m_id));
				break;
			}
			LOGDEBUG(wxString::Format("%d sink Entry(2.1): releasing pipe lock\n", m_id));
		}

		processed_something = false;

		//3)
		{
			LOGDEBUG(wxString::Format("%d sink Entry(3): aquiring source_buffer lock\n", m_id));
			wxCriticalSectionLocker source_locker(m_source_buffer_lock);
			LOGDEBUG(wxString::Format("%d sink Entry(3): aquired source_buffer lock\n", m_id));

			wxSizeT buffer_length = m_source_buffer_pos;
			while (0 < buffer_length)
			{
				LOGDEBUG(wxString::Format("%d sink: handle %d bytes\n", m_id));
				if (ID_OK != (status=HandleBytes(m_source_buffer, buffer_length)))
				{
					LOGDEBUG(wxString::Format("%d sink: handling bytes failed\n", m_id));
					ReportPipeFailure(status);
					continue;
				}
				
				if (0 < buffer_length)
				{
					processed_something = true;
					memmove(m_source_buffer, m_source_buffer+buffer_length, buffer_length);
					m_source_buffer_pos -= buffer_length;

					LOGDEBUG(wxString::Format("%d sink: moving buffer. len:%d pos:%d\n", m_id, m_sink_buffer_len, m_sink_buffer_pos));
					buffer_length = m_source_buffer_pos;
				}
			}
			LOGDEBUG(wxString::Format("%d sink Entry(3): releasing source_buffer lock\n", m_id));
		}

		//4)
		{
			LOGDEBUG(wxString::Format("%d sink Entry(4): aquiring source_buffer lock\n", m_id));
			wxCriticalSectionLocker locker(m_source_buffer_lock);
			LOGDEBUG(wxString::Format("%d sink Entry(4): aquired source_buffer lock\n", m_id));

			if (!Eof() && (m_source_buffer_pos+1)<m_source_buffer_len)
			{
				wxSizeT buffer_length = m_source_buffer_len-m_source_buffer_pos;
				LOGDEBUG(wxString::Format("%d sink: request %d bytes\n", m_id, buffer_length));
				if (ID_OK != (status=m_source->RequestBytes(buffer_length)))
				{
					LOGDEBUG(wxString::Format("%d sink: requesting bytes failed\n", m_id));
					ReportPipeFailure(status);
					LOGDEBUG(wxString::Format("%d sink Entry(4.0): releasing source_buffer lock\n", m_id));
					continue;
				}
				processed_something |= (0 < buffer_length);
			}
			LOGDEBUG(wxString::Format("%d sink Entry(4.0): releasing source_buffer lock\n", m_id));
		}
		LOGDEBUG(wxString::Format("%d sink: end of loop. Prosessed_something:%d\n", m_id, processed_something));
	}
	LOGDEBUG(wxString::Format("%d sink: The End!\n", m_id));
	return (wxThread::ExitCode)0;
}
