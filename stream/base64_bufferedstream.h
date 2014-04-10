#ifndef _BASE64_BUFFEREDSTREAM_H_
#define _BASE64_BUFFEREDSTREAM_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "base64_bufferedstream.h"
#endif

#include "buffered_streambase.h"


namespace wxMailto
{

class Base64DecodeStream : public BufferedInputStream
{
public:
	Base64DecodeStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length, wxBase64DecodeMode mode=wxBase64DecodeMode_SkipWS);

	//From BufferedInputStream
public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof);
private:
	wxBase64DecodeMode m_mode;
};

class Base64EncodeStream : public BufferedOutputStream
{
public:
	Base64EncodeStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);

	//From BufferedOutputStream
public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof);
};

}

#endif // _BASE64_BUFFEREDSTREAM_H_
