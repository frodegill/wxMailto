
// Copyright (C) 2008-2011  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "imap.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "imap.h"
#include INCLUDE_LOG1

using namespace wxMailto;

Imap::Imap(Account* account)
: ProtocolInterface(account)
{
}

Imap::~Imap()
{
}

wxmailto_status Imap::Sync()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Imap::CleanupAndAbort()
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Imap::ReadAuthenticationNegotiationLine(SafeString& WXUNUSED(buffer))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status Imap::WriteAuthenticationNegotiationLine(const SafeString& WXUNUSED(buffer))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}
