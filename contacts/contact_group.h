#ifndef _CONTACT_GROUP_H_
#define _CONTACT_GROUP_H_

// Copyright (C) 2009-2010  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "contact_group.h"
#endif

#include <wx/list.h>
#include "../defines.h"


namespace wxMailto
{

class ContactGroup
{
public:
	ContactGroup();
	virtual ~ContactGroup();

};

WX_DECLARE_LIST(ContactGroup, ContactGroupList);

}

#endif // _CONTACT_GROUP_H_
