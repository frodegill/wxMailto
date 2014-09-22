#ifndef _OUTGOING_MESSAGE_H_
#define _OUTGOING_MESSAGE_H_

// Copyright (C) 2009-2010  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "outgoing_message.h"
#endif

#include <wx/stream.h>
#include <wx/thread.h>
#include "message.h"
#include "../contacts/contact.h"


namespace wxMailto
{

class OutgoingMessage : public Message
{
public:
  enum RecipientType {RECIPIENT_TO, RECIPIENT_CC, RECIPIENT_BCC};

public:
	OutgoingMessage();
 	OutgoingMessage(wxMessageId db_id); //To load Draft
	virtual ~OutgoingMessage();

	wxmailto_status GetRFC2822Message(wxBool regenerate_message_id, wxInputStream*& message_stream, wxBool dot_stuff=false, wxBool use_8bit=false); //If returning true, caller will have to delete message_stream
	wxmailto_status GetRFC822MessageBody(wxInputStream*& message_body_stream, wxBool dot_stuff=false, wxBool use_8bit=false); //If returning true, caller will have to delete message_body_stream

	void SetFrom(Contact* contact) {m_from = contact;}
	Contact* GetFrom() const {return m_from;}

	wxmailto_status AddRecipient(Contact* contact, RecipientType type);
	wxmailto_status RemoveRecipient(Contact* contact, RecipientType type);
	wxmailto_status GetAllRecipients(ContactList& recipients); //Const for everything but m_recipients_lock

private:
	wxmailto_status AppendAllRecipientsRecursive(ContactList* contact_list, ContactList& recipients) const; //Must be called within a critical section for m_recipients_lock to be thread-safe
	wxmailto_status GetHeader(RecipientType type, wxString& header);
	wxmailto_status GetHeader(ContactList* contact_list, wxString& header) const;

private:
	Contact* m_from;

	wxCriticalSection m_recipients_lock;
	ContactList* m_to;
	wxString* m_to_headervalue; //Cache To: header value
	ContactList* m_cc;
	wxString* m_cc_headervalue; //Cache Cc: header value
	ContactList* m_bcc;
	wxString* m_bcc_headervalue; //Cache Bcc: header value
};

WX_DECLARE_LIST(OutgoingMessage, OutgoingMessageList);

}

#endif // _OUTGOING_MESSAGE_H_
