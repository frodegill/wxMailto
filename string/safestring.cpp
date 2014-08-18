
// Copyright (C) 2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "safestring.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <gcrypt.h>
#endif

#include "safestring.h"

using namespace wxMailto;


SafeString::SafeString()
:	m_string(NULL),
  m_length(0),
  m_mode(NOOP)
{
}

SafeString::~SafeString()
{
	Clear();
}


wxmailto_status SafeString::Set(const wxUint8* src, const wxSizeT& src_len, SafeStringMode mode)
{
	wxmailto_status status;
	if (ID_OK!=(status=Clear()))
		return status;

	m_string = src;
	m_length = src_len;
	m_mode = mode;
	return ID_OK;
}

wxmailto_status SafeString::Set(const SafeString& src)
{
	return StrDup(src.m_string, src.m_length);
}

wxmailto_status SafeString::SetStr(const char* src, SafeStringMode mode)
{
	wxSizeT src_len = strlen(src);
	return Set(reinterpret_cast<const wxUint8*>(src), src_len, mode);
}

wxmailto_status SafeString::StrDup(const wxUint8* src, const wxSizeT& src_len)
{
	wxUint8* tmp = static_cast<wxUint8*>(malloc(src_len));
	if (!tmp)
		return ID_OUT_OF_MEMORY;

	memcpy(tmp, src, src_len);
	return Set(tmp, src_len, FREE);
}

wxmailto_status SafeString::StrDup(const char* src)
{
	wxSizeT src_len = strlen(src);
	return StrDup(reinterpret_cast<const wxUint8*>(src), src_len);
}

wxmailto_status SafeString::StrDupIndexed(const char* src, int index)
{
	int max_length = strlen(src)+20; //No number should exceed 20 characters
	wxUint8* tmp = static_cast<wxUint8*>(malloc(max_length));
	if (!tmp)
		return ID_OUT_OF_MEMORY;

	snprintf(reinterpret_cast<char*>(tmp), max_length, src, index);
	int length = strnlen(reinterpret_cast<char*>(tmp), max_length);
	return Set(tmp, length, FREE);
}

wxmailto_status SafeString::Get(const wxUint8*& dst, wxSizeT& length) const
{
	dst = m_string;
	length = m_length;
	return ID_OK;
}

wxmailto_status SafeString::GetStr(const char*& dst) const
{
	dst = reinterpret_cast<const char*>(m_string);
	wxSizeT str_length = strnlen(dst, m_length+1); //Add one to test that it actually is terminated at m_length
	if (str_length != m_length) //If it does not match, the string is either not terminated or contains a \0, either of which is considered a valid C-string
	{
		dst = NULL;
		return ID_INVALID_FORMAT;
	}

	return ID_OK;
}

wxmailto_status SafeString::Clear()
{
	if (!m_mode || NOOP==m_mode)
		return ID_OK;

	if (0 < m_length)
		memset(const_cast<wxUint8*>(m_string), 0, m_length);

	if (FREE == m_mode)
	{
		free(const_cast<wxUint8*>(m_string));
	}
	else if (GCRY_FREE == m_mode)
	{
		gcry_free(const_cast<wxUint8*>(m_string));
	}
	else if (DELETE == m_mode)
	{
		delete[] m_string;
	}

	m_string = NULL;
	m_length = 0;
	return ID_OK;
}

wxmailto_status SafeString::Append(const SafeString& str)
{
	int new_length = m_length + str.m_length;
	wxUint8* tmp = static_cast<wxUint8*>(malloc(new_length));
	if (!tmp)
		return ID_OUT_OF_MEMORY;

	memcpy(tmp, m_string, m_length);
	memcpy(tmp+m_length, str.m_string, str.m_length);
	return Set(tmp, new_length, FREE);
}
