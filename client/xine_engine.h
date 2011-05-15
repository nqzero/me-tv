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

#ifndef __XINE_ENGINE_H__
#define __XINE_ENGINE_H__

#include "engine.h"

#include <xine.h>
#include <X11/X.h>
#include <X11/Xlib.h>

class XineEngine : public Engine
{
private:
	xine_t*				xine;
	xine_stream_t*		stream;
	xine_video_port_t*	video_port;
	xine_audio_port_t*	audio_port;
	xine_event_queue_t*	event_queue;

	int					window;
	Display*			display;
	double				pixel_aspect;
	int 				screen;

public:
	XineEngine();
	~XineEngine();

	void set_mrl(const String& mrl);
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
	void on_expose(GdkEventExpose* event_expose);
	void set_mute_state(gboolean mute);

	Display* get_display() const { return display; }
	int get_window() const { return window; }
	double get_pixel_aspect() const { return pixel_aspect; }
};

#endif
