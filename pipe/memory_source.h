#ifndef _MEMORY_SOURCE_H_
#define _MEMORY_SOURCE_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "memory_source.h"
#endif

#include "source.h"


namespace wxMailto
{

class MemorySource : public Source
{
public:
	MemorySource(wxInt id, const wxUint8* buffer, const wxSizeT& buffer_length, wxBool owns_buffer=true);
	virtual ~MemorySource();

	void SetOwnsBuffer(wxBool owns_buffer) {m_owns_buffer = owns_buffer;}

protected: //From Source
	virtual wxmailto_status ProvideBytes(wxUint8* buffer,
	                                     wxSizeT& buffer_len); //IN: capacity. OUT: bytes written

private:
	wxCriticalSection m_buffer_lock;
	const wxUint8* m_buffer;
	wxSizeT m_buffer_length;
	wxSizeT m_buffer_pos; //We haqve the whole buffer in memory. To avoid moves, pos is start of providable buffer, m_buffer_length is end
	wxUInt m_owns_buffer:1;
};

}

#endif // _MEMORY_SOURCE_H_
