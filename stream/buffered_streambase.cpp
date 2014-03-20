
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "buffered_streambase.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "buffered_streambase.h"


using namespace wxMailto;


BufferedStreamBase::BufferedStreamBase(wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length)
: m_underflow_buffer(NULL),
	m_max_underflow_buffer_length(max_underflow_buffer_length),
	m_underflow_buffer_length(0),
	m_underflow_buffer_used_length(0),
	m_overflow_buffer(NULL),
	m_max_overflow_buffer_length(max_overflow_buffer_length),
	m_overflow_buffer_length(0),
	m_overflow_buffer_used_length(0)
{
	m_underflow_buffer_length = m_max_underflow_buffer_length;
	m_underflow_buffer = new wxUint8[m_underflow_buffer_length];

	m_overflow_buffer_length = m_max_overflow_buffer_length;
	m_overflow_buffer = new wxUint8[m_overflow_buffer_length];
}

BufferedStreamBase::~BufferedStreamBase()
{
	delete[] m_underflow_buffer;
	delete[] m_overflow_buffer;
}

/********************************/

BufferedInputStream::BufferedInputStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length)
: BufferedStreamBase(max_underflow_buffer_length, max_overflow_buffer_length),
  wxInputStream(),
  m_stream(stream),
  m_eof(false)
{
}

BufferedInputStream::~BufferedInputStream()
{
	wxASSERT_MSG(0==m_underflow_buffer_used_length && 0==m_overflow_buffer_used_length,
							 _("Remember to call Close() before dtor"));
}

bool BufferedInputStream::Eof() const
{
	return m_eof || wxInputStream::Eof();
}

// m_related_stream (-> m_underflow_buffer)-> Process (-> m_overflow_buffer) -> buffer
size_t BufferedInputStream::OnSysRead(void* buffer, size_t size)
{
	if (!m_stream || !buffer || 0==size) //This should not happen
	{
		m_lasterror = wxSTREAM_READ_ERROR;
		return 0;
	}

	wxSizeT remaining_bytes = size;

// 1) if m_overflow_buffer contains data, write m_overflow_buffer to buffer
// 2) if m_underflow_buffer has room for data, fill m_underflow_buffer from m_related_stream
// 3) if m_underflow_buffer contains data and m_overflow_buffer has room for data, process m_underflow_buffer to m_overflow_buffer
// 4) If noop 1, noop 2, noop 3, Goto 6
// 5) While more buffer, Goto 1
// 6) return

	wxBool processed_something;
	do {
		processed_something = false;

// 1) if m_overflow_buffer contains data, write m_overflow_buffer to buffer
		if (0<remaining_bytes && 0<m_overflow_buffer_used_length)
		{
			wxSizeT last_written = UMIN(remaining_bytes, m_overflow_buffer_used_length);
			memcpy(buffer, m_overflow_buffer, last_written);
			processed_something |= (0 < last_written);

			remaining_bytes -= last_written;
			m_overflow_buffer_used_length -= last_written;
			if (0 < m_overflow_buffer_used_length) //Couldn't flush everything
			{
				memmove(m_overflow_buffer, m_overflow_buffer+last_written, m_overflow_buffer_used_length);
			}
		}

// 2) if m_underflow_buffer has room for data, fill m_underflow_buffer from m_related_stream
		if (m_underflow_buffer_used_length < m_underflow_buffer_length)
		{
			wxSizeT last_read = m_stream->Read(m_underflow_buffer+m_underflow_buffer_used_length, m_underflow_buffer_length-m_underflow_buffer_used_length).LastRead();
			processed_something |= (0 < last_read);

			m_underflow_buffer_used_length += last_read;
		}

// 3) if m_underflow_buffer contains data and m_overflow_buffer has room for data, process m_underflow_buffer to m_overflow_buffer
		if (0<m_underflow_buffer_used_length && m_overflow_buffer_used_length<m_overflow_buffer_length)
		{
			wxSizeT last_read;
			wxSizeT last_written;
			Process(m_underflow_buffer, m_underflow_buffer_used_length, last_read,
			        m_overflow_buffer+m_overflow_buffer_used_length, m_overflow_buffer_length-m_overflow_buffer_used_length, last_written,
			        Eof());
			
			if (0<last_read || 0<last_written)
			{
				processed_something = true;

				m_underflow_buffer_used_length -= last_read;
				m_overflow_buffer_used_length += last_written;
				if (0<last_read && 0<m_underflow_buffer_used_length) //Couldn't process everything
				{
					memmove(m_underflow_buffer, m_underflow_buffer+last_read, m_underflow_buffer_used_length);
				}
			}
		}
	} while (processed_something && wxSTREAM_NO_ERROR==GetLastError());

	return size - remaining_bytes;
}

