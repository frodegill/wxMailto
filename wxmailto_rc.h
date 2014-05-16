#ifndef _WXMAILTO_RC_H_
#define _WXMAILTO_RC_H_

// Copyright (C) 2009-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "wxmailto_rc.h"
#endif

#include "wxmailto_errors.h"

namespace wxMailto
{

class IDManager {
public:
enum IDref {
//Dialog constants (ID)
ID_ABOUT_DIALOG=0,
ID_DATASOURCE_DIALOG,
ID_GPG_KEY_DIALOG,
ID_MODAL_MESSAGE_DIALOG,
ID_MODELESS_MESSAGE_DIALOG,
ID_NEWVERSION_DIALOG,
ID_PROGRESS_DIALOG,

//Dialog control constants (IDD)
IDD_CONNECTIONSTRING,
IDD_HTMLWINDOW,
IDD_MESSAGE,

//Button constants (IDB)
IDB_GENERATE_NEW_KEY,

//List constants (IDL)
IDL_KEYS,

//Menu constants (IDM)
IDM_CHECK_VERSION,

//Timer constants (IDT)
IDT_STATUSBAR0,
IDT_STATUSBAR1,
IDT_SYNC_ACCOUNT,

//Command events (IDC)
IDC_NEW_BETA_VERSION_AVAILABLE,
IDC_NEW_VERSION_AVAILABLE,
#if 0
IDC_SOCKET_EVENT,
#endif
IDC_VERSION_CHECK_FAILED,
IDC_VERSION_CHECK_OK,

//Threaded app events (IDTE)
IDTE_UPDATE_PROGRESS_EVENT,
IDTE_NEW_ACCOUNT_EVENT,
IDTE_MODIFY_ACCOUNT_EVENT,
IDTE_DELETE_ACCOUNT_EVENT,
IDTE_RESCHEDULE_ACCOUNT_SYNC,

TotalCount
};

public:
	IDManager();
	~IDManager();
	
	bool OnInit();
	wxWindowID GetId(IDref id_ref) const;
	
private:
	wxWindowID* m_ids;
};	

}

#endif // _WXMAILTO_RC_H_
