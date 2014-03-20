
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "base64_bufferedstream.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "base64_bufferedstream.h"


using namespace wxMailto;


Base64InputStream::Base64InputStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length, wxBase64DecodeMode mode)
: BufferedInputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length),
  m_mode(mode)
{
}

void Base64InputStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
                                wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
                                wxBool eof)
{
	read_bytes = written_bytes = 0;
	if (!src || 0==src_length || !dst || 0==dst_length)
		return;

	wxSizeT aligned_src_length = eof ? src_length : (src_length - src_length%4);
	wxSizeT max_encoded_length = UMIN(aligned_src_length, wxBase64EncodedSize(dst_length));
	
	written_bytes = wxBase64Decode(dst, dst_length, reinterpret_cast<const char*>(src), max_encoded_length, m_mode);
	read_bytes = max_encoded_length;
}

Base64OutputStream::Base64OutputStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length)
: BufferedOutputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length)
{
}

void Base64OutputStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
                                 wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
                                 wxBool eof)
{
	read_bytes = written_bytes = 0;
	if (!src || 0==src_length || !dst || 0==dst_length)
		return;

	wxSizeT aligned_src_length = eof ? src_length : (src_length - src_length%3);
	wxSizeT max_decoded_length = UMIN(aligned_src_length, wxBase64DecodedSize(dst_length));
	
	written_bytes = wxBase64Encode(reinterpret_cast<char*>(dst), dst_length, src, max_decoded_length);
	read_bytes = max_decoded_length;
}
