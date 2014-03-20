
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

MemorySource::MemorySource(wxInt id, const wxUint8* buffer, const wxSizeT& buffer_length, wxBool owns_buffer)
: Source(id),
  m_buffer(buffer),
  m_buffer_length(buffer_length),
  m_buffer_pos(0),
  m_owns_buffer(owns_buffer)
{
	LOGDEBUG(wxString::Format("%d memory_source: ctor\n", m_id));
	InitializeSinkBuffer();
}

MemorySource::~MemorySource()
{
	LOGDEBUG(wxString::Format("%d memory_source: dtor\n", m_id));
	if (m_owns_buffer)
	{
		{
			LOGDEBUG(wxString::Format("%d memory_source: dtor deleting buffer\n", m_id));
			LOGDEBUG(wxString::Format("%d memory_source ~MemorySource(1): aquiring buffer lock\n", m_id));
			wxCriticalSectionLocker locker(m_buffer_lock);
			LOGDEBUG(wxString::Format("%d memory_source ~MemorySource(1): aquired buffer lock\n", m_id));

			delete[] m_buffer;
			m_buffer = NULL;
			LOGDEBUG(wxString::Format("%d memory_source: dtor buffer deleted\n", m_id));
			LOGDEBUG(wxString::Format("%d memory_source ~MemorySource(1): releasing buffer lock\n", m_id));
		}
	}
}

wxmailto_status MemorySource::ProvideBytes(wxUint8* buffer, wxSizeT& buffer_len)
{
	{
		LOGDEBUG(wxString::Format("%d memory_source: provide %d bytes. Wait for lock\n", m_id, buffer_len));
		LOGDEBUG(wxString::Format("%d memory_source ProvideBytes(1): aquiring buffer lock\n", m_id));
		wxCriticalSectionLocker locker(m_buffer_lock);
		LOGDEBUG(wxString::Format("%d memory_source ProvideBytes(1): aquired buffer lock\n", m_id));

		LOGDEBUG(wxString::Format("%d memory_source: provideBytes aquired lock\n", m_id));
		buffer_len = UMIN(buffer_len, m_buffer_length-m_buffer_pos);
		memcpy(buffer, m_buffer+m_buffer_pos, buffer_len);
		m_buffer_pos += buffer_len;
		LOGDEBUG(wxString::Format("%d memory_source: provided %d bytes\n", m_id, buffer_len));
		LOGDEBUG(wxString::Format("%d memory_source ProvideBytes(1): releasing buffer lock\n", m_id));
	}
	return ID_OK;
}
