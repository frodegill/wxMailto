#ifndef _SOURCE_H_
#define _SOURCE_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "source.h"
#endif

#include "pipe.h"


namespace wxMailto
{

class Source : public Pipe
{
public:
	Source();
	virtual ~Source();

public:
	virtual wxmailto_status SetSource(Pipe* source);
};

}

#endif // _SOURCE_H_
