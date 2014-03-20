#ifndef _FILE_SINK_H_
#define _FILE_SINK_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "file_sink.h"
#endif

#include "sink.h"


namespace wxMailto
{

class FileSink : public Sink
{
public:
	FileSink(wxInt id, SinkResult* sink_result, const wxString& filename);
	virtual ~FileSink();

protected: //From Sink
	virtual wxmailto_status HandleBytes(wxUint8* buffer,
	                                     wxSizeT& buffer_len); //IN: capacity. OUT: bytes handled

private:
	wxFFile* m_file;
};

}

#endif // _SINK_H_
