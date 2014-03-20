#ifndef _BUFFERED_STREAMBASE_H_
#define _BUFFERED_STREAMBASE_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

// wxStreamBuffer doesn't handle neither underflow nor overflow very well,
// so this is an attempt to do it better.

#ifdef __GNUG__
  #pragma interface "buffered_streambase.h"
#endif

#include <wx/stream.h>


namespace wxMailto
{

class BufferedStreamBase
{
public:
	BufferedStreamBase(wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);
	~BufferedStreamBase();

protected:
	virtual bool IsOk() const {return NULL!=m_underflow_buffer && NULL!=m_overflow_buffer;}

protected:
	wxUint8* m_underflow_buffer;
	wxSizeT m_max_underflow_buffer_length;
	wxSizeT m_underflow_buffer_length;
	wxSizeT m_underflow_buffer_used_length;
	wxUint8* m_overflow_buffer;
	wxSizeT m_max_overflow_buffer_length;
	wxSizeT m_overflow_buffer_length;
	wxSizeT m_overflow_buffer_used_length;
};


class BufferedInputStream : public BufferedStreamBase, public wxInputStream
{
public:
	BufferedInputStream(wxInputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);
	~BufferedInputStream();

public:
	//From wxInputStream
	virtual bool IsSeekable() const {return false;}
	virtual bool IsOk() const {return NULL!=m_stream && BufferedStreamBase::IsOk() && wxInputStream::IsOk();}
	virtual bool Eof() const;
	size_t OnSysRead(void* buffer, size_t bufsize);

public:
	wxBool Close();
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof) = 0;

private:
	wxInputStream* m_stream;
	wxBool m_eof;
};

class BufferedOutputStream : public BufferedStreamBase, public wxOutputStream
{
public:
	BufferedOutputStream(wxOutputStream* stream, wxSizeT max_underflow_buffer_length, wxSizeT max_overflow_buffer_length);
	~BufferedOutputStream();

public:
	//From wxOutputStream
	virtual bool Close();
	virtual bool IsSeekable() const {return false;}
	virtual bool IsOk () const {return NULL!=m_stream && BufferedStreamBase::IsOk() && wxOutputStream::IsOk();}
	size_t OnSysWrite(const void* buffer, size_t bufsize);

public:
	virtual void Process(const wxUint8* src, wxSizeT src_length, wxSizeT& read_bytes,
	                     wxUint8* dst, wxSizeT dst_length, wxSizeT& written_bytes,
	                     wxBool eof) = 0;

private:
	wxOutputStream* m_stream;
	wxBool m_eof;
};

}

#endif // _BUFFERED_STREAMBASE_H_
