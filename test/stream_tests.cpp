
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

#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>

#include "../gui/app_module_manager.h"
#include "../gui/wxmailto_app.h"
#include "../stream/base64_bufferedstream.h"
#include "../stream/hex_bufferedstream.h"
#include "../stream/qp_bufferedstream.h"


using namespace wxMailto;

#ifdef RUN_TESTS

StreamTests::StreamTests()
{
}

wxmailto_status StreamTests::RunTests()
{
	wxmailto_status status;
	
	if (ID_OK!=(status=HexEncodeTest()) ||
	    ID_OK!=(status=HexDecodeTest()) ||
	    ID_OK!=(status=Bas64EncodeTest()) ||
	    ID_OK!=(status=Bas64DecodeTest()) ||
	    ID_OK!=(status=QPEncodeTest()) ||
	    ID_OK!=(status=QPDecodeTest()))
	{
		return status;
	}

	return ID_OK;
}

wxmailto_status StreamTests::HexEncodeTest()
{
	{
		wxStringOutputStream os;

		HexEncodeStream hos(&os, 1*1024, 1*1024);
		if (!hos.IsOk())
			return LOGERROR(ID_TEST_FAILED);

		{
			wxUint8 buf[] = {0x00, 0x09, 0x0A, 0x7F, 0x80, 0xFF};
			wxMemoryInputStream is(buf, 6);

			hos.Write(is);
			hos.Close();
		}

		if (!os.GetString().IsSameAs("00090A7F80FF"))
			return LOGERROR(ID_TEST_FAILED);
	}
	return ID_OK;
}

wxmailto_status StreamTests::HexDecodeTest()
{
	{
		wxStringInputStream is("00090A7F80FF");

		HexDecodeStream his(&is, 1*1024, 1*1024);
		if (!his.IsOk())
			return LOGERROR(ID_TEST_FAILED);

		wxUint8 buf[100];
		{
			wxMemoryOutputStream os(&buf, 100);

			his.Read(os);
			his.Close();
		}
		
		if (0x00!=buf[0] || 0x09!=buf[1] || 0x0A!=buf[2] ||
		    0x7F!=buf[3] || 0x80!=buf[4] || 0xFF!=buf[5])
		{
			return LOGERROR(ID_TEST_FAILED);
		}
	}
	return ID_OK;
}

wxmailto_status StreamTests::Bas64EncodeTest()
{
	wxmailto_status status = ID_OK;

	{
		wxStringOutputStream os;

		Base64EncodeStream b64s(&os, 3*1024, 4*1024);
		if (b64s.IsOk())
		{
			char buf[] = {0, 1, 2, 3};
			wxMemoryInputStream input(buf, 4);

			b64s.Write(input);
			b64s.Close();
		}

		if (!os.GetString().IsSameAs("AAECAw=="))
		{
			status = LOGERROR(ID_TEST_FAILED);
		}
	}
	return status;
}

wxmailto_status StreamTests::Bas64DecodeTest()
{
	{
		wxFileInputStream fis("/tmp/base64-test.txt");

		Base64DecodeStream b64s(&fis, 4*1024, 3*1024);
		if (b64s.IsOk())
		{
			wxFileOutputStream fos("/tmp/base64-test2.txt");

			b64s.Read(fos);
			b64s.Close();
		}
	}
	return ID_OK;
}

wxmailto_status StreamTests::QPEncodeTest()
{
	wxmailto_status status = ID_OK;

	{
		wxStringOutputStream os;

		QPEncodeStream qps(&os, 1*1024, 1*1024);
		if (qps.IsOk())
		{
			const char* buf = "Now's the time =\r\n"
                        "for all folk to come=\r\n"
                        " to the aid of their country.";
			wxMemoryInputStream input(buf, strlen(buf));

			qps.Write(input);
			qps.Close();
		}

		wxString s = os.GetString();
		if (!os.GetString().IsSameAs(""))
		{
			status = LOGERROR(ID_TEST_FAILED);
		}
	}
	return status;
}

wxmailto_status StreamTests::QPDecodeTest()
{
	{
		wxFileInputStream fis("/tmp/qp-test.txt");

		QPDecodeStream qps(&fis, 1*1024, 1*1024);
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
