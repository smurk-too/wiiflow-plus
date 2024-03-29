#include "svnrev.h"
#include "menu.hpp"

#include "loader/sys.h"
#include "loader/wbfs.h"
#include "gecko.h"
#include "lockMutex.hpp"
#include "defines.h"

#define newIOS 249

extern int mainIOS;

int version_num = 0, num_versions = 0, i;
int CMenu::_version[9] = {0, atoi(SVN_REV), atoi(SVN_REV), atoi(SVN_REV), atoi(SVN_REV), atoi(SVN_REV), atoi(SVN_REV), atoi(SVN_REV), atoi(SVN_REV)};

void CMenu::_system()
{
	int msg = 0, newVer = atoi(SVN_REV);
	lwp_t thread = LWP_THREAD_NULL;
	wstringEx prevMsg;

	SetupInput();
	m_btnMgr.setText(m_systemBtnBack, _t("dl1", L"Cancel"));
	m_thrdStop = false;
	m_thrdMessageAdded = false;
	m_showtimer = -1;
	while (true)
	{
		_mainLoopCommon(false, m_thrdWorking);
		if (m_showtimer == -1)
		{
			m_showtimer = 120;
			m_btnMgr.show(m_downloadPBar);
			m_btnMgr.setProgress(m_downloadPBar, 0.f);
			m_thrdStop = false;
			m_thrdWorking = true;
			LWP_CreateThread(&thread, (void *(*)(void *))CMenu::_versionTxtDownloaderInit, (void *)this, 0, 8192, 40);
		}
		if (m_showtimer > 0 && !m_thrdWorking)
		{
			if (thread != LWP_THREAD_NULL)
			{
				if(LWP_ThreadIsSuspended(thread))
					LWP_ResumeThread(thread);

				LWP_JoinThread(thread, NULL);
				thread = LWP_THREAD_NULL;
			}
			if (--m_showtimer == 0)
			{
				m_btnMgr.hide(m_downloadPBar);
				m_btnMgr.hide(m_downloadLblMessage[0], 0, 0, 1.f, 0.f);
				m_btnMgr.hide(m_downloadLblMessage[1], 0, 0, 1.f, 0.f);
				CMenu::_version[1] = m_version.getInt("GENERAL", "version", atoi(SVN_REV));
				num_versions = m_version.getInt("GENERAL", "num_versions", 1);
				for (i = 2; i < num_versions; i++)
				{
					CMenu::_version[i] = m_version.getInt(fmt("VERSION%i", i-1u), "version", atoi(SVN_REV));
					//add the changelog info here
				}
				if (num_versions > 1 && version_num == 0) version_num = 1;
				i = min((u32)version_num, ARRAY_SIZE(CMenu::_version) -1u);
				newVer = CMenu::_version[i];
				_showSystem();
			}
		}
		if ((BTN_HOME_PRESSED || BTN_B_PRESSED || m_exit) && !m_thrdWorking)
			break;
		else if (BTN_UP_PRESSED)
			m_btnMgr.up();
		else if (BTN_DOWN_PRESSED)
			m_btnMgr.down();
		if ((BTN_A_PRESSED) && !(m_thrdWorking && m_thrdStop))
		{
			if ((m_btnMgr.selected(m_systemBtnDownload)) && !m_thrdWorking)
			{
				// Download selected version
				_hideSystem();
				m_btnMgr.show(m_downloadPBar);
				m_btnMgr.setProgress(m_downloadPBar, 0.f);
				m_thrdStop = false;
				m_thrdWorking = true;
				gprintf("\nVersion to DL: %i\n", newIOS);

				m_app_update_size = m_version.getInt("GENERAL", "app_zip_size", 0);
				m_data_update_size = m_version.getInt("GENERAL", "data_zip_size", 0);

				m_app_update_url = fmt("%s/r%i/%i.zip", m_version.getString("GENERAL", "update_url", "http://update.wiiflow.org").c_str(), newVer, newIOS);
				m_data_update_url = fmt("%s/r%i/data.zip", m_version.getString("GENERAL", "update_url", "http://update.wiiflow.org").c_str(), newVer);

				m_showtimer = 120;
				LWP_CreateThread(&thread, (void *(*)(void *))CMenu::_versionDownloaderInit, (void *)this, 0, 8192, 40);
				if (m_exit && !m_thrdWorking)
				{
					m_thrdStop = true;
					break;
				}
			}
			else if (m_btnMgr.selected(m_systemBtnBack))
			{
				LockMutex lock(m_mutex);
				m_thrdStop = true;
				m_thrdMessageAdded = true;
				m_thrdMessage = _t("dlmsg6", L"Canceling...");
			}
			else if (m_btnMgr.selected(m_systemBtnVerSelectM))
			{
				if (version_num > 1)
					--version_num;
				else
					version_num = num_versions;
				i = min((u32)version_num, ARRAY_SIZE(CMenu::_version) -1u);
				{
					m_btnMgr.setText(m_systemLblVerSelectVal, wstringEx(sfmt("%i", CMenu::_version[i])));
					newVer = CMenu::_version[i];
					if (i > 1 && i != num_versions)
						m_btnMgr.setText(m_systemLblInfo, m_version.getWString(sfmt("VERSION%i", i - 1u), "changes"));
					else
						if (i == num_versions)
							m_btnMgr.setText(m_systemLblInfo, _t("sys7", L"Installed Version."));
						else
							m_btnMgr.setText(m_systemLblInfo, m_version.getWString("GENERAL", "changes"));
				}
			}
			else if (m_btnMgr.selected(m_systemBtnVerSelectP))
			{
				if (version_num < num_versions)
					++version_num;
				else
					version_num = 1;
				i = min((u32)version_num, ARRAY_SIZE(CMenu::_version) -1u);
				{
					m_btnMgr.setText(m_systemLblVerSelectVal, wstringEx(sfmt("%i", CMenu::_version[i])));
					newVer = CMenu::_version[i];
					if (i > 1 && i != num_versions)
						m_btnMgr.setText(m_systemLblInfo, m_version.getWString(sfmt("VERSION%i", i - 1u), "changes"));
					else
						if (i == num_versions)
							m_btnMgr.setText(m_systemLblInfo, _t("sys7", L"Installed Version."));
						else
							m_btnMgr.setText(m_systemLblInfo, m_version.getWString("GENERAL", "changes"));
				}
			}
		}
		if (Sys_Exiting())
		{
			LockMutex lock(m_mutex);
			m_thrdStop = true;
			m_thrdMessageAdded = true;
			m_thrdMessage = _t("dlmsg6", L"Canceling...");
		}
		//
		if (m_thrdMessageAdded)
		{
			LockMutex lock(m_mutex);
			m_thrdMessageAdded = false;
			m_btnMgr.setProgress(m_downloadPBar, m_thrdProgress);
			if (m_thrdProgress == 1.f)
				m_btnMgr.setText(m_systemBtnBack, _t("dl2", L"Back"));
			if (prevMsg != m_thrdMessage)
			{
				prevMsg = m_thrdMessage;
				m_btnMgr.setText(m_downloadLblMessage[msg], m_thrdMessage, false);
				m_btnMgr.hide(m_downloadLblMessage[msg], -200, 0, 1.f, 0.5f, true);
				m_btnMgr.show(m_downloadLblMessage[msg]);
				msg ^= 1;
				m_btnMgr.hide(m_downloadLblMessage[msg], +400, 0, 1.f, 1.f);
			}
		}
		if (m_thrdStop && !m_thrdWorking)
			break;
	}
	if (thread != LWP_THREAD_NULL)
	{
		if(LWP_ThreadIsSuspended(thread))
			LWP_ResumeThread(thread);

		LWP_JoinThread(thread, NULL);
		thread = LWP_THREAD_NULL;
	}
	_hideSystem();
}

