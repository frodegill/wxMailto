#ifndef _DATASOURCE_DLG_H_
#define _DATASOURCE_DLG_H_

// Copyright (C) 2009-2014  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "datasource.h"
#endif
 
#include "threaded_dialog.h"
#include "../../defines.h"

namespace wxMailto
{

#define DSN_SALT "dsn@wxMailto"


class DatasourceDialog : public ThreadedModalDialog
{
DECLARE_DYNAMIC_CLASS(DatasourceDialog)
public:
	DatasourceDialog(wxWindow* parent, wxCriticalSection* dialog_critical_section);

	bool Show();

	void OnOk(wxCommandEvent& event);

private:
	wxSizer* DatasourceDialogFunc(wxWindow* parent, wxBool call_fit=true, wxBool set_sizer=true);

private:
	wxString* m_connectionstring;

private:
    DECLARE_EVENT_TABLE()
};

}

#endif // _DATASOURCE_DLG_H_
