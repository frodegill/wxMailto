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
	wxmailto_status HexEncodeTest_HappyDay();
	wxmailto_status HexDecodeTest_HappyDay();
	wxmailto_status Bas64EncodeTest_HappyDay();
	wxmailto_status Bas64DecodeTest_HappyDay();
	wxmailto_status QPEncodeTest_HappyDay();
	wxmailto_status QPDecodeTest_HappyDay();
	wxmailto_status MimeLinewrappedTest_HappyDay();
};

#endif //RUN_TESTS

}

#endif // _STREAM_TESTS_H__
