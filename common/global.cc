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

ChannelManager				channel_manager;
ScheduledRecordingManager	scheduled_recording_manager;
DeviceManager				device_manager;
StreamManager				stream_manager;
RequestHandler				request_handler;
Glib::RefPtr<Connection>	data_connection;

Glib::ustring			preferred_language;
Glib::StaticRecMutex	mutex;
bool					verbose_logging			= false;
bool					disable_epg_thread		= false;
Glib::ustring			devices;
Glib::ustring			text_encoding;
gboolean				ignore_teletext = true;
Glib::ustring			recording_directory;
int						read_timeout = 5000;
int						server_port = 1999;

