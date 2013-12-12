/* This file is very much based on work done by David Hart for his 4Pane */

/////////////////////////////////////////////////////////////////////////////
// Name:        Misc.cpp
// Purpose:     Misc stuff
// Part of:     4Pane
// Author:      David Hart
// Copyright:   (c) 12 David Hart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/* This file will use hic copyright and his choice of license */

#ifdef __GNUG__
  #pragma implementation "sudo.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/cmdline.h>
#endif

#include <errno.h>
#include <pty.h>
#include <sys/wait.h>

#include "../gui/wxmailto_app.h"
#include "../gui/app_module_manager.h"
#include "../wxmailto_errors.h"
#include "sudo.h"


using namespace wxMailto;


ArgsArray::ArgsArray(const wxArrayString& args)
{
	Init(args.size());
	int i;
	for (i=0; i<m_argc; i++)
	{
		m_argv[i] = wxStrdup(args[i]);
	}
}

ArgsArray::ArgsArray(wchar_t **wargv)
{
	int argc = 0;
	while (wargv[argc])
		argc++;

	Init(argc);
	int i;
	for (i=0; i<m_argc; i++)
	{
		m_argv[i] = wxSafeConvertWX2MB(wargv[i]).release();
	}
}

ArgsArray::~ArgsArray()
{
	int i;
	for (i=0; i<m_argc; i++)
	{
		free(m_argv[i]);
	}
	delete[] m_argv;
}

void ArgsArray::Init(int argc)
{
	m_argc = argc;
	m_argv = new char*[m_argc+1];
	m_argv[m_argc] = NULL;
}



