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

#ifndef __VLC_ENGINE_H__
#define __VLC_ENGINE_H__

#include <vlc/vlc.h>
#include <glibmm.h>
#include "engine.h"

class VlcEngine : public Engine
{
private:
	libvlc_instance_t*		instance;
	libvlc_media_player_t*	media_player;
	libvlc_event_manager_t* event_manager;
	
public:
	VlcEngine(bool use_ffmpeg_demux = false);
	~VlcEngine();

	void set_mrl(const Glib::ustring& mrl);
	void set_window(int window);
	void play();
	void pause(gboolean state);
	void stop();

	bool has_media();
	int get_length();
	int get_time();
	double get_volume();
	void set_volume(double value);
	void set_time(int time);
	float get_percentage();
	void set_percentage(float percentage);
	void set_mute_state(gboolean mute);

	Glib::ustring get_time_text();
};

#endif
