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

#ifndef __EPG_EVENTS_H__
#define __EPG_EVENTS_H__

#include "data.h"
#include "epg_event.h"

typedef std::list<EpgEvent> EpgEventList;

class EpgEvents
{
public:
	static void			add_epg_event(const EpgEvent& epg_event);
	static void			update_epg_event(const EpgEvent& epg_event);
	static gboolean		get_current(guint channe_id, EpgEvent& epg_event);
	static EpgEventList	get_all(time_t start_time = 0, time_t end_time = -1);
	static EpgEvent		get(int epg_event_id);
	static void			load(Glib::RefPtr<DataModelIter> iter, EpgEvent& epg_event);
	static EpgEventList	search(const String& text, gboolean search_description);
};

#endif
