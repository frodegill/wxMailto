
// Copyright (C) 2009-2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "contact.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "contact.h"
#include INCLUDE_LOG1

using namespace wxMailto;

# include <wx/listimpl.cpp>
WX_DEFINE_LIST(ContactList);


Contact::Contact()
: m_contact_type(CONTACT_PERSON),
  m_groups(NULL),
  m_children(NULL),
  m_obsoleted_by(NULL),
  m_obsoletes(NULL)
{
}

Contact::~Contact()
{
	delete m_groups;
	delete m_children;
	delete m_obsoleted_by;
	delete m_obsoletes;
}

const Contact* Contact::GetCurrentValidContact() const
{
	wxDateTime nowUTC = wxDateTime::Now();

	//Is this contact not valid yet?
	if (m_valid_from>nowUTC)
		return NULL;

	//Not valid anymore?
	if (m_valid_to!=wxDefaultDateTime && m_valid_to<nowUTC)
	{
		return NULL!=m_obsoleted_by ? m_obsoleted_by->GetCurrentValidContact() : NULL;
	}

	return this;
}

wxmailto_status Contact::GetRFC2822Address(wxString& address) const
{
	wxmailto_status status;
	address.Empty();

	if (CONTACT_GROUP==m_contact_type)
	{
		if (m_name.IsEmpty())
			return LOGERROR(ID_CONTACT_GROUP_WITHOUT_NAME);

		address.Append(m_name);
		address.Append(":");

		wxString child_address;
		wxBool first = true;
		ContactList::iterator iter;
		for (iter=m_children->begin(); iter!=m_children->end(); ++iter)
		{
			const Contact* current = *iter;
			if (!current) continue;
			current = current->GetCurrentValidContact();
			if (!current)
				return LOGERROR(ID_CONTACT_NO_VALID_ADDRESS_FOR_PERIOD);

 			if (ID_OK != (status=current->GetRFC2822Address(child_address)))
				return status;

			if (!first)
			{
				address.Append(", ");
			}
			address.Append(m_name);
			first = false;
		}
		address.Append(";");
	}
	else if (CONTACT_PERSON==m_contact_type)
	{
		if (m_email.IsEmpty()) //ToDo: Handle mail groups
			return LOGERROR(ID_CONTACT_WITHOUT_EMAIL);

		if (!m_name.IsEmpty())
		{
			address.Append("\"");
			address.Append(m_name);
			address.Append("\" ");
		}
	
		address.Append("<");
		address.Append(m_email);
		address.Append(">");
	
		if (!m_comment.IsEmpty())
		{
			address.Append(" (");
			address.Append(m_comment);
			address.Append(" )");
		}
	}
	else
	{
		return LOGERROR(ID_CONTACT_INVALID_TYPE);
	}

	return ID_OK;
}
