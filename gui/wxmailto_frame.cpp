
// Copyright (C) 2008-2013  Frode Roxrud Gill
// See LICENSE file for license

#ifdef __GNUG__
  #pragma implementation "wxmailto_frame.h"
#endif

#ifdef WX_PRECOMP
# include "../pch.h"
#else
# include <wx/config.h>
# include <wx/icon.h>
# include <wx/menu.h>
# include <wx/msgdlg.h>
#endif

#include "wxmailto_frame.h"

#include "wxmailto_app.h"
#include "app_module_manager.h"
#include "version_check.h"
#include "dialogs/about.h"
#include "dialogs/threaded_messagedialog.h"
#include "../wxmailto_rc.h"

using namespace wxMailto;

// the application icon
#include "../wxmailto.xpm"


BEGIN_EVENT_TABLE(wxMailto_Frame, wxFrame)
	EVT_MENU(wxID_EXIT,			wxMailto_Frame::OnQuit)
	EVT_MENU(wxID_ABOUT, 		wxMailto_Frame::OnAbout)
END_EVENT_TABLE()

IMPLEMENT_CLASS(wxMailto_Frame, wxFrame)

wxMailto_Frame::wxMailto_Frame(const wxString& title)
: wxFrame(),
	m_datasource_dlg(NULL),
	m_gpg_key_dlg(NULL),
	m_next_progress_id(0),
	m_progress_dlg(NULL),
  m_application_title(title),
  m_statusbar_timer0(this, wxGetApp().GetWindowID(IDManager::IDT_STATUSBAR0)),
  m_statusbar_timer1(this, wxGetApp().GetWindowID(IDManager::IDT_STATUSBAR1))
{
	Bind(wxEVT_COMMAND_MENU_SELECTED, &wxMailto_Frame::OnCheckVersion, this, wxGetApp().GetWindowID(IDManager::IDM_CHECK_VERSION));
	Bind(wxEVT_TIMER, &wxMailto_Frame::OnTimer, this, wxGetApp().GetWindowID(IDManager::IDT_STATUSBAR0));
	Bind(wxEVT_TIMER, &wxMailto_Frame::OnTimer, this, wxGetApp().GetWindowID(IDManager::IDT_STATUSBAR1));
	Bind(wxEVT_TIMER, &wxMailto_Frame::OnTimer, this, wxGetApp().GetWindowID(IDManager::IDT_SYNC_ACCOUNT));
	Bind(wxEVT_COMMAND_MENU_SELECTED, &wxMailto_Frame::OnVersionCheckFailed, this, wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_FAILED));
	Bind(wxEVT_COMMAND_MENU_SELECTED, &wxMailto_Frame::OnVersionCheckOK, this, wxGetApp().GetWindowID(IDManager::IDC_VERSION_CHECK_OK));
	Bind(wxEVT_COMMAND_MENU_SELECTED, &wxMailto_Frame::OnNewVersionAvailable, this, wxGetApp().GetWindowID(IDManager::IDC_NEW_VERSION_AVAILABLE));
	Bind(wxEVT_COMMAND_MENU_SELECTED, &wxMailto_Frame::OnNewBetaVersionAvailable, this, wxGetApp().GetWindowID(IDManager::IDC_NEW_BETA_VERSION_AVAILABLE));
	//
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnUpdateProgress, this, wxGetApp().GetWindowID(IDManager::IDTE_UPDATE_PROGRESS_EVENT));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnNewAccount, this, wxGetApp().GetWindowID(IDManager::IDTE_NEW_ACCOUNT_EVENT));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnModifyAccount, this, wxGetApp().GetWindowID(IDManager::IDTE_MODIFY_ACCOUNT_EVENT));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnDeleteAccount, this, wxGetApp().GetWindowID(IDManager::IDTE_DELETE_ACCOUNT_EVENT));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnRescheduleAccountSync, this, wxGetApp().GetWindowID(IDManager::IDTE_RESCHEDULE_ACCOUNT_SYNC));
	//
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnThreadedModelessMessagebox, this, wxGetApp().GetWindowID(IDManager::ID_MODELESS_MESSAGE_DIALOG));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnThreadedModalMessagebox, this, wxGetApp().GetWindowID(IDManager::ID_MODAL_MESSAGE_DIALOG));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnDatasourceDialog, this, wxGetApp().GetWindowID(IDManager::ID_DATASOURCE_DIALOG));
	Bind(wxEVT_THREAD, &wxMailto_Frame::OnGPGKeyDialog, this, wxGetApp().GetWindowID(IDManager::ID_GPG_KEY_DIALOG));

	//Init default values
	wxPoint pos = wxPoint(0,0);
	wxSize size = wxSize(500,400);
	wxBool is_maximized = false;

	//Read from config
	wxConfigBase* config = wxConfigBase::Get();
	if (config)
	{
		config->Read("App X", &pos.x, pos.x);
		config->Read("App Y", &pos.y, pos.y);
		int width,height;
		config->Read("App Width", &width, size.GetWidth());
		config->Read("App Height", &height, size.GetHeight());
		size.Set(width,height);
		config->Read("App Maximize", &is_maximized, false);
	}

	/*
	** Create frame
	*/
	Create(NULL, wxID_ANY, title, pos, size);
	Maximize(is_maximized);

	SetIcon(wxIcon(wxmailto_xpm));

	wxMenuBar* main_menu = new wxMenuBar();
	wxMenu* file_menu = new wxMenu;
	wxMenu* help_menu = new wxMenu;
	if (!main_menu || !file_menu || !help_menu)
	{
		delete main_menu;
		delete file_menu;
		delete help_menu;
		wxASSERT_MSG(false, _("menu is NULL"));
		wxGetApp().RequestExit();
		return;
	}
	
	//File menu
	file_menu->Append(wxID_EXIT, _("E&xit\tAlt-X"), _("Quit this program"));

	//Help menu
	help_menu->Append(wxGetApp().GetWindowID(IDManager::IDM_CHECK_VERSION), _("&Check for update"), _("Contacts server to see if a more recent version of "WXMAILTO_TITLE" is available"));
	help_menu->AppendSeparator();
	help_menu->Append(wxID_ABOUT, _("&About...\tF1"), _("Show about dialog"));

	//Main menu
	main_menu->Append(file_menu, _("&File"));
	main_menu->Append(help_menu, _("&Help"));
	SetMenuBar(main_menu);

	CreateStatusBar(2);

	SetTitle(m_application_title);
}

