#ifndef _SOURCE_H_
#define _SOURCE_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "source.h"
#endif

#include "../gui/wxmailto_app.h"
#include "pipe.h"


namespace wxMailto
{

class Source : public Pipe
{
public:
	Source(wxInt id);
	virtual ~Source();

protected: //From Pipe
	virtual wxThread::ExitCode Entry();

protected:
	virtual wxmailto_status ProvideBytes(wxUint8* buffer,
	                                     wxSizeT& buffer_len) = 0; //IN: capacity. OUT: bytes written
};

}

#endif // _SOURCE_H_
