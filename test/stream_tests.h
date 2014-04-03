#ifndef _STREAM_TESTS_H_
#define _STREAM_TESTS_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "stream_tests.h"
#endif

#include "../wxmailto_errors.h"


namespace wxMailto
{

#ifdef RUN_TESTS

class StreamTests
{
public:
	StreamTests();

public:
	wxmailto_status RunTests();
};

#endif //RUN_TESTS

}

#endif // _STREAM_TESTS_H__
