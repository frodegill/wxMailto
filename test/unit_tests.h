#ifndef _UNIT_TESTS_H_
#define _UNIT_TESTS_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "unit_tests.h"
#endif

#include "../wxmailto_errors.h"


namespace wxMailto
{

#ifdef RUN_TESTS
	
class UnitTests
{
public:
	UnitTests();

public:
	wxmailto_status RunTests();
};

#endif //RUN_TESTS

}

#endif // _UNIT_TESTS_H__
