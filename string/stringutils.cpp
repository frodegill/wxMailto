
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

static const char g_hexstring[] = "0123456789abcdef";
static const wxUint8 g_hexvalues[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                       0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 16, 16, 16, 16, 16, 16,
                                      16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
                                      16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};

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

	for (wxSizeT i=0; source_length>i; i++)
	{
		md5sum_hex[i*2] = g_hexstring[((source_bytes[i]>>4)&0x0F)];
		md5sum_hex[i*2+1] = g_hexstring[source_bytes[i]&0x0F];
	}
 
	destination = wxString(md5sum_hex, source_length*2);
	delete[] md5sum_hex;
	return ID_OK;
}

wxmailto_status StringUtils::HexStringToByteArrayAllocates(const wxString& source, wxUint8*& destination_bytes, wxSizeT& destination_length)
{
	wxSizeT source_length = source.length();
	if (0 != (source_length%2))
		return LOGERROR(ID_INVALID_FORMAT);

	const char* source_ptr = source.c_str();
	const wxUint8* source_bytes = reinterpret_cast<const wxUint8*>(source_ptr);

	destination_length = 0;
	destination_bytes = new wxUint8[source_length/2];
	if (!destination_bytes)
		return LOGERROR(ID_OUT_OF_MEMORY);

	wxUint8 msb, lsb;
	for (wxSizeT i=0; source_length>i; i++)
	{
		msb = g_hexvalues[*source_bytes++];
		lsb = g_hexvalues[*source_bytes++];
		if (16<=msb || 16<=lsb)
		{
			delete[] destination_bytes;
			return LOGERROR(ID_INVALID_FORMAT);
		}
		destination_bytes[destination_length++] = (msb<<4) + lsb;
	}
	
	return ID_OK;
}
