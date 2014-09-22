
// Copyright (C) 2009-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "outgoing_message.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "outgoing_message.h"

using namespace wxMailto;

# include <wx/listimpl.cpp>
WX_DEFINE_LIST(OutgoingMessageList);


OutgoingMessage::OutgoingMessage()
: Message(),
  m_from(NULL),
  m_to(NULL),
  m_cc(NULL),
  m_bcc(NULL)
{
}

OutgoingMessage::OutgoingMessage(wxMessageId db_id)
: Message(db_id),
  m_from(NULL),
  m_to(NULL),
  m_to_headervalue(NULL),
  m_cc(NULL),
  m_cc_headervalue(NULL),
  m_bcc(NULL),
  m_bcc_headervalue(NULL)
{
}

OutgoingMessage::~OutgoingMessage()
{
	delete m_from;
	{
		wxCriticalSectionLocker locker(m_recipients_lock);
		delete m_to;
		delete m_to_headervalue;
		delete m_cc;
		delete m_cc_headervalue;
		delete m_bcc;
		delete m_bcc_headervalue;
	}
}

wxmailto_status OutgoingMessage::GetRFC2822Message(wxBool WXUNUSED(regenerate_message_id), wxInputStream*& WXUNUSED(message_stream), wxBool dot_stuff, wxBool WXUNUSED(use_8bit))
{
	if (dot_stuff)
	{
//ToDo
	}

	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status OutgoingMessage::GetRFC822MessageBody(wxInputStream*& WXUNUSED(message_body_stream), wxBool WXUNUSED(dot_stuff), wxBool WXUNUSED(use_8bit))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status OutgoingMessage::AddRecipient(Contact* contact, RecipientType type)
{
	{
		wxCriticalSectionLocker locker(m_recipients_lock);
		switch(type)
		{
		case RECIPIENT_TO:
			{
				if (!m_to) {m_to=new ContactList(); m_to->DeleteContents(true);}
				m_to->Append(contact);
				delete m_to_headervalue;
				m_to_headervalue = NULL;
				return ID_OK;
			}
		case RECIPIENT_CC:
			{
				if (!m_cc) {m_cc=new ContactList(); m_cc->DeleteContents(true);}
				m_cc->Append(contact);
				delete m_cc_headervalue;
				m_cc_headervalue = NULL;
				return ID_OK;
			}
		case RECIPIENT_BCC:
			{
				if (!m_bcc) {m_bcc=new ContactList(); m_bcc->DeleteContents(true);}
				m_bcc->Append(contact);
				delete m_bcc_headervalue;
				m_bcc_headervalue = NULL;
				return ID_OK;
			}
		default: return LOGERROR(ID_INVALID_FORMAT);
		}
	}

	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

wxmailto_status OutgoingMessage::RemoveRecipient(Contact* contact, RecipientType type)
{
	{
		wxCriticalSectionLocker locker(m_recipients_lock);
		switch(type)
		{
		case RECIPIENT_TO:
			{
				if (!m_to) return ID_OK;
				m_to->DeleteObject(contact);
				delete m_to_headervalue;
				m_to_headervalue = NULL;
				return ID_OK;
			}
		case RECIPIENT_CC:
			{
				if (!m_cc) return ID_OK;
				m_cc->DeleteObject(contact);
				delete m_cc_headervalue;
				m_cc_headervalue = NULL;
				return ID_OK;
			}
		case RECIPIENT_BCC:
			{
				if (!m_bcc) return ID_OK;
				m_bcc->DeleteObject(contact);
				delete m_bcc_headervalue;
				m_bcc_headervalue = NULL;
				return ID_OK;
			}
		default: return LOGERROR(ID_INVALID_FORMAT);
		}
	}

	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

wxmailto_status OutgoingMessage::GetAllRecipients(ContactList& recipients)
{
	wxmailto_status status;
	recipients.Clear();
	ContactList* contact_list = NULL;
	wxInt i;
	{
		wxCriticalSectionLocker locker(m_recipients_lock);
		for (i=0; i<3; i++)
		{
			switch(i)
			{
			case 0: contact_list=m_to; break;
			case 1: contact_list=m_cc; break;
			case 2: contact_list=m_bcc; break;
			default: wxASSERT(false); break;
			}
	
			if (ID_OK!=(status=AppendAllRecipientsRecursive(contact_list, recipients)))
				return status;
		}
	}
	return ID_OK;
}

wxmailto_status OutgoingMessage::AppendAllRecipientsRecursive(ContactList* contact_list, ContactList& recipients) const
{
	if (!contact_list)
		return ID_OK; //Nothing to append

	wxmailto_status status;
	Contact* contact;
	ContactList::iterator iter;
	for (iter=contact_list->begin(); iter!=contact_list->end(); ++iter)
	{
		contact = *iter;
		if (!contact)
			continue;

		if (Contact::CONTACT_PERSON!=contact->GetContactType() && contact->HasChildren())
		{
			if (ID_OK!=(status=AppendAllRecipientsRecursive(contact->GetChildren(), recipients)))
				return status;
		} else {
			recipients.Append(contact);
		}
	}
	return ID_OK;
}

wxmailto_status OutgoingMessage::GetHeader(RecipientType type, wxString& header)
{
	header.Empty();

	wxmailto_status status;
	{
		wxCriticalSectionLocker locker(m_recipients_lock);
		switch(type)
		{
		case RECIPIENT_TO:
			{
				if (!m_to || 0==m_to->GetCount()) return ID_OK; //No value for this header
				if (!m_to_headervalue) m_to_headervalue=new wxString();
				if (!m_to_headervalue)
					return LOGERROR(ID_OUT_OF_MEMORY);

				if (ID_OK!=(status=GetHeader(m_to, *m_to_headervalue)))
				{
					delete m_to_headervalue;
					m_to_headervalue = NULL;
					return status;
				}
				m_to_headervalue->insert(0, "To: ");
				header = *m_to_headervalue;
				return ID_OK;
			}
		case RECIPIENT_CC:
			{
				if (!m_cc || 0==m_cc->GetCount()) return ID_OK; //No value for this header
				if (!m_cc_headervalue) m_cc_headervalue=new wxString();
				if (!m_cc_headervalue)
					return LOGERROR(ID_OUT_OF_MEMORY);

				if (ID_OK!=(status=GetHeader(m_cc, *m_cc_headervalue)))
				{
					delete m_cc_headervalue;
					m_cc_headervalue = NULL;
					return status;
				}
				m_cc_headervalue->insert(0, "Cc: ");
				header = *m_cc_headervalue;
				return ID_OK;
			}
		case RECIPIENT_BCC:
			{
				if (!m_bcc || 0==m_bcc->GetCount()) return ID_OK; //No value for this header
				if (!m_bcc_headervalue) m_bcc_headervalue=new wxString();
				if (!m_bcc_headervalue)
					return LOGERROR(ID_OUT_OF_MEMORY);

				if (ID_OK!=(status=GetHeader(m_bcc, *m_bcc_headervalue)))
				{
					delete m_bcc_headervalue;
					m_bcc_headervalue = NULL;
					return status;
				}
				m_bcc_headervalue->insert(0, "Bcc: ");
				header = *m_bcc_headervalue;
				return ID_OK;
			}
		default: return LOGERROR(ID_INVALID_FORMAT);
		}
	}
	return LOGERROR(ID_SHOULDNT_GET_HERE);
}

wxmailto_status OutgoingMessage::GetHeader(ContactList* contact_list, wxString& header) const
{
	header.Empty();
	if (!contact_list || 0==contact_list->GetCount())
		return ID_OK;

	wxmailto_status status;
	wxString contact_address;
	wxBool first = true;
	ContactList::iterator iter;
	for (iter=contact_list->begin(); iter!=contact_list->end(); ++iter)
	{
		Contact* current = *iter;
		if (!current) continue;

		if (ID_OK!=(status=current->GetRFC2822Address(contact_address)))
			return status;

		if (!first)
		{
			header.Append(", ");
		}
		header.Append(contact_address);
		first = false;
	}

	return ID_OK;
}
