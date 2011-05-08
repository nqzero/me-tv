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

#ifndef __ME_TV_H__
#define __ME_TV_H__

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gtkmm.h>
#include "../common/client.h"
#include "../common/me-tv-types.h"

extern bool				verbose_logging;
extern bool				safe_mode;
extern bool				minimised_mode;
extern bool				disable_epg_thread;
extern bool				disable_epg;
extern bool				no_screensaver_inhibit;
extern String	devices;
extern gint				read_timeout;
extern String	engine_type;
extern String	preferred_language;
extern Client			client;
extern guint			record_extra_before;
extern guint			record_extra_after;

void replace_text(String& text, const String& from, const String& to);
String encode_xml(const String& s);
String trim_string(const String& s);

String get_local_time_text(const gchar* format);
String get_local_time_text(time_t t, const gchar* format);

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);
void start_server(const String& server_host);

extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_fullscreen;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_mute;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_record_current;
extern Glib::RefPtr<Gtk::ToggleAction> toggle_action_visibility;

extern Glib::RefPtr<Gtk::Action> action_about;
extern Glib::RefPtr<Gtk::Action> action_auto_record;
extern Glib::RefPtr<Gtk::Action> action_channels;
extern Glib::RefPtr<Gtk::Action> action_change_view_mode;
extern Glib::RefPtr<Gtk::Action> action_decrease_volume;
extern Glib::RefPtr<Gtk::Action> action_epg_event_search;
extern Glib::RefPtr<Gtk::Action> action_increase_volume;
extern Glib::RefPtr<Gtk::Action> action_preferences;
extern Glib::RefPtr<Gtk::Action> action_present;
extern Glib::RefPtr<Gtk::Action> action_quit;
extern Glib::RefPtr<Gtk::Action> action_restart_server;
extern Glib::RefPtr<Gtk::Action> action_scheduled_recordings;

extern sigc::signal<void, int> signal_start_broadcasting;
extern sigc::signal<void> signal_stop_broadcasting;
extern sigc::signal<void, int> signal_add_scheduled_recording;
extern sigc::signal<void, int> signal_remove_scheduled_recording;

#endif
