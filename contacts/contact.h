#ifndef _CONTACT_H_
#define _CONTACT_H_

// Copyright (C) 2009-2010  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "contact.h"
#endif

#include <wx/datetime.h>
#include <wx/list.h>
#include "../defines.h"
#include "../wxmailto_errors.h"

namespace wxMailto
{

class Contact;
WX_DECLARE_LIST(Contact, ContactList);

class Contact
{
public:
  enum ContactType {CONTACT_PERSON, CONTACT_GROUP};

public:
	Contact();
	virtual ~Contact();

	ContactType GetContactType() const {return m_contact_type;}
	wxBool IsInValidPeriod(const wxDateTime& time) const {return m_valid_from<=time && (wxDefaultDateTime==m_valid_to || time<=m_valid_to);}

	wxDateTime GetValidFrom() const {return m_valid_from;}
	void SetValidFrom(const wxDateTime& time) {m_valid_from=time;}
	wxDateTime GetValidTo() const {return m_valid_to;}
	void SetValidTo(const wxDateTime& time) {m_valid_to=time;}
	const Contact* GetCurrentValidContact() const;

	wxBool HasGroups() const {return m_groups && 0<m_groups->GetCount();}
	wxBool HasChildren() const {return m_children && 0<m_children->GetCount();}
	ContactList* GetChildren() const {return m_children;}

	wxString GetName() const {return m_name;}
	wxmailto_status SetName(const wxString& name) {m_name=name; return ID_OK;} //ToDo, persist
	wxString GetEmail() const {return m_email;}
	wxmailto_status SetEmail(const wxString& email) {m_email=email; return ID_OK;} //ToDo, handle IDNA, persist
	wxString GetIdnaEmail() const {return m_email;} //ToDo
	wxString GetComment() const {return m_comment;}
	wxmailto_status SetComment(const wxString& comment) {m_comment=comment; return ID_OK;} //ToDo, persist

	wxmailto_status GetRFC2822Address(wxString& address) const;

private:
	ContactType m_contact_type;
	wxDateTime m_valid_from;
	wxDateTime m_valid_to;
	ContactList* m_groups;
	ContactList* m_children;
	Contact* m_obsoleted_by;
	ContactList* m_obsoletes;
	wxString m_name;
	wxString m_email;
	wxString m_comment;
};

}

#endif // _CONTACT_H_
