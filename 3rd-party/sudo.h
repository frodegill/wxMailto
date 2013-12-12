/* This file is very much based on work done by David Hart for his 4Pane */

/////////////////////////////////////////////////////////////////////////////
// Name:       Misc.h
// Purpose:    Misc stuff
// Part of:    4Pane
// Author:     David Hart
// Copyright:  (c) 2012 David Hart
// Licence:    wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/* This file will use hic copyright and his choice of license */

#ifndef _SUDO_H_
#define _SUDO_H_

#ifdef __GNUG__
  #pragma interface "sudo.h"
#endif
 

namespace wxMailto
{

// helper class for storing arguments as char** array suitable for passing to
// execvp(), whatever form they were passed to us
class ArgsArray
{
public:
	ArgsArray(const wxArrayString& args);
	ArgsArray(wchar_t** wargv);

	~ArgsArray();

	operator char**() const {return m_argv;}

private:
	void Init(int argc);

private:
	int m_argc;
	char **m_argv;
};

class Sudo
{
#define ERROR_RETURN_CODE -1    // We're always executing wxEXEC_SYNC

public:
	wxmailto_status ExecuteInPty(const wxString& cmd);

	wxArrayString& GetInputArray() {return m_input_array;}
	wxArrayString& GetOutputArray() {return m_output_array;}
	wxArrayString& GetErrorsArray() {return m_errors_array;}

private:
	PasswordManager* GetPasswordMgr();
	int ReadLine(int fd, wxString& line);

private:
	wxArrayString m_input_array;
	wxArrayString m_output_array;
	wxArrayString m_errors_array;
};

}

#endif // _SUDO_H_
