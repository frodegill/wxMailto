#ifndef _SINK_H_
#define _SINK_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "sink.h"
#endif

#include "pipe.h"


namespace wxMailto
{

class Sink : public Pipe
{
public:
	Sink();
	virtual ~Sink();

public:
	virtual wxmailto_status SetSink(Pipe* sink);
};

}

#endif // _SINK_H_
