/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Michael Lamothe 2010 <michael.lamothe@gmail.com>
 * 
 * gnome-media-player is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gnome-media-player is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "me-tv-client.h"

extern gboolean next;

class Engine
{
protected:
	String format_2_digit(gint time);
	String format_time(gint time);

public:
	virtual ~Engine();
	
	virtual void set_mrl(const String& mrl) = 0;
	virtual void set_window(int window) = 0;
	virtual void play() = 0;
	virtual void pause(gboolean state) = 0;
	virtual void stop() = 0;
	virtual double get_volume() = 0;
	virtual void set_volume(double value) = 0;
	virtual int get_length() = 0;
	virtual int get_time() = 0;
	virtual void set_time(int time) = 0;
	virtual float get_percentage() = 0;
	virtual void set_percentage(float percentage) = 0;
	virtual void on_expose(GdkEventExpose* event_expose) {};
	virtual void set_mute_state(gboolean mute) = 0;

	void increment(gboolean forward, gboolean sh);
	String get_text();
};

#endif
