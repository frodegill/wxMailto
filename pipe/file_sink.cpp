
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

FileSink::FileSink(SinkResult* sink_result, const wxString& filename)
: Sink(sink_result),
  m_file(NULL)
{
	m_file = new wxFFile(filename, "wb");
	if (m_file && !m_file->IsOpened())
	{
		delete m_file;
		m_file = NULL;
	}
}

FileSink::~FileSink()
{
	delete m_file;
}

wxmailto_status FileSink::HandleBytes(wxUint8* buffer, wxSizeT& buffer_len)
{
	buffer_len = m_file->Write(buffer, buffer_len);
	if (Eof())
	{
		Terminate();
	}
	return ID_OK;
}
