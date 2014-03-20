
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "file_sink.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/ffile.h>
#endif

#include "file_sink.h"


using namespace wxMailto;

FileSink::FileSink(wxInt id, SinkResult* sink_result, const wxString& filename)
: Sink(id, sink_result),
  m_file(NULL)
{
	LOGDEBUG(wxString::Format("%d file_sink: ctor\n", m_id));
	m_file = new wxFFile(filename, "wb");
	if (m_file && !m_file->IsOpened())
	{
		LOGDEBUG(wxString::Format("%d file_sink: creating file failed\n", m_id));
		delete m_file;
		m_file = NULL;
	}
	InitializeSourceBuffer();
}

FileSink::~FileSink()
{
	LOGDEBUG(wxString::Format("%d file_sink: dtor\n", m_id));
	delete m_file;
}

wxmailto_status FileSink::HandleBytes(wxUint8* buffer, wxSizeT& buffer_len)
{
	LOGDEBUG(wxString::Format("%d file_sink: handle %d bytes\n", m_id, buffer_len));
	buffer_len = m_file->Write(buffer, buffer_len);
	if (Eof())
	{
		LOGDEBUG(wxString::Format("%d file_sink: eof. Terminate!\n", m_id));
		Terminate();
	}
	LOGDEBUG(wxString::Format("%d file_sink: handled bytes\n", m_id));
	return ID_OK;
}
