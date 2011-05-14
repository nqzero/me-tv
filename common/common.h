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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <glibmm.h>
#include "channel_manager.h"
#include "scheduled_recording_manager.h"
#include "device_manager.h"
#include "stream_manager.h"

extern bool							verbose_logging;
extern bool							disable_epg_thread;
extern bool							disable_epg;
extern String						devices;
extern String						preferred_language;
extern int							read_timeout;
extern String						recording_directory;
extern guint						record_extra_before;
extern guint						record_extra_after;
extern gboolean						ignore_teletext;
extern String						broadcast_address;
extern String						text_encoding;

extern DeviceManager				device_manager;
extern StreamManager				stream_manager;
extern Glib::RefPtr<Connection>		data_connection;

extern sigc::signal<void>			signal_update;
extern sigc::signal<void, String>	signal_error;
extern sigc::signal<void>			signal_quit;

void replace_text(String& text, const String& from, const String& to);
String get_local_time_text(const gchar* format);
String get_local_time_text(time_t t, const gchar* format);
String encode_xml(const String& s);
String trim_string(const String& s);

void write_string(int fd, const String& data);
String read_string(int fd);

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);
void handle_error();
void on_error(const String& message);
void make_directory_with_parents(const String& path);

#endif
