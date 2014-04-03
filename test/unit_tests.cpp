
// Copyright (C) 2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "unit_tests.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "unit_tests.h"

#include "stream_tests.h"


using namespace wxMailto;

#ifdef RUN_TESTS

UnitTests::UnitTests()
{
}

wxmailto_status UnitTests::RunTests()
{
	StreamTests stream_tests;
	stream_tests.RunTests();
      
#if 0
	wxUint8* buffer = new wxUint8[100];
	strcpy((char*)buffer, "Test!\n");
	LOGDEBUG("gpg_manager: Created buffer\n");
	MemorySource* source = new MemorySource(1, buffer, 6, true);
	LOGDEBUG("gpg_manager: Created MemorySource\n");

	SinkResult sink_result(2);
	LOGDEBUG("gpg_manager: Created sink_result\n");
	FileSink* sink = new FileSink(3, &sink_result, "/tmp/test.txt");
	LOGDEBUG("gpg_manager: Created sink /tmp/test.txt\n");
	source->ConnectTo(sink);
	LOGDEBUG("gpg_manager: Connected source to sink\n");
	
	LOGDEBUG("gpg_manager: before flow\n");
	sink->StartFlow();
	LOGDEBUG("gpg_manager: flow started. Wait\n");
	sink_result.Wait();
	LOGDEBUG("gpg_manager: After wait\n");
	delete sink;
	LOGDEBUG("gpg_manager: deleted sink\n");
	delete source;
	LOGDEBUG("gpg_manager: deleted source\n");
#endif
	
	return ID_OK;
}

#endif //RUN_TESTS