wxMailto_Frame::~wxMailto_Frame()
{
	wxConfigBase* config = wxConfigBase::Get();
	if (config)
	{
		config->Write("App X", GetPosition().x);
		config->Write("App Y", GetPosition().y);
		config->Write("App Width", GetSize().GetWidth());
		config->Write("App Height", GetSize().GetHeight());
		config->Write("App Maximize", IsMaximized());
	}
}

void wxMailto_Frame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	wxGetApp().RequestExit();
}

void wxMailto_Frame::OnCheckVersion(wxCommandEvent& WXUNUSED(event))
{
	CheckVersion();
}

void wxMailto_Frame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	AboutDialog about(this);
	about.ShowModal();
}

void wxMailto_Frame::OnTimer(wxTimerEvent& event)
{
	wxWindowID id = event.GetId();
	if (wxGetApp().GetWindowID(IDManager::IDT_STATUSBAR0) == id)
	{
		m_statusbar_timer0.Stop();
		SetStatusText(wxEmptyString, 0);
	}
	else if (wxGetApp().GetWindowID(IDManager::IDT_STATUSBAR1) == id)
	{
		m_statusbar_timer1.Stop();
		SetStatusText(wxEmptyString, 1);
	}
#if 0
	else if (wxGetApp().GetWindowID(IDManager::IDT_SYNC_ACCOUNT) == id)
	{
		wxGetApp().GetAccountManager()->OnSyncAccount();
	}
#endif
}

