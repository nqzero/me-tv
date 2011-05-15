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

#ifndef __GSTREAMER_ENGINE_H__
#define __GSTREAMER_ENGINE_H__

#include "engine.h"
#include <gst/gstelement.h>
#include <gst/video/video.h>

class GStreamerEngine : public Engine
{
private:
	GstElement* playbin;
	GstElement* videosink;
public:
	GStreamerEngine();
	~GStreamerEngine();

	void set_mrl(const String& mrl);
	void set_window(int window);
	void play();
	void pause(gboolean state);
	void stop();
	double get_volume();
	void set_volume(double value);
	int get_length();
	int get_time();
	void set_time(int time);
	float get_percentage();
	void set_mute_state(gboolean mute);	
	void set_percentage(float percentage);
};

#endif
