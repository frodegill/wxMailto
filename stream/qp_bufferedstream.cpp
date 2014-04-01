
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "qp_bufferedstream.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "qp_bufferedstream.h"


using namespace wxMailto;

static const wxUint8 FROM_HEX[] =
 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //0x00-0x0F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //0x10-0x1F	 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x20-0x2F
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	0xFF, //0x30-0x3F
  0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x40-0x4F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x50-0x5F
  0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x60-0x6F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x70-0x7F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x80-0x8F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0x90-0x9F
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0xA0-0xAF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0xB0-0xBF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0xC0-0xCF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0xD0-0xDF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//0xE0-0xEF
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};//0xF0-0xFF

static const wxUint8* TO_HEX = reinterpret_cast<const wxUint8*>("0123456789ABCDEF");


QPInputStream::QPInputStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length)
: BufferedInputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length)
{
}

void QPInputStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
                                wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
                                wxBool eof)
{
	read_bytes = written_bytes = 0;
	if (!src || 0==src_length || !dst || 0==dst_length)
		return;

	wxUint8 c;
	wxUint8 ms_nibble, ls_nibble;
	while (written_bytes<dst_length && read_bytes<src_length)
	{
		c = src[read_bytes];

		if ('='==c && (read_bytes+3)>=src_length)
		{
			if (eof) { //If we're at the eof, consider this a robust handling of an illegal quote-pair
				dst[written_bytes++] = c;
				read_bytes++;
				continue;
			}
			else
			{
				break; //We have a quote-pair (=XX) or soft line-wrap (=\r\n), but not all the bytes of it
			}
		}

		if ('='==c && ('\r'==src[read_bytes+1] || '\n'==src[read_bytes+1]))
		{
			if ('\r'==src[read_bytes+1] && '\n'==src[read_bytes+2])
			{
				read_bytes += 3; //Skip Soft Line Break CRLF
			}
			else
			{
				read_bytes += 2; //Skip Soft Line Break CR or LF
			}
			continue;
		}
		
		if ((c<32 && '\t'!=c && '\r'!=c && '\n'!=c) || 126<c)
		{
			read_bytes++; //Simply skip character that should not exist in quote-printable data
			continue;
		}
		
		if ('=' != c) { //Normal character. Copy and move on
			dst[written_bytes++] = c;
			read_bytes++;
			continue;
		}
		else
		{
			ms_nibble = FROM_HEX[src[read_bytes+1]];
			ls_nibble = FROM_HEX[src[read_bytes+2]];
			if (0x0F<ms_nibble || 0x0F<ls_nibble) //Invalid Quote-Pair. RFC2045 suggests to include the "=" character and the following character in the decoded data without any transformation
			{
				dst[written_bytes++] = '=';
				dst[written_bytes++] = src[read_bytes+1];
				read_bytes += 2;
			}
			else
			{
				dst[written_bytes++] = (ms_nibble<<4)|ls_nibble;
				read_bytes += 3;
			}
		}
	}
}



QPOutputStream::QPOutputStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length,
                               wxUint32 mode, wxSizeT wrap_col, wxSizeT current_col)
: BufferedOutputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length),
  m_mode(mode),
  m_wrap_col(wrap_col),
  m_current_col(current_col)
{
}

void QPOutputStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
                                 wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
                                 wxBool eof)
{
	read_bytes = written_bytes = 0;
	if (!src || 0==src_length || !dst || 0==dst_length)
		return;

	wxBool encode;
	wxUint8 c;
	while (written_bytes<dst_length && read_bytes<src_length)
	{
		c = src[read_bytes];

		if ('\r'==c || '\n'==c)
		{
			encode = 0!=(m_mode&QP_ENCODE_BINARY);
		}
		else if ('\t'==c || ' '==c)
		{
			encode = 0!=(m_mode&QP_ENCODE_WS);
		}
		else if ('!'==c || '"'==c || '#'==c || '$'==c || '@'==c || '['==c || '\\'==c ||
		         ']'==c || '^'==c || '`'==c || '{'==c || '|'==c || '}'==c || '~'==c)
		{
			encode = 0!=(m_mode&QP_ENCODE_SPECIAL);
		}
		else
		{
			encode = (33>c || 126<c || 61==c);
		}
		
		if (encode && (written_bytes+3)<dst_length)
		{
			break; //Not enough room
		}

		if (encode)
		{
			dst[written_bytes++] = '=';
			dst[written_bytes++] = TO_HEX[(c&0xF0)>>4];
			dst[written_bytes++] = TO_HEX[c&0x0F];
		}
		else
		{
			dst[written_bytes++] = c;
		}
	}
	
	wxSizeT aligned_src_length = eof ? src_length : (src_length - src_length%3);
	wxSizeT max_decoded_length = UMIN(aligned_src_length, wxBase64DecodedSize(dst_length));
	
	written_bytes = wxBase64Encode(reinterpret_cast<char*>(dst), dst_length, src, max_decoded_length);
	read_bytes = max_decoded_length;
}
