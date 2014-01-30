
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "memory_source.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "memory_source.h"


using namespace wxMailto;

MemorySource::MemorySource(const wxUint8* buffer, const wxSizeT& buffer_length, wxBool owns_buffer)
: Source(),
  m_buffer(buffer),
  m_buffer_length(buffer_length),
  m_buffer_pos(0),
  m_owns_buffer(owns_buffer)
{
}

MemorySource::~MemorySource()
{
	if (m_owns_buffer)
	{
		{
			wxCriticalSectionLocker locker(m_buffer_lock);

			delete[] m_buffer;
			m_buffer = NULL;
		}
	}
}

wxmailto_status MemorySource::ProvideBytes(wxUint8* buffer, wxSizeT& buffer_len)
{
	{
		wxCriticalSectionLocker locker(m_buffer_lock);

		buffer_len = UMIN(buffer_len, m_buffer_length-m_buffer_pos);
		memcpy(buffer, m_buffer+m_buffer_pos, buffer_len);
		m_buffer_pos += buffer_len;
	}
	return ID_OK;
}