void CMenu::_hideSystem(bool instant)
{
	m_btnMgr.hide(m_systemLblTitle, instant);
	m_btnMgr.hide(m_systemLblVersionTxt, instant);
	m_btnMgr.hide(m_systemLblVersion, instant);
	m_btnMgr.hide(m_systemBtnBack, instant);
	m_btnMgr.hide(m_systemBtnDownload, instant);
	m_btnMgr.hide(m_downloadPBar, instant);
	m_btnMgr.hide(m_downloadLblMessage[0], 0, 0, -2.f, 0.f, instant);
	m_btnMgr.hide(m_downloadLblMessage[1], 0, 0, -2.f, 0.f, instant);
	m_btnMgr.hide(m_systemLblInfo);
	m_btnMgr.hide(m_systemLblVerSelectVal);
	m_btnMgr.hide(m_systemBtnVerSelectM);
	m_btnMgr.hide(m_systemBtnVerSelectP);
	for (u32 i = 0; i < ARRAY_SIZE(m_systemLblUser); ++i)
		if (m_systemLblUser[i] != -1u)
			m_btnMgr.hide(m_systemLblUser[i], instant);
}

void CMenu::_showSystem(void)
{
	_setBg(m_systemBg, m_systemBg);
	m_btnMgr.show(m_systemLblTitle);
	m_btnMgr.show(m_systemLblVersionTxt);
	m_btnMgr.show(m_systemLblVersion);
	m_btnMgr.show(m_systemBtnBack);
	m_btnMgr.show(m_systemLblInfo);
	m_btnMgr.show(m_systemLblVerSelectVal);
	m_btnMgr.show(m_systemBtnVerSelectM);
	m_btnMgr.show(m_systemBtnVerSelectP);
	m_btnMgr.show(m_systemBtnDownload);
	for (u32 i = 0; i < ARRAY_SIZE(m_systemLblUser); ++i)
		if (m_systemLblUser[i] != -1u)
			m_btnMgr.show(m_systemLblUser[i]);
	_textSystem();
}

