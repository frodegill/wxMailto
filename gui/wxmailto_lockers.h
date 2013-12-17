#ifndef _WXMAILTO_LOCKERS_H_
#define _WXMAILTO_LOCKERS_H_

// Copyright (C) 2012-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "wxmailto_lockers.h"
#endif

#include <wx/thread.h>


namespace wxMailto
{

struct GlobalLockers {
	wxCriticalSection m_block_exit_lock;
	wxCriticalSection m_next_credential_id_lock;
#if 0
	wxCriticalSection m_generic_property_lock;
	wxCriticalSection m_next_multipart_id_lock;
#endif
};

}

#endif // _WXMAILTO_LOCKERS_H_
