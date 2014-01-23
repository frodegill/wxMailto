
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "source.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "source.h"
#include "../gui/wxmailto_app.h"


using namespace wxMailto;

Source::Source()
: Pipe()
{
}

Source::~Source()
{
}

wxmailto_status Source::SetSource(Pipe* WXUNUSED(source))
{
	return LOGERROR(ID_SHOULDNT_GET_HERE);
}