wxmailto_status Sudo::ExecuteInPty(const wxString& cmd)
{
	if (cmd.empty())
		return ID_INVALID_FORMAT;

	ArgsArray argv(wxCmdLineParser::ConvertStringToArgs(cmd, wxCMD_LINE_SPLIT_UNIX));

	m_output_array.Clear();
	m_errors_array.Clear();
	size_t count = 0;
	int ret;

	int fd;
	pid_t pid = forkpty(&fd, NULL, NULL, NULL);
	if (-1 == pid)
	{
		return LOGERROR_MSG(ID_GENERIC_ERROR, "Failed to create a separate process");
	}

	if (0 == pid)                                   // The child process
	{
		setsid();

		struct termios tos;                         // Turn off echo
		tcgetattr(STDIN_FILENO, &tos);
		tos.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
		tos.c_oflag &= ~(ONLCR);
		tcsetattr (STDIN_FILENO, TCSANOW, &tos);

		if (0 != (ret=(-1 == execvp(*argv, argv))))
		{
			close(fd);
			return LOGERROR_MSG(ID_GENERIC_ERROR, wxString::Format("program exited with code %i\n", ret));
		}
	}
                                                // The parent process
	int fl;
	if (-1 == (fl=fcntl(fd, F_GETFL, 0)))
		fl = 0;

	fcntl(fd, F_SETFL, fl | O_NONBLOCK);            // Make non-blocking   

	int status;
	ret = 1;
 
	fd_set fd_in, fd_out;
	do
	{
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 20000;

		FD_ZERO(&fd_in);
		FD_ZERO(&fd_out);
		FD_SET(fd, &fd_in);
		FD_SET(fd, &fd_out);

		int rc = select(fd + 1, &fd_in, &fd_out, NULL, &tv);
		if (-1 == rc)
		{
			close(fd);
			return LOGERROR_MSG(ID_GENERIC_ERROR, wxString::Format("Error %d on select()", errno));
		}

		if (!rc)
			continue;

		if (FD_ISSET(fd, &fd_in))
		{
			wxString line;
			ret = ReadLine(fd, line);

			if (!ret)
				continue;                     // We've temporarily read all the data, but there should be more. Reselect

			if (-1 == ret)
			{
				if (line.Len())
					m_output_array.Add(line);   // There's a problem, but salvage any data we've already read

				m_errors_array = m_output_array;
				close(fd);
				return LOGERROR(ID_GENERIC_ERROR);
			}

			// If we're here, ret was >0. 1 means there's a full line; 2 that we got EAGAIN, which may mean it's waiting for a password
			if (FD_ISSET(fd, &fd_out) && line.Lower().Contains("password") && !line.Lower().Contains("incorrect"))
			{				
				wxString pw;
				if (ID_OK!=GetPasswordMgr()->GetSudoPassword(pw) || pw.empty())
				{
					close(fd);
					return LOGERROR(ID_GENERIC_ERROR); // Presumably the user cancelled
				}

				pw << '\n';
				if (-1 == write(fd, pw.mb_str(wxConvUTF8), pw.Len()))
				{
					close(fd);
					return LOGERROR_MSG(ID_GENERIC_ERROR, "Writing the string failed");
				}
			}
			// Either a single 'su' failure (you only get 1 chance) or 3 'sudo' ones
			else if (line.Lower().Contains("authentication failure") ||
			    line.Lower().Contains("incorrect password"))
			{
				GetPasswordMgr()->ForgetSudoPassword();  // Forget the failed attempt, or it'll automatically be offered again and again and...
				m_output_array.Add(line);               // Store the 'fail' line so that it's available to show the user
			}
			else if (line.Lower().Contains("try again"))   // The first or second sudo failure
			{
				GetPasswordMgr()->ForgetSudoPassword();
			}
			else
			{
				if (!(m_output_array.IsEmpty() && line.IsSameAs('\r')))  // When a password is accepted, su emits a \r for some reason
						m_output_array.Add(line);
			}

			continue;
		}

		if (FD_ISSET(fd, &fd_out))
		{
#if 0
			if (!GetCallerTextCtrl())
			{ // Though there shouldn't be any input required for these commands (apart from the password, handled above)
				// for some reason we do land here several times per command. So we can't just abort, and 'continue' seems to work
				// Nevertheless limit the number of tries, just in case someone does try an input-requiring command
				if (1000 > ++count)
				{
					wxMilliSleep(10);
					if (!(count % 10))
					{
						wxYieldIfNeeded();
					}
					continue;
				}

				close(fd);
				return LOGERROR_MSG(ID_GENERIC_ERROR, "Expecting input that we can't provide. Bail out to avoid a hang");
			}
#endif
			if (!m_input_array.empty())
			{
				wxString line = m_input_array.Item(0);
				m_input_array.RemoveAt(0);
				line << '\n';                            // as this will have been swallowed by the textctrl
				if (-1 == write(fd, line.mb_str(wxConvUTF8), line.Len()))
				{
					close(fd);
					return LOGERROR_MSG(ID_GENERIC_ERROR, "Writing the string failed");
				}

				count = 0;                                    // reset the anti-hang device
				wxYieldIfNeeded();
			}
			else
			{
				if (1000 > ++count)
				{
					wxMilliSleep(10);
					if (!(count % 10))
					{
						wxYieldIfNeeded();
					}
					continue;
				}

				close(fd);
				return LOGERROR_MSG(ID_GENERIC_ERROR, "\nTimed out\n"); // If we're here, bail out to avoid hanging
			}
		}
	}
	while (0 <= ret);

	while (!m_output_array.IsEmpty())                       // Remove any terminal emptyish lines
	{
		wxString last(m_output_array.Last());
		if (last.empty() || last.IsSameAs('\n') || last.IsSameAs('\r'))
			m_output_array.RemoveAt(m_output_array.GetCount()-1); 
		else
			break;
	}

	waitpid(pid, &status, 0);

	close(fd);

	if (WIFEXITED(status))
	{
		int retcode = WEXITSTATUS(status);
		if (0 < retcode)
		{
			m_errors_array = m_output_array;
			m_output_array.Clear();
		} // If there's been a problem, it makes more sense for the output to be here

		return (0==retcode) ? ID_OK : ID_GENERIC_ERROR;
	}

	return ID_GENERIC_ERROR;  // For want of something more sensible. We should seldom reach here anyway; only from signals, I think
}

PasswordManager* Sudo::GetPasswordMgr()
{
	return wxGetApp().GetAppModuleManager()->GetPasswordManager();
}

int Sudo::ReadLine(int fd, wxString& line)
{
	wxMemoryBuffer buf;
	line.Empty();

	while (true)
	{
		char c;
		int nread = read(fd, &c, 1);                 // Get a char from the input

		if (-1 == nread)
		{
			if (buf.GetDataLen())                   // There may be a problem: salvage the data we've already read
			{
				wxString ln(static_cast<const char*>(buf.GetData()), wxConvUTF8, buf.GetDataLen());
				if (0 < ln.Len())
				{
					line = ln;
				}
			}
			if ((EAGAIN == errno) || (0 == errno))
				return 2; // No more data is available atm. That may be because a line doesn't end in \n e.g. 'Password: '

			if (EIO == errno)
				return -2;            // This is the errno we get when the slave has died, presumably successfully

			wxLogDebug("Error %d while trying to read input", errno);
			return -1;
		}

		if (0 == nread)
			return 0;                   // We've got ahead of the available data

		if (c == '\n')
		{
			wxString ln(static_cast<const char*>(buf.GetData()), wxConvUTF8, buf.GetDataLen()); // Convert the line to utf8
			line = ln;
			return 1;
		}

		buf.AppendByte(c);                          // Otherwise just append the char and ask for more please
	}
}
