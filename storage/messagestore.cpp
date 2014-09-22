
// Copyright (C) 2009-2011  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "messagestore.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "messagestore.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"

using namespace wxMailto;


MessageStore::MessageStore()
 : wxMailto_Module()
{
}

MessageStore::~MessageStore()
{
}

wxmailto_status MessageStore::Initialize()
{
	wxGetApp().GetAppModuleManager()->RegisterModule(this);
	return ID_OK;
}

wxmailto_status MessageStore::PrepareShutdown()
{
	WaitForNoMoreDependencies();
	wxGetApp().GetAppModuleManager()->UnregisterModule(this);

	return ID_OK;
}

wxmailto_status MessageStore::GetMessage(wxMessageId WXUNUSED(id), Message*& WXUNUSED(message))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status MessageStore::GetMessagesWithTag(MessageList& WXUNUSED(messages), const wxString& WXUNUSED(tag), Order WXUNUSED(order), wxInt WXUNUSED(limit), wxInt WXUNUSED(offset))
{
//ToDo
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status MessageStore::GetMessagesWithoutTag(MessageList& WXUNUSED(messages), const wxString& WXUNUSED(tag), Order WXUNUSED(order), wxInt WXUNUSED(limit), wxInt WXUNUSED(offset))
{
//ToDo
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status MessageStore::GetOutgoingMessages(OutgoingMessageList& messages, Order order, wxInt limit, wxInt offset)
{
	wxmailto_status status;
	MessageList message_list;
	if (ID_OK!=(status=GetMessagesWithoutTag(message_list, GetOutboxTag(), order, limit, offset)))
		return status;

	messages.Clear();
	MessageList::iterator iter;
	for (iter=message_list.begin(); iter!=message_list.end(); ++iter)
	{
		messages.Append(static_cast<OutgoingMessage*>(*iter));
	}
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status MessageStore::AddTag(wxMessageId WXUNUSED(id), const wxString& WXUNUSED(tag))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status MessageStore::RemoveTag(wxMessageId WXUNUSED(id), const wxString& WXUNUSED(tag))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}

wxmailto_status MessageStore::HasTag(wxMessageId WXUNUSED(id), const wxString& WXUNUSED(tag))
{
	return LOGERROR(ID_NOT_IMPLEMENTED);
}
