// Copyright (C) 2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "wxmailto_rc.h"
#endif

#ifdef WX_PRECOMP
# include "pch.h"
#else
#endif

#include <wx/window.h>

#include "wxmailto_rc.h"


using namespace wxMailto;


IDManager::IDManager()
: m_ids(NULL)
{
}

IDManager::~IDManager()
{
	delete[] m_ids;
}
	
bool IDManager::OnInit()
{
	size_t count = TotalCount;
	if (NULL==(m_ids=new wxWindowID[count]))
		return false;

	size_t i;
	for (i=0; i<count; i++)
	{
		m_ids[i] = wxWindow::NewControlId();
	}
	return true;
}

wxWindowID IDManager::GetId(IDref id_ref) const
{
	return m_ids[id_ref];
}
