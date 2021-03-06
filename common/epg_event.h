/*
 * Copyright (C) 2011 Michael Lamothe
 *
 * This file is part of Me TV
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __EPG_EVENT_H__
#define __EPG_EVENT_H__

#include "me-tv-types.h"

class EpgEventText
{
public:
	guint			id;
	guint			epg_event_id;
	String	language;
	String	title;
	String	subtitle;
	String	description;
		
	EpgEventText();
};

typedef std::list<EpgEventText> EpgEventTextList;

class EpgEvent
{
public:
	guint				id;
	guint				channel_id;
	guint				version_number;
	guint				event_id;
	time_t				start_time;
	guint				duration;
	gboolean			save;
	EpgEventTextList	texts;
	
	EpgEvent();
		
	time_t get_end_time() const { return start_time + duration; }
	EpgEventText get_default_text(const String& preferred_language = "") const;
	String get_title() const;
	String get_subtitle() const;
	String get_description() const;
	String get_start_time_text() const;
	String get_duration_text() const;
};

#endif
