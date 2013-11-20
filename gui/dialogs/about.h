#ifndef _ABOUT_DLG_H_
#define _ABOUT_DLG_H_

// Copyright (C) 2009-2010  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "about.h"
#endif
 
#include <wx/dialog.h>

namespace wxMailto
{

class AboutDialog : public wxDialog
{
DECLARE_DYNAMIC_CLASS(AboutDialog)
public:
	AboutDialog(wxWindow* parent);

	virtual bool Show(bool show = true);

private:
	wxSizer* AboutDialogFunc(wxWindow* parent, bool call_fit=true, bool set_sizer=true);
};

}

#endif // _ABOUT_DLG_H_
