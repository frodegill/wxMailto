#ifndef _QP_BUFFEREDSTREAM_H_
#define _QP_BUFFEREDSTREAM_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "qp_bufferedstream.h"
#endif

#include "buffered_streambase.h"


namespace wxMailto
{

static const wxUint32 QP_ENCODE_BINARY  = 1<<0; //Encode CRLF
static const wxUint32 QP_ENCODE_WS      = 1<<1; //Encode SPACE and TAB
static const wxUint32 QP_ENCODE_SPECIAL = 1<<2; //Encode special characters to make it EBCDIC safe


class QPDecodeStream : public BufferedInputStream
{
public:
	QPDecodeStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);

	//From BufferedInputStream
public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof);
};

class QPEncodeStream : public BufferedOutputStream
{
public:
	QPEncodeStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length,
	               wxUint32 mode=0, wxSizeT wrap_col=0, wxSizeT current_col=0);

	//From BufferedOutputStream
public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof);
private:
	wxUint32 m_mode;
	wxSizeT m_wrap_col;
	wxSizeT m_current_col;
};

}

#endif // _QP_BUFFEREDSTREAM_H_
