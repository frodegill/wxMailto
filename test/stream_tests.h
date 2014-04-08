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

	wxmailto_status RunTests();

private:
	wxmailto_status Bas64EncodeTest();
	wxmailto_status Bas64DecodeTest();
	wxmailto_status QPEncodeTest();
	wxmailto_status QPDecodeTest();
};

#endif //RUN_TESTS

}

#endif // _STREAM_TESTS_H__
