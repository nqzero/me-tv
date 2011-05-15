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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glibmm.h>
#include "../common/me-tv-types.h"
#include "channel_manager.h"
#include "scheduled_recording_manager.h"
#include "stream_manager.h"
#include "device_manager.h"
#include "data.h"

extern bool									verbose_logging;
extern bool									disable_epg_thread;
extern bool									disable_epg;
extern String						devices;
extern String						preferred_language;
extern int									read_timeout;
extern String						server_address;
extern int									server_port;
extern String						recording_directory;
extern guint								record_extra_before;
extern guint								record_extra_after;
extern sigc::signal<void>					signal_update;
extern sigc::signal<void, String>	signal_error;

extern ChannelManager				channel_manager;
extern ScheduledRecordingManager	scheduled_recording_manager;
extern DeviceManager				device_manager;
extern StreamManager				stream_manager;
extern Glib::RefPtr<Connection>		data_connection;

#endif