void wxMailto_Frame::OnVersionCheckFailed(wxCommandEvent& WXUNUSED(event))
{
	SetTimeoutStatusText(_("Failed to fetch version information"), 1, 10);
}

void wxMailto_Frame::OnVersionCheckOK(wxCommandEvent& WXUNUSED(event))
{
	SetTimeoutStatusText(_("Version check completed - No new release"), 1, 10);
}

void wxMailto_Frame::OnNewVersionAvailable(wxCommandEvent& event)
{
	SetStatusText(wxEmptyString, 1);
	NewVersionDialog newversion(this, event.GetString(), false);
	newversion.ShowModal();
}

void wxMailto_Frame::OnNewBetaVersionAvailable(wxCommandEvent& event)
{
#if WXMAILTO_VERSION_BETA
	SetStatusText(wxEmptyString, 1);
	NewVersionDialog newversion(this, event.GetString(), true);
	newversion.ShowModal();
#else
	if (event.GetString())
	{
		SetTimeoutStatusText(_("A new release of "WXMAILTO_TITLE" is now available"), 1, 10);
	}
	else
	{
		SetTimeoutStatusText(wxString::Format(_("A new release of "WXMAILTO_TITLE" is now available at %s"), event.GetString().c_str()), 1, 10);
	}
#endif
}

//Threaded app events
void wxMailto_Frame::OnNewAccount(wxThreadEvent& WXUNUSED(event))
{
}

void wxMailto_Frame::OnModifyAccount(wxThreadEvent& WXUNUSED(event))
{
}

void wxMailto_Frame::OnDeleteAccount(wxThreadEvent& WXUNUSED(event))
{
}

void wxMailto_Frame::OnRescheduleAccountSync(wxThreadEvent& WXUNUSED(event))
{
#if 0
	AccountManager* account_manager = wxGetApp().GetAppModuleManager()->GetAccountManager();
	const Account* account = account_manager->GetAccount(event.GetInt());
	account_manager->SyncAccount(account->GetAccountId(), account->GetPollInterval());
#endif
}

//Dialogs
void wxMailto_Frame::OnThreadedModelessMessagebox(wxThreadEvent& event)
{
	ThreadedModelessMessageDialog* dlg = new ThreadedModelessMessageDialog(this); //If this OOMs, we have trouble
	ThreadedModelessDialogLauncherData* launcher_data = event.GetPayload<ThreadedModelessDialogLauncherData*>();
	wxASSERT_MSG(launcher_data, _("Launcher is NULL"));
	if (launcher_data)
	{
		dlg->Show(launcher_data->m_message, launcher_data->m_caption);
		delete launcher_data;
	}
}

void wxMailto_Frame::OnThreadedModalMessagebox(wxThreadEvent& event)
{
	ThreadedModalMessageDialog* dlg = new ThreadedModalMessageDialog(this); //If this OOMs, we have trouble
	dlg->AddLauncher(event.GetPayload<ThreadedModalDialogLauncherData*>());
	dlg->Show("message", "caption");
}

void wxMailto_Frame::OnDatasourceDialog(wxThreadEvent& event)
{
	{
		wxCriticalSectionLocker locker(m_datasource_dlg_lock);

		wxBool dialog_already_open = NULL!=m_datasource_dlg;
		
		if (!dialog_already_open)
		{
      m_datasource_dlg = new DatasourceDialog(this, &m_datasource_dlg_lock); //If this OOMs, we have trouble
		}

		m_datasource_dlg->AddLauncher(event.GetPayload<ThreadedModalDialogLauncherData*>());

		if (!dialog_already_open)
		{
			m_datasource_dlg->Show();
		}
	}
}

void wxMailto_Frame::DestroyDatasourceDialog()
{
	{
		wxCriticalSectionLocker locker(m_datasource_dlg_lock);
		
		if (!m_datasource_dlg)
		{
			return;
		}

		m_datasource_dlg->Destroy();

		m_datasource_dlg = NULL;
	}
}

