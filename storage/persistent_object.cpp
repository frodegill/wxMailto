
// Copyright (C) 2012-2013  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "persistent_property.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "persistent_object.h"
#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../storage/database_update.h"


using namespace wxMailto;


PersistentProperty::PersistentProperty()
: m_lock(NULL)
{
}

wxmailto_status PersistentProperty::Initialize(const wxString& key, wxCriticalSection* lock)
{
	if (!lock)
		return LOGERROR(ID_NULL_POINTER);
	
	m_key = key;
	m_lock = lock;
	return ID_OK;
}

wxmailto_status PersistentProperty::GetNextAvailableId(wxUint32& next_available_id, wxInt count)
{
	wxmailto_status status;
	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	{
		wxCriticalSectionLocker locker(*m_lock);

		wxString value;
		wxBool exists;
		if (ID_OK!=(status=PocoGlue::StartTransaction(session)) ||
		    ID_OK!=(status=ReadValueWhileLocked(session, value, exists)))
		{
			poco_glue->ReleaseSession(session);
			return status;
		}

		next_available_id = 0;
		if (exists)
		{
			wxLong dummy;
			if (!value.ToLong(&dummy))
			{
				status = ID_INVALID_FORMAT;
			}
			else
			{
				next_available_id = dummy;
			}
		}
		
		if (ID_OK == status &&
			  ID_OK == (status=WriteValueWhileLocked(session, wxString::Format("%d", next_available_id+count))))
		{
			status = PocoGlue::CommitTransaction(session);
		}
	}
	poco_glue->ReleaseSession(session);
	return status;
}

wxmailto_status PersistentProperty::GetStringValue(wxString& value, wxBool& exists)
{
	return ReadValue(value, exists);
}

wxmailto_status PersistentProperty::SetStringValue(const wxString& value)
{
	return WriteValue(value);
}

wxmailto_status PersistentProperty::GetIntValue(wxInt& value, wxBool& exists)
{
	wxmailto_status status;
	wxString string_value;
	if (ID_OK != (status=ReadValue(string_value, exists)))
		return status;

	if (!exists)
		return ID_OK;

	wxLong dummy;
	if (!string_value.ToLong(&dummy))
		return LOGERROR(ID_INVALID_FORMAT);

	value = dummy;
	return ID_OK;
}

wxmailto_status PersistentProperty::SetIntValue(wxInt value)
{
	return WriteValue(wxString::Format("%d", value));
}

wxBool PersistentProperty::HasValue()
{
	wxString dummy;
	wxBool exists;
	if (ID_OK != ReadValue(dummy, exists))
		return false;

	return exists;
}

wxmailto_status PersistentProperty::DeleteValue()
{
	wxmailto_status status = ID_OK;

	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	{
		if (ID_OK!=(status=PocoGlue::StartTransaction(session)))
		{
			poco_glue->ReleaseSession(session);
			return status;
		}

		std::string key_string = std::string(m_key.ToUTF8());
		*session << "DELETE FROM property "\
		            "WHERE property_key = ?",
			Poco::Data::use(key_string),
			Poco::Data::now;

		status = PocoGlue::CommitTransaction(session);
	}

	poco_glue->ReleaseSession(session);
	return status;
}

wxmailto_status PersistentProperty::ReadValue(wxString& value, wxBool& exists)
{
	wxmailto_status status = ID_OK;

	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	{
		wxCriticalSectionLocker locker(*m_lock);

		status = ReadValueWhileLocked(session, value, exists);
	}

	poco_glue->ReleaseSession(session);
	return status;
}

wxmailto_status PersistentProperty::ReadValueWhileLocked(Poco::Data::Session* session, wxString& value, wxBool& exists)
{
		std::string key_string = std::string(m_key.ToUTF8());
		int rowcount = 0;
		std::string value_string;
		*session << "SELECT COUNT(*) AS rowcount, property_value FROM property WHERE property_key = ?",
			Poco::Data::into(rowcount),
			Poco::Data::into(value_string),
			Poco::Data::use(key_string),
			Poco::Data::now;
	
	exists = (0<rowcount);
	value = wxString(value_string);
	return ID_OK;
}

wxmailto_status PersistentProperty::WriteValue(const wxString& value)
{
	wxmailto_status status = ID_OK;

	PocoGlue* poco_glue = wxGetApp().GetAppModuleManager()->GetPocoGlue();
	Poco::Data::Session* session;
	if (ID_OK!=(status=poco_glue->CreateSession(session)))
	{
		return status;
	}

	{
		wxCriticalSectionLocker locker(*m_lock);

		status = WriteValueWhileLocked(session, value);
	}

	poco_glue->ReleaseSession(session);
	return status;
}

wxmailto_status PersistentProperty::WriteValueWhileLocked(Poco::Data::Session* session, const wxString& value)
{
	wxmailto_status status;

	wxString dummy;
	wxBool exists;
	if (ID_OK != (status=ReadValueWhileLocked(session, dummy, exists)) ||
	    ID_OK != (status=PocoGlue::StartTransaction(session)))
	{
		return status;
	}

	std::string key_string = std::string(m_key.ToUTF8());
	std::string value_string = std::string(value.ToUTF8());
	if (exists)
	{
		*session << "UPDATE property "\
		            "SET property_value = ? "\
		            "WHERE property_key = ?",
			Poco::Data::use(value_string),
			Poco::Data::use(key_string),
			Poco::Data::now;
	}
	else
	{
		*session << "INSERT INTO property "\
                "(property_key, property_value) "\
                "VALUES "\
                "(?, ?)",
			Poco::Data::use(key_string),
			Poco::Data::use(value_string),
			Poco::Data::now;
	}

	return PocoGlue::CommitTransaction(session);
}
