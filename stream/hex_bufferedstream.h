#ifndef _HEX_BUFFEREDSTREAM_H_
#define _HEX_BUFFEREDSTREAM_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "hex_bufferedstream.h"
#endif

#include "buffered_streambase.h"


extern const wxUint8 FROM_HEX[];
extern const wxUint8 TO_HEX[];

namespace wxMailto
{

class HexDecodeStream : public BufferedInputStream
{
public:
	HexDecodeStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);

	//From BufferedInputStream
public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof);
};

class HexEncodeStream : public BufferedOutputStream
{
public:
	HexEncodeStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);

	//From BufferedOutputStream
public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof);
};

}

#endif // _HEX_BUFFEREDSTREAM_H_