void wxMailto_Frame::OnGPGKeyDialog(wxThreadEvent& event)
{
	{
		wxCriticalSectionLocker locker(m_gpg_key_dlg_lock);

		wxBool dialog_already_open = NULL!=m_gpg_key_dlg;
		
		if (!dialog_already_open)
		{
      m_gpg_key_dlg = new GPGKeyDialog(this, &m_gpg_key_dlg_lock); //If this OOMs, we have trouble
		}

		m_gpg_key_dlg->AddLauncher(event.GetPayload<ThreadedModalDialogLauncherData*>());

		if (!dialog_already_open)
		{
			m_gpg_key_dlg->Show();
		}
	}
}

void wxMailto_Frame::DestroyGPGKeyDialog()
{
	{
		wxCriticalSectionLocker locker(m_gpg_key_dlg_lock);
		
		if (!m_gpg_key_dlg)
		{
			return;
		}

		m_gpg_key_dlg->Destroy();

		m_gpg_key_dlg = NULL;
	}
}

void wxMailto_Frame::OnUpdateProgress(wxThreadEvent& event)
{
	{
		wxCriticalSectionLocker locker(m_progress_dlg_lock);

		if (!m_progress_dlg)
		{
      m_progress_dlg = new ProgressDialog(this); //If this OOMs, we have trouble
      m_progress_dlg->Show();
		}
		
		m_progress_dlg->UpdateProgress(event.GetPayload<ProgressInfo*>());
	}
}

void wxMailto_Frame::DestroyProgressDialog()
{
	{
		wxCriticalSectionLocker locker(m_progress_dlg_lock);
		
		if (!m_progress_dlg)
		{
			return;
		}

		m_progress_dlg->Destroy();
		m_progress_dlg = NULL;
	}
}

void wxMailto_Frame::SetTimeoutStatusText(const wxString& text, wxInt number, wxInt timeout)
{
	SetStatusText(text, number);

	if (0 == number)
	{
		m_statusbar_timer0.Stop();
		m_statusbar_timer0.Start(timeout*1000, true);
	}
	else if (1 == number)
	{
		m_statusbar_timer1.Stop();
		m_statusbar_timer1.Start(timeout*1000, true);
	}
	wxASSERT_MSG(number>=0 && number<=1, _("Only 0 and 1 currently supported"));
}

wxmailto_status wxMailto_Frame::CheckVersion()
{
#if WXMAILTO_VERSION_BETA
	wxDateTime compile_date;
	if ((wxChar*)NULL != compile_date.ParseFormat(wxT(__DATE__), "%b %d %Y", wxDateTime((time_t)0)))
	{
		wxInt remaining_days = BETA_DAYS_TO_LIVE - wxDateTime::Today().Subtract(compile_date).GetDays();
		if (remaining_days <= 0)
		{
			::wxMessageBox(_("This BETA version has expired"), _("Version Error"), wxOK|wxICON_INFORMATION, this);
			wxGetApp().RequestExit();
			return LOGERROR(ID_EXPIRED);
		}
		else if (remaining_days <= 7)
		{
			::wxMessageBox(wxString::Format(_("This BETA version will expire in %d day(s)"), remaining_days), _("Version Warning"), wxOK|wxICON_INFORMATION, this);
		}
	}
	else
	{
		wxASSERT_MSG(false, _("Parsing compiled date failed"));
	}
#endif

	VersionCheck* version_check = new VersionCheck(this);
	if (!version_check)
		return LOGERROR(ID_NULL_POINTER);

	if (wxTHREAD_NO_ERROR != version_check->Create())
	{
		wxASSERT_MSG(false, _("Failed to create thread"));
		version_check->Delete();
		return LOGERROR(ID_GENERIC_ERROR);
	}

	SetTimeoutStatusText(_("Checking version information"), 1, 10);
	version_check->Run(); //Thread will delete itself
	return ID_OK;
}
