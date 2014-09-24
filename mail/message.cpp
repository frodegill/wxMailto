
// Copyright (C) 2008-2014  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "message.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <streambuf>
#include "message.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../storage/persistent_object.h"
#include "../string/stringutils.h"

using namespace wxMailto;

# include <wx/listimpl.cpp>
WX_DEFINE_LIST(MessageList);


Message::Message()
: m_db_id(0),
  m_mime_entity(NULL)
{
}

Message::Message(wxMessageId db_id)
: m_db_id(db_id),
  m_mime_entity(NULL)
{
}

Message::~Message()
{
	delete m_mime_entity;
}

wxmailto_status Message::SetRFC2822Message(const char* message)
{
	if (!message)
		return LOGERROR(ID_NULL_POINTER);
	
	std::stringstream* message_stream = new std::stringstream;
	if (!message_stream)
		return LOGERROR(ID_OUT_OF_MEMORY);
	
	message_stream->write(message, strlen(message));
	wxmailto_status status = SetRFC2822Message(message_stream);
	delete message_stream;
	return status;
}

wxmailto_status Message::SetRFC2822Message(std::stringstream* message_stream)
{
	if (!message_stream)
		return LOGERROR(ID_NULL_POINTER);

	MimeEntity* mime_entity = new MimeEntity(*message_stream);
	if (!mime_entity)
		return LOGERROR(ID_OUT_OF_MEMORY);

  //ToDo

	//Testing
	MimeHeaders::iterator hbit, heit;
  hbit = mime_entity->header().begin();
  heit = mime_entity->header().end();
  for(; hbit != heit; ++hbit)
  {
      wxLogDebug("header: " + wxString(hbit->name()) + " = " + wxString(hbit->value()));
  }
  return ID_OK;
}

wxmailto_status Message::AddTag(const wxString& tag)
{
	return wxGetApp().GetAppModuleManager()->GetMessageStore()->AddTag(m_db_id, tag);
}

wxmailto_status Message::RemoveTag(const wxString& tag)
{
	return wxGetApp().GetAppModuleManager()->GetMessageStore()->RemoveTag(m_db_id, tag);
}

wxBool Message::HasTag(const wxString& tag)
{
	return wxGetApp().GetAppModuleManager()->GetMessageStore()->HasTag(m_db_id, tag);
}

wxmailto_status Message::SaveToDB()
{
	wxmailto_status status;
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	//Save message, and commit if OK
	if (ID_OK==(status=PocoGlue::StartTransaction(session)) &&
	    ID_OK==(status=SaveToDB(session)))
	{
		status = PocoGlue::CommitTransaction(session);
	}
	poco_glue->ReleaseSession(session);

	return status;
}

wxmailto_status Message::SaveToDB(Poco::Data::Session* session)
{
	wxASSERT_MSG(session, _("Message cannot be saved without a session!"));
	if (!session)
		return LOGERROR(ID_NULL_POINTER);

	wxmailto_status status;
	if (0!=m_db_id) //Update existing message
	{
		*session << "DELETE FROM message WHERE message_id = ?", //cascades
			Poco::Data::use(m_db_id),
			Poco::Data::now;
	}
	else //New message
	{
		if (ID_OK!=(status=SetDBId()))
			return status;
	}

	*session << "INSERT INTO message (message_id) VALUES (?)",
		Poco::Data::use(m_db_id),
		Poco::Data::now;
		
		//Todo, persist raw message
#if 0
	if (m_mime_parts)
	{
		MimePartList::iterator iter;
		for (iter=m_mime_parts->begin(); iter!=m_mime_parts->end(); ++iter)
		{
			if (ID_OK!=(status=(*iter)->SaveToDB(session)))
				return status;
		}
	}
#endif
	return ID_OK;
}

wxmailto_status Message::SetDBId()
{
	wxmailto_status status;
	PersistentProperty p;
	if (ID_OK != (status=p.Initialize("next_message_id", &wxGetApp().GetGlobalLockers()->m_generic_property_lock)))
		return status;

	return p.GetNextAvailableId(m_db_id);
}
