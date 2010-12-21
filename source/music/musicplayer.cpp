#include "musicplayer.h"
#include "list/list.hpp"
#include "gecko/gecko.h"

using namespace std;

void MusicPlayer::Init(Config &cfg, string musicDir, string themeMusicDir) 
{
	m_fade_mode = FADE_NONE;
	m_music = NULL;
	m_manual_stop = true;
	m_fade_rate = cfg.getInt("GENERAL", "music_fade_rate", 8);
	m_music_volume = cfg.getInt("GENERAL", "sound_volume_music", 255);

	SetVolume(m_music_volume);
	
	CList::Instance()->GetPaths(m_music_files, ".ogg|.mp3", themeMusicDir); //|.mod|.xm|.s3m", musicDir);
	if (m_music_files.size() == 0)
	{
		CList::Instance()->GetPaths(m_music_files, ".ogg|.mp3", musicDir); //|.mod|.xm|.s3m", musicDir);
	}
	
	if (cfg.getBool("GENERAL", "randomize_music", false) && m_music_files.size() > 0)
	{
		srand(unsigned(time(NULL)));
		random_shuffle(m_music_files.begin(), m_music_files.end());
	}
	m_current_music = m_music_files.begin();
}

MusicPlayer::~MusicPlayer()
{
	if (m_music != NULL)
	{
		m_music->Stop();
		delete m_music;
	}
}

void MusicPlayer::SetFadeMode(Fade mode)
{
	m_fade_mode = mode;
	
	if (m_music_files.empty())
	{
		if (mode == FADE_OUT)
			SetVolume(0);
		else if (mode == FADE_IN)
			SetVolume(m_music_volume);
	}
}

void MusicPlayer::SetVolume(int volume)
{
	m_music_current_volume = volume > m_music_volume ? m_music_volume : volume;
	if (m_music != NULL)
	{
		m_music->SetVolume(volume);
	}
/*	
	SetVolumeOgg(volume);
	MP3Player_Volume(volume);
*/	
}

void MusicPlayer::SetVolume(int volume, int max_volume)
{
	m_music_volume = max_volume;
	SetVolume(volume);
}

void MusicPlayer::Previous()
{
	m_current_music--;
	if (m_current_music == m_music_files.begin())
	{
		m_current_music = m_music_files.end() - 1;
	}

	LoadCurrentFile();
}

void MusicPlayer::Next()
{
	m_current_music++;
	if (m_current_music == m_music_files.end())
		m_current_music = m_music_files.begin();
	
	LoadCurrentFile();
}

void MusicPlayer::Pause()
{
	if (m_music != NULL)
	{
		m_music->Pause();
	}
/*
	if (StatusOgg() == OGG_STATUS_RUNNING) PauseOgg(1);
	MP3Player_Stop();
*/	
}

void MusicPlayer::Play()
{
	m_manual_stop = false; // Next tick will start the music
	if (m_music != NULL)
	{
		m_music->SetVolume(m_music_current_volume);
	}
/*
	if(!m_music_ismp3 && !MP3Player_IsPlaying() && StatusOgg() == OGG_STATUS_PAUSED)
		PauseOgg(0);
	else if (m_music_ismp3)
	{
		if (StatusOgg() == OGG_STATUS_RUNNING) PauseOgg(1);
		MP3Player_PlayBuffer((char *)m_music.get(), m_music_fileSize, NULL);
	}
*/	
}
	
void MusicPlayer::Stop()
{
	m_manual_stop = true;

	if (m_music != NULL)
	{
		m_music->Stop();
		delete m_music;
		m_music = NULL;
	}
}

void MusicPlayer::Tick(bool isVideoPlaying)
{
	if (m_music_files.empty()) return;
	if (m_music_current_volume == 0 && m_fade_mode != FADE_IN) return;

	if (m_music != NULL && m_fade_mode != FADE_NONE)
	{
		if (m_fade_mode == FADE_IN)
		{
			if (m_music->Status() == MUSIC_PAUSED || (!m_manual_stop && m_music->Status() == MUSIC_STOPPED))
			{
				m_music->Play();
			}
			SetVolume(m_music_current_volume + m_fade_rate);
			if (m_music_current_volume >= m_music_volume)
			{
				SetVolume(m_music_volume);
				m_fade_mode = FADE_NONE;
			}
		}
		else if (m_fade_mode == FADE_OUT)
		{
			SetVolume(m_music_current_volume - m_fade_rate);
			if (m_music_current_volume <= 0)
			{
				SetVolume(0);
				m_fade_mode = FADE_NONE;
				m_music -> Pause();
			}
		}
	}
	if (!isVideoPlaying && !m_manual_stop && (m_music == NULL || m_music->Status() == MUSIC_STOPPED))
	{
		// Load next file...
		Next();
//		LoadCurrentFile();
	}
}

void MusicPlayer::LoadCurrentFile()
{
	if (m_music_files.empty()) return;

	if (m_music != NULL)
	{
		m_music->Stop();
		delete m_music;
		m_music = NULL;
	}
	
	m_music = MusicFile::Load((*m_current_music));
	if (m_music != NULL && !m_manual_stop)
	{
		m_music->SetVolume(m_music_current_volume);
		m_music->Play();
	}
}