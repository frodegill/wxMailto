
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

#include "hex_bufferedstream.h"


using namespace wxMailto;


QPDecodeStream::QPDecodeStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length)
: BufferedInputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length)
{
}

void QPDecodeStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
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



QPEncodeStream::QPEncodeStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length,
                               wxUint32 mode)
: BufferedOutputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length),
  m_mode(mode)
{
}

void QPEncodeStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
                             wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
                             wxBool WXUNUSED(eof))
{
	read_bytes = written_bytes = 0;
	if (!src || 0==src_length || !dst || 0==dst_length)
		return;

	wxBool encode;
	wxUint8 c;
	while (written_bytes<dst_length && read_bytes<src_length) //If writing more than one byte, an extra test on dst_length is performed below
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

		if (encode)
		{
			if ((written_bytes+3)>dst_length)
			{
				break; //Not enough room
			}

			dst[written_bytes++] = '=';
			dst[written_bytes++] = TO_HEX[(c&0xF0)>>4];
			dst[written_bytes++] = TO_HEX[c&0x0F];
			read_bytes++;
		}
		else
		{
			dst[written_bytes++] = c;
			read_bytes++;
		}
	}
}


MimeLinewrappedStream::MimeLinewrappedStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length,
                                             wxSizeT wrap_col, wxSizeT current_col, MimeMode mode, wxBool create_mime_encoded_words)
: BufferedOutputStream(stream, max_underflow_buffer_length, max_overflow_buffer_length),
  m_wrap_col(wrap_col),
  m_current_col(current_col),
  m_mode(mode),
  m_create_mime_encoded_words(false!=create_mime_encoded_words),
  m_in_encoded_word(false)
{
}

void MimeLinewrappedStream::Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
                                    wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
                                    wxBool WXUNUSED(eof))
{
	static const wxSizeT WORD_PREFIX_LENGTH = 10; // "=?utf-8?" + [QB] + "?"
	static const wxSizeT WORD_MINIMUM_BODY_LENGTH = 4; //Should have room for at least one Base64-encoded triplet, right?
	static const wxSizeT WORD_POSTFIX_LENGTH = 2; // "?="
	static const wxSizeT WORD_SEPARATOR_LENGTH = 3; // "\r\n "
	static const wxSizeT SOFT_LINEBREAK_LENGTH = 3; // "=\r\n"
	static const wxSizeT SOFT_LINEBREAK_MARKER_LENGTH = 1; // "="

	read_bytes = written_bytes = 0;
	if (!src || 0==src_length || !dst || 0==dst_length)
		return;

	wxUint8 c;
	wxSizeT needed_bytes;
	while (written_bytes<dst_length && read_bytes<src_length) //If writing more than one byte, an extra test on dst_length is performed below
	{
		c = src[read_bytes];
		needed_bytes = (BASE64==m_mode) ? 4 : ('='==c ? 3 : 1);

		//Insert linebreak if needed
		if (m_in_encoded_word && (m_current_col+needed_bytes+WORD_POSTFIX_LENGTH)>m_wrap_col)
		{
			if ((written_bytes+WORD_POSTFIX_LENGTH+WORD_SEPARATOR_LENGTH) > dst_length)
				break;

			dst[written_bytes++] = '?';
			dst[written_bytes++] = '=';
			dst[written_bytes++] = '\r';
			dst[written_bytes++] = '\n';
			dst[written_bytes++] = ' ';
			m_in_encoded_word = false;
			m_current_col = 1;
		}
		if (QP==m_mode && !m_create_mime_encoded_words && (m_current_col+needed_bytes+SOFT_LINEBREAK_MARKER_LENGTH)>m_wrap_col)
		{
			if ((written_bytes+SOFT_LINEBREAK_LENGTH) > dst_length)
				break;

			dst[written_bytes++] = '=';
			dst[written_bytes++] = '\r';
			dst[written_bytes++] = '\n';
			m_current_col = 0;
		}

		//Start a word, inserting a word separator if needed
		if (!m_in_encoded_word && m_create_mime_encoded_words)
		{
			if ((m_current_col+WORD_PREFIX_LENGTH+WORD_MINIMUM_BODY_LENGTH+WORD_POSTFIX_LENGTH) >= m_wrap_col)
			{
			   if ((written_bytes+WORD_SEPARATOR_LENGTH) > dst_length)
					 break;

				dst[written_bytes++] = '\r';
				dst[written_bytes++] = '\n';
				dst[written_bytes++] = ' ';
				m_current_col = 1;
			}

			if ((written_bytes+WORD_PREFIX_LENGTH) > dst_length)
				break;

			memcpy(dst+written_bytes, "=?utf-8?", 8);
			dst[written_bytes+8] = (BASE64==m_mode ? 'B' : 'Q');
			dst[written_bytes+9] = '?';
			written_bytes += WORD_PREFIX_LENGTH;
			m_current_col += WORD_PREFIX_LENGTH;
			m_in_encoded_word = true;
		}

		//We are now ready to copy content
		if (BASE64==m_mode)
		{
			if ((written_bytes+needed_bytes)>dst_length || (read_bytes+needed_bytes)>src_length)
			{
				break; //Could not process a complete Base64-encoded triplet
			}

			dst[written_bytes++] = c;
			read_bytes++;
			dst[written_bytes++] = src[read_bytes++];
			dst[written_bytes++] = src[read_bytes++];
			dst[written_bytes++] = src[read_bytes++];
			m_current_col += 4;
		}
		else //QP==m_mode
		{
			if ((written_bytes+needed_bytes)>dst_length || (read_bytes+needed_bytes)>src_length)
			{
				break; //Could not process the (possibly QP-encoded) character
			}

			dst[written_bytes++] = c;
			read_bytes++;
			if (3==needed_bytes)
			{
				dst[written_bytes++] = src[read_bytes++];
				dst[written_bytes++] = src[read_bytes++];
			}
			m_current_col += needed_bytes;
		}
	}
}
