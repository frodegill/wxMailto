#ifndef _SAFESTRING_H_
#define _SAFESTRING_H_

// Copyright (C) 2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "safestring.h"
#endif

//#include <wx/string.h>
//#include "../defines.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

enum SafeStringMode
{
	NOOP, //Do nothing, src is probably a global or shared string
	CLEAR,  //Overwrite by \0. Buffer is deleted somewhere else
	FREE, //Overwrite and free()
	DELETE //Overwrite and delete[]
};

class SafeString
{
public:
	SafeString();
	~SafeString();

	wxmailto_status Set(const wxUint8* src, const wxSizeT& src_len, SafeStringMode mode); //Set, and for FREE or DELETE: claim ownership
	wxmailto_status Set(const SafeString& src);
	wxmailto_status SetStr(const char* src, SafeStringMode mode); //Set, and for FREE or DELETE: claim ownership
	wxmailto_status StrDup(const wxUint8* src, const wxSizeT& src_len);
	wxmailto_status StrDup(const char* src);
	wxmailto_status StrDupIndexed(const char* src, int index);

	wxBool IsEmpty() const {return !m_string || 0==m_length;}
	wxSizeT Length() const {return m_length;}
 
	wxmailto_status Get(const wxUint8*& dst, wxSizeT& length) const;
	wxmailto_status GetStr(const char*& dst) const;

	wxmailto_status Clear();

	wxmailto_status Append(const SafeString& str);

private:
	const wxUint8* m_string;
	wxSizeT m_length;
	SafeStringMode m_mode;
};

}

#endif // _STRINGUTILS_H_
