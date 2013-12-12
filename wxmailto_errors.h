#ifndef _WXMAILTO_ERRORS_H_
#define _WXMAILTO_ERRORS_H_

// Copyright (C) 2009-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "wxmailto_errors.h"
#endif

#include <wx/debug.h> 

#include "defines.h"

namespace wxMailto
{

#ifdef __WXDEBUG__
# define LOGERROR(x) (wxGetApp().LogError(x))
# define LOGWARNING(x) (wxGetApp().LogWarning(x))
# define LOGERROR_MSG(x,y) (wxGetApp().LogErrorMsg(x,(y)))
# define INCLUDE_LOG1 "../gui/wxmailto_app.h"	
#else
# define LOGERROR(x) (x)
# define LOGWARNING(x) (x)
# define LOGERROR_MSG(x,y) (x)
# define INCLUDE_LOG1 "../wxmailto_errors.h" 
#endif

enum wxmailto_status
{
	ID_OK=0,
	ID_UNINITIALIZED,
	ID_GENERIC_ERROR,
	ID_OUT_OF_MEMORY,
	ID_NULL_POINTER,
	ID_INVALID_FORMAT,
	ID_NOT_IMPLEMENTED,
	ID_EXIT_REQUESTED,
	ID_SHOULDNT_GET_HERE,

	//Contacts
#if 0
	ID_CONTACT_GROUP_WITHOUT_NAME,
	ID_CONTACT_WITHOUT_EMAIL,
	ID_CONTACT_INVALID_TYPE,
	ID_CONTACT_NO_VALID_ADDRESS_FOR_PERIOD,
#endif
	//ODBC
	ID_INVALID_DATASOURCE,
#if 0
	ID_DATASOURCE_ERROR,
	ID_NO_DATA,
#endif
	//Protocols
#if 0
	ID_UNKNOWN_SERVER,
	ID_CONNECTION_FAILED,
	ID_NOT_CONNECTED,
	ID_INVALID_CONVERTER,
	ID_NETWORK_ERROR,
	ID_ERROR_FROM_SERVER,
	ID_AUTHENTICATION_FAILED,
	ID_AUTHENTICATION_NOT_SUPPORTED,
	ID_ALREADY_SYNCING,
#endif
	//POP
#if 0
	ID_POP_LOGIN_DELAY, //RFC2449 §8.1.1
	ID_POP_IN_USE, //RFC2449 §8.1.2
#endif
	//Version
#if 0
	ID_UPDATE_DATABASE_FAILED,
#endif
	ID_EXPIRED
};

}

#endif // _WXMAILTO_ERRORS_H_
