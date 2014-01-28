#ifndef _WXMAILTO_DEFINES_H_
#define _WXMAILTO_DEFINES_H_

// Copyright (C) 2008-2010  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma interface "defines.h"
#endif

#include <wx/defs.h>

#define WXMAILTO_LICENSE "GPLv. See LICENSE file for license"

#define WXMAILTO_TITLE	"wxMailto:"
#define WXMAILTO_VERSION	(0.01)
#define WXMAILTO_VERSION_STR	"0.01"
#define	WXMAILTO_VERSION_BETA	1
#define BETA_DAYS_TO_LIVE	365
#define WXMAILTO_HOMEPAGE	"http://gill-roxrud.dyndns.org/wxMailto/"
#define VERSION_SERVERNAME	"gill-roxrud.dyndns.org"
#define VERSION_URI	"/wxmailto.version.xml"
#define FORCE_VERSION_CHECK		1

typedef bool	wxBool;
typedef int	wxInt;
typedef unsigned int	wxUInt;
typedef long	wxLong;
typedef size_t wxSizeT;
typedef wxUint32	wxMessageId;

#ifndef MIN_FUNC
#define MIN_FUNC
static inline wxInt SMIN(wxInt a, wxInt b) {return a>b ? a : b;}
#endif
#ifndef UMIN_FUNC
#define UMIN_FUNC
static inline wxSizeT UMIN(wxSizeT a, wxSizeT b) {return a>b ? a : b;}
#endif
#ifndef DMIN_FUNC
#define DMIN_FUNC
static inline wxDouble DMIN(wxDouble a, wxDouble b) {return a<b ? a : b;}
#endif

#ifndef MAX_FUNC
#define MAX_FUNC
static inline wxInt SMAX(wxInt a, wxInt b) {return a<b ? a : b;}
#endif
#ifndef UMAX_FUNC
#define UMAX_FUNC
static inline wxSizeT UMAX(wxSizeT a, wxSizeT b) {return a<b ? a : b;}
#endif
#ifndef DMAX_FUNC
#define DMAX_FUNC
static inline wxDouble DMAX(wxDouble a, wxDouble b) {return a>b ? a : b;}
#endif


#define RANDOM(x) rand()/(RAND_MAX+1.0)*(x)

#endif // _WXMAILTO_DEFINES_H_
