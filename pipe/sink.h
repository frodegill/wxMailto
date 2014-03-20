#ifndef _SINK_H_
#define _SINK_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "sink.h"
#endif

#include "../gui/wxmailto_app.h"
#include "pipe.h"


namespace wxMailto
{

class SinkResult {

public:
	SinkResult(wxInt id);
	~SinkResult();

	wxmailto_status Wait();

	void SetStatus(const wxmailto_status& status) {m_status=status;}
	void Signal();

private:
	wxInt m_id;

	wxMutex m_signal_lock;
	wxCondition* m_signal_condition;
	wxmailto_status m_status;
	wxUInt m_should_terminate:1;
};


class Sink : public Pipe
{
public:
	Sink(wxInt id, SinkResult* sink_result);
	virtual ~Sink();

protected: //From Pipe
	virtual wxThread::ExitCode Entry();

protected:
	virtual wxmailto_status HandleBytes(wxUint8* buffer,
	                                     wxSizeT& buffer_len) = 0; //IN: capacity. OUT: bytes handled

private:
	SinkResult* m_sink_result;
};

}

#endif // _SINK_H_
