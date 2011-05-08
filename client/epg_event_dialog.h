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

#ifndef __EPG_EVENT_DIALOG_H__
#define __EPG_EVENT_DIALOG_H__

#include "me-tv-ui.h"
#include "../common/client.h"

class EpgEventDialog : public Gtk::Dialog
{
private:
	const Glib::RefPtr<Gtk::Builder> builder;
public:	
	EpgEventDialog(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

	static EpgEventDialog& create(Glib::RefPtr<Gtk::Builder> builder);

	void show_epg_event(Client::EpgEvent& epg_event);
};

#endif
