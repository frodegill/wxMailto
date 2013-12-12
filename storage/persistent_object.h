#ifndef _PERSISTENTPROPERTY_H_
#define _PERSISTENTPROPERTY_H_

// Copyright (C) 2012-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "persistentproperty.h"
#endif

#include <wx/thread.h>

#include "../glue/pocoglue.h"
#include "../wxmailto_errors.h"


namespace wxMailto
{

#define DB_VERSION                    "db_version"
#if 0
#define ENABLE_PERSISTENT_MIME_PARSE	"enable_persistent_mime_parse"
#define GPG_DEFAULT_KEY								"gpg_default_key"
#define NEXT_MESSAGE_ID								"next_message_id"
#define NEXT_MULTIPART_ID							"next_multipart_id"
#endif
#define NEXT_CREDENTIAL_ID            "next_credential_id"


class PersistentProperty
{
public:
	PersistentProperty();
	wxmailto_status Initialize(const wxString& key, wxCriticalSection* lock);

public:
	wxmailto_status GetNextAvailableId(wxUint32& next_available_id, wxInt count=1);
	
public:
	wxmailto_status GetStringValue(wxString& value, wxBool& exists);
	wxmailto_status SetStringValue(const wxString& value);
	wxmailto_status GetIntValue(wxInt& value, wxBool& exists);
	wxmailto_status SetIntValue(wxInt value);
	wxBool HasValue();
	wxmailto_status DeleteValue();

private:
	wxmailto_status ReadValue(wxString& value, wxBool& exists);
	wxmailto_status ReadValueWhileLocked(Poco::Data::Session* session, wxString& value, wxBool& exists);
	wxmailto_status WriteValue(const wxString& value);
	wxmailto_status WriteValueWhileLocked(Poco::Data::Session* session, const wxString& value);
	
private:
	wxString m_key;
	wxCriticalSection* m_lock;
};

}

#endif // _PERSISTENTPROPERTY_H_
