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

#include "me-tv.h"
#include "../common/i18n.h"
#include "../common/exception.h"
#include "../common/common.h"
#include <glib/gprintf.h>

bool	disable_epg				= false;
bool	safe_mode				= false;
bool	minimised_mode			= false;
bool	no_screensaver_inhibit	= false;
String	engine_type				= "vlc";
Client	client;

Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_record_current;
Glib::RefPtr<Gtk::ToggleAction> toggle_action_visibility;

Glib::RefPtr<Gtk::Action> action_about;
Glib::RefPtr<Gtk::Action> action_auto_record;
Glib::RefPtr<Gtk::Action> action_channels;
Glib::RefPtr<Gtk::Action> action_change_view_mode;
Glib::RefPtr<Gtk::Action> action_decrease_volume;
Glib::RefPtr<Gtk::Action> action_epg_event_search;
Glib::RefPtr<Gtk::Action> action_increase_volume;
Glib::RefPtr<Gtk::Action> action_preferences;
Glib::RefPtr<Gtk::Action> action_present;
Glib::RefPtr<Gtk::Action> action_quit;
Glib::RefPtr<Gtk::Action> action_restart_server;
Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

sigc::signal<void, int> signal_start_broadcasting;
sigc::signal<void> signal_stop_broadcasting;
sigc::signal<void, int> signal_add_scheduled_recording;
sigc::signal<void, int> signal_remove_scheduled_recording;

void start_server(const String& server_host)
{
	if (server_host != "localhost" && server_host != "127.0.0.1")
	{
		throw Exception("Can only start a local server");
	}

	Glib::spawn_command_line_async("me-tv-server");
}

