#ifndef _MESSAGE_H_
#define _MESSAGE_H_

// Copyright (C) 2008-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "message.h"
#endif

#include <wx/list.h>
#include "../defines.h"
#include "../glue/mimeglue.h"
#include "../glue/pocoglue.h"
#include "../gui/wxmailto_app.h"


namespace wxMailto
{

class Message
{
public:
	Message();
	Message(wxMessageId db_id);
	virtual ~Message();

public:
	wxmailto_status SetRFC2822Message(const char* message);
	wxmailto_status SetRFC2822Message(std::stringstream* message_stream);

public:
	wxmailto_status AddTag(const wxString& tag);
	wxmailto_status RemoveTag(const wxString& tag);
	wxBool HasTag(const wxString& tag);

	wxmailto_status GetMimeParts(MimeEntityList*&);
	wxmailto_status GetPreferredMimePart(MimeEntity*&);
	wxmailto_status GetAttachmentList();

public:
	wxmailto_status SaveToDB();
	wxmailto_status SaveToDB(Poco::Data::Session* session);
private:
	wxmailto_status SetDBId();
public:
	wxMessageId GetDBId() const {return m_db_id;}

protected:
	wxMessageId m_db_id;
	MimeEntity* m_mime_entity;
};

WX_DECLARE_LIST(Message, MessageList);

}

#endif // _MESSAGE_H_
