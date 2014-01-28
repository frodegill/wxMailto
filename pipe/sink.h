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

class Sink : public Pipe
{
public:
	Sink();
	virtual ~Sink();

protected: //From Pipe
	virtual wxThread::ExitCode Entry();

protected:
	virtual wxmailto_status HandleBytes(wxUint8* buffer,
	                                     wxSizeT& buffer_len) = 0; //IN: capacity. OUT: bytes handled
};

}

#endif // _SINK_H_
