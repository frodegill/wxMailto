
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
	    ID_OK!=(status=QPDecodeTest()) ||
	    ID_OK!=(status=MimeLinewrappedTest()))
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
		
		wxUint8 expected[] = {0x00, 0x09, 0x0A, 0x7F, 0x80, 0xFF};
		wxSizeT i;
		for (i=0; sizeof(expected)/sizeof(expected[0]) > i; i++)
		{
			if (buf[i] != expected[i])
				return LOGERROR(ID_TEST_FAILED);
		}
	}
	return ID_OK;
}

wxmailto_status StreamTests::Bas64EncodeTest()
{
	{
		wxStringOutputStream os;

		Base64EncodeStream b64s(&os, 3*1024, 4*1024);
		if (!b64s.IsOk())
			return LOGERROR(ID_TEST_FAILED);
		
		{
			char buf[] = {0, 1, 2, 3};
			wxMemoryInputStream input(buf, 4);

			b64s.Write(input);
			b64s.Close();
		}

		if (!os.GetString().IsSameAs("AAECAw=="))
			return LOGERROR(ID_TEST_FAILED);
	}
	return ID_OK;
}

wxmailto_status StreamTests::Bas64DecodeTest()
{
	{
		wxStringInputStream is("AAECAw==");

		Base64DecodeStream b64s(&is, 4*1024, 3*1024);
		if (!b64s.IsOk())
			return LOGERROR(ID_TEST_FAILED);

		wxUint8 buf[100];
		{
			wxMemoryOutputStream os(&buf, 100);

			b64s.Read(os);
			b64s.Close();
		}
		
		wxUint8 expected[] = {0, 1, 2, 3};
		wxSizeT i;
		for (i=0; sizeof(expected)/sizeof(expected[0]) > i; i++)
		{
			if (buf[i] != expected[i])
				return LOGERROR(ID_TEST_FAILED);
		}
	}
	return ID_OK;
}

wxmailto_status StreamTests::QPEncodeTest()
{
	{
		wxStringOutputStream os;

		QPEncodeStream qps(&os, 1*1024, 1*1024, 0);
		if (!qps.IsOk())
			return LOGERROR(ID_TEST_FAILED);

		{
			wxStringInputStream is("=? \r\n =AD === .");
			qps.Write(is);
			qps.Close();
		}

		wxString s = os.GetString();
		if (!os.GetString().IsSameAs("=3D? \r\n =3DAD =3D=3D=3D ."))
		{
			return LOGERROR(ID_TEST_FAILED);
		}
	}
	return ID_OK;
}

wxmailto_status StreamTests::QPDecodeTest()
{
	{
		wxStringInputStream is("Now's the time =\r\n"
		                       "for all folk to come=\r\n"
		                       " to the aid of their country.");

		QPDecodeStream qps(&is, 1*1024, 1*1024);
		if (!qps.IsOk())
			return LOGERROR(ID_TEST_FAILED);

		wxString s;
		{
			wxStringOutputStream os;

			qps.Read(os);
			qps.Close();
			s = os.GetString();
		}

		if (!s.IsSameAs("Now's the time "
			              "for all folk to come"
			              " to the aid of their country."))
		{
			return LOGERROR(ID_TEST_FAILED);
		}
	}
	return ID_OK;
}

wxmailto_status StreamTests::MimeLinewrappedTest()
{
	{
		wxStringOutputStream os;

		MimeLinewrappedStream mls(&os, 1*1024, 1*1024, 18, 0, QP, false);
		if (!mls.IsOk())
			return LOGERROR(ID_TEST_FAILED);

		{
			wxStringInputStream is("Now's the time "
			                       "for all folk to come"
			                       " to the aid of their country.");
			mls.Write(is);
			mls.Close();
		}

		wxString s = os.GetString();
		if (!os.GetString().IsSameAs("Now's the time =\r\n"
			                           "for all folk to come=\r\n"
			                           " to the aid of their country."))
		{
			return LOGERROR(ID_TEST_FAILED);
		}
	}
	return ID_OK;
}

#endif //RUN_TESTS