wxBool BufferedInputStream::Close()
{
	m_eof = true;
	if (0<m_underflow_buffer_used_length || 0<m_overflow_buffer_used_length)
	{
		OnSysRead(NULL, 0);
	}
	return (0==m_underflow_buffer_used_length && 0==m_overflow_buffer_used_length);
}




BufferedOutputStream::BufferedOutputStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length)
: BufferedStreamBase(max_underflow_buffer_length, max_overflow_buffer_length),
  wxOutputStream(),
  m_stream(stream),
  m_eof(false)
{
}

BufferedOutputStream::~BufferedOutputStream()
{
	wxASSERT_MSG(0==m_underflow_buffer_used_length && 0==m_overflow_buffer_used_length,
							 _("Remember to call Close() before dtor"));
}

bool BufferedOutputStream::Close()
{
	m_eof = true;
	if (0<m_underflow_buffer_used_length || 0<m_overflow_buffer_used_length)
	{
		OnSysWrite(NULL, 0);
		if (0<m_underflow_buffer_used_length || 0<m_overflow_buffer_used_length)
		{
			return false;
		}
	}
	return wxOutputStream::Close();
}

// buffer (-> m_underflow_buffer)-> Process (-> m_overflow_buffer) -> m_related_stream
size_t BufferedOutputStream::OnSysWrite(const void* buffer, size_t size)
{
	if (!m_stream) //This should not happen
	{
		m_lasterror = wxSTREAM_WRITE_ERROR;
		return 0;
	}

	const wxUint8* buffer_ptr = static_cast<const wxUint8*>(buffer);
	wxSizeT remaining_bytes = size;

// 1) if m_overflow_buffer contains data, write m_overflow_buffer to m_related_stream
// 2) if m_underflow_buffer has room for data, fill m_underflow_buffer from buffer
// 3) if m_underflow_buffer contains data and m_overflow_buffer has room for data, process m_underflow_buffer to m_overflow_buffer
// 4) If noop 1, noop 2, noop 3, Goto 6
// 5) While more buffer, Goto 1
// 6) return

	wxBool processed_something;
	do {
		processed_something = false;

// 1) if m_overflow_buffer contains data, write m_overflow_buffer to m_related_stream
		if (0 < m_overflow_buffer_used_length)
		{
			wxSizeT last_write = m_stream->Write(m_overflow_buffer, m_overflow_buffer_used_length).LastWrite();
			processed_something |= (0 < last_write);

			m_overflow_buffer_used_length -= last_write;
			if (0 < m_overflow_buffer_used_length) //Couldn't flush everything
			{
				memmove(m_overflow_buffer, m_overflow_buffer+last_write, m_overflow_buffer_used_length);
			}
		}

// 2) if m_underflow_buffer has room for data, fill m_underflow_buffer from buffer
		if (0<remaining_bytes && m_underflow_buffer_used_length<m_underflow_buffer_length)
		{
			wxSizeT last_read = UMIN(remaining_bytes, m_underflow_buffer_length-m_underflow_buffer_used_length);
			memcpy(m_underflow_buffer+m_underflow_buffer_used_length, buffer_ptr+(size-remaining_bytes), last_read);
			processed_something |= (0 < last_read);

			m_underflow_buffer_used_length += last_read;
			remaining_bytes -= last_read;
		}

// 3) if m_underflow_buffer contains data and m_overflow_buffer has room for data, process m_underflow_buffer to m_overflow_buffer
		if (0<m_underflow_buffer_used_length && m_overflow_buffer_used_length<m_overflow_buffer_length)
		{
			wxSizeT last_read;
			wxSizeT last_written;
			Process(m_underflow_buffer, m_underflow_buffer_used_length, last_read,
			        m_overflow_buffer+m_overflow_buffer_used_length, m_overflow_buffer_length-m_overflow_buffer_used_length, last_written,
			        m_eof);
			
			if (0<last_read || 0<last_written)
			{
				processed_something = true;

				m_underflow_buffer_used_length -= last_read;
				m_overflow_buffer_used_length += last_written;
				if (0<last_read && 0<m_underflow_buffer_used_length) //Couldn't process everything
				{
					memmove(m_underflow_buffer, m_underflow_buffer+last_read, m_underflow_buffer_used_length);
				}
			}
		}
	} while (processed_something && wxSTREAM_NO_ERROR==GetLastError());

	return size - remaining_bytes;
}
