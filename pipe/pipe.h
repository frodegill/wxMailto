#ifndef _PIPE_H_
#define _PIPE_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "pipe.h"
#endif

#include <wx/thread.h>

#include "../wxmailto_errors.h"


namespace wxMailto
{

class Pipe : public wxThread
{
friend class Source;
friend class Sink;

public:
	Pipe(wxInt id);
	virtual ~Pipe();

	wxmailto_status InitializeSourceBuffer(const wxSizeT& initial_size=8*1024, const wxSizeT& max_size=0);
	wxmailto_status InitializeSinkBuffer(const wxSizeT& initial_size=8*1024, const wxSizeT& max_size=0);

	Pipe* ConnectTo(Pipe* sink);

protected: //From wxThread
	virtual wxThread::ExitCode Entry();

public:
	virtual wxmailto_status StartFlow();
	virtual void Terminate();
	virtual void ReportPipeFailure(const wxmailto_status& status, const wxString& msg = wxEmptyString);

public:
	virtual wxBool Eof();

protected:
	wxmailto_status RequestBytes(const wxSizeT& requested_bytes); 

	virtual wxmailto_status Process(wxUint8* source_buffer,
	                                wxSizeT& source_buffer_len, //IN: capacity. OUT: bytes read
	                                wxUint8* sink_buffer,
	                                wxSizeT& sink_buffer_len); //IN: capacity. OUT: bytes written
	
	wxmailto_status ReceiveBytes(wxUint8* buffer,
	                             wxSizeT& buffer_length); //IN: capacity. OUT: bytes written

private:
	wxmailto_status GrowSourceBuffer();
	wxmailto_status GrowSinkBuffer();

protected:
	wxInt m_id;

private:
	wxMutex m_signal_lock;
	wxCondition* m_signal_condition;
		
	wxCriticalSection m_pipe_lock;
	wxSizeT m_requested_bytes;
	wxUInt m_got_bytes:1;
	wxUInt m_should_terminate:1;

	Pipe* m_source;
	wxCriticalSection m_source_buffer_lock;
	wxUint8* m_source_buffer;
	wxSizeT m_source_buffer_len;
	wxSizeT m_source_buffer_pos;
	wxSizeT m_max_source_buffer_len;

	Pipe* m_sink;
	wxCriticalSection m_sink_buffer_lock;
	wxUint8* m_sink_buffer;
	wxSizeT m_sink_buffer_len;
	wxSizeT m_sink_buffer_pos;
	wxSizeT m_max_sink_buffer_len;
};

}

#endif // _PIPE_H_
