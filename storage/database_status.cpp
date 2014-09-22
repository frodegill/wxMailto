
// Copyright (C) 2012  Frode Roxrud Gill
// See license.h for License

#ifdef __GNUG__
  #pragma implementation "database_status.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "database_status.h"

using namespace wxMailto;


DatabaseStatus::DatabaseStatus()
: m_status(UNCHANGED)
{
}

DatabaseStatus::Status DatabaseStatus::SetStatus(Status status)
{
	Status old_status = m_status;
	if (NEW==old_status && DELETED==status)
	{
		m_status = UNCHANGED;
	}
	else
	{
		m_status = status;
	}
	return old_status;
}
