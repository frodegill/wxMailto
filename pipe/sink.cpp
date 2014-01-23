
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "sink.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "sink.h"
#include "../gui/wxmailto_app.h"


using namespace wxMailto;

Sink::Sink()
: Pipe()
{
}

Sink::~Sink()
{
}

wxmailto_status Sink::SetSink(Pipe* WXUNUSED(sink))
{
	return LOGERROR(ID_SHOULDNT_GET_HERE);
}
