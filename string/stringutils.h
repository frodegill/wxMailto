#ifndef _STRINGUTILS_H_
#define _STRINGUTILS_H_

// Copyright (C) 2009-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "stringutils.h"
#endif

#include <wx/string.h>
#include "../defines.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

class StringUtils
{
public:
	StringUtils() {}
	virtual ~StringUtils() {}

	static wxSizeT Utf8ByteLength(const wxUint8 first_byte);

	static wxBool IsWSP(const wxChar c);

	static wxmailto_status ByteArrayToHexString(const wxUint8* source_bytes, wxSizeT source_length, wxString& destination);
	static wxmailto_status HexStringToByteArrayAllocates(const wxString& source, wxUint8*& destination_bytes, wxSizeT& destination_length);
};

}

#endif // _STRINGUTILS_H_
