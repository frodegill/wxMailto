
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "stream_tests.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "stream_tests.h"

#include <wx/wfstream.h>
#include <wx/mstream.h>
#include "../stream/base64_bufferedstream.h"
#include "../stream/qp_bufferedstream.h"


using namespace wxMailto;

#ifdef RUN_TESTS

StreamTests::StreamTests()
{
}

wxmailto_status StreamTests::RunTests()
{
	{
		wxFileOutputStream fos("/tmp/base64-test.txt");

		Base64OutputStream b64s(&fos, 3*1024, 4*1024);
		if (b64s.IsOk())
		{
			char buf[] = {0, 1, 2, 3};
			wxMemoryInputStream input(buf, 4);

			b64s.Write(input);
			b64s.Close();
		}
	}

	{
		wxFileInputStream fis("/tmp/base64-test.txt");

		Base64InputStream b64s(&fis, 4*1024, 3*1024);
		if (b64s.IsOk())
		{
			wxFileOutputStream fos("/tmp/base64-test2.txt");

			b64s.Read(fos);
			b64s.Close();
		}
	}

	{
		wxFileOutputStream fos("/tmp/qp-test.txt");

		QPOutputStream qps(&fos, 1*1024, 1*1024);
		if (qps.IsOk())
		{
			const char* buf = "Now's the time =\r\n"
                        "for all folk to come=\r\n"
                        " to the aid of their country.";
			wxMemoryInputStream input(buf, strlen(buf));

			qps.Write(input);
			qps.Close();
		}
	}

	{
		wxFileInputStream fis("/tmp/qp-test.txt");

		QPInputStream qps(&fis, 1*1024, 1*1024);
		if (qps.IsOk())
		{
			wxFileOutputStream fos("/tmp/qp-test2.txt");

			qps.Read(fos);
			qps.Close();
		}
	}

	return ID_OK;
}

#endif //RUN_TESTS
