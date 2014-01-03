
// Copyright (C) 2009-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "stringutils.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "stringutils.h"
#include INCLUDE_LOG1

using namespace wxMailto;

wxSizeT StringUtils::Utf8ByteLength(const wxUint8 first_byte)
{
	if (0 == (0x80&first_byte))
		return 1;
	else if (0xC0 == (0xE0&first_byte))
		return 2;
	else if (0xE0 == (0xF0&first_byte))
		return 3;
	else if (0xF0 == (0xF8&first_byte))
		return 4;
	else
		return 0;
}

wxBool StringUtils::IsWSP(const wxChar c)
{
	return (' '==c || '\t'==c);
}

wxmailto_status StringUtils::ByteArrayToHexString(const wxUint8* source_bytes, wxSizeT source_length, wxString& destination)
{
	char* md5sum_hex = new char[source_length*2];
	if (!md5sum_hex)
		return LOGERROR(ID_OUT_OF_MEMORY);

	const char hexvalues[] = "0123456789abcdef";
	for (wxSizeT i=0; source_length>i; i++)
	{
		md5sum_hex[i*2] = hexvalues[((source_bytes[i]>>4)&0x0F)];
		md5sum_hex[i*2+1] = hexvalues[source_bytes[i]&0x0F];
	}
 
	destination = wxString(md5sum_hex, source_length*2);
	delete[] md5sum_hex;
	return ID_OK;
}
