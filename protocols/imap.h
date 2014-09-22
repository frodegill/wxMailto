#ifndef _IMAP_H_
#define _IMAP_H_

// Copyright (C) 2008-2011  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "imap.h"
#endif

#include "protocol_iface.h"


namespace wxMailto
{

class Imap : public ProtocolInterface
{
public:
	Imap(Account* account);
	virtual ~Imap();

public: //ProtocolInterface API
	wxmailto_status Sync();
	wxmailto_status CleanupAndAbort();
	wxmailto_status ReadAuthenticationNegotiationLine(SafeString& buffer);
	wxmailto_status WriteAuthenticationNegotiationLine(const SafeString& buffer);
	void WriteAuthenticationAbortedLine() {wxASSERT_MSG(false, "NOT_IMPLEMENTED");}
};

}

#endif // _IMAP_H_
