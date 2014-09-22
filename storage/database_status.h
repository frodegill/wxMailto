#ifndef _DATABASE_STATUS_H_
#define _DATABASE_STATUS_H_

// Copyright (C) 2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma interface "database_status.h"
#endif

//#include <wx/string.h>

//#include "../defines.h"
//#include "../wxmailto_errors.h"


namespace wxMailto
{

class DatabaseStatus {
public:
	enum Status {
		UNCHANGED,
		NEW,
		MODIFIED,
		DELETED
	};

public:
	DatabaseStatus();

	Status GetStatus() const {return m_status;}
	/*
	* @return Returns old status
	*/
	DatabaseStatus::Status SetStatus(Status status);

private:
	Status m_status;
};

}

#endif // _DATABASE_STATUS_H_
