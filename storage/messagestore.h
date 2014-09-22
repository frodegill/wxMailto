#ifndef _MESSAGESTORE_H_
#define _MESSAGESTORE_H_

// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "messagestore.h"
#endif

#include "../defines.h"
#include "../wxmailto_errors.h"
#include "../gui/wxmailto_module.h"
#include "../mail/message.h"
#include "../mail/outgoing_message.h"


namespace wxMailto
{

class MessageStore : public wxMailto_Module
{
public:
  enum Order {ANY, SUBJECT, SUBJECT_DESC, DATE, DATE_DESC, SIZE, SIZE_DESC, SENDER, SENDER_DESC};

public:
	MessageStore();
	virtual ~MessageStore();

	wxString GetName() const {return "MessageStore";}
	ModuleType GetType() const {return wxMailto_Module::MESSAGESTORE;}

	wxmailto_status Initialize();
	wxmailto_status PrepareShutdown();

public:
	wxmailto_status GetMessage(wxMessageId id, Message*& message);
	wxmailto_status GetMessagesWithTag(MessageList& messages, const wxString& tag, Order order=ANY, wxInt limit=-1, wxInt offset=-1);
	wxmailto_status GetMessagesWithoutTag(MessageList& messages, const wxString& tag, Order order=ANY, wxInt limit=-1, wxInt offset=-1);
	wxmailto_status GetOutgoingMessages(OutgoingMessageList& messages, Order order=ANY, wxInt limit=-1, wxInt offset=-1);

	wxmailto_status AddTag(wxMessageId id, const wxString& tag);
	wxmailto_status RemoveTag(wxMessageId id, const wxString& tag);
	wxmailto_status HasTag(wxMessageId id, const wxString& tag);

	wxString GetInboxTag() const {return "INBOX";}
	wxString GetOutboxTag() const {return "OUTBOX";}
	wxString GetDraftTag() const {return "DRAFT";}
	wxString GetSentTag() const {return "SENT";}
	wxString GetTrashTag() const {return "TRASH";}
};

}

#endif // _MESSAGESTORE_H_
