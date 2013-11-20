
// Copyright (C) 2010-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "wxmailto_module.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
#endif

#include "wxmailto_module.h"
#include INCLUDE_LOG1

using namespace wxMailto;

wxMailto_Module::wxMailto_Module()
: m_dependency_condition(NULL),
  m_dependency_count(0)
{
}

wxMailto_Module::~wxMailto_Module()
{
	if (m_dependency_condition)
		delete m_dependency_condition;
}

wxmailto_status wxMailto_Module::AddModuleDependency()
{
	{
		wxMutexLocker locker(m_dependency_lock);
		m_dependency_count++;

		if (!m_dependency_condition)
			m_dependency_condition = new wxCondition(m_dependency_lock);

		if (!m_dependency_condition)
			return LOGERROR(ID_OUT_OF_MEMORY);
	}
	return ID_OK;
}

wxmailto_status wxMailto_Module::RemoveModuleDependency()
{
	wxASSERT(0 < m_dependency_count);
	{
		wxMutexLocker locker(m_dependency_lock);
		if (0 == --m_dependency_count)
		{
			m_dependency_condition->Broadcast();
		}
	}
	return ID_OK;
}

void wxMailto_Module::WaitForNoMoreDependencies()
{
	{
		wxMutexLocker locker(m_dependency_lock);
		if (0 == m_dependency_count)
			return;

		m_dependency_condition->Wait();
	}
}
