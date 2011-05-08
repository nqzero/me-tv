/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Michael Lamothe 2010 <michael.lamothe@gmail.com>
 * 
 * Me TV is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Me TV is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vlc_engine.h"
#include "../common/exception.h"
#include "me-tv.h"
#include "../common/i18n.h"

#define DEFAULT_VOLUME	130

void end_reached_callback(const libvlc_event_t* event, void* data)
{
	// This happens out of GDK threads
	next = true;
}

VlcEngine::VlcEngine(bool use_ffmpeg_demux)
{
	instance		= NULL;
	media_player	= NULL;

	int i = 0;
	const char * vlc_argv[50];

	vlc_argv[i++] = "-I", "dummy";
	vlc_argv[i++] = "--ignore-config";
	vlc_argv[i++] = "--verbose=0";
	vlc_argv[i++] = "--no-osd";
	vlc_argv[i++] = "--file-caching=2000";
	vlc_argv[i++] = "--alsa-caching=2000";
	vlc_argv[i++] = "--udp-caching=2000";
	vlc_argv[i++] = "--no-disable-screensaver";

	if (use_ffmpeg_demux)
	{
		g_debug("Using FFMpeg demux");
		vlc_argv[i++] = "--demux=ffmpeg";
	}

	if (true)
	{
		g_debug("Enabling deinterlacer");
		vlc_argv[i++] = "--video-filter=deinterlace";
		vlc_argv[i++] = "--deinterlace-mode=blend";
	}
	else
	{
		g_debug("Deinterlacer disabled");
	}

	instance = libvlc_new (i, vlc_argv);
	
	media_player = libvlc_media_player_new(instance);
	libvlc_video_set_mouse_input(media_player, false);
	libvlc_audio_set_volume(media_player, DEFAULT_VOLUME);

	event_manager = libvlc_media_player_event_manager(media_player);
	libvlc_event_attach(event_manager, libvlc_MediaPlayerEndReached, end_reached_callback, NULL);
}

VlcEngine::~VlcEngine()
{
	g_debug("Destroying VLC engine");
	
	stop();
	
	if (media_player != NULL)
	{
		libvlc_media_player_release(media_player);
		media_player = NULL;
	}

	if (instance == NULL)
	{
		libvlc_release(instance);
		instance = NULL;
	}

	g_debug("VLC engine destroyed");
}

void VlcEngine::set_window(int window)
{
	libvlc_media_player_set_xwindow(media_player, window);
}

void VlcEngine::pause(gboolean state)
{
	if (has_media())
	{
		if (state)
		{
			libvlc_media_player_pause(media_player);
		}
		else
		{
			libvlc_media_player_play(media_player);
		}
	}
}

void VlcEngine::play()
{
	if (has_media())
	{
		libvlc_media_player_play(media_player);
	}
}

void VlcEngine::stop()
{
	if (has_media())
	{
		libvlc_media_t* media = libvlc_media_player_get_media(media_player);
		libvlc_media_player_stop(media_player);
		libvlc_media_player_set_media(media_player, NULL);
		libvlc_media_release(media);
	}
}

void VlcEngine::set_mrl(const Glib::ustring& mrl)
{	
	if (mrl.empty())
	{
		throw Exception(_("MRL not specified"));
	}

	stop();
	
	libvlc_media_t* media = libvlc_media_new_path(instance, mrl.c_str());
	libvlc_media_player_set_media(media_player, media);
	libvlc_media_release(media);
}

void VlcEngine::set_time(int time)
{
	if (has_media())
	{
		int new_time = time;
	
		if (new_time < 0)
		{
			new_time = 0;
		}
		else if (new_time > get_length())
		{
			next = true;
		}
		else
		{
			libvlc_media_player_set_time(media_player, new_time);
		}
	}
}

bool VlcEngine::has_media()
{
	libvlc_media_t* media = libvlc_media_player_get_media(media_player);
	return media != NULL;
}

int VlcEngine::get_time()
{
	int result = 0;

	if (has_media())
	{
		result = libvlc_media_player_get_time(media_player);
	}
	
	return result;
}

int VlcEngine::get_length()
{
	int result = 0;

	if (has_media())
	{
		result = libvlc_media_player_get_length(media_player);
	}
	
	return result;
}

float VlcEngine::get_percentage()
{
	float result = 0;
	
	if (has_media())
	{
		result = libvlc_media_player_get_position(media_player);
	}
	
	return result;
}

void VlcEngine::set_percentage(float percentage)
{
	if (has_media())
	{
		libvlc_media_player_set_position(media_player, percentage);
	}
}

void VlcEngine::set_volume(double value)
{
	if (has_media())
	{
		libvlc_audio_set_volume(media_player, value);
	}
}

double VlcEngine::get_volume()
{
	return has_media() ? libvlc_audio_get_volume(media_player) : 0;
}

void VlcEngine::set_mute_state(gboolean mute)
{
	if (has_media())
	{
		libvlc_audio_set_mute(media_player, mute);
	}
}