void CMenu::_initSystemMenu(CMenu::SThemeData &theme)
{
	_addUserLabels(theme, m_systemLblUser, ARRAY_SIZE(m_systemLblUser), "SYSTEM");
	m_systemBg = _texture(theme.texSet, "SYSTEM/BG", "texture", theme.bg);
	m_systemLblTitle = _addTitle(theme, "SYSTEM/TITLE", 20, 30, 600, 60, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE);
	m_systemLblVersionTxt = _addLabel(theme, "SYSTEM/VERSION_TXT", 40, 80, 220, 56, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
	m_systemLblVersion = _addLabel(theme, "SYSTEM/VERSION", 260, 80, 200, 56, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_MIDDLE);
	m_systemBtnDownload = _addButton(theme, "SYSTEM/DOWNLOAD_BTN", 20, 410, 200, 56);
	m_systemBtnBack = _addButton(theme, "SYSTEM/BACK_BTN", 420, 410, 200, 56);

	m_systemLblInfo = _addText(theme, "SYSTEM/INFO", 40, 210, 560, 180, FTGX_JUSTIFY_LEFT | FTGX_ALIGN_TOP);
	m_systemLblVerSelectVal = _addLabel(theme, "SYSTEM/VER_SELECT_BTN", 494, 80, 50, 56, FTGX_JUSTIFY_CENTER | FTGX_ALIGN_MIDDLE, theme.btnTexC);
	m_systemBtnVerSelectM = _addPicButton(theme, "SYSTEM/VER_SELECT_MINUS", theme.btnTexMinus, theme.btnTexMinusS, 438, 80, 56, 56);
	m_systemBtnVerSelectP = _addPicButton(theme, "SYSTEM/VER_SELECT_PLUS", theme.btnTexPlus, theme.btnTexPlusS, 544, 80, 56, 56);
	//
	_setHideAnim(m_systemLblTitle, "SYSTEM/TITLE", 0, 100, 0.f, 0.f);
	_setHideAnim(m_systemBtnDownload, "SYSTEM/DOWNLOAD_BTN", 0, 0, 1.f, 0.f);
	_setHideAnim(m_systemBtnBack, "SYSTEM/BACK_BTN", 0, 0, 1.f, 0.f);
	_setHideAnim(m_systemLblVersionTxt, "SYSTEM/VERSION_TXT", -100, 0, 0.f, 0.f);
	_setHideAnim(m_systemLblVersion, "SYSTEM/VERSION", 200, 0, 0.f, 0.f);

	_setHideAnim(m_systemLblInfo, "SYSTEM/INFO", 0, -180, 1.f, -1.f);
	_setHideAnim(m_systemLblVerSelectVal, "SYSTEM/VER_SELECT_BTN", 0, 0, 1.f, -1.f);
	_setHideAnim(m_systemBtnVerSelectM, "SYSTEM/VER_SELECT_MINUS", 0, 0, 1.f, -1.f);
	_setHideAnim(m_systemBtnVerSelectP, "SYSTEM/VER_SELECT_PLUS", 0, 0, 1.f, -1.f);
	//
	_hideSystem(true);
	_textSystem();
}

void CMenu::_textSystem(void)
{
	m_btnMgr.setText(m_systemLblTitle, _t("sys1", L"System"));
	m_btnMgr.setText(m_systemLblVersionTxt, _t("sys2", L"Wiiflow Advanced Version:"));
	m_btnMgr.setText(m_systemLblVersion, wfmt(L"v%s r%s", APP_VERSION, SVN_REV).c_str());
	m_btnMgr.setText(m_systemBtnBack, _t("sys3", L"Cancel"));
	m_btnMgr.setText(m_systemBtnDownload, _t("sys4", L"Upgrade"));
	i = min((u32)version_num, ARRAY_SIZE(CMenu::_version) -1u);
	if (i == 0)
	{
		m_btnMgr.setText(m_systemLblVerSelectVal, wfmt(L"%i", atoi(SVN_REV)).c_str());
	}
	else
	{
		m_btnMgr.setText(m_systemLblVerSelectVal, wstringEx(sfmt("%i", CMenu::_version[i])));
		if (i > 1 && i != num_versions)
			m_btnMgr.setText(m_systemLblInfo, m_version.getWString(sfmt("VERSION%i", i - 1u), "changes"));
		else
			if (i == num_versions)
				m_btnMgr.setText(m_systemLblInfo, _t("sys7", L"Installed Version."));
			else
				m_btnMgr.setText(m_systemLblInfo, m_version.getWString("GENERAL", "changes"));
	}
}